/*
Source File : AutoUpdate.cpp
Created for the purpose of demonstration for http://www.codeproject.com

Copyright 2017 Michael Haephrati, Secured Globe Inc.
See also: https://www.codeproject.com/script/Membership/View.aspx?mid=5956881

Secured Globe, Inc.
http://www.securedglobe.com
*/

#include "autoupdate.h"
#include <urlmon.h>
#include <iostream>
#include <fstream>
#include <string>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Wininet.lib")

extern tiny_rcon_handles app_handles;
extern PROCESS_INFORMATION pr_info;

using namespace std;

#define SELF_REMOVE_STRING TEXT("cmd.exe /C ping 1.1.1.1 -n 1 -w 3000 > nul & del /f /q \"%s\"")

void delete_me()
{
  char file_path[MAX_PATH];
  char command_to_run[2 * MAX_PATH];
  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};
  GetModuleFileNameA(nullptr, file_path, MAX_PATH);
  snprintf(command_to_run, 2 * MAX_PATH, SELF_REMOVE_STRING, file_path);
  CreateProcessA(nullptr, command_to_run, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
}

unsigned long get_version_number(const string &file_name)
{
  unsigned long version_number{};

  for (size_t start{}, last; (start = file_name.find_first_of("0123456789", start)) != string::npos;) {
    last = file_name.find_first_not_of("0123456789", start + 1);
    version_number *= 100;
    if (string::npos == last) {
      last = file_name.length();
    }
    version_number += stoul(file_name.substr(start, last - start));
    start = last + 1;
  }

  return version_number;
}

unsigned long get_version_number(const version_data *ver) noexcept
{
  unsigned long version_number{};
  if (ver != nullptr) {
    version_number += ver->major;
    version_number *= 100;
    version_number += ver->minor;
    version_number *= 100;
    version_number += ver->revision;
    version_number *= 100;
    version_number += ver->sub_revision;
  }

  return version_number;
}

bool run_executable(const char *file_path_for_executable)
{
  PROCESS_INFORMATION ProcessInfo{};
  STARTUPINFO StartupInfo{};
  StartupInfo.cb = sizeof(StartupInfo);

  if (CreateProcessA(file_path_for_executable, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &StartupInfo, &ProcessInfo)) {
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
    return true;
  }

  return false;
}

std::string get_file_name_from_path(const std::string &file_path)
{
  const auto slash_pos{ file_path.rfind('\\') };
  return file_path.substr(slash_pos + 1);
}

auto_update_manager::auto_update_manager(tiny_cod2_rcon_client_application &main_app) : app{ main_app }
{
  char exe_file_path[MAX_PATH];

  if (GetModuleFileNameA(nullptr, exe_file_path, MAX_PATH)) {

    string exe_file_path_str{ exe_file_path };
    set_self_current_working_directory({ exe_file_path_str.cbegin(), exe_file_path_str.cbegin() + exe_file_path_str.rfind('\\') + 1 });
    set_self_full_path(std::move(exe_file_path_str));
    set_self_file_name(get_file_name_from_path(exe_file_path));

    version_data ver{};
    unsigned long version_number{};
    if (get_file_version(exe_file_path, &ver, version_number))
      current_version_number = version_number;

    replace_temporary_version();

    if (is_current_instance_temporary_version)
      return;

    const string ftp_download_site_info{ "ftp://"s + app.get_ftp_download_site_ip_address() + (!app.get_ftp_download_folder_path().empty() ? "/"s + app.get_ftp_download_folder_path() + "/"s : "/"s) };

    std::snprintf(message_buffer, std::size(message_buffer), "^3Searching for updates at ^1%s\n", ftp_download_site_info.c_str());
    print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, true, true, true);

    internet_open_handle = InternetOpenA("tinyrcon", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (NULL != internet_open_handle) {

      internet_connect_handle = InternetConnectA(internet_open_handle, app.get_ftp_download_site_ip_address().c_str(), INTERNET_DEFAULT_FTP_PORT, nullptr, nullptr, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);

      if (NULL != internet_connect_handle) {

        WIN32_FIND_DATAA file_data{};

        string folder_path_files_pattern{
          app.get_ftp_download_folder_path()
        };

        if (!folder_path_files_pattern.empty() && folder_path_files_pattern.front() != '/')
          folder_path_files_pattern.insert(0, 1, '/');

        if (folder_path_files_pattern.back() != '/')
          folder_path_files_pattern.push_back('/');

        std::snprintf(download_file_path_pattern, 512, "%s*.exe", folder_path_files_pattern.c_str());
        regex download_file_regex(app.get_ftp_download_file_pattern());
        read_file_data_handle = FtpFindFirstFileA(internet_connect_handle, download_file_path_pattern, &file_data, INTERNET_FLAG_NEED_FILE, INTERNET_NO_CALLBACK);
        if (read_file_data_handle != NULL) {

          unsigned long max_version_number{ current_version_number };

          do {

            string ftp_download_file_name{ file_data.cFileName };
            smatch matches;
            if (regex_match(ftp_download_file_name, matches, download_file_regex)) {

              version_number = get_version_number(matches[1].str());
              next_version_number_str = matches[1].str();

              if (version_number > max_version_number) {
                next_version_number = max_version_number = version_number;
                latest_version_to_download = std::move(ftp_download_file_name);
              }
            }

            Sleep(50);
            ZeroMemory(&file_data, sizeof(WIN32_FIND_DATAA));
            InternetFindNextFileA(read_file_data_handle, &file_data);
          } while (stl::helper::len(file_data.cFileName) > 0);

          InternetCloseHandle(read_file_data_handle);
          read_file_data_handle = NULL;
        }


        InternetCloseHandle(internet_connect_handle);
        internet_connect_handle = NULL;
      }

      InternetCloseHandle(internet_open_handle);
      internet_open_handle = NULL;
    }

    check_for_updates();
  }
}

