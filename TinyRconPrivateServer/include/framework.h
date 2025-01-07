// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once
// #define _CRTDBG_MAP_ALLOC
#include <cstdlib>
// #include <crtdbg.h>
#include "../targetver.h"
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define ASIO_STANDALONE
// Windows Header Files
#include <Windows.h>
// C RunTime Header Files
#include <memory.h>
#include <tchar.h>
#include <cstdio>
#include <windowsx.h>
// C++ header files
#include <condition_variable>
#include <cstring>
#include <format>
#include <string>
#include <unordered_map>
#include <thread>
#include "simple_grid.h"
#include "tiny_rcon_client_application.h"
#include "autoupdate.h"
#include <filesystem>
#include "json_parser.hpp"
#include "stack_trace_element.h"