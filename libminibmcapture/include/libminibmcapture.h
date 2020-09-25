#pragma once

#include "pch.h"
#include "utils.h"

#include "decklink_api/DeckLinkAPIVersion.h"
#include "DeckLinkAPI_h.h"

namespace minibm {

  struct Globals {
    bool comInitialized_;
    Globals();
    bool initialize();
    void shutdown();
  };

  extern Globals g_globals;

  class BGRA32VideoFrame: public IDeckLinkVideoFrame {
  private:
    long width_;
    long height_;
    BMDFrameFlags flags_;
    vector<uint8_t> buffer_;
    atomic<uint32_t> refCount_;
  public:
    BGRA32VideoFrame(): width_( 0 ), height_( 0 ), flags_( 0 ), refCount_( 1 ) {}
    BGRA32VideoFrame( long width, long height, BMDFrameFlags flags ):
      width_( width ), height_( height ), flags_( flags ), refCount_( 1 )
    {
      buffer_.resize( width_ * height_ * 4 );
    }
    inline void resize( long width, long height )
    {
      width_ = width;
      height_ = height;
      buffer_.resize( width_ * height_ * 4 );
    }
    inline void match( IDeckLinkVideoFrame* other )
    {
      if ( width_ != other->GetWidth() || height_ != other->GetHeight() )
        resize( other->GetWidth(), other->GetHeight() );
    }
    inline const vector<uint8_t>& buffer() const { return buffer_; }
    void swap( BGRA32VideoFrame& other )
    {
      std::swap( width_, other.width_ );
      std::swap( height_, other.height_ );
      buffer_.swap( other.buffer_ );
    }
    // IDeckLinkVideoFrame
    virtual long STDMETHODCALLTYPE GetWidth() { return width_; }
    virtual long STDMETHODCALLTYPE GetHeight() { return height_; }
    virtual long STDMETHODCALLTYPE GetRowBytes() { return width_ * 4; }
    virtual HRESULT STDMETHODCALLTYPE GetBytes( void** buffer )
    {
      *buffer = reinterpret_cast<void*>( buffer_.data() );
      return S_OK;
    }
    virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags() { return flags_; }
    virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat() { return BMDPixelFormat::bmdFormat8BitBGRA; }
    virtual HRESULT STDMETHODCALLTYPE GetAncillaryData( IDeckLinkVideoFrameAncillary** ancillary ) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE GetTimecode( BMDTimecodeFormat format, IDeckLinkTimecode** timecode ) { return E_NOTIMPL; }
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID* ppv )
    {
      if ( !ppv )
        return E_INVALIDARG;
      if ( iid == IID_IUnknown || iid == IID_IDeckLinkVideoFrame )
      {
        *ppv = this;
        AddRef();
        return S_OK;
      }
      return E_NOINTERFACE;
    }
    virtual ULONG STDMETHODCALLTYPE AddRef() { return refCount_.fetch_add( 1 ); }
    virtual ULONG STDMETHODCALLTYPE Release()
    {
      auto nval = refCount_.fetch_sub( 1 );
      if ( nval == 0 )
      {
        delete this;
        return 0;
      }
      return nval;
    }
  };

  class DecklinkDevice;

  using DecklinkDeviceVector = vector<DecklinkDevice*>;

  struct DisplayMode {
    long width_;
    long height_;
    BMDFieldDominance fields_;
    BMDTimeValue frameDuration_;
    BMDTimeScale timeScale_;
    BMDDisplayMode value_;
    DisplayMode(): width_( 0 ), height_( 0 ), fields_( bmdProgressiveFrame ),
      frameDuration_( 0 ), timeScale_( 0 ), value_( BMDDisplayMode::bmdModeUnknown ) {}
    DisplayMode( IDeckLinkDisplayMode* src )
    {
      width_ = src->GetWidth();
      height_ = src->GetHeight();
      value_ = src->GetDisplayMode();
      fields_ = src->GetFieldDominance();
      src->GetFrameRate( &frameDuration_, &timeScale_ );
    }
    string format()
    {
      float fps = ( (float)timeScale_ / (float)frameDuration_ );
      char buffer[128];
      sprintf_s( buffer, 128, "%ix%i%s %.2f fps", width_, height_,
        fields_ == BMDFieldDominance::bmdUnknownFieldDominance ? ""
        : fields_ == BMDFieldDominance::bmdLowerFieldFirst ? "i"
        : fields_ == BMDFieldDominance::bmdUpperFieldFirst ? "i"
        : fields_ == BMDFieldDominance::bmdProgressiveFrame ? "p"
        : fields_ == BMDFieldDominance::bmdProgressiveSegmentedFrame ? "psf"
        : "",
        fps );
      return buffer;
    }
  };

  using DisplayModeVector = vector<DisplayMode>;

  class DecklinkCapture {
    friend class DecklinkDevice;
  private:
    DecklinkDeviceVector devices_;
    DecklinkDevice* currentCaptureDevice_ = nullptr;
    IDeckLinkVideoConversion* converter_ = nullptr;
    RWLock lock_;
    void iterateDevices();
    bool convertFrame( IDeckLinkVideoFrame* source, IDeckLinkVideoFrame* destination );
  public:
    static const string& getVersion();
    DecklinkCapture();
    ~DecklinkCapture();
    bool initialize();
    inline const DecklinkDeviceVector& getDevices() const
    {
      return devices_;
    }
    bool startCaptureSingle( DecklinkDevice* device, BMDDisplayMode displayMode );
    bool getFrameBlocking( BGRA32VideoFrame** out_frame, uint32_t& out_index );
    void stopCaptureSingle();
    void shutdown();
  };

  class DecklinkDevice: public IDeckLinkInputCallback {
    friend class DecklinkCapture;
  private:
    IDeckLink* decklink_ = nullptr;
    IDeckLinkProfileAttributes* attributes_ = nullptr;
    IDeckLinkConfiguration* config_ = nullptr;
    IDeckLinkInput* input_ = nullptr;
    bool applyDetectedMode_ = false;
    RWLock lock_;
    DecklinkCapture* owner_;
    BGRA32VideoFrame frame_;
    BGRA32VideoFrame storedFrame_;
    Event newFrameEvent_;
    atomic<uint32_t> frameIndex_;
    uint32_t lastReturnedFrameIndex_;
    bool init();
  protected:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID* ppv );
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
    LONG refCount_;
    // IDeckLinkDeviceNotificationCallback
    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(
      BMDVideoInputFormatChangedEvents notificationEvents,
      IDeckLinkDisplayMode* newDisplayMode,
      BMDDetectedVideoInputFormatFlags detectedSignalFlags );
    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(
      IDeckLinkVideoInputFrame* videoFrame,
      IDeckLinkAudioInputPacket* audioPacket );
  public:
    string name_;
    int64_t id_ = 0;
    bool usable_ = false;
    bool hasInput_ = false;
    bool hasFormatDetection_ = false;
    bool capturing_ = false;
    BMDPixelFormat pixelFormat_ = bmdFormat8BitYUV;
    DisplayMode displayMode_;
    DisplayModeVector displayModes_;
    DecklinkDevice( DecklinkCapture* owner, IDeckLink* dl );
    bool startCapture( BMDDisplayMode displayMode );
    bool getFrameBlocking( BGRA32VideoFrame** out_frame, uint32_t& out_index );
    void stopCapture();
    ~DecklinkDevice();
  };

}