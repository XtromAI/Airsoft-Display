#pragma once
#include "sh1107_driver.h"

// Runs a demonstration of SH1107 display features.
// If init_display is true, the demo will initialize the display (calls display.begin()).
// Returns true if successful, false if initialization failed.
bool sh1107_demo(SH1107_Display& display, uint32_t delay_ms = 2000);
