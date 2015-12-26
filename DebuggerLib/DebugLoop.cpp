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
#include "DebugLoop.h"
#include "Exceptions.h"

DebugLoop::DebugLoop(): _processId(0)
{
}

DebugLoop::~DebugLoop()
{
   if (_processId)
   {
      DebugActiveProcessStop(_processId);
   }
}

void DebugLoop::addHandler(DebugHandlerRef handler, uint32_t priority)
{
   _handlers.insert(make_pair(priority, handler));
}

void DebugLoop::perform()
{
   DEBUG_EVENT debugEvent;
   while (true)
   {
      WaitForDebugEvent(&debugEvent, INFINITE);
      _processId = debugEvent.dwProcessId;

      for (auto& pair : _handlers)
      {
         pair.second->handle(debugEvent);
      }

      if (EXIT_PROCESS_DEBUG_EVENT == debugEvent.dwDebugEventCode)
      {
         //only debugging one process at a time
         _processId = 0;
         if ( ! DebugActiveProcessStop(debugEvent.dwProcessId))
         {
            throw WinApiException();
         }
         break;
      }

      ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
   }
}
