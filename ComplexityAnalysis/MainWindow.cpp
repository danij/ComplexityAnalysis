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
#include "MainWindow.h"
#include "resource.h"
#include <string>

using namespace std;

enum CommandID
{
   ID_RIBBON_OPEN = 0x8001,
   ID_RIBBON_RUN,
   ID_RIBBON_COPY_CLIPBOARD,
   ID_RIBBON_EXPORT_CSV,
   ID_RIBBON_PROGRESS,
   ID_RIBBON_PROGRESS_TIMER,
   ID_RIBBON_COUNT_INSTRUCTIONS,
   ID_RIBBON_COUNT_USER_TIME,
   ID_RIBBON_HELP,
   ID_RESULTS_TAB_CONTROL,
   ID_RESULTS_LIST,
   ID_STATUS_TEXT,
};

enum UserMessage
{
   WM_STOPRUNNING = WM_USER + 1,
   WM_SHOWERROR
};

BEGIN_MESSAGE_MAP(MainWindow, CFrameWndEx)
ON_WM_CREATE()
ON_COMMAND(ID_RIBBON_OPEN, &MainWindow::OnRibbonOpen)
ON_UPDATE_COMMAND_UI(ID_RIBBON_OPEN, &MainWindow::OnUpdateRibbonOpen)
ON_COMMAND(ID_RIBBON_RUN, &MainWindow::OnRibbonRun)
ON_UPDATE_COMMAND_UI(ID_RIBBON_RUN, &MainWindow::OnUpdateRibbonRun)
ON_COMMAND(ID_RIBBON_COPY_CLIPBOARD, &MainWindow::OnRibbonCopyToClipboard)
ON_UPDATE_COMMAND_UI(ID_RIBBON_COPY_CLIPBOARD, &MainWindow::OnUpdateRibbonCopyToClipboard)
ON_COMMAND(ID_RIBBON_EXPORT_CSV, &MainWindow::OnRibbonExportToCsv)
ON_UPDATE_COMMAND_UI(ID_RIBBON_EXPORT_CSV, &MainWindow::OnUpdateRibbonExportToCsv)
ON_COMMAND(ID_RIBBON_COUNT_INSTRUCTIONS, &MainWindow::OnRibbonCountInstructions)
ON_UPDATE_COMMAND_UI(ID_RIBBON_COUNT_INSTRUCTIONS, &MainWindow::OnUpdateRibbonCountInstructions)
ON_COMMAND(ID_RIBBON_COUNT_USER_TIME, &MainWindow::OnRibbonCountUserTime)
ON_UPDATE_COMMAND_UI(ID_RIBBON_COUNT_USER_TIME, &MainWindow::OnUpdateRibbonCountUserTime)
ON_COMMAND(ID_RIBBON_HELP, &MainWindow::OnRibbonHelp)
ON_UPDATE_COMMAND_UI(ID_STATUS_TEXT, &MainWindow::OnUpdateStatusText)
ON_WM_TIMER()
ON_WM_GETMINMAXINFO()
ON_WM_SIZE()
ON_MESSAGE(WM_STOPRUNNING, &MainWindow::OnStopRunning)
ON_MESSAGE(WM_SHOWERROR, &MainWindow::OnShowError)
END_MESSAGE_MAP()

MainWindow::MainWindow() : _uiThread(GetCurrentThreadId()), _isProgressTimerActive(false), _state(ExecutableNotSelected)
{
   Create(nullptr, L"Practical Complexity Analysis", WS_OVERLAPPEDWINDOW, RECT{0, 0, 800, 600});
   ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
   CenterWindow();
   SetIcon(AfxGetApp()->LoadIconW(IDI_ICON), FALSE);
   _presenter.setView(this);
}

MainWindow::~MainWindow()
{
}

void MainWindow::showError(const CString& value)
{
   if (GetCurrentThreadId() == _uiThread)
   {
      MessageBox(value, L"Error", MB_OK | MB_ICONERROR);
   }
   else
   {
      SendMessage(WM_SHOWERROR, reinterpret_cast<WPARAM>(&value));
   }
}

void changeColumnName(CMFCListCtrl& ctrl, int index, WCHAR* value)
{
   LVCOLUMNW column{};
   column.mask = LVCF_TEXT;
   ctrl.GetColumn(index, &column);
   column.pszText = value;
   ctrl.SetColumn(index, &column);
}

void MainWindow::showSelectedExecutable(const CString& value)
{
   _statusBar.SetPaneText(0, L"Click run to start gathering");
   _resultTabs.SetTabLabel(0, value);
   changeColumnName(_resultsList, 1, L"Result");
   _resultsList.DeleteAllItems();

   _state = ExecutableSelected;
}

