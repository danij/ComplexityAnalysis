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

class WinApiException : public std::exception
{
public:
   WinApiException();

   DWORD getLastError() const;

   char const* what() const override;

protected:
   DWORD _lastError;
   std::string _lastErrorString;
};

class ProcessCreateFailedException : public WinApiException
{   
};

class MemoryReadException : public WinApiException
{
};

class MemoryWriteException : public WinApiException
{
};

class SymbolLoadException : public WinApiException
{   
};

class ThreadContextException : public WinApiException
{
};

class FunctionNotFoundException : public std::exception
{
public:
   FunctionNotFoundException(const char* functionName);

   virtual char const* what() const override;
   virtual const char* functionName() const;

protected:
   std::string _functionName;
};
