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

volatile uint8_t txPulsesToSend = 0;
volatile uint8_t txBitsToSend = 0;
volatile uint8_t txSendNextBurstAfter = 0;
volatile uint8_t txSendNextBitAfter = 0;
volatile bool txWaitingBeforeBurst = false;
volatile uint16_t txByteToSend = 0;
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
	txPulsesToSend = 7;
}

void txBitInterrupt() {
	if (txBitsToSend == 0) { // No data left in this byte
		txLoadNextByte(); // Try to load a new byte
		return;
	}
	txBitsToSend--;
	if (txBitsToSend > 11) { // Send a start half-bit, and come back for the next one
		txSendNextBitAfter=13;
		txSendBurst();
		return;
	}
	txSendNextBitAfter=27;
	if (((txByteToSend>>txBitsToSend)&1) == 1) {
		txSendBurst();
	} else {
		txSendNextBurstAfter=13;
	}
	if (txBitsToSend == 0) {
		txSendNextBitAfter=41; // Enforce post-frame delay
		txByteToSend = 0;
	}
}

void txPulse() {
	if (txPulsesToSend == 0) {
		REDEYE_PWM_REG = REDEYE_PULSE_LEN + 1;
		// Duty cycle = 0%
	} else {
		REDEYE_PWM_REG = REDEYE_PULSE_LEN / 2;
		// Duty cycle = 50%
		txPulsesToSend--;
	}
	if (txSendNextBitAfter == 0) {
		txBitInterrupt();
	} else {
		txSendNextBitAfter--;
	}
	if (txSendNextBurstAfter>0) {
		txSendNextBurstAfter--;
		if (txSendNextBurstAfter==0) {
			txSendBurst();
		}
	}
}

void txLoadNextByte() {
	if (slowSendLinesAvailable == 0 || txByteToSend!=0) {
		return;
	}
	txByteToSend = txBuffer[txReadIndex];
	byte character = txByteToSend;
	if ((character == 4) || (character == 10)) { // Newlines
		slowSendLinesAvailable --;
	}
	txBuffer[txReadIndex] = 0;
	txReadIndex ++;
	txReadIndex %= REDEYE_TX_BUFFER_SIZE;
	txBitsToSend = 15;
	return;
}


} // namespace redeye