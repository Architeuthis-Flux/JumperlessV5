#include <Arduino.h>
#line 1 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
int LED = 2;
int BUTTON = 3;

volatile byte state = LOW;

// hello
#line 7 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
void press_button();
#line 11 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
void setup();
#line 19 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
void loop();
#line 7 "/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/WokwiSketch/WokwiSketch.ino"
void press_button() {
  state = !state;
}

void setup() {
  // put your setup code here, to run once:
  delay(1900);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), press_button, CHANGE);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED, state);
}
