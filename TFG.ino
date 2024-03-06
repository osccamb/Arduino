#include <Servo.h>
#include <LiquidCrystal.h>
#include <Wire.h>

Servo myservo;

// Cycle system variables
float max_deg = 180;
float min_deg = 0;
float pos = 0;  // servo position variable

float step = 1;           // degree variation per loop
float step_time = 10;     // time between each loop of the arduino
float cycle_time = 1000;  // time between the start of one cycle and another
int num_cycles = 3000;     // cycles to perform
bool running = false;     // finish the experiment after completing the cycles
float time_left = 1 + ((1120 + cycle_time) * num_cycles) / 1000.0 + (num_cycles * step_time * (max_deg - min_deg) / (1000.0 * step));
float time_total = time_left;
float time_cycle = (1120 + cycle_time) / 1000.0 + step_time * (max_deg - min_deg) / (1000.0 * step);
// Resistance measurement variables
//int analogPin = 0;
//int raw = 0;
//int Vin = 5;
//float Vout = 0;
//float R1 = 10000;  // value of the known resistor
//float Rm = 0;      // unknown sample resistance
//float buffer = 0;

// LCD variables
int Contrast = 50;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int startButtonPin = 7;    // Pin for starting the experiment
int pauseButtonPin = 8;    // Pin for pausing or resuming the experiment
int nextButtonPin = 9;     // Pin for going to the next panel
bool nextPressed = false;  // prevents to pulse the nextButton various times, causing lag to the servo
int displayOption = 1;

void setup() {
  myservo.attach(13);

  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(pauseButtonPin, INPUT_PULLUP);
  pinMode(nextButtonPin, INPUT_PULLUP);

  analogWrite(6, Contrast);
  lcd.begin(16, 2);
  Serial.begin(9600);  // For printing measurements to the monitor
}

void formatTime(float seconds, char* buffer) {
  int hours = seconds / 3600;
  int minutes = (seconds - (hours * 3600)) / 60;
  int secs = seconds - (hours * 3600) - (minutes * 60);
  sprintf(buffer, "%02d:%02d:%02d", hours, minutes, secs);
}

void loop() {
  if (digitalRead(startButtonPin) == LOW && !running) {
    running = true;
    lcd.setCursor(0, 0);
    lcd.print("Measuring...       ");  // Ensure the length of the text matches the space available on the LCD
    time_left = time_total - 1 + time_cycle;
    delay(1000);  // Delay to debounce the button
  }

  while (running) {
    for (int cycles = 1; cycles <= num_cycles; cycles++) {
      Serial.println(cycles);
      time_left = time_left - time_cycle;
      char time_buffer[9];
      formatTime(time_left, time_buffer);
      // Servo movement from min to max
      for (pos = min_deg; pos <= max_deg; pos += step) {
        myservo.write(pos);
        delay(step_time);
        if (digitalRead(pauseButtonPin) == LOW) {
          lcd.setCursor(0, 0);
          lcd.print("Paused            ");  // Ensure the length of the text matches the space available on the LCD
          lcd.setCursor(0, 1);
          lcd.print("           ");
          //Serial.println("Pause");
          running = false;
          break;  // Exit the servo movement loop
        }

        if (digitalRead(nextButtonPin) == LOW && !nextPressed) {
          lcd.clear();
          displayOption++;
          //Serial.println("Next Panel");
          nextPressed = true;
        } else if (displayOption % 3 == 2) {
          lcd.setCursor(0, 0);
          lcd.print("R/R0 (Ohm):");  // Ensure the length of the text matches the space available on the LCD
          lcd.setCursor(0, 1);
          lcd.print("Falta");

        } else if (displayOption % 3 == 0) {
          lcd.setCursor(0, 0);
          lcd.print("Ciclo/");  // Ensure the length of the text matches the space available on the LCD
          lcd.setCursor(0, 1);
          lcd.print(cycles);
          lcd.setCursor(6, 0);
          lcd.print(num_cycles);
        } else if (displayOption % 3 == 1) {
          lcd.setCursor(0, 0);
          lcd.print("Falta (hh:mm:ss):");  // Ensure the length of the text matches the space available on the LCD
          lcd.setCursor(0, 1);
          lcd.print(time_buffer);
        }
      }
      if (!running)
        break;                 // Exit the cycles loop if paused
      myservo.write(min_deg);  // Return the servo to its initial position
      nextPressed = false;
      delay(cycle_time);
    }
    lcd.setCursor(0, 1);
    lcd.print("Finalizado    ");
    running = false;  // Finish the cycles
  }
}