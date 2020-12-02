/* =======================BLEHandler.c===============================
Created by: Benjamin Fa, Faiyaz Mostofa, Melissa Yang, and Yongye Zhu
EE445L Fall 2020 for McDermott, Mark 

BLE handler
===================================================================== */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "BLEHandler.h"
#include "../inc/user.h"
#include "../inc/UART1int.h"
#include "../inc/tm4c123gh6pm.h"
#include "./BGLib/sl_bt_api.h"
#include "./BGLib/sl_bt_ncp_host.h"
#include "../inc/ST7735.h"
#include "Display.h"

#define gattdb_device_name 				11
#define gattdb_contact_user 			21
#define gattdb_data_ready 				23
#define gattdb_fake_device_name 	27
#define gattdb_date 							29
#define gattdb_time								31


SL_BT_API_DEFINE();
static void sl_bt_on_event(sl_bt_msg_t* evt);
static void uart_tx_wrapper(uint32_t len, uint8_t* data);
static int32_t uartRx(uint32_t len, uint8_t* data);
static int32_t uartRxPeek(void);

static struct sl_bt_evt_scanner_scan_report_s report;
static const int8_t MIN_RSSI = -45;

#define CONTACT_LIST_SIZE 256
user_profile_t Contacts[CONTACT_LIST_SIZE];
uint16_t CurContactIdx;

static char message[100];

const char default_device_name[] = "D1";

void BLEHandler_Init(void) {
	SL_BT_API_INITIALIZE_NONBLOCK(uart_tx_wrapper, uartRx, uartRxPeek);
	UART1_Init();
	ST7735_OutString("EE445L Final\nInitializing BLE...");
	CurContactIdx = 0;
	
	sl_bt_system_reset(0);
}

void BLEHandler_Main_Loop(void){
	sl_bt_msg_t evt;
	
	sl_status_t status = sl_bt_pop_event(&evt);
	if(status != SL_STATUS_OK) return;
	sl_bt_on_event(&evt);
}

//****************************************//
//        Helper Functions                //
//****************************************//


const uint8_t PROFILE_SIZE = 10;

user_profile_t dummy_profile[PROFILE_SIZE] = {
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x30}, {9,  20, 20}, {12, 20}}, 
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x31}, {10, 20, 20}, {12, 20}}, 
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x32}, {11, 20, 20}, {12, 20}}, 
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x33}, {12, 20, 20}, {12, 20}},
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x34}, {1, 20, 20}, {12, 20}},
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x35}, {2, 20, 20}, {12, 20}},
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x36}, {3, 20, 20}, {12, 20}},
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x37}, {4, 20, 20}, {12, 20}},
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x38}, {5, 20, 20}, {12, 20}},
{{0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x39}, {6, 20, 20}, {12, 20}}
};

static bool existingContact(uint8_t* data, uint8_t data_len, uint8_t* index){
	bool existingContact;
	for(int i = 0; i < PROFILE_SIZE; i++){
		existingContact = true;
		for(int j = 0; j < data_len - 11; j++){
			if(data[j + 11] != Contacts[i].name[j]) existingContact = false;
		}
		if(existingContact){ 
			*index = i;
			return true; 
		}
	}
	return false;
}

extern int time;
extern uint8_t month;
extern uint8_t day;
extern uint8_t year;

static void parseAndAdd(uint8_t* data, uint8_t len){
	for(uint8_t i = 0; i < len - 11; i++) Contacts[CurContactIdx].name[i] = data[i + 11];
	Contacts[CurContactIdx].name[len - 11] = 0;
	Contacts[CurContactIdx].startDate[0] = month;
	Contacts[CurContactIdx].startDate[1] = day;
	Contacts[CurContactIdx].startDate[2] = year;
	Contacts[CurContactIdx].startTime[0] = time % 100;
	Contacts[CurContactIdx].startTime[1] = time / 100;
	Display_NewProfile(&Contacts[CurContactIdx]);
	CurContactIdx = (CurContactIdx + 1) % PROFILE_SIZE;
}

static bool validBLE(int8_t rssi, uint8_t *data){
	if(rssi < MIN_RSSI){ return false; }
//	if(data.len < 18){ return false; }
	return data[3] == 0x05 
			&& data[4] == 0xFF 
			&& data[5] == 0xFF 
			&& data[6] == 0x02 
			&& data[7] == 0x00 
			&& data[8] == 0xFF;
}

