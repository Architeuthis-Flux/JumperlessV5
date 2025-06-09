int LED = 2;
int BUTTON = 3;

volatile byte state = LOW;

// hello
void press_button() {
  state = !state;
}

void setup() {
  // put your setup code here, to run once:
  delay(1600);
  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), press_button, CHANGE);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED, state);
  Serial.println(state);
  delay(1000);

}