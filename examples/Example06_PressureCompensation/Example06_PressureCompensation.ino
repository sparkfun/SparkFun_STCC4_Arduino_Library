/*
  Example 06 - Pressure Compensation

  The STCC4 assumes sea-level air pressure (101,300 Pa) by default. If you are at altitude,
  or you have a barometric pressure sensor handy, writing the true ambient pressure to the
  sensor improves CO2 accuracy. The value persists until overwritten or power-cycled, and
  the sensor accepts 40,000 to 110,000 Pa.

  (Humidity and temperature compensation need no help from you - the STCC4 reads the
  onboard SHT40 by itself. Pressure is the one input it cannot sense.)

  Set kAmbientPressurePa below for your location. Typical values by altitude:

    Altitude (m) | Pressure (Pa)
    -------------|--------------
    Sea level    | 101,300
    500          |  95,500
    1,000        |  89,900
    1,564 (SFE!) |  83,900
    2,000        |  79,500
    3,000        |  70,100

  If the pressure changes while your sketch runs (weather, or a moving platform), simply
  call setPressureCompensation() again - it is accepted during measurement.

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

// The ambient pressure at your location, in Pascals (1 hPa / mbar = 100 Pa). Defaults to sea
// level. For reference, SparkFun HQ in Boulder, Colorado sits at 1,564 m, where the pressure is
// about 83,900 Pa - set kAmbientPressurePa accordingly if you are at altitude.
const uint32_t kAmbientPressurePa = 101325;

void setup()
{
    Serial.begin(115200);
    Serial.println("SparkFun STCC4 Example 6 - Pressure Compensation");

    Wire.begin();

    while (mySensor.begin() == false)
    {
        Serial.println("STCC4 not connected, check your wiring!");
        delay(1000);
    }

    Serial.println("STCC4 connected!");

    // Tell the sensor the true ambient pressure before measuring.
    mySensor.setPressureCompensation(kAmbientPressurePa);

    Serial.print("Pressure compensation set to ");
    Serial.print(kAmbientPressurePa);
    Serial.println(" Pa");

    mySensor.startContinuousMeasurement();

    delay(1000);

    Serial.println("CO2 (ppm)\tTemperature (C)\tHumidity (%RH)");
}

void loop()
{
    mySensor.readMeasurement();

    Serial.print(mySensor.getCO2());
    Serial.print("\t\t");
    Serial.print(mySensor.getTemperature(), 1);
    Serial.print("\t\t");
    Serial.println(mySensor.getHumidity(), 1);

    delay(1000);
}
