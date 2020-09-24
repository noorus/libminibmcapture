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
#include <fstream>
#include <sstream>
#include <queue>
#include <regex>
#include <stack>
#include <cstdint>
#include <algorithm>
#include <random>
#include <filesystem>
#include <queue>
#include <unordered_map>
#include <utility>
#include <variant>
#include <any>
#include <optional>