/**
 * @file sfDevSTCC4.cpp
 * @brief Implementation file for the SparkFun STCC4 CO2 Sensor Driver.
 *
 * @details
 * This file implements the sfDevSTCC4 class methods for controlling and reading data from the
 * Sensirion STCC4 CO2 sensor. The driver provides a comms-agnostic interface using the SparkFun
 * Toolkit and implements Sensirion's command-based I2C protocol, including CRC-8 validation of
 * every data word sent and received.
 *
 * @author SparkFun Electronics
 * @date 2026
 * @copyright Copyright (c) 2026, SparkFun Electronics Inc. This project is released under the MIT License.
 *
 * SPDX-License-Identifier: MIT
 *
 * @see https://github.com/sparkfun/SparkFun_STCC4_Arduino_Library
 */

#include "sfDevSTCC4.h"

// ========================= Setup & Identity ===============================

sfTkError_t sfDevSTCC4::begin(sfTkIBus *theBus)
{
    // Adopt the supplied bus if one was provided; otherwise keep any bus set by a prior begin().
    if (theBus != nullptr)
        _theBus = theBus;

    // We need a bus to talk to.
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    // Confirm an STCC4 is actually present and responding correctly before continuing. The product
    // ID read also validates the CRC, so a successful match is strong evidence of a real STCC4.
    if (isConnected())
        return ksfTkErrOk;

    // The identity check requires the sensor to be idle, so it fails if the sensor was left in
    // sleep mode or left measuring. Both states survive a controller reset or re-upload, because
    // the sensor keeps power from the bus the whole time. Recover from each in turn.

    // Sleep mode: send the wake-up byte and check again. exitSleepMode() re-verifies the product ID
    // once the sensor is awake.
    if (exitSleepMode() == ksfTkErrOk)
        return ksfTkErrOk;

    // Continuous measurement still running: stop it (this blocks for the 1.2 s execution time) and
    // check one last time.
    if (stopContinuousMeasurement() != ksfTkErrOk)
        return ksfTkErrBusNoResponse;

    return isConnected() ? ksfTkErrOk : ksfTkErrBusNoResponse;
}

sfTkError_t sfDevSTCC4::reset(void)
{
    // The soft reset uses I2C-specific addressing (the general call address), so we need the I2C
    // view of the bus. The STCC4 is an I2C-only device, so the bus is always an sfTkII2C.
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    sfTkII2C *i2cBus = (sfTkII2C *)_theBus;

    // Remember the configured sensor address - begin() may have selected the alternate.
    uint8_t sensorAddress = i2cBus->address();

    // Send the single-byte reset command to the I2C general call address. The command is not
    // acknowledged by the sensor, so ignore the write result.
    i2cBus->setAddress(kGeneralCallAddress);

    uint8_t command = kCommandSoftReset;
    (void)i2cBus->writeData(&command, sizeof(command));

    // Restore the sensor's own address for all subsequent communication.
    i2cBus->setAddress(sensorAddress);

    // Give the sensor time to complete the reset before it is addressed again.
    sftk_delay_ms(kSoftResetDelayMs);
    return ksfTkErrOk;
}

bool sfDevSTCC4::isConnected(void)
{
    uint32_t productId = 0;
    uint64_t serialNumber = 0;

    if (getProductId(productId, serialNumber) != ksfTkErrOk)
        return false;

    return productId == kProductId;
}

// ========================= Measurement Control ============================

sfTkError_t sfDevSTCC4::startContinuousMeasurement(void)
{
    return sendCommand(kCommandStartContinuousMeasurement);
}

sfTkError_t sfDevSTCC4::stopContinuousMeasurement(void)
{
    sfTkError_t rc = sendCommand(kCommandStopContinuousMeasurement);
    if (rc != ksfTkErrOk)
        return rc;

    // The sensor does not respond on the bus until the stop command finishes executing.
    sftk_delay_ms(kStopMeasurementDelayMs);
    return ksfTkErrOk;
}

sfTkError_t sfDevSTCC4::measureSingleShot(void)
{
    sfTkError_t rc = sendCommand(kCommandMeasureSingleShot);
    if (rc != ksfTkErrOk)
        return rc;

    // Wait out the measurement execution time so the data is ready for readMeasurement().
    sftk_delay_ms(kSingleShotDelayMs);
    return ksfTkErrOk;
}

