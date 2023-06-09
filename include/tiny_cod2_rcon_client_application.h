#pragma once

#include <atomic>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include "connection_manager.h"
#include "game_server.h"

using std::string;

class tiny_cod2_rcon_client_application
{
  std::atomic<bool> is_connection_settings_valid{ false };
  bool is_enable_automatic_connection_flood_ip_ban{ true };
  bool is_log_file_open{};
  bool is_draw_border_lines{ true };
  size_t minimum_number_of_connections_from_same_ip_for_automatic_ban{ 5 };
  size_t maximum_number_of_warnings_for_automatic_kick{ 2 };
  game_name_t game_name{ game_name_t::unknown };
  string username{ "^1Administrator" };
  string game_server_name;
  string codmp_exe_path;
  string cod2mp_s_exe_path;
  string iw3mp_exe_path;
  string cod5mp_exe_path;
  string command_line_info;
  std::unordered_map<std::string, std::string> admin_messages{
    { "user_defined_warn_msg", "^7{PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
    { "user_defined_kick_msg", "^7{PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
    { "user_defined_temp_ban_msg", "^7{PLAYERNAME} ^7you are being ^1temporarily banned ^7for ^1{TEMPBAN_DURATION} hours ^7by ^1admin {ADMINNAME}.{{br}} ^3Reason: ^1{REASON}" },
    { "user_defined_ban_msg", "^7{PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
    { "user_defined_ip_ban_msg", "^7{PLAYERNAME} ^1you are being permanently banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}" },
    { "automatic_remove_temp_ban_msg", "^1{ADMINNAME}: ^7{PLAYERNAME}'s ^1tempban ^7[start date: ^3{TEMP_BAN_START_DATE} ^7expired on ^3{TEMP_BAN_END_DATE}]{{br}} ^7has automatically been removed. ^5Reason of ban: ^1{REASON}" },
    { "automatic_kick_temp_ban_msg", "^1{ADMINNAME}: ^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}} ^7Your temporary ban expires on ^1{TEMP_BAN_END_DATE}.{{br}} ^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{TEMP_BAN_START_DATE}" },
    {
      "automatic_kick_ip_ban_msg",
      "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is being automatically ^1kicked.{{br}} ^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}",
    }
  };

  const std::unordered_map<game_name_t, std::string> game_names{
    { game_name_t::unknown, "Unknown game!" },
    { game_name_t::cod1, "Call of Duty Multiplayer" },
    { game_name_t::cod2, "Call of Duty 2 Multiplayer" },
    { game_name_t::cod4, "Call of Duty 4: Modern Warfare" },
    { game_name_t::cod5, "Call of Duty 5: World at War" }
  };
  string server_message{ "^5| ^3Server is empty!^5" };
  game_server server;
  connection_manager rcon_connection_manager;
  std::queue<command_t> command_queue{};
  std::unordered_map<std::string, std::string> tinyrcon_dict{
    { "{ADMINNAME}", username },
    { "{PLAYERNAME}", "" },
    { "{REASON}", "not specified" }
  };

  std::string program_title{ "Welcome to TinyRcon!" };
  std::ofstream log_file;
  std::recursive_mutex command_queue_mutex{};


public:
  tiny_cod2_rcon_client_application() = default;
  ~tiny_cod2_rcon_client_application() = default;

  void set_is_connection_settings_valid(const bool new_value) noexcept
  {
    is_connection_settings_valid.store(new_value);
  }

  bool get_is_connection_settings_valid() const noexcept
  {
    return is_connection_settings_valid.load();
  }

  bool get_is_enable_automatic_connection_flood_ip_ban() const noexcept
  {
    return is_enable_automatic_connection_flood_ip_ban;
  }

  void set_is_enable_automatic_connection_flood_ip_ban(const bool new_value) noexcept
  {
    is_enable_automatic_connection_flood_ip_ban = new_value;
  }

  size_t get_minimum_number_of_connections_from_same_ip_for_automatic_ban() const noexcept
  {
    return minimum_number_of_connections_from_same_ip_for_automatic_ban;
  }

  void set_minimum_number_of_connections_from_same_ip_for_automatic_ban(size_t new_value) noexcept
  {
    new_value = std::max<size_t>(5, new_value);
    minimum_number_of_connections_from_same_ip_for_automatic_ban = new_value;
  }

  size_t get_maximum_number_of_warnings_for_automatic_kick() const noexcept
  {
    return maximum_number_of_warnings_for_automatic_kick;
  }

  void set_maximum_number_of_warnings_for_automatic_kick(size_t new_value) noexcept
  {
    if (!(new_value >= 1 && new_value <= 5))
      new_value = 3;
    maximum_number_of_warnings_for_automatic_kick = new_value;
  }

  inline game_name_t get_game_name() const noexcept
  {
    return game_name;
  }

  inline void set_game_name(const game_name_t new_game_name) noexcept
  {
    game_name = new_game_name;
  }

  inline const string &get_username() const noexcept
  {
    return username;
  }

  inline void set_username(string new_value) noexcept
  {
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

  inline const std::string &get_server_message() const noexcept
  {
    return server_message;
  }

  inline void set_server_message(std::string new_value) noexcept
  {
    server_message = std::move(new_value);
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
    if (cmd.type == command_type::rcon) {
      process_rcon_command(cmd.command, cmd.is_wait_for_reply);
    } else if (cmd.type == command_type::user) {
      process_user_command(cmd.command);
    }
  }
};
