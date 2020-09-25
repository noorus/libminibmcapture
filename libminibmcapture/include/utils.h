#pragma once

#include "pch.h"

namespace minibm {

  class RWLock {
  protected:
    SRWLOCK lock_;
  public:
    RWLock() { InitializeSRWLock( &lock_ ); }
    inline void lock() { AcquireSRWLockExclusive( &lock_ ); }
    inline void unlock() { ReleaseSRWLockExclusive( &lock_ ); }
    inline void lockShared() { AcquireSRWLockShared( &lock_ ); }
    inline void unlockShared() { ReleaseSRWLockShared( &lock_ ); }
  };

  class ScopedRWLock {
  protected:
    RWLock* lock_;
    bool exclusive_;
    bool locked_;
  public:
    ScopedRWLock( RWLock* lock, bool exclusive = true ):
      lock_( lock ), exclusive_( exclusive ), locked_( true )
    {
      exclusive_ ? lock_->lock() : lock_->lockShared();
    }
    void unlock()
    {
      if ( locked_ )
        exclusive_ ? lock_->unlock() : lock_->unlockShared();
      locked_ = false;
    }
    ~ScopedRWLock()
    {
      unlock();
    }
  };

  class Event {
  public:
    using NativeType = HANDLE;
  private:
    NativeType handle_;
  public:
    Event( bool initialState = false ): handle_( 0 )
    {
      handle_ = CreateEventW( nullptr, TRUE, initialState ? TRUE : FALSE, nullptr );
      if ( !handle_ )
        throw std::exception( "Event creation failed" );
    }
    inline void set() { SetEvent( handle_ ); }
    inline void reset() { ResetEvent( handle_ ); }
    inline bool wait( uint32_t milliseconds = INFINITE ) const
    {
      return ( WaitForSingleObject( handle_, milliseconds ) == WAIT_OBJECT_0 );
    }
    inline bool check() const
    {
      return ( WaitForSingleObject( handle_, 0 ) == WAIT_OBJECT_0 );
    }
    inline NativeType get() const { return handle_; }
    ~Event()
    {
      if ( handle_ )
        CloseHandle( handle_ );
    }
  };

  inline string bstrToString( BSTR bstr )
  {
    auto widelen = SysStringLen( bstr );
    auto mblen = WideCharToMultiByte(
      CP_ACP, 0,static_cast<wchar_t*>( bstr ),
      widelen, nullptr, 0, nullptr, nullptr );

    string ret( mblen, '\0' );
    mblen = ::WideCharToMultiByte(
      CP_ACP, 0, static_cast<wchar_t*>( bstr ),
      widelen, &ret[0], mblen, nullptr, nullptr );

    return ret;
  };

}