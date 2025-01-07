#include "autoupdate.h"
#include "md5.h"
#include "tiny_rcon_client_application.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Wininet.lib")

extern tiny_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;

using namespace std;

#define SELF_REMOVE_STRING TEXT("cmd.exe /C ping 1.1.1.1 -n 1 -w 3000 > nul & del /f /q \"%s\"")
#define SELF_RENAME_STRING TEXT("cmd.exe /C ping 1.1.1.1 -n 1 -w 3000 > nul & MOVE /Y \"%s\" \"%s\"")

class MyCallback : public IBindStatusCallback
{
  tiny_rcon_client_application &main_app;
  tiny_rcon_handles &app_handles;
  std::string file_name_;
  ULONG previous_available_data{};

public:
  MyCallback(tiny_rcon_client_application &main_app, tiny_rcon_handles &app_handles)
    : main_app{ main_app }, app_handles{ app_handles }, previous_available_data{}
  {
  }
  ~MyCallback() = default;

  void set_file_name(std::string file_name)
  {
    file_name_ = std::move(file_name);
  }

  // This one is called by URLDownloadToFile
  // STDMETHOD(OnProgress)
  //(/* [in] */ ULONG ulProgress, /* [in] */ ULONG ulProgressMax, /* [in] */
  // ULONG ulStatusCode, /* [in] */ LPCWSTR)
  //{
  //  // You can use your own logging function here
  //  wprintf(L"Downloaded %d of %d. Status code %d\n", ulProgress,
  //  ulProgressMax, ulStatusCode); return S_OK;
  //}

  // This one is called by URLDownloadToFile
  STDMETHOD(OnProgress)
  (/* [in] */ ULONG ulProgress,
    /* [in] */ ULONG ulProgressMax,
    /* [in] */ ULONG /*ulStatusCode*/,
    /* [in] */ LPCWSTR szStatusText)
  //{
  {
    // wcout << ulProgress << L" of " << ulProgressMax << endl; Sleep(200);
    if (ulProgress != 0 && ulProgressMax != 0) {
      main_app.add_to_next_downloaded_data_in_bytes(ulProgress - previous_available_data);
      // main_app.update_download_and_upload_speed_statistics();
      const double output{ (double(ulProgress) / ulProgressMax) * 100 };
      previous_available_data = ulProgress;
      const std::string status_text{ wstring_to_string(szStatusText) };
      const std::string info_message{
        std::format("Downloading file {} ... {:.2f}% (status: {})", file_name_, output, status_text)
      };
      append_to_title(app_handles.hwnd_main_window, info_message);
      Sleep(20);
    }
    return S_OK;
  }

  STDMETHOD(OnStartBinding)
  (/* [in] */ DWORD, /* [in] */ IBinding __RPC_FAR *)
  {
    return E_NOTIMPL;
  }

  STDMETHOD(GetPriority)
  (/* [out] */ LONG __RPC_FAR *)
  {
    return E_NOTIMPL;
  }

  STDMETHOD(OnLowResource)
  (/* [in] */ DWORD)
  {
    return E_NOTIMPL;
  }

  STDMETHOD(OnStopBinding)
  (/* [in] */ HRESULT, /* [unique][in] */ LPCWSTR)
  {
    return E_NOTIMPL;
  }

  STDMETHOD(GetBindInfo)
  (/* [out] */ DWORD __RPC_FAR *, /* [unique][out][in] */ BINDINFO __RPC_FAR *)
  {
    return E_NOTIMPL;
  }

  STDMETHOD(OnDataAvailable)
  (DWORD /*grfBSCF*/, DWORD /*dwSize*/, FORMATETC * /*pformatetc*/, STGMEDIUM * /*pstgmed*/)
  {
    return E_NOTIMPL;
  }

  STDMETHOD(OnObjectAvailable)
  (/* [in] */ REFIID, /* [iid_is][in] */ IUnknown __RPC_FAR *)
  {
    return E_NOTIMPL;
  }

  // IUnknown stuff
  STDMETHOD_(ULONG, AddRef)
  ()
  {
    return 0;
  }

  STDMETHOD_(ULONG, Release)
  ()
  {
    return 0;
  }

  STDMETHOD(QueryInterface)
  (/* [in] */ REFIID, /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *)
  {
    return E_NOTIMPL;
  }
};

