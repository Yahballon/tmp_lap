// SPDX-License-Identifier: MIT
#pragma once
#include "weather_model.hpp"
namespace standby::weather {
// All HTTPS I/O occurs in a bounded worker thread; never block the draw loop.
void init();
bool refresh();
Data snapshot();
} // namespace standby::weather
