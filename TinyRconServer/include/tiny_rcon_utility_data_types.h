#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// #include "stl_helper_functions.hpp"
#include "tiny_rcon_client_user.h"

struct tiny_rcon_handles
{
  HINSTANCE hInstance;
  HWND hwnd_main_window;
  HWND hwnd_users_table;
  HWND hwnd_online_admins_information;
  HWND hwnd_re_messages_data;
  HWND hwnd_e_user_input;
  HWND hwnd_confirmation_dialog;
  HWND hwnd_yes_button;
  HWND hwnd_no_button;
  HWND hwnd_re_confirmation_message;
  HWND hwnd_e_reason;
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
  explicit message_t(std::string message_command, std::string message_data, const std::shared_ptr<tiny_rcon_client_user> &s, const bool is_show_in_messages = true) : command{ std::move(message_command) }, data{ std::move(message_data) }, sender{ s }, is_show_in_messages{ is_show_in_messages } {}
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

struct row_of_player_data_to_display
{
  char pid[6]{};
  char score[8]{};
  char ping[8]{};
  char player_name[33]{};
  char ip_address[20]{};
  char geo_info[128]{};
  const char *country_code{};
};