sfTkError_t sfDevSTCC4::readMeasurement(uint8_t maxAttempts)
{
    if (maxAttempts == 0)
        maxAttempts = 1;

    // Read four CRC-protected words: CO2, temperature, humidity, and status. The sensor NACKs the
    // transfer when no new data point is available yet, so retry on the datasheet-recommended
    // 150 ms interval.
    uint16_t words[4] = {0};
    sfTkError_t rc = ksfTkErrFail;

    for (uint8_t attempt = 0; attempt < maxAttempts; attempt++)
    {
        if (attempt > 0)
            sftk_delay_ms(kDataNotReadyDelayMs);

        rc = readWords(kCommandReadMeasurement, words, 4, kReadMeasurementDelayMs);
        if (rc == ksfTkErrOk)
            break;
    }

    if (rc != ksfTkErrOk)
        return rc;

    _co2Ticks = words[0];
    _temperatureTicks = words[1];
    _humidityTicks = words[2];
    _status = words[3];

    return ksfTkErrOk;
}

// ====================== Cached Measurement Values =========================

int16_t sfDevSTCC4::getCO2(void)
{
    // The CO2 word is a signed 16-bit value in ppm.
    return (int16_t)_co2Ticks;
}

float sfDevSTCC4::getHumidity(void)
{
    float humidity = kHumidityOffset + kHumiditySlope * (float)_humidityTicks * kTicksFullScaleInv;

    // The conversion can produce values slightly outside the physical range; clamp per datasheet.
    if (humidity < 0.0f)
        humidity = 0.0f;
    else if (humidity > 100.0f)
        humidity = 100.0f;

    return humidity;
}

float sfDevSTCC4::getTemperature(void)
{
    return kTemperatureOffsetC + kTemperatureSlopeC * (float)_temperatureTicks * kTicksFullScaleInv;
}

float sfDevSTCC4::getTemperatureF(void)
{
    return kTemperatureOffsetF + kTemperatureSlopeF * (float)_temperatureTicks * kTicksFullScaleInv;
}

uint16_t sfDevSTCC4::getStatus(void)
{
    return _status;
}

bool sfDevSTCC4::isTestingModeEnabled(void)
{
    return (_status & kStatusTestingModeMask) != 0;
}

// ===================== Raw Cached Measurement Ticks =======================

uint16_t sfDevSTCC4::getHumidityRaw(void)
{
    return _humidityTicks;
}

uint16_t sfDevSTCC4::getTemperatureRaw(void)
{
    return _temperatureTicks;
}

// ========================== Compensation ==================================

sfTkError_t sfDevSTCC4::setRHTCompensation(float temperature, float humidity)
{
    // Clamp to the convertible input ranges before applying the datasheet input formulas.
    if (temperature < kTemperatureOffsetC)
        temperature = kTemperatureOffsetC;
    else if (temperature > kTemperatureOffsetC + kTemperatureSlopeC)
        temperature = kTemperatureOffsetC + kTemperatureSlopeC;

    if (humidity < 0.0f)
        humidity = 0.0f;
    else if (humidity > 100.0f)
        humidity = 100.0f;

    // Input ticks = (T + 45) * 65535 / 175 and (RH + 6) * 65535 / 125, rounded to nearest. The
    // divisions are folded into the precomputed ticks-per-unit constants so this stays multiply-only.
    uint16_t args[2];
    args[0] = (uint16_t)((temperature - kTemperatureOffsetC) * kTempTicksPerDegreeC + 0.5f);
    args[1] = (uint16_t)((humidity - kHumidityOffset) * kHumidityTicksPerPercent + 0.5f);

    sfTkError_t rc = sendCommand(kCommandSetRHTCompensation, args, 2);
    if (rc != ksfTkErrOk)
        return rc;

    sftk_delay_ms(kSetCompensationDelayMs);
    return ksfTkErrOk;
}

