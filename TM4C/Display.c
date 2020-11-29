/* =======================Display.c===================================
Created by: Benjamin Fa, Faiyaz Mostofa, Melissa Yang, and Yongye Zhu
EE445L Fall 2020 for McDermott, Mark 

Display interface
===================================================================== */

#include <stdint.h>
#include <stdio.h>
#include "Display.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/ST7735.h"

static uint32_t CurX, CurY;
extern uint32_t StX, StY;

static void moveCursor() {
	CurX = 0;
	if (CurY + 1 > 15) {
		ST7735_FillScreen(ST7735_BLACK);
	}
	CurY = (CurY + 1) % 16;
	ST7735_SetCursor(CurX, CurY);
}

void DisplaySend_String(char *string) {
	ST7735_OutString(string);
	moveCursor();
}

void DisplaySend_Integer(int number) {
	if (number < 0)
		ST7735_OutString("-");
	ST7735_OutUDec((uint32_t)number);
	moveCursor();
}

void Display_Init() {
	CurX = 0;
	CurY = 0;
	Output_Init();
	ST7735_SetCursor(0, 0);
	ST7735_SetTextColor(ST7735_GREEN);
	DisplaySend_String("UART Log:");
}

void Display_Time(int time, uint8_t month, uint8_t day, uint8_t year){
	uint32_t setx = StX;
	uint32_t sety = StY;
	static char current_time[20];
	ST7735_SetCursor(0, 0);
	
	sprintf(current_time, "%02d:%02d %02d/%02d/%02d", time/100, time % 100, month, day, year);
	ST7735_OutString(current_time);
	StX = setx;
	StY = sety;
}


void Display_NewProfile(user_profile_t *user){
	static char message[20];
	sprintf(message, "%s: %02d:%02d %02d/%02d\n", user->name, user->startTime[1], user->startTime[0], user->startDate[0], user->startDate[1]);
	ST7735_OutString(message);

}
