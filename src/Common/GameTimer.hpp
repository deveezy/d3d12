#pragma once
#include <Common/defines.hpp>

class GameTimer
{
public:
    GameTimer();

    f32 TotalTime() const; // in seconds
    f32 DeltaTime() const; // in seconds

    void Reset(); // Call before message loop.
    void Start(); // Call when unpaused.
    void Stop();  // Call when paused.
    void Tick(); // Call every frame.

private:
    f64 mSecondsPerCount;
    f64 mDeltaTime;

    i64 mBaseTime;
    i64 mPausedTime;
    i64 mStopTime;
    i64 mPrevTime;
    i64 mCurrTime;

    bool mStopped;
};