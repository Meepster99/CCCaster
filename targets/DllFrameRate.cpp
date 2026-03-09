#include "DllFrameRate.hpp"
#include "TimerManager.hpp"
#include "Constants.hpp"
#include "ProcessManager.hpp"
#include "DllAsmHacks.hpp"

#include <timeapi.h>

#include <d3dx9.h>
#include <math.h>

#include "DllAsmHacks.hpp"

//using namespace std;
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

	// this runs regardless of the setting in caster. why. 

    // TODO find an alternative because this doesn't work on Wine
    WRITE_ASM_HACK ( AsmHacks::disableFpsLimit );
    WRITE_ASM_HACK ( AsmHacks::disableFpsCounter );

	// this will call limitfps after present (and the stuff which hooks onto present) exits
	// it might be better? but it makes my frame counter look worse, for obvious reasons
	for ( const AsmHacks::Asm& hack : AsmHacks::hookPresentCaller ) {
		WRITE_ASM_HACK ( hack );
	}

    isEnabled = true;

    LOG ( "Enabling FPS control!" );
}

void oldCasterFrameLimiter() {

    static uint64_t last1f = 0, last5f = 0, last30f = 0, last60f = 0;
    static uint8_t counter = 0;
    
    ++counter;

    uint64_t now = TimerManager::get().getNow ( true );
    
    /**
     * The best timer resolution is only in milliseconds, and we need to make
     * sure the spacing between frames is as close to even as possible.
     *
     * What this code does is check every 30f, 5f, and 1f how many milliseconds have
     * passed since the last check and make sure we are close to or under the desired FPS.
     */
    

    /*
    below, the 1000 / desiredFps, the 1000 casts said number to an int
    causing the value to be 16, instead of 16.666, which is what led to the game running at ~62.5 instead of 60
    */ 
   
     if ( counter % 30 == 0 )
    {
        while ( now - last30f < ( 30 * 1000 ) / desiredFps )
            now = TimerManager::get().getNow ( true );

        last30f = now;
    }
    else if ( counter % 5 == 0 )
    {
        while ( now - last5f < ( 5 * 1000 ) / desiredFps )
            now = TimerManager::get().getNow ( true );

        last5f = now;
    }
    else
    {
        while ( now - last1f < 1000 / desiredFps )
            now = TimerManager::get().getNow ( true );
    }

    last1f = now;

    if ( counter >= 60 )
    {
        now = TimerManager::get().getNow ( true );

        actualFps = 1000.0 / ( ( now - last60f ) / 60.0 );

        *CC_FPS_COUNTER_ADDR = uint32_t ( actualFps + 0.5 );

        counter = 0;
        last60f = now;
    }

    
}