void rename_myself(const char *src_file_path, const char *dst_file_path)
{
  char file_path[MAX_PATH];
  char command_to_run[2 * MAX_PATH];
  STARTUPINFO process_startup_info{};
  PROCESS_INFORMATION pr_info{};
  GetModuleFileName(nullptr, file_path, MAX_PATH);
  snprintf(command_to_run, 2 * MAX_PATH, SELF_RENAME_STRING, src_file_path, dst_file_path);
  CreateProcess(nullptr, command_to_run, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &process_startup_info, &pr_info);
  if (process_startup_info.hStdError)
    CloseHandle(process_startup_info.hStdError);
  if (process_startup_info.hStdInput)
    CloseHandle(process_startup_info.hStdInput);
  if (process_startup_info.hStdOutput)
    CloseHandle(process_startup_info.hStdOutput);
  CloseHandle(pr_info.hThread);
  CloseHandle(pr_info.hProcess);
}

void delete_me()
{
  char file_path[MAX_PATH];
  char command_to_run[2 * MAX_PATH];
  STARTUPINFO process_startup_info{};
  PROCESS_INFORMATION pr_info{};
  GetModuleFileName(nullptr, file_path, MAX_PATH);
  snprintf(command_to_run, 2 * MAX_PATH, SELF_REMOVE_STRING, file_path);
  CreateProcess(nullptr, command_to_run, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &process_startup_info, &pr_info);
  if (process_startup_info.hStdError)
    CloseHandle(process_startup_info.hStdError);
  if (process_startup_info.hStdInput)
    CloseHandle(process_startup_info.hStdInput);
  if (process_startup_info.hStdOutput)
    CloseHandle(process_startup_info.hStdOutput);
  CloseHandle(pr_info.hThread);
  CloseHandle(pr_info.hProcess);
}

bool auto_update_manager::get_file_version(const string &exe_file, version_data &ver, unsigned long &version_number /*, const bool is_print_information*/) const
{
  unique_ptr<BYTE[]> lp_version_info_buffer{};

  try {
    DWORD temp{};
    const DWORD dwFVISize{ GetFileVersionInfoSize(exe_file.c_str(), &temp) };
    lp_version_info_buffer = make_unique<BYTE[]>(dwFVISize);
    GetFileVersionInfo(exe_file.c_str(), 0, dwFVISize, lp_version_info_buffer.get());
    UINT uLen;
    VS_FIXEDFILEINFO *lpFfi;
    VerQueryValue(lp_version_info_buffer.get(), "\\", (LPVOID *)&lpFfi, &uLen);
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
      return true;
    }
  } catch (exception &) {
    return false;
  }

  return false;
}

const string &auto_update_manager::get_self_full_path() const noexcept
{
  return self_full_path;
}

void auto_update_manager::set_self_current_working_directory(string cwd)
{
  self_current_working_directory = std::move(cwd);
}

const std::string &auto_update_manager::get_self_current_working_directory() const noexcept
{
  return self_current_working_directory;
}

void auto_update_manager::set_self_full_path(string path)
{
  self_full_path = std::move(path);
}
const string &auto_update_manager::get_self_file_name() const noexcept
{
  return self_file_name;
}

void auto_update_manager::set_self_file_name(string file_name)
{
  self_file_name = std::move(file_name);
}

vector<string> auto_update_manager::get_file_name_matches_for_specified_ftp_path_and_file_pattern(
  internet_handle &internet_connect_handle,
  const char *ftp_path,
  const char *file_pattern) const
{
  vector<string> found_file_names;

  snprintf(download_file_path_pattern, std::size(download_file_path_pattern), "%s%s", ftp_path, file_pattern);
  WIN32_FIND_DATA file_data{};
  internet_handle read_file_data_handle{ FtpFindFirstFileA(internet_connect_handle.get(), download_file_path_pattern, &file_data, INTERNET_FLAG_NEED_FILE, INTERNET_NO_CALLBACK) };

  if (read_file_data_handle.get() != nullptr) {
    while (stl::helper::len(file_data.cFileName) > 0) {
      found_file_names.emplace_back(file_data.cFileName);
      Sleep(20);
      ZeroMemory(&file_data, sizeof(WIN32_FIND_DATA));
      InternetFindNextFileA(read_file_data_handle.get(), &file_data);
    }
  }

  return found_file_names;
}

