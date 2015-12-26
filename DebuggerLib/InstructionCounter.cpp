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
#include "InstructionCounter.h"
#include "Exceptions.h"
#include "RemotePointer.h"

using namespace std;

InstructionCounter::InstructionCounter(const char* functionName, SymbolCollectorRef symbolCollector)
   : _functionName(functionName), _symbolCollector(symbolCollector), _functionAddress(nullptr), 
   _functionReturnAddress(nullptr), _externalFunctionReturnAddress(nullptr), _process(nullptr), _thread(nullptr), 
   _enteredFunction(false), _currentInput(0), _currentNrOfInstructions(0)
{
}

void InstructionCounter::handle(const DEBUG_EVENT& event)
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
      else if (EXCEPTION_SINGLE_STEP == event.u.Exception.ExceptionRecord.ExceptionCode)
      {
         onStepByStep(event.u.Exception.ExceptionRecord.ExceptionAddress);
      }
      break;
   default:
      break;
   }
}

const map<uint32_t, uint64_t>& InstructionCounter::getResults() const
{
   return _instructionCount;
}

void InstructionCounter::setNewResultHandler(const function<void(const pair<uint32_t, uint64_t>&)>& value)
{
   _newResultHandler = value;
}

void InstructionCounter::onCreateProcess(const CREATE_PROCESS_DEBUG_INFO& info)
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

   for (auto& pair : symbols)
   {
      _addressCache.push_back(make_pair(pair.second.address, pair.second.size));
   }
   sort(_addressCache.begin(), _addressCache.end(), [](const pair<void*, size_t>& first, const pair<void*, size_t>& second)
   {
      return first.first < second.first;
   });
}

void InstructionCounter::onFunctionEntered()
{
   _currentNrOfInstructions = 0;

   auto threadContext = getThreadContext();

   RemotePointer<uint32_t> currentInputPtr(_process, reinterpret_cast<void*>(threadContext.Esp + 4));
   _currentInput = *currentInputPtr;
   
   //enable single-step execution
   threadContext.EFlags |= 0x100; //trap flag for single-step execution
   setThreadContext(threadContext);

   _enteredFunction = true;
}

void InstructionCounter::onFunctionReturn()
{
   auto it = _instructionCount.find(_currentInput);
   if (it != _instructionCount.end())
   {
      _instructionCount.erase(it);
   }

   auto pair = make_pair(_currentInput, _currentNrOfInstructions);
   _instructionCount.insert(pair);
   if (_newResultHandler)
   {
      _newResultHandler(pair);
   }
   _currentNrOfInstructions = 0;
   _currentInput = 0;

   //function might be called again
   _functionEnterBreakpoint->set();
   _enteredFunction = false;
}

void InstructionCounter::onBreakpointHit(void* address)
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
   else if (address == _externalFunctionReturnAddress)
   {
      removeBreakpoint(_externalFunctionReturnBreakpoint);
      onStepByStep(address);
   }
}

void InstructionCounter::onStepByStep(void* address)
{
   if (isAddressInCurrentExecutable(address))
   {
      auto threadContext = getThreadContext();
      _currentNrOfInstructions += 1;

      threadContext.EFlags |= 0x100; //re-enable single-step execution
      setThreadContext(threadContext);
   }
   else if (_enteredFunction)
   {
      //skip external function
      _externalFunctionReturnAddress = getCurrentReturnAddress();      
      _externalFunctionReturnBreakpoint = createBreakpoint(_externalFunctionReturnAddress);
      _externalFunctionReturnBreakpoint->set();
   }
}

void* InstructionCounter::getCurrentReturnAddress() const
{
   auto threadContext = getThreadContext();

   RemotePointer<void*> ptr(_process, reinterpret_cast<void*>(threadContext.Esp));
   return *ptr;
}

void InstructionCounter::removeBreakpoint(BreakpointRef& breakPoint)
{
   _breakpoints.erase(remove(_breakpoints.begin(), _breakpoints.end(), breakPoint), _breakpoints.end());
   breakPoint.reset();
}

CONTEXT InstructionCounter::getThreadContext() const
{
   CONTEXT context;
   context.ContextFlags = CONTEXT_CONTROL;
   if ( ! GetThreadContext(_thread, &context))
   {
      throw ThreadContextException();
   }
   return context;
}

void InstructionCounter::setThreadContext(const CONTEXT& context)
{
   if ( ! SetThreadContext(_thread, &context))
   {
      throw ThreadContextException();
   }
}

bool InstructionCounter::isAddressInCurrentExecutable(void* address)
{
   for (auto& pair : _addressCache)
   {
      if (pair.first > address)
      {
         break;
      }
      if (reinterpret_cast<void*>(reinterpret_cast<ULONG>(pair.first)  + pair.second) > address)
      {
         return true;
      }
   }

   //is the current instruction a JMP?
   //if so, allow it and only verify the next address
   const unsigned char jmpCodes[] = { 0xE9, 0xEB, 0xEA, 0xFF };
   
   auto ptr = RemotePointer<unsigned char>(_process, address);
   unsigned char code = *ptr;

   if (find(begin(jmpCodes), end(jmpCodes), code) != end(jmpCodes))
   {
      return true;
   }

   return false;
}

BreakpointRef InstructionCounter::createBreakpoint(void* address)
{
   auto result = make_shared<Breakpoint>(_process, _thread, address);
   _breakpoints.push_back(result);
   return result;
}
