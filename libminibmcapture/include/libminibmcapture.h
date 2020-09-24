#pragma once

#include "pch.h"
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

  class DecklinkDevice;

  using DecklinkDeviceVector = vector<DecklinkDevice*>;

  struct DisplayMode {
    long width_;
    long height_;
    BMDFieldDominance fields_;
    BMDTimeValue frameDuration_;
    BMDTimeScale timeScale_;
    BMDDisplayMode value_;
    string format()
    {
      float fps = ( (float)timeScale_ / (float)frameDuration_ );
      char buffer[128];
      sprintf_s( buffer, 128, "%ix%i%s %.2f FPS", width_, height_,
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
  private:
    DecklinkDeviceVector devices_;
    DecklinkDevice* currentCaptureDevice_ = nullptr;
    void iterateDevices();
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
    DisplayModeVector displayModes_;
    DecklinkDevice( IDeckLink* dl );
    bool startCapture( BMDDisplayMode displayMode );
    void stopCapture();
    ~DecklinkDevice();
  };

}