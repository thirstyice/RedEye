#include <Arduino.h>
#include "RedEyeSender.h"

void redEyeSenderInterrupt() {
	redEyeSender.pulseInterrupt();
}

#ifdef __AVR_ATtinyX41__
	#ifdef REDEYE_PWM_USE_TIMER_2
		ISR(TIMER2_COMPB_vect) {
			redEyeSenderInterrupt();
		}
		RedEyeSender::RedEyeSender() {
			TCCR2A = 0b10100010; // Unset on compare match, set @ Bottom (inverted logic)
			TCCR2B = 0b00011001; // Fast PWM, Clk/1
			TCCR2C = 0b00000000; // Has to be all 0 in PWM mode
			TOCPMSA1 = 0b10000000; // Output OC2A to TOCC7 (PB2)
			TOCPMSA0 = 0b00000000; // Don't output elsewhere
			TOCPMCOE = 0b10000000; // Enable TOCC7 Output
			ICR2 = 488; // 16MHz / 488 cycles ~= 32768Hz
			OCR2A = 488; // 0% duty cycle
			OCR2B = 1;
			TIMSK2 = _BV(OCIE2B);
			pinMode(PIN_PB2, OUTPUT);
		}
	#else
		ISR(TIMER1_COMPB_vect) {
			pulseInterrupt();
		}
		#error "RedEyeSender: Timer 1 not yet supported on this board!"
	#endif
#else
	#error "RedEyeSender: Unsupported Board!"
#endif

void RedEyeSender::setSlowMode(bool newMode) {
	slowMode = newMode;
}

void RedEyeSender::loop() {
	if (slowMode == true && availBufferLines<4) {
		while ((millis() - lineTime) > 2000) {
			lineTime += 2000;
			availBufferLines ++;
		}
		if (availBufferLines > 4) {
			availBufferLines = 4;
		}
	}
}

void RedEyeSender::sendBurst() {
	outputPulses = 7;
}

void RedEyeSender::bitInterrupt() {
	cycleCounter=27;
	if (bitCounter == 0) { // No data to send
		return;
	}
	bitCounter--;
	if (bitCounter > 11) { // Send a start half-bit, and come back for the next one
		cycleCounter=13;
		sendBurst();
	} else {
		if (((outByte>>bitCounter)&1) == 1) {
			sendBurst();
		} else {
			waiting = true;
			waitCounter=13;
		}
	}
	if (bitCounter == 0) {
		cycleCounter=41; // Enforce post-frame delay
	}
}

void RedEyeSender::pulseInterrupt() {
	if (outputPulses == 0) {
		OCR2A = 488;
		// Duty cycle = 0%
	} else {
		OCR2A = 244;
		// Duty cycle = 50%
		outputPulses--;
	}
	if (cycleCounter == 0) {
		bitInterrupt();
	} else {
		cycleCounter--;
	}
	if (waiting == true) {
		if (waitCounter == 0) {
			waiting=false;
			sendBurst();
		} else {
			waitCounter --;
		}
	}
}

bool RedEyeSender::parity(unsigned x) {
	bool parity = 0;
	while (x>0) {
		x &= x-1;
		parity = !parity;
	}
	return parity;
}

bool RedEyeSender::sendByte(byte toSend) {
	if ((bitCounter != 0) || (availBufferLines == 0)) {
		loop();
		return false;
	}
	if (toSend==0) {
		return true;
	}
	lineBytes++;
	if ((toSend == 4)|| (toSend == 10)) {
		lineBytes = 0;
		lastLineFeed = toSend;
		availBufferLines ++;
	}
	uint16_t errorCorrection = 0;
	errorCorrection += parity(toSend & 0b10001011)<<8;
	errorCorrection += parity(toSend & 0b11010101)<<9;
	errorCorrection += parity(toSend & 0b11100110)<<10;
	errorCorrection += parity(toSend & 0b01111000)<<11;
	errorCorrection += toSend;
	outByte=errorCorrection;
	bitCounter=15; // Data 0-7, ECC 8-11, 3 start bursts, plus post delay

	if (lineBytes == 24) { // Line is full
		while (sendByte(lastLineFeed)==false); // Continue on new line
	}
	return true;
}

RedEyeSender redEyeSender;

