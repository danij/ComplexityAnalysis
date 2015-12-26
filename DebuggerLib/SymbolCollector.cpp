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
#include <Dia2.h>
#include "SymbolCollector.h"
#include "Exceptions.h"

using namespace std;

SymbolCollector::SymbolCollector(): _process(nullptr), _symbolBaseAddress(0), _imageBaseAddress(0)
{
}

SymbolCollector::~SymbolCollector()
{
}

void SymbolCollector::handle(const DEBUG_EVENT& event)
{
   if (EXIT_PROCESS_DEBUG_EVENT == event.dwDebugEventCode)
   {
      SymCleanup(_process);
      return;
   }

   if (CREATE_PROCESS_DEBUG_EVENT != event.dwDebugEventCode)
   {
      return;
   }

   auto& processInfo = event.u.CreateProcessInfo;
   _process = processInfo.hProcess;

   if ( ! SymInitialize(_process, nullptr, false))
   {
      throw SymbolLoadException();
   }

   _symbolBaseAddress = SymLoadModule64(_process, processInfo.hFile, nullptr, nullptr, 0, 0);
   _imageBaseAddress = reinterpret_cast<ULONG>(processInfo.lpBaseOfImage);

   IMAGEHLP_MODULE64 module_info{};
   module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

   auto bSuccess = SymGetModuleInfo64(_process, _symbolBaseAddress, &module_info);
   if (bSuccess && module_info.SymType == SymPdb)
   {
      SymEnumSymbols(_process, _symbolBaseAddress, "*", [](
         _In_     PSYMBOL_INFO pSymInfo,
         _In_     ULONG        SymbolSize,
         _In_opt_ PVOID        UserContext)
      {
         if (SymTagFunction != pSymInfo->Tag)
         {
            return TRUE;
         }

         auto _this = static_cast<SymbolCollector*>(UserContext);

         _this->_symbols.insert(make_pair(string(pSymInfo->Name), FunctionSymbol
         {
            reinterpret_cast<void*>(pSymInfo->Address - _this->_symbolBaseAddress + _this->_imageBaseAddress),
            pSymInfo->Size
         }));

         return TRUE;
      }, this);      
   }
}

const map<string, const FunctionSymbol>& SymbolCollector::getSymbols() const
{
   return _symbols;
}
