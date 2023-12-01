#include <Arduino.h>

const int EN_PIN = 5;    // enable, active when LOW
const int DIR_PIN = 3;   // direction, (DIR_PIN, isClockwise ? HIGH : LOW)
const int STEP_PIN = 4;  // step, 1.8 degrees per step.

const int TOP_SENSOR_PIN = 13;
const int BOTTOM_SENSOR_PIN = 12;

int _angle_current_position = 0;
int _motor_state = 0; // 0 - Stopped, 1 - Clockwise, 2 - Counter Clockwise

void setup() {
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  pinMode(TOP_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BOTTOM_SENSOR_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.flush();
}

String getValue(String data, char separator, int index) {
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
  digitalWrite(EN_PIN, HIGH);
  _motor_state = 0;
  Serial.print("ST,0,STOP,OK," + String(_angle_current_position) + ",ED\r\n");
}

void moveMotor(int angle, int speedPercent) {
  int steps = abs(angle - _angle_current_position);
  bool isClockwise = (angle > _angle_current_position);

  digitalWrite(DIR_PIN, isClockwise ? HIGH : LOW);
  digitalWrite(EN_PIN, LOW);

  int stepDelay = map(speedPercent, 0, 100, 1600, 20); // Adjust this value

  for (int i = 0; i < steps; i++) {
    if (digitalRead(TOP_SENSOR_PIN) == LOW && isClockwise) {
      stopMotor();
      return;
    }

    if (digitalRead(BOTTOM_SENSOR_PIN) == LOW && !isClockwise) {
      stopMotor();
      return;
    }

    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay);

    if (isClockwise)
      _angle_current_position++;
    else
      _angle_current_position--;
  }

  stopMotor();
}

void processCommand(String commandStr) {
  commandStr.trim();

  String cmd = getValue(commandStr, ',', 2);

  if (cmd.equals("MOTOR")) {
    int angle = getValue(commandStr, ',', 4).toInt();
    int speedPercent = getValue(commandStr, ',', 5).toInt();

    moveMotor(angle, speedPercent);
  } else if (cmd.equals("STOP")) {
    stopMotor();
  } else {
    Serial.println("ST,0,RETINVALID,ERR_INVALID_COMMAND,ED\r\n");
  }
}

void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readString();
    processCommand(data);
  }
}
