#pragma once

#include <atomic>
#include <fstream>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include "tiny_rcon_utility_functions.h"
#include "connection_manager_for_messages.h"
#include "game_server.h"
#include "tiny_rcon_client_user.h"


#undef max

using std::string;

class tiny_rcon_server_application
{
  std::atomic<bool> is_connection_settings_valid{ true };
  bool is_log_file_open{};
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
    { "automatic_remove_temp_ban_msg", "^1{ADMINNAME}: ^7{PLAYERNAME}'s ^1temporary ban ^7[start date: ^3{TEMP_BAN_START_DATE} ^7expired on ^3{TEMP_BAN_END_DATE}]{{br}}^7has automatically been removed. ^5Reason of ban: ^1{REASON}" },
    { "automatic_kick_temp_ban_msg", "^1{ADMINNAME}: ^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}}^7Your temporary ban expires on ^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{TEMP_BAN_START_DATE}" },
    { "automatic_kick_ip_ban_msg",
      "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}" },
    {
      "automatic_kick_city_ban_msg",
      "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an IP address from a ^1banned city: ^5{CITY_NAME} ^7is being automatically ^1kicked.",
    },
    { "automatic_kick_country_ban_msg",
      "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an IP address from a ^1banned country:  ^5{COUNTRY_NAME} ^7is being automatically ^1kicked." }
  };

  game_server server;
  connection_manager_for_messages cm_for_messages;

  std::queue<command_t> command_queue{};
  std::queue<message_t> message_queue{};

  std::vector<std::shared_ptr<tiny_rcon_client_user>> users;
  std::unordered_map<std::string, std::shared_ptr<tiny_rcon_client_user>> name_to_user;

  std::unordered_map<std::string, std::string> tinyrcon_dict{
    { "{ADMINNAME}", username },
    { "{PLAYERNAME}", "" },
    { "{REASON}", "not specified" }
  };

  std::unordered_set<std::string> sent_rcon_public_messages;
  std::unordered_map<std::string, bool> is_user_data_received;
  std::unordered_map<std::string, std::function<void(const std::vector<std::string> &)>> command_handlers;
  std::unordered_map<std::string, std::function<void(const std::string &, time_t, const std::string &, const bool, const string &)>> message_handlers;


  std::string program_title{ "Welcome to TinyRcon" };
  std::string current_working_directory;
  std::string ftp_download_site_ip_address{ "85.222.189.119" };
  std::string ftp_download_folder_path{ "tinyrcon" };
  std::string ftp_bans_folder_path{ "C:\\tinyrcon\\bans" };
  std::string ftp_download_file_pattern{ R"(^_U_TinyRcon[\._-]?v?(\d{1,2}\.\d{1,2}\.\d{1,2}\.\d{1,2})\.exe$)" };
  std::string plugins_geoIP_geo_dat_md5;
  std::ofstream log_file;
  std::mutex user_data_mutex{};
  std::recursive_mutex command_queue_mutex{};
  std::mutex message_queue_mutex{};
  std::string tiny_rcon_ftp_server_username;
  std::string tiny_rcon_ftp_server_password;
  std::string tiny_rcon_server_ip_address;
  int tiny_rcon_server_port{};