void MainWindow::showStartRunning()
{
   if (_isProgressTimerActive)
   {
      return;
   }
   _statusBar.SetPaneText(0, L"Running the application...");
   _resultsList.DeleteAllItems();
      
   switch (_presenter.getAnalysisMode())
   {
   case CountInstructions:
      changeColumnName(_resultsList, 1, L"Instruction Count");
      break;
   case CountUserTime:
      changeColumnName(_resultsList, 1, L"Duration (µs)");
      break;
   default:
      changeColumnName(_resultsList, 1, L"Result");
      break;
   }

   _isProgressTimerActive = true;
   _progressBar->SetPos(0);
   _state = Running;

   SetTimer(ID_RIBBON_PROGRESS_TIMER, 40, nullptr);
}

void MainWindow::showStopRunning(bool errorOccured)
{
   if (GetCurrentThreadId() == _uiThread)
   {
      if (!_isProgressTimerActive)
      {
         return;
      }
      _isProgressTimerActive = false;
      KillTimer(ID_RIBBON_PROGRESS_TIMER);
      _progressBar->SetPos(0);

      if (errorOccured)
      {
         _statusBar.SetPaneText(0, L"Error occured");
         _state = ExecutableSelected;
      }
      else
      {
         _statusBar.SetPaneText(0, L"Gathering complete");
         _state = ResultsAvailable;
      }
   }
   else
   {
      SendMessage(WM_STOPRUNNING, reinterpret_cast<WPARAM>(&errorOccured));
   }
}

void MainWindow::addNewResult(const pair<uint32_t, uint64_t>& value)
{
   auto index = _resultsList.GetItemCount();
   _resultsList.InsertItem(index, to_wstring(value.first).c_str());
   _resultsList.SetItemText(index, 1, to_wstring(value.second).c_str());
}

int MainWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
      return -1;
   
   _ribbonBar.Create(this);
   _ribbonBar.SetApplicationButton(new CMFCRibbonApplicationButton(IDB_RIBBON_ICON), CSize(48, 48));
   
   auto category = _ribbonBar.AddCategory(L"General", IDB_RIBBON_ICONS_16, IDB_RIBBON_ICONS_32);
   
   auto operationsPanel = category->AddPanel(L"Operations");
   operationsPanel->Add(new CMFCRibbonButton(ID_RIBBON_OPEN, L"Open", -1, 0));
   operationsPanel->Add(new CMFCRibbonButton(ID_RIBBON_RUN, L"Start", -1, 1));
   operationsPanel->AddSeparator();
   operationsPanel->Add(new CMFCRibbonButton(ID_RIBBON_COPY_CLIPBOARD, L"Copy to Clipboard", 2, -1));
   operationsPanel->Add(new CMFCRibbonButton(ID_RIBBON_EXPORT_CSV, L"Export to CSV", 3, -1));
   operationsPanel->Add(_progressBar = new CMFCRibbonProgressBar(ID_RIBBON_PROGRESS, 90 + 28));
   
   auto settingsPanel = category->AddPanel(L"Settings");
   settingsPanel->Add(new CMFCRibbonButton(ID_RIBBON_COUNT_INSTRUCTIONS, L"Count Instructions", -1, 4));
   settingsPanel->Add(new CMFCRibbonButton(ID_RIBBON_COUNT_USER_TIME, L"Count User Time", -1, 5));

   auto& images = category->GetSmallImages();
   _ribbonBar.AddToTabs(new CMFCRibbonButton(ID_RIBBON_HELP, L"Help", images.ExtractIconW(6)));

   _progressBar->SetInfiniteMode(TRUE);
   _progressBar->SetRange(0, 100000);

   _statusBar.Create(this);
   UINT indicators[] = { 0 };
   _statusBar.SetIndicators(indicators, 1);
   _statusBar.SetPaneInfo(0, ID_STATUS_TEXT, SBPS_STRETCH, 200);
   _statusBar.SetPaneText(0, L"Select an executable to begin");
   
   _resultTabs.Create(CMFCTabCtrl::STYLE_3D_ROUNDED, CRect(0, 0, 300, 300), this, ID_RESULTS_TAB_CONTROL);
   _resultTabs.SetLocation(CMFCBaseTabCtrl::LOCATION_TOP);

   _resultsList.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL, CRect(10, 10, 200, 200), &_resultTabs, 1230);
   _resultsList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   _resultsList.InsertColumn(0, L"Input", LVCFMT_LEFT, 100);
   _resultsList.InsertColumn(1, L"Result", LVCFMT_RIGHT, 100);
   _resultsList.GetHeaderCtrl().ModifyStyle(HDS_BUTTONS, 0);

   _resultTabs.AddTab(&_resultsList, L"<no executable selected>", 0);
   
   return 0;
}

