#pragma once

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include "connection_manager.h"
#include "connection_manager_for_messages.h"
#include "game_server.h"
#include "tiny_rcon_client_user.h"
#include "internet_handle.h"
#include "tiny_rcon_utility_functions.h"
#include <format>

#undef max

using std::string;

extern tiny_rcon_handles app_handles;

class tiny_rcon_client_application
{
  bool is_log_file_open{};
  bool is_draw_border_lines{ true };
  bool is_disable_automatic_kick_messages{ false };
  bool is_use_original_admin_messages{ true };
  bool is_ftp_server_online{ true };
  game_name_t game_name{ game_name_t::unknown };
  std::condition_variable command_queue_cv{};
  string username{ "^1Admin" };
  string game_server_name{
    "185.158.113.146:28995 CoD2 CTF"
  };
  string codmp_exe_path;
  string cod2mp_s_exe_path;
  string iw3mp_exe_path;
  string cod5mp_exe_path;
  string command_line_info;
  std::unordered_map<std::string, std::string> admin_messages{
    { "user_defined_warn_msg", "^7{PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
    { "user_defined_kick_msg", "^7{PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
    { "user_defined_temp_ban_msg", "^7{PLAYERNAME} ^7you are being ^1temporarily banned ^7for ^1{TEMPBAN_DURATION} hours ^7by ^1admin {ADMINNAME}.{{br}}^3Reason: ^1{REASON}" },
    { "user_defined_ban_msg", "^7{PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
    { "user_defined_ip_ban_msg", "^7{PLAYERNAME} ^1you are being permanently banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
    { "user_defined_ip_address_range_ban_msg", "^7Admin ^5{ADMINNAME} ^7has ^1banned IP address range ^5{IP_ADDRESS_RANGE}. ^3Reason: ^1{REASON}" },
    { "user_defined_city_ban_msg", "^7Admin ^5{ADMINNAME} ^1has globally banned city: ^5{CITY_NAME}" },
    { "user_defined_city_unban_msg", "^7Admin ^5{ADMINNAME} ^1has removed previously banned city: ^5{CITY_NAME}" },
    { "user_defined_enable_city_ban_feature_msg", "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned cities." },
    {
      "user_defined_disable_city_ban_feature_msg",
      "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned cities.",
    },
    {
      "user_defined_country_ban_msg",
      "^7Admin ^5{ADMINNAME} ^1has globally banned country: ^5{COUNTRY_NAME}",
    },
    { "user_defined_country_unban_msg", "^7Admin ^5{ADMINNAME} ^1has removed previously banned country: ^5{COUNTRY_NAME}" },
    { "user_defined_enable_country_ban_feature_msg", "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned countries." },
    { "user_defined_disable_country_ban_feature_msg", "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned countries." },
    { "automatic_remove_temp_ban_msg", "^1{BANNED_BY}: ^7{PLAYERNAME}'s ^1temporary ban ^7[start date: ^3{TEMP_BAN_START_DATE} ^7expired on ^3{TEMP_BAN_END_DATE}]{{br}}^7has automatically been removed. ^5Reason of ban: ^1{REASON}" },
    { "automatic_kick_temp_ban_msg", "^1{BANNED_BY}: ^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}}^7Your temporary ban expires on ^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{TEMP_BAN_START_DATE}" },
    { "automatic_kick_ip_ban_msg",
      "^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}" },
    { "automatic_kick_ip_address_range_ban_msg", "^7Player {PLAYERNAME} ^7with an ^1IP address ^7from a previously ^1banned IP address range ^7is being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}" },
    {
      "automatic_kick_city_ban_msg",
      "^7Player {PLAYERNAME} ^7with an IP address from a ^1banned city: ^5{CITY_NAME} ^7is being automatically ^1kicked.",
    },
    { "automatic_kick_country_ban_msg",
      "^7Player {PLAYERNAME} ^7with an IP address from a ^1banned country:  ^5{COUNTRY_NAME} ^7is being automatically ^1kicked." },
    { "user_defined_protect_ip_address_message", "^1{ADMINNAME} ^7has protected ^1IP address ^7of {PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}" },
    { "user_defined_unprotect_ip_address_message", "^1{ADMINNAME} ^7has removed a previously protected ^1IP address^7.{{br}}^5Reason: ^1{REASON}" },
    { "user_defined_protect_ip_address_range_message", "^1{ADMINNAME} ^7has protected ^1IP address range ^7of {PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}" },
    { "user_defined_unprotect_ip_address_range_message", "^1{ADMINNAME} ^7has removed a previously ^1protected IP address range^7.{{br}}^5Reason: ^1{REASON}" },
    { "user_defined_protect_city_message", "^1{ADMINNAME} ^7has protected ^1city ({CITY_NAME}) ^7of {PLAYERNAME}^7." },
    { "user_defined_unprotect_city_message", "^1{ADMINNAME} ^7has removed a previously ^1protected city: {CITY_NAME}" },
    { "user_defined_protect_country_message", "^1{ADMINNAME} ^7has protected ^1country ({COUNTRY_NAME}) ^7of {PLAYERNAME}^7." },
    { "user_defined_unprotect_country_message", "^1{ADMINNAME} ^7has removed a previously ^1protected country: {COUNTRY_NAME}" }
  };

  const std::unordered_map<game_name_t, std::string> game_names{
    { game_name_t::unknown, "Unknown game!" },
    { game_name_t::cod1, "Call of Duty Multiplayer" },
    { game_name_t::cod2, "Call of Duty 2 Multiplayer" },
    { game_name_t::cod4, "Call of Duty 4: Modern Warfare" },
    { game_name_t::cod5, "Call of Duty 5: World at War" }
  };

  game_server server;
  connection_manager rcon_connection_manager;
  connection_manager_for_messages cm_for_messages;
  std::queue<command_t> command_queue{};
  std::queue<message_t> message_queue{};

  std::vector<std::shared_ptr<tiny_rcon_client_user>> users;
  std::unordered_set<std::string> seen_inform_messages;
  std::unordered_map<std::string, std::shared_ptr<tiny_rcon_client_user>> name_to_user;
  std::unordered_map<std::string, bool> is_user_data_received;

  std::unordered_map<std::string, std::string> tinyrcon_dict{
    { "{ADMINNAME}", username },
    { "{PLAYERNAME}", "" },
    { "{REASON}", "not specified" }
  };

  std::unordered_map<std::string, std::function<void(const std::vector<std::string> &)>> command_handlers;
  std::unordered_map<std::string, std::function<void(const std::string &, const time_t, const std::string &, const bool)>> message_handlers;

  std::shared_ptr<tiny_rcon_client_user> current_user{ nullptr };

  std::string program_title{ "Welcome to TinyRcon client" };
  std::string user_ip_address;
  std::string current_working_directory;
  std::string tinyrcon_config_file_path{ "config\\tinyrcon.json" };
  std::string user_data_file_path{ "data\\user.txt" };
  std::string temp_bans_file_path{ "data\\tempbans.txt" };
  std::string ip_bans_file_path{ "data\\bans.txt" };
  std::string ip_range_bans_file_path{ "data\\ip_range_bans.txt" };
  std::string banned_countries_file_path{ "data\\banned_countries.txt" };
  std::string banned_cities_file_path{ "data\\banned_cities.txt" };
  std::string protected_ip_addresses_file_path{ "data\\protected_ip_addresses.txt" };
  std::string protected_ip_address_ranges_file_path{ "data\\protected_ip_address_ranges.txt" };
  std::string protected_cities_file_path{ "data\\protected_cities.txt" };
  std::string protected_countries_file_path{ "data\\protected_countries.txt" };
  std::string ftp_download_site_ip_address{ "85.222.189.119" };
  std::string ftp_download_folder_path{ "tinyrcon" };
  std::string ftp_download_file_pattern{ R"(^_U_TinyRcon[\._-]?v?(\d{1,2}\.\d{1,2}\.\d{1,2}\.\d{1,2})\.exe$)" };
  std::string plugins_geoIP_geo_dat_md5;
  std::ofstream log_file;
  // std::mutex user_data_mutex{};
  std::mutex command_mutex{};
  std::recursive_mutex command_queue_mutex{};
  std::recursive_mutex message_queue_mutex{};
  std::string tiny_rcon_ftp_server_username{ "tinyrcon" };
  std::string tiny_rcon_ftp_server_password{ "08021980" };
  std::string tiny_rcon_server_ip_address{
    "85.222.189.119"
  };
  uint_least16_t tiny_rcon_server_port{ 27015 };


public:
  tiny_rcon_client_application() = default;
  ~tiny_rcon_client_application() = default;

  bool get_is_disable_automatic_kick_messages() const noexcept
  {
    return is_disable_automatic_kick_messages;
  }

  void set_is_disable_automatic_kick_messages(const bool new_value) noexcept
  {
    is_disable_automatic_kick_messages = new_value;
  }

  bool get_is_use_original_admin_messages() const noexcept
  {
    return is_use_original_admin_messages;
  }

  void set_is_use_original_admin_messages(const bool new_value) noexcept
  {
    is_use_original_admin_messages = new_value;
  }

  bool get_is_ftp_server_online() const noexcept
  {
    return is_ftp_server_online;
  }

  void set_is_ftp_server_online(const bool new_value) noexcept
  {
    is_ftp_server_online = new_value;
  }

  inline game_name_t get_game_name() const noexcept
  {
    return game_name;
  }

  inline void set_game_name(const game_name_t new_game_name) noexcept
  {
    game_name = new_game_name;
  }

  inline std::condition_variable &get_command_queue_cv() noexcept
  {
    return command_queue_cv;
  }

  inline std::mutex &get_command_queue_mutex() noexcept
  {
    return command_mutex;
  }

  inline const string &get_username() const noexcept
  {
    return username;
  }

  inline void set_username(string new_value) noexcept
  {
    remove_disallowed_character_in_string(new_value);
    username = std::move(new_value);
  }

  inline const string &get_game_server_name() const noexcept
  {
    return game_server_name;
  }

  inline void set_game_server_name(string new_value) noexcept
  {
    game_server_name = std::move(new_value);
  }

  inline const string &get_codmp_exe_path() const noexcept
  {
    return codmp_exe_path;
  }

  inline void set_codmp_exe_path(string new_value) noexcept
  {
    codmp_exe_path = std::move(new_value);
  }

  inline const string &get_cod2mp_exe_path() const noexcept
  {
    return cod2mp_s_exe_path;
  }

  inline void set_cod2mp_exe_path(string new_value) noexcept
  {
    cod2mp_s_exe_path = std::move(new_value);
  }

  inline const string &get_iw3mp_exe_path() const noexcept
  {
    return iw3mp_exe_path;
  }

  inline void set_iw3mp_exe_path(string new_value) noexcept
  {
    iw3mp_exe_path = std::move(new_value);
  }

  inline const string &get_cod5mp_exe_path() const noexcept
  {
    return cod5mp_exe_path;
  }

  inline void set_cod5mp_exe_path(string new_value) noexcept
  {
    cod5mp_exe_path = std::move(new_value);
  }


  inline void set_command_line_info(string new_value) noexcept
  {
    command_line_info = std::move(new_value);
  }

  inline connection_manager &get_connection_manager() noexcept
  {
    return rcon_connection_manager;
  }

  inline connection_manager_for_messages &get_connection_manager_for_messages() noexcept
  {
    return cm_for_messages;
  }

  inline game_server &get_game_server() noexcept { return server; }

  inline std::unordered_map<std::string, std::string> &
    get_admin_messages() noexcept
  {
    return admin_messages;
  }

  inline const std::string &get_program_title() const noexcept
  {
    return program_title;
  }

  inline void set_program_title(std::string new_program_title) noexcept
  {
    program_title = std::move(new_program_title);
  }

  inline const std::string &get_user_ip_address() const noexcept
  {
    return user_ip_address;
  }

  inline void set_user_ip_address(std::string new_user_ip_address) noexcept
  {
    user_ip_address = std::move(new_user_ip_address);
  }

  std::unordered_set<std::string> &get_already_seen_messages() noexcept
  {
    return seen_inform_messages;
  }

  std::unordered_map<std::string, std::shared_ptr<tiny_rcon_client_user>> &get_name_to_user()
  {
    return name_to_user;
  }

  std::unordered_map<std::string, bool> &get_is_user_data_received()
  {
    return is_user_data_received;
  }

  bool get_is_user_data_received_for_user(const std::string &name, const string &ip_address)
  {
    string cleaned_name{ get_cleaned_user_name(name) };
    player pd{};
    convert_guid_key_to_country_name(rcon_connection_manager.get_geoip_data(), ip_address, pd);

    if (name.find("Admin") != string::npos) {
      cleaned_name += std::format("_{}", pd.country_name);
    }

    if (!is_user_data_received.contains(cleaned_name)) {
      is_user_data_received.emplace(cleaned_name, false);
    }

    return is_user_data_received[cleaned_name];
  }

  void set_is_user_data_received_for_user(const std::string &name, const string &ip_address)
  {
    string cleaned_name{ get_cleaned_user_name(name) };
    player pd{};
    convert_guid_key_to_country_name(rcon_connection_manager.get_geoip_data(), ip_address, pd);

    if (name.find("Admin") != string::npos) {
      cleaned_name += std::format("_{}", pd.country_name);
    }

    if (!is_user_data_received.contains(cleaned_name)) {
      is_user_data_received.emplace(cleaned_name, true);
    }

    is_user_data_received[cleaned_name] = true;
  }

  std::vector<std::shared_ptr<tiny_rcon_client_user>> &get_users()
  {
    return users;
  }

  std::shared_ptr<tiny_rcon_client_user> &get_user_for_name(const std::string &name, const string &ip_address)
  {
    string cleaned_name{ get_cleaned_user_name(name) };
    player pd{};
    convert_guid_key_to_country_name(rcon_connection_manager.get_geoip_data(), ip_address, pd);

    if (name.find("Admin") != string::npos) {
      cleaned_name += std::format("_{}", pd.country_name);
    }

    if (!name_to_user.contains(cleaned_name)) {
      users.emplace_back(std::make_shared<tiny_rcon_client_user>());
      users.back()->user_name = name;
      name_to_user.emplace(cleaned_name, users.back());
    }

    return name_to_user.at(cleaned_name);
  }

  std::shared_ptr<tiny_rcon_client_user> &get_current_user() noexcept
  {
    if (!current_user)
      current_user = get_user_for_name(get_username(), get_user_ip_address());

    return current_user;
  }

  std::unordered_map<std::string, std::string> &get_tinyrcon_dict() noexcept
  {
    return tinyrcon_dict;
  }

  std::string get_user_defined_warn_message()
  {
    return admin_messages["user_defined_warn_msg"];
  }

  std::string get_user_defined_kick_message()
  {
    return admin_messages["user_defined_kick_msg"];
  }

  std::string get_user_defined_tempban_message()
  {
    return admin_messages["user_defined_temp_ban_msg"];
  }

  std::string get_user_defined_ban_message()
  {
    return admin_messages["user_defined_ban_msg"];
  }

  std::string get_user_defined_ipban_message()
  {
    return admin_messages["user_defined_ip_ban_msg"];
  }

  std::string get_user_defined_ip_address_range_ban_message()
  {
    return admin_messages["user_defined_ip_address_range_ban_msg"];
  }

  std::string get_user_defined_city_ban_msg()
  {
    return admin_messages["user_defined_city_ban_msg"];
  }

  std::string get_user_defined_city_unban_msg()
  {
    return admin_messages["user_defined_city_unban_msg"];
  }

  std::string get_user_defined_enable_city_ban_feature_msg()
  {
    return admin_messages["user_defined_enable_city_ban_feature_msg"];
  }

  std::string get_user_defined_disable_city_ban_feature_msg()
  {
    return admin_messages["user_defined_disable_city_ban_feature_msg"];
  }

  std::string get_user_defined_country_ban_msg()
  {
    return admin_messages["user_defined_country_ban_msg"];
  }

  std::string get_user_defined_country_unban_msg()
  {
    return admin_messages["user_defined_country_unban_msg"];
  }

  std::string get_user_defined_enable_country_ban_feature_msg()
  {
    return admin_messages["user_defined_enable_country_ban_feature_msg"];
  }

  std::string get_user_defined_disable_country_ban_feature_msg()
  {
    return admin_messages["user_defined_disable_country_ban_feature_msg"];
  }

  std::string get_automatic_remove_temp_ban_msg()
  {
    return admin_messages["automatic_remove_temp_ban_msg"];
  }

  std::string get_automatic_kick_temp_ban_msg()
  {
    return admin_messages["automatic_kick_temp_ban_msg"];
  }

  std::string get_automatic_kick_ip_ban_msg()
  {
    return admin_messages["automatic_kick_ip_ban_msg"];
  }

  std::string get_automatic_kick_ip_address_range_ban_msg()
  {
    return admin_messages["automatic_kick_ip_address_range_ban_msg"];
  }

  std::string get_automatic_kick_city_ban_msg()
  {
    return admin_messages["automatic_kick_city_ban_msg"];
  }

  std::string get_automatic_kick_country_ban_msg()
  {
    return admin_messages["automatic_kick_country_ban_msg"];
  }

  std::string get_user_defined_protect_ip_address_message()
  {
    return admin_messages["user_defined_protect_ip_address_message"];
  }

  std::string get_user_defined_unprotect_ip_address_message()
  {
    return admin_messages["user_defined_unprotect_ip_address_message"];
  }

  std::string get_user_defined_protect_ip_address_range_message()
  {
    return admin_messages["user_defined_protect_ip_address_range_message"];
  }

  std::string get_user_defined_unprotect_ip_address_range_message()
  {
    return admin_messages["user_defined_unprotect_ip_address_range_message"];
  }

  std::string get_user_defined_protect_city_message()
  {
    return admin_messages["user_defined_protect_city_message"];
  }

  std::string get_user_defined_unprotect_city_message()
  {
    return admin_messages["user_defined_unprotect_city_message"];
  }

  std::string get_user_defined_protect_country_message()
  {
    return admin_messages["user_defined_protect_country_message"];
  }

  std::string get_user_defined_unprotect_country_message()
  {
    return admin_messages["user_defined_unprotect_country_message"];
  }

  inline const char *get_game_title() noexcept
  {
    if (game_names.contains(game_name))
      return game_names.at(game_name).c_str();
    return "Unknown game!";
  }

  bool get_is_draw_border_lines() const noexcept
  {
    return is_draw_border_lines;
  }

  void set_is_draw_border_lines(const bool new_value) noexcept
  {
    is_draw_border_lines = new_value;
  }

  const std::string &get_current_working_directory() const noexcept
  {
    return current_working_directory;
  }

  void set_current_working_directory()
  {
    constexpr size_t max_path_limit{ 32768 };
    std::unique_ptr<char[]> exe_file_path{ std::make_unique<char[]>(max_path_limit) };
    if (const auto path_len = GetModuleFileNameA(nullptr, exe_file_path.get(), max_path_limit); path_len != 0) {
      std::string_view exe_file_path_sv(exe_file_path.get(), path_len);
      current_working_directory.assign(string(cbegin(exe_file_path_sv), cbegin(exe_file_path_sv) + exe_file_path_sv.rfind('\\') + 1));
    } else {
      std::filesystem::path entry("TinyRcon.exe");
      current_working_directory.assign(entry.parent_path().string());
      if (!current_working_directory.empty() && current_working_directory.back() != '\\') {
        if ('/' == current_working_directory.back())
          current_working_directory.back() = '\\';
        else
          current_working_directory.push_back('\\');
      }
    }

    tinyrcon_config_file_path.assign(format("{}{}", current_working_directory, tinyrcon_config_file_path));
    user_data_file_path.assign(format("{}{}", current_working_directory, user_data_file_path));
    temp_bans_file_path.assign(format("{}{}", current_working_directory, temp_bans_file_path));
    ip_bans_file_path.assign(format("{}{}", current_working_directory, ip_bans_file_path));
    ip_range_bans_file_path.assign(format("{}{}", current_working_directory, ip_range_bans_file_path));
    banned_countries_file_path.assign(format("{}{}", current_working_directory, banned_countries_file_path));
    banned_cities_file_path.assign(format("{}{}", current_working_directory, banned_cities_file_path));
    protected_ip_addresses_file_path.assign(format("{}{}", current_working_directory, protected_ip_addresses_file_path));
    protected_ip_address_ranges_file_path.assign(format("{}{}", current_working_directory, protected_ip_address_ranges_file_path));
    protected_cities_file_path.assign(format("{}{}", current_working_directory, protected_cities_file_path));
    protected_countries_file_path.assign(format("{}{}", current_working_directory, protected_countries_file_path));
  }

  const char *get_tinyrcon_config_file_path() const noexcept
  {
    return tinyrcon_config_file_path.c_str();
  }

  const char *get_user_data_file_path() const noexcept
  {
    return user_data_file_path.c_str();
  }

  const char *get_temp_bans_file_path() const noexcept
  {
    return temp_bans_file_path.c_str();
  }

  const char *get_ip_bans_file_path() const noexcept
  {
    return ip_bans_file_path.c_str();
  }

  const char *get_ip_range_bans_file_path() const noexcept
  {
    return ip_range_bans_file_path.c_str();
  }

  const char *get_banned_countries_file_path() const noexcept
  {
    return banned_countries_file_path.c_str();
  }

  const char *get_banned_cities_file_path() const noexcept
  {
    return banned_cities_file_path.c_str();
  }

  const char *get_protected_ip_addresses_file_path() const noexcept
  {
    return protected_ip_addresses_file_path.c_str();
  }

  const char *get_protected_ip_address_ranges_file_path() const noexcept
  {
    return protected_ip_address_ranges_file_path.c_str();
  }

  const char *get_protected_cities_file_path() const noexcept
  {
    return protected_cities_file_path.c_str();
  }

  const char *get_protected_countries_file_path() const noexcept
  {
    return protected_countries_file_path.c_str();
  }

  const std::string &get_tiny_rcon_ftp_server_username() const noexcept
  {
    return tiny_rcon_ftp_server_username;
  }

  void set_tiny_rcon_ftp_server_username(string new_tiny_rcon_ftp_server_username) noexcept
  {
    tiny_rcon_ftp_server_username = std::move(new_tiny_rcon_ftp_server_username);
  }

  const std::string &get_tiny_rcon_ftp_server_password() const noexcept
  {
    return tiny_rcon_ftp_server_password;
  }

  void set_tiny_rcon_ftp_server_password(string new_tiny_rcon_ftp_server_password) noexcept
  {
    tiny_rcon_ftp_server_password = std::move(new_tiny_rcon_ftp_server_password);
  }

  // tiny_rcon_server_ip_address
  const std::string &get_tiny_rcon_server_ip_address() const noexcept
  {
    return tiny_rcon_server_ip_address;
  }

  void set_tiny_rcon_server_ip_address(string new_tiny_rcon_server_ip_address) noexcept
  {
    tiny_rcon_server_ip_address = std::move(new_tiny_rcon_server_ip_address);
  }

  // tiny_rcon_server_port
  uint_least16_t get_tiny_rcon_server_port() const noexcept
  {
    return tiny_rcon_server_port;
  }

  void set_tiny_rcon_server_port(const int new_tiny_rcon_server_port) noexcept
  {
    tiny_rcon_server_port = static_cast<uint_least16_t>(new_tiny_rcon_server_port);
  }

  const std::string &get_ftp_download_site_ip_address() const noexcept
  {
    return ftp_download_site_ip_address;
  }

  void set_ftp_download_site_ip_address(string new_ftp_download_site) noexcept
  {
    ftp_download_site_ip_address = std::move(new_ftp_download_site);
  }
  const std::string &get_ftp_download_folder_path() const noexcept
  {
    return ftp_download_folder_path;
  }

  void set_ftp_download_folder_path(string new_ftp_download_folder_path) noexcept
  {
    ftp_download_folder_path = std::move(new_ftp_download_folder_path);
  }

  const std::string &get_ftp_download_file_pattern() const noexcept
  {
    return ftp_download_file_pattern;
  }

  void set_ftp_download_file_pattern(string new_ftp_download_file_pattern) noexcept
  {
    ftp_download_file_pattern = std::move(new_ftp_download_file_pattern);
  }

  const std::string &get_plugins_geoIP_geo_dat_md5() const noexcept
  {
    return plugins_geoIP_geo_dat_md5;
  }

  void set_plugins_geoIP_geo_dat_md5(string new_plugins_geoIP_geo_dat_md5) noexcept
  {
    plugins_geoIP_geo_dat_md5 = std::move(new_plugins_geoIP_geo_dat_md5);
  }

  bool open_log_file(const char *file_path) noexcept
  {
    if (log_file.is_open()) {
      log_file.flush();
      log_file.close();
    }

    log_file.open(file_path, std::ios::app);
    is_log_file_open = !log_file.fail();
    return is_log_file_open;
  }

  void log_message(const std::string &message)
  {
    if (is_log_file_open) {
      log_file << message << std::flush;
    }
  }

  bool is_command_queue_empty()
  {
    std::lock_guard lg{ command_queue_mutex };
    return command_queue.empty();
  }

  inline void add_command_to_queue(std::vector<std::string> cmd, const command_type cmd_type, const bool wait_for_reply)
  {
    {
      std::lock_guard lg{ command_queue_mutex };
      command_queue.emplace(std::move(cmd), cmd_type, wait_for_reply);
    }
    // std::unique_lock ul{ command_mutex };
    command_queue_cv.notify_one();
  }

  inline command_t get_command_from_queue()
  {
    std::lock_guard lg{ command_queue_mutex };
    command_t next_command = command_queue.front();
    command_queue.pop();
    return next_command;
  }

  inline void process_queue_command(command_t cmd)
  {
    std::lock_guard lg{ command_queue_mutex };
    if (cmd.type == command_type::rcon) {
      process_rcon_command(cmd.command);
    } else if (cmd.type == command_type::user) {
      process_user_command(cmd.command);
    }
  }

  bool is_message_queue_empty()
  {
    std::lock_guard lg{ message_queue_mutex };
    return message_queue.empty();
  }

  inline void add_message_to_queue(message_t message)
  {
    std::lock_guard lg{ message_queue_mutex };
    message_queue.emplace(std::move(message));
  }

  inline message_t get_message_from_queue()
  {
    std::lock_guard lg{ message_queue_mutex };
    auto next_message = std::move(message_queue.front());
    message_queue.pop();
    return next_message;
  }

  const std::unordered_map<std::string, std::function<void(const std::vector<std::string> &)>> &get_command_handlers() const noexcept
  {
    return command_handlers;
  }

  void add_command_handler(std::vector<std::string> command_names, std::function<void(const std::vector<std::string> &)> command_handler)
  {
    if (command_names.size() > 1) {
      for (size_t i{}; i < command_names.size() - 1; ++i) {
        command_handlers.emplace(std::move(command_names[i]), command_handler);
      }
    }
    command_handlers.emplace(std::move(command_names.back()), std::move(command_handler));
  }

  std::pair<bool, const std::function<void(const std::vector<std::string> &)> &> get_command_handler(const std::string &command_name) const noexcept
  {
    static std::function<void(const std::vector<std::string> &)> unknown_command_handler{
      [](const std::vector<std::string> &) {}
    };

    if (command_handlers.contains(command_name))
      return make_pair(true, command_handlers.at(command_name));
    return make_pair(false, unknown_command_handler);
  }

  const std::unordered_map<std::string, std::function<void(const std::string &, const time_t, const std::string &, const bool)>> &get_message_handlers() const noexcept
  {
    return message_handlers;
  }

  void add_message_handler(std::string message_name, std::function<void(const std::string &, const time_t, const std::string &, const bool)> message_handler)
  {
    message_handlers.emplace(std::move(message_name), std::move(message_handler));
  }

  const std::function<void(const std::string &, const time_t, const std::string &, const bool)> &get_message_handler(const std::string &message_name) const noexcept
  {
    static std::function<void(const std::string &, const time_t, const std::string &, const bool)> unknown_message_handler{
      [](const std::string &, const time_t, const std::string &, const bool) {}
    };

    if (message_handlers.contains(message_name))
      return message_handlers.at(message_name);
    return unknown_message_handler;
  }
};
