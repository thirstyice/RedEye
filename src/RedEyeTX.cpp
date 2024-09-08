/*******************************************************************************
* Project: RedEye                                                              *
* Filename: /src/RedEyeTX.cpp                                                  *
*                                                                              *
* Created: 2024-08-31                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see RedEye/LICENSE (if applicable)                               *
*                                                                              *
*******************************************************************************/

#include "RedEyeBTS.h"

namespace redeye {

volatile uint8_t txPulses = 0;
volatile uint8_t txBitCounter = 0;
volatile uint8_t txBurstWaitCounter = 0;
volatile uint8_t txBitWaitCounter = 0;
volatile bool txWaitingBeforeBurst = false;
volatile uint16_t txByte = 0;
unsigned long lastLineTime = 0;
volatile uint8_t slowSendLinesAvailable = 4;

void txUpdateLineTimes() {
	if (txSlowMode == true) {
		while ((millis() - lastLineTime) > 2000) {
			lastLineTime += 2000;
			slowSendLinesAvailable ++;
		}
		if (slowSendLinesAvailable > 4) {
			slowSendLinesAvailable = 4;
		}
	} else {
		slowSendLinesAvailable = 128;
	}
}

void txSendBurst() {
	txPulses = 7;
}

void txBitInterrupt() {
	txBitWaitCounter=27;
	if (txBitCounter == 0) { // No data left in this byte
		txLoadNextByte();
		return;
	}
	txBitCounter--;
	if (txBitCounter > 11) { // Send a start half-bit, and come back for the next one
		txBitWaitCounter=13;
		txSendBurst();
	} else {
		if (((txByte>>txBitCounter)&1) == 1) {
			txSendBurst();
		} else {
			txWaitingBeforeBurst = true;
			txBurstWaitCounter=13;
		}
	}
	if (txBitCounter == 0) {
		txBitWaitCounter=41; // Enforce post-frame delay
	}
}

void txPulse() {
	if (txPulses == 0) {
		REDEYE_PWM_REG = REDEYE_PULSE_LEN + 1;
		// Duty cycle = 0%
	} else {
		REDEYE_PWM_REG = REDEYE_PULSE_LEN / 2;
		// Duty cycle = 50%
		txPulses--;
	}
	if (txBitWaitCounter == 0) {
		txBitInterrupt();
	} else {
		txBitWaitCounter--;
	}
	if (txWaitingBeforeBurst == true) {
		if (txBurstWaitCounter == 0) {
			txWaitingBeforeBurst=false;
			txSendBurst();
		} else {
			txBurstWaitCounter --;
		}
	}
}

void txLoadNextByte() {
	if (slowSendLinesAvailable == 0) {
		return;
	}
	if (txWriteIndex != txReadIndex) {
		txByte = txBuffer[txReadIndex];
		byte character = txByte;
		if ((character == 4)|| (character == 10)) {
			slowSendLinesAvailable --;
		}
		txBuffer[txReadIndex] = 0;
		txReadIndex ++;
		txReadIndex %= REDEYE_TX_BUFFER_SIZE;
		txBitCounter = 15;
	}
	return;
}


} // namespace redeye