#include <Servo.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <SD.h>

Servo myservo;

File myFile;  // Create servo object

// Cycle system variables, change accordingly
float max_deg = 180;  // Maximum degree the servo will rotate to
float min_deg = 90;   // Minimum degree the servo will rotate to
float pos = 0;        // Servo position variable

float step = 1;                                                                                                                        // Degree variation per loop
float step_time = 1;                                                                                                                   // Time between each loop in milliseconds
float cycle_time = 0;                                                                                                                  // Time between the start of one cycle and another
int num_cycles = 500;                                                                                                                  // Number of cycles to perform
bool running = false;                                                                                                                  // Indicates whether the experiment is running
float time_left = 1 + ((1120 + cycle_time) * num_cycles) / 1000.0 + (num_cycles * step_time * (max_deg - min_deg) / (1000.0 * step));  // Time left for the experiment
float time_total = time_left;                                                                                                          // Total time of the experiment
float time_cycle = (1120 + cycle_time) / 1000.0 + step_time * (max_deg - min_deg) / (1000.0 * step);                                   // Time per cycle

// Resistance measurement variables
int analogPin = 0;      // Analog pin for reading the voltage
int raw = 0;            // Raw value from analog read
int Vin = 5;            // Input voltage
float Vout = 0;         // Output voltage
float R1 = 1000;        // Known resistor value
float Rm = 0;           // Unknown sample resistance
float R0 = 3400;        // Original resistance of the sample
float ratio = Rm / R0;  // Ratio of sample resistance to original resistance
float buffer = 0;       // Buffer for calculations

// LCD variables
int Contrast = 50;                    // Contrast level of the LCD
LiquidCrystal lcd(8, 7, 5, 4, 3, 2);  // LCD pins configuration
int startButtonPin = A1;              // Pin for starting the experiment
int pauseButtonPin = A2;              // Pin for pausing or resuming the experiment
int nextButtonPin = A3;               // Pin for going to the next display option
bool nextPressed = false;             // Prevents multiple presses causing lag
int displayOption = 1;                // Current display option

void setup() {
  myservo.attach(A4);  // Attach the servo to pin A4

  pinMode(startButtonPin, INPUT_PULLUP);
  pinMode(pauseButtonPin, INPUT_PULLUP);
  pinMode(nextButtonPin, INPUT_PULLUP);

  analogWrite(6, Contrast);  // Set the LCD contrast
  lcd.begin(16, 2);          // Initialize the LCD
  Serial.begin(9600);        // Start serial communication for debugging
}

// Formats time in seconds into hh:mm:ss format
void formatTime(float seconds, char* buffer) {
  int hours = seconds / 3600;
  int minutes = (seconds - (hours * 3600)) / 60;
  int secs = seconds - (hours * 3600) - (minutes * 60);
  sprintf(buffer, "%02d:%02d:%02d", hours, minutes, secs);
}

void loop() {
  // Start the experiment if start button is pressed and not already running
  if (digitalRead(startButtonPin) == LOW && !running) {
    running = true;  // Start the experiment
    lcd.setCursor(0, 0);
    lcd.print("Measuring...       ");  // Ensure the text fits the LCD
    time_left = time_total - 1 + time_cycle;
    delay(1000);  // Debounce delay
  }

  while (running) {
    if (!SD.begin(9)) {  // Initialize SD card
      Serial.println("initialization failed!");
      while (1)
        ;
    }
    Serial.println("initialization done.");

    myFile = SD.open("TFG.txt", FILE_WRITE);  // Open file for writing

    if (!myFile) {  // Check if file opened successfully
      Serial.println("error opening TFG.txt");
      while (1)
        ;
    }

    for (int cycles = 1; cycles <= num_cycles; cycles++) {  // Loop through cycles
      time_left -= time_cycle;                              // Update remaining time
      char time_buffer[9];
      formatTime(time_left, time_buffer);  // Format remaining time
      Serial.println(time_left);           // Print remaining time to serial
      raw = analogRead(analogPin);         // Read analog value
      if (raw != 0) {
        buffer = raw * Vin;
        Vout = buffer / 1024.0;  // Calculate output voltage
        buffer = (Vin / Vout) - 1;
        Rm = R1 * buffer;  // Calculate unknown resistance
        ratio = Rm / R0;   // Calculate resistance ratio
      }

      // Write ratio and cycle to SD card
      myFile.print(cycles);
      myFile.print(" ");
      myFile.println(ratio);

      // Servo movement from min to max
      for (pos = min_deg; pos <= max_deg; pos += step) {
        myservo.write(pos);
        delay(step_time);
        if (digitalRead(pauseButtonPin) == LOW) {
          lcd.setCursor(0, 0);
          lcd.print("Stopped          ");  // Ensure the length of the text matches the space available on the LCD
          lcd.setCursor(0, 1);
          lcd.print("           ");
          running = false;
          break;  // Exit the servo movement loop
        }

        if (digitalRead(nextButtonPin) == LOW && !nextPressed) {
          lcd.clear();
          displayOption++;
          nextPressed = true;  // Prevent multiple presses
        } else if (displayOption % 3 == 2) {
          lcd.setCursor(0, 0);
          lcd.print("R/R0 (Ohm):");  // Ensure the length of the text matches the space available on the LCD
          lcd.setCursor(0, 1);
          lcd.print(ratio);

        } else if (displayOption % 3 == 1) {
          lcd.setCursor(0, 0);
          lcd.print("Cycle/");  // Ensure the length of the text matches the space available on the LCD
          lcd.setCursor(0, 1);
          lcd.print(cycles);
          lcd.setCursor(6, 0);
          lcd.print(num_cycles);

        } else if (displayOption % 3 == 0) {
          lcd.setCursor(0, 0);
          lcd.print("Left (hh:mm:ss):");  // Ensure the length of the text matches the space available on the LCD
          lcd.setCursor(0, 1);
          lcd.print(time_buffer);
        }
      }
      nextPressed = false;
      if (!running)
        break;                 // Exit the cycles loop if paused
      myservo.write(min_deg);  // Return the servo to its initial position
    }
    myFile.close();  // Close the file after finishing all cycles
    lcd.setCursor(0, 0);
    lcd.print("Finished    ");
    lcd.setCursor(0, 1);
    lcd.print("              ");
    running = false;  // Finish the cycles
  }
}