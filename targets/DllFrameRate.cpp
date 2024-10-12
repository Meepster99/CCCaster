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

constexpr int fpsBufferLen = 60;
double fpsBuffer[fpsBufferLen];
int fpsBufferIndex = 0;

constexpr int fpsLargeBufferLen = 600;
double fpsLargeBuffer[fpsLargeBufferLen];
int fpsLargeBufferIndex = 0;

void enable()
{
    if ( isEnabled )
        return;

    timeBeginPeriod(1); // increase timer resolution

    // TODO find an alternative because this doesn't work on Wine
    WRITE_ASM_HACK ( AsmHacks::disableFpsLimit );
    WRITE_ASM_HACK ( AsmHacks::disableFpsCounter );
    WRITE_ASM_HACK ( AsmHacks::limitHiteffectLoop );
    WRITE_ASM_HACK ( AsmHacks::limitEffectLoop );

    for(int i = 0; i<fpsBufferLen; i++) {
        fpsBuffer[i] = 60.0;
    }

    for(int i = 0; i<fpsLargeBufferLen; i++) {
        fpsLargeBuffer[i] = 60.0;
    }

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

    constexpr long long limit = (1000000000.0f / 60.0f) * 0.5;
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
        //delta = time - DllFrameRate::frameStartTime;
	//} while (delta < limit);
    } while(time < nextFrameTime);

    delta = time - DllFrameRate::frameStartTime;

    double fpsThisFrame = (1000000000.0 / (double)delta); 
    fpsBuffer[fpsBufferIndex] = fpsThisFrame;
    fpsBufferIndex = (fpsBufferIndex + 1) % fpsBufferLen;
    
    fpsLargeBuffer[fpsLargeBufferIndex] = fpsThisFrame;
    fpsLargeBufferIndex = (fpsLargeBufferIndex + 1) % fpsLargeBufferLen;

    double minFps = fpsBuffer[0];
    double mean = fpsBuffer[0];
    double maxFps = fpsBuffer[0];
    for(int i=1; i<fpsBufferLen; i++) {
        minFps = MIN(minFps, fpsBuffer[i]);
        mean += fpsBuffer[i];
        maxFps = MAX(maxFps, fpsBuffer[i]);
    }   

    mean *= (1.0 / (double)fpsBufferLen);

    double largeMean = 0.0;
    for(int i=0; i<fpsLargeBufferLen; i++) {
        largeMean += fpsLargeBuffer[i];
    }
    largeMean *= (1.0 / (double)fpsLargeBufferLen);

    DWORD temp = 0;
    temp += (DWORD)round(mean + 0.5);
    temp *= 1000;
    temp += (DWORD)round(minFps + 0.5);
    temp *= 1000;
    temp += (DWORD)round(maxFps + 0.5);

    *CC_FPS_COUNTER_ADDR = temp;

    time = getNanoSec();
    prevTime = time;
    DllFrameRate::frameStartTime = time;
    // 63 seemed ideal on my laptop, but more testing/dynamic value needed
    // this system self balances. is that ok? also, working in nanoseconds is becoming an issue due to precision.
    static double goalDivisor = 60.0;
    
    double goalDiff = largeMean - 60.0;
    double goalDiffSquare = goalDiff * goalDiff;
    if(goalDiff < 0.0) {
        goalDiffSquare *= -1;
    }
    goalDivisor -= ( goalDiffSquare * 0.001);

    //DllFrameRate::nextFrameTime = frameStartTime + ((long long)(((double)1000000000.0 / (double)63.0))); 
    DllFrameRate::nextFrameTime = frameStartTime + ((long long)(((double)1000000000.0 / goalDivisor))); 

}

void PresentFrameEnd ( IDirect3DDevice9 *device ) 
{
    DllFrameRate::limitFPSAfterPresent();
}