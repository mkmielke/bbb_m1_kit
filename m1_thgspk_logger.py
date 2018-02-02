#!/usr/bin/env python

# 01-08-18: Restructuring the main loop to reduce update jitter
# 01-26-18: After the error handling difficulties of the state machine 
#           implementation, I've come back here and done a top-down redesign.

from Adafruit_I2C import Adafruit_I2C
import serial
import time

UPDATE_PERIOD = 120  # send data every 120 seconds
EARLY_DIAL_TIME = 10 # attempt socket dial this many seconds before UPDATE_PERIOD expires
CMD_TIMEOUT = 3      # timeout if command does not rx response after x seconds
LONG_TIMEOUT = 5 * 60 # a timeout to catch unexpected responses that would cause the code to hang
MAX_SD_COUNT = 50   # maximum number of time the socket dial function can fail without losing registration

DEBUG = True

def debug_print( msg ):
    global DEBUG

    if DEBUG == True:
        msg = str( msg )
        print msg


def sht31_get_temp_hum( device ):
    # Start single shot measurement (high repeatability, stretching)
    device.write8( 0x2C, 0x06 ) # the command is 0x2C06

    # read the temp and humidity data
    # [ temp msb | temp lsb | temp cksum | hum msb | hum lsb | hum cksum ]
    ss_readout = device.readList( 0x00, 6 ) # the 0x00 gets ignored

    temp = ( ss_readout[0] << 8 ) | ss_readout[1]
    temp = -45 + 175 * ( temp / 65535.0 ) # datasheet 4.13
    temp = temp * 9 / 5 + 32

    hum = ( ss_readout[3] << 8 ) | ss_readout[4]
    hum = 100 * hum / 65535.0 # datasheet 4.13

    return temp, hum


def serial_read_line( module, timeout = None ):
    # set the timeout of the module and return if invalid parameter     
    try:
        module.timeout = timeout
    except ValueError:
        return ''

    return module.readline()


# for debugging purposes
def delay_and_read_lines( module, delay ):
    start = time.time()

    resp = serial_read_line( module, max( 0, delay - ( time.time() - start ) ) )
    while resp != '':
        resp = serial_read_line( module, max( 0, delay - ( time.time() - start ) ) )
        print repr( resp )


def svzm20_disable_echo( module ):
    global CMD_TIMEOUT

    # Disregard any unread unsolicited responses or command responses
    module.reset_input_buffer()

    # Send command "ATE0"
    module.write( "ATE0\r\n" )

    # Wait for "OK", "ERROR", or timeout ('')
    resp = serial_read_line( module, CMD_TIMEOUT )
    while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
        resp = serial_read_line( module, CMD_TIMEOUT )

    if resp == "OK\r\n":
        return 0
    else:
        return 1


def svzm20_set_apn( module, context ):
    global CMD_TIMEOUT

    # Disregard any unread unsolicited responses or command responses
    module.reset_input_buffer()

    # Send command "AT+CDGCONT?" to get the current PDP context
    module.write( "AT+CGDCONT?\r\n" )

    # Wait for "OK", "ERROR", or timeout ('')
    # Check each received line for "+CGDCONT: " + context
    context_set_flag = 1
    resp = serial_read_line( module, CMD_TIMEOUT )
    while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
        context_set_flag = resp.find( "+CGDCONT: " + context ) & context_set_flag
        resp = serial_read_line( module, CMD_TIMEOUT )

    # if the command was not successfully executed, return error
    if resp != "OK\r\n":
        return 1

    # if a string starting with "+CGDCONT: " + context was not found
    if context_set_flag != 0:
        # send the command to set the context
        module.write( "AT+CGDCONT=" + context + "\r\n" )

        # Wait for "OK", "ERROR", or timeout ('')
        resp = serial_read_line( module, CMD_TIMEOUT )
        while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
            resp = serial_read_line( module, CMD_TIMEOUT )

        if resp != "OK\r\n":
            return 1

    return 0


def svzm20_enable_cellular( module ):
    global CMD_TIMEOUT

    # Disregard any unread unsolicited responses or command responses
    module.reset_input_buffer()

    # Send command "AT+CFUN?" to see if cellular functionality already enabled
    module.write( "AT+CFUN?\r\n" )

    # Wait for "OK", "ERROR", or timeout ('')
    # check each received line for "+CFUN: 1"
    enabled_flag = 1
    resp = serial_read_line( module, CMD_TIMEOUT )
    while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
        enabled_flag = resp.find( "+CFUN: 1" ) & enabled_flag
        resp = serial_read_line( module, CMD_TIMEOUT )

    # if the command was not successfully executed, return error
    if resp != "OK\r\n":
        return 1

    # if a string starting with "+CFUN: 1" was not received
    if enabled_flag != 0:
        # send the command to enable cellular functionality
        module.write( "AT+CFUN=1\r\n" )

        # Wait for "OK", "ERROR", or timeout ('')
        resp = serial_read_line( module, CMD_TIMEOUT )
        while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
            resp = serial_read_line( module, CMD_TIMEOUT )

        if resp != "OK\r\n":
            return 1

    return 0


