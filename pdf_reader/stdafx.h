// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <zlib.h>
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <functional>

#include <xlslib.h>

#ifdef _DEBUG
#pragma comment(lib, "zlibstatd.lib")
#pragma comment(lib, "xlslibd.lib")
#else
#pragma comment(lib, "zlibstat.lib")
#pragma comment(lib, "xlslib.lib")
#endif
// TODO: reference additional headers your program requires here
