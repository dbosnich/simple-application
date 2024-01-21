//--------------------------------------------------------------
// Copyright (c) David Bosnich <david.bosnich.public@gmail.com>
//
// This code is licensed under the MIT License, a copy of which
// can be found in the license.txt file included at the root of
// this distribution, or at https://opensource.org/licenses/MIT
//--------------------------------------------------------------

#pragma once

#include "update_loop.h"

//--------------------------------------------------------------
namespace Simple
{

//--------------------------------------------------------------
//! An UpdateLoop which stores arguments passed to the program.
//--------------------------------------------------------------
class Application : public UpdateLoop
{
public:
    Application() = default;
    Application(int a_argc, char* a_argv[]);
    ~Application() override = default;

    Application(const Application&) = delete;
    Application& operator= (const Application&) = delete;

    int GetArgCount() const;
    char** GetArgValues() const;

private:
    int m_argCount = 0;
    char** m_argValues = nullptr;
};

//--------------------------------------------------------------
//! Constructor.
//! \param[in] a_argc Count of arguments passed to the program.
//! \param[in] a_argv Array of arguments passed to the program.
//--------------------------------------------------------------
inline Application::Application(int a_argc, char* a_argv[])
    : m_argCount(a_argc), m_argValues(a_argv)
{
}

//--------------------------------------------------------------
//! Get the count of arguments passed to the program at startup.
//! \return Count of arguments passed to the program at startup.
//--------------------------------------------------------------
inline int Application::GetArgCount() const
{
    return m_argCount;
}

//--------------------------------------------------------------
//! Get the array of arguments passed to the program at startup.
//! \return Array of arguments passed to the program at startup.
//--------------------------------------------------------------
inline char** Application::GetArgValues() const
{
    return m_argValues;
}

} // namespace Simple
