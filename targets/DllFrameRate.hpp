#pragma once

#include <cstdint>


namespace DllFrameRate
{

extern double desiredFps;

extern double actualFps;

void enable();

void limitFPS();

}

extern "C" __attribute__((visibility("default"))) void setDesiredFPS(double desiredFps_);
