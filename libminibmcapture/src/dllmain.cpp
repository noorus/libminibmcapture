// libminibmcapture (c) 2020 noorus
// This software is licensed under the zlib license.
// See the LICENSE file which should be included with
// this source distribution for details.

#include "pch.h"
#include "minibmcap.h"

#include <sstream>
#include <iomanip>

using namespace std;

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

string escape(string str) {
    std::ostringstream o;
    for (auto c = str.cbegin(); c != str.cend(); c++) {
        if (*c == '"' || *c == '\\' || ('\x00' <= *c && *c <= '\x1f')) {
            o << "\\u"
                << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
        }
        else {
            o << *c;
        }
    }
    return o.str();
}

static string json_buffer;

extern "C" {

  // Could be used to figure out API/ABI compatibility stuff later, if the library evolves
  const uint32_t c_myVersion = 2;

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

  int MINIBM_EXPORT get_json_length() {
      char dev_name[2048];
      ostringstream ss;
      int n = get_devices();
      ss << "[";
      for (int dev = 0; dev < n; dev++) {
          if (dev > 0)
              ss << ",";
          int64_t bm_id;
          uint32_t mode_count;
          uint32_t bm_flags;
          if (!get_device(dev, dev_name, 2048, &bm_id, &mode_count, &bm_flags))
              break;
          ss << "{\"id\": " << dev << ",";
          ss << "\"name\": \"" << escape(string(dev_name)) << "\",";
          ss << "\"bmId\": \"" << bm_id << "\",";
          ss << "\"bmFlags\": \"" << bm_flags << "\",";
          ss << "\"caps\": [";
          for (uint32_t dcap = 0; dcap < mode_count; dcap++) {
              uint32_t width;
              uint32_t height;
              uint32_t timescale;
              uint32_t frameduration;
              uint32_t modecode;
              if (!get_device_displaymode(dev, dcap, &width, &height, &timescale, &frameduration, &modecode))
                  break;

              double fps = (double)timescale / (double)frameduration;
              int interval = (int)(10000000 / fps);

              if (dcap > 0)
                  ss << ",";
              ss << "{";
              ss << "\"id\": " << dcap << ",";
              ss << "\"minCX\": " << width << ",";
              ss << "\"minCY\": " << height << ",";
              ss << "\"maxCX\": " << width << ",";
              ss << "\"maxCY\": " << height << ",";
              ss << "\"granularityCX\": " << 1 << ",";
              ss << "\"granularityCY\": " << 1 << ",";
              ss << "\"minInterval\": " << interval << ",";
              ss << "\"maxInterval\": " << interval << ",";
              ss << "\"bmTimescale\": " << timescale << ",";
              ss << "\"bmFrameduration\": " << frameduration << ",";
              ss << "\"bmModecode\": " << modecode << ",";
              ss << "\"rating\": 1,";
              ss << "\"format\": 100";
              ss << "}";
          }
          ss << "]}";
      }
      ss << "]";
      json_buffer = ss.str();
      return (int)json_buffer.length() + 1;
  }

  void MINIBM_EXPORT get_json(char *out_buffer, uint32_t buffer_length) {
      strncpy(out_buffer, json_buffer.c_str(), buffer_length);
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