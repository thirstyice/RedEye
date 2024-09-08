#include <Arduino.h>
#include <RedEye.h>

char toPrint = 'A';

void setup() {
	pinMode(11, OUTPUT);
	pinMode(2, INPUT_PULLUP);
	Serial.begin(115200);
	RedEye.begin(2,11, false, false);
	RedEye.setMode(redeye::ModeDuplex);
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