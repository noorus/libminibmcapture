#include "pch.h"
#include "libminidmcapture.h"

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

  inline void resetIterator( IDeckLinkIterator** out )
  {
    if ( *out )
    {
      ( *out )->Release();
      *out = nullptr;
    }

    auto result = CoCreateInstance( CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL,
      IID_IDeckLinkIterator, reinterpret_cast<void**>( out ) );

    if ( result != S_OK )
      *out = nullptr;
  }

  DecklinkCapture::DecklinkCapture()
  {
    //
  }

  DecklinkCapture::~DecklinkCapture()
  {
    //
  }

  bool DecklinkCapture::startCaptureSingle( DecklinkDevice* device, BMDDisplayMode displayMode )
  {
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

  void DecklinkCapture::stopCaptureSingle()
  {
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
      auto instance = new DecklinkDevice( device );
      if ( instance->usable_ && instance->hasInput_ )
        devices_.push_back( move( instance ) );
      else
        instance->Release();
    }

    iterator->Release();
  }

  bool DecklinkCapture::initialize()
  {
    if ( !g_globals.comInitialized_ && !g_globals.initialize() )
      return false;

    iterateDevices();
    return true;
  }

  void DecklinkCapture::shutdown()
  {
    for ( auto device : devices_ )
    {
      device->Release();
    }

    devices_.clear();
  }

}