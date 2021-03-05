#include <Common/GameTimer.hpp>
#include <Windows.h>

GameTimer::GameTimer() :
    mSecondsPerCount(.0), 
    mDeltaTime(-1.),
    mBaseTime(0),
    mPausedTime(0),
    mPrevTime(0),
    mCurrTime(0),
    mStopped(false)
{
    i64 countsPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    mSecondsPerCount = 1. / (f64)(countsPerSec);
}

// Returns the total time elapsed since Reset() was called, 
// NOT counting any time when the clock is stopped.
f32 GameTimer::TotalTime() const
{
    // If we are stopped, do not count the time that has passed since we stopped.
	// Moreover, if we previously already had a pause, the distance 
	// mStopTime - mBaseTime includes paused time, which we do not want to count.
	// To correct this, we can subtract the paused time from mStopTime:  
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime
    // if stopped then currTime = stopTime for the first frame
    // we need to take into account paused time from the moment of start app.
    if (mStopped)  
    {
        return (f32) ( ( ( mStopTime - mPausedTime ) - mBaseTime ) * mSecondsPerCount );
    }
    // The distance mCurrTime - mBaseTime includes paused time,
	// which we do not want to count.  To correct this, we can subtract 
	// the paused time from mCurrTime:  
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime
    else
    {
        return (f32) ( ( ( mCurrTime - mPausedTime ) - mBaseTime ) * mSecondsPerCount );
    }
}

f32 GameTimer::DeltaTime() const
{
    return (f32)(mDeltaTime);
}

void GameTimer::Reset() 
{
    i64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    mBaseTime = currTime;
    mPrevTime = currTime;
    mStopTime = 0;
    mStopped = false;
}

void GameTimer::Start() 
{
    // startTime (time after unpause for example)
    i64 startTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
    // Accumulate the time elapsed between stop and start pairs.
    //
    //                         pausedTime
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime    
    if (mStopped)
    {
        mPausedTime += (startTime - mStopTime);
        mPrevTime = startTime;
        mStopTime = 0;
        mStopped  = false;
    }
}

void GameTimer::Stop() 
{
    if (!mStopped)
    {
        i64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
        mStopTime = currTime;
        mStopped = true;
    }
}

void GameTimer::Tick() 
{
    if (mStopped) 
    {
        mDeltaTime = .0;
        return;
    }

    i64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
    mCurrTime = currTime;

    // Time difference between this frame and the previous.
    mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

    // Prepare for next frame;
    mPrevTime = mCurrTime;

    // Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
	// processor goes into a power save mode or we get shuffled to another
	// processor, then mDeltaTime can be negative.
    if (mDeltaTime < .0)
    {
        mDeltaTime = .0;
    }
}
