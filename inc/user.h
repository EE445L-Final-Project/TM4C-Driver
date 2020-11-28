/* =======================user.h=====================================
Created by: Benjamin Fa, Faiyaz Mostofa, Melissa Yang, and Yongye Zhu
EE445L Fall 2020 for McDermott, Mark 

A class to manage modification of the user profile of the current device owner. 
===================================================================== */

#ifndef USER_H
#define USER_H

#include <stdint.h>

/** User profile thru a single encounter*/
typedef struct _user_profile{
	uint8_t name[7];
	uint8_t startDate[3];
	uint8_t endDate[3];
	uint8_t startTime[2];
	uint8_t endTime[2];
} user_profile_t;

#endif // USER_H
