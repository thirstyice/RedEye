/*******************************************************************************
* Project: RedEye                                                              *
* Filename: /src/RedEye.h                                                      *
*                                                                              *
* Created: 2024-09-08                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see RedEye/LICENSE (if applicable)                               *
*                                                                              *
*******************************************************************************/

#pragma once

#include <Arduino.h>

#define REDEYE_RX_BUFFER_SIZE 16
#define REDEYE_TX_BUFFER_SIZE 16
namespace redeye {
enum Mode : uint8_t {
	ModeDisabled = 0,
	ModeTx = 0b001,
	ModeRx = 0b010,
	ModeDuplex = 0b011
};
class RedEyeClass : public Stream {
public:
	void begin(const uint8_t rxPin, const uint8_t txPin, bool rxInverseLogic = true, bool txInverseLogic = true);

	void setSlowTx(bool); // Set to true when using a real printer
	void setMode(Mode);

	size_t write(uint8_t);
	int available();
	int read();
	int peek();
	void flush();

	using Print::write;
};
}
extern redeye::RedEyeClass RedEye;
