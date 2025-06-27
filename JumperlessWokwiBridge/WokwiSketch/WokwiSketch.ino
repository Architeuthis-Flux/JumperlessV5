void setup() {
  // put your setup code here, to run once:
pinMode(LED_BUILTIN, OUTPUT);
Serial.begin(115200);
delay(1000);

}

void loop() {
  // put your main code here, to run repeatedly:
digitalWrite(LED_BUILTIN, HIGH);
Serial.print("fuck\n\r");
delay(100);
digitalWrite(LED_BUILTIN, LOW);
delay(100);
}