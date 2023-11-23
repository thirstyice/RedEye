#include <Arduino.h>

#pragma once

#define REDEYE_PWM_USE_TIMER_2

class RedEyeSender
{
	private:
		volatile uint8_t outputPulses = 0;
		volatile uint8_t bitCounter = 0;
		volatile uint8_t waitCounter = 0;
		volatile uint8_t cycleCounter = 0;
		volatile bool waiting = false;
		bool slowMode = false;
		unsigned long lineTime = 0;
		uint8_t availBufferLines = 4;
		uint8_t lineBytes = 0;
		byte lastLineFeed = 10;
		uint16_t outByte = 0;
		bool parity(unsigned);
		void sendBurst();
		void bitInterrupt();
	public:
		RedEyeSender();
		void pulseInterrupt();
		bool sendByte(byte); // Returns true on success, false if buffer is full
		void setSlowMode(bool); // Set to true if printing to an actual printer
		void loop();
};

extern RedEyeSender redEyeSender;
