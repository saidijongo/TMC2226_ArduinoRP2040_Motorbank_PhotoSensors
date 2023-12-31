#include <Arduino.h>

const int EN_PIN = 5;
const int DIR_PIN = 3;
const int STEP_PIN = 4;

const int TOP_SENSOR_PIN = 12;
const int BOTTOM_SENSOR_PIN = 13;

const int RELAY_PIN[] = {2, 6, 7, 8};

const float SENSOR_BACK_STEP = 50;
const float STEP_ANGLE = 1.8;
const int STEP_DELAY = 1500;

float _step_start_position = 0;
float _step_current_position = 0;
float _step_target_position = 0;

int _step_delay = STEP_DELAY;
bool _stopCommandReceived = false;
bool _motorRunning = false;

void setup() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(TOP_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BOTTOM_SENSOR_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.flush();

  homePosition();

  for (int i = 0; i < sizeof(RELAY_PIN) / sizeof(RELAY_PIN[0]); i++) {
    pinMode(RELAY_PIN[i], OUTPUT);
    digitalWrite(RELAY_PIN[i], HIGH);
  }
}

String inputValues(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void stopMotor() {
  digitalWrite(EN_PIN, LOW);
  _motorRunning = false;
}

void motorStep(bool isClockwise, int steps) {
  for (int i = 0; i < steps; i++) {
    int step_current_position = _step_current_position;

    digitalWrite(DIR_PIN, isClockwise ? LOW : HIGH);
    delayMicroseconds(_step_delay);
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(_step_delay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(_step_delay);

    if (isClockwise)
      step_current_position++;
    else
      step_current_position--;

    _step_current_position = step_current_position;
  }
}

void homePosition() {
  while (digitalRead(TOP_SENSOR_PIN) == HIGH) {
    motorStep(false, 1);
  }

  stopMotor();
  delay(1000);
  motorStep(true, SENSOR_BACK_STEP);
  stopMotor();

  _step_start_position = 0;
  _step_current_position = 0;
}

void relayControl(String relayCommand) {
  String relayName = inputValues(relayCommand, ',', 2);
  relayName.toUpperCase();
  int relayState = inputValues(relayCommand, ',', 3).toInt();

  int relayPinIndex = -1;

  if (relayName.equals("PRINTER")) {
    relayPinIndex = 0;
  } else if (relayName.equals("DISPENSER")) {
    relayPinIndex = 1;
  } else if (relayName.equals("CARD_READER")) {
    relayPinIndex = 2;
  } else if (relayName.equals("PASSPORT_READER")) {
    relayPinIndex = 3;
  } else {
    Serial.print("ST,0," + relayName + ",ERR_INVALID_COMMAND,ED\r\n");
    return;
  }

  // Check and update the relay state
  if (relayState == 1 && digitalRead(RELAY_PIN[relayPinIndex]) == LOW) {
    // Turn ON (HIGH) only if it's currently OFF (LOW)
    digitalWrite(RELAY_PIN[relayPinIndex], HIGH);
  } else if (relayState == 0 && digitalRead(RELAY_PIN[relayPinIndex]) == HIGH) {
    // Turn OFF (LOW) only if it's currently ON (HIGH)
    digitalWrite(RELAY_PIN[relayPinIndex], LOW);
  }

  // Send the current relay state in the response
  Serial.print("ST,0," + relayName + ",OK," + String(digitalRead(RELAY_PIN[relayPinIndex])) + ",ED\r\n");
}

////////////////
void motorControl(String motorCommand) {
  int targetAngle = inputValues(motorCommand, ',', 4).toInt();
  int speedPercent = inputValues(motorCommand, ',', 5).toInt();

  int direction = (targetAngle > _step_current_position) ? 1 : -1;
  int steps = static_cast<int>(abs(targetAngle - _step_current_position) / STEP_ANGLE);

  _step_target_position = targetAngle;

  while (_step_current_position != _step_target_position) {
    bool isTopSensorLow = digitalRead(TOP_SENSOR_PIN) == LOW;
    bool isBottomSensorLow = digitalRead(BOTTOM_SENSOR_PIN) == LOW;

    if (_stopCommandReceived) {
      stopMotor();
      Serial.print("ST,0,MOTOR,0," + String(_step_current_position) + "," + String(speedPercent) + ",ED\r\n");
      _stopCommandReceived = false;
      return;
    }

    if (isTopSensorLow || isBottomSensorLow) {  //any sensor interrupted
      stopMotor();
      delay(1000);

      if (direction == 1) {
        motorStep(false, SENSOR_BACK_STEP); // Rotate 7 degrees CCW
      } else {
        motorStep(true, SENSOR_BACK_STEP); // Rotate 7 degrees CW
      }

      delay(1000);
      Serial.print("ST,0,MOTOR,0," + String(_step_current_position) + "," + String(speedPercent) + ",ED\r\n");
      return;
    }

    motorStep(direction == 1, 1);
  }

  Serial.print("ST,0,MOTOR,0," + String(_step_current_position) + "," + String(speedPercent) + ",ED\r\n");
}
/////////////////////
void commandProcess(String commandStr) {
  commandStr.trim();

  String cmd = inputValues(commandStr, ',', 2);

  if (cmd.equals("MOTOR")) {
    motorControl(commandStr);
  } else if (cmd.equals("DISPENSER") || cmd.equals("CARD_READER") || cmd.equals("PASSPORT_READER") || cmd.equals("PRINTER")) {
    relayControl(commandStr);
  } else {
    Serial.print("ST,0," + cmd + ",ERR_INVALID_COMMAND,ED\r\n");
  }
}


void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readString();
    commandProcess(data);
  }
}
//"ST,0,MOTOR,0,30,90,ED\r\n"
//"ST,0,CARD_READER,1,ED\r\n"
//"ST,0,PRINTER,1,ED\r\n"
//"ST,0,PRINTER,0,ED\r\n"
