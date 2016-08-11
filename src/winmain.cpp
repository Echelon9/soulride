/*
    Copyright 2000, 2001, 2002, 2003 Slingshot Game Technology, Inc.

    This file is part of The Soul Ride Engine, see http://soulride.com

    The Soul Ride Engine is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    The Soul Ride Engine is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
// winmain.cpp	-thatcher 1/28/1998 Copyright Thatcher Ulrich

// WinMain() and supporting stuff, derived from MS DirectX sample code.


#include <windows.h>
#include <windowsx.h>	// ?
#include <commctrl.h>
#include <stdio.h>	// ?
#include <string.h>	// ?
#include <time.h>	// ?
#include <search.h>	// ?
#include <ddraw.h>
#include <dbt.h>
#include <new.h>
#include <windows.h>
//#include <d3d.h>

#include "exceptionhandler.hpp"

#include "resource.h"

#include "main.hpp"
#include "winmain.hpp"
#include "error.hpp"
#include "render.hpp"
#include "gameloop.hpp"
#include "config.hpp"
#include "input.hpp"
#include "sound.hpp"
#include "music.hpp"
#include "timer.hpp"
#include "ui.hpp"


static bool	AppInit(HINSTANCE hInstance, LPSTR lpCmdLine);
long FAR PASCAL	WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK	DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam); 
long FAR PASCAL	GameWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


#define START_WIN_XSIZE	640
#define START_WIN_YSIZE	480

#define MAX_WIN_XSIZE 1024
#define MAX_WIN_YSIZE 768
#define MIN_WIN_XSIZE 320
#define MIN_WIN_YSIZE 240

		
// Charles Simonyi must be punished.

namespace Main {
	HWND	hWndMain = 0;
	HINSTANCE	hInstApp = 0;

	HWND	GameWindow = 0;
};
using namespace Main;


bool	DisplayChanged = false;


HWND	Main::GetMainWindow() { return hWndMain; }
HINSTANCE	Main::GetAppInstance() { return hInstApp; }
HWND	Main::GetGameWindow() { return GameWindow; }


static void	ShowMouseCursor(bool show)
// Turns the cursor on or off as desired.
{
	static bool	Showing = true;
	
	if (show == true && Showing == false) {
		ShowCursor(TRUE);
		Showing = true;
	} else if (show == false && Showing == true) {
		ShowCursor(FALSE);
		Showing = false;
	}
}


// Exception class.
class bad_alloc {
};


int	bad_alloc_handler(size_t s)
{
	bad_alloc	a;
	throw a;

	return 0;
}


static float	MouseX = 0;
static float	MouseY = 0;
static float	MouseSpeedX = 0;
static float	MouseSpeedY = 0;
static int	LastMouseSampleTicks = 0;


static void	MeasureMouseSpeed()
// This function measures the mouse speed input, filters it, and passes
// the results to the Input:: module.
{
	int	ticks = Timer::GetTicks();
	int	dt = ticks - LastMouseSampleTicks;
	float	NewMouseX, NewMouseY;
	int	dummy;
	Input::GetMouseStatus(&NewMouseX, &NewMouseY, &dummy);
	if (dt > 0 &&
	    (dt > 50 || NewMouseX != MouseX || NewMouseY != MouseY))
	{
		float	c0 = expf(-(dt/1000.0f)/0.20f);
		float	msx = (NewMouseX - MouseX) * 1000.0f / float(dt);
		float	msy = (NewMouseY - MouseY) * 1000.0f / float(dt);
		MouseSpeedX = MouseSpeedX * c0 + msx * (1 - c0);
		MouseSpeedY = MouseSpeedY * c0 + msy * (1 - c0);
		Input::NotifyMouseSpeed(MouseSpeedX, MouseSpeedY);
		MouseX = NewMouseX;
		MouseY = NewMouseY;
		LastMouseSampleTicks = ticks;
	}
}


// Turn UTF-8 chars into something SetWindowText() can accept.  Tries
// to convert Polish chars from Unicode to the appropriate windows
// garbage.
static void	UTFToWindows(unsigned char* buf, int buflen, const char* str)
{
	LCID	lcid = GetUserDefaultLCID();

	// Are we in a Polish locale?
	bool	polish = false;
	if ((lcid & 0x0FFFF) == 0x0415)
	{
		polish = true;
	}

	unsigned char*	p = (unsigned char*) str;
	unsigned char*	q = buf;
	while (*p && q < buf + buflen - 1)
	{
		int	c = *p++;
		if (c < 0x80)
		{
			*q++ = c;
		}
		else
		{
			// UTF-8.
			if ((c & 0xE0) == 0xC0)
			{
				unsigned ch1 = *p++;
				if (ch1 == 0
					|| (ch1 & 0xC0) != 0x80)
				{
					// invalid utf-8, but maybe this is valid Latin-1.  Try using ch as-is.
					p--;
					*q++ = c;
				}
				else
				{
					int	unicode = ((int(c) & 0x1F) << 6) | (int(ch1) & 0x3F);

					int	winChar = unicode;

					if (polish)
					{
						// Character translations...
						static struct Trans {
							int	unicode;
							int cp1250;
						} transTable[] = {
							{ 0x104, 165 },
							{ 0x106, 198 },
							{ 0x118, 202 },
							{ 0x141, 163 },
							{ 0x143, 209 },
							{ 0x0D3, 211 },
							{ 0x15A, 140 },
							{ 0x179, 143 },
							{ 0x17B, 175 },
							{ 0x105, 185 },
							{ 0x107, 230 },
							{ 0x119, 234 },
							{ 0x142, 179 },
							{ 0x144, 241 },
							{ 0x0F3, 243 },
							{ 0x15B, 156 },
							{ 0x17A, 159 },
							{ 0x17C, 191 },
							{ 0, 0 }
						};

						for (int i = 0; ; i++)
						{
							if (transTable[i].unicode == 0) break;
							if (transTable[i].unicode == unicode)
							{
								// Match; do the translation.
								winChar = transTable[i].cp1250;
								break;
							}
						}
					}

					*q++ = winChar;					// might trunc/overflow!
				}
			}
			else
			{
				// Invalid, or higher utf-8.  Pretend it's Latin-1 or something.
				*q++ = c;
			}
		}
	}
	*q = 0;
}


// Take a UTF-8 string and try to jam it into the text control
// specified by hwnd.
static void	SetWindowTextUTF(HWND hwnd, const char* str)
{
	const int	BUFLEN = 1000;
	unsigned char	buf[BUFLEN];

	UTFToWindows(buf, BUFLEN, str);

	SetWindowText(hwnd, (const char*) buf);
}


int PASCAL	HandledWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// Windows app entry point.
{
	//#if VC5
	// Set up standard-ish out-of-memory exception behavior.
	_set_new_handler(bad_alloc_handler);
	//#endif

#ifndef NDEBUG
		//  turn on noisy fpu exceptions
#define FE_INEXACT       0x20
#define FE_DIVBYZERO     0x04
#define FE_UNDERFLOW     0x10
#define FE_OVERFLOW      0x08
#define FE_INVALID       0x01

		WORD control_word;
		__asm fstcw control_word;
		__asm and control_word, ~(FE_DIVBYZERO | FE_INVALID)
		__asm fldcw control_word;
#endif // DEBUG
	
	MSG	msg;
	HACCEL hAccelApp;

	bool	CaughtError = false;
	char	ErrorMessage[2000];
	
	try {
		// Initialize subsystems.
		Main::Open(lpCmdLine);

		// Create the main window.
		if (!AppInit(hInstance, lpCmdLine)) return FALSE;

		// Setup keyboard accelerators ?xxxx?
		hAccelApp = LoadAccelerators(hInstance, "AppAccel");

		while (!Main::GetQuit()) {
			// Monitor the message queue until there are no pressing
			// messages
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					Main::SetQuit(true);
					break;
				}

				if (hWndMain && IsDialogMessage(hWndMain, &msg)) {
					// Handled by IsDialogMessage().
				} else if (hWndMain == NULL || !TranslateAccelerator(hWndMain, hAccelApp, &msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				
			// If the app is not paused and the game loop is open, then run an iteration of the game loop.
			} else if (Main::GetPaused() == false && GameLoop::GetIsOpen() == true) {
				// Compute mouse speed and notify Input::
				MeasureMouseSpeed();

				GameLoop::Update();
			} else {
				WaitMessage();
			}
		}
	}
	catch (bad_alloc) {
		ShowCursor(TRUE);
		MessageBox(hWndMain, "Out Of Memory.\r\nTry clearing some space off your C: hard disk", "Soul Ride", MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
		exit(1);
	}
	catch (Error& e) {
		CaughtError = true;
		strcpy(ErrorMessage, e.GetMessage());
	}
//	catch (...) {
//		MessageBox(hWndMain, "Unknown exception caught.", "Soul Ride", MB_OK);
//	}

	// Close subsystems.
	try {
		// Clean up resources, post a quit message.
		if (GameLoop::GetIsOpen()) {
			GameLoop::Close();
		}

		Main::Close();
	}
	catch (Error& e) {
		if (!CaughtError) {
			CaughtError = true;
			strcpy(ErrorMessage, "While closing: ");
			strcat(ErrorMessage, e.GetMessage());
		}
	}
//	catch (...) {
//		MessageBox(hWndMain, "Unknown exception caught in Main::Close().", "Soul Ride", MB_OK);
//	}

	// Show any error.
	if (CaughtError) {
		ShowCursor(TRUE);
		MessageBox(hWndMain, ErrorMessage, "Soul Ride", MB_OK);
	}
	
	// Close the intro window and return.
	PostQuitMessage(0);
	DestroyWindow(hWndMain);

	return msg.wParam;
}


int PASCAL	WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// Wrapper containing Windows structured exception handler.
{
	int Result = -1;
	__try
	{
		Result = HandledWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except(RecordExceptionInfo(GetExceptionInformation(), "main thread"))
	{
//xxxx		// Do nothing here - RecordExceptionInfo() has already done
//		// everything that is needed. Actually this code won't even
//		// get called unless you return EXCEPTION_EXECUTE_HANDLER from
//		// the __except clause.

		// Try to clean up.  May not work, but give it a try anyway.
		if (GameLoop::GetIsOpen()) {
			GameLoop::Close();
		}
		Main::Close();
	}
	return Result;
}


void	Main::AbortError(const char* message)
// Pop up a message box with the error message, and then abort
// the program.
{
	MessageBox(hWndMain, message, "Soul Ride", MB_OK);

	exit(1);
}


static bool	AppInit(HINSTANCE hInstance, LPSTR lpCmdLine)
// Creates the app's main window.
{
	hInstApp = hInstance;

	hWndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_INITIAL_MENU), NULL /* owner window */, (DLGPROC) &DialogProc);
 
	if (!hWndMain) {
		MessageBox(hWndMain, "CreateDialog failed", "Soul Ride", MB_OK);
		return FALSE;
	}

	// Center the window on the screen.
	int	cx = GetSystemMetrics(SM_CXSCREEN) >> 1;
	int	cy = GetSystemMetrics(SM_CYSCREEN) >> 1;
	RECT	rect;
	GetWindowRect(hWndMain, &rect);
	int	w = rect.right - rect.left;
	int	h = rect.bottom - rect.top;
	MoveWindow(hWndMain, cx - w/2, cy - h/2, w, h, FALSE /* repaint */);
 
	
	// Display the window
	ShowWindow(hWndMain, SW_SHOWNORMAL);
	UpdateWindow(hWndMain);

	return TRUE;
}