void MainWindow::OnRibbonOpen()
{
   CFileDialog dialog(TRUE, L"exe", nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"Executables (*.exe)|*.exe||", this);
   dialog.m_pOFN->lpstrTitle = L"Select Executable";
   if (dialog.DoModal() == IDOK)
   {
      _presenter.openExecutable(dialog.GetPathName());
   }
}

void MainWindow::OnUpdateRibbonOpen(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(_state != Running);
}

void MainWindow::OnRibbonRun()
{
   _presenter.runCurrentExecutable();
}

void MainWindow::OnUpdateRibbonRun(CCmdUI* pCmdUI)
{
   pCmdUI->Enable((_state == ExecutableSelected) || (_state == ResultsAvailable));
}

void MainWindow::OnRibbonCopyToClipboard()
{
   _presenter.copyResultsToClipboard();
}

void MainWindow::OnUpdateRibbonCopyToClipboard(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(_state == ResultsAvailable);
}

void MainWindow::OnRibbonExportToCsv()
{
   CFileDialog dialog(FALSE, L"csv", nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"Comma Separated Values|*.csv|All Files|*.*||", this);
   dialog.m_pOFN->lpstrTitle = L"Select Results File";
   if (dialog.DoModal() == IDOK)
   {
      _presenter.exportResultsToCsv(dialog.GetPathName());
   }
}

void MainWindow::OnUpdateRibbonExportToCsv(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(_state == ResultsAvailable);
}

void MainWindow::OnRibbonCountInstructions()
{
   _presenter.setAnalysisMode(CountInstructions);
}

void MainWindow::OnRibbonCountUserTime()
{
   _presenter.setAnalysisMode(CountUserTime);
}

void MainWindow::OnUpdateRibbonCountInstructions(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(_state != Running);
   pCmdUI->SetCheck(_presenter.getAnalysisMode() == CountInstructions);
}

void MainWindow::OnUpdateRibbonCountUserTime(CCmdUI *pCmdUI)
{
   pCmdUI->Enable(_state != Running);
   pCmdUI->SetCheck(_presenter.getAnalysisMode() == CountUserTime);
}

void MainWindow::OnUpdateStatusText(CCmdUI* pCmdUI)
{
   pCmdUI->Enable(TRUE);
}

void MainWindow::OnRibbonHelp()
{
   _presenter.showHelp(GetSafeHwnd());
}

void MainWindow::OnTimer(UINT_PTR nIDEvent)
{
   if (ID_RIBBON_PROGRESS_TIMER == nIDEvent)
   {
      _progressBar->SetPos(_progressBar->GetPos() + 1);
      return;
   }

   CFrameWndEx::OnTimer(nIDEvent);
}

void MainWindow::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
   lpMMI->ptMinTrackSize.x = 500;
   lpMMI->ptMinTrackSize.y = 400;

   CFrameWndEx::OnGetMinMaxInfo(lpMMI);
}

void MainWindow::OnSize(UINT nType, int cx, int cy)
{
   CFrameWndEx::OnSize(nType, cx, cy);

   if (_ribbonBar.GetSafeHwnd() && _statusBar.GetSafeHwnd() && _resultTabs.GetSafeHwnd())
   {
      CRect tabsRect, windowRect, ribbonRect, statusRect;
      GetClientRect(&windowRect);
      _ribbonBar.GetClientRect(&ribbonRect);
      _statusBar.GetClientRect(&statusRect);

      tabsRect.left = ribbonRect.left;
      tabsRect.right = ribbonRect.right;
      tabsRect.top = ribbonRect.bottom;
      tabsRect.bottom = tabsRect.top + windowRect.Height() - ribbonRect.Height() - statusRect.Height();

      _resultTabs.SetWindowPos(nullptr, tabsRect.left,
         tabsRect.top, tabsRect.Width(), tabsRect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);

      CRect listRect;
      _resultsList.GetClientRect(&listRect);

      auto nrColumns = _resultsList.GetHeaderCtrl().GetItemCount();
      for (int i = 0; i < nrColumns; i++)
      {
         _resultsList.SetColumnWidth(i, listRect.Width() / nrColumns - 20);
      }
      _resultsList.Invalidate();
   }
}

LRESULT MainWindow::OnStopRunning(WPARAM wParam, LPARAM lParam)
{
   showStopRunning(*reinterpret_cast<bool*>(wParam));
   return 0;
}

LRESULT MainWindow::OnShowError(WPARAM wParam, LPARAM lParam)
{
   showError(*reinterpret_cast<CString*>(wParam));
   return 0;
}