sfTkError_t sfDevSTCC4::setPressureCompensation(uint32_t pascals)
{
    // The sensor clips inputs to this range; clamp here so the 16-bit conversion below is valid.
    if (pascals < kPressureMinPa)
        pascals = kPressureMinPa;
    else if (pascals > kPressureMaxPa)
        pascals = kPressureMaxPa;

    // Input ticks = Pascals / 2.
    uint16_t arg = (uint16_t)(pascals / kPressureDivisor);

    sfTkError_t rc = sendCommand(kCommandSetPressureCompensation, &arg, 1);
    if (rc != ksfTkErrOk)
        return rc;

    sftk_delay_ms(kSetCompensationDelayMs);
    return ksfTkErrOk;
}

// ========================= Power Management ===============================

sfTkError_t sfDevSTCC4::enterSleepMode(void)
{
    sfTkError_t rc = sendCommand(kCommandEnterSleepMode);
    if (rc != ksfTkErrOk)
        return rc;

    sftk_delay_ms(kEnterSleepDelayMs);
    return ksfTkErrOk;
}

sfTkError_t sfDevSTCC4::exitSleepMode(void)
{
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    // The wake-up is a single payload byte that the sensor intentionally does not acknowledge, so
    // the write is expected to report a NACK error - ignore it.
    uint8_t payload = kCommandExitSleepMode;
    (void)_theBus->writeData(&payload, sizeof(payload));

    sftk_delay_ms(kExitSleepDelayMs);

    // Confirm the sensor actually woke up by reading its product ID (per the datasheet).
    return isConnected() ? ksfTkErrOk : ksfTkErrBusNoResponse;
}

// ====================== Calibration & Maintenance =========================

sfTkError_t sfDevSTCC4::performConditioning(void)
{
    sfTkError_t rc = sendCommand(kCommandPerformConditioning);
    if (rc != ksfTkErrOk)
        return rc;

    // Conditioning runs a fixed 22 s operation profile; the sensor is busy for the duration.
    sftk_delay_ms(kConditioningDelayMs);
    return ksfTkErrOk;
}

sfTkError_t sfDevSTCC4::performForcedRecalibration(uint16_t targetCO2, int16_t &frcCorrection)
{
    frcCorrection = 0;

    uint16_t rawCorrection = 0;
    sfTkError_t rc = readWords(kCommandForcedRecalibration, &targetCO2, 1, &rawCorrection, 1, kForcedRecalDelayMs);
    if (rc != ksfTkErrOk)
        return rc;

    // The sensor reports 0xFFFF when the recalibration could not be performed.
    if (rawCorrection == kCommandFailed)
        return ksfTkErrFail;

    // The applied correction is offset-encoded: ppm = raw - 0x8000.
    frcCorrection = (int16_t)((int32_t)rawCorrection - (int32_t)kFrcCorrectionOffset);
    return ksfTkErrOk;
}

sfTkError_t sfDevSTCC4::performFactoryReset(void)
{
    uint16_t result = 0;
    sfTkError_t rc = readWords(kCommandPerformFactoryReset, &result, 1, kFactoryResetDelayMs);
    if (rc != ksfTkErrOk)
        return rc;

    // The sensor reports 0 on success, 0xFFFF on failure.
    return (result == 0) ? ksfTkErrOk : ksfTkErrFail;
}

sfTkError_t sfDevSTCC4::performSelfTest(uint16_t &result)
{
    return readWords(kCommandPerformSelfTest, &result, 1, kSelfTestDelayMs);
}

// =========================== Testing Mode =================================

sfTkError_t sfDevSTCC4::enableTestingMode(void)
{
    return sendCommand(kCommandEnableTestingMode);
}

sfTkError_t sfDevSTCC4::disableTestingMode(void)
{
    return sendCommand(kCommandDisableTestingMode);
}

// ============================= Identity ===================================

sfTkError_t sfDevSTCC4::getProductId(uint32_t &productId, uint64_t &serialNumber)
{
    // The response is six 16-bit words: two for the product ID (MSW first), four for the serial.
    uint16_t words[kMaxWords] = {0};
    sfTkError_t rc = readWords(kCommandGetProductId, words, kMaxWords, kProductIdDelayMs);
    if (rc != ksfTkErrOk)
        return rc;

    productId = ((uint32_t)words[0] << 16) | (uint32_t)words[1];
    serialNumber =
        ((uint64_t)words[2] << 48) | ((uint64_t)words[3] << 32) | ((uint64_t)words[4] << 16) | (uint64_t)words[5];

    return ksfTkErrOk;
}

