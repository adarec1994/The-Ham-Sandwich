#pragma once

#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <list>
#include <chrono>
#include <thread>
#include <mutex>
#include <future>
#include <string>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <cwctype>
#include <cctype>
#include <stack>
#include <regex>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include <Windows.h>
#include <windowsx.h>
#include <ShObjIdl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <gl/GL.h>
#include <gl/glext.h>
#include <gl/GLU.h>
#include <gl/wglext.h>
#include <gdiplus.h>

#include <Awesomium/WebCore.h>
#include <Awesomium/WebView.h>
#include <Awesomium/BitmapSurface.h>
#include <Awesomium/STLHelpers.h>

#include <v8/v8.h>

typedef unsigned __int64 uint64;
typedef signed __int64 int64;
typedef unsigned __int32 uint32;
typedef signed __int32 int32;
typedef unsigned __int16 uint16;
typedef signed __int16 int16;
typedef unsigned __int8 uint8;
typedef signed __int8 int8;

#define SHARED_TYPE(T) typedef std::shared_ptr<T> T##Ptr
#define SHARED_FWD(T) class T; SHARED_TYPE(T)

extern std::wstring toUnicode(const std::string& ascii);
extern std::string toMultibyte(const std::wstring& unicode);

#include "String.h"
#include "zlib/zlib.h"
#include "BinStream.h"
#include "GlExt.h"

#undef max
#undef min

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);