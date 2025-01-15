#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Wire.h>

#define CE_PIN 7
#define CSN_PIN 8
#define SLAVE_ADDRESS 8  // I2C address of the motor control Arduino

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

void setup() {
  Serial.begin(9600);

  // Initialize nRF24L01 as Receiver
  if (!radio.begin()) {
    Serial.println("Radio hardware is not responding!");
    while (1); // Halt execution
  }
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening(); // Set as receiver
  Serial.println("Radio Receiver Ready. Waiting for commands...");

  // Initialize I2C Master
  Wire.begin();
  Serial.println("I2C Master Ready. Transmitting to Slave...");
}

void loop() {
  if (radio.available()) {
    char command;
    radio.read(&command, sizeof(command)); // Read incoming command from nRF24L01

    Serial.print("Received command from radio: ");
    Serial.println(command);

    // Check if the command is valid
    if (command >= '0' && command <= '4') {
      // Send command to the I2C slave
      Wire.beginTransmission(SLAVE_ADDRESS);
      Wire.write(command);
      byte error = Wire.endTransmission();

      // Check if the I2C transmission was successful
      if (error == 0) {
        Serial.print("Command sent to I2C Slave: ");
        switch (command) {
          case '0':
            Serial.println("STOP");
            break;
          case '1':
            Serial.println("FORWARD");
            break;
          case '2':
            Serial.println("BACKWARD");
            break;
          case '3':
            Serial.println("LEFT");
            break;
          case '4':
            Serial.println("RIGHT");
            break;
        }
      } else {
        Serial.print("Error sending command to I2C Slave. Error code: ");
        Serial.println(error);
      }
    } else {
      Serial.println("Invalid command received from radio.");
    }
  }
}
