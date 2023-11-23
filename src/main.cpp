#include <Arduino.h>
#include <RedEyeSender.h>

byte nextByte = 0;

void setup() {
	Serial.begin(2400);
	delay(2000);
	while (!Serial);
	Serial.println("Ready!");
}




void loop() {
	if (nextByte == 0) {
		nextByte = Serial.read(); // Get the next byte
	}
	if (redEyeSender.sendByte(nextByte) == true) { // Send the next byte
		nextByte = 0;
	}
	redEyeSender.loop();
}