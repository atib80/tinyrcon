#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <type_traits>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "stl_helper_functions.hpp"
#include "tiny_rcon_client_user.h"

struct tiny_rcon_handles
{
  HINSTANCE hInstance;
  HWND hwnd_main_window;
  HWND hwnd_players_grid;
  HWND hwnd_servers_grid;
  HWND hwnd_online_admins_information;
  HWND hwnd_match_information;
  HWND hwnd_re_messages_data;
  HWND hwnd_re_help_data;
  HWND hwnd_e_user_input;
  HWND hwnd_progress_bar;
  HWND hwnd_combo_box_map;
  HWND hwnd_combo_box_gametype;
  HWND hwnd_combo_box_sortmode;
  HWND hwnd_button_players_view;
  HWND hwnd_button_game_servers_list_view;
  HWND hwnd_button_refresh_game_servers;
  HWND hwnd_button_load;
  HWND hwnd_button_warn;
  HWND hwnd_button_kick;
  HWND hwnd_button_tempban;
  HWND hwnd_button_ipban;
  HWND hwnd_button_view_tempbans;
  HWND hwnd_button_view_ipbans;
  HWND hwnd_button_view_adminsdata;
  HWND hwnd_button_view_rcon;
  HWND hwnd_refresh_players_data_button;
  HWND hwnd_connect_button;
  HWND hwnd_connect_private_slot_button;
  HWND hwnd_say_button;
  HWND hwnd_tell_button;
  HWND hwnd_quit_button;
  HWND hwnd_confirmation_dialog;
  HWND hwnd_yes_button;
  HWND hwnd_no_button;
  HWND hwnd_re_confirmation_message;
  HWND hwnd_e_reason;
  HWND hwnd_configuration_dialog;
  HWND hwnd_user_name;
  HWND hwnd_server_name;
  HWND hwnd_server_ip_address;
  HWND hwnd_server_port;
  HWND hwnd_rcon_password;
  HWND hwnd_enable_city_ban;
  HWND hwnd_enable_country_ban;
  HWND hwnd_save_settings_button;
  HWND hwnd_test_connection_button;
  HWND hwnd_close_button;
  HWND hwnd_exit_tinyrcon_button;
  HWND hwnd_configure_server_settings_button;
  HWND hwnd_clear_messages_button;
  HWND hwnd_cod1_path_edit;
  HWND hwnd_cod2_path_edit;
  HWND hwnd_cod4_path_edit;
  HWND hwnd_cod5_path_edit;
  HWND hwnd_cod1_path_button;
  HWND hwnd_cod2_path_button;
  HWND hwnd_cod4_path_button;
  HWND hwnd_cod5_path_button;
  HWND hwnd_download_speed_info;
  HWND hwnd_upload_speed_info;
};

enum class is_append_message_to_richedit_control {
  no,
  yes
};

enum class is_log_message {
  no,
  yes
};

enum class is_log_datetime {
  no,
  yes
};

enum class game_name_t {
  unknown,
  cod1,
  cod2,
  cod4,
  cod5
};

enum class command_type { rcon,
  user };

struct command_t
{
  command_t(std::vector<std::string> cmd, const command_type cmd_type, const bool wait_for_reply) : command{ std::move(cmd) }, type{ cmd_type }, is_wait_for_reply{ wait_for_reply } {}
  std::vector<std::string> command;
  command_type type;
  bool is_wait_for_reply{};
};

enum class message_type_t { send,
  receive };

struct message_t
{
  explicit message_t(std::string message_command, std::string message_data, const bool is_show_in_messages = true) : command{ std::move(message_command) }, data{ std::move(message_data) }, is_show_in_messages{ is_show_in_messages } {}
  std::string command;
  std::string data;
  const bool is_show_in_messages;
};

struct server_message_t
{
  explicit server_message_t(std::string message_command, std::string message_data, const std::shared_ptr<tiny_rcon_client_user> &s, const bool is_show_in_messages = true) : command{ std::move(message_command) }, data{ std::move(message_data) }, sender{ s }, is_show_in_messages{ is_show_in_messages } {}
  std::string command;
  std::string data;
  std::shared_ptr<tiny_rcon_client_user> sender;
  const bool is_show_in_messages;
};

enum class sort_type {
  unknown,
  pid_desc,
  pid_asc,
  score_desc,
  score_asc,
  ping_desc,
  ping_asc,
  name_desc,
  name_asc,
  ip_desc,
  ip_asc,
  geo_desc,
  geo_asc
};

enum class color_type { black,
  red,
  green,
  yellow,
  blue,
  cyan,
  magenta,
  white,
};

namespace color {
static COLORREF black{ RGB(0, 0, 0) };
static COLORREF blue{ RGB(0, 0, 255) };
static COLORREF cyan{ RGB(0, 255, 255) };
static COLORREF green{ RGB(0, 255, 0) };
static COLORREF grey{ RGB(128, 128, 128) };
static COLORREF light_blue{ RGB(173, 216, 230) };
static COLORREF magenta{ RGB(255, 0, 255) };
static COLORREF maroon{ RGB(128, 0, 0) };
static COLORREF purple{ RGB(128, 0, 128) };
static COLORREF red{ RGB(255, 0, 0) };
static COLORREF teal{ RGB(0, 128, 128) };
static COLORREF yellow{ RGB(255, 255, 0) };
static COLORREF white{ RGB(255, 255, 255) };
}// namespace color

struct player;

struct geoip_data
{
  unsigned long lower_ip_bound;
  unsigned long upper_ip_bound;
  char country_code[4];
  char country_name[35];
  char region[35];
  char city[35];

  geoip_data() = default;

  geoip_data(const unsigned long lib, const unsigned long uib, const char *code, const char *country, const char *reg, const char *ci) : lower_ip_bound{ lib }, upper_ip_bound{ uib }
  {

    const size_t no_of_chars_for_country_code = std::min<size_t>(3U, stl::helper::len(code));
    memcpy(country_code, code, no_of_chars_for_country_code);
    country_code[no_of_chars_for_country_code] = 0;

    const size_t no_of_chars_for_country_name = std::min<size_t>(34U, stl::helper::len(country));
    memcpy(country_name, country, no_of_chars_for_country_name);
    country_name[no_of_chars_for_country_name] = 0;

    const size_t no_of_chars_for_region = std::min<size_t>(34U, stl::helper::len(reg));
    memcpy(region, reg, no_of_chars_for_region);
    region[no_of_chars_for_region] = 0;

    const size_t no_of_chars_for_city = std::min<size_t>(34U, stl::helper::len(ci));
    memcpy(city, ci, no_of_chars_for_city);
    city[no_of_chars_for_city] = 0;
  }

  constexpr const char *get_country_code() const
  {
    return country_code;
  }

  constexpr const char *get_country_name() const
  {
    return country_name;
  }
  constexpr const char *get_region() const
  {
    return region;
  }

  constexpr const char *get_city() const
  {
    return city;
  }
};

struct row_of_player_data_to_display
{
  char pid[6]{};
  char score[8]{};
  char ping[8]{};
  char player_name[36]{};
  char ip_address[20]{};
  char geo_info[128]{ "Unknown, Unknown" };
  const char *country_code{ "xy" };
};

template<typename T>
concept string_convertible = requires(const T &value) {
  {
    (to_string(value) || value.to_string())
  } -> std::convertible_to<std::string>;
};