BOOL FAR PASCAL	AppAbout(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
// About box message handler.
{
	switch (msg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) EndDialog(hwnd, TRUE);
		break;
	case WM_INITDIALOG:
		return TRUE;
	}
	return FALSE;
}


BOOL FAR PASCAL	AppEditDriver(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
// Message handler for dialog box for editing driver properties.
{
	static int	DriverToEdit = 0;
	
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		{
//			// Get the driver name.
//			int	FullscreenChecked = IsDlgButtonChecked(hwnd, IDC_FULLSCREEN);
//			bool	Fullscreen = (FullscreenChecked == BST_CHECKED);
//			
//			// Get the driver comment.
//			HWND	ModeListBox = GetDlgItem(hwnd, IDC_DISPLAYMODE);
//			int	Mode = SendMessage(ModeListBox, LB_GETCURSEL, 0, 0);

			char	dll[1000];
			char	comment[1000];
			GetDlgItemText(hwnd, IDC_DRIVER_DLL_FILE, dll, 1000);
			GetDlgItemText(hwnd, IDC_DRIVER_COMMENT, comment, 1000);

			Render::SetDriverInfo(DriverToEdit, dll, comment);

			// Commit changes.
			EndDialog(hwnd, 1);
			break;
		}

		case IDCANCEL:
			// Exit, without making any changes.
			EndDialog(hwnd, 0);
			break;

		case IDC_BROWSE:
		{
			// Bring up a File Open dialog, to let the user choose a DLL.
			//static const char*	Filter = "DLL files\0*.dll\0";	// @@ localize
			unsigned char	filter[1000];
			UTFToWindows(filter, 500, UI::String("win_dll_files", "DLL files"));
			strcpy((char*) filter + strlen((char*) filter) + 1, "*.dll");
			filter[strlen((char*) filter) + 1] = 0;	// extra terminating \0
				
			char	buf[1000];
			buf[0] = 0;
			
			char	dialogTitle[1000];
			UTFToWindows((unsigned char*) dialogTitle, 1000, UI::String("win_choose_opengl_dll", "Choose an OpenGL .DLL"));

			OPENFILENAME	ofn;
			memset(&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.hInstance = hInstApp;
			ofn.lpstrFilter = (LPCSTR) filter;
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = buf;
			ofn.nMaxFile = 1000;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.lpstrTitle = dialogTitle;
			ofn.Flags = OFN_FILEMUSTEXIST |
				    OFN_PATHMUSTEXIST |
				    OFN_HIDEREADONLY |
				    OFN_NOCHANGEDIR |
				    0;
			ofn.lpstrDefExt = "dll";
			
			BOOL	result = GetOpenFileName(&ofn);
			if (result == TRUE) {
				SetDlgItemText(hwnd, IDC_DRIVER_DLL_FILE, buf);
			}
			
			break;
		}
		
		default:
			break;
		}
		break;
		
	case WM_INITDIALOG:
	{
		DriverToEdit = lParam;
		
		// Initialize the driver name and comment to their current values.
		SetDlgItemText(hwnd, IDC_DRIVER_DLL_FILE, Render::GetDriverName(DriverToEdit));
		SetDlgItemText(hwnd, IDC_DRIVER_COMMENT, Render::GetDriverComment(DriverToEdit));

		// Localize.
		SetWindowTextUTF(hwnd, UI::String("win_driver_info_caption", "Driver Information"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_DRIVER_DLL_FILE_CAPTION), UI::String("win_driver_dll_file", "Driver DLL File")),
		SetWindowTextUTF(GetDlgItem(hwnd, IDOK), UI::String("win_ok", "OK"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDCANCEL), UI::String("win_cancel", "Cancel"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_COMMENT), UI::String("win_comment", "Comment"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_BROWSE), UI::String("win_browse", "Browse..."));

		return FALSE;	// ?????
	}

	default:
		break;
	}
	
	return FALSE;
}


