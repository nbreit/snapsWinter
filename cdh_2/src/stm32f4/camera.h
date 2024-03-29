#pragma once
#include "global.h"

bool Camera_Cmd(FunctionalState);
void Camera_Record_Snippet(uint32_t timeToRecord);
static void Camera_Enable_Standby(void);
static void Camera_Disable_Standby(void);
bool Camera_Power_Good(void);
uint8_t Camera_Is_Enabled(void);