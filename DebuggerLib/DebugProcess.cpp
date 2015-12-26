/*
Profiler for evaluating an algorithms' complexity by running it with a predefined set of input values
Copyright (C) 2015 dani.user@gmail.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "DebugProcess.h"
#include "Exceptions.h"

using namespace std;

DebugProcess::DebugProcess(const wchar_t* fileName) : _fileName(fileName)
{
   memset(&_startupInfo, 0, sizeof(STARTUPINFO));
   _startupInfo.cb = sizeof(STARTUPINFO);

   memset(&_processInfo, 0, sizeof(PROCESS_INFORMATION));
}

DebugProcess::~DebugProcess()
{
   if (_processInfo.hProcess)
   {
      CloseHandle(_processInfo.hProcess);
   }
}

void DebugProcess::create()
{
   if ( ! CreateProcess(_fileName.c_str(), L"", nullptr, nullptr, FALSE, DEBUG_ONLY_THIS_PROCESS | PROCESS_QUERY_INFORMATION, 
      nullptr, nullptr, &_startupInfo, &_processInfo))
   {
      throw ProcessCreateFailedException();
   }
}

void DebugProcess::waitForExit() const
{
   WaitForSingleObject(getProcessHandle(), INFINITE);
}

void DebugProcess::terminate(UINT exitCode)
{
   TerminateProcess(getProcessHandle(), exitCode);
}

const PROCESS_INFORMATION& DebugProcess::getProcessInfo() const
{
   return _processInfo;
}

HANDLE DebugProcess::getProcessHandle() const
{
   return _processInfo.hProcess;
}
