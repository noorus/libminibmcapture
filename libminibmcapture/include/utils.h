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