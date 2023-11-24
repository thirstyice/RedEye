#include <Arduino.h>
#include "RedEyeSender.h"

#ifdef __AVR_ATtinyX41__
	#ifdef REDEYE_USE_TIMER_2
		ISR(TIMER2_COMPB_vect) {
			RedEye::handleTimer();
		}
		#define REDEYE_TIMER_IS_16_BIT
		#define REDEYE_COMP_REGISTER OCR2A
		#define REDEYE_TX_PIN PIN_PB2

		void RedEye::begin() {
			pinMode(REDEYE_TX_PIN, INPUT);
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
			pinMode(REDEYE_TX_PIN, OUTPUT);
			activeInstance = this;
		}
	#else
		#error "RedEye: Timer 1 not yet supported on this board!"
	#endif
#elif __AVR_ATmega32U4__
	#ifdef REDEYE_USE_TIMER_2
		// Timer2 is actually timer3 on ATmega32u4
		#error "Second timer not yet supported on this board!"
	#else
		#define REDEYE_TIMER_IS_16_BIT
		#define REDEYE_COMP_REGISTER OCR1A
		#define REDEYE_TX_PIN PB6
		ISR(TIMER1_COMPB_vect) {
			RedEye::handleTimer();
		}
		RedEye::RedEye(const uint8_t rxPin, bool txInverseLogic, bool rxInverseLogic) {
			{ // Setup timer
				pinMode(REDEYE_TX_PIN, INPUT);
				TCCR1A = 0b10100010 | (txInverseLogic<<COM1A0) | (txInverseLogic << COM1B1);
				TCCR1B = 0b00011001;
				ICR1 = 488;
				OCR1A = 488;
				OCR1B = 1;
				TIMSK1 = _BV(OCIE1B);
			}
		}
	#endif
#else
	#error "RedEyeSender: Unsupported Board!"
#endif
#ifdef REDEYE_TIMER_IS_16_BIT
	#define REDEYE_TIMER_COMP 488
#else
	#define REDEYE_TIMER_COMP 61
#endif

RedEye::RedEye(const uint8_t rxPin, bool txInverseLogic, bool rxInverseLogic) {
	_txInverseLogic = txInverseLogic;
}

RedEye* RedEye::activeInstance = 0;

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
			if ((txByte == 4)|| (txByte == 10)) {
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
	return 0;
}

int RedEye::read() {
	return 0;
}

int RedEye::peek() {
	return 0;
}

