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
///////////////////////////////////////////////////////
//
//  File: vecedit_winmain.cpp
//
//  Vector Editor -- Windows frontend
//
//  Author: Mike Linkovich
//
//  start date: Feb 29 2000
//
////////////////////////////////


//
//  Includes
//

#define WIN32_LEAN_AND_MEAN     //  Minimize includes
#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>           //  For timeGetTime()
#include "gl/gl.h"              //  For OpenGL

//#include "gamegui.h"          //  GameGUI main header

#include "veceditor.h"
#include "vecedit_res.h"

//#include <objbase.h>          //  For CoInitialize() -- dsound.lib, dxguid.lib need not be linked
//#include "gg_audio.h"         //  GG_Audio Used only by this file, for sound loading
                                //  utilities.  DSound buffers are passed to
                                //  GameGUI via callbacks.

static char logBuf[256] = "";   //  For misc strings


//
//  Function Declarations
//
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
VOID EnableOpenGL( HWND hWnd, HDC * hDC, HGLRC * hRC );
VOID DisableOpenGL( HWND hWnd, HDC hDC, HGLRC hRC );
//GG_Rval loadSndCallback( void *appObj, char *fileName, void **newSnd );

//
//  File scope global variables
//
static VecEditor  *g_editor = null;
static const int  APP_WIDTH = 640;
static const int  APP_HEIGHT = 480;
static bool       g_bOpenGLEnabled = false;
static char       g_curFileName[256] = "";


