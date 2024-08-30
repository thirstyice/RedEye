/*******************************************************************************
* Project: HP Printer Tester                                                   *
* Filename: /lib/RedEye/RedEyeCommon.cpp                                          *
*                                                                              *
* Created: 2024-08-30                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see HP Printer Tester/LICENSE (if applicable)                    *
*                                                                              *
*******************************************************************************/
#include "RedEyeBTS.h"

redeye::RedEyeClass RedEye;

namespace redeye {

bool redeyeActive = false;

volatile uint8_t rxBuffer[REDEYE_TX_BUFFER_SIZE];
uint8_t rxReadIndex = 0;
volatile uint8_t rxWriteIndex = 0;
bool rxInverseLogic = true;
int rxInterrupt = NOT_AN_INTERRUPT;
volatile bool rxBurstRecieved = false;

uint16_t txBuffer[REDEYE_TX_BUFFER_SIZE]; // RedEye bytes are 11 bits. Rip RAM usage
volatile uint8_t txReadIndex = 0;
uint8_t txWriteIndex = 0;
uint8_t txPin = NOT_A_PIN;
bool txInverseLogic = true;
volatile uint8_t txBytesInCurrentLine = 0;
byte txLastLineFeed = 10;
bool txSlowMode = false;
bool transmitMode = false;

ISR(REDEYE_PULSE_VECT) {
	if (transmitMode == true) {
		txPulse();
	} else {
		rxPulse();
	}
	if (redeyeActive) {
		digitalWrite(txPin, txInverseLogic);
	}
}

/**
 * @brief When we hit the pwm threshold, turn on the TX led
 */
ISR(REDEYE_PWM_VECT) {
	digitalWrite(txPin, !txInverseLogic);
}

bool calculateParity(unsigned x) {
	bool parity = 0;
	while (x>0) {
		x &= x-1;
		parity = !parity;
	}
	return parity;
}

void RedEyeClass::begin(const uint8_t _rxPin, const uint8_t _txPin, bool _rxInverseLogic, bool _txInverseLogic) {
	end();
	rxInterrupt = digitalPinToInterrupt(_rxPin);
	txPin = _txPin;
	txInverseLogic = _txInverseLogic;
	rxInverseLogic = _rxInverseLogic;
	if (rxInterrupt != NOT_AN_INTERRUPT) {
		attachInterrupt(rxInterrupt, rxInterruptHandler, RISING - rxInverseLogic);
	}
	REDEYE_TCCRA = 0b00000010; // Fast PWM, TOP is ICRn
	REDEYE_TCCRB = 0b00011001; // Fast PWM, Clk/1
#ifdef REDEYE_TCCRC
	REDEYE_TCCRC = 0b00000000; // Just in case
#endif
	REDEYE_ICR = REDEYE_PULSE_LEN;
	REDEYE_PWM_REG = REDEYE_PULSE_LEN + 1; // Never reached, so 0% duty cycle
	REDEYE_PULSE_REG = 1;
	REDEYE_TIMSK = _BV(REDEYE_PULSE_EN_BIT) | _BV(REDEYE_PWM_EN_BIT);

	pinMode(txPin, OUTPUT);
	redeyeActive = true;
}

void RedEyeClass::end() {
	if (rxInterrupt != NOT_AN_INTERRUPT) {
		detachInterrupt(rxInterrupt);
	}
	REDEYE_TIMSK = 0;
	if (txPin != NOT_A_PIN) {
		digitalWrite(txPin, txInverseLogic);
	}
	redeyeActive = false;
}

void RedEyeClass::setSlowMode(bool newMode) {
	txSlowMode = newMode;
}

void RedEyeClass::setTransmitMode(bool newMode) {
	if (newMode == false && transmitMode != false) {
		flush(); // Finish what we're currently transmitting before switching to RX
	}
	transmitMode = newMode;
}

size_t RedEyeClass::write(uint8_t toSend) {
	txUpdateLineTimes();
	if (toSend == 0) {
		return 1; // Yep, we definitely sent that nothing!
	}
	txBytesInCurrentLine++;
	if ((toSend == 4)|| (toSend == 10)) {
		txBytesInCurrentLine = 0;
		txLastLineFeed = toSend;
	}

	uint16_t errorCorrection = 0;
	errorCorrection += calculateParity(toSend & 0b10001011)<<8;
	errorCorrection += calculateParity(toSend & 0b11010101)<<9;
	errorCorrection += calculateParity(toSend & 0b11100110)<<10;
	errorCorrection += calculateParity(toSend & 0b01111000)<<11;
	errorCorrection += toSend;

	while (((txWriteIndex+1) % REDEYE_TX_BUFFER_SIZE) == txReadIndex); // Wait so we don't overflow the buffer
	txBuffer[txWriteIndex] = errorCorrection;
	txWriteIndex++;
	txWriteIndex %= REDEYE_TX_BUFFER_SIZE;

	return 1;
}

void RedEyeClass::flush() {
	while (txWriteIndex != txReadIndex);
}

int RedEyeClass::available() {
	uint8_t available = rxWriteIndex - rxReadIndex;
	return available;
}

int RedEyeClass::read() {
	if (rxReadIndex == rxWriteIndex) {
		return 0;
	}
	byte toRead = rxBuffer[rxReadIndex];
	rxBuffer[rxReadIndex] = 0;
	rxReadIndex ++;
	rxReadIndex %= REDEYE_RX_BUFFER_SIZE;
	return toRead;
}

int RedEyeClass::peek() {
	if (rxReadIndex == rxWriteIndex) {
		return 0;
	}
	return rxBuffer[rxReadIndex];
}

}