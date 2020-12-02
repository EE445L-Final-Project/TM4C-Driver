/* =======================Display.c===================================
Created by: Benjamin Fa, Faiyaz Mostofa, Melissa Yang, and Yongye Zhu
EE445L Fall 2020 for McDermott, Mark 

Display interface
===================================================================== */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "Display.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"

extern uint32_t StX, StY;


void Display_Init() {
	Output_Init();
	ST7735_SetCursor(0, 0);
	ST7735_SetTextColor(ST7735_GREEN);
	ST7735_OutString("UART Log:\n");
}

void Display_Time(int time, uint8_t month, uint8_t day, uint8_t year){
	uint32_t setx = StX;
	uint32_t sety = StY;
	static char current_time[20];
	ST7735_SetCursor(3, 0);
	
	sprintf(current_time, "%02d:%02d   %02d/%02d/%02d", time/100, time % 100, month, day, year);
	ST7735_OutString(current_time);
	StX = setx;
	StY = sety;
}

void Display_ProfileHeader(void){
	ST7735_SetCursor(0, 4);
	ST7735_OutString("Name     Time  Date\n");
}

void Display_NewProfile(user_profile_t *user){
	char name[10];
	if(strlen(user->name) > 8){
		memcpy(name, user->name, 8);
		name[8] = '.';
		name[9] = 0;
	}else{
		memcpy(name, user->name, strlen(user->name));
		for(int i = strlen(user->name); i < 7; i++) name[i] = ' ';
		name[7] = 0;
	}
	static char message[20];
	ST7735_OutString(name);
	sprintf(message, "  %02d:%02d %02d/%02d\n", user->startTime[1], user->startTime[0], user->startDate[0], user->startDate[1]);
	ST7735_OutString(message);
}

void Display_DeviceName(uint8_t* name, uint8_t len){
	uint32_t setx = StX;
	uint32_t sety = StY;
	name[len] = 0;
	ST7735_SetCursor(0, 1);
	ST7735_OutString("Name: ");
	ST7735_OutString(name);
	for(int i = 0; i < 14 - len; i++) ST7735_OutString(" ");
	StX = setx;
	StY = sety;
}

void Display_ToggleConnection(uint8_t state){
	uint32_t setx = StX;
	uint32_t sety = StY;
	ST7735_SetCursor(0, 2);
	ST7735_OutString("Phone Connect: ");
	if(state){
		ST7735_OutString("On ");
	}else{
		ST7735_OutString("Off");
	}
	StX = setx;
	StY = sety;
}
