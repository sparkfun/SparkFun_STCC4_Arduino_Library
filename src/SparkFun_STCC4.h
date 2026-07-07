/**
 * @file SparkFun_STCC4.h
 * @brief Arduino-specific implementation for the SparkFun Qwiic CO2 Sensor - STCC4.
 *
 * @details
 * This file provides the Arduino-specific wrapper for the SparkFun Qwiic CO2 Sensor - STCC4. The
 * SfeSTCC4ArdI2C class inherits from the platform-independent sfDevSTCC4 driver and implements I2C
 * communication using Arduino's Wire library via the SparkFun Toolkit.
 *
 * On this board the companion SHT40 humidity and temperature sensor is wired to the STCC4's
 * dedicated sensor interface pins, so the STCC4 reads it by itself: a single readMeasurement()
 * returns compensated CO2 plus ambient temperature and humidity. SfeSTCC4ArdI2C is the only class
 * a sketch needs. See the examples folder.
 *
 * @author SparkFun Electronics
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * @see https://github.com/sparkfun/SparkFun_STCC4_Arduino_Library
 */

#pragma once

// clang-format off
#include <SparkFun_Toolkit.h>
#include "sfTk/sfDevSTCC4.h"
#include <Arduino.h>
// clang-format on

/**
 * @class SfeSTCC4ArdI2C
 * @brief Arduino I2C implementation for the STCC4 CO2 sensor.
 *
 * @details
 * This class provides Arduino-specific I2C communication for the STCC4. It inherits all command
 * access methods from sfDevSTCC4 (including reset()) and adds the begin() method required for
 * Arduino initialization. The class owns an sfTkArdI2C bus object which wraps the Arduino Wire
 * library.
 *
 * @see sfDevSTCC4
 * @see TwoWire
 */
class SfeSTCC4ArdI2C : public sfDevSTCC4
{
  public:
    SfeSTCC4ArdI2C()
    {
    }

    /**
     * @brief Initializes the STCC4 with I2C communication.
     *
     * @details
     * Initializes the Toolkit I2C bus, confirms the device is present on the bus, then calls the
     * base class begin() to verify the device identity by reading and validating its product ID.
     * The identity check requires the sensor to be idle; if the sensor was left asleep or left
     * measuring (for example, the controller was reset or re-flashed mid-sketch while the sensor
     * kept power), the base class begin() recovers automatically by waking it or stopping the
     * running measurement. The sensor is always idle when begin() returns true. Worst case, the
     * measurement-stop recovery blocks for its 1.2 s execution time.
     *
     * @param address 7-bit I2C address of the device: 0x64 with the ADDR pin low (the default on
     * the SparkFun board), or 0x65 with the ADDR pin high.
     * @param wirePort TwoWire instance to use for I2C communication (default: Wire).
     *
     * @return true If initialization is successful.
     * @return false If any initialization step fails.
     */
    bool begin(uint8_t address = kI2CAddressDefault, TwoWire &wirePort = Wire)
    {
        // Initialize the Toolkit I2C bus with the given Wire port and address.
        if (_theI2CBus.init(wirePort, address) != ksfTkErrOk)
            return false;

        // Confirm a device is actually responding at this address before we read from it.
        if (_theI2CBus.ping() != ksfTkErrOk)
            return false;

        // The base class begin() verifies the device identity (and recovers it to idle if it was
        // left asleep or measuring).
        return sfDevSTCC4::begin(&_theI2CBus) == ksfTkErrOk;
    }

  private:
    /** @brief Arduino I2C bus interface instance used for all communication with the STCC4. */
    sfTkArdI2C _theI2CBus;
};
