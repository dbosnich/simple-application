//--------------------------------------------------------------
// Copyright (c) David Bosnich <david.bosnich.public@gmail.com>
//
// This code is licensed under the MIT License, a copy of which
// can be found in the license.txt file included at the root of
// this distribution, or at https://opensource.org/licenses/MIT
//--------------------------------------------------------------

#include <simple/application/application.h>
#include <catch2/catch.hpp>
#include <inttypes.h>
#include <random>

//--------------------------------------------------------------
struct TestParams
{
    uint32_t numFrames = 1;
    uint32_t numRestarts = 0;
    uint32_t targetFPSMin = 60;
    uint32_t targetFPSMax = 60;
    uint32_t updateStartMsMin = 0;
    uint32_t updateStartMsMax = 0;
    uint32_t updateFixedMsMin = 0;
    uint32_t updateFixedMsMax = 0;
    uint32_t updateEndedMsMin = 0;
    uint32_t updateEndedMsMax = 0;
    bool cappedTargetFPS = true;
    bool printFrameStats = true;
    bool runningInThread = false;
    bool useSleepForWork = true;
};

//--------------------------------------------------------------
class TestApplication : public Simple::Application
{
public:
    TestApplication(const TestParams& a_testParams);
    TestApplication(int a_argc, char* a_argv[]);
    ~TestApplication() override;

protected:
    void StartUp() override;
    void ShutDown() override;

    void UpdateStart(float a_deltaTimeSeconds) override;
    void UpdateFixed(float a_fixedTimeSeconds) override;
    void UpdateEnded(float a_deltaTimeSeconds) override;

    void OnFrameComplete(const FrameStats& a_stats) override;

private:
    bool TestingWithFixedFPS() const;
    void SleepFor(uint32_t a_milliseconds) const;
    void SpinFor(uint32_t a_milliseconds) const;
    void WorkFor(uint32_t a_millisecondsMin,
                 uint32_t a_millisecondsMax) const;

    const TestParams m_testParams;

    uint32_t m_startUpCountThisRun = 0;
    uint32_t m_shutDownCountThisRun = 0;
    uint32_t m_updateStartCountThisRun = 0;
    uint32_t m_updateFixedCountThisRun = 0;
    uint32_t m_updateEndedCountThisRun = 0;

    uint32_t m_startUpCountTotal = 0;
    uint32_t m_shutDownCountTotal = 0;
    uint32_t m_updateStartCountTotal = 0;
    uint32_t m_updateFixedCountTotal = 0;
    uint32_t m_updateEndedCountTotal = 0;
    uint32_t m_restartRequests = 0;
};

//--------------------------------------------------------------
template <class T = uint32_t>
T RandomInt(T a_min, T a_max)
{
    static std::random_device s_randomDevice;
    static std::mt19937 s_generator(s_randomDevice());
    std::uniform_int_distribution<T> distribution(a_min, a_max);
    return distribution(s_generator);
}

//--------------------------------------------------------------
TestApplication::TestApplication(const TestParams& a_testParams)
    : m_testParams(a_testParams)
{
    SetCappedFPS(m_testParams.cappedTargetFPS);

    // Per run values (reset in ShutDown).
    REQUIRE(m_startUpCountThisRun == 0);
    REQUIRE(m_shutDownCountThisRun == 0);
    REQUIRE(m_updateStartCountThisRun == 0);
    REQUIRE(m_updateFixedCountThisRun == 0);
    REQUIRE(m_updateEndedCountThisRun == 0);

    // Total values (persist between restarts).
    REQUIRE(m_startUpCountTotal == 0);
    REQUIRE(m_shutDownCountTotal == 0);
    REQUIRE(m_updateStartCountTotal == 0);
    REQUIRE(m_updateFixedCountTotal == 0);
    REQUIRE(m_updateEndedCountTotal == 0);
}

//--------------------------------------------------------------
TestApplication::TestApplication(int a_argc, char* a_argv[])
    : Application(a_argc, a_argv)
{
}