// vector<wstring> auto_update_manager::get_file_name_matches_for_specified_ftp_path_and_file_pattern(
//	internet_handle& internet_connect_handle,
//	const wchar_t* ftp_path,
//	const wchar_t* file_pattern) const
//{
//	vector<wstring> found_file_names;
//
//	swprintf_s(download_file_path_pattern, std::size(download_file_path_pattern), L"%s%s", ftp_path, file_pattern);
//	WIN32_FIND_DATA file_data{};
//	internet_handle read_file_data_handle{ FtpFindFirstFile(internet_connect_handle.get(), download_file_path_pattern, &file_data, INTERNET_FLAG_NEED_FILE, INTERNET_NO_CALLBACK) };
//
//	if (read_file_data_handle.get() != nullptr) {
//		while (stl::helper::len(file_data.cFileName) > 0) {
//			found_file_names.emplace_back(file_data.cFileName);
//			Sleep(20);
//			ZeroMemory(&file_data, sizeof(WIN32_FIND_DATA));
//			InternetFindNextFile(read_file_data_handle.get(), &file_data);
//		}
//	}
//
//	return found_file_names;
// }

bool auto_update_manager::download_file(const char *download_url, const char *downloaded_file_path) const
{
  DeleteUrlCacheEntryA(download_url);
  string file_name{ download_url };
  file_name.erase(cbegin(file_name), cbegin(file_name) + file_name.rfind('/') + 1);
  MyCallback pCallback{ main_app, app_handles };
  pCallback.set_file_name(std::move(file_name));
  HRESULT hr = URLDownloadToFileA(nullptr, download_url, downloaded_file_path, 0, &pCallback);
  return SUCCEEDED(hr);
}

bool auto_update_manager::download_file(const wchar_t *download_url, const wchar_t *downloaded_file_path) const
{
  DeleteUrlCacheEntryW(download_url);
  wstring file_name{ download_url };
  file_name.erase(cbegin(file_name), cbegin(file_name) + file_name.rfind(L'/') + 1);
  MyCallback pCallback{ main_app, app_handles };
  pCallback.set_file_name(wstring_to_string(file_name.c_str()));
  HRESULT hr = URLDownloadToFileW(nullptr, download_url, downloaded_file_path, 0, &pCallback);
  return SUCCEEDED(hr);
}

