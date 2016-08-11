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
//  Stand-Alone GameGUI player application
//
//  winmain.cpp
//
//  (c) 2000 by Mike Linkovich
//
//  start date: Feb 8 2000
//
//  OpenGL initialization derived from:
//  GLSAMPLE.CPP by Blaine Hodge
//
////////////////////////////////


//
//  Includes
//

#include "gamegui.h"          //  GameGUI main header

#include "gg_string.h"
#include "gg_audio.h"         //  GG_Audio Used only by this file, for sound loading
                              //  utilities.  DSound buffers are passed to
                              //  GameGUI via callbacks.

// #define INITGUID
#define WIN32_LEAN_AND_MEAN   //  Minimize includes
#include <windows.h>
#include <commdlg.h>          //  For Open File Dialog
#include <mmsystem.h>         //  For timeGetTime()
#include <objbase.h>          //  For CoInitialize() -- dsound.lib, dxguid.lib need not be linked
#include "gl/gl.h"            //  For OpenGL


//
//  Function Declarations
//
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
VOID EnableOpenGL( HWND hWnd, HDC * hDC, HGLRC * hRC );
VOID DisableOpenGL( HWND hWnd, HDC hDC, HGLRC hRC );
GG_Rval loadSndCallback( void *appObj, char *fileName, void **newSnd );

//
//  Globals
//
static int32 g_APP_WIDTH = 640;
static int32 g_APP_HEIGHT = 480;

static char logBuf[256] = "";


///////////////////////////////////////////////////////
//
//  WinMain
//