void newCasterFrameLimiter() {

    /*
    
    overall, this frame limiter is much better, but still is causing some issues
    is it needed to swing back and do a frame fast if a lag frame occurs?
    i had assumed that it would be better this way, maybe not.

    people are having issues, but ive also fixed a ton of other issues. its 50/50

    maybe aiming for a consistent 60 over ... yk what im doing now is better

    but hypothetically, if someone had a massive lagframe, it would cause the game to over/under/idkflow the value needed for the bounceback
    which would cause fastmelty? that ... makes sense tbh
    but doesnt explain how those issues last for more than 60 frames

    but tbh the other counter doesnt even do bounceback like how im saying here. it does wait every frame, and then does weird shit on every 5 and 30 and 60, but it still is just waiting
    why the fuck is this causing issues?

    if im really going to throw shit at the wall, its possible that metlys frame limiter not being disabled also causes issues?

	what if.. i just isolate the 
	i could maybe seperate the render state/gamestate

	in process explorer, nothings melty had rtkvhd64.sys as the entry instead of mbaa entry
	could that be something?

    */

    static int rollingFrameAverageSum = 0;
    static uint32_t rollingFrameAverage[60] = {0};\
    const int rollingFrameAverageLen = (sizeof(rollingFrameAverage)/sizeof(rollingFrameAverage[0]));
    static int rollingFrameAverageIndex = 0;

	static LARGE_INTEGER baseFreq;
	static LARGE_INTEGER freq; 
	static LARGE_INTEGER prevFrameTime;
	static LARGE_INTEGER millisecondDuration;

    static DWORD lastColor = 0x000000FF; // avoid excessive calls to patch

    static int lagFrameTimer = 0;

	static MMRESULT timeBeginPeriodRes = TIMERR_NOERROR;

    static uint32_t omfg;
	static bool isFirstRun = true;
	if(isFirstRun) {
		isFirstRun = false;

		QueryPerformanceFrequency(&baseFreq); // i need to handle errors here, and maybe fallback to a different system. is this guarenteed to have enough resolution?

		millisecondDuration.QuadPart = baseFreq.QuadPart / 1000;

        // fill the buffer with 60 to start
        uint32_t fillVar = desiredFps * 100;
        for(int i=0; i<rollingFrameAverageLen; i++) {
            rollingFrameAverage[i] = fillVar;
        }
        rollingFrameAverageSum = rollingFrameAverageLen * fillVar;

        omfg = fillVar;

		prevFrameTime.QuadPart = 0;

		timeBeginPeriodRes = timeBeginPeriod(1);
		timeEndPeriod(1);
	}

	freq.QuadPart = baseFreq.QuadPart / desiredFps; // hopefully this doesnt mess things up? this desiredfps var isnt touched anywhere really

	LARGE_INTEGER currTime;

    bool wasLagFrame = true;
	
	
	//log("%lld %lld %lld", currTime.QuadPart - prevFrameTime.QuadPart, millisecondDuration.QuadPart, freq.QuadPart);

	// does this help ? 
	// no fucking clue. i think it does?
	// check process explorer for a cycle count. try setting affinity to use only one core. 
	// this change seems to provide a good reduction in cpu usage (check process explorer thread properties)
	QueryPerformanceCounter(&currTime);
	LARGE_INTEGER frameTimeLeft;
	frameTimeLeft.QuadPart = (freq.QuadPart - (currTime.QuadPart - prevFrameTime.QuadPart));
	if((timeBeginPeriodRes == TIMERR_NOERROR) && (frameTimeLeft.QuadPart > 2 * millisecondDuration.QuadPart)) { // i tried putting this shit in the wait loop, dont, bad idea

		DWORD millis = frameTimeLeft.QuadPart / millisecondDuration.QuadPart;

		/*
		DWORD sleepTime = millis * 0.5;

		if(millis > 10) {
			sleepTime = millis * 0.75; // feel more comfy sleeping for longer if we have more leeway
		}*/

		DWORD sleepTime = millis - 1;
		if(lagFrameTimer > 0) { // we are lagging. possibly sleep less?
			sleepTime = millis * 0.67;
		}

		timeBeginPeriod(1);
		Sleep(sleepTime);
		timeEndPeriod(1);
	}
	
    while(true) {
		QueryPerformanceCounter(&currTime);
		if(currTime.QuadPart - prevFrameTime.QuadPart > freq.QuadPart) {
			break;
		}
        wasLagFrame = false; // being here means we have some time to wait.

		// ANY quantity of sleep in this loop massively frees up cpu, massively decreasing people complaining, but having it be done accurately is close to impossible.
	}

    if(wasLagFrame) { 
        lagFrameTimer = 30;
    } else {
        if(lagFrameTimer > 0) {
            lagFrameTimer--;
        }
    }

	uint32_t temp = (100*baseFreq.QuadPart) / (currTime.QuadPart - prevFrameTime.QuadPart - 1); // minus one is there to display 60, not 59.999999
	//log("%d %d", temp, omfg);
    rollingFrameAverageSum -= rollingFrameAverage[rollingFrameAverageIndex];
    rollingFrameAverageSum += temp;
    rollingFrameAverage[rollingFrameAverageIndex] = temp;
    rollingFrameAverageIndex = (rollingFrameAverageIndex + 1) % rollingFrameAverageLen;

    if(wasLagFrame || rollingFrameAverageIndex == 0) { // old frame limiter only updated this value every second
        //*CC_FPS_COUNTER_ADDR = ceil(((float)rollingFrameAverageSum * 0.01)/ rollingFrameAverageLen);
        //*CC_FPS_COUNTER_ADDR = round(((float)rollingFrameAverageSum * 0.01)/ rollingFrameAverageLen);
        *CC_FPS_COUNTER_ADDR = round(((float)rollingFrameAverageSum * 0.01)/ rollingFrameAverageLen);

        if(wasLagFrame || lagFrameTimer) {
            DWORD tempCol = 0xFFFF00DD;
            if(lastColor != tempCol) {
                patchDWORD(CC_FPS_COUNTER_COLOR, tempCol);
                lastColor = tempCol;
            }
        } else {
            DWORD tempCol = 0x0000FFFF;
            if(lastColor != tempCol) {
                patchDWORD(CC_FPS_COUNTER_COLOR, tempCol);
                lastColor = tempCol;
            }
        }   
    }
   
	prevFrameTime.QuadPart = currTime.QuadPart;
}

void limitFPS() {

	if ( !isEnabled || *CC_SKIP_FRAMES_ADDR )
        return;

	//oldCasterFrameLimiter();
	
	newCasterFrameLimiter();

}

}

void PresentFrameEnd ( IDirect3DDevice9 *device )
{
	// comment this out if you uncommented the hookPresentCaller hack
	//DllFrameRate::limitFPS();
}

void setDesiredFPS(double desiredFps_) {
    desiredFps = desiredFps_;
}
