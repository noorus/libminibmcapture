#include "pch.h"
#include "libminibmcapture.h"
#include "utils.h"

namespace minibm {

  DecklinkDevice::DecklinkDevice( IDeckLink* dl ): decklink_( dl ), refCount_( 1 )
  {
    dl->AddRef();
    usable_ = init();
  }

  HRESULT DecklinkDevice::VideoInputFormatChanged(
    BMDVideoInputFormatChangedEvents notificationEvents,
    IDeckLinkDisplayMode* newDisplayMode,
    BMDDetectedVideoInputFormatFlags detectedSignalFlags )
  {
    printf( "VideoInputFormatChanged from thread 0x%X\r\n", GetCurrentThreadId() );

    if ( notificationEvents & bmdVideoInputColorspaceChanged )
    {
      if ( detectedSignalFlags & bmdDetectedVideoInputYCbCr422 )
        pixelFormat_ = bmdFormat10BitYUV;
      else if ( detectedSignalFlags & bmdDetectedVideoInputRGB444 )
        pixelFormat_ = bmdFormat10BitRGB;
    }

    if ( ( notificationEvents & bmdVideoInputDisplayModeChanged )
      && applyDetectedMode_ )
    {
      input_->StopStreams();
      input_->EnableVideoInput( newDisplayMode->GetDisplayMode(), pixelFormat_, bmdVideoInputEnableFormatDetection );
      input_->StartStreams();
    }

    return S_OK;
  }

  HRESULT DecklinkDevice::VideoInputFrameArrived(
    IDeckLinkVideoInputFrame* videoFrame,
    IDeckLinkAudioInputPacket* audioPacket )
  {
    printf( "VideoInputFrameArrived from thread 0x%X\r\n", GetCurrentThreadId() );
    return S_OK;
  }

  bool DecklinkDevice::init()
  {
    BSTR tmpStr;
    if ( decklink_->GetModelName( &tmpStr ) == S_OK )
      name_ = bstrToString( tmpStr );

    if ( decklink_->QueryInterface( IID_IDeckLinkProfileAttributes,
      reinterpret_cast<void**>( &attributes_ ) ) != S_OK )
      return false;

    if ( decklink_->QueryInterface( IID_IDeckLinkConfiguration,
      reinterpret_cast<void**>( &config_ ) ) != S_OK )
      return false;

    int64_t ioSupport;

    auto result = attributes_->GetInt( BMDDeckLinkVideoIOSupport, &ioSupport );
    hasInput_ = ( ioSupport & bmdDeviceSupportsCapture );

    BOOL fmtDetect = TRUE;
    if ( attributes_->GetFlag( BMDDeckLinkSupportsInputFormatDetection, &fmtDetect ) != S_OK )
      fmtDetect = FALSE;
    hasFormatDetection_ = ( fmtDetect );

    if ( attributes_->GetInt( BMDDeckLinkPersistentID, &id_ ) != S_OK )
      id_ = 0;

    if ( decklink_->QueryInterface( IID_IDeckLinkInput,
      reinterpret_cast<void**>( &input_ ) ) != S_OK )
      return false;

    displayModes_.clear();

    IDeckLinkDisplayModeIterator* dmIterator;
    if ( input_->GetDisplayModeIterator( &dmIterator ) == S_OK && dmIterator )
    {
      IDeckLinkDisplayMode* displayMode;
      while ( dmIterator->Next( &displayMode ) == S_OK )
      {
        DisplayMode dm;
        dm.width_ = displayMode->GetWidth();
        dm.height_ = displayMode->GetHeight();
        dm.value_ = displayMode->GetDisplayMode();
        dm.fields_ = displayMode->GetFieldDominance();
        displayMode->GetFrameRate( &dm.frameDuration_, &dm.timeScale_ );
        // hack: we'll only push progressive modes as valid for now
        // because I don't feel like dealing with scanlines right out of the gate
        if ( dm.fields_ == BMDFieldDominance::bmdProgressiveFrame )
        {
          displayModes_.push_back( move( dm ) );
        }
        displayMode->Release();
      }
      dmIterator->Release();
    }

    return true;
  }

  bool DecklinkDevice::startCapture( BMDDisplayMode displayMode )
  {
    bool modeValid = false;
    for ( auto& mode : displayModes_ )
    {
      if ( mode.value_ == displayMode )
        modeValid = true;
    }

    if ( !modeValid || capturing_ )
      return false;

    input_->SetCallback( this );

    BMDVideoInputFlags inputFlags = bmdVideoInputFlagDefault;
    if ( hasFormatDetection_ )
    {
      inputFlags |= bmdVideoInputEnableFormatDetection;
      applyDetectedMode_ = true;
    } else
      applyDetectedMode_ = false;

    pixelFormat_ = bmdFormat8BitYUV;

    if ( input_->EnableVideoInput( displayMode, pixelFormat_, inputFlags ) != S_OK )
    {
      input_->SetCallback( nullptr );
      return false;
    }

    if ( input_->StartStreams() != S_OK )
    {
      input_->SetCallback( nullptr );
      return false;
    }

    capturing_ = true;

    printf( "startCapture from thread 0x%X\r\n", GetCurrentThreadId() );

    return true;
  }

  void DecklinkDevice::stopCapture()
  {
    if ( input_ )
    {
      input_->StopStreams();
      input_->SetCallback( nullptr );
    }
    capturing_ = false;
  }

  DecklinkDevice::~DecklinkDevice()
  {
    if ( input_ )
      input_->Release();
    if ( config_ )
      config_->Release();
    if ( attributes_ )
      attributes_->Release();
    if ( decklink_ )
      decklink_->Release();
  }

  HRESULT STDMETHODCALLTYPE DecklinkDevice::QueryInterface( REFIID iid, LPVOID* ppv )
  {
    HRESULT result = E_NOINTERFACE;

    if ( ppv == NULL )
      return E_INVALIDARG;

    *ppv = NULL;

    if ( iid == IID_IUnknown )
    {
      *ppv = this;
      AddRef();
      result = S_OK;
    }
    else if ( iid == IID_IDeckLinkInputCallback )
    {
      *ppv = (IDeckLinkInputCallback*)this;
      AddRef();
      result = S_OK;
    }
    else if ( iid == IID_IDeckLinkNotificationCallback )
    {
      *ppv = (IDeckLinkNotificationCallback*)this;
      AddRef();
      result = S_OK;
    }

    return result;
  }

  ULONG STDMETHODCALLTYPE DecklinkDevice::AddRef( void )
  {
    return InterlockedIncrement( (LONG*)&refCount_ );
  }

  ULONG STDMETHODCALLTYPE DecklinkDevice::Release( void )
  {
    int newRefValue;

    newRefValue = InterlockedDecrement( (LONG*)&refCount_ );
    if ( newRefValue == 0 )
    {
      delete this;
      return 0;
    }

    return newRefValue;
  }

}