#include "drivers.hpp"

static Drivers g_drivers;  // one global, file-scope, controlled

Drivers&       getDrivers() {
    return g_drivers;  // single access point
}
