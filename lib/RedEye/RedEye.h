#include <Arduino.h>

#pragma once

#define REDEYE_RX_BUFFER_SIZE 16
#define REDEYE_TX_BUFFER_SIZE 16
// #define REDEYE_USE_TIMER_2 // Defaults to timer 1

#ifndef REDEYE_RX_INTERRUPT
	#define REDEYE_RX_INTERRUPT INT0
#endif

class RedEye : public Stream {
public:
	RedEye(const uint8_t rxPin, bool txInverseLogic = true, bool rxInverseLogic = true);
	void begin();
	void end();

	void setSlowMode(bool); // Set to true when using a real printer

	size_t write(uint8_t);
	int available();
	int read();
	int peek();
	void flush();


	
	static inline void handleInterrupt();
	static inline void handleTimer();

	using Print::write;


private:	
	volatile uint8_t rxBuffer[REDEYE_RX_BUFFER_SIZE];
	uint8_t rxReadIndex = 0;
	volatile uint8_t rxWriteIndex = 0;
	bool _rxInverseLogic;
	int _rxInterrupt;

	volatile uint8_t rxBurstTimer = 0;
	volatile bool rxBurstRecieved = false;
	volatile uint8_t rxHalfBitTimer = 0;
	volatile uint8_t rxByteBits = 0;
	volatile uint8_t rxBitTimer = 0;
	volatile uint8_t rxPulses = 0;
	volatile uint8_t rxBit = 0;
	volatile uint16_t rxByte = 0;

	uint16_t txBuffer[REDEYE_TX_BUFFER_SIZE]; // RedEye bytes are 11 bits. Rip RAM usage
	volatile uint8_t txReadIndex = 0;
	uint8_t txWriteIndex = 0;
	volatile uint8_t txPulses = 0;
	volatile uint8_t txBitCounter = 0;
	volatile uint8_t txBurstWaitCounter = 0;
	volatile uint8_t txBitWaitCounter = 0;
	volatile bool txWaitingBeforeBurst = false;
	bool _txInverseLogic;
	byte txByte = 0;
	volatile uint8_t txBytesInCurrentLine = 0;
	byte txLastLineFeed = 10;
	
	bool slowMode = false;
	unsigned long lastLineTime = 0;
	volatile uint8_t slowSendLinesAvailable = 4;
	static RedEye *activeInstance;
	void updateLineTimes();
	void sendBurst();
	void bitInterrupt();
	bool calculateParity(unsigned);
	void loadNextByte();
	void pulseInterrupt();
	void rxInterrupt();
	void rxEndOfBurst();
	void rxHalfBitFinished();
	void rxBitFinished();
	void rxByteFinished();
	bool addToRxBuffer(byte);
};
