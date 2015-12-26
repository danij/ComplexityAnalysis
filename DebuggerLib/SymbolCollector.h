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
#include "DebugLoop.h"

struct FunctionSymbol
{
   void* address;
   size_t size;
};

class SymbolCollector : public IDebugEventHandler
{
public:
   SymbolCollector();
   ~SymbolCollector();

   virtual void handle(const DEBUG_EVENT& event) override;

   const std::map<std::string, const FunctionSymbol>& getSymbols() const;

private:
   std::map<std::string, const FunctionSymbol> _symbols;
   HANDLE _process;
   DWORD64 _symbolBaseAddress;
   ULONG _imageBaseAddress;
};

typedef std::shared_ptr<SymbolCollector> SymbolCollectorRef;
