#pragma once

#include <windows.h>
#include <mmsystem.h>  
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>
#include "DllTrialManager.hpp"

inline long long getNanoSec() {
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(value, min_val, max_val) MAX(MIN((value), (max_val)), (min_val))

namespace DllFrameRate
{

extern double desiredFps;

extern double actualFps;

extern long long frameStartTime;
extern long long nextFrameTime;

void enable();

void limitFPSBeforePresent();

void limitFPSAfterPresent();

}

