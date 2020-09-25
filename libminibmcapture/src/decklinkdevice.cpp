// libminibmcapture (c) 2020 noorus
// This software is licensed under the zlib license.
// See the LICENSE file which should be included with
// this source distribution for details.

#include "pch.h"
#include "minibmcap.h"
#include "utils.h"

namespace minibm {

  DecklinkDevice::DecklinkDevice( DecklinkCapture* owner, IDeckLink* dl ):
    owner_( owner ), decklink_( dl ), refCount_( 1 ), frameIndex_( 0 ),
    newFrameEvent_( false )
  {
    dl->AddRef();
    usable_ = init();
  }

  HRESULT DecklinkDevice::VideoInputFormatChanged(
    BMDVideoInputFormatChangedEvents notificationEvents,
    IDeckLinkDisplayMode* newDisplayMode,
    BMDDetectedVideoInputFormatFlags detectedSignalFlags )
  {
    ScopedRWLock lock( &lock_ );

    if ( notificationEvents & bmdVideoInputColorspaceChanged )
    {
      if ( detectedSignalFlags & bmdDetectedVideoInputYCbCr422 )
        pixelFormat_ = bmdFormat10BitYUV;
      else if ( detectedSignalFlags & bmdDetectedVideoInputRGB444 )
        pixelFormat_ = bmdFormat10BitRGB;
    }

    if ( notificationEvents & bmdVideoInputDisplayModeChanged )
    {
      displayMode_ = DisplayMode( newDisplayMode );
    }

    if ( applyDetectedMode_ )
    {
      input_->StopStreams();
      input_->EnableVideoInput( displayMode_.value_, pixelFormat_, bmdVideoInputEnableFormatDetection );
      input_->StartStreams();
    }

    return S_OK;
  }

  HRESULT DecklinkDevice::VideoInputFrameArrived(
    IDeckLinkVideoInputFrame* videoFrame,
    IDeckLinkAudioInputPacket* audioPacket )
  {
    if ( videoFrame )
    {
      ScopedRWLock lock( &lock_ );
      frame_.match( videoFrame );
      owner_->convertFrame( videoFrame, &frame_ );
      frameIndex_.fetch_add( 1 );
      newFrameEvent_.set();
    }

    return S_OK;
  }

  bool DecklinkDevice::getFrameBlocking( BGRA32VideoFrame** out_frame, uint32_t& out_index )
  {
    if ( !capturing_ )
      return false;
    while ( frameIndex_.load() <= lastReturnedFrameIndex_ )
    {
      newFrameEvent_.reset();
      newFrameEvent_.wait( 1000 );
    }
    {
      lock_.lock();
      lastReturnedFrameIndex_ = frameIndex_.load();
      storedFrame_.swap( frame_ );
      lock_.unlock();
    }
    *out_frame = &storedFrame_;
    out_index = lastReturnedFrameIndex_;
    return true;
  }

  bool DecklinkDevice::init()
  {
    ScopedRWLock lock( &lock_ );

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
        DisplayMode dm( displayMode );
        // hack: we'll only push progressive modes as valid, for now
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
    ScopedRWLock lock( &lock_ );

    bool modeValid = false;
    for ( auto& mode : displayModes_ )
    {
      if ( mode.value_ == displayMode )
      {
        displayMode_ = mode;
        modeValid = true;
      }
    }

    if ( !modeValid || capturing_ )
      return false;

    frameIndex_.store( 0 );

    input_->SetCallback( this );

    BMDVideoInputFlags inputFlags = bmdVideoInputFlagDefault;
    if ( hasFormatDetection_ )
    {
      inputFlags |= bmdVideoInputEnableFormatDetection;
      applyDetectedMode_ = true;
    } else
      applyDetectedMode_ = false;

    pixelFormat_ = bmdFormat8BitARGB;

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

    return true;
  }

  void DecklinkDevice::stopCapture()
  {
    ScopedRWLock lock( &lock_ );

    if ( input_ )
    {
      input_->StopStreams();
      input_->SetCallback( nullptr );
    }

    capturing_ = false;
  }

  DecklinkDevice::~DecklinkDevice()
  {
    ScopedRWLock lock( &lock_ );

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