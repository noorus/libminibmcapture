#define NOMINMAX
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>
#include <ctype.h>
#include <string.h>
#include <exception>
#include <math.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstdint>
#include <strsafe.h>

enum DeviceFlags: uint32_t {
  Device_CanAutodetectDisplayMode = 1
};

typedef void( __stdcall* fn_get_version )( char* out_version, uint32_t versionlen, uint32_t* out_minibmver );
typedef uint32_t( __stdcall* fn_get_devices )();
typedef bool( __stdcall* fn_get_device )( uint32_t index, char* out_name, uint32_t namelen, int64_t* out_id, uint32_t* out_displaymodecount, uint32_t* out_flags );
typedef bool( __stdcall* fn_get_device_displaymode )( uint32_t device, uint32_t displaymode, uint32_t* out_width, uint32_t* out_height, uint32_t* out_timescale, uint32_t* out_frameduration, uint32_t* out_modecode );
typedef bool( __stdcall* fn_start_capture_single )( uint32_t index, const char* capturemode );
typedef void( __stdcall* fn_stop_capture_single )();

int wmain( int argc, wchar_t** argv, wchar_t** env )
{
  if ( FAILED( CoInitializeEx( nullptr, COINIT_MULTITHREADED ) ) )
  {
    printf( "COM init failed\r\n" );
    return 1;
  }

  auto lib = LoadLibraryW( L"libminibmcapture64.dll" );
  if ( !lib )
  {
    printf( "Couldn't load library\r\n" );
    return 1;
  }

  auto get_version = (fn_get_version)GetProcAddress( lib, "get_version" );
  if ( get_version )
  {
    char verstr[256] = { 0 };
    uint32_t minibmVer = 0;
    get_version( verstr, 256, &minibmVer );
    printf( "get_version: %s / minibmcap API version %i\r\n", verstr, minibmVer );
  }

  uint32_t deviceCount = 0;
  auto get_devices = (fn_get_devices)GetProcAddress( lib, "get_devices" );
  if ( get_devices )
  {
    deviceCount = get_devices();
    printf( "get_devices: %i\r\n", deviceCount );
  }

  uint32_t dev0DispModeCount = 0;

  auto get_device = (fn_get_device)GetProcAddress( lib, "get_device" );
  if ( get_device )
  {
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
  }

  auto get_device_displaymode = (fn_get_device_displaymode)GetProcAddress( lib, "get_device_displaymode" );
  if ( get_device_displaymode && deviceCount > 0 )
  {
    for ( uint32_t i = 0; i < dev0DispModeCount; ++i )
    {
      uint32_t w, h, timescale, duration, code;
      get_device_displaymode( 0, i, &w, &h, &timescale, &duration, &code );
      printf( "get_device_displaymode: %ix%ip %.2f (code %x)\r\n", w, h, (float)timescale / (float)duration, code );
    }
    /*auto start_capture_single = (fn_start_capture_single)GetProcAddress( lib, "start_capture_single" );
    if ( start_capture_single && deviceCount > 0 )
    {
      auto ret = start_capture_single( 0, "" );
      printf( "start_capture_single: %s\r\n", ret ? "true" : "false" );
    }*/
  }

  FreeLibrary( lib );

  CoUninitialize();

  return 0;
}