static int CALLBACK VecEditorDlg( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
  static HDC      s_glhdc = NULL;   //  For GL use
  static HGLRC    s_glhrc = NULL;
  static HWND     s_glhwnd = NULL;
  static GG_Rect  s_rcDisplay;

  switch( iMsg )
  {
    case WM_INITDIALOG:
      //
      //  On create, move the dialog to the centre of screen
      //
      {
        char *fileName = (char*)lParam;
        s_rcDisplay.set(0,0,0,0);
        GG_Rect rc;
//        int     w, h;
        HWND    hwndDisplay = NULL;

//        GetClientRect( hWnd, (LPRECT)&rc );
//        if( (rc.x2 - rc.x1) != APP_WIDTH )
//          w = APP_WIDTH + (APP_WIDTH - (rc.x2 - rc.x1));
//        if( (rc.y2 - rc.y1) != APP_HEIGHT )
//          h = APP_HEIGHT + (APP_HEIGHT - (rc.y2 - rc.y1));
//        MoveWindow( hWnd,
//                    (GetSystemMetrics(SM_CXFULLSCREEN) - w) / 2,
//                    (GetSystemMetrics(SM_CYFULLSCREEN) - h) / 2,
//                    w, h, FALSE );

        if( (s_glhwnd = GetDlgItem(hWnd, DLGWND_DISPLAY)) == null )
          return TRUE;

        GetClientRect( s_glhwnd, (LPRECT)&rc );

        //  Setup OpenGL for this dialog window
        //
        EnableOpenGL( s_glhwnd, &s_glhdc, &s_glhrc );

        //  Create the Editor object
        //
        g_editor = new VecEditor( rc );

        //  Find the offset..
        //
        GetClientRect( s_glhwnd, (LPRECT)&s_rcDisplay );

        //  Load vector file
        //
        if( strlen(fileName) > 1 )
        {
          if( g_editor->read( fileName ) != VE_OK )
            MessageBox( hWnd, fileName, "Failed to load vector file", MB_OK );
          else
            strcpy( g_curFileName, fileName );
        }
      }
      return TRUE;

    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
      }
      return 0;

    case WM_DRAWITEM:
      {
        LPDRAWITEMSTRUCT pDis = (LPDRAWITEMSTRUCT)lParam;
        switch( (UINT)wParam )
        {
          case DLGWND_DISPLAY:
            if( g_editor != null )
            {
              g_editor->paint();
              SwapBuffers( pDis->hDC );   // s_glhdc
            }
            return TRUE;
        }
      }
      return 0;

    case WM_LBUTTONDOWN:
      if( g_editor != null )
      {
        VE_Message msg;
        msg.p.x = LOWORD(lParam);
        msg.p.y = HIWORD(lParam);
        MapWindowPoints( hWnd, s_glhwnd, (LPPOINT)&msg.p, 1 );
        if( s_rcDisplay.inside(msg.p) )
        {
          msg.id = VE_MSG_MOUSE1DOWN;
          msg.p2.x = 0;
          msg.p2.y = 0;
          if( wParam & MK_CONTROL )
            msg.p2.x |= VE_MSGMOD_CTRL;
          if( wParam & MK_SHIFT )
            msg.p2.x |= VE_MSGMOD_SHIFT;

          if( g_editor->handleMessage(msg) == VE_DIRTY )
          {
            g_editor->paint();
            SwapBuffers( s_glhdc );   //  Show the newly loaded shape
          }

          g_editor->getStatusStr( logBuf );
          SendDlgItemMessage( hWnd, TXT_STATUS, WM_SETTEXT, 0, (LPARAM)logBuf );

          GG_ColorARGBf c;
          g_editor->getPenColor( &c );
          sprintf( logBuf, "%1.2f", c.r );
          SendDlgItemMessage( hWnd, EDT_COLOR_R, WM_SETTEXT, 0, (LPARAM)logBuf );
          sprintf( logBuf, "%1.2f", c.g );
          SendDlgItemMessage( hWnd, EDT_COLOR_G, WM_SETTEXT, 0, (LPARAM)logBuf );
          sprintf( logBuf, "%1.2f", c.b );
          SendDlgItemMessage( hWnd, EDT_COLOR_B, WM_SETTEXT, 0, (LPARAM)logBuf );
          sprintf( logBuf, "%1.2f", c.a );
          SendDlgItemMessage( hWnd, EDT_COLOR_A, WM_SETTEXT, 0, (LPARAM)logBuf );
        }
      }
      return 0;

    case WM_RBUTTONDOWN:
      if( g_editor != null )
      {
        VE_Message msg;
        msg.p.x = LOWORD(lParam);
        msg.p.y = HIWORD(lParam);
        MapWindowPoints( hWnd, s_glhwnd, (LPPOINT)&msg.p, 1 );
        if( s_rcDisplay.inside(msg.p) )
        {
          msg.id = VE_MSG_MOUSE2DOWN;
          msg.p2.x = 0;
          msg.p2.y = 0;
          if( wParam & MK_CONTROL )
            msg.p2.x |= VE_MSGMOD_CTRL;
          if( wParam & MK_SHIFT )
            msg.p2.x |= VE_MSGMOD_SHIFT;

          if( g_editor->handleMessage(msg) == VE_DIRTY )
          {
            g_editor->paint();
            SwapBuffers( s_glhdc );   //  Show the newly loaded shape
          }

          g_editor->getStatusStr( logBuf );
          SendDlgItemMessage( hWnd, TXT_STATUS, WM_SETTEXT, 0, (LPARAM)logBuf );
        }
      }
      return 0;

    case WM_LBUTTONUP:
      if( g_editor != null )
      {
        VE_Message msg;
        msg.id = VE_MSG_MOUSE1UP;
        msg.p.x = LOWORD(lParam);
        msg.p.y = HIWORD(lParam);
        MapWindowPoints( hWnd, s_glhwnd, (LPPOINT)&msg.p, 1 );
        msg.p2.x = 0;
        msg.p2.y = 0;
        if( wParam & MK_CONTROL )
          msg.p2.x |= VE_MSGMOD_CTRL;
        if( wParam & MK_SHIFT )
          msg.p2.x |= VE_MSGMOD_SHIFT;

        if( g_editor->handleMessage(msg) == VE_DIRTY )
        {
          g_editor->paint();
          SwapBuffers( s_glhdc );   //  Show the newly loaded shape
        }

        g_editor->getStatusStr( logBuf );
        SendDlgItemMessage( hWnd, TXT_STATUS, WM_SETTEXT, 0, (LPARAM)logBuf );
      }
      return 0;

    case WM_MOUSEMOVE:
      if( g_editor != null )
      {
        VE_Message msg;
        msg.id = VE_MSG_MOUSEMOVE;
        msg.p.x = LOWORD(lParam);
        msg.p.y = HIWORD(lParam);
        MapWindowPoints( hWnd, s_glhwnd, (LPPOINT)&msg.p, 1 );
        if( s_rcDisplay.inside(msg.p) )
        {
          msg.p2.x = 0;
          msg.p2.y = 0;
          if( wParam & MK_CONTROL )
            msg.p2.x |= VE_MSGMOD_CTRL;
          if( wParam & MK_SHIFT )
            msg.p2.x |= VE_MSGMOD_SHIFT;

          if( g_editor->handleMessage(msg) == VE_DIRTY )
          {
            g_editor->paint();
            SwapBuffers( s_glhdc );
          }
          g_editor->getStatusStr( logBuf );
          SendDlgItemMessage( hWnd, TXT_STATUS, WM_SETTEXT, 0, (LPARAM)logBuf );
        }
      }
      return 0;

    case WM_COMMAND:
      switch( LOWORD(wParam) )
      {
        case MNU_FILE_OPEN:
          if( g_editor != null )
          {
            char fileName[255] = "";
            OPENFILENAME fileInfo;

            memset( &fileInfo, 0, sizeof(OPENFILENAME) );
            fileInfo.lStructSize = sizeof(OPENFILENAME);
            fileInfo.hwndOwner = hWnd;
            fileInfo.lpstrFilter = "Vector shape files (*.ggv)\0*.ggv\0\0";
            fileInfo.lpstrFile = fileName;
            fileInfo.lpstrTitle = " Open Vector Shape file...";
            fileInfo.nMaxFile = 250;
            fileInfo.Flags = OFN_NOCHANGEDIR;

            if( GetOpenFileName( &fileInfo ) != 0 )
            {
              if( g_editor->read( fileName ) != VE_OK )
                MessageBox( hWnd, fileName, "Failed to load vector file", MB_OK );
              else
                strcpy( g_curFileName, fileName );

              g_editor->paint();
              SwapBuffers( s_glhdc );   //  Show the newly loaded shape
            }
          }
          break;

        case MNU_FILE_CLOSE:
          if( g_editor != null )
          {
            g_editor->clear();        //  Clear the shape from memory
            g_editor->paint();
            SwapBuffers( s_glhdc );   //  Show the empty screen
            g_curFileName[0] = '\0';
          }
          break;

        case MNU_FILE_SAVE:
          if( g_editor != null )
          {
            if( strlen(g_curFileName) > 0 )
            {
              g_editor->write(g_curFileName);
            }
          }
          break;

        case BTN_SETCOLOR:
          {
            //  Retrieve the color values from the edit boxes..
            GG_ColorARGBf c;
            SendDlgItemMessage(  hWnd, EDT_COLOR_R, WM_GETTEXT, 32, (LPARAM)logBuf );
            sscanf( logBuf, "%f", &c.r );
            SendDlgItemMessage(  hWnd, EDT_COLOR_G, WM_GETTEXT, 32, (LPARAM)logBuf );
            sscanf( logBuf, "%f", &c.g );
            SendDlgItemMessage(  hWnd, EDT_COLOR_B, WM_GETTEXT, 32, (LPARAM)logBuf );
            sscanf( logBuf, "%f", &c.b );
            SendDlgItemMessage(  hWnd, EDT_COLOR_A, WM_GETTEXT, 32, (LPARAM)logBuf );
            sscanf( logBuf, "%f", &c.a );

            c.clamp();    //  Trim to 0.0<=c<=1.0

            sprintf( logBuf, "%1.2f", c.r );    //  Reduce to 2 dec. places
            sscanf( logBuf, "%f", &c.r );       //  and re-set edit-text
            SendDlgItemMessage(  hWnd, EDT_COLOR_R, WM_SETTEXT, 0, (LPARAM)logBuf );
            sprintf( logBuf, "%1.2f", c.g );
            sscanf( logBuf, "%f", &c.g );
            SendDlgItemMessage(  hWnd, EDT_COLOR_G, WM_SETTEXT, 0, (LPARAM)logBuf );
            sprintf( logBuf, "%1.2f", c.b );
            sscanf( logBuf, "%f", &c.b );
            SendDlgItemMessage(  hWnd, EDT_COLOR_B, WM_SETTEXT, 0, (LPARAM)logBuf );
            sprintf( logBuf, "%1.2f", c.a );
            sscanf( logBuf, "%f", &c.a );
            SendDlgItemMessage(  hWnd, EDT_COLOR_A, WM_SETTEXT, 0, (LPARAM)logBuf );

            VE_Message msg;                     //  Send this color to the editor
            msg.id = VE_MSG_SETCOLOR;           //  which will color any selected points,
            *((GG_ColorARGBf*)&msg.p) = c;      //  and become current pen color
            if( g_editor->handleMessage( msg ) == VE_DIRTY )
            {
              g_editor->paint();
              SwapBuffers( s_glhdc );
            }
          }
          break;

        case IDCANCEL:
          if( g_editor != null )
          {
            delete g_editor;
            g_editor = null;
          }
          DisableOpenGL( s_glhwnd, s_glhdc, s_glhrc );
          s_glhdc = NULL;
          s_glhrc = NULL;
          s_glhwnd = NULL;
          EndDialog( hWnd, 0 );
          return TRUE;
          break;
      }
      return TRUE;

    case WM_KEYDOWN:
      if( g_editor != null )
      {
        switch( (int)wParam )
        {
          case VK_DELETE:
            {
              VE_Message msg;
              msg.id = VE_MSG_DELETESEL;
              msg.p.set(0,0);
              msg.p2.set(0,0);
              if( g_editor->handleMessage(msg) == VE_DIRTY )
              {
                g_editor->paint();
                SwapBuffers( s_glhdc );
              }
            }
            break;
        }
      }
      return 0;

    case WM_DESTROY:
    case WM_CLOSE:
      if( g_editor != null )
      {
        delete g_editor;
        g_editor = null;
      }
      DisableOpenGL( s_glhwnd, s_glhdc, s_glhrc );
      s_glhdc = NULL;
      s_glhrc = NULL;
      s_glhwnd = NULL;
      EndDialog( hWnd, 0 );
      return TRUE;
  }

  return FALSE;
}


