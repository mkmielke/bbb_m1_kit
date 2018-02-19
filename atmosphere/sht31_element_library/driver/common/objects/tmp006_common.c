/**
 *  ----------------------------------------------------------------------------
 *  Copyright (c) 2014, Anaren Microwave, Inc.
 *
 *  For more information on licensing, please see Anaren Microwave, Inc's
 *  end user software licensing agreement: EULA.txt.
 *
 *  ----------------------------------------------------------------------------
 *
 *  tmp006.c - driver interface for the Texas Instruments TMP006 Infrared
 *  Thermopile Sensor.  The TMP006 allows for up to 8 devices on a single i2c
 *  bus, therefore each function accepts a bus identifier in the range 0 - 7 to
 *  select which device is currently being accessed.  This ID is added to the
 *  device's base address of 0x40 to determine the final i2c address.
 *
 *  @version    1.0.01
 *  @date       26 Sep 2014
 *  @author     Anaren, air@anaren.com
 *
 *  assumptions
 *  ===========
 *  - The i2c driver provides the proper signaling sequences for read & write
 *    operations.
 *  - The i2c driver meets the timing requirements specified in the TMP006
 *    datasheet.
 *
 *  file dependency
 *  ===============
 *  i2c.h : defines the i2c read & write interfaces.
 *	math.h : floating point calculations.
 *	fp_math.h : floating point calculations using fixed point math.  Use when math.h not implemented.
 *
 *  revision history
 *  ================
 *  ver 1.0.00 : 18 Jul 2014
 *  - initial release
 *  ver 1.0.01 : 24 Sep 2014
 *  - added support for doing floating point calculations using fixed point math
 */
#include "tmp006.h"

#ifdef AIR_FLOATING_POINT_NOT_AVAILABLE
#include "../fp_math/fp_math.h"
#else
#include <math.h>
#define fp_pow powf
#define fp_sqrt sqrtf
#endif

// -----------------------------------------------------------------------------
/**
 *  Global data
 */

// -----------------------------------------------------------------------------
/**
 *  Private interface
 */

/**
Calculate temperature of an object based on tdie and vobj
@param tDie temperature of the die in Kelvin
@param vObj object voltage converted first by multiplying 1.5625e-7
@return temperature of an object in Celsius
@see TMP006 datasheet and application note
@note from TI TMP006 BoosterPack sample code
*/
static float TMP006_CalculateTemperature(const float *tDie, const float *vObj)
{
  const float S0 = 6.0E-14;
  const float a1 = 1.75E-3;
  const float a2 = -1.678E-5;
  const float b0 = -2.94E-5;
  const float b1 = -5.7E-7;
  const float b2 = 4.63E-9;
  const float c2 = 13.4;
  const float Tref = 298.15;

#ifdef AIR_FLOATING_POINT_NOT_AVAILABLE
  float S = S0*((float)1.0 + a1*(*tDie - Tref) + a2*fp_pow((*tDie - Tref),2));
  float Vos = b0 + b1*(*tDie - Tref) + b2*fp_pow((*tDie - Tref),2);
  float fObj = (*vObj - Vos) + c2*fp_pow((*vObj - Vos),2);
  float Tobj = fp_sqrt(fp_sqrt(fp_pow(*tDie,4) + (fObj/S)));
#else
  float S = S0*((float)1.0 + a1*(*tDie - Tref) + a2*pow((*tDie - Tref),2));
  float Vos = b0 + b1*(*tDie - Tref) + b2*pow((*tDie - Tref),2);
  float fObj = (*vObj - Vos) + c2*pow((*vObj - Vos),2);
  float Tobj = sqrt(sqrt(pow(*tDie,4) + (fObj/S)));
#endif

  return (Tobj - (float)273.15);
}

// -----------------------------------------------------------------------------
/**
 *  Public interface
 */

void TMP006_WriteReg(uint8_t id, uint8_t addr, uint16_t data)
{
  uint8_t writeBytes[3];

  writeBytes[0] = addr;
  writeBytes[1] = data >> 8;
  writeBytes[2] = data & 0xFF;
  AIR_I2C_Write(TMP006_SLAVE_BASE_ADDR + id, writeBytes, 3);
}

uint16_t TMP006_ReadReg(uint8_t id, uint8_t addr)
{
  uint8_t writeBytes[1] = {0};
  uint8_t readBytes[2] = {0};
  uint16_t readData = 0;

  writeBytes[0] = addr;
  AIR_I2C_ComboRead(TMP006_SLAVE_BASE_ADDR + id, writeBytes, 1, readBytes, 2);
  readData = (unsigned int)readBytes[0] << 8;
  readData |= readBytes[1];
  return readData;
}

