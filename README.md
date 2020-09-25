# libminibmcapture

Super duper minimal C-style exported dynamic library, without dependencies,  
for acquiring video input from Blackmagic DeckLink devices on Windows.  
Truly a *minimum viable product* at this point in time.

API (as exported by libminibmcapture*.dll):  
```cpp
void get_version( char* out_version, uint32_t versionlen, uint32_t* out_minibmver );
uint32_t get_devices();
bool get_device( uint32_t index, char* out_name, uint32_t namelen, int64_t* out_id,
                 uint32_t* out_displaymodecount, uint32_t* out_flags );
bool get_device_displaymode( uint32_t device, uint32_t displaymode, uint32_t* out_width,
                             uint32_t* out_height, uint32_t* out_timescale,
                             uint32_t* out_frameduration, uint32_t* out_modecode );
bool start_capture_single( uint32_t index, uint32_t modecode, const char* source );
bool get_frame_bgra32_blocking( uint32_t* out_width, uint32_t* out_height,
                                uint8_t** out_buffer, uint32_t* out_index );
void stop_capture_single();
```

See the `test` project for usage in practice.
