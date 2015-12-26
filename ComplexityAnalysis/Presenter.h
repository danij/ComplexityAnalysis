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
#include "IView.h"

enum AnalysisMode
{
   CountInstructions,
   CountUserTime
};

class Presenter
{
public:
   Presenter();
   virtual ~Presenter();

   void setView(IView* value);
   void openExecutable(const CString& fileName);
   void runCurrentExecutable();
   void copyResultsToClipboard();
   void exportResultsToCsv(const CString& fileName);
   void setAnalysisMode(AnalysisMode value);
   AnalysisMode getAnalysisMode() const;
   void showHelp(HWND hWnd);

protected:
   AnalysisMode _analysisMode;
   CString _currentExecutable;
   IView* _view;
   std::map<uint32_t, uint64_t> _results;
};

