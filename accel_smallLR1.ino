#include <AccelStepper.h>

#define EN_PIN         5  // enable
#define DIR_PIN        3  // direction
#define STEP_PIN       4  // step
#define TOP_SENSOR     12 // top sensor
#define BOTTOM_SENSOR  13 // bottom sensor

AccelStepper stepper(1, STEP_PIN, DIR_PIN);

void setup() {
  // Set pin modes
  pinMode(EN_PIN, OUTPUT);
  pinMode(TOP_SENSOR, INPUT_PULLUP);   // Using INPUT_PULLUP for low-active sensor
  pinMode(BOTTOM_SENSOR, INPUT_PULLUP);// Using INPUT_PULLUP for low-active sensor
  digitalWrite(EN_PIN, HIGH); // deactivate driver (LOW active)
  Serial.begin(9600); // Initialize serial communication
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    executeCommand(command);
  }
}

void executeCommand(String command) {
  command.trim(); // Remove leading and trailing whitespaces

  if (command == "CW") {
    rotateClockwiseUntilTopSensor();
  } else if (command == "CCW") {
    rotateCounterClockwiseUntilBottomSensor();
  } else if (command == "STOP") {
    stopMotor();
  } else {
    Serial.println("Invalid command");
  }
}

void rotateClockwiseUntilTopSensor() {
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(100);
  stepper.moveTo(3600); // 360 steps, adjust based on your motor specifications
  
  digitalWrite(EN_PIN, LOW); // Activate motor

  while (!digitalRead(TOP_SENSOR)) {
    stepper.run();
  }

  stepper.stop();
  delay(1000); // Delay to ensure the stepper motor stops before next action
  
  // Rotate 7 degrees in CCW
  stepper.moveTo(-7); // -7 steps, adjust based on your motor specifications
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  stopMotor();
}

void rotateCounterClockwiseUntilBottomSensor() {
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(100);
  stepper.moveTo(-3600); // -360 steps, adjust based on your motor specifications

  digitalWrite(EN_PIN, LOW); // Activate motor

  while (!digitalRead(BOTTOM_SENSOR)) {
    stepper.run();
  }

  stepper.stop();
  delay(1000); // Delay to ensure the stepper motor stops before next action
  
  // Rotate 7 degrees in CW
  stepper.moveTo(7); // 7 steps, 
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  stopMotor();
}

void stopMotor() {
  digitalWrite(EN_PIN, HIGH); // Deactivate motor
  delay(1000); // Delay to ensure the stepper motor stops before next action
  Serial.println("Motor Stopped");
}
