// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once
// #define _CRTDBG_MAP_ALLOC
// #include <crtdbg.h>
// clang-format off
#include "../targetver.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define ASIO_STANDALONE
// Windows Header Files
#include "autoupdate.h"
#include "simple_grid.h"
#include "stack_trace_element.h"
#include "stats.h"
#include "tiny_rcon_client_application.h"
#include <Windows.h>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory.h>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <windowsx.h>
// clang-format on

// #define ASSERT _ASSERTE
//
// #ifdef _DEBUG
// #undef VERIFY
// #define VERIFY ASSERT
// #else
// #define VERIFY(expression) (expression)
// #endif
//
// struct last_exception
//{
//   DWORD result;
//
//   last_exception() : result{ GetLastError() } {}
// };
//
// struct manual_reset_event
//{
//   HANDLE m_handle;
//
//   manual_reset_event()
//   {
//     m_handle = CreateEvent(nullptr,
//       true,
//       false,
//       nullptr);
//
//     if (!m_handle) {
//       throw last_exception();
//     }
//   }
//
//   ~manual_reset_event()
//   {
//     VERIFY(CloseHandle(m_handle));
//   }
// };
//
// #ifdef _DEBUG
// inline auto Trace(char const *format, ...) -> void
//{
//   va_list args;
//   va_start(args, format);
//
//   char buffer[512];
//
//   ASSERT(-1 != _vsnprintf_s(buffer, _countof(buffer) - 1, format, args));
//
//   va_end(args);
//
//   OutputDebugString(buffer);
// }
//
// struct Tracer
//{
//   char const *m_filename;
//   unsigned m_line;
//
//   Tracer(char const *filename, unsigned const line) : m_filename{ filename },
//                                                       m_line{ line }
//   {
//   }
//
//   template<typename... Args>
//   auto operator()(char const *format, Args... args) const -> void
//   {
//     char buffer[512];
//
//     auto count = sprintf_s(buffer,
//       "%S(%d): ",
//       m_filename,
//       m_line);
//
//     ASSERT(-1 != count);
//
//     ASSERT(-1 != _snprintf_s(buffer + count, _countof(buffer) - count,
//     _countof(buffer) - count - 1, format, args...));
//
//     OutputDebugStringA(buffer);
//   }
// };
//
// #endif
//
// #ifdef _DEBUG
// #define TRACE Tracer(__FILE__, __LINE__)
// #else
// #define TRACE __noop
// #endif
