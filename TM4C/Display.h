/* =======================display_handler.h==========================
Created by: Benjamin Fa, Faiyaz Mostofa, Melissa Yang, and Yongye Zhu
EE445L Fall 2020 for McDermott, Mark 

The high level interface for sending debugging data to the ST7735 LCD display.
The LCD is only used for debugging purposes because it requires a lot of current (and
thus a lot of power).
===================================================================== */

#ifndef DISPLAY_INTERFACE_H
#define DISPLAY_INTERFACE_H

#include <stdint.h>
#include "../inc/user.h"

/** Initialize Port A for the display. */
void Display_Init(void);

// Send data ============================================
void Display_Time(int time, uint8_t month, uint8_t day, uint8_t year);

void Display_NewProfile(user_profile_t *user);

void Display_DeviceName(uint8_t* name, uint8_t len);

void Display_ToggleConnection(uint8_t state);

void Display_ProfileHeader(void);

#endif // DISPLAY_INTERFACE_H
