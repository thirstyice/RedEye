/*******************************************************************************
* Project: RedEye                                                              *
* Filename: /src/RedEye.h                                                      *
*                                                                              *
* Created: 2024-09-08                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see RedEye/LICENSE (if applicable)                               *
*                                                                              *
*******************************************************************************/

#pragma once

#include <Arduino.h>

#define REDEYE_RX_BUFFER_SIZE 16
#define REDEYE_TX_BUFFER_SIZE 16

// #define REDEYE_USE_TIMER 3

#ifndef REDEYE_USE_TIMER
	#define REDEYE_USE_TIMER 1
#endif

namespace redeye {

class RedEyeClass : public Stream {
public:
	enum Mode : uint8_t {
		ModeDisabled = 0,
		ModeTx = 0b001,
		ModeRx = 0b010,
		ModeDuplex = 0b011
	};
	void begin(const uint8_t rxPin, const uint8_t txPin, bool rxInverseLogic = true, bool txInverseLogic = true);

	void setSlowTx(bool); // Set to true when using a real printer
	void setMode(Mode);

	size_t write(uint8_t);
	int available();
	int read();
	int peek();
	void flush();

	using Print::write;
};
}

extern redeye::RedEyeClass RedEye;

// Define the registers

#if REDEYE_USE_TIMER == 0
	#warning "Using RedEye on timer 0 may interfere w/ Arduino timing functions!"
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
#define REDEYE_TCCRC REDEYE_ID(TCCR,C)
#define REDEYE_ICR REDEYE_ID(ICR, )
#ifdef TIMSK
	#define REDEYE_TIMSK TIMSK
#else
	#define REDEYE_TIMSK REDEYE_ID(TIMSK, )
#endif

#if !(\
	defined(REDEYE_PWM_REG) &&\
	defined(REDEYE_PWM_EN_BIT) &&\
	defined(REDEYE_PWM_VECT) &&\
	defined(REDEYE_PULSE_REG) &&\
	defined(REDEYE_PULSE_EN_BIT) &&\
	defined(REDEYE_PULSE_VECT) &&\
	defined(REDEYE_TCCRA) &&\
	defined(REDEYE_TCCRB) &&\
	defined(REDEYE_ICR) &&\
	defined(REDEYE_TIMSK)\
)
	#error "RedEye Missing needed registers; does the selected timer exist?"
#endif
