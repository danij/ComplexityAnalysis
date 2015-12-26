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
#include "Presenter.h"
#include "DebugProcess.h"
#include <DebugLoop.h>
#include <SymbolCollector.h>
#include <InstructionCounter.h>
#include <Exceptions.h>
#include <UserTimeCounter.h>

using namespace std;

Presenter::Presenter(): _analysisMode(CountInstructions), _view(nullptr)
{
}

Presenter::~Presenter()
{
}

void Presenter::setView(IView* value)
{
   _view = value;
}

bool fileExists(LPCWSTR fileName)
{
   auto attr = GetFileAttributes(fileName);
   return !((INVALID_FILE_ATTRIBUTES == attr) || (attr & FILE_ATTRIBUTE_DIRECTORY));
}

void Presenter::openExecutable(const CString& fileName)
{
   if ( ! fileExists(fileName))
   {
      _view->showError(L"The selected executable does not exist");
      return;
   }
   _currentExecutable = fileName;
   _view->showSelectedExecutable(fileName);
}

void Presenter::runCurrentExecutable()
{
   if (!fileExists(_currentExecutable))
   {
      _view->showError(L"The selected executable does not exist");
      return;
   }

   _results.clear();
   _view->showStartRunning();

   thread([&]()
   {     
      const char* functionName = "functionToMonitor";
      const WCHAR* processName = _currentExecutable;

      DebugProcess process(processName);
      bool errorOccured = false;

      try
      {
         DebugLoop loop;
   
         auto symbolCollector = make_shared<SymbolCollector>();
         shared_ptr<IResultCollector> counter;
         if (CountUserTime == _analysisMode)
         {
            counter = make_shared<UserTimeCounter>(functionName, symbolCollector);
         }
         else
         {
            counter = make_shared<InstructionCounter>(functionName, symbolCollector);
         }

         counter->setNewResultHandler([&](const pair<uint32_t, uint64_t>& value)
         {
            _view->addNewResult(value);
         });
   
         process.create();
   
         loop.addHandler(symbolCollector, 100);
         loop.addHandler(counter, 200);
   
         loop.perform();
         process.waitForExit();
   
         _results = counter->getResults();
      }
      catch(const WinApiException& ex)
      {
         errorOccured = true;
         process.terminate(1);
         _view->showError(L"An error occurred: " + CString(ex.what()));
      }
      catch(const FunctionNotFoundException& ex)
      {
         errorOccured = true;
         process.terminate(1);
         _view->showError(CString(L"The required function ('") + 
            CString(ex.functionName()) + CString(L"') was not found in the executable."));
      }
      catch (...)
      {
         errorOccured = true;
         process.terminate(1);
         _view->showError(L"Unknown error");
      }
      
      _view->showStopRunning(errorOccured);
   }).detach();   
}

void Presenter::copyResultsToClipboard()
{
   if ( ! _results.size())
   {
      _view->showError(L"No results available");
      return;
   }
   wstringstream stream;

   stream << L"Input\t";
   switch (_analysisMode)
   {
   case CountInstructions:
      stream << L"Instruction Count";
      break;
   case CountUserTime:
      stream << L"Duration (us)";
      break;
   default:
      stream << L"Result";
      break;
   }
   stream << L"\r\n";

   for (auto& pair : _results)
   {
      stream << pair.first << L'\t' << pair.second << L"\r\n";
   }
   auto string = stream.str();

   auto count = (string.size() + 1) * sizeof(string[0]);
   auto handle = GlobalAlloc(GMEM_MOVEABLE, count);
   auto buffer = GlobalLock(handle);
   
   ZeroMemory(buffer, count);
   memcpy(buffer, string.c_str(), count);
   GlobalUnlock(handle);

   OpenClipboard(nullptr);
   EmptyClipboard();
   SetClipboardData(CF_UNICODETEXT, handle);
   CloseClipboard();
}

void Presenter::exportResultsToCsv(const CString& fileName)
{
   if (!_results.size())
   {
      _view->showError(L"No results available");
      return;
   }
   ofstream stream(fileName);
   if ( ! stream)
   {
      _view->showError(CString(L"Could not open ") + fileName);
      return;
   }
   stream << "Input,";
   switch (_analysisMode)
   {
   case CountInstructions:
      stream << "Instruction Count";
      break;
   case CountUserTime:
      stream << "Duration (us)";
      break;
   default:
      stream << "Result";
      break;
   }
   stream << "\n";
   for (auto& pair : _results)
   {
      stream << pair.first << ',' << pair.second << "\n";
   }
}

void Presenter::setAnalysisMode(AnalysisMode value)
{
   _analysisMode = value;
}

AnalysisMode Presenter::getAnalysisMode() const
{
   return _analysisMode;
}

void Presenter::showHelp(HWND hWnd)
{
   ShellExecute(hWnd, L"Open", L"https://github.com/danij/ComplexityAnalysis", L"", L"", SW_SHOW);
}