void	SetupDriverAndModeLists(HWND hwnd, int device)
// Initialize the driver, mode & fullscreen dialog controls.
{
	// Initialize the driver list-box with the appropriate choices.
	{
		int	i;
		
		HWND	DriverList = GetDlgItem(hwnd, IDC_DRIVER_LIST);

//		// Empty the list box.
//		SendMessage(DriverListBox, LB_RESETCONTENT, 0, 0);

		// Set up the list view columns.
		while (ListView_DeleteColumn(DriverList, 0)) ;
		
		LV_COLUMN lvc;
		lvc.mask = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		lvc.fmt = 0;

		lvc.cx = 200;
		lvc.pszText = (char*) UI::String("win_dll_filename", "DLL filename");
		lvc.iSubItem = 0;
		ListView_InsertColumn(DriverList, 0, &lvc);

		lvc.cx = 200;
		lvc.pszText = (char*) UI::String("win_comment", "Comment");
		lvc.iSubItem = 1;
		ListView_InsertColumn(DriverList, 1, &lvc);

		// Add the available drivers.
		ListView_SetItemCount(DriverList, Render::GetDriverCount(device));
		ListView_DeleteAllItems(DriverList);
		for (i = 0; i < Render::GetDriverCount(device); i++) {
			LV_ITEM	lvi;
			lvi.mask = LVIF_TEXT;
			lvi.iItem = i;
			lvi.iSubItem = 0;
			lvi.pszText = Render::GetDriverName(i);
			lvi.lParam = i;

//			SendMessage(DriverList, LVM_INSERTITEM, 0, (LPARAM) &lvi);
			ListView_InsertItem(DriverList, &lvi);
			ListView_SetItemText(DriverList, i, 1, Render::GetDriverComment(i));
			
//			SendMessage(DriverListBox, LB_ADDSTRING, 0, (LPARAM) Render::GetDriverDescription(device, i));
//			SendMessage(DriverListBox, LB_SETITEMDATA, i, (LPARAM) i);
		}

		for (i = 0; i < Render::GetDriverCount(device); i++) {
			ListView_SetItemState(DriverList, i, (i == Render::GetCurrentDriver(device)) ? LVIS_SELECTED : 0, LVIS_SELECTED);
		}
		
//		// Set selection to the current driver.
//		SendMessage(DriverListBox, LB_SETCURSEL, (WPARAM) Render::GetCurrentDriver(device), 0);
	}
	
	// Initialize the mode list-box with the appropriate choices.
	{
		HWND	ModeListBox = GetDlgItem(hwnd, IDC_DISPLAYMODE);

		// Empty the list box.
		SendMessage(ModeListBox, LB_RESETCONTENT, 0, 0);

		// Add the available modes.
		for (int i = 0; i < Render::GetModeCount(device); i++) {
			char	temp[200];
			Render::ModeInfo	mode;
			Render::GetModeInfo(device, &mode, i);
			// Create a string that describes the display mode.
			sprintf(temp, "%d x %d x %dbpp", mode.Width, mode.Height, mode.BPP);
			SendMessage(ModeListBox, LB_ADDSTRING, 0, (LPARAM) temp);
			SendMessage(ModeListBox, LB_SETITEMDATA, i, (LPARAM) i);
			// Somehow grey-out the modes that the current driver can't do????
		}
		
		// Set selection to the current mode.
		SendMessage(ModeListBox, LB_SETCURSEL, (WPARAM) Render::GetCurrentMode(device), 0);
	}
}


int	FindSelectedDriver(HWND dialog)
// Looks in the driver list ListView for the first selected item.
// Returns its index.
{
	HWND	DriverList = GetDlgItem(dialog, IDC_DRIVER_LIST);
	int	count = ListView_GetItemCount(DriverList);
	int	Driver = 0;
	
	// Find the first selected item.
	for (int i = 0; i < count; i++) {
		if (ListView_GetItemState(DriverList, i, LVIS_SELECTED)) {
			Driver = i;
			break;
		}
	}

	return Driver;
}


BOOL FAR PASCAL	AppDisplayOptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
// Message handler for display-options dialog box.
{
//	static int	SelectedDevice = 0;
	
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		{
			// Query check state of Fullscreen box.
			int	FullscreenChecked = IsDlgButtonChecked(hwnd, IDC_FULLSCREEN);
			bool	Fullscreen = (FullscreenChecked == BST_CHECKED);
			
			// Query display-mode choice.
			HWND	ModeListBox = GetDlgItem(hwnd, IDC_DISPLAYMODE);
			int	Mode = SendMessage(ModeListBox, LB_GETCURSEL, 0, 0);
			
			// Query driver choice.
			int	Driver = FindSelectedDriver(hwnd);
		
			Render::SetDisplayOptions(0, Driver, Mode, Fullscreen);
			
			EndDialog(hwnd, TRUE);
			break;
		}

//		case IDC_D3DDEVICE:
//		{
//			// May need to rebuild dialog entries...
//			HWND	DeviceList = GetDlgItem(hwnd, IDC_D3DDEVICE);
//			int	s = SendMessage(DeviceList, CB_GETCURSEL, 0, 0);
//			if (s != SelectedDevice) {
//				SelectedDevice = s;
//				SetupDriverAndModeLists(hwnd, SelectedDevice);
//			}
//			break;
//		}
			
		case IDC_DRIVERS:
		{
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				break;
			}
			break;
		}
			
		case IDCANCEL:
			// Exit, without making any changes.
			EndDialog(hwnd, TRUE);
			break;

		case IDC_ADD_DRIVER:
		{
			// Bring up a blank dialog for the user to enter the driver name and description.
			int	NewDriver = Render::AddNewDriver();
			if (NewDriver >= 0) {
				int	result = DialogBoxParam(hInstApp, MAKEINTRESOURCE(IDD_EDIT_DRIVER), hWndMain, (DLGPROC) AppEditDriver, NewDriver);
				if (result != 1) {
					// User canceled, so remove the driver we just added.
					Render::DeleteDriver(NewDriver);
				} else {
					// Update the driver list view.
					SetupDriverAndModeLists(hwnd, 0);
				}
			}
			
			break;
		}

		case IDC_EDIT_DRIVER:
		{
			// Get the info on the current driver, and bring up the driver dialog with that
			// info.
			int	driver = FindSelectedDriver(hwnd);
			// Don't allow editing of driver 0, the default.
			if (driver != 0) {
				int	result = DialogBoxParam(hInstApp, MAKEINTRESOURCE(IDD_EDIT_DRIVER), hWndMain, (DLGPROC) AppEditDriver, driver);
				if (result == 1) {
					// Make any changes visible.
					SetupDriverAndModeLists(hwnd, 0);
				}
			}
			
			break;
		}
		
		case IDC_DELETE_DRIVER:
		{
			// Delete the current driver.
			int	Driver = FindSelectedDriver(hwnd);
			// Don't ever let the user delete the first (default) driver.
			if (Driver != 0) {
				Render::DeleteDriver(Driver);
				SetupDriverAndModeLists(hwnd, 0);
			}
			
			break;
		}

		default:
			break;
		}
		break;
		
	case WM_INITDIALOG:
	{
		// Set the check-mark to the appropriate value.
		bool	Fullscreen = Render::GetFullscreen();
		CheckDlgButton(hwnd, IDC_FULLSCREEN, Fullscreen ? BST_CHECKED : BST_UNCHECKED);	// BST_INDETERMINATE

		SetupDriverAndModeLists(hwnd, 0);

		// Localize.
		SetWindowTextUTF(hwnd, UI::String("win_display_options_caption", "Display Options"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_FULLSCREEN), UI::String("win_fullscreen", "Fullscreen"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_DESIRED_MODE), UI::String("win_desired_mode", "Desired Mode"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_OPENGL_DRIVER), UI::String("win_opengl_driver", "OpenGL Driver"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_ADD_DRIVER), UI::String("win_add", "Add..."));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_DELETE_DRIVER), UI::String("win_delete", "Delete"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_CHECK1), UI::String("win_32_bit_textures", "32-bit textures"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_EDIT_DRIVER), UI::String("win_edit", "Edit..."));
		SetWindowTextUTF(GetDlgItem(hwnd, IDC_OPTIONS), UI::String("win_options", "Options"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDOK), UI::String("win_ok", "OK"));
		SetWindowTextUTF(GetDlgItem(hwnd, IDCANCEL), UI::String("win_cancel", "Cancel"));
		
		return FALSE;	// ?????
	}

	default:
		break;
	}
	
	return FALSE;
}


