    float value;
    
    /**
    Read the temperature of an object.  When set to Continuous Conversion mode, the
    device periodically performs temperature conversions at a predefined rate.  This
    function calulates the object temperature using only the most recent conversion
    values for die temperature and sensor voltage.
    @param id device ID (0 to 7) on i2c bus
    @return temperature of an object in Celsius
    */
    value = TMP006_GetObjectTemperature(0);
    
    return value;