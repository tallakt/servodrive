/*
 * Copyright (C) Chris Desjardins 2008 - code@chrisd.info
 */
#ifndef SERVOSOC_Hxx
#define SERVOSOC_Hxx

#define SERVOSOC_MAX_SERVOS     8
#define SERVOSOC_SERVO_INVALID -1
#define SERVOSOC_SERVO_A        0
#define SERVOSOC_SERVO_B        1
#define SERVOSOC_SERVO_C        2
#define SERVOSOC_SERVO_D        3
#define SERVOSOC_SERVO_E        4
#define SERVOSOC_SERVO_F        5
#define SERVOSOC_SERVO_G        6
#define SERVOSOC_SERVO_H        7
#define SERVOSOC_PORT           42673
typedef struct SServoData
{
    int m_nJoyValue;
    int m_nServoNumber;
} TServoData;

#endif
