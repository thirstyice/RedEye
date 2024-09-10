#include <Arduino.h>
#include <RedEye.h>

#ifndef REDEYE_RX_PIN
	#define REDEYE_RX_PIN 2
#endif
#ifndef REDEYE_TX_PIN
	#define REDEYE_TX_PIN 3
#endif
#ifndef REDEYE_INVERTED
	#define REDEYE_INVERTED false
#endif
#ifndef REDEYE_SPEED_PIN
	#define REDEYE_SPEED_PIN 18
#endif

void setup() {
	pinMode(REDEYE_TX_PIN, OUTPUT);
	pinMode(REDEYE_RX_PIN, INPUT_PULLUP);
	pinMode(REDEYE_SPEED_PIN, INPUT_PULLUP);
	Serial.begin(115200);
	RedEye.begin(REDEYE_RX_PIN,REDEYE_TX_PIN, REDEYE_INVERTED, REDEYE_INVERTED);
	RedEye.setMode(RedEye.ModeRx);
	while (!Serial);
	Serial.println("RedEye Adapter");
}




void loop() {
	RedEye.setSlowTx(digitalRead(REDEYE_SPEED_PIN));
	while (RedEye.available()) {
		Serial.write(RedEye.read());
	}
	if (Serial.available()) {
		RedEye.setMode(RedEye.ModeTx);
		while (Serial.available()) {
			RedEye.write(Serial.read());
		}
		RedEye.setMode(RedEye.ModeRx);
	}
}