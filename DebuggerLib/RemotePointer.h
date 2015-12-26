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
#include "Exceptions.h"

template<typename T>
class RemotePointerRef;

template<typename T>
class RemotePointer
{
public:
   RemotePointer(HANDLE process, void* pointer) : _process(process), _pointer(pointer) { }
   
   RemotePointerRef<T> operator*() const
   {
      return RemotePointerRef<T>(_process, _pointer);
   }

private:
   HANDLE _process;
   void* _pointer;
};

template<typename T>
class RemotePointerRef
{
public:
   RemotePointerRef(HANDLE process, void* pointer) : _process(process), _pointer(pointer) { }
   RemotePointerRef& operator=(const RemotePointerRef<T>&) = delete;

   operator T() const
   {
      SIZE_T read;
      T result;

      ReadProcessMemory(_process, _pointer, &result, sizeof(T), &read);
      if (read < sizeof(T))
      {
         throw MemoryReadException();
      }
      return result;
   }

   RemotePointerRef& operator=(const T& value)
   {
      SIZE_T written;
      WriteProcessMemory(_process, _pointer, &value, sizeof(T), &written);
      if (written < sizeof(T))
      {
         throw MemoryWriteException();
      }
      return *this;
   }

   friend class RemotePointer<T>;

private:
   RemotePointerRef(const RemotePointerRef<T>&) = default;

   HANDLE _process;
   void* _pointer;
};
