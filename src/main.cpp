#include <Arduino.h>
#include <RedEye.h>

RedEye redEye(INT1); // Defaults to INT0 unless our build flags say otherwise

void setup() {
	pinMode(10, OUTPUT);
	Serial.begin(2400);
	redEye.begin();
	delay(2000);
	Serial.println("Ready!");
}




void loop() {
	digitalWrite(10, LOW);
	while (redEye.available()) {
		Serial.write(redEye.read());
	}
	redEye.print("A");
	delay(1000);
}