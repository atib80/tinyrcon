#include "autoupdate.h"
#include <urlmon.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstring>
#include <memory>
#include "md5.h"

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Wininet.lib")

extern tiny_rcon_client_application main_app;
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


std::string get_file_name_from_path(const std::string &file_path)
{
  const auto slash_pos{ file_path.rfind('\\') };
  return file_path.substr(slash_pos + 1);
}

auto_update_manager::auto_update_manager()
{
  char exe_file_path[MAX_PATH];

  if (GetModuleFileNameA(nullptr, exe_file_path, MAX_PATH)) {

    string exe_file_path_str{ exe_file_path };
    set_self_current_working_directory({ exe_file_path_str.cbegin(), exe_file_path_str.cbegin() + exe_file_path_str.rfind('\\') + 1 });
    set_self_full_path(std::move(exe_file_path_str));
    set_self_file_name(get_file_name_from_path(exe_file_path));

    version_data ver{};
    unsigned long version_number{};
    if (get_file_version(exe_file_path, ver, version_number))
      current_version_number = version_number;

    replace_temporary_version();

    if (is_current_instance_temporary_version)
      return;

    const string ftp_download_site_info{ "ftp://"s + main_app.get_ftp_download_site_ip_address() + (!main_app.get_ftp_download_folder_path().empty() ? "/"s + main_app.get_ftp_download_folder_path() + "/"s : "/"s) };

    std::snprintf(message_buffer, std::size(message_buffer), "^3Searching for updates at ^1%s\n", ftp_download_site_info.c_str());
    print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

    internet_connection_handles threadParam;
    threadParam.internet_open_handle.set(InternetOpenA("tinyrcon", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0));
    if (NULL != threadParam.internet_open_handle.get()) {

      // Create a worker thread
      HANDLE hThread{};
      DWORD dwThreadID{};
      DWORD dwExitCode{};
      DWORD dwTimeout{ 1000 };// 1s for timeout delay

      hThread = CreateThread(
        nullptr,// Pointer to thread security attributes
        0,// Initial thread stack size, in bytes
        worker_function1,// Pointer to thread function
        &threadParam,// The argument for the new thread
        0,// Creation flags
        &dwThreadID// Pointer to returned thread identifier
      );

      if (0 != hThread) {

        // Wait for the call to InternetConnect in worker function to complete
        if (WaitForSingleObject(hThread, dwTimeout) == WAIT_TIMEOUT) {
          main_app.set_is_ftp_server_online(false);
          snprintf(message_buffer, std::size(message_buffer), "^3Could not connect to ^5%s ^3to check for ^5Tiny^6Rcon ^3updates!\n ^5The FTP download site ^3might be offline at the moment. ^2Please, try again later.\n", ftp_download_site_info.c_str());
          print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          // Wait until the worker thread exits
          WaitForSingleObject(hThread, INFINITE);
          GetExitCodeThread(hThread, &dwExitCode);
          CloseHandle(hThread);
          if (dwExitCode)
            return;
        }
      }


      if (NULL != threadParam.internet_connect_handle.get()) {

        string ftp_download_folder_path{
          main_app.get_ftp_download_folder_path()
        };

        if (!ftp_download_folder_path.empty() && ftp_download_folder_path.front() != '/')
          ftp_download_folder_path.insert(0, 1, '/');

        if (ftp_download_folder_path.back() != '/')
          ftp_download_folder_path.push_back('/');

        auto available_file_names = get_file_name_matches_for_specified_file_pattern(threadParam.internet_connect_handle, ftp_download_folder_path.c_str(), "_U_TinyRcon*.exe");

        unsigned long max_version_number{ current_version_number };
        regex version_number_regex{ R"((\d+\.\d+.\d+.\d+))" };

        for (auto &ftp_download_file_name : available_file_names) {

          smatch version_smatch_results{};
          if (regex_search(ftp_download_file_name, version_smatch_results, version_number_regex)) {
            version_number = version_data::get_version_number(version_smatch_results[1].str());
            next_version_number_str = version_smatch_results[1].str();

            if (version_number > max_version_number) {
              next_version_number = max_version_number = version_number;
              latest_version_to_download = std::move(ftp_download_file_name);
            }
          }
        }

        if (latest_version_to_download.empty()) {
          snprintf(message_buffer, std::size(message_buffer), "^2There is no newer version of ^5Tiny^6Rcon ^2at ^5%s\n", ftp_download_site_info.c_str());
          print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }

        Sleep(20);

        available_file_names = get_file_name_matches_for_specified_file_pattern(threadParam.internet_connect_handle, ftp_download_folder_path.c_str(), "geo-*.7z");
        if (!available_file_names.empty()) {
          const string new_geo_dat_file_name{ available_file_names[0] };
          const string new_geo_dat_file_md5{ new_geo_dat_file_name.substr(4, new_geo_dat_file_name.length() - 7) };
          const string current_geo_dat_file_md5 = [&]() {
            if (main_app.get_plugins_geoIP_geo_dat_md5() != new_geo_dat_file_md5) {
              const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\geo.dat" };
              ifstream geo_dat_file(geo_dat_file_path.c_str(), std::ios::binary | std::ios::in);

              if (!geo_dat_file)
                return string{};

              geo_dat_file.seekg(0, std::ios::end);
              const size_t length{ static_cast<size_t>(geo_dat_file.tellg()) };
              geo_dat_file.seekg(0, std::ios::beg);

              unique_ptr<char[]> file_data{ make_unique<char[]>(length) };
              geo_dat_file.read(file_data.get(), length);

              main_app.set_plugins_geoIP_geo_dat_md5(md5(file_data.get(), length));
            }

            return main_app.get_plugins_geoIP_geo_dat_md5();
          }();

          if (current_geo_dat_file_md5 != new_geo_dat_file_md5) {
            main_app.set_plugins_geoIP_geo_dat_md5(new_geo_dat_file_md5);
            write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
            const string seven_zip_dll_file_path{ self_current_working_directory + "7za.dll" };
            if (!check_if_file_path_exists(seven_zip_dll_file_path.c_str())) {
              Sleep(20);
              available_file_names = get_file_name_matches_for_specified_file_pattern(threadParam.internet_connect_handle, ftp_download_folder_path.c_str(), "7za.dll");
              if (!available_file_names.empty()) {
                char download_url_buffer[256];
                snprintf(download_url_buffer, 256, "ftp://%s/%s/7za.dll", main_app.get_ftp_download_site_ip_address().c_str(), main_app.get_ftp_download_folder_path().c_str());
                snprintf(message_buffer, std::size(message_buffer), "^2Downloading missing ^17za.dll ^2for extracting updated\n ^1geoIP database ^2file from ^5%s\n", ftp_download_site_info.c_str());
                print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
                if (download_file(download_url_buffer, seven_zip_dll_file_path.c_str())) {
                  snprintf(message_buffer, std::size(message_buffer), "^2Successfully downloaded ^57za.dll ^2from ^5%s\n", ftp_download_site_info.c_str());
                  print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

                } else {
                  snprintf(message_buffer, std::size(message_buffer), "^3Failed to download ^57za.dll ^2from ^5%s\n", ftp_download_site_info.c_str());
                  print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
                }
              }
            }

            // download updated geoIP database file geo-3bd1621e4ce010e8b48c9efca8c5d07b.7z and extract it to plugins/geoIP/geo.dat
            const string old_geo_dat_file_parent_folder_path{ self_current_working_directory + "plugins\\geoIP\\" };
            const string old_geo_dat_file_path{ old_geo_dat_file_parent_folder_path + "geo.dat" };
            const string new_geo_dat_7zip_file_path{ self_current_working_directory + "plugins\\geoIP\\"s + new_geo_dat_file_name };
            if (!check_if_file_path_exists(old_geo_dat_file_parent_folder_path.c_str())) {
              error_code ec{};
              std::filesystem::create_directories(old_geo_dat_file_parent_folder_path, ec);
            }
            Sleep(20);
            char download_url_buffer[256];
            snprintf(download_url_buffer, 256, "ftp://%s/%s/%s", main_app.get_ftp_download_site_ip_address().c_str(), main_app.get_ftp_download_folder_path().c_str(), new_geo_dat_file_name.c_str());
            snprintf(message_buffer, std::size(message_buffer), "^2Downloading updated ^5%s ^2geoIP database file\n from ^5%s...\n", new_geo_dat_file_name.c_str(), ftp_download_site_info.c_str());
            print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (download_file(download_url_buffer, new_geo_dat_7zip_file_path.c_str())) {
              snprintf(message_buffer, std::size(message_buffer), "^2Successfully downloaded updated ^5%s ^2geoIP database file\n from ^5%s.\n", new_geo_dat_file_name.c_str(), ftp_download_site_info.c_str());
              print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              snprintf(message_buffer, std::size(message_buffer), "^2Starting to extract downloaded ^5%s ^2geoIP database file\n to ^5plugins/geoIP/geo.dat ...\n", new_geo_dat_file_name.c_str());
              print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

              if (check_if_file_path_exists(old_geo_dat_file_path.c_str())) {
                DeleteFileA(old_geo_dat_file_path.c_str());
              }

              // const wstring file_path_to_geo_7z(new_geo_dat_7zip_file_path.cbegin(), new_geo_dat_7zip_file_path.cend());
              string dir_path_to_geo_7z{ format("{}plugins\\geoIP\\", main_app.get_current_working_directory()) };
              auto [status, message] = extract_7z_file_to_specified_path(new_geo_dat_7zip_file_path.c_str(), dir_path_to_geo_7z.c_str());
              if (status) {
                DeleteFile(new_geo_dat_7zip_file_path.c_str());
                snprintf(message_buffer, std::size(message_buffer), "^2Finished extracting downloaded ^5%s ^2geoIP database file\n to ^5plugins/geoIP/geo.dat\n", new_geo_dat_file_name.c_str());
                print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

              } else {
                snprintf(message_buffer, std::size(message_buffer), "^3Failed to extract downloaded ^5%s ^3geoIP database file\n to ^5plugins/geoIP/geo.dat!\n^1Error: ^5%s\n", new_geo_dat_file_name.c_str(), message.c_str());
                print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              }

            } else {
              snprintf(message_buffer, std::size(message_buffer), "^3Failed to download updated ^1%s ^3geoIP database file\n from ^5%s\n", new_geo_dat_file_name.c_str(), ftp_download_site_info.c_str());
              print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            }
          }
        }

      } else {
        snprintf(message_buffer, std::size(message_buffer), "^3Could not connect to ^5%s ^3to check for ^5Tiny^6Rcon ^3updates!\n ^5The FTP download site ^3might be offline at the moment. ^2Please, try again later.\n", ftp_download_site_info.c_str());
        print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.set_is_ftp_server_online(false);
        return;
      }

      downloaded_latest_version_of_program();
    }
  }
}

