#include <Arduino.h>
#include <RedEye.h>

char toPrint = 'A';

#ifndef REDEYE_RX_PIN
	#define REDEYE_RX_PIN 2
#endif
#ifndef REDEYE_TX_PIN
	#define REDEYE_TX_PIN 3
#endif
#ifndef REDEYE_INVERTED
	#define REDEYE_INVERTED false
#endif

void setup() {
	pinMode(REDEYE_TX_PIN, OUTPUT);
	pinMode(REDEYE_RX_PIN, INPUT_PULLUP);
	Serial.begin(115200);
	RedEye.begin(REDEYE_RX_PIN,REDEYE_TX_PIN, REDEYE_INVERTED, REDEYE_INVERTED);
	RedEye.setMode(RedEye.ModeDuplex);
	while (!Serial);
	Serial.println("Ready!");
}




void loop() {
	while (RedEye.available()) {
		Serial.write(RedEye.read());
	}

	RedEye.print(toPrint);
	toPrint ++;
	if (toPrint > 'z') {
		toPrint = 'A';
	}
	delay(500);
	RedEye.flush();
}