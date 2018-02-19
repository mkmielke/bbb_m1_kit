    float value;
    
    /**
    Read the ambient (die) temperature.  When set to Continuous Conversion mode, the
    device periodically performs temperature conversions at a predefined rate.  This
    function calulates the temperature using only the most recent conversion value.
    @param id device ID (0 to 7) on i2c bus
    @return temperature of the TMP006 die in Celsius
    */
    value = TMP006_GetAmbientTemperature(0);
    
    return value; 
