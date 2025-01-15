//sender with radio
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

void setup() {
  Serial.begin(9600);
  if (!radio.begin()) {
    Serial.println("Radio hardware is not responding!");
    while (1); // Halt execution
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening(); // Set as transmitter
  Serial.println("Master Ready. Send '1' or '0' to control the motors.");
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read(); // Read user input

    // Process valid commands: '1' or '0'
    if (command >= '0' && command <= '4') {
      bool success = radio.write(&command, sizeof(command)); // Send the command
      if (success) {
        Serial.print("Command sent: ");
        switch (command) {
          case '1':
            Serial.println("FORWARD");
            break;
          case '0':
            Serial.println("STOP");
            break;
        }
      } else {
        Serial.println("Failed to send command. Check receiver and connection.");
      }
    } else {
      Serial.println("Invalid command. Use only '1' or '0'.");
    }

    // Clear any remaining characters in the serial buffer
    while (Serial.available()) {
      Serial.read();
    }
  }
}
