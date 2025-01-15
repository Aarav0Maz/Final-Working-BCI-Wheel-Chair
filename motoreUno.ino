#include <Wire.h>
#include "AFMotor.h"

#define SLAVE_ADDRESS 8  // Define the slave address
#define ECHO_PIN 12      // Pin for the echo
#define TRIG_PIN 13      // Pin for the trigger
#define STOP_DISTANCE 5  // Distance in cm to stop the motors

// Define line tracking sensor pins
#define LINE_SENSOR_LEFT A0
#define LINE_SENSOR_CENTER A1
#define LINE_SENSOR_RIGHT A2

// Threshold for line detection (adjust for black color sensitivity)
#define LINE_THRESHOLD 300  // Lower value for more sensitivity to black color

// Initialize motors
AF_DCMotor motor1(1);
AF_DCMotor motor2(2);
AF_DCMotor motor3(3);
AF_DCMotor motor4(4);

volatile boolean commandReceived = false;
volatile char lastCommand = '0';
volatile boolean followLine = false;  // Flag to enable or disable line-following mode

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  // Initialize I2C
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
  
  // Set up distance sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Set up line tracking sensor pins
  pinMode(LINE_SENSOR_LEFT, INPUT);
  pinMode(LINE_SENSOR_CENTER, INPUT);
  pinMode(LINE_SENSOR_RIGHT, INPUT);

  Serial.println("\nI2C Slave Debug Mode");
  Serial.println("------------------");
  
  // Initialize motors
  motor1.setSpeed(0);
  motor2.setSpeed(0);
  motor3.setSpeed(0);
  motor4.setSpeed(0);

  Serial.println("Motors initialized");
  Serial.print("I2C Address: ");
  Serial.println(SLAVE_ADDRESS);
}

void loop() {
  // Check distance
  float distance = measureDistance();
  if (distance <= STOP_DISTANCE) {
    Serial.println("Obstacle detected! Stopping motors.");
    stopMotors();
    followLine = false;  // Disable line following if an obstacle is detected
    return;
  }

  if (commandReceived) {
    Serial.print("\nExecuting command: ");
    Serial.println(lastCommand);
    executeCommand(lastCommand);
    commandReceived = false;
  }

  // Perform line tracking if enabled
  if (followLine) {
    trackLine();
  } else {
    stopMotors();  // Stop the motors if not in line-following mode
  }
}

void receiveEvent(int numBytes) {
  Serial.print("\nReceived ");
  Serial.print(numBytes);
  Serial.println(" bytes");
  
  while (Wire.available()) {
    lastCommand = Wire.read();
    Serial.print("Command value: ");
    Serial.println(lastCommand);
    commandReceived = true;
  }
}

void executeCommand(char command) {
  switch (command) {
    case '1': // Enable Line Following
      Serial.println("Enabling Line Following Mode");
      followLine = true;
      break;

    case '0': // Stop Motors and Disable Line Following
      Serial.println("Stopping Motors and Disabling Line Following Mode");
      stopMotors();
      followLine = false;
      break;

    default:
      Serial.println("Invalid Command");
      followLine = false;  // Disable line-following mode for invalid commands
      stopMotors();
      break;
  }
}

void stopMotors() {
  motor1.setSpeed(0);
  motor2.setSpeed(0);
  motor3.setSpeed(0);
  motor4.setSpeed(0);
  motor1.run(RELEASE);
  motor2.run(RELEASE);
  motor3.run(RELEASE);
  motor4.run(RELEASE);
}

float measureDistance() {
  // Send a 10-microsecond pulse to the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the time for the echo
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance in cm
  float distance = (duration / 2.0) * 0.0343;

  Serial.print("Measured distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}

void trackLine() {
  int leftSensor = analogRead(LINE_SENSOR_LEFT);
  int centerSensor = analogRead(LINE_SENSOR_CENTER);
  int rightSensor = analogRead(LINE_SENSOR_RIGHT);

  Serial.print("Line Sensor Values: L=");
  Serial.print(leftSensor);
  Serial.print(", C=");
  Serial.print(centerSensor);
  Serial.print(", R=");
  Serial.println(rightSensor);

  // Center sensor detects black line and sensitivity adjusted
  if (centerSensor < LINE_THRESHOLD) {
    // Move forward if center sensor detects the line
    motor1.setSpeed(100);
    motor2.setSpeed(100);
    motor3.setSpeed(100);
    motor4.setSpeed(100);
    motor1.run(2);
    motor2.run(2);
    motor3.run(2);
    motor4.run(2);
    Serial.println("Moving Forward");
  } 
  // Left sensor detects the black line
  else if (leftSensor < LINE_THRESHOLD) {
    // Turn left if left sensor detects the line
    motor1.setSpeed(100);
    motor2.setSpeed(50);
    motor3.setSpeed(50);
    motor4.setSpeed(100);
    motor1.run(2);
    motor2.run(2);
    motor3.run(2);
    motor4.run(2);
    Serial.println("Turning Left");
  } 
  // Right sensor detects the black line
  else if (rightSensor < LINE_THRESHOLD) {
    // Turn right if right sensor detects the line
    motor1.setSpeed(50);
    motor2.setSpeed(100);
    motor3.setSpeed(100);
    motor4.setSpeed(50);
    motor1.run(2);
    motor2.run(2);
    motor3.run(2);
    motor4.run(2);
    Serial.println("Turning Right");
  } 
  // Stop if no line is detected
  else {
    stopMotors();
    Serial.println("Line Lost! Stopping.");
  }
}
