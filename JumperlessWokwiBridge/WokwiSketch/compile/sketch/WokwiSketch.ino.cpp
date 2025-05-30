#include <Arduino.h>
#line 1 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
// Example Arduino sketch for Jumperless local file monitoring
// Save this file and assign it to a slot to test automatic flashing

#line 4 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
void setup();
#line 11 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
void loop();
#line 4 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(1000);
  Serial.println("Local file monitoring test - setup complete!");
}

void loop() {
  // Blink the built-in LED
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  
  Serial.println("Hello from local file!");
}