BOOL FAR PASCAL	AppInputOptions(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
// Message handler for input-options dialog.
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		{
			// Query check state of AutoEdging checkbox.
			int	AutoEdgingChecked = IsDlgButtonChecked(hwnd, IDC_AUTOEDGING);
			// Set the configuration variable according to the check state.
			Config::SetValue("AutoEdging", AutoEdgingChecked == BST_CHECKED ? "1" : "0");
			
			EndDialog(hwnd, TRUE);
			break;
		}

		case IDCANCEL:
			// Exit, without making any changes.
			EndDialog(hwnd, TRUE);
			break;

		default:
			break;
		}
		break;
		
	case WM_INITDIALOG:
	{
		// Set the AutoEdging check-mark to the appropriate value.
		const char*	val = Config::GetValue("AutoEdging");
		bool	AutoEdging = false;
		if (val != 0 && val[0] != 0 && strcmp(val, "0") != 0) AutoEdging = true;
		CheckDlgButton(hwnd, IDC_AUTOEDGING, AutoEdging ? BST_CHECKED : BST_UNCHECKED);	// BST_INDETERMINATE

		return FALSE;	// ?????
	}

	default:
		break;
	}
	
	return FALSE;
}


BOOL CALLBACK	DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
// Main window message handler.
{
	switch (message) {
	case WM_INITDIALOG:
		// Show the version string.
		SetWindowTextUTF(GetDlgItem(hWnd, IDC_VERSION), Main::GetVersionString());

		// Localize.
		SetWindowTextUTF(GetDlgItem(hWnd, IDC_COPYRIGHT), UI::String("win_copyright", "© 2003\nSlingshot Game Technology"));
		SetWindowTextUTF(GetDlgItem(hWnd, IDC_START_GAME), UI::String("win_start_game", "Start Game"));
		SetWindowTextUTF(GetDlgItem(hWnd, IDC_DISPLAY_OPTIONS), UI::String("win_display_options", "Display Options..."));

		return 0;
		
	case WM_DESTROY:
	case WM_CLOSE:
		hWndMain = NULL;	//????
		PostQuitMessage(0);
		return 1;

	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
		// Make sure mouse cursor is showing.
		ShowMouseCursor(true);
		break;
		
	case WM_MOVE:
		// We get this message after the window has been moved.
//		Render::RefreshWindow();
		break;

	case WM_GETMINMAXINFO:
		// Prevent resizing through this message
//		RECT rc;
//		GetWindowRect(hWnd, &rc);
//		((LPMINMAXINFO)lParam)->ptMaxTrackSize.x = MAX_WIN_XSIZE;
//		((LPMINMAXINFO)lParam)->ptMaxTrackSize.y = MAX_WIN_YSIZE;
//		((LPMINMAXINFO)lParam)->ptMinTrackSize.x = MIN_WIN_XSIZE;
//		((LPMINMAXINFO)lParam)->ptMinTrackSize.y = MIN_WIN_YSIZE;
		return 0;
		
//	case WM_ENTERMENULOOP:
//		Main::SetPaused(true);
////		ShowMouseCursor(true);
//		break;
		
//	case WM_EXITMENULOOP:
//		Main::SetPaused(false);
//		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_QUIT:
			hWndMain = NULL;	//???
			PostQuitMessage(0);
			return 1;
			
		case IDC_DISPLAY_OPTIONS:
			Main::SetPaused(true);
			DialogBox(hInstApp, MAKEINTRESOURCE(IDD_DISPLAYOPTIONS), hWndMain, (DLGPROC) AppDisplayOptions);
			Main::SetPaused(false);
			return 1;

		case IDC_START_GAME:
			if (GameLoop::GetIsOpen() == false) {
				GameLoop::Open();
			}
			return 1;

#ifdef NOT
		case IDC_CAMERA:
			Config::SetValue("Camera", "1");
			if (GameLoop::GetIsOpen() == false) {
				GameLoop::Open();
			}
			return 1;
		
		case IDC_PERF_TEST:
			Config::SetValue("PerfTest", "1");
			if (GameLoop::GetIsOpen() == false) {
				GameLoop::Open();
			}
			return 1;
#endif // NOT
			
#ifdef NOT
		case MENU_ABOUT:
			Main::SetPaused(true);
//			ShowMouseCursor(true);
			DialogBox(hInstApp, MAKEINTRESOURCE(IDD_ABOUT), hWndMain, (DLGPROC) AppAbout);
			Main::SetPaused(false);
			break;

		case MENU_DISPLAYOPTIONS:
			Main::SetPaused(true);
//			ShowMouseCursor(true);
			DialogBox(hInstApp, MAKEINTRESOURCE(IDD_DISPLAYOPTIONS), hWndMain, (DLGPROC) AppDisplayOptions);
			Main::SetPaused(false);
			break;

		case MENU_INPUTOPTIONS:
			Main::SetPaused(true);
//			ShowMouseCursor(true);
			DialogBox(hInstApp, MAKEINTRESOURCE(IDD_INPUTOPTIONS), hWndMain, (DLGPROC) AppInputOptions);
			Main::SetPaused(false);
			break;

		case MENU_RUNGAME:
			if (GameLoop::GetIsOpen() == false) {
				GameLoop::Open();
			}
			break;

		case MENU_RUNCAMERA:
			Config::SetValue("Camera", "1");
			if (GameLoop::GetIsOpen() == false) {
				GameLoop::Open();
			}
			break;
			
		case MENU_RUNPERFTEST:
			Config::SetValue("PerfTest", "1");
			if (GameLoop::GetIsOpen() == false) {
				GameLoop::Open();
			}
			break;
			
		case MENU_EXIT:
			hWndMain = NULL;
			PostQuitMessage(0);
			break;
#endif // NOT
		}
	}
			
