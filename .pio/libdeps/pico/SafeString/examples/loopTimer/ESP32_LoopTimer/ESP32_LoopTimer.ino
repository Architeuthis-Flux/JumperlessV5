// ESP32_LoopTimer.ino
// LED set for SparkFun ESP32 Thing board
/*
   (c)2019 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/

#include <loopTimer.h>
#include <millisDelay.h>

#include <BufferedOutput.h>
// install SafeString library from Library manager or from https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
// to get BufferedOutput. See https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html for a full tutorial
// on Arduino Serial I/O that Works

int led = 5; // pin 5 for LED on SparkFun EPS32 Thing
createBufferedOutput(bufferedOut, 80, DROP_UNTIL_EMPTY);

loopTimerClass blinkTaskTimer("blink");

bool ledOn = false;
millisDelay ledDelay;

void blinkTask(void* parameter) {
  ledOn = false;
  // start delay
  ledDelay.start(1000);

  for (;;) {   // loop forever
    blinkTaskTimer.check();
    vTaskDelay(1); // need this to prevent wdt panic

    if (ledDelay.justFinished()) {
      ledDelay.repeat(); // start delay again without drift
      // toggle the led
      ledOn = !ledOn;
      if (ledOn) {
        digitalWrite(led, HIGH); // turn led on
      } else {
        digitalWrite(led, LOW); // turn led off
      }
    }
  }
  /* delete a task when finish,
    this will never happen because this is infinity loop */
  vTaskDelete( NULL );
}

millisDelay printTimers;
const unsigned long PRINT_TIMERS_DELAY = 5000;

void setup() {
  Serial.begin(115200);
  for (int i = 10; i > 0; i--) {
    Serial.println(i);
    delay(500);
  }
  // initialize digital pin led as an output.
  pinMode(led, OUTPUT);

  xTaskCreatePinnedToCore(
    blinkTask,           /* Task function. */
    "blinkTask",        /* name of task. */
    1000,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */  // loop() has priority == 1
    NULL,                    /* Task handle to keep track of created task */
    CONFIG_ARDUINO_RUNNING_CORE); //pin to core 1, WiFi is pinned to 0

  printTimers.start(PRINT_TIMERS_DELAY);
  bufferedOut.connect(Serial);
}


void loop() {
  loopTimer.check();
  bufferedOut.nextByteOut(); // check if any output to print

  if (printTimers.justFinished()) {
    printTimers.restart(); // this may drift
    loopTimer.print(bufferedOut);
    blinkTaskTimer.print(bufferedOut);
  }
  vTaskDelay(1); // need this to prevent wdt panic
}
