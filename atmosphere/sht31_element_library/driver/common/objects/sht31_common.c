//TODO: change this header
//TODO: give credit to "Exploring BBB" guy
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
 *  ver 1.0.00 : 02-19-18
 *  - initial release
 */
#include "sht31.h"


/******************************************************************************/
/*                             Globals (Private)                              */
/******************************************************************************/
int i2c_fd;     // file descriptor for i2c bus
int init_error; // store exit status of the init function


/******************************************************************************/
/*                       Function Prototypes (Private)                        */
/******************************************************************************/
unsigned char crc_8( char* data, unsigned int len, unsigned char poly, unsigned char init_val );



/******************************************************************************/
/*                       Function Definitions (Private)                       */
/******************************************************************************/
/*******************************************************************************
 * Function:    crc_8
 * Author:      Matt Mielke
 * Description:   This is a simple implementaion of an 8-bit wide CRC checksum.
 *              It's correctness was verified using the following website:
 *              www.sunshine2k.de/coding/javascript/crc/crc_js.html.
 * Parameters:
 *   data     - array of input data. 
 *   len      - length of data array
 *   poly     - the generator polynomial
 *   init_val - the initial crc value
 * Date:        03/01/2018
*******************************************************************************/
unsigned char crc_8( char* data, unsigned int len, unsigned char poly, unsigned char init_val )
{
  unsigned char rem;
  int i, j;

  rem = init_val;
  for ( i = 0 ; i < len ; i++ )
  {
    rem ^= data[i];
    for ( j = 0 ; j < 8 ; j++ )
    {
      if ( rem & 0x80 )
      {
        rem <<= 1;
        rem ^= poly;
      }
      else
      {
        rem <<= 1;
      }
    }
  }

  return rem;
}


/******************************************************************************/
/*                       Function Definitions (Public)                        */
/******************************************************************************/
/*******************************************************************************
 * Function:    sht31_get_humidity
 * Author:      Matt Mielke
 * Description:   Get a single shot measuremnt from the I2C device at 
 *              /dev/i2c-1. Issuing the signle shot measurement command will 
 *              cause the device to return temperature and humidity measurements
 *              along with checksums for each. This function will save the 
 *              humidity checksum value and recalulate it to ensure the humidity
 *              bytes were not currputed. The humidity value is then calculated
 *              and returned. 
 * Parameters:  none
 * Date:        02/19/2018
*******************************************************************************/
// TODO: change error codes values that could not be interpreted as temperatures
float sht31_get_humidity( void )
{
  // TODO: Abstract the I2C write and I2C read functionality
  char w_buff[2];          // write buffer
  char r_buff[6];          // read buffer
  unsigned char hum_crc;   // checksum of humidity bytes
  float humidity;          // humidity value

  // if the init function exited in error, exit with the same error
  if ( init_error != 0 )
  {
    return init_error;
  }

  // send the command 0x2C06: single shot measurement with clock stretching and
  // high repeatability
  w_buff[0] = 0x2C;
  w_buff[1] = 0x06;
  if ( write( i2c_fd, w_buff, 2 ) != 2 )
  {
    return -3;
  }

  // read the resulting measurement results
  // format: [ Temp. MSB | Temp. LSB | CRC | Hum. MSB | Hum. LSB | CRC ]
  if ( read( i2c_fd, r_buff, 6 ) != 6 )
  {
    return -4;
  }

  // save the checksum of the temperature bytes
  hum_crc = r_buff[5];

  // recalculate the temperature check sum and compare with the received value
  // TODO: don't just return. try measurements until checksum is valid.
  r_buff[5] = '\0';
  if ( crc_8( r_buff + 3, 2, 0x31, 0xFF ) != hum_crc )
  {
    //TODO: remove this
    //printf( "Buffer: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n", r_buff[0], r_buff[1], r_buff[2], r_buff[3], r_buff[4], hum_crc ); 
    return -5;
  }

  // calculate the temperature according to section 4.13 of the datasheet
  humidity = ( r_buff[3] << 8 ) | r_buff[4];
  humidity = 100 * ( humidity / 65535.0 );

  return humidity;
}


/*******************************************************************************
 * Function:    sht31_get_temperature
 * Author:      Matt Mielke
 * Description:   Get a single shot measuremnt from the I2C device at 
 *              /dev/i2c-1. Issuing the signle shot measurement command will 
 *              cause the device to return temperature and humidity measurements
 *              along with checksums for each. This function will save the 
 *              temperature checksum value and recalulate it to ensure the 
 *              temperature bytes were not currputed. The temperature value is
 *              then calculated and returned. 
 * Parameters:  none
 * Date:        02/19/2018
*******************************************************************************/
// TODO: change error codes values that could not be interpreted as temperatures
float sht31_get_temperature( void )
{
  // TODO: Abstract the I2C write and I2C read functionality
  char w_buff[2];          // write buffer
  char r_buff[6];          // read buffer
  unsigned char temp_crc;  // checksum of temperature bytes
  float temperature;       // temperature value

  // if the init function exited in error, exit with the same error
  if ( init_error != 0 )
  {
    return init_error;
  }

  // send the command 0x2C06: single shot measurement with clock stretching and
  // high repeatability
  w_buff[0] = 0x2C;
  w_buff[1] = 0x06;
  if ( write( i2c_fd, w_buff, 2 ) != 2 )
  {
    return -3;
  }

  // read the resulting measurement results
  // format: [ Temp. MSB | Temp. LSB | CRC | Hum. MSB | Hum. LSB | CRC ]
  if ( read( i2c_fd, r_buff, 6 ) != 6 )
  {
    return -4;
  }

  // save the checksum of the temperature bytes
  temp_crc = r_buff[2];

  // recalculate the temperature check sum and compare with the received value
  // TODO: don't just return. try measurements until checksum is valid.
  r_buff[2] = '\0';
  if ( crc_8( r_buff, 2, 0x31, 0xFF ) != temp_crc )
  {
    // TODO: remove this
    //printf( "Buffer: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n", r_buff[0], r_buff[1], temp_crc,  r_buff[3], r_buff[4], r_buff[5] ); 
    return -5;
  }

  // calculate the temperature according to section 4.13 of the datasheet
  temperature = ( r_buff[0] << 8 ) | r_buff[1];
  temperature = -45 + 175 * ( temperature / 65535.0 );
  
  return temperature;
}


/*******************************************************************************
 * Function:    sht31_init
 * Author:      Matt Mielke
 * Description:   Open the device /dev/i2c-1, which maps to I2C1, in R/W mode 
 *              and initialize it to communicate with the slave device at 
 *              address 0x44. This is the SHT31's slave address. The global 
 *              variable 'init_error' is used to store the exit status of the 
 *              function for debugging purposes. 
 * Parameters:  none
 * Date:        02/19/2018
*******************************************************************************/
void sht31_init( void )
{
  init_error = 0;

  // open the file /dev/i2c-1 for read/write access
  if ( ( i2c_fd = open( "/dev/i2c-1", O_RDWR ) ) < 0 )
  {
    init_error = -1;
    return;
  }

  // set the slave address to that of the sht31 sensor
  if ( ioctl( i2c_fd, I2C_SLAVE, 0x44 ) < 0 )
  {
    init_error = -2;
    return;
  }
}

