#pragma once
#define WIN32_MEAN_AND_LEAN
// clang-format off
// #include <Windows.h>
// #include <wininet.h>
// clang-format on

using BOOL = int;
using HINTERNET = void*;
extern "C" __declspec(dllimport) BOOL __stdcall InternetCloseHandle(HINTERNET hInternet);

class internet_handle
{
    HINTERNET handle{};

  public:
    internet_handle() = default;    
    
    explicit internet_handle(HINTERNET &&new_handle) noexcept : handle{new_handle}
    {
        new_handle = NULL;
    }
    internet_handle(const internet_handle &) = delete;
    internet_handle &operator=(const internet_handle &) = delete;
    internet_handle(internet_handle &&rhs) noexcept
    {
        handle = rhs.handle;
        rhs.handle = nullptr;
    }

    internet_handle &operator=(internet_handle &&rhs) noexcept
    {
        if (nullptr != handle)
        {
            close();
        }
        handle = rhs.handle;
        rhs.handle = nullptr;

        return *this;
    }

    ~internet_handle() noexcept
    {
        close();
    }

    constexpr explicit operator bool() const noexcept
    {
        return handle != nullptr;
    }

    const HINTERNET &get() const noexcept
    {
        return handle;
    }
 
    void set(HINTERNET &&new_handle) noexcept 
    {
        close();
        handle = new_handle;
        new_handle = nullptr;
    }

  private:
    void close() noexcept
    {
        if (nullptr != handle)
        {
            InternetCloseHandle(handle);
            handle = nullptr;
        }
    }
};
