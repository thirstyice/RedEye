#include <Arduino.h>
#include <RedEye.h>

byte nextByte = 0;
RedEye redEye(REDEYE_RX_INTERRUPT); // Defaults to INT0 unless our build flags say otherwise

void setup() {
	Serial.begin(2400);
	redEye.begin();
	delay(2000);
	Serial.println("Ready!");
}




void loop() {
	while (redEye.available()) {
		Serial.print(redEye.read());
	}
	redEye.println("Test");
	Serial.println("Sent");
	delay(1000);
}