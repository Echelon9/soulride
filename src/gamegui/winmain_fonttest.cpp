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
//  Author: Mike Linkovich
//
//  start date: Feb 8 2000
//
////////////////////////////////


//
//  Includes
//

#define WIN32_LEAN_AND_MEAN   //  Minimize includes
#include <windows.h>

#include "gl/gl.h"            //  For OpenGL

#include "gamegui.h"          //  GameGUI main header

//
//  Function Declarations
//
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
VOID EnableOpenGL( HWND hWnd, HDC * hDC, HGLRC * hRC );
VOID DisableOpenGL( HWND hWnd, HDC hDC, HGLRC hRC );

char strOutput[] = "    My name is Boris Balkan and I once translated \"The Charterhouse of Parma\". Apart from that, I've edited a few books on the nineteenth-century popular novel, my reviews and articles appear in supplements and journals throughout Europe, and I organize summer-school courses on contemporary writers. Nothing spectacular, I'm afraid. Particularly these days, when suicide disguises itself as homicide, novels are written by Roger Ackroyd's doctor, and far too many people insist on publishing two hundred pages on the fascinating emotions they experience when they look in the mirror.\n"
    "But let's stick to the story.\n"
    "I first met Lucas Corso when he came to see me; he was carrying \"The Anjou Wine\" under his arm. Corso was a mercenary of the book world, hunting down books for other people. That meant talking fast and getting his hands dirty. He needed good reflexes, patience, and a lot of luck - and a prodigious memory to recall the exact dusty corner of an old man's shop where a book now worth a fortune lay forgotten.";

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
  GameGUI       *gg = null;                 //  GameGUI objects
  GG_Font       *font = null;
  GG_Rect2D     rcText = { -0.9, 0.9, 0.9, -0.9 };
  float         fntSize = 0.08;
  GG_Rect       rcgg;
  GG_FontFX     fontFx;
  GG_Rval       rval = GG_OK;

  bool          bQuit = false;              //  Quit flag
  int32         widthWin, heightWin,        //  Window sizing calcs
                widthScreen, heightScreen;  //  Screen size

  WNDCLASS  wc;                             //  windoze
  HWND      hWnd = NULL;
  HDC       hDC = NULL;
  HGLRC     hRC = NULL;
  MSG       msg;


  //
  //  Find switches marked with a '-' character.
  //  Get the info, then truncate the string leaving
  //  only the filename (which must be the first param)
  //
  if( strlen(strCmd) > 1 )
  {
    strncpy( strOutput, strCmd, 255 );
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
  wc.lpszClassName = "FontTest";
  RegisterClass( &wc );

  widthScreen = GetSystemMetrics(SM_CXFULLSCREEN);
  heightScreen = GetSystemMetrics(SM_CYFULLSCREEN);

  hWnd = CreateWindowEx( WS_EX_DLGMODALFRAME,
                         "FontTest", "FontTest",
                         (WS_OVERLAPPEDWINDOW) & (~(WS_MAXIMIZEBOX | WS_THICKFRAME)),
                         (widthScreen - g_APP_WIDTH) / 2,
                         (heightScreen - g_APP_HEIGHT) / 2,
                         g_APP_WIDTH, g_APP_HEIGHT,
                         NULL, NULL, hInstance, NULL );

  if( hWnd == null )
  {
    MessageBox( NULL, "Failed to create window", "Error:", MB_OK | MB_ICONSTOP );
    goto exitNow;
  }

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


  //
  //  enable OpenGL for the window
  //
  EnableOpenGL( hWnd, &hDC, &hRC );

  //
  //  Create the GameGUI object
  //
  GetClientRect( hWnd, (LPRECT)&rcgg );
  rval = GameGUICreate( &rcgg, null, 0, &gg );

  if( rval != GG_OK )
  {
    MessageBox( hWnd, "Failed to create GameGUI", "Error", MB_OK | MB_ICONSTOP );
    goto exitNow;
  }

  if( gg->loadFont( "swiss-xcbi.ggf", &font ) != GG_OK )
  {
    MessageBox( hWnd, "Failed to load font 'score.ggf'", "Error", MB_OK | MB_ICONSTOP );
    goto exitNow;
  }

  gg->setFont(font);
  gg->setFontSize( fntSize );
  gg->setFontSpacing( 0.0 );
  gg->setFontStretch( 1.0 );
  gg->setFontLeading( fntSize * 1.25 );
  fontFx.flags = GG_FONTFX_COLORMOD;
  fontFx.clrMod.set( 1.0, 0.0, 1.0, 0.9 );

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

      //  Test font by drawing some characters..
      gg->drawTextBox( strOutput, &rcText, GG_TEXTALIGN_LEFT, &fontFx );

      SwapBuffers( hDC );     //  Flip buffer

      Sleep(100);
    }
  }


exitNow:

  //  shutdown GameGUI
  if( font != null )
  {
    font->unRef();
    font = null;
  }
  if( gg != null )
  {
    delete gg;
    gg = null;
  }

  // shutdown OpenGL
  DisableOpenGL( hWnd, hDC, hRC );

  // destroy the window explicitly
  if( hWnd != NULL )
  {
    DestroyWindow(hWnd);
    hWnd = null;
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
  }

  return DefWindowProc( hWnd, message, wParam, lParam );
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
