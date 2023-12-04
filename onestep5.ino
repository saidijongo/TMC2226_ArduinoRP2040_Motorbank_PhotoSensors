#include <Arduino.h>

// Arduino Pins and Constant Values
const int EN_PIN = 5;  // enable, active when LOW
const int DIR_PIN = 3; // direction, (DIR_PIN, isClockwise ? LOW : HIGH)
const int STEP_PIN = 4; // step, for-loop step angles

const int TOP_SENSOR_PIN = 12;
const int BOTTOM_SENSOR_PIN = 13;

const float SENSOR_BACK_STEP = 50; // 3300 8 degrees 2850
const float STEP_ANGLE = 1.8; // 0.0072;
const int STEP_DELAY = 1000; // Initial step delay value 64

float _step_start_position = 0;
float _step_end_position = 0;
float _step_current_position = 0;
float _step_zero_angle = 50; // 8 degrees from the sensor

int _step_delay = STEP_DELAY; // Sensor response delay

bool isTopSensorLow = true;
bool isBottomSensorLow = true;

void stopMotor() {
  digitalWrite(EN_PIN, LOW);
}

void motorPWM(bool isClockwise, int steps) {
  for (int i = 0; i < steps; i++) {
    int step_current_position = _step_current_position; // Store the current position

    digitalWrite(DIR_PIN, isClockwise ? LOW : HIGH);
    delayMicroseconds(_step_delay);
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(_step_delay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(_step_delay);

    if (isClockwise)
      step_current_position++; // Update the stored position for clockwise movement
    else
      step_current_position--; // Update the stored position for counterclockwise movement

    _step_current_position = step_current_position; // Update the actual motor position
  }
}

void moveMotor(int angle, int speedPercent) {
  int steps = static_cast<int>((angle - _step_current_position) / STEP_ANGLE);

  int step_target_position = static_cast<int>((angle / STEP_ANGLE) + _step_zero_angle);

  bool isClockwise = (_step_current_position < step_target_position);

  int speed = map(speedPercent, 0, 100, 800, 20);
  Serial.print("ST,0,MOTOR,OK,0,0,ED\r\n"); // Acknowledge the command

  while (_step_current_position != step_target_position) {
    bool isTopSensorLow = digitalRead(TOP_SENSOR_PIN) == LOW;
    bool isBottomSensorLow = digitalRead(BOTTOM_SENSOR_PIN) == LOW;

    if (isTopSensorLow || isBottomSensorLow) {
      stopMotor();
      delay(1000);

      if (isClockwise) {
        motorPWM(false, SENSOR_BACK_STEP); // Rotate 7 degrees CCW
      } else {
        motorPWM(true, SENSOR_BACK_STEP); // Rotate 7 degrees CW
      }

      delay(1000);
      Serial.print("ST,0,MOTOR,ERR_INTERRUPT,0,0,ED\r\n");
      return;
    }

    motorPWM(isClockwise, 1);

    if (isClockwise == true && _step_current_position >= _step_end_position) {
      Serial.print("ST,0,MOTOR,ERR_LESS,0,0,ED\r\n");
      return;
    }

    if (isClockwise == false && _step_current_position <= _step_start_position) {
      Serial.print("ST,0,MOTOR,ERR_OVER,0,0,ED\r\n");
      return;
    }
  }

  Serial.print("_step_current_position: " + String(_step_current_position) + "\r\n");
  Serial.print("step_target_position: " + String(step_target_position) + "\r\n");

  Serial.print("Parsed angle: " + String(angle) + "\r\n");
  Serial.print("Parsed speedPercent: " + String(speedPercent) + "\r\n");

  Serial.print("ST,0,MOTOR,OK," + String(_step_current_position * STEP_ANGLE) + "," + String(speed) + ",ED\r\n");
}

void setup() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(TOP_SENSOR_PIN, INPUT_PULLUP);
  pinMode(BOTTOM_SENSOR_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.flush();

  // Rotate motor in a counter-clockwise direction until the TOP_SENSOR_PIN is interrupted
  while (digitalRead(TOP_SENSOR_PIN) == HIGH) {
    motorPWM(false, 1);
  }

  delay(1000);

  // Rotate 7 degrees in a clockwise direction and stop the motor
  motorPWM(true, static_cast<int>(SENSOR_BACK_STEP));

  // Mark this point as a home base
  _step_start_position = 0;
  _step_current_position = 0;
  _step_end_position = 0; 
}

void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readString();
    data.trim();

    if (data.startsWith("ST,0,MOTOR")) {
      int angle = data.substring(data.indexOf(",") + 1, data.lastIndexOf(",")).toInt();
      int speedPercent = data.substring(data.lastIndexOf(",") + 1, data.indexOf(",ED")).toInt();

      // Move the motor to the specified angle
      moveMotor(angle, speedPercent);
    }
  }
}

