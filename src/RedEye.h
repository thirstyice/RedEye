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

#if F_CPU < 16000000L
	#warning "Clock speed below 16MHz; you may encounter problems!"
#endif

#define __REDEYE_ID(x, y, z) x ## y ## z
#define _REDEYE_ID(x, y, z) __REDEYE_ID(x, y, z)
#define REDEYE_ID(x, y) _REDEYE_ID(x, REDEYE_USE_TIMER, y)

// Which timers exist?
#ifdef TCNT0
	#define REDEYE_TIMER_0_AVAIL 1
#endif
#ifdef TCNT1
	#define REDEYE_TIMER_1_AVAIL 1
#endif
#ifdef TCNT2
	#define REDEYE_TIMER_2_AVAIL 1
#endif
#ifdef TCNT3
	#define REDEYE_TIMER_3_AVAIL 1
#endif

// Which timers are 16bit?
#ifdef TCNT0H
	#define REDEYE_TIMER_0_16BIT 1
#endif
#ifdef TCNT1H
	#define REDEYE_TIMER_1_16BIT 1
#endif
#ifdef TCNT2H
	#define REDEYE_TIMER_2_16BIT 1
#endif
#ifdef TCNT3H
	#define REDEYE_TIMER_3_16BIT 1
#endif

// Whiich timers have TCCRxC?
#ifdef TCCR0C
	#define REDEYE_TIMER_0_TCCRC 1
#endif
#ifdef TCCR1C
	#define REDEYE_TIMER_1_TCCRC 1
#endif
#ifdef TCCR2C
	#define REDEYE_TIMER_2_TCCRC 1
#endif
#ifdef TCCR3C
	#define REDEYE_TIMER_3_TCCRC 1
#endif

#define REDEYE_TIMER_AVAIL REDEYE_ID(REDEYE_TIMER_, _AVAIL)
#define REDEYE_TIMER_16BIT REDEYE_ID(REDEYE_TIMER_, _16BIT)
#define REDEYE_TIMER_TCCRC REDEYE_ID(REDEYE_TIMER_, _TCCRC)

#if REDEYE_TIMER_AVAIL != 1
	#error "Selected timer does not exist!"
#endif

#if REDEYE_TIMER_16BIT == 1
	#define REDEYE_TIMER_MAX UINT16_MAX
#else
	#define REDEYE_TIMER_MAX UINT8_MAX
#endif

#define REDEYE_PULSE_LEN F_CPU/32768U

#if REDEYE_PULSE_LEN >= REDEYE_TIMER_MAX
	#undef REDEYE_PULSE_LEN
	#define REDEYE_PULSE_LEN F_CPU / 8 / 32768U
	#define REDEYE_PRESCALER 1U
#else
	#define REDEYE_PRESCALER 0
#endif

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
#if REDEYE_TIMER_TCCRC
	#define REDEYE_TCCRC REDEYE_ID(TCCR,C)
#endif
#ifdef TIMSK
	#define REDEYE_TIMSK TIMSK
#else
	#define REDEYE_TIMSK REDEYE_ID(TIMSK, )
#endif
