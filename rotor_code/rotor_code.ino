#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin definitions
const int LDR_PIN = A0;
const int laserPin = 9;
const int motorLeftPin = 8;
const int motorRightPin = 7;

const int THRESHOLD = 500; // Adjust based on your LDR readings

// LCD 16x2 I2C address 0x27 (change if needed)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// State machines for start and degree rotation
enum StartState { IDLE, WAIT_FIRST_LIGHT, WAIT_SECOND_LIGHT, DONE, TIMEOUT };
StartState startState = IDLE;

enum DegState { DEG_IDLE, DEG_RUNNING, DEG_DONE };
DegState degState = DEG_IDLE;

unsigned long startTime = 0;
unsigned long firstDetectTime = 0;
unsigned long secondDetectTime = 0;

unsigned long fullRotationTime = 0; // in milliseconds, from calibration or start

// Degree rotation vars
unsigned long degStartTime = 0;
unsigned long degRunDuration = 0;
int runDirection = 0; // 1 = left, 2 = right
bool isRunning = false;

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;     // whether the string is complete

// For LDR signal edge detection
bool lastLdrState = false;

void setup() {
  Serial.begin(9600);

  pinMode(LDR_PIN, INPUT);
  pinMode(laserPin, OUTPUT);
  pinMode(motorLeftPin, OUTPUT);
  pinMode(motorRightPin, OUTPUT);

  laserOff();
  stopMotor();

  inputString.reserve(50); // reserve some memory for inputString

  lcd.begin();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready");
}

void loop() {
  // Check for serial input
  if (stringComplete) {
    processCommand(inputString);
    inputString = "";
    stringComplete = false;
  }

  // Update start detection state machine
  if (startState == WAIT_FIRST_LIGHT || startState == WAIT_SECOND_LIGHT) {
    updateStartDetection();
  }

  // Update degree rotation
  if (degState == DEG_RUNNING) {
    unsigned long now = millis();
    if (now - degStartTime >= degRunDuration) {
      stopMotor();
      laserOff();
      degState = DEG_DONE;
      isRunning = false;
      Serial.println("deg,done");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Rotation done");
    }
  }
}

// Serial event handler
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      if (inputString.length() > 0) {
        stringComplete = true;
      }
    } else {
      inputString += inChar;
    }
  }
}

// Process commands from serial input
void processCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd.startsWith("start")) {
    int commaIndex = cmd.indexOf(',');
    if (commaIndex == -1) {
      Serial.println("error,missing_direction");
      return;
    }
    String dirStr = cmd.substring(commaIndex + 1);
    dirStr.trim();
    int dir = dirStr.toInt();
    if (dir != 1 && dir != 2) {
      Serial.println("error,invalid_direction");
      return;
    }
    if (isRunning) {
      Serial.println("error,busy");
      return;
    }
    runDirection = dir;
    startDetection(runDirection);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Calibrating...");
  }
  else if (cmd.startsWith("cal")) {
    if (isRunning) {
      Serial.println("error,busy");
      return;
    }
    if (fullRotationTime == 0) {
      Serial.println("error,norotationtime");
      return;
    }
    Serial.print("cal,");
    Serial.println(fullRotationTime);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Full rot:");
    lcd.setCursor(0, 1);
    lcd.print(fullRotationTime);
    lcd.print(" ms");
  }
  else if (cmd.startsWith("deg")) {
    int commaIndex = cmd.indexOf(',');
    if (commaIndex == -1) {
      Serial.println("error,missing_angle");
      return;
    }
    String angleStr = cmd.substring(commaIndex + 1);
    angleStr.trim();
    int angle = angleStr.toInt();
    if (angle <= 0 || angle > 360) {
      Serial.println("error,invalid_angle");
      return;
    }
    if (isRunning) {
      Serial.println("error,busy");
      return;
    }
    if (fullRotationTime == 0) {
      Serial.println("error,norotationtime");
      return;
    }

    // Ensure runDirection is set, default to 1 if not
    if (runDirection != 1 && runDirection != 2) {
      runDirection = 1;
    }

    degStart(angle);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Rotating ");
    lcd.print(angle);
    lcd.write(223); // Degree symbol
  }
  else if (cmd == "stop") {
    stopAll();
    Serial.println("stop,OK");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Stopped");
  }
  else {
    Serial.println("error,unknown_command");
  }
}

// Start detection state machine
void startDetection(int direction) {
  isRunning = true;
  runDirection = direction;

  laserOn();

  if (direction == 1) motorLeft();
  else motorRight();

  startState = WAIT_FIRST_LIGHT;
  startTime = millis();

  // Reset detection times
  firstDetectTime = 0;
  secondDetectTime = 0;

  lastLdrState = false;  // reset edge detection state
}

// Update detection for start command with edge detection and no forced 100ms delay
void updateStartDetection() {
  unsigned long now = millis();

  // Timeout check
  if (now - startTime > 15000) {  // 15 seconds timeout
    stopMotor();
    laserOff();
    isRunning = false;
    startState = TIMEOUT;
    Serial.println("start,timeout");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout");
    return;
  }

  int ldrVal = analogRead(LDR_PIN);
  bool currentLdrState = (ldrVal > THRESHOLD);

  // Detect rising edge: from false to true
  if (!lastLdrState && currentLdrState) {
    if (startState == WAIT_FIRST_LIGHT) {
      firstDetectTime = now;
      startState = WAIT_SECOND_LIGHT;

      // Short debounce delay (non-blocking recommended)
      // Just ignore other edges for next 50 ms
      delay(50);
    }
    else if (startState == WAIT_SECOND_LIGHT) {
      secondDetectTime = now;

      // Calculate time difference
      fullRotationTime = secondDetectTime - firstDetectTime;

      stopMotor();
      laserOff();
      isRunning = false;
      startState = DONE;

      Serial.print("start,");
      Serial.println(runDirection);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Calib done:");
      lcd.setCursor(0, 1);
      lcd.print(fullRotationTime);
      lcd.print(" ms");
    }
  }

  lastLdrState = currentLdrState;
}

// Degree rotation start
void degStart(int angle) {
  degRunDuration = (unsigned long)((fullRotationTime * angle) / 360.0);

  Serial.print("deg,duration=");
  Serial.println(degRunDuration);

  if (degRunDuration == 0) {
    Serial.println("error,zeroduration");
    return;
  }

  laserOn();

  if (runDirection == 1) motorLeft();
  else motorRight();

  degStartTime = millis();
  degState = DEG_RUNNING;
  isRunning = true;
}

// Stop motor and laser
void stopAll() {
  stopMotor();
  laserOff();
  isRunning = false;
  startState = IDLE;
  degState = DEG_IDLE;
}

// Motor control helpers
void motorLeft() {
  digitalWrite(motorLeftPin, LOW);
  digitalWrite(motorRightPin, HIGH);
}

void motorRight() {
  digitalWrite(motorLeftPin, HIGH);
  digitalWrite(motorRightPin, LOW);
}

void stopMotor() {
  digitalWrite(motorLeftPin, HIGH);
  digitalWrite(motorRightPin, HIGH);
}

void laserOn() {
  digitalWrite(laserPin, HIGH);
}

void laserOff() {
  digitalWrite(laserPin, LOW);
}
