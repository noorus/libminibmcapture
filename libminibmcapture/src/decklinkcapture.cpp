// libminibmcapture (c) 2020 noorus
// This software is licensed under the zlib license.
// See the LICENSE file which should be included with
// this source distribution for details.

#include "pch.h"
#include "minibmcap.h"

namespace minibm {

  Globals g_globals;

  Globals::Globals(): comInitialized_( false )
  {
    //
  }

  bool Globals::initialize()
  {
    auto result = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
    if ( FAILED( result ) )
    {
      comInitialized_ = false;
      return false;
    }
    else
    {
      comInitialized_ = true;
      return true;
    }
  }

  void Globals::shutdown()
  {
    if ( comInitialized_ )
    {
      CoUninitialize();
    }
  }

  const string& DecklinkCapture::getVersion()
  {
    static string version = "libminibmcapture alpha (DeckLink API v" BLACKMAGIC_DECKLINK_API_VERSION_STRING ")";
    return version;
  }

  bool DecklinkCapture::convertFrame( IDeckLinkVideoFrame* source, IDeckLinkVideoFrame* destination )
  {
    if ( !converter_ )
      return false;
    return ( SUCCEEDED( converter_->ConvertFrame( source, destination ) ) );
  }

  DecklinkCapture::DecklinkCapture()
  {
    auto ret = CoCreateInstance( CLSID_CDeckLinkVideoConversion, NULL, CLSCTX_ALL,
      IID_IDeckLinkVideoConversion, reinterpret_cast<void**>( &converter_ ) );
    if ( FAILED( ret ) )
      converter_ = nullptr;
  }

  DecklinkCapture::~DecklinkCapture()
  {
    if ( converter_ )
      converter_->Release();
  }

  bool DecklinkCapture::startCaptureSingle( DecklinkDevice* device, BMDDisplayMode displayMode )
  {
    ScopedRWLock lock( &lock_ );

    if ( currentCaptureDevice_ )
      return false;

    if ( std::find( devices_.begin(), devices_.end(), device ) == devices_.end() )
      return false;

    if ( device->startCapture( displayMode ) )
    {
      currentCaptureDevice_ = device;
      currentCaptureDevice_->AddRef();
      return true;
    }

    return false;
  }

  bool DecklinkCapture::getFrameBlocking( BGRA32VideoFrame** out_frame, uint32_t& out_index )
  {
    ScopedRWLock lock( &lock_ );

    if ( !currentCaptureDevice_ )
      return false;

    return currentCaptureDevice_->getFrameBlocking( out_frame, out_index );
  }

  void DecklinkCapture::stopCaptureSingle()
  {
    ScopedRWLock lock( &lock_ );

    if ( currentCaptureDevice_ )
    {
      currentCaptureDevice_->stopCapture();
      currentCaptureDevice_->Release();
      currentCaptureDevice_ = nullptr;
    }
  }

  void DecklinkCapture::iterateDevices()
  {
    for ( auto device : devices_ )
    {
      device->Release();
    }

    devices_.clear();

    IDeckLinkIterator* iterator = nullptr;
    auto result = CoCreateInstance( CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL,
      IID_IDeckLinkIterator, reinterpret_cast<void**>( &iterator ) );

    if ( result != S_OK || !iterator )
      return;

    IDeckLink* device;
    while ( iterator->Next( &device ) == S_OK )
    {
      auto instance = new DecklinkDevice( this, device );
      if ( instance->usable_ && instance->hasInput_ )
        devices_.push_back( move( instance ) );
      else
        instance->Release();
    }

    iterator->Release();
  }

  bool DecklinkCapture::initialize()
  {
    ScopedRWLock lock( &lock_ );

    if ( !g_globals.comInitialized_ && !g_globals.initialize() )
      return false;

    iterateDevices();
    return true;
  }

  void DecklinkCapture::shutdown()
  {
    ScopedRWLock lock( &lock_ );

    for ( auto device : devices_ )
    {
      device->Release();
    }

    devices_.clear();
  }

}