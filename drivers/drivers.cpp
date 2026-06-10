#include "drivers.hpp"

static Drivers g_Driver;

Drivers&       getDriver() {
    return g_Driver;
}