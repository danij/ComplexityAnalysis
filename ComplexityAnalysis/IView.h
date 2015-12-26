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

class IView
{
public:
   virtual ~IView() = default;

   virtual void showError(const CString& value) = 0;
   virtual void showSelectedExecutable(const CString& value) = 0;
   virtual void showStartRunning() = 0;
   virtual void showStopRunning(bool errorOccured) = 0;
   virtual void addNewResult(const std::pair<uint32_t, uint64_t>& value) = 0;
};
