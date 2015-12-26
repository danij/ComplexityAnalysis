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
#include "Breakpoint.h"
#include "RemotePointer.h"

Breakpoint::Breakpoint(HANDLE process, HANDLE thread, void* address) : 
   _process(process), _thread(thread), _address(address), _previousInstruction(0)
{
}

Breakpoint::~Breakpoint()
{
}

void Breakpoint::set()
{
   const UCHAR breakInstruction = 0xcc; //INT 3

   RemotePointer<UCHAR> existingInstructionPtr(_process, _address);

   _previousInstruction = *existingInstructionPtr;
   *existingInstructionPtr = breakInstruction;

   FlushInstructionCache(_process, _address, 1);
}

void Breakpoint::undo()
{
   RemotePointer<UCHAR> existingInstructionPtr(_process, _address);

   *existingInstructionPtr = _previousInstruction;
   FlushInstructionCache(_process, _address, 1);

   CONTEXT threadContext;
   threadContext.ContextFlags = CONTEXT_CONTROL;
   if ( ! GetThreadContext(_thread, &threadContext))
   {
      throw ThreadContextException();
   }

   threadContext.Eip -= 1;
   if ( ! SetThreadContext(_thread, &threadContext))
   {
      throw ThreadContextException();
   }

   FlushInstructionCache(_process, reinterpret_cast<void*>(reinterpret_cast<ULONG>(_address) - 1), 1);
   FlushInstructionCache(_process, _address, 1);
}

void* Breakpoint::getAddress() const
{
   return _address;
}
