/*******************************************************************************
* Project: HP Printer Tester                                                   *
* Filename: /lib/RedEye/RedEye.h                                               *
*                                                                              *
* Created: 2024-08-27                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see HP Printer Tester/LICENSE (if applicable)                    *
*                                                                              *
*******************************************************************************/
#pragma once

#include <Arduino.h>

#define REDEYE_RX_BUFFER_SIZE 16
#define REDEYE_TX_BUFFER_SIZE 16
#define REDEYE_USE_TIMER 1
namespace redeye {
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

	using Print::write;
};
}
extern redeye::RedEyeClass RedEye;
