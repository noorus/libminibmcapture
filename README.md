# libminibmcapture

Super duper minimal C-style exported dynamic library, without dependencies,  
for acquiring video input from Blackmagic DeckLink devices on Windows.  
Truly a *minimum viable product* at this point in time.

API (as exported by libminibmcapture*.dll):  
```cpp
//! \fn void __stdcall get_version( char* out_version, uint32_t versionlen, uint32_t* out_minibmver );
//! \brief Get the library version.
//! \param [out] out_version   Pointer to a buffer where the version string will be copied.
//! \param       versionlen    Length of the buffer pointed to by out_version.
//! \param [out] out_minibmver Pointer to a variable that will receive the libminibmcapture API version number.
void get_version( char* out_version, uint32_t versionlen, uint32_t* out_minibmver );

//! \fn uint32_t __stdcall get_devices();
//! \brief Get the number of available Blackmagic devices.
//! \returns The number of devices.
uint32_t get_devices();

//! \fn void __stdcall set_options( const char* library_options );
//! \brief Sets a global options string for the library.
//! \param library_options A properly formatted options string.
//!                        Can be empty or null if no extra options are needed.
void set_options( const char* library_options );

//! \fn bool __stdcall get_device( uint32_t index, char* out_name, uint32_t namelen, int64_t* out_id, uint32_t* out_displaymodecount, uint32_t* out_flags );
//! \brief Get the details of a specific Blackmagic device.
//! \param       index                 Zero-based index of the target device.
//! \param [out] out_name              Pointer to a buffer where the device name will be copied.
//! \param       namelen               Length of the buffer pointed to by out_name.
//! \param [out] out_id                Pointer to a variable that will receive the persistent unique device ID.
//! \param [out] out_displaymodecount  Pointer to a variable that will receive the number of available display modes on the device.
//! \param [out] out_flags             Pointer to a variable that will receive the device flags. \see DeviceFlags
//! \returns True if it succeeds, false if it fails.
bool get_device( uint32_t index, char* out_name, uint32_t namelen, int64_t* out_id, uint32_t* out_displaymodecount, uint32_t* out_flags );

//! \fn bool __stdcall get_device_displaymode( uint32_t device, uint32_t displaymode, uint32_t* out_width, uint32_t* out_height, uint32_t* out_timescale, uint32_t* out_frameduration, uint32_t* out_modecode );
//! \brief Get the details of a specifics display mode on a specific Blackmagic device.
//!        The display mode's FPS can be calculated by: (timescale / frameduration)
//! \param       device            Zero-based index of the target device.
//! \param       displaymode       Zero-based index of the target display mode.
//! \param [out] out_width         Pointer to a variable that will receive the width in pixels.
//! \param [out] out_height        Pointer to a variable that will receive the height in pixels.
//! \param [out] out_timescale     Pointer to a variable that will receive the time scale.
//! \param [out] out_frameduration Pointer to a variable that will receive the frame duration.
//! \param [out] out_modecode      Pointer to a variable that will receive the internal unique code for this display mode.
//! \returns True if it succeeds, false if it fails.
bool get_device_displaymode( uint32_t device, uint32_t displaymode, uint32_t* out_width, uint32_t* out_height, uint32_t* out_timescale, uint32_t* out_frameduration, uint32_t* out_modecode );

//! \fn bool __stdcall start_capture_single( uint32_t index, uint32_t modecode, const char* source );
//! \brief Starts capturing on a single Blackmagic device.
//! \param index           Zero-based index of the target device.
//! \param modecode        The unique code of the display mode to use. Can be found by enumerating get_device_displaymode.
//! \param capture_options A properly formatted string of extra options on how the capture should behave.
//!                        Can be empty or null if no extra options are needed.
//! \returns True if it succeeds, false if it fails.
bool start_capture_single( uint32_t index, uint32_t modecode, const char* capture_options );

//! \fn bool __stdcall get_frame_bgra32_blocking( uint32_t* out_width, uint32_t* out_height, uint8_t** out_buffer, uint32_t* out_index );
//! \brief Get a single frame from the currently ongoing capture.
//!        The call might block until a new frame is available.
//!        It does not have to be called with any exact timing.
//!        It will always return the latest received frame, and never the same frame twice.
//!        The frame index can be used to figure out the number of possibly skipped frames.
//!        The image format is always 32-bit BGRA.
//! \param [out] out_width  Pointer to a variable that will receive the frame width in pixels.
//! \param [out] out_height Pointer to a variable that will receive the frame height in pixels.
//! \param [out] out_buffer Pointer to a variable that will receive a pointer to the data buffer.
//!              The buffer and data in it will be valid until the next get_frame call or stopped capture.
//! \param [out] out_index  Pointer to a variable that will receive the index of the returned frame.
//!              Frame index counting will always restart from zero when starting a new capture.
//! \returns True if it succeeds, false if it fails.
bool get_frame_bgra32_blocking( uint32_t* out_width, uint32_t* out_height, uint8_t** out_buffer, uint32_t* out_index );

//! \fn void __stdcall stop_capture_single();
//! \brief Stop capturing on a single Blackmagic device.
void stop_capture_single();

//! \fn int __stdcall get_json_length();
//! \brief Gets the length of the needed buffer for JSON output.
//! \returns The JSON length in bytes.
int get_json_length();

//! \fn void __stdcall get_json(char *out_buffer, uint32_t buffer_length);
//! \brief Fills a buffer with a JSON dump of all supported device and display mode data.
//! \param [out] out_buffer Pointer to a buffer that will receive the JSON dump.
//! \param       buffer_length Length of the buffer in bytes.
void get_json(char *out_buffer, uint32_t buffer_length);

bool read_frame_bgra32_blocking(uint8_t *buffer, uint32_t len)
```

See the `test` project for usage in practice.
