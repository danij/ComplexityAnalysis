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

#pragma once

#include "stdafx.h"
#include "SymbolCollector.h"
#include "Breakpoint.h"
#include "IResultCollector.h"

class InstructionCounter : public IResultCollector
{
public:
   explicit InstructionCounter(const char* functionName, SymbolCollectorRef symbolCollector);

   void handle(const DEBUG_EVENT& event) override;

   const std::map<uint32_t, uint64_t>& getResults() const override;
   void setNewResultHandler(const std::function<void(const std::pair<uint32_t, uint64_t>&)>& value) override;

private:
   void onCreateProcess(const CREATE_PROCESS_DEBUG_INFO& info);
   void onFunctionEntered();
   void onFunctionReturn();
   void onBreakpointHit(void* address);
   void onStepByStep(void* address);
   void* getCurrentReturnAddress() const;
   void removeBreakpoint(BreakpointRef& breakPoint);
   CONTEXT getThreadContext() const;
   void setThreadContext(const CONTEXT& context);
   bool isAddressInCurrentExecutable(void* address);
   BreakpointRef createBreakpoint(void* address);

   std::string _functionName;
   SymbolCollectorRef _symbolCollector;
   void* _functionAddress;
   void* _functionReturnAddress;
   void* _externalFunctionReturnAddress;
   HANDLE _process;
   HANDLE _thread;
   bool _enteredFunction;

   BreakpointRef _functionEnterBreakpoint;
   BreakpointRef _functionReturnBreakpoint;
   BreakpointRef _externalFunctionReturnBreakpoint;
   std::vector<BreakpointRef> _breakpoints;

   uint32_t _currentInput;
   size_t _currentNrOfInstructions;
   std::map<uint32_t, uint64_t> _instructionCount;
   std::function<void(const std::pair<uint32_t, uint64_t>&)> _newResultHandler;
   std::vector<std::pair<void*, size_t>> _addressCache;
};
