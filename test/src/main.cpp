// libminibmcapture (c) 2020 noorus
// This software is licensed under the zlib license.
// See the LICENSE file which should be included with
// this source distribution for details.

#include <io.h>
#include <stdio.h>
#include <windows.h>
#include <cstdint>

#define USE_DYNAMIC

#ifdef USE_DYNAMIC

# undef MINIBM_STATIC
# include "libminibmcapture.h"

minibm::fn_get_version get_version = nullptr;
minibm::fn_get_devices get_devices = nullptr;
minibm::fn_get_device get_device = nullptr;
minibm::fn_get_device_displaymode get_device_displaymode = nullptr;
minibm::fn_start_capture_single start_capture_single = nullptr;
minibm::fn_get_frame_bgra32_blocking get_frame_bgra32_blocking = nullptr;
minibm::fn_stop_capture_single stop_capture_single = nullptr;

HMODULE lib = 0;

bool loadDynamically()
{
  lib = LoadLibraryW( L"libminibmcapture64.dll" );
  if ( !lib )
    return false;

  get_version = (minibm::fn_get_version)GetProcAddress( lib, "get_version" );
  get_devices = (minibm::fn_get_devices)GetProcAddress( lib, "get_devices" );
  get_device = (minibm::fn_get_device)GetProcAddress( lib, "get_device" );
  get_device_displaymode = (minibm::fn_get_device_displaymode)GetProcAddress( lib, "get_device_displaymode" );
  start_capture_single = (minibm::fn_start_capture_single)GetProcAddress( lib, "start_capture_single" );
  stop_capture_single = (minibm::fn_stop_capture_single)GetProcAddress( lib, "stop_capture_single" );
  get_frame_bgra32_blocking = (minibm::fn_get_frame_bgra32_blocking)GetProcAddress( lib, "get_frame_bgra32_blocking" );

  return ( get_version && get_devices && get_device && get_device_displaymode
    && start_capture_single && stop_capture_single && get_frame_bgra32_blocking );
}

void unloadDynamically()
{
  FreeLibrary( lib );
}

#else
# define MINIBM_STATIC
# include "libminibmcapture.h"
# pragma comment( lib, "libminibmcapture64" )
#endif

using namespace minibm;

int wmain( int argc, wchar_t** argv, wchar_t** env )
{
  if ( FAILED( CoInitializeEx( nullptr, COINIT_MULTITHREADED ) ) )
  {
    printf( "COM init failed\r\n" );
    return 1;
  }

#ifdef USE_DYNAMIC
  if ( !loadDynamically() )
  {
    printf( "Dynamical library load failed.\r\nLibrary or needed exports not found.\r\n" );
    return 1;
  }
#endif

  char verstr[256] = { 0 };
  uint32_t minibmVer = 0;
  get_version( verstr, 256, &minibmVer );
  printf( "get_version: %s / minibmcap API version %i\r\n", verstr, minibmVer );

  uint32_t deviceCount = get_devices();
  printf( "get_devices: %i\r\n", deviceCount );

  uint32_t dev0DispModeCount = 0;
  for ( uint32_t i = 0; i < deviceCount; ++i )
  {
    char namestr[256] = { 0 };
    int64_t id = 0;
    uint32_t dms = 0;
    uint32_t flags = 0;
    auto ret = get_device( i, namestr, 256, &id, &dms, &flags );
    printf( "get_device %i: %s, id %I64x, %i display modes, %s autodetect display mode\r\n",
      i, ret ? namestr : "failed", id, dms,
      ( flags & DeviceFlags::Device_CanAutodetectDisplayMode ) ? "can" : "can't"
    );
    if ( i == 0 )
      dev0DispModeCount = dms;
  }

  if ( deviceCount > 0 )
  {
    for ( uint32_t i = 0; i < dev0DispModeCount; ++i )
    {
      uint32_t w, h, timescale, duration, code;
      get_device_displaymode( 0, i, &w, &h, &timescale, &duration, &code );
      printf( "get_device_displaymode: %ix%ip %.2f (code %x)\r\n", w, h, (float)timescale / (float)duration, code );
    }
    auto ret = start_capture_single( 0, 0x48703330, "" );
    printf( "start_capture_single: %s\r\n", ret ? "true" : "false" );
    size_t ctr = 0;
    if ( ret )
    {
      printf( "entering get_frame loop\r\n" );
      while ( ctr < 100 )
      {
        uint32_t width, height, frameidx;
        uint8_t* buf;
        if ( get_frame_bgra32_blocking( &width, &height, &buf, &frameidx ) )
        {
          printf( "get_frame_bgra32_blocking: got frame %i, size %ix%i (at 0x%I64x)\r\n", frameidx, width, height, reinterpret_cast<uint64_t>( buf ) );
        }
        ctr++;
        // Sleep( 100 );
      }
      stop_capture_single();
      printf( "stop_capture_single\r\n" );
    }
  }

#ifdef USE_DYNAMIC
  unloadDynamically();
#endif

  CoUninitialize();

  return 0;
}