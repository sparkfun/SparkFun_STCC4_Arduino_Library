/*
  Example 05 - Forced Recalibration

  The STCC4 continuously self-calibrates by assuming it sees fresh air (~400 ppm CO2) at
  least once per week. If you need to correct the sensor immediately - for example after
  installing it, or if it cannot regularly see fresh air - you can force a recalibration
  against a known reference concentration.

  To use this sketch, place the board in air with a known CO2 concentration. Outdoor air
  works well: it is roughly 420 ppm. Set kTargetCO2 below to your reference value. The
  sketch then:
    1. Runs continuous measurement for 60 seconds so the reading stabilizes.
    2. Stops the measurement (the sensor must be idle for recalibration).
    3. Performs the forced recalibration and prints the correction that was applied.
    4. Restarts continuous measurement so you can watch the corrected output.

  Keep the environment (and the CO2 concentration!) stable for the whole procedure.
  To undo all forced recalibrations and self-calibration history, use performFactoryReset().

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

// The known CO2 concentration of the air around the sensor, in ppm. Outdoor air is ~420.
const uint16_t kTargetCO2 = 420;

// How long to measure before recalibrating. The datasheet requires at least 30 s.
const uint8_t kStabilizeSeconds = 60;

// Take one reading and print it. Returns true on success.
bool readAndPrint()
{
    if (mySensor.readMeasurement() != ksfTkErrOk)
    {
        Serial.println("Failed to read measurement!");
        return false;
    }

    Serial.print("CO2: ");
    Serial.print(mySensor.getCO2());
    Serial.println(" ppm");
    return true;
}

void setup()
{
    Serial.begin(115200);
    Serial.println("SparkFun STCC4 Example 5 - Forced Recalibration");

    Wire.begin();

    while (mySensor.begin() == false)
    {
        Serial.println("STCC4 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("STCC4 connected!");
    Serial.print("Recalibrating to ");
    Serial.print(kTargetCO2);
    Serial.println(" ppm. Keep the sensor in your reference air for the whole procedure.");
    Serial.println();

    // Step 1: measure for a while so the sensor output stabilizes in the reference air.
    if (mySensor.startContinuousMeasurement() != ksfTkErrOk)
    {
        Serial.println("Failed to start measurement. Halting.");
        while (1)
            ;
    }

    Serial.println("Stabilizing...");
    delay(1000);

    for (uint8_t i = 0; i < kStabilizeSeconds; i++)
    {
        readAndPrint();
        delay(1000);
    }

    // Step 2: the sensor must be idle for the recalibration command.
    Serial.println("Stopping measurement (takes 1.2 s)...");
    mySensor.stopContinuousMeasurement();

    // Step 3: recalibrate. The sensor reports the correction it applied to its output.
    int16_t frcCorrection = 0;
    if (mySensor.performForcedRecalibration(kTargetCO2, frcCorrection) != ksfTkErrOk)
    {
        Serial.println("Forced recalibration FAILED. Make sure the sensor measured for at");
        Serial.println("least 30 seconds before recalibrating, then try again.");
        return;
    }

    Serial.print("Forced recalibration applied. Correction: ");
    Serial.print(frcCorrection);
    Serial.println(" ppm");
    Serial.println();

    // Step 4: back to normal operation - watch the corrected output.
    if (mySensor.startContinuousMeasurement() != ksfTkErrOk)
    {
        Serial.println("Failed to restart measurement. Halting.");
        while (1)
            ;
    }

    delay(1000);
}

void loop()
{
    readAndPrint();
    delay(1000);
}