//****************************************//
//        Event Handler                   //
//****************************************//
static void sl_bt_on_event(sl_bt_msg_t* evt){
	sl_status_t sc;
	
	bd_addr address;
	uint8_t address_type;
	uint8_t system_id[8];
	
	static uint8_t advertising_set_handle = 0xff;
	static uint8_t adv_data_len = 17;
	static uint8_t adv_data[31]={
		0x02, 0x01, 0x06, // flags
		0x05, 0xFF, 0xFF, 0x02, 0x00, 0xFF, // specific data (identifier: 0x00FF)
		0x07, 0x08, 0x44, 0x65, 0x76, 0x69, 0x63, 0x65 // shortened local name
	};
	static int profile_index = 0;
	 
	
	switch(SL_BT_MSG_ID(evt->header)){
		case sl_bt_evt_system_boot_id:{
			ST7735_OutString("Connection Success");
			sprintf(message, "Bluttooth Stack \nBooted:\n v%d.%d.%d-b%d\n", evt->data.evt_system_boot.major, evt->data.evt_system_boot.minor, 
																											evt->data.evt_system_boot.patch, evt->data.evt_system_boot.build);
			ST7735_OutString(message);
			sc = sl_bt_system_hello();
			if(sc != SL_STATUS_OK){
				ST7735_OutString("Connection Failed");
			}
			sc = sl_bt_system_get_identity_address(&address, &address_type);
			if(sc != SL_STATUS_OK){
				ST7735_OutString("Failed to get address");
				break;
			}
			sprintf(message, "%s Address:\n %02X:%02X:%02X:%02X:%02X:%02X\n", address_type? "static random": "public device", address.addr[5], address.addr[4], address.addr[3], address.addr[2], address.addr[1], address.addr[0]);
			ST7735_OutString(message);
			

			sc = sl_bt_gatt_server_write_attribute_value(gattdb_device_name, 0, strlen(default_device_name) , default_device_name);
			if(sc != SL_STATUS_OK){
				ST7735_OutString("Failed to set attribute\n");
			}
			
			adv_data_len = 11 + strlen(default_device_name);
			adv_data[9] = strlen(default_device_name) + 1;
			memcpy(adv_data + 11, default_device_name, strlen(default_device_name));
				
			// Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
			if (sc != SL_STATUS_OK){
				ST7735_OutString("Failed to create advertising set\n");
				break;
			}
			
			
		 // Set advertising data
			sc = sl_bt_advertiser_set_data(advertising_set_handle, 0, adv_data_len, adv_data);
			if (sc != SL_STATUS_OK){
				ST7735_OutString("Failed to set advertising data\n");
				break;
			}
			
      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
			if (sc != SL_STATUS_OK){
				ST7735_OutString("Failed to set \nadvertising timing\n");
				break;
			}
				
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_user_data,
        advertiser_connectable_scannable);
			if (sc != SL_STATUS_OK){
				ST7735_OutString("Failed to start advertising\n");
				break;
			}
					
			ST7735_OutString("BLE Successfully inisialized...\n");
			//for(int i = 0; i < 99999999; i++);
			ST7735_FillScreen(ST7735_BLACK);	
			Display_Time(time, month, day, year);
			Display_DeviceName(default_device_name, strlen(default_device_name));
			Display_ToggleConnection(0);
			Display_ProfileHeader();
			
			// Start scanning
			sc = sl_bt_scanner_start(1, 1);
			if(sc != SL_STATUS_OK){
				ST7735_OutString("Failed to start scanning\n");
			}
			
			break;
		}
		case sl_bt_evt_connection_opened_id:{
			// ST7735_OutString("New Connection Opened\n");
			sc = sl_bt_scanner_stop();
			if(sc != SL_STATUS_OK){
				ST7735_OutString("Failed to stop scanning\n");
			}
			Display_ToggleConnection(1);
			profile_index = 0;
			break;
		}
		case sl_bt_evt_connection_closed_id:{
			// ST7735_OutString("Connection Closed\n");
      // Start general advertising and enable connections.
      sc = sl_bt_advertiser_start(
        advertising_set_handle,
        advertiser_user_data,
        advertiser_connectable_scannable);
			if (sc != SL_STATUS_OK){
				ST7735_OutString("Failed to start advertising\n");
				break;
			}
			sc = sl_bt_scanner_start(1, 1);
			if(sc != SL_STATUS_OK){
				ST7735_OutString("Failed to start scanning\n");
			}
			Display_ToggleConnection(0);
			break;
		}
		
		case sl_bt_evt_gatt_server_attribute_value_id:{
			switch (evt->data.evt_gatt_server_attribute_value.attribute){
				case gattdb_fake_device_name: {
					// ST7735_OutString("Device Name Changed\n");
					size_t value_len;
					const size_t max_length = 15;
					uint8_t value[16];
					sc = sl_bt_gatt_server_read_attribute_value(gattdb_fake_device_name, 0, max_length, &value_len, value);
					if(sc != SL_STATUS_OK){
						ST7735_OutString("Failed to get device name\n");
					}	
					sc = sl_bt_gatt_server_write_attribute_value(gattdb_device_name, 0, value_len , value);
					if(sc != SL_STATUS_OK){
						ST7735_OutString("Failed to set attribute\n");
					}
					adv_data_len = 11 + value_len;
					adv_data[9] = value_len + 1;
					memcpy(adv_data + 11, value, value_len);					
					sc = sl_bt_advertiser_set_data(advertising_set_handle, 0, adv_data_len, adv_data);
					if (sc != SL_STATUS_OK){
						ST7735_OutString("Failed to set advertising data\n");
						break;
					}
					Display_DeviceName(value, value_len);
					break;
				}
				case gattdb_data_ready: {
					if(profile_index == CurContactIdx){
						return;
					}
					size_t name_len = strlen(Contacts[profile_index].name);
					uint8_t ready = 1;
					uint16_t sent_len;
					uint8_t user_profile[20];
					memcpy(user_profile, Contacts[profile_index].name, name_len);
					memcpy(user_profile + name_len, Contacts[profile_index].startDate, 3);
					memcpy(user_profile + name_len + 3, Contacts[profile_index].startTime, 2);
					sc = sl_bt_gatt_server_write_attribute_value(gattdb_contact_user, 0, name_len + 5, user_profile);
					if(sc != SL_STATUS_OK){
						ST7735_OutString("Fail to write user profile\n");	
					}			
					profile_index ++;
					
					sc = sl_bt_gatt_server_send_characteristic_notification(0xff, gattdb_data_ready, 1, &ready, &sent_len);
					if(sc != SL_STATUS_OK){
						ST7735_OutString("Fail to write user profile\n");	
					}
					break;
				}
				case gattdb_date:{
					size_t value_len;
					uint8_t value[3];
					sc = sl_bt_gatt_server_read_attribute_value(gattdb_date, 0, 3, &value_len, value);
					if(sc != SL_STATUS_OK){
						ST7735_OutString("Failed to get date information\n");
						break;
					}
					month = value[0];
					day = value[1];
					year = value[2];
					Display_Time(time, month, day, year);				
					break;
				}
				case gattdb_time:{
					size_t value_len;
					uint8_t value[2];
					sc = sl_bt_gatt_server_read_attribute_value(gattdb_time, 0, 2, &value_len, value);
					if(sc != SL_STATUS_OK){
						ST7735_OutString("Failed to get time information\n");
						break;
					}
					time = value[1] * 100 + value[0];
					Display_Time(time, month, day, year);
					break;
				}
				default:
					break;			
			}
			break;
		}
		
		case sl_bt_evt_scanner_scan_report_id:{
			report = evt->data.evt_scanner_scan_report;
			int8_t rssi = report.rssi;
			uint8_t len = report.data.len;
			uint8_t value_raw[31];
			for(uint8_t i = 0; i < len; i++){
				value_raw[i] = evt->data.evt_scanner_scan_report.data.data[i];
			}
			
			if (validBLE(rssi, value_raw)){
				uint8_t index;
				if(existingContact(value_raw, len, &index)){
					Contacts[index].endDate[0] = month;
					Contacts[index].endDate[1] = day;
					Contacts[index].endDate[2] = year;
					Contacts[index].endTime[0] = time % 100;
					Contacts[index].endTime[1] = time / 100;
				}
				else{
					parseAndAdd(value_raw, len);
				}
			}
			break;
		}
		default:
			break;
	}
	
}



//****************************************//
//        UART_TX_WRAPPER                 //
//****************************************//
static void uart_tx_wrapper(uint32_t len, uint8_t* data){
	for(int i = 0; i < len; i++){
		UART1_OutChar(data[i]);
	}
}

//****************************************//
//        UART_RX                         //
//****************************************//
static int32_t uartRx(uint32_t len, uint8_t* data){
		int32_t total = 0;
		for(int i = 0; i < len; i++){
			data[i] = UART1_InChar();
			total++;
		}
		return total;
}
//****************************************//
//        UART_RX_PEEK                    //
//****************************************//
static int32_t uartRxPeek(void){
	return (int32_t)UART1_InStatus();
}



