//--------------------------------------------------------------
// Copyright (c) David Bosnich <david.bosnich.public@gmail.com>
//
// This code is licensed under the MIT License, a copy of which
// can be found in the license.txt file included at the root of
// this distribution, or at https://opensource.org/licenses/MIT
//--------------------------------------------------------------

#pragma once

#include <atomic>
#include <chrono>
#include <thread>

//! @file

//--------------------------------------------------------------
//! The target frames per second at which to run the update loop.
//! Default value; underlying variable can be changed at runtime.
//---------------------------------------------------------------
#ifndef DEFAULT_TARGET_FPS
#define DEFAULT_TARGET_FPS 60u
#endif//DEFAULT_TARGET_FPS

//--------------------------------------------------------------
//! Whether to cap the number of frames per second to the target.
//! Default value; underlying variable can be changed at runtime.
//--------------------------------------------------------------
#ifndef DEFAULT_CAPPED_FPS
#define DEFAULT_CAPPED_FPS true
#endif//DEFAULT_CAPPED_FPS

//--------------------------------------------------------------
namespace Simple
{

//--------------------------------------------------------------
//! Base class for process that starts, runs a loop, then stops.
//--------------------------------------------------------------
class UpdateLoop
{
public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;
    using TimePoint = Clock::time_point;

    UpdateLoop() = default;
    virtual ~UpdateLoop() = default;

    UpdateLoop(const UpdateLoop&) = delete;
    UpdateLoop& operator=(const UpdateLoop&) = delete;

    void Run(uint32_t a_targetFPS = DEFAULT_TARGET_FPS);
    std::thread RunInThread(uint32_t a_targetFPS = 60u);

    void SetTargetFPS(uint32_t a_targetFPS);
    void SetCappedFPS(bool a_cappedFPS);

    uint32_t GetTargetFPS() const;
    bool GetCappedFPS() const;

    void RequestShutDown();
    void RequestRestart();

protected:
    virtual void StartUp() = 0;
    virtual void ShutDown() = 0;

    virtual void UpdateStart(float a_deltaTimeSeconds) = 0;
    virtual void UpdateFixed(float a_fixedTimeSeconds) = 0;
    virtual void UpdateEnded(float a_deltaTimeSeconds) = 0;