///////////////////////////////////////////////////////
//
//  WinMain
//

int WINAPI WinMain( HINSTANCE hInst,
                    HINSTANCE hPrevInst,
                    LPSTR strCmd,
                    int iCmdShow )
{
//  if( (g_hAccel = LoadAccelerators( g_hInst, "ACCELERATOR_1" )) == NULL )
//    MessageBox( NULL, "Failed to LoadAccelerators()", "Error", MB_OK );

  //
  //  Start the Editor (main) dialog
  //  (..Param used to send it the command-line string)
  //
  DialogBoxParam( hInst, "DLG_VECEDIT", NULL, (DLGPROC)VecEditorDlg, (LPARAM)strCmd );

  return 1;
}



//  Enable OpenGL
//
static VOID EnableOpenGL( HWND hWnd, HDC * hDC, HGLRC * hRC )
{
  if( g_bOpenGLEnabled )
    return;

  PIXELFORMATDESCRIPTOR pfd;
  int iFormat;

  // get the device context (DC)
  *hDC = GetDC( hWnd );

  // set the pixel format for the DC
  ZeroMemory( &pfd, sizeof( pfd ) );
  pfd.nSize = sizeof( pfd );
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW |
    PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  pfd.cDepthBits = 16;
  pfd.iLayerType = PFD_MAIN_PLANE;
  iFormat = ChoosePixelFormat( *hDC, &pfd );
  SetPixelFormat( *hDC, iFormat, &pfd );

  // create and enable the render context (RC)
  *hRC = wglCreateContext( *hDC );
  wglMakeCurrent( *hDC, *hRC );

  g_bOpenGLEnabled = true;
}

//  Disable OpenGL
//
static VOID DisableOpenGL( HWND hWnd, HDC hDC, HGLRC hRC )
{
  if( !g_bOpenGLEnabled )
    return;

  wglMakeCurrent( NULL, NULL );
  wglDeleteContext( hRC );
  ReleaseDC( hWnd, hDC );

  g_bOpenGLEnabled = false;
}
