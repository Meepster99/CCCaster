#include "DllFrameRate.hpp"
#include "TimerManager.hpp"
#include "Constants.hpp"
#include "ProcessManager.hpp"
#include "DllAsmHacks.hpp"

#include <d3dx9.h>

using namespace std;
using namespace DllFrameRate;


namespace DllFrameRate
{

double desiredFps = 60.0;

double actualFps = 60.0;

bool isEnabled = false;

void enable()
{
    if ( isEnabled )
        return;

    timeBeginPeriod(1); // increase timer resolution

    // TODO find an alternative because this doesn't work on Wine
    WRITE_ASM_HACK ( AsmHacks::disableFpsLimit );
    WRITE_ASM_HACK ( AsmHacks::disableFpsCounter );

    isEnabled = true;

    LOG ( "Enabling FPS control!" );
}

}

long long DllFrameRate::frameStartTime = 0;
long long DllFrameRate::nextFrameTime = 0;

void DllFrameRate::limitFPSBeforePresent() {

    if ( !isEnabled || *CC_SKIP_FRAMES_ADDR )
      return;

    return;

    constexpr long long limit = (1000000000.0f / 61.0f) * 0.7;
    long long time;
    long long delta;
    do {
		time = getNanoSec();
        delta = time - DllFrameRate::frameStartTime;
	} while (delta < limit);
}

void DllFrameRate::limitFPSAfterPresent() {

    if ( !isEnabled || *CC_SKIP_FRAMES_ADDR )
      return;

	static long long prevTime = 0;
	long long time ;
	long long delta;

    constexpr long long limit = (1000000000.0f / 61.0f) * 1.0f;
    do {
		time = getNanoSec();
        //delta = time - prevTime;
        delta = time - DllFrameRate::frameStartTime;
	//} while (delta < limit);
    } while(time < nextFrameTime);

    constexpr int fpsBufferLen = 60;
    static double fpsBuffer[fpsBufferLen];
    static int fpsBufferIndex = 0;

    double fpsThisFrame = (1000000000.0 / (double)delta); 
    fpsBuffer[fpsBufferIndex] = fpsThisFrame;
    fpsBufferIndex = (fpsBufferIndex + 1) % fpsBufferLen;
    
    double minFps = fpsBuffer[0];
    double mean = fpsBuffer[0];
    double maxFps = fpsBuffer[0];
    for(int i=1; i<60; i++) {
        minFps = MIN(minFps, fpsBuffer[i]);
        mean += fpsBuffer[i];
        maxFps = MAX(maxFps, fpsBuffer[i]);
    }

    mean *= (1.0 / (double)fpsBufferLen);

    DWORD temp = 0;
    temp += (DWORD)round(mean);
    temp *= 1000;
    temp += (DWORD)round(minFps);
    temp *= 1000;
    temp += (DWORD)round(maxFps);

    *CC_FPS_COUNTER_ADDR = temp;

    time = getNanoSec();
    prevTime = time;
    DllFrameRate::frameStartTime = time;
    DllFrameRate::nextFrameTime = frameStartTime + ((long long)((1000000000.0f / 61.0f)));

}

void PresentFrameEnd ( IDirect3DDevice9 *device ) 
{
    DllFrameRate::limitFPSAfterPresent();
}