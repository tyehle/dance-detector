#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"

// Only reset if you are working in dev mode
#define FACTORYRESET_ENABLE 1

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

#define READ_BUFSIZE 20
uint8_t packetBuffer[READ_BUFSIZE+1];


// Cast four bytes as a float
float castFloat(uint8_t* buffer) {
  float f;
  memcpy(&f, buffer, 4);
  return f;
}



void printHex(const uint8_t * data, const uint32_t numBytes)
{
  uint32_t szPos;
  for (szPos=0; szPos < numBytes; szPos++) 
  {
    Serial.print(F("0x"));
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
    {
      Serial.print(F("0"));
      Serial.print(data[szPos] & 0xf, HEX);
    }
    else
    {
      Serial.print(data[szPos] & 0xff, HEX);
    }
    // Add a trailing space if appropriate
    if ((numBytes > 1) && (szPos != numBytes - 1))
    {
      Serial.print(F(" "));
    }
  }
  Serial.println();
}



uint8_t readBLE(Adafruit_BLE *ble, uint8_t buf[], uint8_t len) {
  uint8_t index = 0;
  memset(buf, 0, len);
  while(ble->available() && index < len) {
    buf[index] = ble->read();
    index++;
  }
  return index;
}


float readAccelMag(Adafruit_BLE *ble) {
  memset(packetBuffer, 0, READ_BUFSIZE);

  uint8_t timeout = 20;
  uint8_t remainingTries = timeout;

  uint8_t pos = 0;
  while(remainingTries--) {
    // find the beginning of a packet
    while(ble->available()) {
      char c = ble->read();
      remainingTries = timeout;

      if(pos == 0 && c != '!') {
        // this isn't the start of a packet
        continue;
      }

      if(pos == 1 && c != 'A') {
        // this isn't an accel packet
        pos = 0;
        continue;
      }

      packetBuffer[pos] = c;
      pos++;

      if(pos == 15) {
        // we have a full packet
        break;
      }
    }

    if(remainingTries == 0) {
      // if we failed to read a packet assume we aren't moving
      return -1;
    }

    if(pos == 15) {
      // we've got a full packet
      break;
    }

    delay(1);
  }

  // check the checksum
  uint8_t xsum = 0;
  for(uint8_t i = 0; i < pos-1; i++) {
    xsum += packetBuffer[i];
  }
  xsum = ~xsum;

  if(xsum != packetBuffer[pos - 1]) {
    Serial.print("Checksum mismatch in packet: ");
    printHex(packetBuffer, pos+1);
    return -1;
  }

  // find the accel data
  float x, y, z;
  x = castFloat(packetBuffer+2);
  y = castFloat(packetBuffer+6);
  z = castFloat(packetBuffer+10);

  return sqrt(x*x + y*y + z*z);
}



void error(char* message) {
  Serial.println(message);
  while(true) {}
}



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  delay(3000);

  Serial.begin(115200);
  Serial.println("Dance Detector ONLINE");
  Serial.println("---------------------");

  Serial.println("Initializing BLE module");
  if(!ble.begin(VERBOSE_MODE)) {
    error("Couldn't find the BLE module");
  }
  Serial.println("OK!");

  if(FACTORYRESET_ENABLE) {
    Serial.println("Doing a factory reset");
    if(!ble.factoryReset()) {
      error("Couldn't factory reset");
    }
  }

  ble.echo(false);

  Serial.println("Info dump:");
  ble.info();

  ble.verbose(false);

  // set the LED activity light. Options are DISABLE, MODE, BLEUART, HWUART, SPI, or, MANUAL
  ble.sendCommandCheckOK("AT+HWModeLED=MODE");

  // go into DATA mode
  ble.setMode(BLUEFRUIT_MODE_DATA);
}



void loop() {
  float motionCoeff = 0;

  while(true) {
    float a = readAccelMag(&ble);
    
    if(a >= 0) {
      motionCoeff += abs(a - 1);
      Serial.println(a - 1);
    }

    Serial.println(motionCoeff);

    motionCoeff *= 0.99;
    motionCoeff = min(5, motionCoeff);
    digitalWrite(LED_BUILTIN, motionCoeff > 3);
  }
}