#ifdef NOT
	{
	case WM_MOUSEMOVE:
		// If the mouse is within the client area (i.e. the rendering area) and
		// the game loop's going and we're not paused, then hide the cursor,
		// else show the cursor.
		break;
			
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		/*
		 * Call the sample's MouseHandler if available
		 */

		if (!MouseHandler)
			break;
		if ((MouseHandler)(message, wParam, lParam))
			return 1;
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		/*
		 * Call the sample's keyboard handler if available
		 */
		if (!KeyboardHandler)
			break;
		if ((KeyboardHandler)(message, wParam, lParam))
			return 1;
		break;
		/*
		 * Pause and unpause the app when entering/leaving the menu
		 */
	case WM_ENTERMENULOOP:
		AppPause(TRUE);
		break;
	case WM_EXITMENULOOP:
		AppPause(FALSE);
		break;
	case WM_INITMENUPOPUP:
		/*
		 * Check and enable the appropriate menu items
		 */
		if (d3dapp->ThisDriver.bDoesZBuffer) {
			EnableMenuItem((HMENU)wParam, MENU_ZBUFFER, MF_ENABLED);
			CheckMenuItem((HMENU)wParam, MENU_ZBUFFER, myglobs.rstate.bZBufferOn ? MF_CHECKED : MF_UNCHECKED);
		} else {
			EnableMenuItem((HMENU)wParam, MENU_ZBUFFER, MF_GRAYED);
			CheckMenuItem((HMENU)wParam, MENU_ZBUFFER, MF_UNCHECKED);
		}
		CheckMenuItem((HMENU)wParam, MENU_STEP, (myglobs.bSingleStepMode) ? MF_CHECKED : MF_UNCHECKED);
		EnableMenuItem((HMENU)wParam, MENU_GO, (myglobs.bSingleStepMode) ? MF_ENABLED : MF_GRAYED);
		CheckMenuItem((HMENU)wParam, MENU_FLAT, (myglobs.rstate.ShadeMode == D3DSHADE_FLAT) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, MENU_GOURAUD, (myglobs.rstate.ShadeMode == D3DSHADE_GOURAUD) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, MENU_PHONG, (myglobs.rstate.ShadeMode == D3DSHADE_PHONG) ? MF_CHECKED : MF_UNCHECKED);
		EnableMenuItem((HMENU)wParam, MENU_PHONG, MF_GRAYED);
		CheckMenuItem((HMENU)wParam, MENU_CLEARS, myglobs.bClearsOn ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, MENU_POINT, (myglobs.rstate.FillMode == D3DFILL_POINT) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, MENU_WIREFRAME, (myglobs.rstate.FillMode == D3DFILL_WIREFRAME) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, MENU_SOLID, (myglobs.rstate.FillMode == D3DFILL_SOLID) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, MENU_DITHERING, myglobs.rstate.bDithering ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, MENU_SPECULAR, myglobs.rstate.bSpecular ? MF_CHECKED : MF_UNCHECKED);
		EnableMenuItem((HMENU)wParam, MENU_SPECULAR, MF_ENABLED);
		CheckMenuItem((HMENU)wParam, MENU_FOG, myglobs.rstate.bFogEnabled ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem((HMENU)wParam, MENU_ANTIALIAS, myglobs.rstate.bAntialiasing ? MF_CHECKED : MF_UNCHECKED);
		if (d3dapp->ThisDriver.bDoesTextures) {
			CheckMenuItem((HMENU)wParam, MENU_TEXTURE_TOGGLE, (!d3dapp->bTexturesDisabled) ? MF_CHECKED : MF_UNCHECKED);
			EnableMenuItem((HMENU)wParam, MENU_TEXTURE_TOGGLE, MF_ENABLED);
			EnableMenuItem((HMENU)wParam, MENU_TEXTURE_SWAP, (d3dapp->bTexturesDisabled) ? MF_GRAYED : MF_ENABLED);
			CheckMenuItem((HMENU)wParam, MENU_PERSPCORRECT, myglobs.rstate.bPerspCorrect ? MF_CHECKED : MF_UNCHECKED);
			EnableMenuItem((HMENU)wParam, MENU_PERSPCORRECT, (d3dapp->bTexturesDisabled) ? MF_GRAYED : MF_ENABLED);
			CheckMenuItem((HMENU)wParam, MENU_POINT_FILTER, (myglobs.rstate.TextureFilter == D3DFILTER_NEAREST) ? MF_CHECKED : MF_UNCHECKED);
			EnableMenuItem((HMENU)wParam, MENU_POINT_FILTER, (d3dapp->bTexturesDisabled) ? MF_GRAYED : MF_ENABLED);
			CheckMenuItem((HMENU)wParam, MENU_LINEAR_FILTER, (myglobs.rstate.TextureFilter == D3DFILTER_LINEAR) ? MF_CHECKED : MF_UNCHECKED);
			EnableMenuItem((HMENU)wParam, MENU_LINEAR_FILTER, (d3dapp->bTexturesDisabled) ? MF_GRAYED : MF_ENABLED);
			for (i = 0; i < d3dapp->NumTextureFormats; i++) {
				CheckMenuItem((HMENU)wParam, MENU_FIRST_FORMAT + i, (i == d3dapp->CurrTextureFormat) ? MF_CHECKED : MF_UNCHECKED);
				EnableMenuItem((HMENU)wParam, MENU_FIRST_FORMAT + i, (d3dapp->bTexturesDisabled) ? MF_GRAYED : MF_ENABLED);
			}
		} else {
			EnableMenuItem((HMENU)wParam, MENU_TEXTURE_SWAP, MF_GRAYED);
			EnableMenuItem((HMENU)wParam, MENU_TEXTURE_TOGGLE, MF_GRAYED);
			EnableMenuItem((HMENU)wParam, MENU_POINT_FILTER, MF_GRAYED);
			EnableMenuItem((HMENU)wParam, MENU_LINEAR_FILTER, MF_GRAYED);
			EnableMenuItem((HMENU)wParam, MENU_PERSPCORRECT, MF_GRAYED);
		}
		if (d3dapp->bIsPrimary) {
			CheckMenuItem((HMENU)wParam, MENU_FULLSCREEN, d3dapp->bFullscreen ? MF_CHECKED : MF_UNCHECKED);
			EnableMenuItem((HMENU)wParam, MENU_FULLSCREEN, d3dapp->bFullscreen && !d3dapp->ThisDriver.bCanDoWindow ? MF_GRAYED : MF_ENABLED);
			EnableMenuItem((HMENU)wParam, MENU_NEXT_MODE, (!d3dapp->bFullscreen) ? MF_GRAYED : MF_ENABLED);
			EnableMenuItem((HMENU)wParam, MENU_PREVIOUS_MODE, (!d3dapp->bFullscreen) ? MF_GRAYED : MF_ENABLED);
		} else {
			EnableMenuItem((HMENU)wParam, MENU_FULLSCREEN, MF_GRAYED);
			EnableMenuItem((HMENU)wParam, MENU_NEXT_MODE, MF_GRAYED);
			EnableMenuItem((HMENU)wParam, MENU_PREVIOUS_MODE, MF_GRAYED);
		}
		for (i = 0; i < d3dapp->NumModes; i++) {
			CheckMenuItem((HMENU)wParam, MENU_FIRST_MODE + i, (i == d3dapp->CurrMode) ? MF_CHECKED : MF_UNCHECKED);
			EnableMenuItem((HMENU)wParam, MENU_FIRST_MODE + i, (d3dapp->Mode[i].bThisDriverCanDo) ? MF_ENABLED : MF_GRAYED);
		}
		for (i = 0; i < d3dapp->NumDrivers; i++) {
			CheckMenuItem((HMENU)wParam, MENU_FIRST_DRIVER + i, (i == d3dapp->CurrDriver) ? MF_CHECKED : MF_UNCHECKED);
		}
		break;
	case WM_GETMINMAXINFO:
		/*
		 * Some samples don't like being resized, such as those which use
		 * screen coordinates (TLVERTEXs).
		 */
		if (myglobs.bResizingDisabled) {
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.x = START_WIN_XSIZE;
			((LPMINMAXINFO)lParam)->ptMaxTrackSize.y = START_WIN_YSIZE;
			((LPMINMAXINFO)lParam)->ptMinTrackSize.x = START_WIN_XSIZE;
			((LPMINMAXINFO)lParam)->ptMinTrackSize.y = START_WIN_YSIZE;
			return 0;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
			case MENU_ABOUT:
				AppPause(TRUE);
				DialogBox(myglobs.hInstApp, "AppAbout", myglobs.hWndMain, (DLGPROC)AppAbout);
				AppPause(FALSE);
				break;
			case MENU_EXIT:
				CleanUpAndPostQuit();
				break;
			case MENU_STEP:
				/*
				 * Begin single step more or draw a frame if in single
				 * step mode
				 */
				if (!myglobs.bSingleStepMode) {
					myglobs.bSingleStepMode = TRUE;
					myglobs.bDrawAFrame = TRUE;
				} else if (!myglobs.bDrawAFrame) {
					myglobs.bDrawAFrame = TRUE;
				}
				break;
			case MENU_GO:
				/*
				 * Exit single step mode
				 */
				if (myglobs.bSingleStepMode) {
					myglobs.bSingleStepMode = FALSE;
					ResetFrameRate();
				}
				break;
			case MENU_STATS:
				/*
				 * Toggle output of frame rate and window info
				 */
				if ((myglobs.bShowFrameRate) && (myglobs.bShowInfo)) {
					myglobs.bShowFrameRate = FALSE;
					myglobs.bShowInfo = FALSE;
					break;
				}
				if ((!myglobs.bShowFrameRate) && (!myglobs.bShowInfo)) {
					myglobs.bShowFrameRate = TRUE;
					break;
				}
				myglobs.bShowInfo = TRUE;
				break;
			case MENU_FULLSCREEN:
				if (d3dapp->bFullscreen) {
					/*
					 * Return to a windowed mode.  Let D3DApp decide which
					 * D3D driver to use in case this one cannot render to
					 * the Windows display depth
					 */
					if (!D3DAppWindow(D3DAPP_YOUDECIDE, D3DAPP_YOUDECIDE)) {
						ReportD3DAppError();
						CleanUpAndPostQuit();
						break;
					}
				} else {
					/*
					 * Enter the current fullscreen mode.  D3DApp may
					 * resort to another mode if this driver cannot do
					 * the currently selected mode.
					 */
					if (!D3DAppFullscreen(d3dapp->CurrMode)) {
						ReportD3DAppError();
						CleanUpAndPostQuit();
						break;
					}
				}
				break;
				/*
				 * Texture filter method selection
				 */
			case MENU_POINT_FILTER:
				if (myglobs.rstate.TextureFilter == D3DFILTER_NEAREST)
					break;
				myglobs.rstate.TextureFilter = D3DFILTER_NEAREST;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_LINEAR_FILTER:
				if (myglobs.rstate.TextureFilter == D3DFILTER_LINEAR)
					break;
				myglobs.rstate.TextureFilter = D3DFILTER_LINEAR;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
				/* 
				 * Shading selection
				 */
			case MENU_FLAT:
				if (myglobs.rstate.ShadeMode == D3DSHADE_FLAT)
					break;
				myglobs.rstate.ShadeMode = D3DSHADE_FLAT;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_GOURAUD:
				if (myglobs.rstate.ShadeMode == D3DSHADE_GOURAUD)
					break;
				myglobs.rstate.ShadeMode = D3DSHADE_GOURAUD;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_PHONG:
				if (myglobs.rstate.ShadeMode == D3DSHADE_PHONG)
					break;
				myglobs.rstate.ShadeMode = D3DSHADE_PHONG;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
				/*
				 * Fill mode selection
				 */
			case MENU_POINT:
				if (myglobs.rstate.FillMode == D3DFILL_POINT)
					break;
				myglobs.rstate.FillMode = D3DFILL_POINT;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_WIREFRAME:
				if (myglobs.rstate.FillMode == D3DFILL_WIREFRAME)
					break;
				myglobs.rstate.FillMode = D3DFILL_WIREFRAME;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_SOLID:
				if (myglobs.rstate.FillMode == D3DFILL_SOLID)
					break;
				myglobs.rstate.FillMode = D3DFILL_SOLID;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_PERSPCORRECT:
				/*
				 * Toggle perspective correction
				 */
				myglobs.rstate.bPerspCorrect =
					!myglobs.rstate.bPerspCorrect;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_CLEARS:
				/*
				 * Toggle the clearing the the back buffer and Z-buffer
				 * and set the resized flag to clear the entire window
				 */
				myglobs.bClearsOn = !myglobs.bClearsOn;
				if (myglobs.bClearsOn)
					myglobs.bResized = TRUE;
				break;
			case MENU_ZBUFFER:
				/*
				 * Toggle the use of a Z-buffer
				 */
				myglobs.rstate.bZBufferOn = !myglobs.rstate.bZBufferOn;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_DITHERING:
				/*
				 * Toggle dithering
				 */
				myglobs.rstate.bDithering = !myglobs.rstate.bDithering;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_SPECULAR:
				/*
				 * Toggle specular highlights
				 */
				myglobs.rstate.bSpecular = !myglobs.rstate.bSpecular;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_FOG:
				/*
				 * Toggle fog
				 */
				myglobs.rstate.bFogEnabled = !myglobs.rstate.bFogEnabled;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_ANTIALIAS:
				/*
				 * Toggle anti-aliasing
				 */
				myglobs.rstate.bAntialiasing =
					!myglobs.rstate.bAntialiasing;
				if (!D3DAppSetRenderState(&myglobs.rstate)) {
					ReportD3DAppError();
					break;
				}
				break;
			case MENU_TEXTURE_TOGGLE:
				/*
				 * Release the sample's execute buffers, toggle the
				 * texture disabled state and recreate them.
				 */
				ReleaseView(d3dapp->lpD3DViewport);
				D3DAppDisableTextures(!d3dapp->bTexturesDisabled);
				{
					if (!InitView(d3dapp->lpDD, d3dapp->lpD3D,
						d3dapp->lpD3DDevice,
						d3dapp->lpD3DViewport,
						d3dapp->NumUsableTextures,
						d3dapp->TextureHandle)) {
						Msg("InitView failed.
");
						CleanUpAndPostQuit();
						break;
					}
				}
				myglobs.bResized = TRUE;
				break;
			case MENU_TEXTURE_SWAP:
				/*
				 * Swap textures using the load command
				 */
				if (!D3DAppSwapTextures()) {
					ReportD3DAppError();
					break;
				}
				/*
				 * Just in case we have a texture background
				 */
				myglobs.bResized = TRUE;
				break;
			case MENU_NEXT_MODE:
				/*
				 * Enter the next usable fullscreen mode
				 */
				i = d3dapp->CurrMode;
				do {
					++i;
					if (i >= d3dapp->NumModes)
						i = 0;
					if (!d3dapp->Mode[i].bThisDriverCanDo)
						continue;
					else {
						if (!D3DAppFullscreen(i)) {
							ReportD3DAppError();
							CleanUpAndPostQuit();
						}
						break;
					}
				} while(i != d3dapp->CurrMode);
				break;
			case MENU_PREVIOUS_MODE:
				/*
				 * Enter the previous usable fullscreen mode
				 */
				i = d3dapp->CurrMode;
				do {
					--i;
					if (i < 0)
						i = d3dapp->NumModes - 1;
					if (!d3dapp->Mode[i].bThisDriverCanDo)
						continue;
					else {
						if (!D3DAppFullscreen(i)) {
							ReportD3DAppError();
							CleanUpAndPostQuit();
						}
						break;
					}
				} while(i != d3dapp->CurrMode);
				break;
		}
		if (   LOWORD(wParam) >= MENU_FIRST_FORMAT
		       && LOWORD(wParam) < MENU_FIRST_FORMAT +
		       D3DAPP_MAXTEXTUREFORMATS
		       && d3dapp->CurrTextureFormat !=
		       LOWORD(wParam) - MENU_FIRST_FORMAT) {
			/*
			 * Release the sample's execute buffers, change the texture
			 * format and recreate the view.
			 */
			ReleaseView(d3dapp->lpD3DViewport);
			if (!D3DAppChangeTextureFormat(LOWORD(wParam) -
				MENU_FIRST_FORMAT)) {
				ReportD3DAppError();
				CleanUpAndPostQuit();
			}
			{
				if (!InitView(d3dapp->lpDD, d3dapp->lpD3D,
					      d3dapp->lpD3DDevice, d3dapp->lpD3DViewport,
					      d3dapp->NumUsableTextures,
					      d3dapp->TextureHandle)) {
					Msg("InitView failed.
");
					CleanUpAndPostQuit();
					break;
				}
			}
			ResetFrameRate();
		}
		if (   LOWORD(wParam) >= MENU_FIRST_DRIVER
		       && LOWORD(wParam) < MENU_FIRST_DRIVER + D3DAPP_MAXD3DDRIVERS
		       && d3dapp->CurrDriver != LOWORD(wParam) - MENU_FIRST_DRIVER) {
			/*
			 * Change the D3D driver
			 */
			if (!D3DAppChangeDriver(LOWORD(wParam) - MENU_FIRST_DRIVER,
						NULL)) {
				ReportD3DAppError();
				CleanUpAndPostQuit();
			}
		}
		if (   LOWORD(wParam) >= MENU_FIRST_MODE
		       && LOWORD(wParam) < MENU_FIRST_MODE+100) {
			/*
			 * Switch to the selected fullscreen mode
			 */
			if (!D3DAppFullscreen(LOWORD(wParam) - MENU_FIRST_MODE)) {
				ReportD3DAppError();
				CleanUpAndPostQuit();
			}
		}
		/*
		 * Whenever we receive a command in single step mode, draw a frame
		 */
		if (myglobs.bSingleStepMode)
			myglobs.bDrawAFrame = TRUE;
		return 0L;
	}
#endif // NOT
	
//	return DefWindowProc(hWnd, message, wParam, lParam);
	return 0;
}


//
// Game window.
//

				
void	Main::OpenGameWindow(int Width, int Height, bool Fullscreen)
// Opens a window of the specified width and height that's suitable for rendering the game.
{
	HINSTANCE	hInstance = hInstApp;

	// Register the window class
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = GameWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = 0; // LoadIcon(hInstance, "AppIcon");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = "AppMenu";
	wc.lpszClassName = "Soul Ride Game Window";
	if (!RegisterClass(&wc)) return;

	if (Fullscreen) {

//	if (Fullscreen) {
		// Change the display mode.
		DEVMODE	dm;
		dm.dmSize = sizeof(dm);
		dm.dmPelsWidth        = Width;
		dm.dmPelsHeight       = Height;
		dm.dmFields           = DM_PELSWIDTH | DM_PELSHEIGHT ;
		ChangeDisplaySettings(&dm, CDS_FULLSCREEN /* CDS_TEST */);
		DisplayChanged = true;
//	}
		// Fullscreen window.
		GameWindow = CreateWindowEx(WS_EX_TOPMOST,
					    "Soul Ride Game Window", "Soul Ride",
					  /* WS_BORDER | WS_DLGFRAME | */ WS_VISIBLE | WS_POPUP,
					  0, 0, Width, Height,
					  NULL, NULL, hInstance, NULL);

	} else {
		// Same window settings as QuakeII
		// WS_BORDER WS_DLGFRAME WS_CLIPSIBLINGS
		LONG  h;
		LONG  w;
		
		h = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CXDLGFRAME)*2 + Height;
		w = GetSystemMetrics(SM_CXDLGFRAME)*2 + Width;

		GameWindow = CreateWindow("Soul Ride Game Window", "Soul Ride",
					  WS_BORDER | WS_DLGFRAME | WS_CLIPSIBLINGS,
					  CW_USEDEFAULT, CW_USEDEFAULT, /* 1, 1, */ w, h,
					  NULL, NULL, hInstance, NULL);
	}

	if (!GameWindow) {
		MessageBox(hWndMain, "CreateWindowEx failed", "Soul Ride", MB_OK);
		return;
	}

	// Display the window
	SetFocus(GameWindow);
	ShowWindow(GameWindow, SW_SHOWNORMAL);
	UpdateWindow(GameWindow);

	ShowMouseCursor(false);
}


void	Main::CloseGameWindow()
// Closes the game rendering window.
{
	if (DisplayChanged) {
		ChangeDisplaySettings(NULL, CDS_FULLSCREEN);	// Make sure display is restored.
	}

	ShowWindow(GameWindow, SW_HIDE);
	DestroyWindow(GameWindow);
}


static void	KeyEvent(int vkey, bool Down);


long FAR PASCAL	GameWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
// Game window message handler.  Doesn't do much.
{
	switch (message) {
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		if (Main::GetQuit()) break;
		
		// Hide the cursor.
		ShowMouseCursor(false);

		// Notify the input module of the mouse status.
		Input::NotifyMouseStatus(LOWORD(lParam), HIWORD(lParam),
					 (MK_LBUTTON & wParam ? 1 : 0) |
					 (MK_RBUTTON & wParam ? 2 : 0) |
					 (MK_MBUTTON & wParam ? 4 : 0));

		// Notify the input module specifically if the mouse event is a button up/down event.
		if (message != WM_MOUSEMOVE) {
			Input::MouseEvent	m;
			m.type = 0;
			m.x = LOWORD(lParam);
			m.y = HIWORD(lParam);
			switch (message) {
				case WM_LBUTTONDOWN:	m.type = 1; break;
				case WM_LBUTTONUP:	m.type = -1; break;
				case WM_RBUTTONDOWN:	m.type = 2; break;
				case WM_RBUTTONUP:	m.type = -2; break;
				case WM_MBUTTONDOWN:	m.type = 3; break;
				case WM_MBUTTONUP:	m.type = -3; break;
			}
			Input::NotifyMouseEvent(m);
		}

		break;
			
	case WM_NCMOUSEMOVE:
		// Show the cursor.
		ShowMouseCursor(true);
		break;
		
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		// Check to see if the key-code matches one of the button aliases.  If so, notify the Input module.
		if ((lParam & 0x40000000) == 0 || Config::GetBoolValue("KeyRepeat") == true) {
			KeyEvent(int(wParam), true);
		}
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		KeyEvent(int(wParam), false);
		break;

	case WM_CHAR:
		// Pass info to Input module.
		if (Main::GetQuit()) break;
		
		Input::NotifyAlphaKeyClick(wParam);
		break;

	case MM_MCINOTIFY:
	case MM_MCISIGNAL:
		// Could be a track ending.  Have the music module check.
		Music::CDNotify();
		break;

	case WM_DEVICECHANGE:
		// Be on the lookout for CD disc insertion.
		if (wParam == DBT_DEVICEARRIVAL) {
			// A new disc may have been inserted.
			// Tell the music module to check.
			Music::CDCheckEntry();
		}
		break;
		
	case WM_DESTROY:
//		hWndMain = NULL;
//		PostQuitMessage(0);
		// xxxx Change out of fullscreen mode?
		break;

	case WM_MOVE:
		// We get this message after the window has been moved.
		Render::RefreshWindow();
		return 0;

	case WM_GETMINMAXINFO:
		// Prevent resizing through this message
		RECT rc;
		GetWindowRect(hWnd, &rc);
		((LPMINMAXINFO)lParam)->ptMaxTrackSize.x = MAX_WIN_XSIZE;
		((LPMINMAXINFO)lParam)->ptMaxTrackSize.y = MAX_WIN_YSIZE;
		((LPMINMAXINFO)lParam)->ptMinTrackSize.x = MIN_WIN_XSIZE;
		((LPMINMAXINFO)lParam)->ptMinTrackSize.y = MIN_WIN_YSIZE;
		return 0;
		
	case WM_ENTERMENULOOP:
//		Main::SetPaused(true);
//		ShowMouseCursor(true);
		break;
		
	case WM_EXITMENULOOP:
//		Main::SetPaused(false);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case MENU_ABOUT:
//			Main::SetPaused(true);
//			ShowMouseCursor(true);
//			DialogBox(hInstApp, MAKEINTRESOURCE(IDD_ABOUT), hWndMain, (DLGPROC) AppAbout);
//			Main::SetPaused(false);
			break;

		case MENU_DISPLAYOPTIONS:
//			Main::SetPaused(true);
//			ShowMouseCursor(true);
//			DialogBox(hInstApp, MAKEINTRESOURCE(IDD_DISPLAYOPTIONS), hWndMain, (DLGPROC) AppDisplayOptions);
//			Main::SetPaused(false);
			break;

		case MENU_INPUTOPTIONS:
//			Main::SetPaused(true);
//			ShowMouseCursor(true);
//			DialogBox(hInstApp, MAKEINTRESOURCE(IDD_INPUTOPTIONS), hWndMain, (DLGPROC) AppInputOptions);
//			Main::SetPaused(false);
			break;

		case MENU_RUNGAME:
//			if (GameLoop::GetIsOpen() == false) {
//				GameLoop::Open();
//			}
			break;

		case MENU_RUNCAMERA:
//			Config::SetValue("Camera", "1");
//			if (GameLoop::GetIsOpen() == false) {
//				GameLoop::Open();
//			}
			break;
			
		case MENU_RUNPERFTEST:
//			Config::SetValue("PerfTest", "1");
//			if (GameLoop::GetIsOpen() == false) {
//				GameLoop::Open();
//			}
			break;
			
		case MENU_EXIT:
//			hWndMain = NULL;
//			PostQuitMessage(0);
			break;
		}
	}
	
	return DefWindowProc(hWnd, message, wParam, lParam);
}



