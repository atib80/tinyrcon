#pragma once
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <wininet.h>

class internet_handle
{
  HINTERNET handle;

public:
  explicit internet_handle(HINTERNET&& new_handle) : handle{ std::move(new_handle) } {}
  ~internet_handle() noexcept
  {
    if (NULL != handle) {
      InternetCloseHandle(handle);
      handle = NULL;
    }
  }

  HINTERNET &get() noexcept
  {
    return handle;
  }

  HINTERNET set(HINTERNET&& new_handle) noexcept
  {
    handle = std::move(new_handle);
  }
};