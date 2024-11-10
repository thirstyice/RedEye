/*******************************************************************************
* Project: RedEye                                                              *
* Filename: /src/RedEyeBTS.h                                                   *
*                                                                              *
* Created: 2024-08-31                                                          *
* Author: thirstyice                                                           *
*                                                                              *
* Copyright (c) 2024 Tauran - https://github.com/thirstyice                    *
* For details see RedEye/LICENSE (if applicable)                               *
*                                                                              *
*******************************************************************************/

#pragma once
#include "RedEye.h"


namespace redeye {

extern volatile uint8_t rxBuffer[REDEYE_RX_BUFFER_SIZE];
extern uint8_t rxReadIndex;
extern volatile uint8_t rxWriteIndex;
extern volatile bool rxBurstRecieved;

extern uint16_t txBuffer[REDEYE_TX_BUFFER_SIZE]; // RedEye bytes are 11 bits. Rip RAM usage
extern volatile uint8_t txReadIndex;
extern uint8_t txWriteIndex;
extern bool txSlowMode;

extern RedEyeClass::Mode mode;

bool calculateParity(unsigned);

void rxPulse();
bool addToRxBuffer(byte);
void rxInterruptHandler();
void rxByteFinished();
void rxBitFinished();
void rxHalfBitFinished();
void rxEndOfBurst();

void txPulse();
void txSendBurst();
void txBitInterrupt();
void txLoadNextByte();

}