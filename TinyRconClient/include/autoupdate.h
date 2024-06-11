#pragma once

// clang-format off
#include <SDKDDKVer.h>
#include <atlbase.h>
#include <Wininet.h>
#include <string>
#include <vector>

#include "internet_handle.h"
#include "tiny_rcon_utility_functions.h"
// clang-format on

DWORD WINAPI worker_function1(void *);
DWORD WINAPI worker_function2(void *);
DWORD WINAPI worker_function3(void *);
DWORD WINAPI worker_function4(void *);

struct internet_connection_handles
{
    internet_handle internet_open_handle;
    internet_handle internet_connect_handle;
    const char *ftp_host{};
    const char *user_name{};
    const char *user_pass{};
    const char *upload_file_path{};
    const char *download_file_path{};
    const char *ftp_file_path{};
};

struct version_data
{
    explicit version_data(const int major = 0, const int minor = 0, const int revision = 0, const int subrevision = 0)
        : major{major}, minor{minor}, revision{revision}, sub_revision{subrevision}
    {
    }
    int major;
    int minor;
    int revision;
    int sub_revision;

    static unsigned long get_version_number(const version_data &ver)
    {
        unsigned long version_number{};
        version_number += ver.major;
        version_number *= 100;
        version_number += ver.minor;
        version_number *= 100;
        version_number += ver.revision;
        version_number *= 100;
        version_number += ver.sub_revision;
        return version_number;
    }

    static unsigned long get_version_number(const std::string &file_name)
    {
        unsigned long version_number{};

        for (size_t start{}, last; (start = file_name.find_first_of("0123456789", start)) != std::string::npos;)
        {
            last = file_name.find_first_not_of("0123456789", start + 1);
            version_number *= 100;
            if (std::string::npos == last)
            {
                last = file_name.length();
            }
            version_number += stoul(file_name.substr(start, last - start));
            start = last + 1;
        }

        return version_number;
    }
};

class auto_update_manager
{

  public:
    auto_update_manager() = default;
    ~auto_update_manager() = default;

    bool get_file_version(const std::string &exe_file, version_data &ver,
                          unsigned long &version_number /*, const bool is_print_information = true*/) const;
    const std::string &get_self_full_path() const;
    void replace_temporary_version();
    void downloaded_latest_version_of_program() const;
    void set_self_current_working_directory(std::string cwd);
    const std::string &get_self_current_working_directory() const;
    void set_self_full_path(std::string path);
    const std::string &get_self_file_name() const noexcept;
    void set_self_file_name(std::string file_name);
    std::vector<std::string> get_file_name_matches_for_specified_ftp_path_and_file_pattern(
        internet_handle &internet_connect_handle, const char *relative_path, const char *file_pattern) const;
    bool download_file(const char *download_url, const char *downloaded_file_path) const;
    bool download_file(const wchar_t *download_url, const wchar_t *downloaded_file_path) const;
    void check_for_updates(const char *exe_file_path);

  private:
    mutable unsigned long current_version_number{};
    unsigned long next_version_number{};
    std::string next_version_number_str;
    bool is_current_instance_temporary_version{};
    std::string self_current_working_directory;
    std::string self_full_path;
    std::string self_file_name;
    std::string latest_version_to_download;
    mutable char download_file_path_pattern[512]{};
    mutable char message_buffer[512]{};
};

// std::wstring GetFileNameFromPath(const std::wstring &);
std::string get_tiny_rcon_client_external_ip_address();
bool upload_file_to_ftp_server(const char *ftp_host, const char *user_name, const char *user_pass,
                               const char *upload_file_path, const char *ftp_file_path);
bool download_file_from_ftp_server(const char *ftp_host, const char *user_name, const char *user_pass,
                                   const char *download_file_path, const char *ftp_file_path);
