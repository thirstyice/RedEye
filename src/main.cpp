#include <Arduino.h>
#include <RedEyeSender.h>

byte nextByte = 0;
RedEye redEye(NOT_A_PIN);

void setup() {
	Serial.begin(2400);
	redEye.begin();
	delay(2000);
	Serial.println("Ready!");
}




void loop() {
	redEye.println("String with more than 16 characters testing how long the delay is when sending very long data");
	delay(1000);
	// if (nextByte == 0) {
	// 	nextByte = Serial.read(); // Get the next byte
	// }
	// if (redEye.write(nextByte) == true) { // Send the next byte
	// 	nextByte = 0;
	// }
}