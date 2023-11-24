#include <Arduino.h>

#pragma once

#define REDEYE_RX_BUFFER_SIZE 16
#define REDEYE_TX_BUFFER_SIZE 16
#define REDEYE_USE_TIMER_2 // Defaults to timer 1

class RedEye : public Stream {
public:
enum PrintModes {
		text,
		graphics
	};
	enum RxModes {
		passthrough, // Gives bytes as recieved
		interpreter, // Converts characters to UTF-8
		analyser // Gives pulse timings instead of actual data
	};
	RedEye(const uint8_t rxPin, bool txInverseLogic = true, bool rxInverseLogic = true);
	void begin();
	void end();

	void setSlowMode(bool); // Set to true when using a real printer
	void setRxMode(RxModes);
	PrintModes getCurrentPrintMode();

	size_t write(uint8_t);
	int available();
	int read();
	int peek();
	void flush();


	
	static inline void handleInterrupt();
	static inline void handleTimer();

	using Print::write;


private:	
	enum SpecialCodes {
		linefeed = 4,
		crlf = 10,
		esc = 27
	};
	PrintModes dataMode = text;
	RxModes rxMode = passthrough;
	byte rxBuffer[REDEYE_RX_BUFFER_SIZE];
	uint8_t rxReadIndex = 0;
	volatile uint8_t rxWriteIndex = 0;
	uint8_t overflowCounter = 0;
	uint8_t _rxPin;

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
};
