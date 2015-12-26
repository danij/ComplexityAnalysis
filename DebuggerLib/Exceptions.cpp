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
#include "Exceptions.h"

WinApiException::WinApiException()
{
   _lastError = GetLastError();
   WCHAR buffer[10000];

   auto written = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
      _lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer) / sizeof(buffer[0]), nullptr);

   char utf8Buffer[10000];

   if (written)
   {
      written = WideCharToMultiByte(CP_UTF8, 0, buffer, written, utf8Buffer, sizeof(utf8Buffer) / sizeof(utf8Buffer[0]), nullptr, nullptr);
      utf8Buffer[written] = 0;
   }

   //trim newline at end of string
   auto ptr = utf8Buffer + strlen(utf8Buffer) - 1;
   while (*ptr == '\r' || *ptr == '\n')
   {
      *ptr = 0;
      --ptr;
   }

   _lastErrorString = utf8Buffer;
}

DWORD WinApiException::getLastError() const
{
   return _lastError;
}

char const* WinApiException::what() const
{
   return _lastErrorString.c_str();
}

FunctionNotFoundException::FunctionNotFoundException(const char* functionName) : _functionName(functionName)
{
}

char const* FunctionNotFoundException::what() const
{
   return _functionName.c_str();
}

const char* FunctionNotFoundException::functionName() const
{
   return _functionName.c_str();
}
