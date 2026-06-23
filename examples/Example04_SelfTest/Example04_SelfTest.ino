/*
  Example 04 - Self-test

  The STCC4 has a built-in self-test that checks the supply voltage, internal memory, and
  whether it can see the SHT4x on its dedicated sensor interface pins. It is useful for
  verifying a board after assembly and for debugging. The test takes 360 ms; run it under
  stable conditions (steady supply, temperature, and CO2 concentration).

  Result interpretation (bit masks, see sfe_stcc4_self_test_t):
    0x0000 : everything passed
    bit 0  : supply voltage out of range
    bits 3:1 : debug flags - contact Sensirion if nonzero
    bit 4  : no SHT4x found on the STCC4's sensor interface pins
    bits 6:5 : memory error (try a soft reset, then a power cycle)

  Note: On the SparkFun Qwiic CO2 Sensor - STCC4, the onboard SHT40 is wired to the
  STCC4's sensor interface pins, so a fully healthy board reports 0x0000. If bit 4 is set,
  the STCC4 cannot talk to the SHT40 - its readings will fall back to defaults (25 C,
  50 %RH) and the CO2 output will not be properly compensated.

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
    Serial.begin(115200);
    Serial.println("SparkFun STCC4 Example 4 - Self-test");

    Wire.begin();

    while (mySensor.begin() == false)
    {
        Serial.println("STCC4 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("STCC4 connected!");

    // Run the self-test. This blocks for the 360 ms execution time.
    Serial.println("Running self-test...");

    uint16_t result = 0;
    if (mySensor.performSelfTest(result) != ksfTkErrOk)
    {
        Serial.println("Failed to run the self-test!");
        return;
    }

    Serial.print("Self-test result: 0x");
    Serial.println(result, HEX);

    if (result == STCC4_SELF_TEST_OK)
    {
        Serial.println("Self-test PASSED.");
        return;
    }

    Serial.println("Self-test FAILED:");

    if (result & STCC4_SELF_TEST_VDD_OUT_OF_RANGE)
        Serial.println("  - Supply voltage is out of the specified range");
    if (result & STCC4_SELF_TEST_DEBUG_MASK)
        Serial.println("  - Debug flags set; contact Sensirion for support");
    if (result & STCC4_SELF_TEST_SHT_NOT_CONNECTED)
        Serial.println("  - The STCC4 cannot see the SHT40 on its sensor interface pins");
    if (result & STCC4_SELF_TEST_MEMORY_ERROR_MASK)
        Serial.println("  - Memory error: soft reset the sensor, then power cycle if it persists");
}

void loop()
{
    // Nothing to do here.
}