int WINAPI WinMain( HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR strCmd,
                    int iCmdShow )
{
  bool          bUseAudio = true;           //  Use/don't use audio
  GG_AudioSetup audioSetup;                 //  Needed to create audio obj.
  GG_Audio      *audio = null;              //  Note that audio is a separate component to GameGUI.
                                            //  It may be used as a standalone DSound wrapper

  GG_Callbacks  callbacks;                  //  All app callback functions needed by GameGUI
  GameGUI       *gg = null;                 //  GameGUI objects
  GG_Movie      *movie = null;
  GG_Player     *player = null;
  GG_Rect       rcgg;
  char          fileName[256] = "",
                *pStr = null;
  GG_Rval       rval = GG_OK;

  uint32        prevT = 0,                  //  Timing
                curT = 0,
                dT = 0;
  int           n = 0;                      //  whatever
  bool          bQuit = false;              //  Quit flag
  bool          bCoInited = false;          //  COM initialized flag
  int32         widthWin, heightWin,        //  Window sizing calcs
                widthScreen, heightScreen;  //  Screen size

  WNDCLASS wc;                              //  windoze
  HWND hWnd = NULL;
  HDC hDC = NULL;
  HGLRC hRC = NULL;
  MSG msg;

  //
  //  Find switches marked with a '-' character.
  //  Get the info, then truncate the string leaving
  //  only the filename (which must be the first param)
  //
  if( strlen(strCmd) > 1 )
  {
    char *pTrunc = null;

    if( (pStr = strstr(strCmd, "-width=")) != null )
    {
      sscanf( pStr, "-width=%d", &g_APP_WIDTH );
      pTrunc = pStr;
    }
    if( (pStr = strstr(strCmd, "-height=")) != null )
    {
      sscanf( pStr, "-height=%d", &g_APP_HEIGHT );
      if( pTrunc == null || pStr < pTrunc )
        pTrunc = pStr;
    }
    if( (pStr = strstr(strCmd, "-noaudio")) != null )
    {
      bUseAudio = false;
      if( pTrunc == null || pStr < pTrunc )
        pTrunc = pStr;
    }

    if( pTrunc != null )
      *pTrunc = '\0';       //  Chop where the first switch was found
  }

  //  Check for filename parameter override.
  //  this must appear before the first -switch string encountered,
  //  as they are shaved off the end of the string.
  //
  if( strlen(strCmd) > 1 )
  {
    GG_String::stripComment(strCmd);  // strip leading/trailing spaces, etc.
    strncpy( fileName, strCmd, 255 );
  }

  if( strlen(fileName) < 1 )
  {
    //  Get filename from Open File dialog..
    OPENFILENAME fileInfo;
    memset( &fileInfo, 0, sizeof(OPENFILENAME) );
    fileInfo.lStructSize = sizeof(OPENFILENAME);
    fileInfo.hwndOwner = hWnd;
    fileInfo.lpstrFilter = "Movie files (*.ggm)\0*.ggm\0\0";
    fileInfo.lpstrFile = fileName;
    fileInfo.lpstrTitle = " Open movie file...";
    fileInfo.nMaxFile = 250;
    fileInfo.Flags = OFN_NOCHANGEDIR;

    if( GetOpenFileName( &fileInfo ) == 0 )
      goto exitNow;
  }

  //  Init COM for our DirectX-based components
  //  (if we need audio, that's the only DX component we need)

  if( bUseAudio )
  {
    if( CoInitialize(NULL) == S_OK )
      bCoInited = true;
    else
    {
      MessageBox( NULL, "Failed to initialize COM.  Sound willl not be available", "Error:", MB_OK | MB_ICONSTOP );
      bUseAudio = false;
    }
  }

  // register window class
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
  wc.hCursor = LoadCursor( NULL, IDC_ARROW );
  wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
  wc.lpszMenuName = NULL;
  wc.lpszClassName = "ggplayer";
  RegisterClass( &wc );

  sprintf( logBuf, "GameGUI Player - '%s'", fileName );

  widthScreen = GetSystemMetrics(SM_CXFULLSCREEN);
  heightScreen = GetSystemMetrics(SM_CYFULLSCREEN);

  hWnd = CreateWindowEx( WS_EX_DLGMODALFRAME,
                         "ggplayer", logBuf,
                         (WS_OVERLAPPEDWINDOW) & (~(WS_MAXIMIZEBOX | WS_THICKFRAME)),
                         (widthScreen - g_APP_WIDTH) / 2,
                         (heightScreen - g_APP_HEIGHT) / 2,
                         g_APP_WIDTH, g_APP_HEIGHT,
                         NULL, NULL, hInstance, NULL );

  if( hWnd == null )
  {
    MessageBox( NULL, "Failed to create player window", "Error:", MB_OK | MB_ICONSTOP );
    goto exitNow;
  }

//  ShowWindow( hWnd,  );
//  UpdateWindow( hWnd );

  //  Does it need resizing?
  //  (we must ensure client area = APP_WIDTH x APP_HEIGHT)
  //
  GetClientRect( hWnd, (LPRECT)&rcgg );
  if( (rcgg.x2 - rcgg.x1) != g_APP_WIDTH )
    widthWin = g_APP_WIDTH + (g_APP_WIDTH - (rcgg.x2 - rcgg.x1));
  if( (rcgg.y2 - rcgg.y1) != g_APP_HEIGHT )
    heightWin = g_APP_HEIGHT + (g_APP_HEIGHT - (rcgg.y2 - rcgg.y1));
  MoveWindow( hWnd,
              (widthScreen - widthWin) / 2,
              (heightScreen - heightWin) / 2,
              widthWin, heightWin, FALSE );

  ShowWindow( hWnd, iCmdShow );
  UpdateWindow( hWnd );

  //  Create the audio object
  //
  memset( &callbacks, 0, sizeof(callbacks) );   //  default to nulls (no audio) in case of screwup
  if( bUseAudio )
  {
    audioSetup.hWnd = hWnd;
    audioSetup.hInst = hInstance;

    if( GG_AudioCreate( audioSetup, &audio ) != GG_OK )
    {
      MessageBox( hWnd, "Failed to initialize audio", "Error", MB_OK | MB_ICONSTOP );
      audio = null;
      bUseAudio = false;
    }
    else
    {
      callbacks.loadSound = loadSndCallback;
      callbacks.loadSoundObj = audio;
    }
  }

  //
  //  enable OpenGL for the window
  //
  EnableOpenGL( hWnd, &hDC, &hRC );

  //
  //  Create the GameGUI object
  //
  GetClientRect( hWnd, (LPRECT)&rcgg );

  if( bUseAudio )
    rval = GameGUICreate( &rcgg, &callbacks, 0, &gg );
  else
    rval = GameGUICreate( &rcgg, null, 0, &gg );

  if( rval != GG_OK )
  {
    MessageBox( hWnd, "Failed to create GameGUI", "Error", MB_OK | MB_ICONSTOP );
    goto exitNow;
  }

  //  Load movie
  if( gg->loadMovie( fileName, &movie ) != GG_OK )
  {
    MessageBox( hWnd, fileName, "Failed to load movie file", MB_OK );
    goto exitNow;
  }

  if( gg->createPlayer( movie, &player ) != GG_OK )
  {
    MessageBox( hWnd, "Failed to create player", "Error", MB_OK );
    movie->unRef();
    movie = null;
    goto exitNow;
  }

  player->setPlayMode(GG_PLAYMODE_LOOP);        //  Tell our movie to loop

  movie->unRef();
  movie = null;     //  Not needed anymore

  prevT = timeGetTime();

  // program main loop
  while ( !bQuit )
  {
    // check for messages
    if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    {
      // handle or dispatch messages
      if ( msg.message == WM_QUIT )
        bQuit = TRUE;
      else
      {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
      }
    }
    else
    {
      //  App must clear screen
      glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
      glClear( GL_COLOR_BUFFER_BIT );

      //  Get time elapsed
      curT = timeGetTime();
      dT = curT - prevT;
//      if( dT > 200 )
//        dT = 200;

      //
      //  Play Player
      //
      if( player != null )
        player->play(dT);

      SwapBuffers( hDC );     //  Flip buffer
      prevT = curT;           //  Update time
    }
  }


exitNow:

  //  shutdown GameGUI
  if( player != null )
  {
    player->unRef();
    player = null;
  }
  if( movie != null )
  {
    movie->unRef();
    movie = null;
  }
  if( gg != null )
  {
    delete gg;
    gg = null;
  }
  if( audio != null )
  {
    audio->unRef();
    audio = null;
  }

  // shutdown OpenGL
  DisableOpenGL( hWnd, hDC, hRC );

  // destroy the window explicitly
  if( hWnd != NULL )
  {
    DestroyWindow(hWnd);
    hWnd = null;
  }

  //  un-init COM
  if( bCoInited )
  {
    CoUninitialize();
    bCoInited = false;
  }

  return msg.wParam;

}


