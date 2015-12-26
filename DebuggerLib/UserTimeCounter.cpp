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
#include "UserTimeCounter.h"
#include "Exceptions.h"
#include "RemotePointer.h"

using namespace std;

UserTimeCounter::UserTimeCounter(const char* functionName, SymbolCollectorRef symbolCollector)
   : _functionName(functionName), _symbolCollector(symbolCollector), _functionAddress(nullptr),
   _functionReturnAddress(nullptr), _externalFunctionReturnAddress(nullptr), _process(nullptr), 
   _thread(nullptr), _enteredFunction(false), _currentInput(0)
{
}

void UserTimeCounter::handle(const DEBUG_EVENT& event)
{
   switch (event.dwDebugEventCode)
   {
   case CREATE_PROCESS_DEBUG_EVENT:
      onCreateProcess(event.u.CreateProcessInfo);
      break;
   case EXCEPTION_DEBUG_EVENT:
      if (EXCEPTION_BREAKPOINT == event.u.Exception.ExceptionRecord.ExceptionCode)
      {
         onBreakpointHit(event.u.Exception.ExceptionRecord.ExceptionAddress);
      }
      break;
   default:
      break;
   }
}

const map<uint32_t, uint64_t>& UserTimeCounter::getResults() const
{
   return _instructionCount;
}

void UserTimeCounter::setNewResultHandler(const function<void(const pair<uint32_t, uint64_t>&)>& value)
{
   _newResultHandler = value;
}

void UserTimeCounter::onCreateProcess(const CREATE_PROCESS_DEBUG_INFO& info)
{
   if (_functionAddress)
   {
      //this event should only be handled once
      return;
   }

   auto symbols = _symbolCollector->getSymbols();
   auto entry = symbols.find(_functionName);
   if (entry == symbols.end())
   {
      throw FunctionNotFoundException(_functionName.c_str());
   }

   _functionAddress = entry->second.address;

   _process = info.hProcess;
   _thread = info.hThread;

   _functionEnterBreakpoint = createBreakpoint(_functionAddress);
   _functionEnterBreakpoint->set();
}

void UserTimeCounter::onFunctionEntered()
{
   FILETIME creationTime, exitTime, kernelTime;
   GetProcessTimes(_process, &creationTime, &exitTime, &kernelTime, &_functionStartTime);

   auto threadContext = getThreadContext();

   RemotePointer<uint32_t> currentInputPtr(_process, reinterpret_cast<void*>(threadContext.Esp + 4));
   _currentInput = *currentInputPtr;

   _enteredFunction = true;
}

void UserTimeCounter::onFunctionReturn()
{
   auto it = _instructionCount.find(_currentInput);
   if (it != _instructionCount.end())
   {
      _instructionCount.erase(it);
   }

   FILETIME creationTime, exitTime, kernelTime, functionEndTime;
   GetProcessTimes(_process, &creationTime, &exitTime, &kernelTime, &functionEndTime);

   //100 ns duration
   auto duration = reinterpret_cast<LARGE_INTEGER*>(&functionEndTime)->QuadPart - 
      reinterpret_cast<LARGE_INTEGER*>(&_functionStartTime)->QuadPart;
   //converto to microseconds
   duration /= 10;

   auto pair = make_pair(_currentInput, duration);
   _instructionCount.insert(pair);
   if (_newResultHandler)
   {
      _newResultHandler(pair);
   }
   _currentInput = 0;

   //function might be called again
   _functionEnterBreakpoint->set();
   _enteredFunction = false;
}

void UserTimeCounter::onBreakpointHit(void* address)
{
   for (auto breakpoint : _breakpoints)
   {
      if (breakpoint->getAddress() == address)
      {
         breakpoint->undo();
      }
   }

   if (address == _functionAddress)
   {
      _functionReturnAddress = getCurrentReturnAddress();
      _functionReturnBreakpoint = createBreakpoint(_functionReturnAddress);
      _functionReturnBreakpoint->set();

      onFunctionEntered();
   }
   else if (address == _functionReturnAddress)
   {
      removeBreakpoint(_functionReturnBreakpoint);
      onFunctionReturn();
   }
}

void* UserTimeCounter::getCurrentReturnAddress() const
{
   auto threadContext = getThreadContext();

   RemotePointer<void*> ptr(_process, reinterpret_cast<void*>(threadContext.Esp));
   return *ptr;
}

void UserTimeCounter::removeBreakpoint(BreakpointRef& breakPoint)
{
   _breakpoints.erase(remove(_breakpoints.begin(), _breakpoints.end(), breakPoint), _breakpoints.end());
   breakPoint.reset();
}

CONTEXT UserTimeCounter::getThreadContext() const
{
   CONTEXT context;
   context.ContextFlags = CONTEXT_CONTROL;
   if (!GetThreadContext(_thread, &context))
   {
      throw ThreadContextException();
   }
   return context;
}

BreakpointRef UserTimeCounter::createBreakpoint(void* address)
{
   auto result = make_shared<Breakpoint>(_process, _thread, address);
   _breakpoints.push_back(result);
   return result;
}
