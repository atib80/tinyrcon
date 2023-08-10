/*
Source File : autoupdate.h
Created for the purpose of demonstration for http://www.codeproject.com

Copyright 2017 Michael Haephrati, Secured Globe Inc.
See also: https://www.codeproject.com/script/Membership/View.aspx?mid=5956881

Secured Globe, Inc.
http://www.securedglobe.com
*/

#pragma once

#include <SDKDDKVer.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS// some CString constructors will be explicit

#include <atlbase.h>
#include <atlstr.h>
#include <Wininet.h>
#include <string>
#include "tiny_cod2_rcon_client_application.h"

struct version_data
{
  explicit version_data(const int major = 0, const int minor = 0, const int revision = 0, const int subrevision = 0) : major{ major }, minor{ minor }, revision{ revision }, sub_revision{ subrevision } {}
  int major;
  int minor;
  int revision;
  int sub_revision;
};

class auto_update_manager
{
public:
  auto_update_manager(tiny_cod2_rcon_client_application &main_app);
  ~auto_update_manager() = default;  

  bool get_file_version(const std::string &exe_file, version_data& ver, unsigned long &version_number) const;
  const std::string &get_self_full_path() const;
  void replace_temporary_version();
  bool check_for_updates();
  void set_self_current_working_directory(std::string cwd) noexcept;
  void set_self_full_path(std::string path) noexcept;
  const std::string &get_self_file_name() const;
  void set_self_file_name(std::string file_name);

private:
  tiny_cod2_rcon_client_application &app;
  mutable unsigned long current_version_number{};
  unsigned long next_version_number{};
  string next_version_number_str;
  bool is_current_instance_temporary_version{};
  std::string self_current_working_directory;
  std::string self_full_path;
  std::string self_file_name;
  std::string latest_version_to_download;
  char download_file_path_pattern[512]{};
  mutable char message_buffer[512]{};
};

unsigned long get_version_number(const std::string &);
unsigned long get_version_number(const version_data&) noexcept;
std::string GetFileNameFromPath(const std::string &);

class MyCallback : public IBindStatusCallback
{
public:
  MyCallback() = default;
  ~MyCallback() = default;

  // This one is called by URLDownloadToFile
  STDMETHOD(OnProgress)
  (/* [in] */ ULONG ulProgress, /* [in] */ ULONG ulProgressMax, /* [in] */ ULONG ulStatusCode, /* [in] */ LPCWSTR)
  {
    // You can use your own logging function here
    wprintf(L"Downloaded %d of %d. Status code %d\n", ulProgress, ulProgressMax, ulStatusCode);
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
  (/* [in] */ DWORD, /* [in] */ DWORD, /* [in] */ FORMATETC __RPC_FAR *, /* [in] */ STGMEDIUM __RPC_FAR *)
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
