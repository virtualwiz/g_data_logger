#ifndef __MPU6050_H__
#define __MPU6050_H__
#include "sys.h"

#define q30  1073741824.0f

extern float Pitch,Roll,Yaw;
extern short G0,G1,G2;
extern short A0,A1,A2;

void MPU6050_Init(void);
void MPU6050_Pose(void);

#endif
