// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include "targetver.h"
#include <asio.hpp>
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
// #define ASIO_STANDALONE
// Windows Header Files
#include <Windows.h>
// C RunTime Header Files
#include <cstdio>
#include <memory.h>
#include <tchar.h>
#include <windowsx.h>
// C++ header files
#include "simple_grid.h"
#include "tiny_rcon_handles.h"
#include "tiny_rcon_server_application.h"
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <thread>
#include <unordered_map>
