#include <Arduino.h>
#include "RedEye.h"

RedEye* RedEye::activeInstance = 0;

#if F_CPU != 16000000L
	#error "Unsupported clock frequency"
#endif

void RedEye::begin() {
	if (_rxInterrupt != NOT_AN_INTERRUPT) {
		attachInterrupt(_rxInterrupt, RedEye::handleInterrupt, RISING - _rxInverseLogic);
	}

#ifdef __AVR_ATtinyX41__
	#ifdef REDEYE_USE_TIMER_2
		#define REDEYE_TIMER_IS_16_BIT
		#define REDEYE_COMP_REGISTER OCR2A
		#define REDEYE_TX_PIN PIN_PB2
		#define REDEYE_TX_VECTOR TIMER2_COMPB_vect

			TCCR2A = 0b10100010 | (!_txInverseLogic << COM2A0) | (!_txInverseLogic << COM2B0);
			TCCR2B = 0b00011001; // Fast PWM, Clk/1
			TCCR2C = 0b00000000; // Has to be all 0 in PWM mode
			TOCPMSA1 = 0b10000000; // Output OC2A to TOCC7 (PB2)
			TOCPMSA0 = 0b00000000; // Don't output elsewhere
			TOCPMCOE = 0b10000000; // Enable TOCC7 Output
			ICR2 = 488; // 16MHz / 488 cycles ~= 32768Hz
			OCR2A = 488; // 0% duty cycle
			OCR2B = 1;
			TIMSK2 = _BV(OCIE2B);
	#else
		#error "RedEye: Timer 1 not yet supported on this board!"
	#endif
#elif __AVR_ATmega32U4__
	#ifdef REDEYE_USE_TIMER_2
		// Timer2 is actually timer3 on ATmega32u4
		#error "Second timer not yet supported on this board!"
	#else
		#define REDEYE_TIMER_IS_16_BIT
		#define REDEYE_COMP_REGISTER OCR1C
		#define REDEYE_TX_PIN 11
		#define REDEYE_TX_VECTOR TIMER1_COMPB_vect
		
				TCCR1A = 0b00101010 | (!_txInverseLogic<<COM1C0) | (!_txInverseLogic << COM1B0);
				TCCR1B = 0b00011001;
				ICR1 = 488;
				OCR1C = 488;
				OCR1B = 1;
				TIMSK1 = _BV(OCIE1B);
	#endif
#else
	#error "RedEyeSender: Unsupported Board!"
#endif

	pinMode(REDEYE_TX_PIN, OUTPUT);
	activeInstance = this;
}


#ifdef REDEYE_TIMER_IS_16_BIT
	#define REDEYE_TIMER_COMP 488
#else
	#define REDEYE_TIMER_COMP 61
#endif

ISR(REDEYE_TX_VECTOR) {
	RedEye::handleTimer();
}

RedEye::RedEye(const uint8_t rxInterrupt, bool txInverseLogic, bool rxInverseLogic) {
	_rxInterrupt = rxInterrupt;
	_txInverseLogic = txInverseLogic;
	_rxInverseLogic = rxInverseLogic;
}

void RedEye::handleInterrupt() {
	if (activeInstance) {
		activeInstance->rxInterrupt();
	}
}

 inline void RedEye::handleTimer() {
	if (activeInstance) {
		activeInstance->pulseInterrupt();
	}
}

void RedEye::updateLineTimes() {
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

void RedEye::end() {
	pinMode(REDEYE_TX_PIN, INPUT);
	activeInstance = NULL;
}

void RedEye::setSlowMode(bool newMode) {
	slowMode = newMode;
}

bool RedEye::addToRxBuffer(byte character) {
	if (((rxWriteIndex +1) % REDEYE_RX_BUFFER_SIZE) == rxReadIndex) {
		rxBuffer[rxWriteIndex] = 134; // Buffer overflow character
		return false;
	}
	rxBuffer[rxWriteIndex] = character;
	rxWriteIndex++;
	rxWriteIndex %= REDEYE_RX_BUFFER_SIZE;
	return true;
}

void RedEye::rxInterrupt() {
	if (rxPulses == 0) {
		// Start of burst
		rxBurstTimer = 10;
		rxHalfBitTimer = 13;
		rxBurstRecieved = false;
	}
	rxPulses ++;
}

void RedEye::rxByteFinished() {
	addToRxBuffer(rxByte&0xff);
	if ((calculateParity(rxByte & 0b10001011)!=((rxByte>>8)&1)) ||
	    (calculateParity(rxByte & 0b11010101)!=((rxByte>>9)&1)) ||
	    (calculateParity(rxByte & 0b11100110)!=((rxByte>>10)&1)) ||
	    (calculateParity(rxByte & 0b01111000)!=((rxByte>>11)&1)) ) {
		// Bit has errors
		//addToRxBuffer(127); // Error character

	} else {
		//addToRxBuffer(rxByte & 0xff);
	}
	rxByte = 0;
	rxByteBits = 0;
}

void RedEye::rxBitFinished() {
	uint8_t bit = rxBit & 0b11;
	rxByte = (rxByte<<1 & 0x7ff);
	if (bit == 0b10) {
		rxByte |= 1;
		rxByteBits ++;
	} else if (bit == 0b01) {
		rxByteBits ++;
	}
	if (rxByteBits == 12) {
		rxByteFinished();
	}
}

void RedEye::rxHalfBitFinished() {
	digitalWrite(10, HIGH);
	rxBit=(rxBit<<1) & 0b111; // We only care about the 3 most recent half-bits
	if (rxBurstRecieved == true) {
		rxBit |= 1;
		rxBurstRecieved = false;
	}
	if (rxBit == 0b111) {
		// This is a start bit
		rxByte = 0;
		rxByteBits = 0;
		rxBitTimer = 2;
		return;
	}
	
	if (rxBitTimer>0) {
		rxBitTimer --;
		if (rxBitTimer == 0) {
			rxBitFinished();
			rxBitTimer = 2;
		}
	}
}

void RedEye::rxEndOfBurst() {
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

void RedEye::sendBurst() {
	txPulses = 7;
}

void RedEye::bitInterrupt() {
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

void RedEye::pulseInterrupt() {

/************ TX ************/
	if (txPulses == 0) {
		REDEYE_COMP_REGISTER = REDEYE_TIMER_COMP;
		// Duty cycle = 0%
	} else {
		REDEYE_COMP_REGISTER = REDEYE_TIMER_COMP / 2;
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

/************ RX ************/
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

bool RedEye::calculateParity(unsigned x) {
	bool parity = 0;
	while (x>0) {
		x &= x-1;
		parity = !parity;
	}
	return parity;
}

void RedEye::loadNextByte() {
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

size_t RedEye::write(uint8_t toSend) {
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

void RedEye::flush() {
	while (txWriteIndex != txReadIndex);
}

int RedEye::available() {
	uint8_t available = rxWriteIndex - rxReadIndex;
	return available;
}

int RedEye::read() {
	if (rxReadIndex == rxWriteIndex) {
		return 0;
	}
	byte toRead = rxBuffer[rxReadIndex];
	rxBuffer[rxReadIndex] = 0;
	rxReadIndex ++;
	rxReadIndex %= REDEYE_RX_BUFFER_SIZE;
	return toRead;
}

int RedEye::peek() {
	if (rxReadIndex == rxWriteIndex) {
		return 0;
	}
	return rxBuffer[rxReadIndex];
}

