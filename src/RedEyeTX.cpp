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

void RedEyeClass::flush() {
	while (txWriteIndex != txReadIndex) {
		delay(5); // Poke the watchdog
	}
	while (txBitsToSend>0);
	delay(2); // Finish the last bit
}

void txUpdateLineTimes() {
	if (txSlowMode == true) {
		while (((millis() - lastLineTime) > 2000) && slowSendLinesAvailable<4) {
			lastLineTime += 2000;
			slowSendLinesAvailable ++;
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
		txSendNextBitAfter=70; // (28 for this bit + 42 for post-byte delay)
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
	if (slowSendLinesAvailable == 0 || txByteToSend!=0 || txReadIndex==txWriteIndex) {
		return;
	}
	txByteToSend = txBuffer[txReadIndex];
	txBuffer[txReadIndex] = 0;
	txReadIndex ++;
	txReadIndex %= REDEYE_TX_BUFFER_SIZE;
	if (txByteToSend==0) {
		return;
	}
	byte character = txByteToSend;
	if ((character == 4) || (character == 10)) { // Newlines
		slowSendLinesAvailable --;
	}
	txBitsToSend = 15;
	return;
}


} // namespace redeye