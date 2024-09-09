/*******************************************************************************
* Project: RedEye                                                              *
* Filename: /src/RedEyeBTS.h                                                   *
*                                                                              *
* Created: 2024-08-31                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see RedEye/LICENSE (if applicable)                               *
*                                                                              *
*******************************************************************************/

#pragma once
#include "RedEye.h"

#ifndef REDEYE_USE_TIMER
	#define REDEYE_USE_TIMER 1
#endif

#if REDEYE_USE_TIMER != 1
	#warning "RedEye has only been tested w/ Timer 1! Here there be dragons!"
#endif

#define __REDEYE_ID(x, y, z) x ## y ## z
#define _REDEYE_ID(x, y, z) __REDEYE_ID(x, y, z)
#define REDEYE_ID(x, y) _REDEYE_ID(x, REDEYE_USE_TIMER, y)

#define REDEYE_16BIT REDEYE_ID(TCNT,H)


#if defined(REDEYE_16BIT)
	#define REDEYE_TIMER_MAX UINT16_MAX
#else
	#define REDEYE_TIMER_MAX UINT8_MAX
#endif
#define REDEYE_PULSE_LEN F_CPU/32768U
#if REDEYE_PULSE_LEN >= REDEYE_TIMER_MAX
	#error "Unsupported timer / clock speed combination!"
#endif


#define REDEYE_PWM_REG REDEYE_ID(OCR,A)
#define REDEYE_PWM_EN_BIT REDEYE_ID(OCIE,A)
#define REDEYE_PWM_VECT REDEYE_ID(TIMER,_COMPA_vect)
#define REDEYE_PULSE_REG REDEYE_ID(OCR,B)
#define REDEYE_PULSE_EN_BIT REDEYE_ID(OCIE,B)
#define REDEYE_PULSE_VECT REDEYE_ID(TIMER,_COMPB_vect)
#define REDEYE_TCCRA REDEYE_ID(TCCR,A)
#define REDEYE_TCCRB REDEYE_ID(TCCR,B)
#ifdef REDEYE_ID(TCCR,C)
	#define REDEYE_TCCRC REDEYE_ID(TCCR,C)
#endif
#define REDEYE_ICR REDEYE_ID(ICR, )
#define REDEYE_TIMSK REDEYE_ID(TIMSK, )



namespace redeye {

extern volatile uint8_t rxBuffer[REDEYE_RX_BUFFER_SIZE];
extern uint8_t rxReadIndex;
extern volatile uint8_t rxWriteIndex;
extern volatile bool rxBurstRecieved;

extern uint16_t txBuffer[REDEYE_TX_BUFFER_SIZE]; // RedEye bytes are 11 bits. Rip RAM usage
extern volatile uint8_t txReadIndex;
extern uint8_t txWriteIndex;
extern bool txSlowMode;

extern Mode mode;

bool calculateParity(unsigned);

void rxPulse();
bool addToRxBuffer(byte);
void rxInterruptHandler();
void rxByteFinished();
void rxBitFinished();
void rxHalfBitFinished();
void rxEndOfBurst();

void txPulse();
void txSendBurst();
void txBitInterrupt();
void txLoadNextByte();
void txUpdateLineTimes();

}