    struct FrameStats
    {
        uint64_t numFrames = 0;
        uint32_t actualFPS = 0;
        uint32_t targetFPS = 0;
        Duration actualDur = {};
        Duration targetDur = {};
        Duration excessDur = {};
    };
    virtual void OnFrameComplete(const FrameStats& a_frameStats);

private:
    std::atomic_uint m_targetFPS = { DEFAULT_TARGET_FPS };
    std::atomic_bool m_cappedFPS = { DEFAULT_CAPPED_FPS };
    std::atomic_bool m_shutDownRequested = { false };
    std::atomic_bool m_restartRequested = { false };
};

//--------------------------------------------------------------
//! Run the update loop at the target fps, in the current thread.
//! @param[in] a_targetFPS The target fps (optional, default=60).
//--------------------------------------------------------------
inline void UpdateLoop::Run(uint32_t a_targetFPS)
{
    // Set the target frames per second.
    SetTargetFPS(a_targetFPS);

    do
    {
        // Clear any shut down or restart requests.
        m_shutDownRequested = false;
        m_restartRequested = false;

        // Start the application.
        StartUp();

        // Initialize accumulated frame duration with the target
        // duration to ensure a fixed update on the first frame.
        constexpr intmax_t oneSecond = Clock::period().den;
        Duration accumulatedDuration(oneSecond / m_targetFPS);

        // Initialize other values used to track frame duration.
        Duration lastDuration = Duration::zero();
        TimePoint lastEndTime = Clock::now();
        FrameStats frameStats = {};

        // Loop until a shut down or restart is requested.
        while (!m_shutDownRequested && !m_restartRequested)
        {
            // Target frame duration is fixed but depends on
            // the target fps that can change between frames.
            // Note that m_targetFPS is an atomic_uint value.
            const uint32_t targetFPS = m_targetFPS;
            const Duration targetDuration(oneSecond / targetFPS);
            const float fixedTime = 1.0f / (float)targetFPS;

            // Update at the start of each frame with a variable
            // delta time, derived using the last frame duration,
            // for non-deterministic systems requiring an update
            // each frame prior to any fixed updates (eg. input).
            // Cap in case the app is running slower than target.
            constexpr float oneSecondFloat = (float)oneSecond;
            const float durationF = (float)lastDuration.count();
            const float deltaTime = durationF / oneSecondFloat;
            const float deltaTimeCapped = std::min(deltaTime,
                                                   fixedTime);
            UpdateStart(deltaTimeCapped);

            // Check if accumulated duration has reached target.
            if (accumulatedDuration >= targetDuration)
            {
                // Update with a fixed delta time, derived from
                // the target frame duration, for deterministic
                // systems requiring fixed deltas (eg. physics).
                UpdateFixed(fixedTime);

                // Reduce accumulated duration by the amount
                // 'consumed' by the update. Clamp remainder
                // so it does not increase indefinitely when
                // app is running slower than the target fps.
                accumulatedDuration -= targetDuration;
                if (accumulatedDuration > targetDuration)
                {
                    accumulatedDuration = targetDuration;
                }
            }

            // Update at the end of each frame with the same
            // variable delta time for any non-deterministic
            // systems requiring updates at the end of every
            // frame after any fixed updates (eg. rendering).
            UpdateEnded(deltaTimeCapped);

            // Calculate time elapsed since the last frame ended,
            // and if capped wait until reaching target duration.
            // Note that m_cappedFPS is an atomic_bool value.
            const bool capped = m_cappedFPS;
            TimePoint endTime;
            do
            {
                endTime = Clock::now();
                lastDuration = endTime - lastEndTime;
            }
            while (capped && lastDuration < targetDuration);
            accumulatedDuration += lastDuration;
            lastEndTime = endTime;

            // Increment the frame counter, calculate actual fps
            // by rounding instead of truncating, and send stats.
            ++frameStats.numFrames;
            const intmax_t fpsDen = lastDuration.count();
            const intmax_t fpsNum = oneSecond + (fpsDen / 2);
            frameStats.actualFPS = (uint32_t)(fpsNum / fpsDen);
            frameStats.targetFPS = targetFPS;
            frameStats.actualDur = lastDuration;
            frameStats.targetDur = targetDuration;
            frameStats.excessDur = accumulatedDuration;
            OnFrameComplete(frameStats);
        }

        // Stop the application.
        ShutDown();
    }
    // Return if shut down was requested, loop if restart was.
    while (!m_shutDownRequested && m_restartRequested);
}

//--------------------------------------------------------------
//! Run the update loop at the target fps, spawning a new thread.
//! @param[in] a_targetFPS The target fps (optional, default=60).
//! @return The thread which was spawned to run the update loop.
//--------------------------------------------------------------
inline std::thread UpdateLoop::RunInThread(uint32_t a_targetFPS)
{
    std::thread runThread([this, a_targetFPS]()
    {
        Run(a_targetFPS);
    });
    return runThread;
}

//--------------------------------------------------------------
//! Set the target fps. If the update loop is not yet running it
//! will be overridden by the target fps sent to UpdateLoop::Run.
//! @param[in] a_targetFPS The target fps to run the update loop.
//--------------------------------------------------------------
inline void UpdateLoop::SetTargetFPS(uint32_t a_targetFPS)
{
    // Must always target at least one frame per second.
    m_targetFPS = a_targetFPS ? a_targetFPS : 1;
}

//--------------------------------------------------------------
//! Set whether the update loop is run capped to the target fps.
//! @param[in] a_cappedFPS Should the update loop be run capped?
//--------------------------------------------------------------
inline void UpdateLoop::SetCappedFPS(bool a_cappedFPS)
{
    m_cappedFPS = a_cappedFPS;
}

//--------------------------------------------------------------
//! Get the target fps that the update loop has been set to run.
//! @return Target fps that the update loop has been set to run.
//--------------------------------------------------------------
inline uint32_t UpdateLoop::GetTargetFPS() const
{
    return m_targetFPS;
}

//--------------------------------------------------------------
//! Get whether the update loop is run capped to the target fps.
//! @return True if update loop is run capped to the target fps.
//--------------------------------------------------------------
inline bool UpdateLoop::GetCappedFPS() const
{
    return m_cappedFPS;
}

//--------------------------------------------------------------
//! Request termination of the update loop.
//--------------------------------------------------------------
inline void UpdateLoop::RequestShutDown()
{
    m_shutDownRequested = true;
}

//--------------------------------------------------------------
//! Request a restart of the update loop.
//--------------------------------------------------------------
inline void UpdateLoop::RequestRestart()
{
    m_restartRequested = true;
}

//--------------------------------------------------------------
//! Called once each time the update loop starts running.
//--------------------------------------------------------------
inline void UpdateLoop::StartUp()
{
}

//--------------------------------------------------------------
//! Called once each time the update loop finishes running.
//--------------------------------------------------------------
inline void UpdateLoop::ShutDown()
{
}

//--------------------------------------------------------------
//! Called once each time the update loop starts another frame.
//! @param[in] a_deltaTimeSeconds Actual duration of last frame.
//--------------------------------------------------------------
inline void UpdateLoop::UpdateStart(float a_deltaTimeSeconds)
{
    (void)a_deltaTimeSeconds;
}

//--------------------------------------------------------------
//! Called once each time the accumulated frame duration reaches
//! the target frame duration. This will occur exactly once each
//! frame if the update loop is running capped to the target fps.
//! Otherwise it may not be called every frame, but if called it
//! will always be bookended by calls to UpdateStart/UpdateEnded.
//! @param[in] a_fixedTimeSeconds Target frame duration (fixed).
//--------------------------------------------------------------
inline void UpdateLoop::UpdateFixed(float a_fixedTimeSeconds)
{
    (void)a_fixedTimeSeconds;
}

//--------------------------------------------------------------
//! Called once each time the update loop ends the current frame.
//! @param[in] a_deltaTimeSeconds Actual duration of last frame.
//--------------------------------------------------------------
inline void UpdateLoop::UpdateEnded(float a_deltaTimeSeconds)
{
    (void)a_deltaTimeSeconds;
}

//--------------------------------------------------------------
//! Called once at the completion of each frame with some stats.
//! Should only be used for debug/diagnostic/profiling purposes.
//! @param[in] a_frameStats Stats related to the completed frame.
//--------------------------------------------------------------
inline void UpdateLoop::OnFrameComplete(const FrameStats&)
{
}

} // namespace Simple