public:
  tiny_rcon_server_application() = default;
  ~tiny_rcon_server_application() = default;

  void set_is_connection_settings_valid(const bool new_value)
  {
    is_connection_settings_valid.store(new_value);
  }

  bool get_is_connection_settings_valid() const
  {
    return is_connection_settings_valid.load();
  }

  inline const string &get_username() const
  {
    return username;
  }

  inline void set_username(string new_value)
  {
    remove_disallowed_character_in_string(new_value);
    username = std::move(new_value);
  }

  inline const string &get_game_server_name() const
  {
    return game_server_name;
  }

  inline void set_game_server_name(string new_value)
  {
    game_server_name = std::move(new_value);
  }

  inline const string &get_codmp_exe_path() const
  {
    return codmp_exe_path;
  }

  inline void set_codmp_exe_path(string new_value)
  {
    codmp_exe_path = std::move(new_value);
  }

  inline const string &get_cod2mp_exe_path() const
  {
    return cod2mp_s_exe_path;
  }

  inline void set_cod2mp_exe_path(string new_value)
  {
    cod2mp_s_exe_path = std::move(new_value);
  }

  inline const string &get_iw3mp_exe_path() const
  {
    return iw3mp_exe_path;
  }

  inline void set_iw3mp_exe_path(string new_value)
  {
    iw3mp_exe_path = std::move(new_value);
  }

  inline const string &get_cod5mp_exe_path() const
  {
    return cod5mp_exe_path;
  }

  inline void set_cod5mp_exe_path(string new_value)
  {
    cod5mp_exe_path = std::move(new_value);
  }


  inline void set_command_line_info(string new_value)
  {
    command_line_info = std::move(new_value);
  }

  inline connection_manager_for_messages &get_connection_manager_for_messages()
  {
    return cm_for_messages;
  }

  inline game_server &get_game_server() { return server; }

  inline std::unordered_map<std::string, std::string> &
    get_admin_messages()
  {
    return admin_messages;
  }

  inline const std::string &get_program_title() const
  {
    return program_title;
  }

  inline void set_program_title(std::string new_program_title)
  {
    program_title = std::move(new_program_title);
  }

  std::unordered_map<std::string, std::shared_ptr<tiny_rcon_client_user>> &get_name_to_user()
  {
    return name_to_user;
  }

  std::vector<std::shared_ptr<tiny_rcon_client_user>> &get_users()
  {
    return users;
  }

  std::shared_ptr<tiny_rcon_client_user> &get_user_for_name(const std::string &name, const string &ip_address)
  {
    string cleaned_name{ get_cleaned_user_name(name) };
    player pd{};
    convert_guid_key_to_country_name(cm_for_messages.get_geoip_data(), ip_address, pd);

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

  std::unordered_map<std::string, std::string> &get_tinyrcon_dict()
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

  std::string get_automatic_kick_city_ban_msg()
  {
    return admin_messages["automatic_kick_city_ban_msg"];
  }

  std::string get_automatic_kick_country_ban_msg()
  {
    return admin_messages["automatic_kick_country_ban_msg"];
  }

  const std::string &get_current_working_directory() const
  {
    return current_working_directory;
  }

  void set_current_working_directory()
  {
    char exe_file_path[MAX_PATH];
    if (GetModuleFileNameA(nullptr, exe_file_path, MAX_PATH)) {
      std::string exe_file_path_str{ exe_file_path };
      current_working_directory.assign({ exe_file_path_str.cbegin(), exe_file_path_str.cbegin() + exe_file_path_str.rfind('\\') + 1 });
    }
  }

  const std::string &get_tiny_rcon_ftp_server_username() const
  {
    return tiny_rcon_ftp_server_username;
  }

  void set_tiny_rcon_ftp_server_username(string new_tiny_rcon_ftp_server_username)
  {
    tiny_rcon_ftp_server_username = std::move(new_tiny_rcon_ftp_server_username);
  }

  const std::string &get_tiny_rcon_ftp_server_password() const
  {
    return tiny_rcon_ftp_server_password;
  }

  void set_tiny_rcon_ftp_server_password(string new_tiny_rcon_ftp_server_password)
  {
    tiny_rcon_ftp_server_password = std::move(new_tiny_rcon_ftp_server_password);
  }

  const std::string &get_tiny_rcon_server_ip_address() const
  {
    return tiny_rcon_server_ip_address;
  }

  void set_tiny_rcon_server_ip_address(string new_tiny_rcon_server_ip_address)
  {
    tiny_rcon_server_ip_address = std::move(new_tiny_rcon_server_ip_address);
  }

  int get_tiny_rcon_server_port() const
  {
    return tiny_rcon_server_port;
  }

  void set_tiny_rcon_server_port(const int new_tiny_rcon_server_port)
  {
    tiny_rcon_server_port = new_tiny_rcon_server_port;
  }

  const std::string &get_ftp_download_site_ip_address() const
  {
    return ftp_download_site_ip_address;
  }

  void set_ftp_download_site_ip_address(string new_ftp_download_site)
  {
    ftp_download_site_ip_address = std::move(new_ftp_download_site);
  }

  const std::string &get_ftp_download_folder_path() const
  {
    return ftp_download_folder_path;
  }

  void set_ftp_download_folder_path(string new_ftp_download_folder_path)
  {
    ftp_download_folder_path = std::move(new_ftp_download_folder_path);
  }

  const std::string &get_ftp_bans_folder_path() const
  {
    return ftp_bans_folder_path;
  }

  void set_ftp_bans_folder_path(string new_ftp_bans_folder_path)
  {
    ftp_bans_folder_path = std::move(new_ftp_bans_folder_path);
  }

  const std::string &get_ftp_download_file_pattern() const
  {
    return ftp_download_file_pattern;
  }

  void set_ftp_download_file_pattern(string new_ftp_download_file_pattern)
  {
    ftp_download_file_pattern = std::move(new_ftp_download_file_pattern);
  }

  const std::string &get_plugins_geoIP_geo_dat_md5() const
  {
    return plugins_geoIP_geo_dat_md5;
  }

  void set_plugins_geoIP_geo_dat_md5(string new_plugins_geoIP_geo_dat_md5)
  {
    plugins_geoIP_geo_dat_md5 = std::move(new_plugins_geoIP_geo_dat_md5);
  }

  bool open_log_file(const char *file_path)
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
    std::lock_guard lg{ command_queue_mutex };
    command_queue.emplace(std::move(cmd), cmd_type, wait_for_reply);
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
    if (cmd.type == command_type::user) {
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

  const std::unordered_map<std::string, std::function<void(const std::vector<std::string> &)>> &get_command_handlers() const
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

  std::pair<bool, const std::function<void(const std::vector<std::string> &)> &> get_command_handler(const std::string &command_name) const
  {
    static std::function<void(const std::vector<std::string> &)> unknown_command_handler{
      [](const std::vector<std::string> &) {}
    };

    if (command_handlers.contains(command_name))
      return make_pair(true, command_handlers.at(command_name));
    return make_pair(false, unknown_command_handler);
  }

  const std::unordered_map<std::string, std::function<void(const std::string &, time_t, const std::string &, const bool, const string &)>> &get_message_handlers() const
  {
    return message_handlers;
  }

  void add_message_handler(std::string message_name, std::function<void(const std::string &, time_t, const std::string &, const bool, const string &)> message_handler)
  {
    message_handlers.emplace(std::move(message_name), std::move(message_handler));
  }

  const std::function<void(const std::string &, time_t, const std::string &, const bool, const string &)> &get_message_handler(const std::string &message_name) const
  {
    static std::function<void(const std::string &, time_t, const std::string &, const bool, const string &)> unknown_message_handler{
      [](const std::string &, time_t, const std::string &, const bool, const string &) {}
    };

    if (message_handlers.contains(message_name))
      return message_handlers.at(message_name);
    return unknown_message_handler;
  }

  std::unordered_set<std::string> &get_sent_rcon_public_messages() noexcept
  {
    return sent_rcon_public_messages;
  }

  bool get_is_user_data_received_for_user(const std::string &name, const string &ip_address)
  {
    string cleaned_name{ get_cleaned_user_name(name) };
    player pd{};
    convert_guid_key_to_country_name(cm_for_messages.get_geoip_data(), ip_address, pd);

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
    convert_guid_key_to_country_name(cm_for_messages.get_geoip_data(), ip_address, pd);

    if (name.find("Admin") != string::npos) {
      cleaned_name += std::format("_{}", pd.country_name);
    }

    if (!is_user_data_received.contains(cleaned_name)) {
      is_user_data_received.emplace(cleaned_name, true);
    }

    is_user_data_received[cleaned_name] = true;
  }
};