// ============================ Protected Helpers ===========================

sfTkError_t sfDevSTCC4::sendCommand(uint16_t command)
{
    return sendCommand(command, nullptr, 0);
}

sfTkError_t sfDevSTCC4::sendCommand(uint16_t command, const uint16_t *args, uint8_t numArgs)
{
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    if (numArgs > kMaxArgWords || (numArgs > 0 && args == nullptr))
        return ksfTkErrInvalidParam;

    // The command (MSB first) is followed by each argument word as two data bytes plus a CRC-8.
    // Build the buffer explicitly so the wire order does not depend on the bus byte-order setting.
    uint8_t buffer[2 + kMaxArgWords * kBytesPerWord];
    buffer[0] = (uint8_t)(command >> 8);
    buffer[1] = (uint8_t)(command & 0xFF);

    size_t length = 2;
    for (uint8_t i = 0; i < numArgs; i++)
    {
        buffer[length] = (uint8_t)(args[i] >> 8);
        buffer[length + 1] = (uint8_t)(args[i] & 0xFF);
        buffer[length + 2] = computeCRC8(&buffer[length], 2);
        length += kBytesPerWord;
    }

    return _theBus->writeData(buffer, length);
}

sfTkError_t sfDevSTCC4::readWords(uint16_t command, uint16_t *words, uint8_t numWords, uint32_t readDelayMs)
{
    return readWords(command, nullptr, 0, words, numWords, readDelayMs);
}

sfTkError_t sfDevSTCC4::readWords(uint16_t command, const uint16_t *args, uint8_t numArgs, uint16_t *words,
                                  uint8_t numWords, uint32_t readDelayMs)
{
    if (_theBus == nullptr)
        return ksfTkErrBusNotInit;

    if (words == nullptr || numWords == 0 || numWords > kMaxWords)
        return ksfTkErrInvalidParam;

    if (numArgs > kMaxArgWords || (numArgs > 0 && args == nullptr))
        return ksfTkErrInvalidParam;

    // The command word (plus any CRC-protected argument words) is sent as the "register address",
    // after which the response is read back following the command's execution time.
    uint8_t commandBytes[2 + kMaxArgWords * kBytesPerWord];
    commandBytes[0] = (uint8_t)(command >> 8);
    commandBytes[1] = (uint8_t)(command & 0xFF);

    size_t commandLength = 2;
    for (uint8_t i = 0; i < numArgs; i++)
    {
        commandBytes[commandLength] = (uint8_t)(args[i] >> 8);
        commandBytes[commandLength + 1] = (uint8_t)(args[i] & 0xFF);
        commandBytes[commandLength + 2] = computeCRC8(&commandBytes[commandLength], 2);
        commandLength += kBytesPerWord;
    }

    uint8_t buffer[kMaxWords * kBytesPerWord] = {0};
    size_t numBytes = (size_t)numWords * kBytesPerWord;
    size_t readBytes = 0;

    sfTkError_t rc = _theBus->readRegister(commandBytes, commandLength, buffer, numBytes, readBytes, readDelayMs);
    if (rc != ksfTkErrOk)
        return rc;

    if (readBytes != numBytes)
        return ksfTkErrBusUnderRead;

    // Each word is two data bytes followed by a CRC-8 of those two bytes.
    for (uint8_t i = 0; i < numWords; i++)
    {
        const uint8_t *group = &buffer[i * kBytesPerWord];

        if (computeCRC8(group, 2) != group[2])
            return ksfTkErrFail;

        words[i] = ((uint16_t)group[0] << 8) | (uint16_t)group[1];
    }

    return ksfTkErrOk;
}

uint8_t sfDevSTCC4::computeCRC8(const uint8_t *data, size_t length)
{
    uint8_t crc = kCrcInitialValue;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ kCrcPolynomial);
            else
                crc = (uint8_t)(crc << 1);
        }
    }

    return crc;
}
