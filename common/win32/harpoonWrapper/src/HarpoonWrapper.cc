// Harpoon.cc --- ActivityMonitor for W32
//
// Copyright (C) 2007 Ray Satiro <raysatiro@yahoo.com>
// Copyright (C) 2007, 2008 Rob Caelers <robc@krandor.org>
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

//

#include <assert.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

#include <windows.h>
#include <winuser.h>
#include "HarpoonWrapper.h"

using namespace std;

#define HARPOON_WRAPPER_WINDOW_CLASS "HarpoonWrapperNotificationWindow"

Harpoon::Harpoon()
{
}


Harpoon::~Harpoon()
{
  terminate();
}

bool
Harpoon::init(HINSTANCE hInstance)
{
  this->hInstance = hInstance;

  DWORD dwStyle, dwExStyle;

  dwStyle = WS_OVERLAPPED;
  dwExStyle = WS_EX_TOOLWINDOW;

    WNDCLASSEX wclass =
    {
      sizeof(WNDCLASSEX),
      0,
      harpoon_window_proc,
      0,
      0,
      hInstance,
      NULL,
      NULL,
      NULL,
      NULL,
      HARPOON_WRAPPER_WINDOW_CLASS,
      NULL
    };

  notification_class = RegisterClassEx(&wclass);
  if( !notification_class )
      return FALSE;

  notification_window = CreateWindowEx(
      dwExStyle,
      HARPOON_WINDOW_CLASS,
      HARPOON_WINDOW_CLASS,
      dwStyle,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      NULL,
      NULL,
      hInstance,
      NULL
      );

  if (!notification_window)
    {
      UnregisterClass(HARPOON_WINDOW_CLASS, hInstance);
      notification_class = 0;
      return FALSE;
    }

  init_critical_filename_list();

  bool debug = false;
  bool mouse_lowlevel;
  bool keyboard_lowlevel;

  bool default_mouse_lowlevel = false;
  if ( LOBYTE( LOWORD( GetVersion() ) ) >= 6)
    {
      default_mouse_lowlevel = true;
    }
  
  mouse_lowlevel = default_mouse_lowlevel;
  keyboard_lowlevel = true;

  if (!harpoon_init(critical_filename_list, (BOOL)debug))
    {
      return false;
    }

  return true;
}


//! Stops the activity monitoring.
void
Harpoon::terminate()
{
    harpoon_exit();
}



void
Harpoon::run()
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void
Harpoon::init_critical_filename_list()
{
  int i;

  // Task Manager is always on the critical_filename_list
  if( GetVersion() >= 0x80000000 )
  // Windows Me/98/95
      strcpy( critical_filename_list[ 0 ], "taskman.exe" );
  else if( !check_for_taskmgr_debugger( critical_filename_list[ 0 ] ) )
      strcpy( critical_filename_list[ 0 ], "taskmgr.exe" );

  for( i = 1; i < HARPOON_MAX_UNBLOCKED_APPS; ++i )
      critical_filename_list[ i ][ 0 ] = '\0';

  //int filecount = 0;
  //if( !CoreFactory::get_configurator()->
  //    get_value( "advanced/critical_files/filecount", filecount) || !filecount )
  //        return;

  //if( filecount >= HARPOON_MAX_UNBLOCKED_APPS )
  //// This shouldn't happen
  //  {
  //    filecount = HARPOON_MAX_UNBLOCKED_APPS - 1;
  //    CoreFactory::get_configurator()->
  //        set_value( "advanced/critical_files/filecount", filecount );
  //  }

  //char loc[40];
  //string buffer;
  //for( i = 1; i <= filecount; ++i )
  //  {
  //    sprintf( loc, "advanced/critical_files/file%d", i );
  //    if( CoreFactory::get_configurator()->
  //        get_value( loc, buffer) )
  //      {
  //        strncpy( critical_filename_list[ i ], buffer.c_str(), 510 );
  //        critical_filename_list[ i ][ 510 ] = '\0';
  //      }
  //  }
}


bool
Harpoon::check_for_taskmgr_debugger( char *out )
{
  HKEY hKey = NULL;
  LONG err;
  DWORD size;
  unsigned char *p, *p2, *buffer;

  // If there is a debugger for taskmgr, it's always critical
  err = RegOpenKeyExA( HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"
      "Image File Execution Options\\taskmgr.exe",
      0, KEY_QUERY_VALUE, &hKey );

  if( err != ERROR_SUCCESS )
    {
      RegCloseKey( hKey );
      return false;
    }

  // get the size, in bytes, required for buffer
  err = RegQueryValueExA( hKey, "Debugger", NULL, NULL, NULL, &size );

  if( err != ERROR_SUCCESS || !size )
    {
      RegCloseKey( hKey );
      return false;
    }

  if( !( buffer = (unsigned char *)malloc( size + 1 ) ) )
    {
      RegCloseKey( hKey );
      return false;
    }

  err = RegQueryValueExA( hKey, "Debugger", NULL, NULL, (LPBYTE)buffer, &size );

  if( err != ERROR_SUCCESS || !size )
    {
      free( buffer );
      RegCloseKey( hKey );
      return false;
    }

  buffer[ size ] = '\0';

  // get to innermost quoted
  for( p2 = buffer; *p2 == '\"'; ++p2 )
  ;
  if( p2 != buffer )
  // e.g. "my debugger.exe" /y /x
    {
      if( (p = _mbschr( p2, '\"' )) )
          *p = '\0';
    }
  else
  // e.g. debugger.exe /y /x
    {
      if( (p = _mbschr( p2, ' ' )) )
          *p = '\0';
    }

  // Search the path to find where the filename starts:
  if( (p = (unsigned char *)_mbsrchr( p2, '\\' )) )
  // Point to first (mb) filename character
      ++p;
  else
  // No path.
      p = p2;

  _mbstrncpy_lowercase( out, (char *)p, 510 );
  out[ 510 ] = '\0';

  RegCloseKey( hKey );
  free( buffer );
  return true;
}



LRESULT CALLBACK
Harpoon::harpoon_window_proc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int evt_type;
  evt_type = uMsg - WM_USER;

  if (evt_type >= 0 && evt_type < HARPOON_WRAPPER_EVENT__SIZEOF)
  {
    switch ((HarpoonWrapperEventType) evt_type)
      {
        case HARPOON_WRAPPER_BLOCK:
          harpoon_block_input();
        break;
      
        case HARPOON_WRAPPER_UNBLOCK:
          harpoon_unblock_input();
        break;
      } 
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