//--------------------------------------------------------------
TestApplication::~TestApplication()
{
    // Per run values (reset in ShutDown).
    REQUIRE(m_startUpCountThisRun == 0);
    REQUIRE(m_shutDownCountThisRun == 0);
    REQUIRE(m_updateStartCountThisRun == 0);
    REQUIRE(m_updateFixedCountThisRun == 0);
    REQUIRE(m_updateEndedCountThisRun == 0);

    // Total values (persist between restarts).
    const uint32_t totalRuns = m_testParams.numRestarts + 1;
    const uint32_t totalFrames = (m_testParams.numFrames *
                                  totalRuns);
    REQUIRE(m_startUpCountTotal == totalRuns);
    REQUIRE(m_shutDownCountTotal == totalRuns);
    REQUIRE(m_updateStartCountTotal == totalFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountTotal == totalFrames :
             m_updateFixedCountTotal <= totalFrames));
    REQUIRE(m_updateEndedCountTotal == totalFrames);
}

//--------------------------------------------------------------
void TestApplication::StartUp()
{
    // Per run values (reset in ShutDown).
    REQUIRE(m_startUpCountThisRun == 0);
    REQUIRE(m_shutDownCountThisRun == 0);
    REQUIRE(m_updateStartCountThisRun == 0);
    REQUIRE(m_updateFixedCountThisRun == 0);
    REQUIRE(m_updateEndedCountThisRun == 0);

    // Total values (persist between restarts).
    const uint32_t totalFrames = (m_startUpCountTotal *
                                  m_testParams.numFrames);
    REQUIRE(m_startUpCountTotal == m_restartRequests);
    REQUIRE(m_shutDownCountTotal == m_startUpCountTotal);
    REQUIRE(m_updateStartCountTotal == totalFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountTotal == totalFrames :
             m_updateFixedCountTotal <= totalFrames));
    REQUIRE(m_updateEndedCountTotal == totalFrames);

    // Increment counts.
    ++m_startUpCountThisRun;
    ++m_startUpCountTotal;
}

//--------------------------------------------------------------
void TestApplication::ShutDown()
{
    // Per run values (reset in ShutDown).
    REQUIRE(m_startUpCountThisRun == 1);
    REQUIRE(m_shutDownCountThisRun == 0);
    REQUIRE(m_updateStartCountThisRun == m_testParams.numFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountThisRun == m_testParams.numFrames :
             m_updateFixedCountThisRun <= m_testParams.numFrames));
    REQUIRE(m_updateEndedCountThisRun == m_testParams.numFrames);

    // Total values (persist between restarts).
    const uint32_t totalFrames = (m_startUpCountTotal *
                                  m_testParams.numFrames);
    REQUIRE((m_startUpCountTotal == m_restartRequests ||
             m_restartRequests == m_testParams.numRestarts));
    REQUIRE(m_shutDownCountTotal == m_startUpCountTotal - 1);
    REQUIRE(m_updateStartCountTotal == totalFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountTotal == totalFrames :
             m_updateFixedCountTotal <= totalFrames));
    REQUIRE(m_updateEndedCountTotal == totalFrames);

    // Increment counts.
    ++m_shutDownCountThisRun;
    ++m_shutDownCountTotal;

    // Reset all per run values.
    m_shutDownCountThisRun = 0;
    m_startUpCountThisRun = 0;
    m_updateStartCountThisRun = 0;
    m_updateFixedCountThisRun = 0;
    m_updateEndedCountThisRun = 0;
}