void TMP006_SoftwareReset(uint8_t id)
{
  TMP006_WriteReg(id, TMP006_CONFIG_REG_ADDR, TMP006_CONFIG_REG_RST);
}

void TMP006_SetOperatingMode(uint8_t id, enum eTMP006Mode mode)
{
  uint16_t data = TMP006_ReadReg(id, TMP006_CONFIG_REG_ADDR);

  data &= ~TMP006_CONFIG_REG_MOD;
  data |= (uint16_t)mode;
  TMP006_WriteReg(id, TMP006_CONFIG_REG_ADDR, data);
}

enum eTMP006Mode TMP006_GetOperatingMode(uint8_t id)
{
  return (enum eTMP006Mode)(TMP006_ReadReg(id, TMP006_CONFIG_REG_ADDR) & TMP006_CONFIG_REG_MOD);
}

void TMP006_SetConversionRate(uint8_t id, enum eTMP006Rate rate)
{
  uint16_t data = TMP006_ReadReg(id, TMP006_CONFIG_REG_ADDR);

  data &= ~TMP006_CONFIG_REG_CR;
  data |= (uint16_t)rate;
  TMP006_WriteReg(id, TMP006_CONFIG_REG_ADDR, data);
}

enum eTMP006Rate TMP006_GetConversionRate(uint8_t id)
{
  return (enum eTMP006Rate)(TMP006_ReadReg(id, TMP006_CONFIG_REG_ADDR) & TMP006_CONFIG_REG_CR);
}

void TMP006_SetDataReadyEnable(uint8_t id, bool en)
{
  uint16_t data = TMP006_ReadReg(id, TMP006_CONFIG_REG_ADDR);

  data &= ~TMP006_CONFIG_REG_EN;
  if (en) data |= TMP006_CONFIG_REG_EN;
  TMP006_WriteReg(id, TMP006_CONFIG_REG_ADDR, data);
}

bool TMP006_GetDataReadyEnable(uint8_t id)
{
  return (TMP006_ReadReg(id, TMP006_CONFIG_REG_ADDR) & TMP006_CONFIG_REG_EN) ? true : false;
}

void TMP006_ClearDataReadyStatus(uint8_t id)
{
  uint16_t data = TMP006_ReadReg(id, TMP006_CONFIG_REG_ADDR);

  data &= ~TMP006_CONFIG_REG_DRDY;
  TMP006_WriteReg(id, TMP006_CONFIG_REG_ADDR, data);
}

bool TMP006_GetDataReadyStatus(uint8_t id)
{
  return (TMP006_ReadReg(id, TMP006_CONFIG_REG_ADDR) & TMP006_CONFIG_REG_DRDY) ? true : false;
}

float TMP006_GetAmbientTemperature(uint8_t id)
{
  int16_t tDieRaw = (int16_t)TMP006_ReadReg(id, TMP006_TAMBIENT_REG_ADDR);

  return (((float)(tDieRaw >> 2)) * (float)0.03125);
}

float TMP006_GetObjectTemperature(uint8_t id)
{
  float tDie = TMP006_GetAmbientTemperature(id) + (float)273.15;
  int16_t vObjRaw = (int16_t)TMP006_ReadReg(id, TMP006_VOBJECT_REG_ADDR);
  float vObj = ((float)(vObjRaw)) * (float)156.25E-9;

  return TMP006_CalculateTemperature(&tDie, &vObj);
}

float TMP006_GetObjectTemperatureWithTransientCorrection(uint8_t id, float *tDie)
{
  int16_t vObjRaw = (int16_t)TMP006_ReadReg(id, TMP006_VOBJECT_REG_ADDR);
  float tSlope;
  float vObjCorr;

  tDie[0] = tDie[1];
  tDie[1] = tDie[2];
  tDie[2] = tDie[3];
  tDie[3] = TMP006_GetAmbientTemperature(id) + (float)273.15;
  tSlope = (tDie[0] != 0.0) ? -((float)0.3*tDie[0])-((float)0.1*tDie[1])+((float)0.1*tDie[2])+((float)0.3*tDie[3]) : 0.0;
  vObjCorr = (((float)(vObjRaw)) * (float)156.25E-9) + (tSlope * (float)2.96E-4);

  return TMP006_CalculateTemperature(&tDie[3], &vObjCorr);
}

uint16_t TMP006_GetMfgId(uint8_t id)
{
  return TMP006_ReadReg(id, TMP006_MFG_ID_REG_ADDR);
}

uint16_t TMP006_GetDeviceId(uint8_t id)
{
  return TMP006_ReadReg(id, TMP006_DEVICE_ID_REG_ADDR);
}

