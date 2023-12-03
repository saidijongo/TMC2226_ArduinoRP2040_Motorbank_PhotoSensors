
#include <Arduino.h>

// Arduino Pins and Constant Values
const int EN_PIN = 5;  // enable, active when LOW
const int DIR_PIN = 3; // direction, (DIR_PIN, isClockwise ? LOW : HIGH)
const int STEP_PIN = 4; // step, for-loop step angles

const int TOP_SENSOR_PIN = 12;
const int BOTTOM_SENSOR_PIN = 13;

const float SENSOR_BACK_STEP = 50; // 8 degrees
const float STEP_ANGLE = 0.0072;   // 1.8 degrees
const int STEP_DELAY = 1000;       // Initial step delay value

float _step_start_position = 0;
float _step_end_position = 0;
float _step_current_position = 0;

float _step_zero_angle = 50;     // 8 degrees from the sensor

int _step_delay = STEP_DELAY; 
bool _stopCommandReceived = false;

void setup() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(TOP_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BOTTOM_SENSOR_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.flush();
  // initializeMotor(); 
}

void stopMotor() {
  digitalWrite(EN_PIN, LOW);
  //_motorState = MotorState::STOPPED;
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

void mainFcn(double angle, double speedPercent) {
  int maxSteps = 500000;

  bool isTopSensorLow = digitalRead(TOP_SENSOR_PIN) ==LOW;
  bool isBottomSensorLow = digitalRead(BOTTOM_SENSOR_PIN)==LOW;

  int old_step_delay = _step_delay;
  _step_delay = 1000;

  do {
    motorStep(false, 1);

    if (isTopSensorLow) {
      delay(_step_delay);
      stopMotor();
      return;
    }

  } while (isTopSensorLow = false);

  motorStep(true, 1);
  delay(_step_delay);
  stopMotor();

  _step_start_position = 0;
  _step_current_position = 0;

  int step_target_position = static_cast<int>(angle / STEP_ANGLE) + _step_zero_angle;

  bool isClockwise = (_step_current_position < step_target_position);

  int speed = map(speedPercent, 0, 100, 800, 20);
  String dir = isClockwise ? "CW" : "CCW";
  Serial.println(String(dir) + " Angle " + String(angle) + " Speed " + String(speed) + "\r\n");

  while (_step_current_position != step_target_position) {
    bool isTopSensorLow = digitalRead(TOP_SENSOR_PIN) == LOW;
    bool stopSensorBottom = digitalRead(BOTTOM_SENSOR_PIN) == LOW;

    if (_stopCommandReceived) {
      stopMotor();
      Serial.print("ST,0,RETSTOP,OK," + String(_step_current_position * STEP_ANGLE) + "," + String(speed) + ",ED\r\n");
      _stopCommandReceived = false;
      return;
    }

    if (isTopSensorLow || isBottomSensorLow) {
      stopMotor();
      delay(1000);

      if (isClockwise) {
        motorStep(false, SENSOR_BACK_STEP); // Rotate 7 degrees CCW
      } else {
        motorStep(true, SENSOR_BACK_STEP); // Rotate 7 degrees CW
      }

      delay(1000);
      Serial.print("ST,0,RETMOVE,ERR_INTERRUPT," + String(step_target_position) + "," + String(speedPercent) + ",ED\r\n");
      return;
    }

    motorStep(isClockwise, 1);

    if (isClockwise == true && _step_current_position >= _step_end_position) {
      Serial.print("ST,0,RETMOVE,ERR_LESS," + String(step_target_position) + "," + String(speedPercent) + ",ED\r\n");
      return;
    }

    if (isClockwise == false && _step_current_position <= _step_start_position) {
      Serial.print("ST,0,RETMOVE,ERR_OVER," + String(step_target_position) + "," + String(speedPercent) + ",ED\r\n");
      return;
    }
  }

  _step_end_position = _step_current_position;

  Serial.print("ST,0,RETMOVE,OK," + String(step_target_position) + "," + String(speedPercent) + ",ED\r\n");
}

void isanimSerial(String commandStr) {
  commandStr.trim();

  String cmd = inputValues(commandStr, ',', 2);

  if (cmd.equals("CONN")) {
    if (Serial.available() > 0) {
      Serial.print("ST,0,RETCONN,1,ED\r\n"); // Connected
    } else {
      Serial.print("ST,0,RETCONN,0,ED\r\n"); // NOT Connected
    }
    return;
  }

  if (cmd.equals("MOTOR")) {
    float degreeAngle = inputValues(commandStr, ',', 4).toFloat();
    float speedPercent = inputValues(commandStr, ',', 5).toFloat();
    mainFcn(degreeAngle, speedPercent);
    return;
  }

  if (cmd.equals("STOP")) {
    stopMotor();
    return;
  }
}

void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readString();
    isanimSerial(data);
  }
}
