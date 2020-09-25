// libminibmcapture (c) 2020 noorus
// This software is licensed under the zlib license.
// See the LICENSE file which should be included with
// this source distribution for details.

#include "pch.h"
#include "minibmcap.h"

static minibm::DecklinkCapture* g_cap = nullptr;
static minibm::DecklinkDeviceVector g_devices;

inline minibm::DecklinkCapture& getCap()
{
  if ( !g_cap )
  {
    g_cap = new minibm::DecklinkCapture();
    g_cap->initialize();
  }
  return *g_cap;
}

inline void resetCap()
{
  if ( g_cap )
    delete g_cap;
  g_cap = nullptr;
}

extern "C" {

  // Could be used to figure out API/ABI compatibility stuff later, if the library evolves
  const uint32_t c_myVersion = 1;

  void MINIBM_EXPORT get_version( char* out_version, uint32_t versionlen, uint32_t* out_minibmver )
  {
    auto ver = minibm::DecklinkCapture::getVersion();
    strcpy_s( out_version, versionlen, ver.c_str() );
    *out_minibmver = c_myVersion;
  }

  uint32_t MINIBM_EXPORT get_devices()
  {
    g_devices = getCap().getDevices();
    return static_cast<uint32_t>( g_devices.size() );
  }

  bool MINIBM_EXPORT get_device( uint32_t index, char* out_name, uint32_t namelen, int64_t* out_id, uint32_t* out_displaymodecount, uint32_t* out_flags )
  {
    if ( g_devices.empty() || index >= g_devices.size() )
      return false;

    strcpy_s( out_name, namelen, g_devices[index]->name_.c_str() );
    *out_id = g_devices[index]->id_;
    *out_displaymodecount = static_cast<uint32_t>( g_devices[index]->displayModes_.size() );

    uint32_t flags = 0;
    if ( g_devices[index]->hasFormatDetection_ )
      flags |= minibm::Device_CanAutodetectDisplayMode;

    *out_flags = flags;

    return true;
  }

  bool MINIBM_EXPORT get_device_displaymode( uint32_t device, uint32_t displaymode, uint32_t* out_width, uint32_t* out_height, uint32_t* out_timescale, uint32_t* out_frameduration, uint32_t* out_modecode )
  {
    if ( g_devices.empty() || device >= g_devices.size() )
      return false;

    if ( displaymode >= g_devices[device]->displayModes_.size() )
      return false;

    auto dm = &g_devices[device]->displayModes_[displaymode];
    *out_width = dm->width_;
    *out_height = dm->height_;
    *out_timescale = static_cast<uint32_t>( dm->timeScale_ );
    *out_frameduration = static_cast<uint32_t>( dm->frameDuration_ );
    *out_modecode = dm->value_;

    return true;
  }

  bool MINIBM_EXPORT start_capture_single( uint32_t index, uint32_t modecode, const char* source )
  {
    if ( g_devices.empty() || index >= g_devices.size() )
      return false;

    return getCap().startCaptureSingle( g_devices[index], static_cast<BMDDisplayMode>( modecode ) );
  }

  bool MINIBM_EXPORT get_frame_bgra32_blocking( uint32_t* out_width, uint32_t* out_height, uint8_t** out_buffer, uint32_t* out_index )
  {
    minibm::BGRA32VideoFrame* frame;
    uint32_t index;
    auto ret = getCap().getFrameBlocking( &frame, index );
    if ( !ret )
      return false;

    *out_width = frame->GetWidth();
    *out_height = frame->GetHeight();
    *out_buffer = const_cast<uint8_t*>( frame->buffer().data() );
    *out_index = index;
    return true;
  }

  void MINIBM_EXPORT stop_capture_single()
  {
    getCap().stopCaptureSingle();
  }

}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
  switch ( ul_reason_for_call )
  {
    case DLL_PROCESS_ATTACH:
      minibm::g_globals.initialize();
      break;
    case DLL_PROCESS_DETACH:
      resetCap();
      //minibm::g_globals.shutdown();
      break;
  }
  return TRUE;
}