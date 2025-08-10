#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <string>

// Reads the onboard temperature sensor and returns the temperature as a formatted string (e.g., "24.5 C")
std::string get_formatted_temperature();

#endif // TEMPERATURE_H