def svzm20_enable_auto_attach( module ):
    global CMD_TIMEOUT

    # Disregard any unread unsolicited responses or command responses
    module.reset_input_buffer()

    # Send command "AT^AUTOATT?" to see if auto attach already enabled
    module.write( "AT^AUTOATT?\r\n" )

    # Wait for "OK", "ERROR", or timeout ('')
    # check each received line for "^AUTOATT: 1"
    enabled_flag = 1
    resp = serial_read_line( module, CMD_TIMEOUT )
    while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
        enabled_flag = resp.find( "^AUTOATT: 1" ) & enabled_flag
        resp = serial_read_line( module, CMD_TIMEOUT )

    # if the command was not successfully executed, return error
    if resp != "OK\r\n":
        return 1

    # if a string starting with "^AUTOATT: 1" was not received
    if enabled_flag != 0:
        # send the command to enable auto attach
        module.write( "AT^AUTOATT=1\r\n" )

        # Wait for "OK", "ERROR", or timeout ('')
        resp = serial_read_line( module, CMD_TIMEOUT )
        while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
            resp = serial_read_line( module, CMD_TIMEOUT )

        if resp != "OK\r\n":
            return 1

    return 0


def svzm20_config_socket( module, socket, config ):
    global CMD_TIMEOUT

    # varaible 'socket' needs to be a string so it can be concatenated
    socket = str( socket )

    # Disregard any unread unsolicited responses or command responses
    module.reset_input_buffer()

    # Send command "AT+SQNSCFG?" to see if socket already configured
    module.write( "AT+SQNSCFG?\r\n" )

    # Wait for "OK", "ERROR", or timeout ('')
    # check each received line for "+SQNSCFG: " + socket + "," + config
    configured_flag = 1
    resp = serial_read_line( module, CMD_TIMEOUT )
    while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
        configured_flag = resp.find( "+SQNSCFG: " + socket + "," + config ) & configured_flag
        resp = serial_read_line( module, CMD_TIMEOUT )

    # if the command was not successfully executed, return error
    if resp != "OK\r\n":
        return 1

    # if a string starting with "+SQNSCFG: "... was not received
    if configured_flag != 0:
        # send the command to configure the socket
        module.write( "AT+SQNSCFG=" + socket + "," + config + "\r\n" )

        # Wait for "OK", "ERROR", or timeout ('')
        resp = serial_read_line( module, CMD_TIMEOUT )
        while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
            resp = serial_read_line( module, CMD_TIMEOUT )

        if resp != "OK\r\n":
            return 1

    return 0


def svzm20_config( module ):
    if svzm20_disable_echo( module ) != 0:
        return 1

    if svzm20_set_apn( svzm20, "3,\"IPV4V6\",\"NIMBLINK.GW12.VZWENTP\"" ) != 0:
        return 1

    if svzm20_enable_cellular( module ) != 0:
        return 1

    if svzm20_enable_auto_attach( module ) != 0:
        return 1

    if svzm20_config_socket( module, 3, "3,300,90,600,50" ) != 0:
        return 1

    return 0


def svzm20_is_cellular_enabled( module ):
    global CMD_TIMEOUT

    # Disregard any unread unsolicited responses or command responses
    module.reset_input_buffer()

    # Send command "AT+CFUN?" to see if cellular functionality already enabled
    module.write( "AT+CFUN?\r\n" )

    # Wait for "OK", "ERROR", or timeout ('')
    # check each received line for "+CFUN: 1"
    enabled_flag = 1
    resp = serial_read_line( module, CMD_TIMEOUT )
    while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
        enabled_flag = resp.find( "+CFUN: 1" ) & enabled_flag
        resp = serial_read_line( module, CMD_TIMEOUT )

    # if the command was not successfully executed, return error
    if resp != "OK\r\n":
        return False

    # if a string starting with "+CFUN: 1" was not received
    if enabled_flag != 0:
        return False

    return True


def svzm20_is_registered( module ):
    global CMD_TIMEOUT

    # Disregard any unread unsolicited responses or command responses
    module.reset_input_buffer()

    # Send command to check network registration
    module.write( "AT+CEREG?\r\n" )

    # Wait for "OK", "ERROR", or timeout ('')
    # check each received line for "+CEREG: 2,1,"
    registered_flag = 1
    resp = serial_read_line( module, CMD_TIMEOUT )
    while not any( resp == line for line in [ "OK\r\n", "ERROR\r\n", '' ] ):
        registered_flag = resp.find( "+CEREG: 2,1," ) & registered_flag
        resp = serial_read_line( module, CMD_TIMEOUT )
    
    # if the command was not successfully executed, return error
    if resp != "OK\r\n":
        return False

    # if a string starting with "CEREG: 2,1," was not recieved
    if registered_flag != 0:
        return False

    return True