bool auto_update_manager::get_file_version(const string &exe_file, version_data *ver, unsigned long &version_number) const
{
  DWORD dwDummy;
  DWORD dwFVISize = GetFileVersionInfoSizeA(exe_file.c_str(), &dwDummy);
  auto lp_version_info_buffer = make_unique<BYTE[]>(dwFVISize);
  if (!lp_version_info_buffer)
    return false;

  GetFileVersionInfoA(exe_file.c_str(), 0, dwFVISize, lp_version_info_buffer.get());
  UINT uLen;
  VS_FIXEDFILEINFO *lpFfi;
  VerQueryValueA(lp_version_info_buffer.get(), "\\", (LPVOID *)&lpFfi, &uLen);
  if (lpFfi && uLen) {
    DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
    DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;
    ver->major = HIWORD(dwFileVersionMS);
    ver->minor = LOWORD(dwFileVersionMS);
    ver->revision = HIWORD(dwFileVersionLS);
    ver->sub_revision = LOWORD(dwFileVersionLS);
    version_number = 0UL;
    version_number += ver->major;
    version_number *= 100;
    version_number += ver->minor;
    version_number *= 100;
    version_number += ver->revision;
    version_number *= 100;
    version_number += ver->sub_revision;
    snprintf(message_buffer, std::size(message_buffer), "^2Current version of ^5Tiny^6Rcon ^2is ^5%d.%d.%d.%d\n", ver->major, ver->minor, ver->revision, ver->sub_revision);
    print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, true, true, true);
    return true;
  }

  return false;
}

const string &auto_update_manager::get_self_full_path() const
{
  return self_full_path;
}

void auto_update_manager::set_self_current_working_directory(string cwd) noexcept
{
  self_current_working_directory = std::move(cwd);
}

void auto_update_manager::set_self_full_path(string path) noexcept
{
  self_full_path = std::move(path);
}
const string &auto_update_manager::get_self_file_name() const
{
  return self_file_name;
}

void auto_update_manager::set_self_file_name(string file_name)
{
  self_file_name = std::move(file_name);
}


void auto_update_manager::replace_temporary_version()
{
  if (self_file_name.starts_with("_U_")) {
    is_current_instance_temporary_version = true;
    const string deleted_file_path{ self_current_working_directory + self_file_name.substr(3) };
    while (PathFileExistsA(deleted_file_path.c_str())) {
      Sleep(200);
      DeleteFileA(deleted_file_path.c_str());
    }
    const string copy_file_path{ self_current_working_directory + self_file_name };
    while (!CopyFileA(copy_file_path.c_str(), deleted_file_path.c_str(), FALSE)) {
      Sleep(100);
    }
    if (run_executable(deleted_file_path.c_str())) {
      if (pr_info.hProcess != NULL)
        CloseHandle(pr_info.hProcess);
      if (pr_info.hThread != NULL)
        CloseHandle(pr_info.hThread);
      delete_me();
      _exit(0);
    }

  } else {
    is_current_instance_temporary_version = false;
    const string temp_file_path{ self_current_working_directory + "_U_"s + self_file_name };
    while (PathFileExistsA(temp_file_path.c_str())) {
      DeleteFileA(temp_file_path.c_str());
      Sleep(200);
    }
  }
}


bool auto_update_manager::check_for_updates()
{

  const string ftp_download_site_info{ "ftp://"s + app.get_ftp_download_site_ip_address() + (!app.get_ftp_download_folder_path().empty() ? "/"s + app.get_ftp_download_folder_path() + "/"s : "/"s) };

  if (is_current_instance_temporary_version || latest_version_to_download.empty() || current_version_number >= next_version_number) {
    snprintf(message_buffer, std::size(message_buffer), "^2There is no newer version of ^5Tiny^6Rcon ^2at ^5%s\n", ftp_download_site_info.c_str());
    print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, true, true, true);
    return true;
  }

  MyCallback pCallback;
  const string exe_file_name{ self_current_working_directory + "_U_"s + self_file_name };
  char download_url_buffer[256];
  snprintf(download_url_buffer, 256, "ftp://%s/%s/%s", app.get_ftp_download_site_ip_address().c_str(), app.get_ftp_download_folder_path().c_str(), latest_version_to_download.c_str());

  snprintf(message_buffer, std::size(message_buffer), "^3Searching for updates at ^1%s\n", ftp_download_site_info.c_str());
  print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, true, true, true);
  DeleteFileA(exe_file_name.c_str());
  DeleteUrlCacheEntry(download_url_buffer);
  HRESULT hr = URLDownloadToFileA(
    nullptr,
    download_url_buffer,
    exe_file_name.c_str(),
    0,
    &pCallback);
  if (SUCCEEDED(hr)) {
    snprintf(message_buffer, std::size(message_buffer), "^2Downloaded latest version of ^5Tiny^6Rcon^2: ^1v%s\n", next_version_number_str.c_str());
    print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, true, true, true);

    if (run_executable(exe_file_name.c_str())) {
      if (pr_info.hProcess != NULL)
        CloseHandle(pr_info.hProcess);
      if (pr_info.hThread != NULL)
        CloseHandle(pr_info.hThread);
      delete_me();
      _exit(0);
    } else {
      snprintf(message_buffer, std::size(message_buffer), "^1Couldn't start the temporary version: ^3%s\n", exe_file_name.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, true, true, true);
    }

  } else {
    snprintf(message_buffer, std::size(message_buffer), "^2There is no newer version of ^5Tiny^6Rcon ^2at ^5%s\n", ftp_download_site_info.c_str());
    print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, true, true, true);
  }

  return SUCCEEDED(hr) ? true : false;
}
