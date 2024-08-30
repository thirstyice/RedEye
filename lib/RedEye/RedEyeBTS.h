/*******************************************************************************
* Project: HP Printer Tester                                                   *
* Filename: /lib/RedEye/RedEyeVars.h                                           *
*                                                                              *
* Created: 2024-08-30                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see HP Printer Tester/LICENSE (if applicable)                    *
*                                                                              *
*******************************************************************************/
#pragma once
#include "RedEye.h"

#if REDEYE_USE_TIMER != 1
	#error "Only timer 1 is currently supported!"
#else
	#ifdef TCNT1H
		#define REDEYE_TIMER_IS_16_BIT
		#if F_CPU == 16000000L
			#define REDEYE_PULSE_LEN 488 // 16MHz / 488 cycles ~= 32768Hz
		#else
			#warning "CPU Frequencies other than 16MHz are untested. Good luck!"
			#define REDEYE_SCALE_FACTOR F_CPU/32768U
		#endif
	#else
		#define REDEYE_PULSE_LEN 61
		#error "Only 16 bit timers currently supported!"
	#endif
	#define REDEYE_PWM_REG OCR1A
	#define REDEYE_PWM_EN_BIT OCIE1A
	#define REDEYE_PWM_VECT TIMER1_COMPA_vect
	#define REDEYE_PULSE_REG OCR1B
	#define REDEYE_PULSE_EN_BIT OCIE1B
	#define REDEYE_PULSE_VECT TIMER1_COMPB_vect
	#define REDEYE_TCCRA TCCR1A
	#define REDEYE_TCCRB TCCR1B
	#ifdef TCCR1C
		#define REDEYE_TCCRC TCCR1C
	#endif
	#define REDEYE_ICR ICR1
	#define REDEYE_TIMSK TIMSK1
#endif



namespace redeye {

extern volatile uint8_t rxBuffer[REDEYE_RX_BUFFER_SIZE];
extern uint8_t rxReadIndex;
extern volatile uint8_t rxWriteIndex;
extern volatile bool rxBurstRecieved;

extern uint16_t txBuffer[REDEYE_TX_BUFFER_SIZE]; // RedEye bytes are 11 bits. Rip RAM usage
extern volatile uint8_t txReadIndex;
extern uint8_t txWriteIndex;
extern bool txSlowMode;

extern bool transmitMode;

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