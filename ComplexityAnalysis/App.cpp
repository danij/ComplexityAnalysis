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

#include "App.h"
#include "MainWindow.h"

BOOL CMyApp::InitInstance()
{
   CoInitialize(nullptr);
   InitCommonControls();

   CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));

   auto mainWindow = new MainWindow();

   m_pMainWnd = mainWindow;
   mainWindow->ShowWindow(SW_NORMAL);
   mainWindow->UpdateWindow();

   return true;
}

int CMyApp::ExitInstance()
{
   CoUninitialize();
   return 0;
}
