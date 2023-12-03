#include <Arduino.h>

const int EN_PIN = 5;         // Enable, active when LOW
const int DIR_PIN = 3;        // Direction, (DIR_PIN, isClockwise ? HIGH : LOW)
const int STEP_PIN = 4;       //1.8 degrees per step.

const int TOP_SENSOR_PIN = 13;
const int BOTTOM_SENSOR_PIN = 12;

const float STEP_ANGLE = 1.8;
const int MAX_POSITION = 180;
const int MIN_POSITION = 0;

int _angle_current_position = 0;

// Function declarations
void stopMotor();
void processCommand(String commandStr);
void moveMotor(int angle, int speedPercent);

void setup() {
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);   // Disable the motor initially

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

void moveMotor(int angle, int speedPercent) {
  int direction = (angle > _angle_current_position) ? 1 : -1;
  int steps = abs(angle / STEP_ANGLE - _angle_current_position / STEP_ANGLE);
  int delayTime = map(speedPercent, 0, 100, 1000, 20); 

  digitalWrite(EN_PIN, LOW); 
  digitalWrite(DIR_PIN, (direction == 1) ? HIGH : LOW);

  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(delayTime);

    _angle_current_position += direction;

    if (digitalRead(TOP_SENSOR_PIN) == LOW || digitalRead(BOTTOM_SENSOR_PIN) == LOW) {
      stopMotor();
      break;
    }
  }

  stopMotor();
  Serial.print("ST,0,RETMOVE,OK," + String(_angle_current_position) + "," + String(speedPercent) + ",ED\r\n");
}


void stopMotor() {
  digitalWrite(EN_PIN, HIGH);  // Disable the motor
  _angle_current_position = 0;
}

void processCommand(String commandStr) {
  commandStr.trim();

  String cmd = getValue(commandStr, ',', 2);

  if (cmd.equals("MOTOR")) {
    int angle = getValue(commandStr, ',', 4).toInt();
    int speedPercent = getValue(commandStr, ',', 5).toInt();

    if (angle > MAX_POSITION) {
      angle = MAX_POSITION;
    } else if (angle < MIN_POSITION) {
      angle = MIN_POSITION;
    }

    moveMotor(angle, speedPercent);
  } else if (cmd.equals("STOP")) {
    stopMotor();
    Serial.print("ST,0,RETSTOP,OK," + String(_angle_current_position) + ",ED\r\n");
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
//"ST,0,MOTOR,0,150,100,ED\r\n"