//--------------------------------------------------------------
void TestApplication::UpdateStart(float a_deltaTimeSeconds)
{
    // Called once at the start of every frame.

    // Delta time is variable but should never exceed the target,
    // unless the target fps is changed after the update started.
    if (m_testParams.targetFPSMin == m_testParams.targetFPSMax)
    {
        const float targetFPS = (float)GetTargetFPS();
        REQUIRE(a_deltaTimeSeconds <= 1.0f / targetFPS);
    }

    // Per run values (reset in ShutDown).
    REQUIRE(m_startUpCountThisRun == 1);
    REQUIRE(m_shutDownCountThisRun == 0);
    REQUIRE(m_updateStartCountThisRun < m_testParams.numFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountThisRun == m_updateStartCountThisRun :
             m_updateFixedCountThisRun <= m_updateStartCountThisRun));
    REQUIRE(m_updateEndedCountThisRun == m_updateStartCountThisRun);

    // Total values (persist between restarts).
    const uint32_t totalFrames = (m_updateStartCountThisRun +
                                  ((m_startUpCountTotal - 1) *
                                   m_testParams.numFrames));
    REQUIRE(m_startUpCountTotal == m_restartRequests + 1);
    REQUIRE(m_shutDownCountTotal == m_startUpCountTotal - 1);
    REQUIRE(m_updateStartCountTotal == totalFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountTotal == totalFrames :
             m_updateFixedCountTotal <= totalFrames));
    REQUIRE(m_updateEndedCountTotal == totalFrames);

    // Simulate processing.
    WorkFor(m_testParams.updateStartMsMin,
            m_testParams.updateStartMsMax);

    // Change the target fps if there is a valid range.
    if (m_testParams.targetFPSMin < m_testParams.targetFPSMax)
    {
        const uint32_t min = m_testParams.targetFPSMin;
        const uint32_t max = m_testParams.targetFPSMax;
        SetTargetFPS(RandomInt(min, max));
    }

    // Increment counts.
    ++m_updateStartCountThisRun;
    ++m_updateStartCountTotal;

    // Request restart or shut down after running enough frames.
    if (m_updateStartCountThisRun == m_testParams.numFrames)
    {
        if (m_restartRequests < m_testParams.numRestarts)
        {
            RequestRestart();
            ++m_restartRequests;
        }
        else
        {
            RequestShutDown();
        }
    }
}

//--------------------------------------------------------------
void TestApplication::UpdateFixed(float a_fixedTimeSeconds)
{
    // If running capped to the target fps, UpdateFixed will be
    // called once a frame, between UpdateStart and UpdateEnded.
    // Otherwise it may not be called on every frame, and which
    // frames is indeterminate/hardware dependent, but if it is
    // then it will happen between UpdateStart and UpdateEnded.

    // Delta time is fixed (unless the target fps has changed).
    if (m_testParams.targetFPSMin == m_testParams.targetFPSMax)
    {
        const float targetFPS = (float)GetTargetFPS();
        REQUIRE(a_fixedTimeSeconds == 1.0f / targetFPS);
    }

    // Per run values (reset in ShutDown).
    REQUIRE(m_startUpCountThisRun == 1);
    REQUIRE(m_shutDownCountThisRun == 0);
    REQUIRE(m_updateStartCountThisRun <= m_testParams.numFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountThisRun == m_updateEndedCountThisRun :
             m_updateFixedCountThisRun <= m_updateEndedCountThisRun));
    REQUIRE(m_updateEndedCountThisRun + 1 == m_updateStartCountThisRun);

    // Total values (persist between restarts).
    const uint32_t totalFrames = (m_updateStartCountThisRun +
                                  ((m_startUpCountTotal - 1) *
                                   m_testParams.numFrames));
    REQUIRE((m_startUpCountTotal == m_restartRequests ||
             m_startUpCountTotal == m_restartRequests + 1));
    REQUIRE(m_shutDownCountTotal == m_startUpCountTotal - 1);
    REQUIRE(m_updateStartCountTotal == totalFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountTotal + 1 == totalFrames :
             m_updateFixedCountTotal + 1 <= totalFrames));
    REQUIRE(m_updateEndedCountTotal + 1 == totalFrames);

    // Simulate processing.
    WorkFor(m_testParams.updateFixedMsMin,
            m_testParams.updateFixedMsMax);

    // Increment counts.
    ++m_updateFixedCountThisRun;
    ++m_updateFixedCountTotal;
}

