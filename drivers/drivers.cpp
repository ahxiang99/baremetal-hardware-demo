#include "drivers.hpp"

static DriversList g_drivers; // one global, file-scope, controlled

static SensorsList g_sensors(I2C_Ref::from(g_drivers.i2c1));

DriversList &getDrivers()
{
    return g_drivers; // single access point
}

SensorsList &getSensors()
{
    return g_sensors;
}
