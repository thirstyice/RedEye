#pragma once

#include <Arduino.h>

#define REDEYE_RX_BUFFER_SIZE 16
#define REDEYE_TX_BUFFER_SIZE 16
#define REDEYE_USE_TIMER 1

class RedEyeClass : public Stream {
public:
	void begin(const uint8_t rxPin, const uint8_t txPin, bool rxInverseLogic = true, bool txInverseLogic = true);
	void end();

	void setSlowMode(bool); // Set to true when using a real printer
	void setTransmitMode(bool);

	size_t write(uint8_t);
	int available();
	int read();
	int peek();
	void flush();



	static inline void handleInterrupt();
	static inline void handleTimer();
	static inline void handleCompMatch();

	using Print::write;


private:
	volatile uint8_t rxBuffer[REDEYE_RX_BUFFER_SIZE];
	uint8_t rxReadIndex = 0;
	volatile uint8_t rxWriteIndex = 0;
	bool _rxInverseLogic = true;
	int _rxInterrupt = NOT_AN_INTERRUPT;

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
	bool _txInverseLogic = true;
	volatile uint16_t txByte = 0;
	volatile uint8_t txBytesInCurrentLine = 0;
	byte txLastLineFeed = 10;
	uint8_t txPin = NOT_A_PIN;

	bool slowMode = false;
	bool transmitMode = false;
	unsigned long lastLineTime = 0;
	volatile uint8_t slowSendLinesAvailable = 4;
	static RedEyeClass *activeInstance;
	void updateLineTimes();
	void sendBurst();
	void bitInterrupt();
	bool calculateParity(unsigned);
	void loadNextByte();
	void pulseInterrupt();
	void pulseOffInterrupt();
	void rxInterrupt();
	void rxEndOfBurst();
	void rxHalfBitFinished();
	void rxBitFinished();
	void rxByteFinished();
	bool addToRxBuffer(byte);
};

extern RedEyeClass RedEye;
