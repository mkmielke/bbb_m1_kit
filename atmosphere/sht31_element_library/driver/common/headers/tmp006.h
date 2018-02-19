// Copyright (c) 2014, Anaren Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are those
// of the authors and should not be interpreted as representing official policies, 
// either expressed or implied, of the FreeBSD Project.

#ifndef TMP006_H
#define TMP006_H

#include "../i2c/i2c.h"
#include "tmp006_config.h"
#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
/**
 *  Defines, enumerations, and structure definitions
 */

#define TMP006_SLAVE_BASE_ADDR      0x40

#define TMP006_VOBJECT_REG_ADDR     0x00
#define TMP006_TAMBIENT_REG_ADDR    0x01
#define TMP006_CONFIG_REG_ADDR      0x02
#define TMP006_MFG_ID_REG_ADDR      0xFE
#define TMP006_DEVICE_ID_REG_ADDR   0xFF

#define TMP006_CONFIG_REG_RST       0x8000
#define TMP006_CONFIG_REG_MOD       0x7000
#define TMP006_CONFIG_REG_MOD3      0x4000
#define TMP006_CONFIG_REG_MOD2      0x2000
#define TMP006_CONFIG_REG_MOD1      0x1000
#define TMP006_CONFIG_REG_CR        0x0E00
#define TMP006_CONFIG_REG_CR3       0x0800
#define TMP006_CONFIG_REG_CR2       0x0400
#define TMP006_CONFIG_REG_CR1       0x0200
#define TMP006_CONFIG_REG_EN        0x0100
#define TMP006_CONFIG_REG_DRDY      0x0080
// 
/**
 *  eTMP006Mode - type indicating the operating mode of the TMP006 device.  The
 *  TMP006 offers two modes, Power-Down and Continuous Conversion.  When
 *  ultra-low power consumption is important, the application should place the
 *  device in Power-Down when temperature measurements are not required.  While
 *  in Continuous Conversion mode, the device automatically enters a low-power
 *  state between samples.  However, this low-power state draws significantly
 *  more current than when in Power-Down (240uA compared to 0.5uA typ).
 */
enum eTMP006Mode
{
  TMP006_PowerDown             = 0x0000,
  TMP006_ContinuousConversion  = 0x7000
};

/**
 *  eTMP006Rate - type indicating the number of conversions per second performed
 *  by the TMP006.  Slower conversion rates result in more accurate measuremnts
 *  compared to the higher rates.  The default is one conversion every second.
 *  See the TMP006 datasheet for more information regarding the effect
 *  conversion rate has on accuracy of the result.
 */
enum eTMP006Rate
{
  FourConvPerSecond     = 0x0000,
  TwoConvPerSecond      = 0x0200,
  OneConvPerSecond      = 0x0400,
  HalfConvPerSecond     = 0x0600,
  QuarterConvPerSecond  = 0x0800
};


/** 
Write a 16-bit value to a device register.  All of the TMP006 registers are
read-only except for the Configuration Register.  This function does not do any
form of error checking, so trying to write to one of the read-only registers may
result in undesireable behavior.
@param id device ID (0 to 7) on i2c bus
@param addr device register address
@param data data to be written to the specified register address
*/
void TMP006_WriteReg(uint8_t id, uint8_t addr, uint16_t data);

/** 
Read a 16-bit value from a device register.
@param id device ID (0 to 7) on i2c bus
@param addr device register address
@return data read from the specified register address
*/
uint16_t TMP006_ReadReg(uint8_t id, uint8_t addr);

/**
Issue a software reset to the sensor.
@param id device ID (0 to 7) on i2c bus
@note This is a self-clearing operation.  There is no need for software to clear
the reset condition.
*/
void TMP006_SoftwareReset(uint8_t id);

/**
Select the device operating mode.  Refer to eTMP006Mode definition for details
regarding the allowed states.
@param id device ID (0 to 7) on i2c bus
@param mode specifies the device mode of operation
*/
void TMP006_SetOperatingMode(uint8_t id, enum eTMP006Mode mode);

