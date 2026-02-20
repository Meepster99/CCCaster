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

    // TODO find an alternative because this doesn't work on Wine
    WRITE_ASM_HACK ( AsmHacks::disableFpsLimit );
    WRITE_ASM_HACK ( AsmHacks::disableFpsCounter );

    isEnabled = true;

    LOG ( "Enabling FPS control!" );
}

}

void newCasterFrameLimiter() {
	
	static LARGE_INTEGER baseFreq;
	static LARGE_INTEGER freq; 
	static LARGE_INTEGER prevFrameTime;

	static bool isFirstRun = true;
	if(isFirstRun) {
		isFirstRun = false;

		QueryPerformanceFrequency(&baseFreq); // i need to handle errors here, and maybe fallback to a different system. is this guarenteed to have enough resolution?

		prevFrameTime.QuadPart = 0;
	}

	freq.QuadPart = baseFreq.QuadPart / desiredFps; // hopefully this doesnt mess things up? this desiredfps var isnt touched anywhere really

	LARGE_INTEGER currTime;

	while(true) {
		QueryPerformanceCounter(&currTime);
		if(currTime.QuadPart - prevFrameTime.QuadPart > freq.QuadPart) {
			break;
		}
	}

	uint32_t temp = (baseFreq.QuadPart) / (currTime.QuadPart - prevFrameTime.QuadPart - 1); // minus one is there to display 60, not 59.999999
	
	*CC_FPS_COUNTER_ADDR = temp;

	prevFrameTime.QuadPart = currTime.QuadPart;
}

void PresentFrameEnd ( IDirect3DDevice9 *device )
{
    if ( !isEnabled || *CC_SKIP_FRAMES_ADDR )
        return;

    // return;

    newCasterFrameLimiter();   
}
