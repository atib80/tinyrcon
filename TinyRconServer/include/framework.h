// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#include <asio.hpp>
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
// #define ASIO_STANDALONE
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
#include "tiny_rcon_server_application.h"
#include "tiny_rcon_handles.h"
#include <filesystem>
