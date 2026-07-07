/*
  Example 01 - Basic Readings

  The simplest way to read the SparkFun Qwiic CO2 Sensor (STCC4). This sketch starts
  continuous measurement and reads the CO2 concentration, temperature, and relative
  humidity about once per second.

  The board pairs the STCC4 with an SHT40 humidity and temperature sensor, wired to the
  STCC4's dedicated sensor interface pins. The STCC4 reads the SHT40 by itself, uses the
  values to compensate its CO2 output, and returns them with every measurement - so one
  readMeasurement() call gets you all three values, already compensated.

  Note: After a long time without power, the STCC4 can take up to an hour of operation to
  reach full accuracy, and it self-calibrates by assuming it sees fresh air (~400 ppm) at
  least once per week. The very first 20 seconds after the first ever power-up output a
  fixed bypass value of 390 ppm.

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

void setup()
{
    // Start serial right away so we can report what is happening.
    Serial.begin(115200);
    Serial.println("SparkFun STCC4 Example 1 - Basic Readings");

    // Start I2C communication.
    Wire.begin();

    // Attempt to connect to the sensor. Keep trying so the message is not missed if the
    // Serial Monitor is opened late.
    while (mySensor.begin() == false)
    {
        Serial.println("STCC4 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("STCC4 connected!");

    // Bring the sensor out of idle and into continuous measurement mode (1 s interval).
    mySensor.startContinuousMeasurement();

    // Wait for the first data point to become available.
    delay(1000);

    Serial.println("CO2 (ppm)\tTemperature (C)\tHumidity (%RH)");
}

void loop()
{
    // Read a fresh data point: CO2, plus the temperature and humidity the STCC4 gathered
    // from the onboard SHT40. This validates the CRC of every value before storing it, and
    // automatically retries briefly if the next data point is not quite ready yet.
    mySensor.readMeasurement();

    Serial.print(mySensor.getCO2());
    Serial.print("\t\t");
    Serial.print(mySensor.getTemperature(), 1);
    Serial.print("\t\t");
    Serial.println(mySensor.getHumidity(), 1);

    // The STCC4 produces a new data point every second.
    delay(1000);
}
