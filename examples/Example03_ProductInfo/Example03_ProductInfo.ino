/*
  Example 03 - Product Info

  Every STCC4 reports a 32-bit product ID (0x0901018A) and a unique 64-bit serial number
  assigned by Sensirion during production. This sketch reads and prints both - handy for
  verifying communication and for telling boards apart.

  Note: The product ID can only be read while the STCC4 is in idle state (no measurement
  running). begin() leaves the sensor idle, so this sketch reads it before starting any
  measurement.

  SparkFun Electronics
  Date: 2026
  SparkFun code, firmware, and software is released under the MIT License.
    Please see LICENSE.md for further details.

  Hardware Connections:
  IoT RedBoard --> STCC4
  QWIIC --> QWIIC

  Open the Serial Monitor at 115200 baud.

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/
*/

#include <SparkFun_STCC4.h>

SfeSTCC4ArdI2C mySensor;

// Print an unsigned value as a fixed number of hexadecimal digits, with leading zeros.
void printHexPadded(uint32_t value, uint8_t digits)
{
    for (int8_t shift = (digits - 1) * 4; shift >= 0; shift -= 4)
    {
        uint8_t nibble = (value >> shift) & 0x0F;
        Serial.print(nibble, HEX);
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("SparkFun STCC4 Example 3 - Product Info");

    Wire.begin();

    while (mySensor.begin() == false)
    {
        Serial.println("STCC4 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("STCC4 connected!");
    Serial.println();

    // Read the product ID and unique serial number.
    uint32_t productId = 0;
    uint64_t serialNumber = 0;
    if (mySensor.getProductId(productId, serialNumber) != ksfTkErrOk)
    {
        Serial.println("Failed to read the product ID!");
        return;
    }

    Serial.print("Product ID: 0x");
    printHexPadded(productId, 8);
    Serial.println(" (expected 0x0901018A)");

    // A uint64_t cannot be printed directly on all platforms, so split it into the upper
    // and lower 32 bits and print each as zero-padded hex (8 + 8 = 16 digits).
    Serial.print("Serial number: 0x");
    printHexPadded((uint32_t)(serialNumber >> 32), 8);
    printHexPadded((uint32_t)(serialNumber & 0xFFFFFFFF), 8);
    Serial.println();
}

void loop()
{
    // Nothing to do here.
}
