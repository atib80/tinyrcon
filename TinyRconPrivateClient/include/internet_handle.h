#pragma once
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <wininet.h>

class internet_handle
{
  HINTERNET handle;

public:
  internet_handle() : handle{} {}
  explicit internet_handle(HINTERNET &&new_handle) : handle{ std::move(new_handle) } {}
  internet_handle(const internet_handle &) = delete;
  internet_handle &operator=(const internet_handle &) = delete;
  internet_handle(internet_handle &&rhs) noexcept
  {
    handle = std::move(rhs.handle);
    rhs.handle = nullptr;
  }

  internet_handle &operator=(internet_handle &&rhs) noexcept
  {
    if (nullptr != handle) {
      InternetCloseHandle(handle);
    }
    handle = std::move(rhs.handle);
    rhs.handle = nullptr;
    return *this;
  }

  ~internet_handle() noexcept
  {
    if (nullptr != handle) {
      InternetCloseHandle(handle);
      handle = nullptr;
    }
  }

  explicit operator bool() const noexcept
  {
    return handle != nullptr;
  }

  const HINTERNET &get() const noexcept
  {
    return handle;
  }

  void set(HINTERNET &&new_handle) noexcept
  {
    if (handle != nullptr)
      InternetCloseHandle(handle);
    handle = std::move(new_handle);
  }

private:
  void close() noexcept
  {
    if (nullptr != handle) {
      InternetCloseHandle(handle);
      handle = nullptr;
    }
  }
};