/**
Read the currently selected operating mode.  Refer to eTMP006Mode definition for
details regarding the available states.
@param id device ID (0 to 7) on i2c bus
@return device mode of operation
*/
enum eTMP006Mode TMP006_GetOperatingMode(uint8_t id);

/**
Select the device conversion rate.  Refer to eTMP006Rate definition for details
regarding the allowed rates.
@param id device ID (0 to 7) on i2c bus
@param rate specifies the conversion rate
*/
void TMP006_SetConversionRate(uint8_t id, enum eTMP006Rate rate);

/**
Read the currently selected conversion rate.  Refer to eTMP006Rate definition
for details regarding the available rates.
@param id device ID (0 to 7) on i2c bus
@return device conversion rate
*/
enum eTMP006Rate TMP006_GetConversionRate(uint8_t id);

/**
Enable/disable the device DRDY output pin.
@param id device ID (0 to 7) on i2c bus
@param en true enables the pin output, false disables the output
*/
void TMP006_SetDataReadyEnable(uint8_t id, bool en);

/**
Read the state of the DRDY enable bit in the Configuration register.
@param id device ID (0 to 7) on i2c bus
@return true when the DRDY is enabled, otherwise false
*/
bool TMP006_GetDataReadyEnable(uint8_t id);

/**
Clear the DRDY ready status bit in the Configuration register.
@param id device ID (0 to 7) on i2c bus
*/
void TMP006_ClearDataReadyStatus(uint8_t id);

/**
Read the state of the DRDY status bit in the Configuration register.  The DRDY
status bit is automatically cleared after reading either the device Temperature
register or Sensor Voltage register.  The TMP006GetAmbientTemperature() and
TMP006GetObjectTemperature() functions access these registers, so calling either
will clear the DRDY status bit.  The DRDY status bit can also be cleared by
writing to the Configuration register or calling TMP006ClearDataReadyStatus().
@param id device ID (0 to 7) on i2c bus
@return true when conversion results are ready to read, otherwise false
*/
bool TMP006_GetDataReadyStatus(uint8_t id);

/**
Read the ambient (die) temperature.  When set to Continuous Conversion mode, the
device periodically performs temperature conversions at a predefined rate.  This
function calculates the temperature using only the most recent conversion value.
@param id device ID (0 to 7) on i2c bus
@return temperature of the TMP006 die in Celsius
*/
float TMP006_GetAmbientTemperature(uint8_t id);

/**
Read the temperature of an object.  When set to Continuous Conversion mode, the
device periodically performs temperature conversions at a predefined rate.  This
function calulates the object temperature using only the most recent conversion
values for die temperature and sensor voltage.
@param id device ID (0 to 7) on i2c bus
@return temperature of an object in Celsius
*/
float TMP006_GetObjectTemperature(uint8_t id);

/**
Read the temperature of an object.  When set to Continuous Conversion mode, the
device periodically performs temperature conversions at a predefined rate.  This
function calulates the object temperature using the most recent conversion value
for sensor voltage as well as the four most recent conversion values for die
temperature. This function assumes the size of tDie buffer is 4 float values and
that the application does not modify any of these values.  The application is
responsible only for providing the memory location.  The reason for this
approach, as opposed to defining the buffer in the driver itself, is so the
application can control which sensors on the bus use the transient correction
technique.  If not all sensors on the i2c bus use this method, then memory
utilization is reduced.
@param id device ID (0 to 7) on i2c bus
@param tDie pointer to array of 4 tDie values
@return temperature of an object in Celsius
*/
float TMP006_GetObjectTemperatureWithTransientCorrection(uint8_t id, float *tDie);

/**
Read the Manufacturer ID register.
@param id device ID (0 to 7) on i2c bus
@return contents of the Manufacturer ID register.  Value always = 0x5449.
*/
uint16_t TMP006_GetMfgId(uint8_t id);

/**
Read the Device ID register.
@param id device ID (0 to 7) on i2c bus
@return contents of the Device ID register.  Value always = 0x0067.
*/
uint16_t TMP006_GetDeviceId(uint8_t id);


#endif  /* TMP006_H */
