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
 * access methods from sfDevSTCC4 and adds the begin() method required for Arduino initialization,
 * along with reset(), which uses the I2C general call and therefore needs direct access to the bus.
 * The class owns an sfTkArdI2C bus object which wraps the Arduino Wire library.
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
     * kept power), this method recovers automatically by waking it or stopping the running
     * measurement. The sensor is always idle when begin() returns true. Worst case, the
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

        // The base class begin() verifies the device by reading and validating its product ID.
        if (sfDevSTCC4::begin(&_theI2CBus) == ksfTkErrOk)
            return true;

        // The identity check requires the sensor to be idle, so it fails if the sensor was left
        // in sleep mode or left measuring. Both states survive a controller reset or re-upload,
        // because the sensor keeps power from the bus the whole time. Recover from each in turn.

        // Sleep mode: send the wake-up byte and check again. exitSleepMode() re-verifies the
        // product ID once the sensor is awake.
        if (exitSleepMode() == ksfTkErrOk)
            return true;

        // Continuous measurement still running: stop it (this blocks for the 1.2 s execution
        // time) and check one last time.
        if (stopContinuousMeasurement() != ksfTkErrOk)
            return false;

        return isConnected();
    }

    /**
     * @brief Perform a soft reset of the sensor via the I2C general call.
     *
     * @details
     * The STCC4 soft reset is issued as an I2C general call: the single-byte reset command is sent
     * to address 0x00 rather than the sensor's own address, and is not acknowledged. This method
     * temporarily re-points the bus at the general call address to send the command, restores the
     * sensor address, and waits for the reset to complete. The sensor returns to the same state as
     * after a power cycle (idle mode, default compensation values).
     *
     * @note All devices on the bus that respond to an I2C general call reset will also reset.
     *
     * @return ::ksfTkErrOk on success, or an error code on failure.
     */
    sfTkError_t reset(void)
    {
        // Remember the configured sensor address - begin() may have selected the alternate.
        uint8_t sensorAddress = _theI2CBus.address();

        // Send the single-byte reset command to the I2C general call address. The command is not
        // acknowledged by the sensor, so ignore the write result.
        _theI2CBus.setAddress(kGeneralCallAddress);

        uint8_t command = kCommandSoftReset;
        (void)_theI2CBus.writeData(&command, sizeof(command));

        // Restore the sensor's own address for all subsequent communication.
        _theI2CBus.setAddress(sensorAddress);

        // Give the sensor time to complete the reset before it is addressed again.
        sftk_delay_ms(kSoftResetDelayMs);
        return ksfTkErrOk;
    }

  private:
    /// @brief Arduino I2C bus interface instance used for all communication with the STCC4.
    sfTkArdI2C _theI2CBus;
};