//--------------------------------------------------------------
void TestApplication::UpdateEnded(float a_deltaTimeSeconds)
{
    // Called once at the end of every frame.

    // Delta time is variable but should never exceed the target,
    // unless the target fps is changed after the update started.
    if (m_testParams.targetFPSMin == m_testParams.targetFPSMax)
    {
        const float targetFPS = (float)GetTargetFPS();
        REQUIRE(a_deltaTimeSeconds <= 1.0f / targetFPS);
    }

    // Per run values (reset in ShutDown).
    REQUIRE(m_startUpCountThisRun == 1);
    REQUIRE(m_shutDownCountThisRun == 0);
    REQUIRE(m_updateStartCountThisRun <= m_testParams.numFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountThisRun == m_updateStartCountThisRun :
             m_updateFixedCountThisRun <= m_updateStartCountThisRun));
    REQUIRE(m_updateEndedCountThisRun + 1 == m_updateStartCountThisRun);

    // Total values (persist between restarts).
    const uint32_t totalFrames = (m_updateStartCountThisRun +
                                  ((m_startUpCountTotal - 1) *
                                   m_testParams.numFrames));
    REQUIRE((m_startUpCountTotal == m_restartRequests ||
             m_startUpCountTotal == m_restartRequests + 1));
    REQUIRE(m_shutDownCountTotal == m_startUpCountTotal - 1);
    REQUIRE(m_updateStartCountTotal == totalFrames);
    REQUIRE((TestingWithFixedFPS() ?
             m_updateFixedCountTotal == totalFrames :
             m_updateFixedCountTotal <= totalFrames));
    REQUIRE(m_updateEndedCountTotal + 1 == totalFrames);

    // Simulate processing.
    WorkFor(m_testParams.updateEndedMsMin,
            m_testParams.updateEndedMsMax);

    // Increment counts.
    ++m_updateEndedCountThisRun;
    ++m_updateEndedCountTotal;
}

//--------------------------------------------------------------
inline int64_t ToMs(const TestApplication::Duration& a_duration)
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(a_duration).count();
}

//--------------------------------------------------------------
void TestApplication::OnFrameComplete(const FrameStats& a_stats)
{
    REQUIRE(a_stats.frameCount == m_updateStartCountThisRun);
    REQUIRE((TestingWithFixedFPS() ?
             a_stats.frameCount == m_updateFixedCountThisRun :
             a_stats.frameCount >= m_updateFixedCountThisRun));
    REQUIRE(a_stats.frameCount == m_updateEndedCountThisRun);

    if (GetCappedFPS() && !m_testParams.runningInThread)
    {
        REQUIRE(a_stats.averageFPS <= a_stats.targetFPS);
        REQUIRE(a_stats.actualDur >= a_stats.targetDur);
    }

    if (m_testParams.printFrameStats)
    {
        printf("\n"
               "Frame count:    %" PRIu64 "\n"
               "Test number:    %" PRIu32 "\n"
               "Average FPS:    %" PRIu32 "\n"
               "Target FPS:     %" PRIu32 "\n"
               "Actual Dur:     %" PRIi64 " (ms)\n"
               "Target Dur:     %" PRIi64 " (ms)\n"
               "Excess Dur:     %" PRIi64 " (ms)\n"
               "Total Dur:      %" PRIi64 " (ms)\n",
               a_stats.frameCount,
               m_startUpCountTotal,
               a_stats.averageFPS,
               a_stats.targetFPS,
               ToMs(a_stats.actualDur),
               ToMs(a_stats.targetDur),
               ToMs(a_stats.excessDur),
               ToMs(a_stats.totalDur));
    }
}

//--------------------------------------------------------------
bool TestApplication::TestingWithFixedFPS() const
{
    return (GetCappedFPS() &&
            m_testParams.targetFPSMin == m_testParams.targetFPSMax);
}

//--------------------------------------------------------------
void TestApplication::SleepFor(uint32_t a_milliseconds) const
{
    using namespace std::chrono;
    std::this_thread::sleep_for(milliseconds(a_milliseconds));
}