def svzm20_wait_until_registered( module ):
    # if the module is not registered to the network, make sure cellular is enabled
    while svzm20_is_registered( svzm20 ) != True:
        # if cellular is not enabled, it will never register. exit with error
        if svzm20_is_cellular_enabled( svzm20 ) != True:
            return 1

        # if cellular is enabled, wait a bit and check again
        time.sleep( 1 )

    return 0


def svzm20_socket_dial( module, socket, address ):
    # the variable 'socket' needs to be a string so it can be concatenated
    socket = str( socket )

    # write command to open socket to address
    module.write( "AT+SQNSD=" + socket + ",0,80,\"" + address + "\"\r\n" )

    # get response with long timeout (should be "\r\nCONNECT\r\n")
    resp = serial_read_line( module, LONG_TIMEOUT )
    while not any( resp == line for line in [ "CONNECT\r\n", "ERROR\r\n", "NO CARRIER\r\n", "+SYSSTART\r\n", '' ] ):
        resp = serial_read_line( module, LONG_TIMEOUT )

    if resp != "CONNECT\r\n":
        return 1

    return 0

    
def svzm20_send_data( module, data1, data2 ):
    # Send the data to thingspeak via a GET request
    module.write( "GET /update?api_key=6KCU5BEO4P5YQ60G&field1={:.2f}&field2={:.2f}\n".format( data1, data2 ) )

    # get response until "NO CARRIER\r\n". 
    # the first line should be a number greater than 0
    resp = serial_read_line( module, LONG_TIMEOUT )
    first_resp = resp
    while not any( resp == line for line in [ "NO CARRIER\r\n", "ERROR\r\n", "+SYSSTART\r\n", '' ] ):
        resp = serial_read_line( module, LONG_TIMEOUT )

    try:
        if int( first_resp.strip() ) > 0:
            return 0
        else:
            return 1
    except:
        return 1


def svzm20_reset( module ):
    # send the reset command
    module.write( "AT^RESET\r\n" )

    # get responses until "+SYSSTART\r\n" received
    resp = serial_read_line( module, LONG_TIMEOUT )
    while not any( resp == line for line in [ "+SYSSTART\r\n", '' ] ):
        resp = serial_read_line( module, LONG_TIMEOUT )

    if resp != "+SYSSTART\r\n":

        # hardware reset
        return 1

    return 0


# Slave address of SHT31 is 0x44
# Grove connector J16 uses P9_17 and P9_18 (I2C1)
sht31 = Adafruit_I2C( 0x44, 1 )
ss_readout = [] # readout of single shot measurements

# Open the serial port on UART4 (M1 module)
svzm20 = serial.Serial( "/dev/ttyO4", 921600 )

next_update_time = time.time()

# Wait until the module is registered to the network. Reconfigure if necessary.
while svzm20_wait_until_registered( svzm20 ) != 0:
    debug_print( "Failed to register." )
    while svzm20_config( svzm20 ) != 0:
        debug_print( "Failed to configure module." )
        pass

while True:
    # shorten the delay before the socket dial by EARLY_DIAL_TIME in an attempt
    # to remove the socket dail time from the update period
    if next_update_time - time.time() > EARLY_DIAL_TIME:
        #time.sleep( max( 0, next_update_time - time.time() - EARLY_DIAL_TIME ) )
        delay_and_read_lines( svzm20, max( 0, next_update_time - time.time() - EARLY_DIAL_TIME ) )
    
    # Connect to thingspeak. If unsuccessful, wait until registered to network and
    # try again. Reconfigure if necessary.
    sd_count = 0
    debug_print( "Attempting socket dial: " + str( sd_count ) )
    while svzm20_socket_dial( svzm20, 3, "api.thingspeak.com" ) != 0:
        debug_print( "Socket dial failed." )

        # if socket dial failed, do nothing until registered
        while svzm20_wait_until_registered( svzm20 ) != 0:
            debug_print( "Failed to register." )
            
            # if the module reset itself, registration will "fail" (never 
            # happen) and the device needs to be reconfigured
            while svzm20_config( svzm20 ) != 0:
                debug_print( "Failed to configure module." )
                pass
            
            # since the device was reset, start the count over
            sd_count = -1
            
        # increment the number of socket dial attempts count
        sd_count = sd_count + 1

        # if too many socket dials occured without a reset, reset the module
        if sd_count > MAX_SD_COUNT:
            svzm20_reset( svzm20 )
            sd_count = 0

        debug_print( "Attempting socket dial: " + str( sd_count ) )

    debug_print( "Socket dial successful" )

    # delay until UPDATE_PERIOD expires
    # time.sleep( max( 0, next_update_time - time.time() ) )
    delay_and_read_lines( svzm20, max( 0, next_update_time - time.time() ) )

    # attempt to send data to thingspeak. If unsuccessful, do not change
    # next_update_time, forcing an immediate reconnection attempt
    temp, hum = sht31_get_temp_hum( sht31 )
    debug_print( "Attempting to send data." )
    if svzm20_send_data( svzm20, temp, hum ) == 0:
        debug_print( "Data successfully sent." )
        next_update_time = time.time() + UPDATE_PERIOD
    else:
        debug_print( "Data not sent." )
    
exit()

