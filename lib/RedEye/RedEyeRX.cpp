/*******************************************************************************
* Project: HP Printer Tester                                                   *
* Filename: /lib/RedEye/RedEyeRX.cpp                                           *
*                                                                              *
* Created: 2024-08-30                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see HP Printer Tester/LICENSE (if applicable)                    *
*                                                                              *
*******************************************************************************/
#include "RedEyeBTS.h"


namespace redeye {

volatile uint8_t rxBurstTimer = 0;
volatile uint8_t rxHalfBitTimer = 0;
volatile uint8_t rxByteBits = 0;
volatile uint8_t rxBitTimer = 0;
volatile uint8_t rxPulses = 0;
volatile uint8_t rxBit = 0;
volatile uint16_t rxByte = 0;

bool addToRxBuffer(byte character) {
	if (((rxWriteIndex +1) % REDEYE_RX_BUFFER_SIZE) == rxReadIndex) {
		rxBuffer[rxWriteIndex] = 134; // Buffer overflow character
		return false;
	}
	rxBuffer[rxWriteIndex] = character;
	rxWriteIndex++;
	rxWriteIndex %= REDEYE_RX_BUFFER_SIZE;
	return true;
}

void rxInterruptHandler() {
	if (transmitMode == true) {
		return;
	}
	if (rxPulses == 0) {
		// Start of burst
		rxBurstTimer = 10;
		rxHalfBitTimer = 13;
		rxBurstRecieved = false;
	}
	rxPulses ++;
}

void rxByteFinished() {
	if ((calculateParity(rxByte & 0b10001011)!=((rxByte>>8)&1)) ||
	    (calculateParity(rxByte & 0b11010101)!=((rxByte>>9)&1)) ||
	    (calculateParity(rxByte & 0b11100110)!=((rxByte>>10)&1)) ||
	    (calculateParity(rxByte & 0b01111000)!=((rxByte>>11)&1)) ) {
		// Bit has errors
		addToRxBuffer(127); // Error character

	} else {
		addToRxBuffer(rxByte & 0xff);
	}
	rxByte = 0;
}

void rxBitFinished() {
	uint8_t bit = rxBit & 0b11;
	rxByte = (rxByte<<1) & 0xfff;
	if (bit == 0b10) {
		rxByte |= 1;
		rxByteBits ++;
	} else if (bit == 0b01) {
		rxByteBits ++;
	}
	if (rxByteBits == 12) {
		rxByteFinished();
		rxByteBits = 0;
	}
}

void rxHalfBitFinished() {
	rxBit=(rxBit<<1) & 0b111; // We only care about the 3 most recent half-bits
	if (rxBurstRecieved == true) {
		rxBit |= 1;
		rxBurstRecieved = false;
	}
	if (rxBit == 0b111) {
		// This is a start bit
		rxBit = 0;
		rxByte = 0;
		rxByteBits = 0;
		rxBitTimer = 2;
		return;
	} else if (rxBit == 0) {
		// No data
		rxBitTimer = 0;
		rxHalfBitTimer = 0;
	}

	if (rxBitTimer>0) {
		rxBitTimer --;
		if (rxBitTimer == 0) {
			rxBitFinished();
			rxBitTimer = 2;
		}
	}
}

void rxEndOfBurst() {
	if (rxPulses>=5 && rxPulses<=9) {
		// It's a real burst!
		rxBurstRecieved = true;
	} else {
		// Invalid burst :(
		rxHalfBitTimer = 0;
	}
	rxBurstTimer = 0;
	rxPulses = 0;
}

void rxPulse() {
	if (rxBurstTimer>0) {
		rxBurstTimer--;
		if (rxBurstTimer==0) {
			rxEndOfBurst();
		}
	}
	if (rxHalfBitTimer>0) {
		rxHalfBitTimer--;
		if (rxHalfBitTimer==0) {
			rxHalfBitFinished();
			rxHalfBitTimer = 13;
		}
	}
}

} // namespace redeye
