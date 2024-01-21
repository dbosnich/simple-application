# Simple Application


### Overview
SimpleApplication is a header-only C++ framework that can act as
base class for any process that starts, runs a loop, then stops.

Designed primarily to act as the entry point into an application
that runs in a loop but could also be used to spawn a subprocess.

The primary design consideration was ease-of-use, although every
effort has been made to not compromise efficiency or flexibility.


### Features
#### Target FPS
  Simple::Application::Run accepts a target FPS argument that is
  used to pace all calls to Simple::Application::Update* methods.

  Simple::Application::SetTargetFPS can be called to change the
  target frames per second if the application is already running.

#### Fixed Updates
  Simple::Application::UpdateFixed is called with a fixed delta
  time parameter once accumulated frame time reaches the target,
  meaning it may not get called each frame unless fps is capped.

#### Variable Updates
  Simple::Application::UpdateStart|UpdateEnded are called with a
  variable delta time parameter once every frame. A fixed update
  will always be bookended by calls to UpdateStart / UpdateEnded.

#### Capped FPS
  Simple::Application::SetCappedFPS can be called anytime to cap
  the speed at which variable updates occur to the target FPS so
  UpdateStart/Fixed/Ended are all called exactly once each frame.


### Platforms
This project has been tested using the following C++11 compilers:
- msvc (Visual Studio 2022)
- clang (Xcode 15)
- gcc (make)

CMake can be used to generate test projects (eg. VS, Xcode, make)
that build/run the suite of unit tests found in the tests folder.


### Example
The following is a small sample of the core functionality which
is provided by the framework. The above mentioned test suite is
also a good reference that demonstrates a broader range of uses.

#### MyApplication.h
```
#include <simple/application/application.h>

class MyApplication : public Simple::Application
{
protected:
    void StartUp() override;
    void ShutDown() override;

    void UpdateStart(float a_deltaTimeSeconds) override;
    void UpdateFixed(float a_fixedTimeSeconds) override;
    void UpdateEnded(float a_deltaTimeSeconds) override;
};
```

#### MyApplication.cpp
```
#include "MyApplication.h"

void MyApplication::StartUp()
{
    // Create all systems.
}

void MyApplication::ShutDown()
{
    // Destroy all systems.
}

void MyApplication::UpdateStart(float a_deltaTimeSeconds)
{
    // Update non-deterministic systems that require an update
    // at the start of every frame, prior to any fixed updates.
    // eg. Input
}

void MyApplication::UpdateFixed(float a_fixedTimeSeconds)
{
    // Update deterministic systems that require a fixed update.
    // eg. Physics
}

void MyApplication::UpdateEnded(float a_deltaTimeSeconds)
{
    // Update non-deterministic systems that require an update
    // at the end of every frame, after any fixed update calls.
    // eg. Rendering
}
```

#### main.cpp
```
#include "MyApplication.h"

int main(int argc, char** argv)
{
    MyApplication myApplication;
    myApplication.Run();
}
```


### Authors
- David Bosnich - <david.bosnich.public@gmail.com>


### License
Copyright (c) David Bosnich <david.bosnich.public@gmail.com>

This code is licensed under the MIT License, a copy of which
can be found in the license.txt file included at the root of
this distribution, or at https://opensource.org/licenses/MIT
