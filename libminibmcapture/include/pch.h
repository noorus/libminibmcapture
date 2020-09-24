#pragma once

#define NTDDI_VERSION NTDDI_WIN10
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#include <sdkddkver.h>

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#ifdef _DEBUG
//# define _CRTDBG_MAP_ALLOC
# include <crtdbg.h>
#endif

#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <eh.h>
#include <intrin.h>
#include <assert.h>

#undef min
#undef max

#include <exception>
#include <memory>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <cstdint>
#include <algorithm>

namespace minibm {

  using std::string;
  using std::vector;
  using std::move;
  using std::shared_ptr;
  using std::unique_ptr;

}