// Window Procedure

static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
  switch ( message )
  {
    case WM_CREATE:
      return 0;

    case WM_CLOSE:
      PostQuitMessage( 0 );
      return 0;

    case WM_DESTROY:
      return 0;

    case WM_KEYDOWN:
      switch ( wParam )
      {
        case VK_ESCAPE:
          PostQuitMessage( 0 );
          return 0;
      }
      return 0;

    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
      }
      return 0;

    default:
      return DefWindowProc( hWnd, message, wParam, lParam );
  }
}


//  Enable OpenGL
//
static VOID EnableOpenGL( HWND hWnd, HDC * hDC, HGLRC * hRC )
{
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
}

//  Disable OpenGL
//
static VOID DisableOpenGL( HWND hWnd, HDC hDC, HGLRC hRC )
{
  wglMakeCurrent( NULL, NULL );
  wglDeleteContext( hRC );
  ReleaseDC( hWnd, hDC );
}


//  Sound loader callback, supplied to GameGUI
//
GG_Rval loadSndCallback( void *appObj, char *fileName, void **dsSnd )
{
  GG_Audio *audio = (GG_Audio *)appObj;
  GG_Sound *snd = null;
  if( (snd = audio->createSound(fileName)) == null )
    return GG_ERR;
  *dsSnd = snd->getDSSoundBuffer1();

  //  Note that we didn't destroy the GG_Sound object we created here!
  //  Gonna let it slide, and allow the destruction of audio to take
  //  care of it in the main loop.

  return GG_OK;
}
