#include <Arduino.h>
#include "RedEye.h"

RedEyeClass RedEye;

RedEyeClass* RedEyeClass::activeInstance = 0;

void RedEyeClass::begin(const uint8_t rxPin, const uint8_t _txPin, bool rxInverseLogic, bool txInverseLogic) {
	end();
	_rxInterrupt = digitalPinToInterrupt(rxPin);
	txPin = _txPin;
	_txInverseLogic = txInverseLogic;
	_rxInverseLogic = rxInverseLogic;
	if (_rxInterrupt != NOT_AN_INTERRUPT) {
		attachInterrupt(_rxInterrupt, RedEyeClass::handleInterrupt, RISING - _rxInverseLogic);
	}


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
		#define REDEYE_PULSE_LENGTH 61
		#error "Only 16 bit timers currently supported!"
	#endif
	#define REDEYE_PWM_REG OCR1A
	#define REDEYE_PWM_EN_BIT OCIE1A
	#define REDEYE_PWM_VECT TIMER1_COMPA_vect
	#define REDEYE_PULSE_REG OCR1B
	#define REDEYE_PULSE_EN_BIT OCIE1B
	#define REDEYE_PULSE_VECT TIMER1_COMPB_vect
			TCCR1A = 0b00000010; // Fast PWM, TOP is ICRn
			TCCR1B = 0b00011001; // Fast PWM, Clk/1
	#ifdef TCCR1C
			TCCR1C = 0b00000000; // Just in case
	#endif
			ICR1 = REDEYE_PULSE_LEN;
			REDEYE_PWM_REG = REDEYE_PULSE_LEN + 1; // Never reached, so 0% duty cycle
			REDEYE_PULSE_REG = 1;
			TIMSK1 = _BV(REDEYE_PULSE_EN_BIT) | _BV(REDEYE_PWM_EN_BIT);
#endif

	pinMode(txPin, OUTPUT);
	activeInstance = this;
}

ISR(REDEYE_PULSE_VECT) {
	RedEyeClass::handleTimer();
}
ISR(REDEYE_PWM_VECT) {
	RedEyeClass::handleCompMatch();
}

void RedEyeClass::handleInterrupt() {
	if (activeInstance) {
		activeInstance->rxInterrupt();
	}
}

 inline void RedEyeClass::handleTimer() {
	if (activeInstance) {
		activeInstance->pulseInterrupt();
	}
}

inline void RedEyeClass::handleCompMatch() {
	if (activeInstance) {
		activeInstance->pulseOffInterrupt();
	}
}

void RedEyeClass::pulseOffInterrupt() {
	digitalWrite(txPin, !_txInverseLogic);
}

void RedEyeClass::updateLineTimes() {
	if (slowMode == true) {
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

void RedEyeClass::end() {
	if (_rxInterrupt != NOT_AN_INTERRUPT) {
		detachInterrupt(_rxInterrupt);
	}
	if (txPin != NOT_A_PIN) {
		pinMode(txPin, INPUT);
		digitalWrite(txPin, _txInverseLogic);
	}
	activeInstance = NULL;
}

void RedEyeClass::setSlowMode(bool newMode) {
	slowMode = newMode;
}

void RedEyeClass::setTransmitMode(bool newMode) {
	if (newMode == false && transmitMode != false) {
		flush();
	}
	transmitMode = newMode;
}

bool RedEyeClass::addToRxBuffer(byte character) {
	if (((rxWriteIndex +1) % REDEYE_RX_BUFFER_SIZE) == rxReadIndex) {
		rxBuffer[rxWriteIndex] = 134; // Buffer overflow character
		return false;
	}
	rxBuffer[rxWriteIndex] = character;
	rxWriteIndex++;
	rxWriteIndex %= REDEYE_RX_BUFFER_SIZE;
	return true;
}

void RedEyeClass::rxInterrupt() {
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

void RedEyeClass::rxByteFinished() {
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

void RedEyeClass::rxBitFinished() {
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

void RedEyeClass::rxHalfBitFinished() {
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

void RedEyeClass::rxEndOfBurst() {
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

void RedEyeClass::sendBurst() {
	txPulses = 7;
}

void RedEyeClass::bitInterrupt() {
	txBitWaitCounter=27;
	if (txBitCounter == 0) { // No data left in this byte
		loadNextByte();
		return;
	}
	txBitCounter--;
	if (txBitCounter > 11) { // Send a start half-bit, and come back for the next one
		txBitWaitCounter=13;
		sendBurst();
	} else {
		if (((txByte>>txBitCounter)&1) == 1) {
			sendBurst();
		} else {
			txWaitingBeforeBurst = true;
			txBurstWaitCounter=13;
		}
	}
	if (txBitCounter == 0) {
		txBitWaitCounter=41; // Enforce post-frame delay
	}
}

void RedEyeClass::pulseInterrupt() {
	if (transmitMode == true) {
		if (txPulses == 0) {
			REDEYE_PWM_REG = REDEYE_PULSE_LEN + 1;
			// Duty cycle = 0%
		} else {
			REDEYE_PWM_REG = REDEYE_PULSE_LEN / 2;
			// Duty cycle = 50%
			txPulses--;
		}
		if (txBitWaitCounter == 0) {
			bitInterrupt();
		} else {
			txBitWaitCounter--;
		}
		if (txWaitingBeforeBurst == true) {
			if (txBurstWaitCounter == 0) {
				txWaitingBeforeBurst=false;
				sendBurst();
			} else {
				txBurstWaitCounter --;
			}
		}
	} else {
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
	digitalWrite(txPin, _txInverseLogic);
}

bool RedEyeClass::calculateParity(unsigned x) {
	bool parity = 0;
	while (x>0) {
		x &= x-1;
		parity = !parity;
	}
	return parity;
}

void RedEyeClass::loadNextByte() {
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

size_t RedEyeClass::write(uint8_t toSend) {
	updateLineTimes();
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