static void	KeyEvent(int vkey, bool Down)
// Looks at the vkey code and sends a notification to the Input module if
// the vkey matches one of the input buttons.
// Down should be true when the key goes down or when it auto-repeats.  Down
// should be false for key-up events.
{
	if (Main::GetQuit()) return;

	if (vkey == VK_CONTROL) {
		Input::NotifyControlKey(Down);
	}
	
	Input::ButtonID	id = Input::BUTTONCOUNT;

	switch (vkey) {
	default:
		break;
		
	case VK_CONTROL: id = Input::BUTTON0; break;
	case VK_SHIFT: id = Input::BUTTON1; break;
	case VK_SPACE: id = Input::BUTTON2; break;
	case VK_MENU: id = Input::BUTTON3; break;
	case VK_LEFT: id = Input::LEFT1; break;
	case VK_RIGHT: id = Input::RIGHT1; break;
	case VK_UP: id = Input::UP1; break;
	case VK_DOWN: id = Input::DOWN1; break;
	case VK_RETURN: id = Input::ENTER; break;
	case VK_ESCAPE: id = Input::ESCAPE; break;
	case VK_F1: id = Input::F1; break;
	case VK_F2: id = Input::F2; break;
	case VK_F3: id = Input::F3; break;
	case VK_F4: id = Input::F4; break;
	case VK_F5: id = Input::F5; break;
	case VK_F6: id = Input::F6; break;
	case VK_F7: id = Input::F7; break;
	case VK_F8: id = Input::F8; break;
	case VK_F9:
		id = Input::F9;
		break;
	case VK_F10:
		id = Input::F10;
		break;
	case VK_F11: id = Input::F11; break;
	case VK_F12: id = Input::F12; break;
	}

	if (id < Input::BUTTONCOUNT) {
		Input::NotifyKeyEvent(id, Down, Down);
	}


	// Translate basic editing keys into special character codes.
	if (Down) {
		int	c = 0;
		switch (vkey) {
		case VK_INSERT: c = 128; break;
				
		case VK_END: c = 129; break;
		case VK_DOWN: c = 130; break;
		case VK_NEXT: c = 131; break;	// pg dn

		case VK_LEFT: c = 132; break;
		case VK_RIGHT: c = 134; break;

		case VK_HOME: c = 135; break;
		case VK_UP: c = 136; break;
		case VK_PRIOR: c = 137; break;	// pg up
				   
		case VK_DELETE: c = 138; break;

		case VK_F1: c = 141; break;
		case VK_F2: c = 142; break;
		case VK_F3: c = 143; break;
		case VK_F4: c = 144; break;
		case VK_F5: c = 145; break;
		case VK_F6: c = 146; break;
		case VK_F7: c = 147; break;
		case VK_F8: c = 148; break;
		case VK_F9: c = 149; break;
		case VK_F10: c = 150; break;
		case VK_F11: c = 151; break;
		case VK_F12: c = 152; break;
		}

		if (c) Input::NotifyAlphaKeyClick(c);
	}
}


namespace Main {
;
//


void	CenterMouse()
// Center the mouse cursor.
{
	int	x = Render::GetWindowWidth() >> 1;
	int	y = Render::GetWindowHeight() >> 1;
//	SDL_WarpMouse(x, y);
	SetCursorPos(x, y);

	// Update some mouse-speed measuring state to prevent it from
	// considering this synthetic move a real mouse motion.
	MouseX = float(x);
	MouseY = float(y);
}


}; // end namespace Main