bool auto_update_manager::get_file_version(const string &exe_file, version_data &ver, unsigned long &version_number) const noexcept
{
  unique_ptr<BYTE[]> lp_version_info_buffer{};

  try {
    DWORD temp{};
    const DWORD dwFVISize{ GetFileVersionInfoSizeA(exe_file.c_str(), &temp) };
    lp_version_info_buffer = make_unique<BYTE[]>(dwFVISize);
    GetFileVersionInfoA(exe_file.c_str(), 0, dwFVISize, lp_version_info_buffer.get());
    UINT uLen;
    VS_FIXEDFILEINFO *lpFfi;
    VerQueryValueA(lp_version_info_buffer.get(), "\\", (LPVOID *)&lpFfi, &uLen);
    if (lpFfi && uLen) {
      DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
      DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;
      ver.major = HIWORD(dwFileVersionMS);
      ver.minor = LOWORD(dwFileVersionMS);
      ver.revision = HIWORD(dwFileVersionLS);
      ver.sub_revision = LOWORD(dwFileVersionLS);
      version_number = 0UL;
      version_number += ver.major;
      version_number *= 100;
      version_number += ver.minor;
      version_number *= 100;
      version_number += ver.revision;
      version_number *= 100;
      version_number += ver.sub_revision;
      snprintf(message_buffer, std::size(message_buffer), "^2Current version of ^5Tiny^6Rcon ^2is ^5%d.%d.%d.%d\n", ver.major, ver.minor, ver.revision, ver.sub_revision);
      print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return true;
    }
  } catch (exception &) {

    return false;
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

const std::string &auto_update_manager::get_self_current_working_directory() const noexcept
{
  return self_current_working_directory;
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

vector<string> auto_update_manager::get_file_name_matches_for_specified_file_pattern(internet_handle &internet_connect_handle, const char *relative_path, const char *file_pattern) const
{
  vector<string> found_file_names;

  std::snprintf(download_file_path_pattern, std::size(download_file_path_pattern), "%s%s", relative_path, file_pattern);
  WIN32_FIND_DATAA file_data{};
  internet_handle read_file_data_handle{ FtpFindFirstFileA(internet_connect_handle.get(), download_file_path_pattern, &file_data, INTERNET_FLAG_NEED_FILE, INTERNET_NO_CALLBACK) };

  if (read_file_data_handle.get() != NULL) {
    while (stl::helper::len(file_data.cFileName) > 0) {
      found_file_names.emplace_back(file_data.cFileName);
      Sleep(20);
      ZeroMemory(&file_data, sizeof(WIN32_FIND_DATAA));
      InternetFindNextFileA(read_file_data_handle.get(), &file_data);
    }
  }

  return found_file_names;
}

bool auto_update_manager::download_file(const char *download_url, const char *downloaded_file_path) const
{
  MyCallback pCallback;
  DeleteUrlCacheEntry(download_url);
  HRESULT hr = URLDownloadToFileA(
    nullptr,
    download_url,
    downloaded_file_path,
    0,
    &pCallback);

  return SUCCEEDED(hr);
}


void auto_update_manager::replace_temporary_version()
{
  if (self_file_name.starts_with("_U_")) {
    is_current_instance_temporary_version = true;
    const string deleted_file_path{ self_current_working_directory + self_file_name.substr(3) };
    while (PathFileExistsA(deleted_file_path.c_str())) {
      Sleep(10);
      DeleteFileA(deleted_file_path.c_str());
    }
    const string copy_file_path{ self_current_working_directory + self_file_name };
    while (!CopyFileA(copy_file_path.c_str(), deleted_file_path.c_str(), FALSE)) {
      Sleep(10);
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
      Sleep(10);
    }
  }
}

void auto_update_manager::downloaded_latest_version_of_program() const
{
  if (!is_current_instance_temporary_version && !latest_version_to_download.empty() && next_version_number > current_version_number) {

    const string exe_file_name{ self_current_working_directory + "_U_"s + self_file_name };
    char download_url_buffer[256];
    snprintf(download_url_buffer, 256, "ftp://%s/%s/%s", main_app.get_ftp_download_site_ip_address().c_str(), main_app.get_ftp_download_folder_path().c_str(), latest_version_to_download.c_str());
    DeleteFileA(exe_file_name.c_str());
    if (download_file(download_url_buffer, exe_file_name.c_str())) {
      snprintf(message_buffer, std::size(message_buffer), "^2Downloaded latest version of ^5Tiny^6Rcon^2: ^1v%s\n", next_version_number_str.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      if (run_executable(exe_file_name.c_str())) {
        if (pr_info.hProcess != NULL)
          CloseHandle(pr_info.hProcess);
        if (pr_info.hThread != NULL)
          CloseHandle(pr_info.hThread);
        delete_me();
        _exit(0);
      } else {
        snprintf(message_buffer, std::size(message_buffer), "^1Couldn't start the temporary version: ^3%s\n", exe_file_name.c_str());
        print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    }
  }
}

DWORD WINAPI worker_function1(void *param)
{
  internet_connection_handles *pThreadParm{ reinterpret_cast<internet_connection_handles *>(param) };
  if (pThreadParm->internet_open_handle.get() != NULL) {
    DWORD internet_option_connect_timeout{ 1000 };
    InternetSetOptionA(pThreadParm->internet_open_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &internet_option_connect_timeout, sizeof(internet_option_connect_timeout));
    pThreadParm->internet_connect_handle.set(InternetConnectA(pThreadParm->internet_open_handle.get(), main_app.get_ftp_download_site_ip_address().c_str(), INTERNET_DEFAULT_FTP_PORT, nullptr, nullptr, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0));

    if (!pThreadParm->internet_connect_handle.get())
      return 1;

    return 0;
  }

  return 1;
}

DWORD WINAPI worker_function2(void *param)
{
  internet_connection_handles *pThreadParm{ reinterpret_cast<internet_connection_handles *>(param) };
  if (pThreadParm->internet_open_handle.get() != NULL) {
    DWORD internet_option_connect_timeout{ 1000 };
    InternetSetOptionA(pThreadParm->internet_open_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &internet_option_connect_timeout, sizeof(internet_option_connect_timeout));
    /*pThreadParm->internet_connect_handle.set(InternetConnectA(pThreadParm->internet_open_handle.get(), main_app.get_ftp_download_site_ip_address().c_str(), INTERNET_DEFAULT_FTP_PORT, nullptr, nullptr, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0));*/
    pThreadParm->internet_connect_handle.set(InternetOpenUrl(pThreadParm->internet_open_handle.get(),
     /* "http://myexternalip.com/raw",*/
      "http://icanhazip.com",
      NULL,
      0,
      INTERNET_FLAG_RELOAD,
      0));

    if (!pThreadParm->internet_connect_handle.get())
      return 1;

    return 0;
  }

  return 1;
}

DWORD WINAPI worker_function3(void *param)
{
  internet_connection_handles *pThreadParm{ reinterpret_cast<internet_connection_handles *>(param) };
  if (pThreadParm->internet_open_handle.get() != NULL) {
    DWORD internet_option_connect_timeout{ 1000 };
    InternetSetOptionA(pThreadParm->internet_open_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &internet_option_connect_timeout, sizeof(internet_option_connect_timeout));
    pThreadParm->internet_connect_handle.set(InternetConnectA(pThreadParm->internet_open_handle.get(), pThreadParm->ftp_host, INTERNET_DEFAULT_FTP_PORT, pThreadParm->user_name, pThreadParm->user_pass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0));

    if (!pThreadParm->internet_connect_handle.get())
      return 1;
    if (TRUE == FtpPutFileA(pThreadParm->internet_connect_handle.get(), pThreadParm->upload_file_path, pThreadParm->ftp_file_path, FTP_TRANSFER_TYPE_BINARY, 0))
      return 0;
    return 1;
  }

  return 1;
}

DWORD WINAPI worker_function4(void *param)
{
  internet_connection_handles *pThreadParm{ reinterpret_cast<internet_connection_handles *>(param) };
  if (pThreadParm->internet_open_handle.get() != NULL) {
    DWORD internet_option_connect_timeout{ 1000 };
    InternetSetOptionA(pThreadParm->internet_open_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &internet_option_connect_timeout, sizeof(internet_option_connect_timeout));
    pThreadParm->internet_connect_handle.set(InternetConnectA(pThreadParm->internet_open_handle.get(), pThreadParm->ftp_host, INTERNET_DEFAULT_FTP_PORT, pThreadParm->user_name, pThreadParm->user_pass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0));

    if (!pThreadParm->internet_connect_handle.get())
      return 1;
    if (TRUE == FtpGetFileA(pThreadParm->internet_connect_handle.get(), pThreadParm->ftp_file_path, pThreadParm->download_file_path, FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0)) {
      Sleep(100);
      FtpDeleteFileA(pThreadParm->internet_connect_handle.get(), pThreadParm->ftp_file_path);
      return 0;
    }
    return 1;
  }

  return 1;
}

std::string get_tiny_rcon_client_external_ip_address()
{

  internet_connection_handles threadParam;
  threadParam.internet_open_handle.set(InternetOpenA("IP retriever",
    INTERNET_OPEN_TYPE_PRECONFIG,
    nullptr,
    nullptr,
    0));

  if (NULL != threadParam.internet_open_handle.get()) {

    // Create a worker thread
    HANDLE hThread{};
    DWORD dwThreadID{};
    DWORD dwExitCode{};
    DWORD dwTimeout{ 1000 };// 1s for timeout delay

    hThread = CreateThread(
      nullptr,// Pointer to thread security attributes
      0,// Initial thread stack size, in bytes
      worker_function2,// Pointer to thread function
      &threadParam,// The argument for the new thread
      0,// Creation flags
      &dwThreadID// Pointer to returned thread identifier
    );

    if (0 != hThread) {
      // Wait for the call to InternetConnect in worker function to complete
      if (WaitForSingleObject(hThread, dwTimeout) == WAIT_TIMEOUT) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3Could not connect to ^5http://myexternalip.com/raw ^3to retrieve your ^1external IP address ^3for ^5Tiny^6Rcon ^3updates!\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        // Wait until the worker thread exits
        WaitForSingleObject(hThread, INFINITE);
        GetExitCodeThread(hThread, &dwExitCode);
        CloseHandle(hThread);
      }

      if (dwExitCode)
        return "unknown";

      DWORD read{};
      char buffer[4096];
      InternetReadFile(threadParam.internet_connect_handle.get(), buffer, std::size(buffer), &read);
      return std::string(buffer, read);
    }
  }

  return "unknown";
}

bool upload_file_to_ftp_server(const char *ftp_host, const char *user_name, const char *user_pass, const char *upload_file_path, const char *ftp_file_path)
{
  internet_connection_handles threadParam;
  threadParam.internet_open_handle.set(InternetOpenA("tinyrcon_upload_file", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0));
  threadParam.ftp_host = ftp_host;
  threadParam.user_name = user_name;
  threadParam.user_pass = user_pass;
  threadParam.upload_file_path = upload_file_path;
  threadParam.ftp_file_path = ftp_file_path;

  if (NULL != threadParam.internet_open_handle.get()) {

    // Create a worker thread
    HANDLE hThread{};
    DWORD dwThreadID{};
    DWORD dwExitCode{};
    DWORD dwTimeout{ 1000 };// 1s for timeout delay

    hThread = CreateThread(
      nullptr,// Pointer to thread security attributes
      0,// Initial thread stack size, in bytes
      worker_function3,// Pointer to thread function
      &threadParam,// The argument for the new thread
      0,// Creation flags
      &dwThreadID// Pointer to returned thread identifier
    );

    if (0 != hThread) {
      // Wait for the call to InternetConnect in worker function to complete
      if (WaitForSingleObject(hThread, dwTimeout) == WAIT_TIMEOUT) {
        // Wait until the worker thread exits
        WaitForSingleObject(hThread, INFINITE);
        GetExitCodeThread(hThread, &dwExitCode);
        CloseHandle(hThread);
      }
    }

    if (dwExitCode) {
      /*const string error_msg{ format("^3Could not connect to ^5ftp://{} ^3to upload file ^1{}\n ^3to ^5Tiny^6Rcon ^5server!", main_app.get_tiny_rcon_server_ip_address(), upload_file_path) };
      print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);*/
      return false;
    }

    return true;
  }

  return false;
}

bool download_file_from_ftp_server(const char *ftp_host, const char *user_name, const char *user_pass, const char *download_file_path, const char *ftp_file_path)
{
  internet_connection_handles threadParam;
  threadParam.internet_open_handle.set(InternetOpenA("tinyrcon_download_file", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0));
  threadParam.ftp_host = ftp_host;
  threadParam.user_name = user_name;
  threadParam.user_pass = user_pass;
  threadParam.download_file_path = download_file_path;
  threadParam.ftp_file_path = ftp_file_path;

  if (NULL != threadParam.internet_open_handle.get()) {

    // Create a worker thread
    HANDLE hThread{};
    DWORD dwThreadID{};
    DWORD dwExitCode{};
    DWORD dwTimeout{ 1000 };// 1s for timeout delay

    hThread = CreateThread(
      nullptr,// Pointer to thread security attributes
      0,// Initial thread stack size, in bytes
      worker_function4,// Pointer to thread function
      &threadParam,// The argument for the new thread
      0,// Creation flags
      &dwThreadID// Pointer to returned thread identifier
    );

    if (0 != hThread) {
      // Wait for the call to InternetConnect in worker function to complete
      if (WaitForSingleObject(hThread, dwTimeout) == WAIT_TIMEOUT) {
        // Wait until the worker thread exits
        WaitForSingleObject(hThread, INFINITE);
        GetExitCodeThread(hThread, &dwExitCode);
        CloseHandle(hThread);
      }
    }

    if (dwExitCode) {
      /* const string error_msg{ format("^3Failed to download file ^1{} ^3from ^5Tiny^6Rcon ^5server ^3for processing!", ftp_file_path) };
       print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);*/
      return false;
    }

    /* const string info_msg{ format("^2Successfully downloaded file ^1{} ^2from ^5Tiny^6Rcon ^5server.", ftp_file_path) };
     print_colored_text(app_handles.hwnd_re_messages_data, info_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);*/
    return true;
  }
  return false;
}
