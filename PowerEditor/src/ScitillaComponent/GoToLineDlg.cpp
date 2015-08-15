// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid
// misunderstandings, we consider an application to constitute a
// "derivative work" for the purpose of this license if it does any of the
// following:
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "GoToLineDlg.h"

bool optRememberLastPos = true,
     optUseCurrentPos = false,
     optKeepDialogOpen = false,
     optKeepDialogOnTop = false;

INT_PTR CALLBACK GoToLineDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTOLINE, BM_SETCHECK, TRUE, 0);
			::SendDlgItemMessage(_hSelf, IDC_RADIO_GOTO_OPTREMEMBER, BM_SETCHECK, optRememberLastPos, 0);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_GOTO_KEEPONTOP), optKeepDialogOpen); //because OnTop works only with KeepOpen
			goToCenter();
			return TRUE;
		}
		case WM_COMMAND: 
		{
			switch (wParam)
			{
				case IDCANCEL: //Close
				{
					display(false);
					if (not optRememberLastPos and not optUseCurrentPos)
						cleanLineEdit();
					return TRUE;
				}
				case IDOK:
				{
					int line = getLine();
					if (line != -1)
					{
						if (not optKeepDialogOpen)
							display(false);

						if (not optRememberLastPos and not optUseCurrentPos)
							cleanLineEdit();

						if (_mode == go2line)
						{
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, line-1);
							(*_ppEditView)->execute(SCI_GOTOLINE, line-1);
						}
						else
						{
							int sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, line);
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
							(*_ppEditView)->execute(SCI_GOTOPOS, line);
						}
					}

					// find hotspots
					/*
					NMHDR nmhdr;
					nmhdr.code = SCN_PAINTED;
					nmhdr.hwndFrom = _hSelf;
					nmhdr.idFrom = ::GetDlgCtrlID(nmhdr.hwndFrom);
					::SendMessage(_hParent, WM_NOTIFY, (WPARAM)LINKTRIGGERED, (LPARAM)&nmhdr);
					*/

					SCNotification notification = {};
					notification.nmhdr.code = SCN_PAINTED;
					notification.nmhdr.hwndFrom = _hSelf;
					notification.nmhdr.idFrom = ::GetDlgCtrlID(_hSelf);
					::SendMessage(_hParent, WM_NOTIFY, (WPARAM)LINKTRIGGERED, (LPARAM)&notification);

					if (not optKeepDialogOnTop)
						(*_ppEditView)->getFocus();

					if (optKeepDialogOnTop and optKeepDialogOpen)
						display(); //auto focus on input field

					return TRUE;
				}
				case IDC_CHECK_GOTO_KEEPOPEN:
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_GOTO_KEEPOPEN, BM_GETCHECK, 0, 0));
					optKeepDialogOpen = isChecked;
					::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_GOTO_KEEPONTOP), optKeepDialogOpen); //because OnTop works only with KeepOpen
					return TRUE;
				}
				case IDC_CHECK_GOTO_KEEPONTOP:
				{
					bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_CHECK_GOTO_KEEPONTOP, BM_GETCHECK, 0, 0));
					optKeepDialogOnTop = isChecked;
					return TRUE;
				}
				case IDC_RADIO_GOTO_OPTREMEMBER:
				case IDC_RADIO_GOTO_OPTUSEPOSITION:
				case IDC_RADIO_GOTO_OPTDEFAULT:
				{
					if (wParam == IDC_RADIO_GOTO_OPTREMEMBER)
					{
						optRememberLastPos = true;
						optUseCurrentPos = false;
					}
					else if (wParam == IDC_RADIO_GOTO_OPTUSEPOSITION)
					{
						optRememberLastPos = false;
						optUseCurrentPos = true;
					}
					else //old default
					{
						optRememberLastPos = false;
						optUseCurrentPos = false;
					}
					updateLinesNumbers();
					return TRUE;
				}
				case IDC_RADIO_GOTOLINE:
				case IDC_RADIO_GOTOOFFSET:
				{
					_mode = (wParam == IDC_RADIO_GOTOLINE) ? go2line : go2offsset;
					updateLinesNumbers();
					return TRUE;
				}
				default:
				{
					switch (HIWORD(wParam))
					{
						case EN_SETFOCUS:
						case BN_SETFOCUS:
						{
							updateLinesNumbers();
							return TRUE;
						}
						default:
							return TRUE;
					}
					break;
				}
			}
		}
		case WM_ACTIVATE:
		{
			if (wParam != WA_INACTIVE) //active states
				display();

			return TRUE;
		}
		default:
			return FALSE;
	}
}

void GoToLineDlg::updateLinesNumbers() const 
{
	unsigned int current = 0;
	unsigned int limit = 0;
	
	if (_mode == go2line)
	{
		current = (unsigned int)((*_ppEditView)->getCurrentLineNumber() + 1);
		limit = (unsigned int)((*_ppEditView)->execute(SCI_GETLINECOUNT));
	}
	else
	{
		current = (unsigned int)((*_ppEditView)->execute(SCI_GETCURRENTPOS));
		limit = (unsigned int)((*_ppEditView)->execute(SCI_GETLENGTH));
	}

	::SetDlgItemInt(_hSelf, ID_CURRLINE, current, FALSE);
	::SetDlgItemInt(_hSelf, ID_LASTLINE, limit, FALSE);
	if (optUseCurrentPos)
		::SetDlgItemInt(_hSelf, ID_GOLINE_EDIT, current, FALSE);
}
