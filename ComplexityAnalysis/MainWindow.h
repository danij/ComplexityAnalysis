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
#include "Presenter.h"

class MainWindow : public CFrameWndEx, public IView
{
public:
   MainWindow();
   virtual ~MainWindow();

   void showError(const CString& value) override;
   void showSelectedExecutable(const CString& value) override;
   void showStartRunning() override;
   void showStopRunning(bool errorOccured) override;
   void addNewResult(const std::pair<uint32_t, uint64_t>& value) override;

private:
   CMFCRibbonBar _ribbonBar;
   CMFCStatusBar _statusBar;
   CMFCListCtrl  _resultsList;
   CMFCTabCtrl _resultTabs;
   CMFCRibbonProgressBar* _progressBar;

   DWORD _uiThread;
   Presenter _presenter;
   bool _isProgressTimerActive;

   enum State
   {
      ExecutableNotSelected,
      ExecutableSelected,
      Running,
      ResultsAvailable
   };
   State _state;
   
   DECLARE_MESSAGE_MAP()
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnRibbonOpen();
   afx_msg void OnUpdateRibbonOpen(CCmdUI *pCmdUI);
   afx_msg void OnRibbonRun();
   afx_msg void OnUpdateRibbonRun(CCmdUI *pCmdUI);
   afx_msg void OnRibbonCopyToClipboard();
   afx_msg void OnUpdateRibbonCopyToClipboard(CCmdUI *pCmdUI);
   afx_msg void OnRibbonExportToCsv();
   afx_msg void OnUpdateRibbonExportToCsv(CCmdUI *pCmdUI);
   afx_msg void OnRibbonCountInstructions();
   afx_msg void OnUpdateRibbonCountInstructions(CCmdUI *pCmdUI);
   afx_msg void OnRibbonCountUserTime();
   afx_msg void OnUpdateRibbonCountUserTime(CCmdUI *pCmdUI);
   afx_msg void OnRibbonHelp();
   afx_msg void OnUpdateStatusText(CCmdUI *pCmdUI);
   afx_msg void OnTimer(UINT_PTR nIDEvent);
   afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg LRESULT OnStopRunning(WPARAM wParam, LPARAM lParam);
   afx_msg LRESULT OnShowError(WPARAM wParam, LPARAM lParam);
};