void auto_update_manager::check_for_updates(const char *exe_file_path)
{
  string exe_file_path_str{ exe_file_path };
  set_self_current_working_directory(
    { exe_file_path_str.cbegin(), exe_file_path_str.cbegin() + exe_file_path_str.rfind('\\') + 1 });
  set_self_full_path(std::move(exe_file_path_str));
  set_self_file_name(get_file_name_from_path(exe_file_path));

  version_data ver{};
  unsigned long version_number{};
  if (get_file_version(get_self_full_path(), ver, version_number)) {
    current_version_number = version_number;
  }

  replace_temporary_version();

  if (is_current_instance_temporary_version)
    return;

  const string ftp_download_site_info{ "ftp://"s + main_app.get_ftp_download_site_ip_address() + (!main_app.get_ftp_download_folder_path().empty() ? "/"s + main_app.get_ftp_download_folder_path() + "/"s : "/"s) };

  std::snprintf(message_buffer, std::size(message_buffer), "^3Searching for updates at ^1%s\n", ftp_download_site_info.c_str());
  print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

  internet_connection_handles threadParam;
  threadParam.internet_open_handle.set(InternetOpenA("tinyrcon", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0));
  if (nullptr != threadParam.internet_open_handle.get()) {
    // Create a worker thread
    HANDLE hThread{};
    DWORD dwThreadID{};
    DWORD dwExitCode{};
    DWORD dwTimeout{ 1000 };// 1s for timeout delay

    hThread = CreateThread(nullptr,// Pointer to thread security attributes
      0,// Initial thread stack size, in bytes
      worker_function1,// Pointer to thread function
      &threadParam,// The argument for the new thread
      0,// Creation flags
      &dwThreadID// Pointer to returned thread identifier
    );

    if (nullptr != hThread) {
      // Wait for the call to InternetConnect in worker function to complete
      if (WaitForSingleObject(hThread, dwTimeout) == WAIT_TIMEOUT) {
        main_app.set_is_ftp_server_online(false);
        snprintf(message_buffer, std::size(message_buffer),
          "^3Could not connect to ^5%s ^3to check for ^5Tiny^6Rcon "
          "^3updates!\n ^5The FTP download site ^3might be offline at "
          "the moment. ^2Please, try again later.\n",
          ftp_download_site_info.c_str());
        print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        // Wait until the worker thread exits
        WaitForSingleObject(hThread, INFINITE);
        GetExitCodeThread(hThread, &dwExitCode);
        CloseHandle(hThread);
        if (dwExitCode)
          return;
      }
    }

    if (nullptr != threadParam.internet_connect_handle.get()) {
      string ftp_download_folder_path{ main_app.get_ftp_download_folder_path() };

      if (!ftp_download_folder_path.empty() && ftp_download_folder_path.front() != '/')
        ftp_download_folder_path.insert(0, 1, '/');

      if (ftp_download_folder_path.back() != '/')
        ftp_download_folder_path.push_back('/');

      auto available_file_names = get_file_name_matches_for_specified_ftp_path_and_file_pattern(
        threadParam.internet_connect_handle, ftp_download_folder_path.c_str(), "_U_PrivateTinyRconClient*.exe");

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

      // ***
      available_file_names = get_file_name_matches_for_specified_ftp_path_and_file_pattern(
        threadParam.internet_connect_handle, ftp_download_folder_path.c_str(), "images-*.7z");
      if (!available_file_names.empty()) {
        const string new_images_file_name{ available_file_names[0] };
        const size_t start_pos{ new_images_file_name.find('-') + 1 };
        const size_t end_pos{ new_images_file_name.rfind('.') };
        const string new_images_file_md5{ string(new_images_file_name.cbegin() + start_pos, new_images_file_name.cbegin() + end_pos) };
        // const string current_geo_dat_file_md5{ main_app.get_images_data_md5() };

        if (main_app.get_images_data_md5() != new_images_file_md5) {
          main_app.set_images_data_md5(new_images_file_md5);
          write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
          const string seven_zip_dll_file_path{ self_current_working_directory + "7za.dll" };
          if (!check_if_file_path_exists(seven_zip_dll_file_path.c_str())) {
            Sleep(20);
            available_file_names = get_file_name_matches_for_specified_ftp_path_and_file_pattern(
              threadParam.internet_connect_handle, ftp_download_folder_path.c_str(), "7za.dll");
            if (!available_file_names.empty()) {
              char download_url_buffer[512]{};
              snprintf(download_url_buffer, std::size(download_url_buffer), "ftp://%s/%s/7za.dll", main_app.get_ftp_download_site_ip_address().c_str(), main_app.get_ftp_download_folder_path().c_str());
              snprintf(message_buffer, std::size(message_buffer),
                "^2Downloading missing ^17za.dll ^2for extracting "
                "updated\n ^1geoIP database ^2file from ^5%s\n",
                ftp_download_site_info.c_str());
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

          // download updated images-*.7z file
          // images-3bd1621e4ce010e8b48c9efca8c5d07b.7z and extract it to
          // data/images/maps
          const string data_images_maps_folder_path{ self_current_working_directory + "data\\images\\maps" };
          const string new_updated_images_data_7zip_file_path{ self_current_working_directory + R"(data\images\maps\)" + new_images_file_name };
          if (!check_if_file_path_exists(data_images_maps_folder_path.c_str())) {
            error_code ec{};
            std::filesystem::create_directories(data_images_maps_folder_path, ec);
          }
          Sleep(20);
          char download_url_buffer[512]{};
          snprintf(download_url_buffer, std::size(download_url_buffer), "ftp://%s/%s/%s", main_app.get_ftp_download_site_ip_address().c_str(), main_app.get_ftp_download_folder_path().c_str(), new_images_file_name.c_str());
          snprintf(message_buffer, std::size(message_buffer),
            "^2Downloading updated compressed ^5%s ^2images data file\n from "
            "^5%s...\n",
            new_images_file_name.c_str(),
            ftp_download_site_info.c_str());
          print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

          if (check_if_file_path_exists(new_updated_images_data_7zip_file_path.c_str())) {
            DeleteFile(new_updated_images_data_7zip_file_path.c_str());
          }

          if (download_file(download_url_buffer, new_updated_images_data_7zip_file_path.c_str())) {
            snprintf(message_buffer, std::size(message_buffer),
              "^2Successfully downloaded updated ^5%s ^2images data "
              "file\n from ^5%s.\n",
              new_images_file_name.c_str(),
              ftp_download_site_info.c_str());
            print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            snprintf(message_buffer, std::size(message_buffer),
              "^2Starting to extract downloaded updated ^5%s ^2mages data "
              "file\n to ^5data\\images\\maps ...\n",
              new_images_file_name.c_str());
            print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

            // const wstring
            // file_path_to_geo_7z(new_geo_dat_7zip_file_path.cbegin(),
            // new_geo_dat_7zip_file_path.cend());
            // wstring dir_path_to_images_data_maps{format(L"{}data\\images\\maps\\", main_app.get_current_working_directory())
          };
          auto [status, message] = extract_7z_file_to_specified_path(new_updated_images_data_7zip_file_path.c_str(),
            data_images_maps_folder_path.c_str());
          if (status) {
            DeleteFile(new_updated_images_data_7zip_file_path.c_str());
            snprintf(message_buffer, std::size(message_buffer),
              "^2Finished extracting downloaded updated ^5%s ^2images data "
              "file\n to ^5data\\images\\maps\n",
              new_images_file_name.c_str());
            print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            /*if (check_if_file_path_exists("plugins\\geoIP\\geo.data")) {
              import_geoip_data(main_app.get_connection_manager().get_geoip_data(),
            "plugins\\geoIP\\geo.data");
            }*/
          } else {
            snprintf(message_buffer, std::size(message_buffer),
              "^3Failed to extract downloaded updated ^5%s ^3images data "
              "file\n to ^5images\\data\\maps!\n^1Error: ^5%s\n",
              new_images_file_name.c_str(),
              message.c_str());
            print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          }
        }
        /*else {
          snprintf(message_buffer, std::size(message_buffer),
            "^3Failed to download updated ^1%s ^3images data "
            "file\n from ^5%s\n",
            new_images_file_name.c_str(),
            ftp_download_site_info.c_str());
          print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }*/
      }

      Sleep(20);

      available_file_names = get_file_name_matches_for_specified_ftp_path_and_file_pattern(
        threadParam.internet_connect_handle, ftp_download_folder_path.c_str(), "geo-*.7z");
      if (!available_file_names.empty()) {
        const string new_geo_dat_file_name{ available_file_names[0] };
        const string new_geo_dat_file_md5{ new_geo_dat_file_name.substr(4, new_geo_dat_file_name.length() - 7) };
        const string current_geo_dat_file_md5 = [&]() {
          const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\geo.dat" };

          if (!check_if_file_path_exists(geo_dat_file_path.c_str()))
            return string{};

          const size_t length(get_file_size_in_bytes(geo_dat_file_path.c_str()));
          if (length == 0)
            return string{};

          ifstream geo_dat_file(geo_dat_file_path.c_str(), std::ios::binary | std::ios::in);
          if (!geo_dat_file)
            return string{};

          unique_ptr<char[]> file_data{ make_unique<char[]>(length) };
          geo_dat_file.read(file_data.get(), length);

          string old_geo_dat_md5_sum{ md5(file_data.get(), length) };

          if (main_app.get_plugins_geoIP_geo_dat_md5() != old_geo_dat_md5_sum) {
            main_app.set_plugins_geoIP_geo_dat_md5(old_geo_dat_md5_sum);
            write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
          }
          return old_geo_dat_md5_sum;
        }();

        if (current_geo_dat_file_md5 != new_geo_dat_file_md5) {
          main_app.set_plugins_geoIP_geo_dat_md5(new_geo_dat_file_md5);
          write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
          const string seven_zip_dll_file_path{ self_current_working_directory + "7za.dll" };
          if (!check_if_file_path_exists(seven_zip_dll_file_path.c_str())) {
            Sleep(20);
            available_file_names = get_file_name_matches_for_specified_ftp_path_and_file_pattern(
              threadParam.internet_connect_handle, ftp_download_folder_path.c_str(), "7za.dll");
            if (!available_file_names.empty()) {
              char download_url_buffer[512]{};
              snprintf(download_url_buffer, std::size(download_url_buffer), "ftp://%s/%s/7za.dll", main_app.get_ftp_download_site_ip_address().c_str(), main_app.get_ftp_download_folder_path().c_str());
              snprintf(message_buffer, std::size(message_buffer),
                "^2Downloading missing ^17za.dll ^2for extracting "
                "updated\n ^1geoIP database ^2file from ^5%s\n",
                ftp_download_site_info.c_str());
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

          // download updated geoIP database file
          // geo-3bd1621e4ce010e8b48c9efca8c5d07b.7z and extract it to
          // plugins/geoIP/geo.dat
          const string old_geo_dat_file_parent_folder_path{ self_current_working_directory + "plugins\\geoIP" };
          const string old_geo_dat_file_path{ old_geo_dat_file_parent_folder_path + "geo.dat" };
          const string new_geo_dat_7zip_file_path{ self_current_working_directory + R"(plugins\geoIP\)" + new_geo_dat_file_name };
          if (!check_if_file_path_exists(old_geo_dat_file_parent_folder_path.c_str())) {
            error_code ec{};
            std::filesystem::create_directories(old_geo_dat_file_parent_folder_path, ec);
          }
          Sleep(20);
          char download_url_buffer[512]{};
          snprintf(download_url_buffer, std::size(download_url_buffer), "ftp://%s/%s/%s", main_app.get_ftp_download_site_ip_address().c_str(), main_app.get_ftp_download_folder_path().c_str(), new_geo_dat_file_name.c_str());
          snprintf(message_buffer, std::size(message_buffer),
            "^2Downloading updated ^5%s ^2geoIP database file\n from "
            "^5%s...\n",
            new_geo_dat_file_name.c_str(),
            ftp_download_site_info.c_str());
          print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          if (download_file(download_url_buffer, new_geo_dat_7zip_file_path.c_str())) {
            snprintf(message_buffer, std::size(message_buffer),
              "^2Successfully downloaded updated ^5%s ^2geoIP database "
              "file\n from ^5%s.\n",
              new_geo_dat_file_name.c_str(),
              ftp_download_site_info.c_str());
            print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            snprintf(message_buffer, std::size(message_buffer),
              "^2Starting to extract downloaded ^5%s ^2geoIP database "
              "file\n to ^5plugins\\geoIP\\geo.dat ...\n",
              new_geo_dat_file_name.c_str());
            print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

            if (check_if_file_path_exists(old_geo_dat_file_path.c_str())) {
              DeleteFile(old_geo_dat_file_path.c_str());
            }

            // const wstring
            // file_path_to_geo_7z(new_geo_dat_7zip_file_path.cbegin(),
            // new_geo_dat_7zip_file_path.cend());
            string dir_path_to_geo_7z{
              format("{}plugins\\geoIP\\", main_app.get_current_working_directory())
            };
            auto [status, message] = extract_7z_file_to_specified_path(new_geo_dat_7zip_file_path.c_str(),
              dir_path_to_geo_7z.c_str());
            if (status) {
              DeleteFile(new_geo_dat_7zip_file_path.c_str());
              snprintf(message_buffer, std::size(message_buffer),
                "^2Finished extracting downloaded ^5%s ^2geoIP database "
                "file\n to ^5plugins\\geoIP\\geo.dat\n",
                new_geo_dat_file_name.c_str());
              print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              /*if (check_if_file_path_exists("plugins\\geoIP\\geo.data")) {
                import_geoip_data(main_app.get_connection_manager().get_geoip_data(),
              "plugins\\geoIP\\geo.data");
              }*/
            } else {
              snprintf(message_buffer, std::size(message_buffer),
                "^3Failed to extract downloaded ^5%s ^3geoIP database "
                "file\n to ^5plugins\\geoIP\\geo.dat!\n^1Error: ^5%s\n",
                new_geo_dat_file_name.c_str(),
                message.c_str());
              print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            }
          } else {
            snprintf(message_buffer, std::size(message_buffer),
              "^3Failed to download updated ^1%s ^3geoIP database "
              "file\n from ^5%s\n",
              new_geo_dat_file_name.c_str(),
              ftp_download_site_info.c_str());
            print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          }
        }
      }
    } else {
      snprintf(message_buffer, std::size(message_buffer),
        "^3Could not connect to ^5%s ^3to check for ^5Tiny^6Rcon "
        "^3updates!\n ^5The FTP download site ^3might be offline at the "
        "moment. ^2Please, try again later.\n",
        ftp_download_site_info.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      main_app.set_is_ftp_server_online(false);
      return;
    }

    downloaded_latest_version_of_program();
  }
}

void auto_update_manager::replace_temporary_version()
{
  if (self_file_name.starts_with("_U_")) {
    is_current_instance_temporary_version = true;
    const string deleted_file_path{ self_current_working_directory + self_file_name.substr(3) };
    for (size_t i{}; i < 5 && check_if_file_path_exists(deleted_file_path.c_str()); ++i) {
      Sleep(20);
      DeleteFile(deleted_file_path.c_str());
    }
    const string copy_file_path{ self_current_working_directory + self_file_name };
    for (size_t i{}; i < 5 && !CopyFile(copy_file_path.c_str(), deleted_file_path.c_str(), FALSE); ++i) {
      // while (!CopyFileA(copy_file_path.c_str(), deleted_file_path.c_str(),
      // FALSE)) {
      Sleep(20);
    }

    if (run_executable(deleted_file_path.c_str())) {
      /*if (pr_info.hProcess != nullptr)
        CloseHandle(pr_info.hProcess);
      if (pr_info.hThread != nullptr)
        CloseHandle(pr_info.hThread);*/
      delete_me();
      _exit(0);
    }
  } else {
    is_current_instance_temporary_version = false;
    const string temp_file_path{ self_current_working_directory + "_U_"s + self_file_name };
    for (size_t i{}; i < 5 && check_if_file_path_exists(temp_file_path.c_str()); ++i) {
      DeleteFile(temp_file_path.c_str());
      Sleep(20);
    }
  }
}

void auto_update_manager::downloaded_latest_version_of_program() const
{
  if (!is_current_instance_temporary_version && !latest_version_to_download.empty() && next_version_number > current_version_number) {
    const string exe_file_name{ self_current_working_directory + "_U_"s + self_file_name };
    char download_url_buffer[512];
    snprintf(download_url_buffer, std::size(download_url_buffer), "ftp://%s/%s/%s", main_app.get_ftp_download_site_ip_address().c_str(), main_app.get_ftp_download_folder_path().c_str(), latest_version_to_download.c_str());
    DeleteFile(exe_file_name.c_str());
    if (download_file(download_url_buffer, exe_file_name.c_str())) {
      snprintf(message_buffer, std::size(message_buffer), "^2Downloaded latest version of ^5Tiny^6Rcon^2: ^1v%s\n", next_version_number_str.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      if (run_executable(exe_file_name.c_str())) {
        /*if (pr_info.hProcess != nullptr)
          CloseHandle(pr_info.hProcess);
        if (pr_info.hThread != nullptr)
          CloseHandle(pr_info.hThread);*/
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
  if (pThreadParm->internet_open_handle.get() != nullptr) {
    DWORD internet_option_connect_timeout{ 1000 };
    InternetSetOptionA(pThreadParm->internet_open_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &internet_option_connect_timeout, sizeof(internet_option_connect_timeout));
    // const string ftp_site_url{ main_app.get_ftp_download_site_ip_address() };
    pThreadParm->internet_connect_handle.set(InternetConnectA(
      pThreadParm->internet_open_handle.get(), main_app.get_ftp_download_site_ip_address().c_str(), INTERNET_DEFAULT_FTP_PORT, nullptr, nullptr, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0));

    if (!pThreadParm->internet_connect_handle.get())
      return 1;

    return 0;
  }

  return 1;
}

DWORD WINAPI worker_function2(void *param)
{
  internet_connection_handles *pThreadParm{ reinterpret_cast<internet_connection_handles *>(param) };
  if (pThreadParm->internet_open_handle.get() != nullptr) {
    DWORD internet_option_connect_timeout{ 1000 };
    InternetSetOptionA(pThreadParm->internet_open_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &internet_option_connect_timeout, sizeof(internet_option_connect_timeout));
    /*pThreadParm->internet_connect_handle.set(InternetConnectA(pThreadParm->internet_open_handle.get(),
     * main_app.get_ftp_download_site_ip_address().c_str(),
     * INTERNET_DEFAULT_FTP_PORT, nullptr, nullptr, INTERNET_SERVICE_FTP,
     * INTERNET_FLAG_PASSIVE, 0));*/
    pThreadParm->internet_connect_handle.set(InternetOpenUrlA(pThreadParm->internet_open_handle.get(),
      /* "http://myexternalip.com/raw",*/
      "http://icanhazip.com",
      nullptr,
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
  if (pThreadParm->internet_open_handle.get() != nullptr) {
    DWORD internet_option_connect_timeout{ 1000 };
    InternetSetOptionA(pThreadParm->internet_open_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &internet_option_connect_timeout, sizeof(internet_option_connect_timeout));
    pThreadParm->internet_connect_handle.set(InternetConnectA(
      pThreadParm->internet_open_handle.get(), pThreadParm->ftp_host, INTERNET_DEFAULT_FTP_PORT, pThreadParm->user_name, pThreadParm->user_pass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0));

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
  if (pThreadParm->internet_open_handle.get() != nullptr) {
    DWORD internet_option_connect_timeout{ 1000 };
    InternetSetOptionA(pThreadParm->internet_open_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &internet_option_connect_timeout, sizeof(internet_option_connect_timeout));
    pThreadParm->internet_connect_handle.set(InternetConnectA(
      pThreadParm->internet_open_handle.get(), pThreadParm->ftp_host, INTERNET_DEFAULT_FTP_PORT, pThreadParm->user_name, pThreadParm->user_pass, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0));

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
  threadParam.internet_open_handle.set(
    InternetOpenA("IP retriever", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0));

  if (nullptr != threadParam.internet_open_handle.get()) {
    // Create a worker thread
    HANDLE hThread{};
    DWORD dwThreadID{};
    DWORD dwExitCode{};
    DWORD dwTimeout{ 1000 };// 1s for timeout delay

    hThread = CreateThread(nullptr,// Pointer to thread security attributes
      0,// Initial thread stack size, in bytes
      worker_function2,// Pointer to thread function
      &threadParam,// The argument for the new thread
      0,// Creation flags
      &dwThreadID// Pointer to returned thread identifier
    );

    if (nullptr != hThread) {
      // Wait for the call to InternetConnect in worker function to complete
      if (WaitForSingleObject(hThread, dwTimeout) == WAIT_TIMEOUT) {
        print_colored_text(app_handles.hwnd_re_messages_data,
          "^3Could not connect to ^5http://myexternalip.com/raw ^3to "
          "retrieve your ^1external IP address ^3for ^5Tiny^6Rcon "
          "^3updates!\n",
          is_append_message_to_richedit_control::yes,
          is_log_message::yes,
          is_log_datetime::yes);
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
  threadParam.internet_open_handle.set(
    InternetOpenA("tinyrcon_upload_file", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0));
  threadParam.ftp_host = ftp_host;
  threadParam.user_name = user_name;
  threadParam.user_pass = user_pass;
  threadParam.upload_file_path = upload_file_path;
  threadParam.ftp_file_path = ftp_file_path;

  if (nullptr != threadParam.internet_open_handle.get()) {
    // Create a worker thread
    HANDLE hThread{};
    DWORD dwThreadID{};
    DWORD dwExitCode{};
    DWORD dwTimeout{ 1000 };// 1s for timeout delay

    hThread = CreateThread(nullptr,// Pointer to thread security attributes
      0,// Initial thread stack size, in bytes
      worker_function3,// Pointer to thread function
      &threadParam,// The argument for the new thread
      0,// Creation flags
      &dwThreadID// Pointer to returned thread identifier
    );

    if (nullptr != hThread) {
      // Wait for the call to InternetConnect in worker function to complete
      if (WaitForSingleObject(hThread, dwTimeout) == WAIT_TIMEOUT) {
        // Wait until the worker thread exits
        WaitForSingleObject(hThread, INFINITE);
        GetExitCodeThread(hThread, &dwExitCode);
        CloseHandle(hThread);
      }
    }

    if (dwExitCode) {
      /*const string error_msg{ format("^3Could not connect to ^5ftp://{} ^3to
      upload file ^1{}\n ^3to ^5Tiny^6Rcon ^5server!",
      main_app.get_tiny_rcon_server_ip_address(), upload_file_path) };
      print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str(),
      is_append_message_to_richedit_control::yes, is_log_message::yes,
      is_log_datetime::yes);*/
      return false;
    }

    return true;
  }

  return false;
}

bool download_file_from_ftp_server(const char *ftp_host, const char *user_name, const char *user_pass, const char *download_file_path, const char *ftp_file_path)
{
  internet_connection_handles threadParam;
  threadParam.internet_open_handle.set(
    InternetOpenA("tinyrcon_download_file", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0));
  threadParam.ftp_host = ftp_host;
  threadParam.user_name = user_name;
  threadParam.user_pass = user_pass;
  threadParam.download_file_path = download_file_path;
  threadParam.ftp_file_path = ftp_file_path;

  if (nullptr != threadParam.internet_open_handle.get()) {
    // Create a worker thread
    HANDLE hThread{};
    DWORD dwThreadID{};
    DWORD dwExitCode{};
    DWORD dwTimeout{ 1000 };// 1s for timeout delay

    hThread = CreateThread(nullptr,// Pointer to thread security attributes
      0,// Initial thread stack size, in bytes
      worker_function4,// Pointer to thread function
      &threadParam,// The argument for the new thread
      0,// Creation flags
      &dwThreadID// Pointer to returned thread identifier
    );

    if (nullptr != hThread) {
      // Wait for the call to InternetConnect in worker function to complete
      if (WaitForSingleObject(hThread, dwTimeout) == WAIT_TIMEOUT) {
        // Wait until the worker thread exits
        WaitForSingleObject(hThread, INFINITE);
        GetExitCodeThread(hThread, &dwExitCode);
        CloseHandle(hThread);
      }
    }

    if (dwExitCode) {
      const string error_msg{ format("^3Failed to download file ^1{} ^3from ^5{}", ftp_file_path, ftp_host) };
      print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return false;
    }

    return true;
  }
  return false;
}
