/**
 * @file sfDevSTCC4.h
 * @brief Header file for the SparkFun STCC4 CO2 Sensor Driver.
 *
 * @details
 * sfDevSTCC4 is a comms-agnostic driver for the Sensirion STCC4 miniature CO2 sensor, built on
 * the SparkFun Toolkit. The STCC4 measures CO2 concentration (ppm) using the thermal conductivity
 * sensing principle and supports both a continuous measurement mode (1 s sampling interval) and a
 * low-power single shot measurement mode.
 *
 * The STCC4 requires external relative humidity and temperature values for accurate compensation
 * of the CO2 output. On the SparkFun Qwiic CO2 Sensor - STCC4, the companion SHT40 sensor is wired
 * to the STCC4's dedicated I2C controller interface pins, so the STCC4 reads it autonomously: every
 * measurement returns CO2 already compensated, along with the ambient temperature and humidity. In
 * custom designs without an SHT4x on those pins, the host must supply ambient conditions with
 * setRHTCompensation() instead.
 *
 * The STCC4 uses Sensirion's command-based I2C protocol: 16-bit commands are sent most significant
 * byte first, and data words (sent and received) are 16 bits followed by a CRC-8 byte.
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

#include <stddef.h>
#include <stdint.h>

// SparkFun Toolkit core headers
#include <sfTk/sfToolkit.h>
#include <sfTk/sfTkII2C.h>

///////////////////////////////////////////////////////////////////////////////
// Self-test Result Bits
///////////////////////////////////////////////////////////////////////////////
/// @brief Bit masks for decoding the result word returned by performSelfTest().
/// @details A successful self-test returns 0x0000. On the SparkFun Qwiic CO2 Sensor - STCC4 the
/// onboard SHT40 is wired to the STCC4's dedicated I2C controller interface pins, so a healthy
/// board reports 0x0000; a result of 0x0010 means the STCC4 cannot see the SHT40.
typedef enum sfe_stcc4_self_test_t : uint16_t
{
    STCC4_SELF_TEST_OK = 0x0000,                ///< All checks passed.
    STCC4_SELF_TEST_VDD_OUT_OF_RANGE = 0x0001,  ///< Bit 0: supply voltage out of the specified range.
    STCC4_SELF_TEST_DEBUG_MASK = 0x000E,        ///< Bits 3:1: for debugging; contact Sensirion if nonzero.
    STCC4_SELF_TEST_SHT_NOT_CONNECTED = 0x0010, ///< Bit 4: no SHT4x on the STCC4 controller interface pins.
    STCC4_SELF_TEST_MEMORY_ERROR_MASK = 0x0060  ///< Bits 6:5: memory error (soft reset / power cycle to clear).
} sfe_stcc4_self_test_t;

///////////////////////////////////////////////////////////////////////////////
// Class Declaration
///////////////////////////////////////////////////////////////////////////////

/// @brief Platform-independent driver for the Sensirion STCC4 CO2 sensor.
///
/// @details This class implements command-level access to the STCC4 via the SparkFun Toolkit bus
/// interface. Most methods return a SparkFun Toolkit error code (::ksfTkErrOk on success, a negative
/// value on error). The CRC-8 checksum of every received data word is validated automatically;
/// ::ksfTkErrFail is returned if any checksum does not match.
///
/// The typical continuous-mode flow is: begin(), startContinuousMeasurement(), then once per second
/// call readMeasurement() and read the converted values with getCO2() / getTemperature() /
/// getHumidity(). On the SparkFun board the STCC4 reads the onboard SHT40 by itself, so the CO2
/// value arrives already compensated and the temperature / humidity values are real ambient
/// readings. Only designs without an SHT4x on the STCC4's controller interface pins need to supply
/// ambient conditions with setRHTCompensation().
class sfDevSTCC4
{
  public:
    sfDevSTCC4() : _theBus{nullptr}
    {
    }

    /// @brief Initialize the device driver with the given bus.
    /// @details Adopts the supplied bus and confirms an STCC4 is responding by reading and
    /// validating its product ID. The sensor must be in idle state (the power-on default) for this
    /// check, so call begin() before starting a measurement.
    /// @param theBus Pointer to the initialized bus object. If null, a bus set by a prior call is used.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t begin(sfTkIBus *theBus = nullptr);

    /// @brief Check whether the STCC4 is connected and responding.
    /// @details Reads the product ID, validates its CRC, and compares it against the STCC4 product
    /// ID (0x0901018A). Requires the sensor to be in idle state (no measurement running, not asleep).
    /// @return true if the device responds with a valid STCC4 product ID, false otherwise.
    bool isConnected(void);

    // ========================= Measurement Control ========================

    /// @brief Start continuous measurement mode.
    /// @details Brings the sensor from idle into continuous measurement mode with a 1 s sampling
    /// interval. Wait 1 s after starting before calling readMeasurement(). The sensor must be in
    /// idle state (this command is not accepted while a measurement is running).
    /// @note During the first 20 s of continuous mode after the very first power-up, the sensor
    /// outputs a fixed bypass value of 390 ppm (see the datasheet, "Initial Operation").
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t startContinuousMeasurement(void);

    /// @brief Stop continuous measurement mode and return the sensor to idle.
    /// @details Waits the command's 1.2 s execution time before returning. During that time the
    /// sensor does not acknowledge its I2C address or accept commands.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t stopContinuousMeasurement(void);

    /// @brief Perform a single shot (on-demand) CO2 measurement.
    /// @details Triggers one measurement and waits the 500 ms execution time before returning. Call
    /// readMeasurement() afterwards to fetch the result. The sensor must be in idle state. For
    /// low-power operation, combine with enterSleepMode() / exitSleepMode() and keep the sampling
    /// interval between 5 s and 600 s so the automatic self-calibration algorithm works correctly.
    /// @note The first 2 single shot measurements after the very first power-up output a fixed
    /// bypass value of 390 ppm (see the datasheet, "Initial Operation").
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t measureSingleShot(void);

    /// @brief Read one measurement data point and cache the result.
    /// @details Reads the CO2, temperature, humidity and status words, validates the CRC of each,
    /// and stores the raw values for retrieval via getCO2(), getTemperature(), getHumidity(), and
    /// getStatus(). The sensor NACKs the read when no new data point is available (its internal 1 s
    /// interval has a ±150 ms tolerance), in which case this method waits 150 ms and retries, up to
    /// @p maxAttempts total attempts.
    /// @note The temperature and humidity words are the values the sensor is using for
    /// compensation. On the SparkFun board these are live readings from the onboard SHT40 (wired
    /// to the STCC4's controller interface pins); in designs without an SHT4x there, they echo the
    /// values last written with setRHTCompensation() (defaults: 25 °C / 50 %RH after power-up).
    /// @param maxAttempts Total number of read attempts before giving up (default 4).
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on a
    /// communication failure / no data available.
    sfTkError_t readMeasurement(uint8_t maxAttempts = kReadMeasurementAttempts);

    // ====================== Cached Measurement Values =====================

    /// @brief Get the CO2 concentration from the most recent readMeasurement().
    /// @details The output range is 380 to 32'000 ppm; the value is signed because readings can
    /// undershoot slightly at the low end of the range.
    /// @return CO2 concentration in parts per million (ppm).
    int16_t getCO2(void);

    /// @brief Get the relative humidity from the most recent readMeasurement().
    /// @details This echoes the humidity the sensor is using for compensation (see
    /// readMeasurement()). The value is clamped to the 0–100 %RH range.
    /// @return Relative humidity in percent (%RH).
    float getHumidity(void);

    /// @brief Get the temperature from the most recent readMeasurement(), in degrees Celsius.
    /// @details This echoes the temperature the sensor is using for compensation (see
    /// readMeasurement()).
    /// @return Temperature in degrees Celsius (°C).
    float getTemperature(void);

    /// @brief Get the temperature from the most recent readMeasurement(), in degrees Fahrenheit.
    /// @return Temperature in degrees Fahrenheit (°F).
    float getTemperatureF(void);

    /// @brief Get the raw 16-bit sensor status word from the most recent readMeasurement().
    /// @return The raw status word (use isTestingModeEnabled() to decode the testing mode bit).
    uint16_t getStatus(void);

    /// @brief Check whether the sensor is in testing mode (automatic self-calibration paused).
    /// @details Reflects the status word from the most recent readMeasurement(). See
    /// enableTestingMode() / disableTestingMode().
    /// @return true if testing mode is enabled, false otherwise.
    bool isTestingModeEnabled(void);

    // ===================== Raw Cached Measurement Ticks ===================

    /// @brief Get the raw humidity ticks from the most recent readMeasurement().
    /// @return Raw 16-bit humidity value (%RH = 125 * ticks / 65535 - 6).
    uint16_t getHumidityRaw(void);

    /// @brief Get the raw temperature ticks from the most recent readMeasurement().
    /// @return Raw 16-bit temperature value (°C = 175 * ticks / 65535 - 45).
    uint16_t getTemperatureRaw(void);

    // ========================== Compensation ==============================

    /// @brief Write external relative humidity and temperature compensation values.
    /// @details For custom designs without an SHT4x on the STCC4's dedicated I2C controller
    /// interface pins. The STCC4 uses these values to compensate the CO2 output; they take effect
    /// within one measurement interval and persist until overwritten or power-cycled (defaults:
    /// 25 °C / 50 %RH). Both values must come from the same RHT sensor, and for best accuracy
    /// should be refreshed regularly (for example once per measurement cycle). May be called while
    /// a measurement is running.
    /// @warning Do NOT use this method when an SHT4x is wired to the STCC4's controller interface
    /// pins — as it is on the SparkFun Qwiic CO2 Sensor - STCC4, where the STCC4 manages the
    /// onboard SHT40 (and compensation) entirely by itself.
    /// @param temperature Ambient temperature in degrees Celsius (clamped to -45 to 130 °C).
    /// @param humidity Ambient relative humidity in percent (clamped to 0 to 100 %RH).
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t setRHTCompensation(float temperature, float humidity);

    /// @brief Write an external ambient pressure compensation value.
    /// @details Improves CO2 accuracy when operating away from the default 101'300 Pa (sea level),
    /// for example at altitude. The value takes effect within one measurement interval and persists
    /// until overwritten or power-cycled. The sensor accepts 40'000 to 110'000 Pa (clamped here).
    /// May be called while a measurement is running.
    /// @param pascals Ambient pressure in Pascals (1 hPa / mbar = 100 Pa).
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t setPressureCompensation(uint32_t pascals);

    // ========================= Power Management ===========================

    /// @brief Put the sensor into its lowest-power sleep mode.
    /// @details The sensor must be in idle state. Compensation values and the automatic
    /// self-calibration state are retained during sleep. Wake the sensor with exitSleepMode().
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t enterSleepMode(void);

    /// @brief Wake the sensor from sleep mode into idle mode.
    /// @details Sends the wake-up byte (which the sensor intentionally does not acknowledge), waits
    /// the 5 ms execution time, then confirms the sensor is awake by reading its product ID.
    /// @return ::ksfTkErrOk on success, or ::ksfTkErrBusNoResponse if the sensor did not wake.
    sfTkError_t exitSleepMode(void);

    // ====================== Calibration & Maintenance =====================

    /// @brief Condition the sensor to improve initial accuracy after long idle periods.
    /// @details Recommended when the sensor has not measured for more than 3 hours. Runs a fixed
    /// operation profile on the sensor and BLOCKS for its full 22 s execution time. The sensor must
    /// be in idle state; start a measurement afterwards.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t performConditioning(void);

    /// @brief Perform a forced recalibration (FRC) of the CO2 output.
    /// @details Adjusts the sensor output to match an externally known CO2 concentration (for
    /// example outdoor air at ~420 ppm). Before calling: operate the sensor for at least 30 s of
    /// continuous measurement (or 30 single shots), then stop continuous measurement — the sensor
    /// must be in idle state, with environmental conditions held stable. Blocks for the 90 ms
    /// execution time.
    /// @param targetCO2 The known reference CO2 concentration, in ppm (0 to 32'000).
    /// @param frcCorrection Output reference that receives the applied correction in ppm.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail if the sensor reports the recalibration
    /// failed, or an error code on a communication failure.
    sfTkError_t performForcedRecalibration(uint16_t targetCO2, int16_t &frcCorrection);

    /// @brief Reset the forced recalibration and automatic self-calibration history.
    /// @details Returns the calibration state to factory defaults and re-enables the initial
    /// bypass phase. The sensor must be in idle state. Blocks for the 90 ms execution time.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail if the sensor reports the reset failed, or
    /// an error code on a communication failure.
    sfTkError_t performFactoryReset(void);

    /// @brief Run the on-chip self-test.
    /// @details Checks sensor functionality; useful for end-of-line testing and debugging. Run it
    /// under stable conditions. The sensor must be in idle state. Blocks for the 360 ms execution
    /// time. A healthy SparkFun Qwiic CO2 Sensor - STCC4 reports ::STCC4_SELF_TEST_OK;
    /// ::STCC4_SELF_TEST_SHT_NOT_CONNECTED means the STCC4 cannot see the onboard SHT40 on its
    /// controller interface pins.
    /// @param result Output reference that receives the raw 16-bit self-test result (see
    /// sfe_stcc4_self_test_t for bit definitions).
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on failure.
    sfTkError_t performSelfTest(uint16_t &result);

    // =========================== Testing Mode =============================

    /// @brief Pause the automatic self-calibration (ASC) algorithm.
    /// @details Temporarily disables updates to the ASC state, for example while characterizing the
    /// sensor with reference gases. May be called while a measurement is running. Check the mode
    /// with isTestingModeEnabled() after the next readMeasurement().
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t enableTestingMode(void);

    /// @brief Resume the automatic self-calibration (ASC) algorithm.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t disableTestingMode(void);

    // ============================= Identity ================================

    /// @brief Read the product ID and unique serial number.
    /// @details The STCC4 product ID is 0x0901018A. The 64-bit serial number is unique to each
    /// sensor. The sensor must be in idle state.
    /// @param productId Output reference that receives the 32-bit product ID.
    /// @param serialNumber Output reference that receives the 64-bit serial number.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on failure.
    sfTkError_t getProductId(uint32_t &productId, uint64_t &serialNumber);

  protected:
    /// @brief Send a 16-bit command to the sensor (most significant byte first).
    /// @param command The 16-bit command code.
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t sendCommand(uint16_t command);

    /// @brief Send a 16-bit command followed by CRC-protected 16-bit argument words.
    /// @details Each argument is transmitted as two data bytes (MSB first) followed by its CRC-8.
    /// @param command The 16-bit command code.
    /// @param args Pointer to the argument words.
    /// @param numArgs Number of argument words (no more than kMaxArgWords).
    /// @return ::ksfTkErrOk on success, or an error code on failure.
    sfTkError_t sendCommand(uint16_t command, const uint16_t *args, uint8_t numArgs);

    /// @brief Send a command and read back a sequence of CRC-protected 16-bit words.
    /// @details Each word arrives as two data bytes (MSB first) followed by a CRC-8 byte. The CRC of
    /// every word is validated before the word is stored.
    /// @param command The 16-bit command code that requests the data.
    /// @param words Output buffer that receives @p numWords 16-bit words.
    /// @param numWords Number of 16-bit words to read (no more than kMaxWords).
    /// @param readDelayMs Delay, in milliseconds, between sending the command and reading.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on failure.
    sfTkError_t readWords(uint16_t command, uint16_t *words, uint8_t numWords, uint32_t readDelayMs = 0);

    /// @brief Send a command with CRC-protected argument words, then read back CRC-protected words.
    /// @param command The 16-bit command code.
    /// @param args Pointer to the argument words (may be null when @p numArgs is 0).
    /// @param numArgs Number of argument words (no more than kMaxArgWords).
    /// @param words Output buffer that receives @p numWords 16-bit words.
    /// @param numWords Number of 16-bit words to read (no more than kMaxWords).
    /// @param readDelayMs Delay, in milliseconds, between sending the command and reading.
    /// @return ::ksfTkErrOk on success, ::ksfTkErrFail on a CRC mismatch, or an error code on failure.
    sfTkError_t readWords(uint16_t command, const uint16_t *args, uint8_t numArgs, uint16_t *words, uint8_t numWords,
                          uint32_t readDelayMs);

    /// @brief Compute the Sensirion CRC-8 over a buffer of data bytes.
    /// @details Polynomial 0x31, initial value 0xFF, no final XOR.
    /// @param data Pointer to the data bytes.
    /// @param length Number of bytes to include in the checksum.
    /// @return The 8-bit checksum.
    static uint8_t computeCRC8(const uint8_t *data, size_t length);

    sfTkIBus *_theBus; ///< Pointer to the communication bus device.

    // --- Cached raw values from the most recent readMeasurement() ---
    uint16_t _co2Ticks = 0;         ///< Raw CO2 value (signed ppm).
    uint16_t _temperatureTicks = 0; ///< Raw temperature value.
    uint16_t _humidityTicks = 0;    ///< Raw relative humidity value.
    uint16_t _status = 0;           ///< Raw 16-bit sensor status word.

    ///////////////////////////////////////////////////////////////////////////
    // I2C Addressing
    ///////////////////////////////////////////////////////////////////////////
    static const uint8_t kI2CAddressDefault = 0x64;  ///< 7-bit I2C address with the ADDR pin low (default).
    static const uint8_t kI2CAddressAlt = 0x65;      ///< 7-bit I2C address with the ADDR pin high.
    static const uint8_t kGeneralCallAddress = 0x00; ///< I2C general call address used for soft reset.

    ///////////////////////////////////////////////////////////////////////////
    // Command Codes
    ///////////////////////////////////////////////////////////////////////////
    static const uint16_t kCommandStartContinuousMeasurement = 0x218B; ///< Start continuous measurement (1 s interval).
    static const uint16_t kCommandStopContinuousMeasurement = 0x3F86;  ///< Stop continuous measurement.
    static const uint16_t kCommandReadMeasurement = 0xEC05;            ///< Read measurement data (12 bytes).
    static const uint16_t kCommandSetRHTCompensation = 0xE000;         ///< Write RH/T compensation values.
    static const uint16_t kCommandSetPressureCompensation = 0xE016;    ///< Write the pressure compensation value.
    static const uint16_t kCommandMeasureSingleShot = 0x219D;          ///< Perform a single shot measurement.
    static const uint16_t kCommandEnterSleepMode = 0x3650;             ///< Enter sleep mode (from idle).
    static const uint16_t kCommandPerformConditioning = 0x29BC;        ///< Condition the sensor (22 s).
    static const uint16_t kCommandPerformFactoryReset = 0x3632;        ///< Reset FRC / ASC history.
    static const uint16_t kCommandPerformSelfTest = 0x278C;            ///< Run the on-chip self-test.
    static const uint16_t kCommandEnableTestingMode = 0x3FBC;          ///< Pause the ASC algorithm.
    static const uint16_t kCommandDisableTestingMode = 0x3F3D;         ///< Resume the ASC algorithm.
    static const uint16_t kCommandForcedRecalibration = 0x362F;        ///< Forced recalibration (FRC).
    static const uint16_t kCommandGetProductId = 0x365B;               ///< Read product ID + serial number.
    static const uint8_t kCommandExitSleepMode = 0x00; ///< Single-byte wake-up payload (not acknowledged).
    static const uint8_t kCommandSoftReset = 0x06;     ///< Soft reset (sent to the general call address).

    ///////////////////////////////////////////////////////////////////////////
    // Product ID
    ///////////////////////////////////////////////////////////////////////////
    static const uint32_t kProductId = 0x0901018A; ///< Product ID reported by every STCC4.

    ///////////////////////////////////////////////////////////////////////////
    // CRC Parameters
    ///////////////////////////////////////////////////////////////////////////
    static const uint8_t kCrcPolynomial = 0x31;   ///< CRC-8 polynomial.
    static const uint8_t kCrcInitialValue = 0xFF; ///< CRC-8 initial value.

    ///////////////////////////////////////////////////////////////////////////
    // Conversion Constants
    ///////////////////////////////////////////////////////////////////////////
    static constexpr float kTicksFullScale = 65535.0f;   ///< Full-scale tick count for RH / temperature.
    static constexpr float kHumiditySlope = 125.0f;      ///< Humidity conversion slope.
    static constexpr float kHumidityOffset = -6.0f;      ///< Humidity conversion offset (%RH).
    static constexpr float kTemperatureSlopeC = 175.0f;  ///< Temperature conversion slope (°C).
    static constexpr float kTemperatureOffsetC = -45.0f; ///< Temperature conversion offset (°C).
    static constexpr float kTemperatureSlopeF = 315.0f;  ///< Temperature conversion slope (°F).
    static constexpr float kTemperatureOffsetF = -49.0f; ///< Temperature conversion offset (°F).
    static const uint32_t kPressureDivisor = 2;          ///< Pressure input ticks = Pascals / 2.
    static const uint32_t kPressureMinPa = 40000;        ///< Minimum accepted pressure input (Pa).
    static const uint32_t kPressureMaxPa = 110000;       ///< Maximum accepted pressure input (Pa).

    ///////////////////////////////////////////////////////////////////////////
    // Status / Result Decoding
    ///////////////////////////////////////////////////////////////////////////
    static const uint16_t kStatusTestingModeMask = 0x0040; ///< Testing mode flag in the status word.
    static const uint16_t kCommandFailed = 0xFFFF;          ///< FRC / factory reset failure value.
    static const uint16_t kFrcCorrectionOffset = 0x8000;    ///< FRC correction ppm = raw - 0x8000.

    ///////////////////////////////////////////////////////////////////////////
    // Timing (execution times, in milliseconds)
    ///////////////////////////////////////////////////////////////////////////
    static const uint32_t kStopMeasurementDelayMs = 1200; ///< Execution time for stop continuous measurement.
    static const uint32_t kReadMeasurementDelayMs = 1;    ///< Execution time for read measurement.
    static const uint32_t kSetCompensationDelayMs = 1;    ///< Execution time for the compensation commands.
    static const uint32_t kSingleShotDelayMs = 500;       ///< Execution time for a single shot measurement.
    static const uint32_t kEnterSleepDelayMs = 1;         ///< Execution time for enter sleep mode.
    static const uint32_t kExitSleepDelayMs = 5;          ///< Execution time for exit sleep mode.
    static const uint32_t kConditioningDelayMs = 22000;   ///< Execution time for conditioning.
    static const uint32_t kSoftResetDelayMs = 10;         ///< Execution time for soft reset.
    static const uint32_t kFactoryResetDelayMs = 90;      ///< Execution time for factory reset.
    static const uint32_t kSelfTestDelayMs = 360;         ///< Execution time for the self-test.
    static const uint32_t kForcedRecalDelayMs = 90;       ///< Execution time for forced recalibration.
    static const uint32_t kProductIdDelayMs = 1;          ///< Execution time for read product ID.
    static const uint32_t kDataNotReadyDelayMs = 150;     ///< Retry delay when measurement data is not ready.
    static const uint8_t kReadMeasurementAttempts = 4;    ///< Default total attempts for readMeasurement().

    ///////////////////////////////////////////////////////////////////////////
    // Buffer Sizing
    ///////////////////////////////////////////////////////////////////////////
    static const uint8_t kBytesPerWord = 3; ///< Two data bytes plus one CRC byte.
    static const uint8_t kMaxWords = 6;     ///< Largest response (product ID + serial) is six words.
    static const uint8_t kMaxArgWords = 2;  ///< Largest argument list (RHT compensation) is two words.
};
