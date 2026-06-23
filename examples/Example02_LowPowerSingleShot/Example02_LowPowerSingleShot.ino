/*
  Example 02 - Low Power Single Shot

  The STCC4 supports a low-power, on-demand "single shot" measurement mode: the sensor
  sleeps (about 1 uA) between measurements and is woken only when a reading is needed.
  With a 10 second sampling interval the sensor averages under 100 uA, compared to about
  950 uA in continuous mode - ideal for battery-powered projects.

  Each cycle this sketch:
    1. Wakes the STCC4 from sleep mode.
    2. Triggers a single shot measurement (takes 500 ms) and reads the result. The STCC4
       reads the onboard SHT40 itself, so the measurement arrives already compensated and
       includes temperature and humidity.
    3. Puts the STCC4 back to sleep and waits out the rest of the interval.

  Note: Keep the sampling interval between 5 and 600 seconds so the sensor's automatic
  self-calibration algorithm works correctly. The first 2 single shot measurements after
  the first ever power-up output a fixed bypass value of 390 ppm.

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

// Time between measurements. The sensor sleeps for most of this interval.
const unsigned long kSamplingIntervalMs = 10000;

void setup()
{
    Serial.begin(115200);
    Serial.println("SparkFun STCC4 Example 2 - Low Power Single Shot");

    Wire.begin();

    while (mySensor.begin() == false)
    {
        Serial.println("STCC4 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("STCC4 connected!");
    Serial.println("CO2 (ppm)\tTemperature (C)\tHumidity (%RH)");

    // Start with the sensor asleep; the loop wakes it for each measurement.
    mySensor.enterSleepMode();
}

void loop()
{
    unsigned long measurementStart = millis();

    // Wake the STCC4 from sleep mode (it confirms the sensor responded).
    if (mySensor.exitSleepMode() != ksfTkErrOk)
    {
        Serial.println("Failed to wake the sensor!");
        delay(kSamplingIntervalMs);
        return;
    }

    // Trigger one measurement (this blocks for the 500 ms measurement time) and read it.
    if (mySensor.measureSingleShot() == ksfTkErrOk && mySensor.readMeasurement() == ksfTkErrOk)
    {
        Serial.print(mySensor.getCO2());
        Serial.print("\t\t");
        Serial.print(mySensor.getTemperature(), 1);
        Serial.print("\t\t");
        Serial.println(mySensor.getHumidity(), 1);
    }
    else
    {
        Serial.println("Failed to read measurement!");
    }

    // Back to sleep until the next reading. Compensation values and the self-calibration
    // state are retained while asleep.
    mySensor.enterSleepMode();

    // Wait out the remainder of the sampling interval.
    unsigned long elapsed = millis() - measurementStart;
    if (elapsed < kSamplingIntervalMs)
        delay(kSamplingIntervalMs - elapsed);
}