//--------------------------------------------------------------
void TestApplication::SpinFor(uint32_t a_milliseconds) const
{
    using namespace std::chrono;
    const milliseconds spinForMs = milliseconds(a_milliseconds);
    const Duration spinFor = duration_cast<Duration>(spinForMs);
    const TimePoint spinStart = Clock::now();
    while ((Clock::now() - spinStart) < spinFor);
}

//--------------------------------------------------------------
void TestApplication::WorkFor(uint32_t a_millisecondsMin,
                              uint32_t a_millisecondsMax) const
{
    if (a_millisecondsMin > a_millisecondsMax)
    {
        return;
    }

    const uint32_t milliseconds = RandomInt(a_millisecondsMin,
                                            a_millisecondsMax);
    if (milliseconds == 0)
    {
        return;
    }

    if (m_testParams.useSleepForWork)
    {
        SleepFor(milliseconds);
    }
    else
    {
        SpinFor(milliseconds);
    }
}

//--------------------------------------------------------------
inline void RunTestApplication(const TestParams& a_testParams)
{
    TestApplication testApplication(a_testParams);
    testApplication.Run(a_testParams.targetFPSMin);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Default", "[application][default]")
{
    TestParams testParams;
    TestApplication testApplication(testParams);
    testApplication.Run();
}

//--------------------------------------------------------------
TEST_CASE("Test Application Multiple", "[application][multiple]")
{
    TestParams testParams;
    TestApplication testApplication1(testParams);
    TestApplication testApplication2(testParams);
    testApplication2.Run();
    testApplication1.Run();
}

//--------------------------------------------------------------
TEST_CASE("Test Application Args", "[application][args]")
{
    constexpr int argCount = 3;
    const char* argVals[argCount] = { "arg0", "arg1", "arg2" };
    TestApplication testApplication(argCount, (char**)argVals);
    REQUIRE(testApplication.GetArgCount() == argCount);
    REQUIRE(testApplication.GetArgValues() == argVals);
    testApplication.Run();
}

//--------------------------------------------------------------
TEST_CASE("Test Application Frames", "[application][frames]")
{
    TestParams testParams;
    testParams.numFrames = 3;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Restart", "[application][restart]")
{
    TestParams testParams;
    testParams.numRestarts = 3;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Uncapped", "[application][uncapped]")
{
    TestParams testParams;
    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application 30fps", "[application][30fps]")
{
    TestParams testParams;
    testParams.targetFPSMin = 30;
    testParams.targetFPSMax = 30;
    testParams.numFrames = 1;
    testParams.numRestarts = 0;
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application 120fps", "[application][120fps]")
{
    TestParams testParams;
    testParams.targetFPSMin = 120;
    testParams.targetFPSMax = 120;
    testParams.numFrames = 2;
    testParams.numRestarts = 0;
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application 240fps", "[application][240fps]")
{
    TestParams testParams;
    testParams.targetFPSMin = 240;
    testParams.targetFPSMax = 240;
    testParams.numFrames = 3;
    testParams.numRestarts = 0;
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application rnd fps", "[application][rnd_fps]")
{
    TestParams testParams;
    testParams.targetFPSMin = 1;
    testParams.targetFPSMax = 240;
    testParams.numFrames = 5;
    testParams.numRestarts = 0;
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Update Start", "[application][start]")
{
    TestParams testParams;
    testParams.targetFPSMin = 100;
    testParams.targetFPSMax = 100;
    testParams.numFrames = 5;
    testParams.numRestarts = 0;
    testParams.updateStartMsMin = 10;
    testParams.updateStartMsMax = 10;
    testParams.useSleepForWork = false; // Sleep is not precise
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Update Fixed", "[application][fixed]")
{
    TestParams testParams;
    testParams.targetFPSMin = 50;
    testParams.targetFPSMax = 50;
    testParams.numFrames = 5;
    testParams.numRestarts = 0;
    testParams.updateFixedMsMin = 20;
    testParams.updateFixedMsMax = 20;
    testParams.useSleepForWork = false; // Sleep is not precise
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Update Ended", "[application][ended]")
{
    TestParams testParams;
    testParams.targetFPSMin = 200;
    testParams.targetFPSMax = 200;
    testParams.numFrames = 5;
    testParams.numRestarts = 0;
    testParams.updateEndedMsMin = 5;
    testParams.updateEndedMsMax = 5;
    testParams.useSleepForWork = false; // Sleep is not precise
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Split", "[application][split]")
{
    TestParams testParams;
    testParams.targetFPSMin = 30;
    testParams.targetFPSMax = 30;
    testParams.numFrames = 5;
    testParams.numRestarts = 0;
    testParams.updateStartMsMin = 10;
    testParams.updateStartMsMax = 10;
    testParams.updateFixedMsMin = 10;
    testParams.updateFixedMsMax = 10;
    testParams.updateEndedMsMin = 10;
    testParams.updateEndedMsMax = 10;
    testParams.useSleepForWork = false; // Sleep is not precise
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Faster", "[application][faster]")
{
    TestParams testParams;
    testParams.targetFPSMin = 30;
    testParams.targetFPSMax = 30;
    testParams.numFrames = 5;
    testParams.numRestarts = 0;
    testParams.updateStartMsMin = 20;
    testParams.updateStartMsMax = 20;
    testParams.useSleepForWork = false; // Sleep is not precise
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Slower", "[application][slower]")
{
    TestParams testParams;
    testParams.targetFPSMin = 60;
    testParams.targetFPSMax = 60;
    testParams.numFrames = 5;
    testParams.numRestarts = 0;
    testParams.updateStartMsMin = 30;
    testParams.updateStartMsMax = 30;
    testParams.useSleepForWork = false; // Sleep is not precise
    RunTestApplication(testParams);

    testParams.cappedTargetFPS = false;
    RunTestApplication(testParams);
}

//--------------------------------------------------------------
TEST_CASE("Test Application Random", "[application][random]")
{
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<uint32_t> fps(1, 240);
    std::uniform_int_distribution<uint32_t> frames(1, 5);
    std::uniform_int_distribution<uint32_t> restarts(0, 3);
    std::uniform_int_distribution<uint32_t> fpsCapped(0, 1);

    TestParams testParams;
    constexpr uint32_t numRandomTests = 3;
    for (uint32_t i = 0; i < numRandomTests; ++i)
    {
        const uint32_t targetFPS = fps(generator);
        testParams.targetFPSMin = targetFPS;
        testParams.targetFPSMax = targetFPS;
        testParams.numFrames = frames(generator);
        testParams.numRestarts = restarts(generator);
        testParams.updateStartMsMin = (1000 / targetFPS) / 2;
        testParams.updateStartMsMax = (1000 / targetFPS) * 2;
        testParams.cappedTargetFPS = fpsCapped(generator);
        RunTestApplication(testParams);
    }
}

//--------------------------------------------------------------
TEST_CASE("Test Application Thread", "[application][thread]")
{
    TestParams testParams;
    testParams.targetFPSMin = 1;
    testParams.targetFPSMax = 240;
    testParams.numFrames = 10;
    testParams.numRestarts = 3;
    testParams.updateStartMsMin = 10;
    testParams.updateStartMsMax = 30;
    testParams.runningInThread = true;

    TestApplication testApplication(testParams);
    testApplication.SetCappedFPS(true);
    std::atomic_bool running = { true };
    std::thread runThread([&testApplication, &running]()
    {
        testApplication.Run();
        running = false;
    });

    while (running)
    {
        const uint32_t min = testParams.targetFPSMin;
        const uint32_t max = testParams.targetFPSMax;
        const uint32_t fps = RandomInt(min, max);
        testApplication.SetTargetFPS(fps);
        testApplication.SetCappedFPS(RandomInt(0, 1));
    }

    runThread.join();

    TestApplication testApplication2(testParams);
    std::thread runThread2 = testApplication2.RunInThread();
    std::thread runThread3 = testApplication2.RunInThread(); // Should do nothing.
    runThread3.join();
    runThread2.join();
}
