#pragma once

#include <string>
#include <utility>
#include <vector>
#include <CommCtrl.h>
#include <Richedit.h>
#include <regex>
#include <set>
#include "stl_helper_functions.hpp"
#include "tiny_rcon_client_user.h"

#undef max

class tiny_rcon_client_application;

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
  HWND hwnd_mute_player_button;
  HWND hwnd_unmute_player_button;
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

struct print_message_t
{
  explicit print_message_t(std::string message, is_log_message log_to_file, is_log_datetime is_log_current_date_time, bool is_remove_color_codes_for_log_message) : message_{ std::move(message) }, log_to_file_{ log_to_file }, is_log_current_date_time_{ is_log_current_date_time }, is_remove_color_codes_for_log_message_{ is_remove_color_codes_for_log_message } {}
  // HWND& control_;
  std::string message_;
  is_log_message log_to_file_;
  is_log_datetime is_log_current_date_time_;
  bool is_remove_color_codes_for_log_message_;
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

std::pair<bool, std::string> create_necessary_file_path(const std::string &file_);
std::pair<bool, std::string> create_necessary_folders_and_files(const std::vector<std::string> &folder_file_paths);
void set_rich_edit_control_colors(HWND richEditCtrl, const COLORREF fg_color, const COLORREF bg_color = color::black, const char *font_face_name = "Consolas");
CHARFORMAT get_char_fmt(HWND hwnd, DWORD range = SCF_SELECTION);
void set_char_fmt(HWND hwnd, const CHARFORMAT2 &cf, DWORD range = SCF_SELECTION);
void replace_sel(HWND hwnd, const char *str);
void cursor_to_bottom(HWND hwnd);
void scroll_to_beginning(HWND hwnd);
void scroll_to(HWND hwnd, DWORD pos);
void scroll_to_bottom(HWND hwnd);
void append_to_title(HWND window, std::string text, const char *animation_sequence_chars = "-\\|/");

void show_error(HWND parent_window, const char *, const size_t);
size_t get_number_of_lines_in_file(const char *file_path);
bool parse_geodata_lite_csv_file(const char *);

bool write_tiny_rcon_json_settings_to_file(const char *);

bool check_ip_address_validity(std::string_view, unsigned long &);
bool check_ip_address_range_validity(const std::string &ip_address_range);

void convert_guid_key_to_country_name(const std::vector<geoip_data> &geo_data,
  std::string_view player_ip,
  player &player_data);

size_t get_number_of_characters_without_color_codes(const char *);

template<typename Iter>
size_t find_longest_entry_length(
  Iter first,
  Iter last,
  const bool count_color_codes)
{
  if (first == last)
    return 0;
  size_t max_player_name_length{ 32 };
  while (first != last) {
    max_player_name_length =
      std::max<size_t>(count_color_codes ? first->length() : get_number_of_characters_without_color_codes(first->c_str()), max_player_name_length);
    ++first;
  }

  return max_player_name_length;
}

template<typename Iter>
size_t find_longest_player_name_length(
  Iter first,
  const Iter last,
  const bool count_color_codes)
{
  if (first == last)
    return 0;
  size_t max_player_name_length{ 8 };
  while (first != last) {
    max_player_name_length =
      std::max<size_t>(count_color_codes ? stl::helper::len(first->player_name) : get_number_of_characters_without_color_codes(first->player_name), max_player_name_length);
    ++first;
  }

  return max_player_name_length;
}

size_t find_longest_player_country_city_info_length(
  const std::vector<player> &,
  const size_t number_of_players_to_process);

size_t find_longest_user_name_length(
  const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users,
  const bool count_color_codes,
  const size_t number_of_users_to_process);

size_t find_longest_user_country_city_info_length(
  const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users,
  const size_t number_of_users_to_process);

void parse_tinyrcon_tool_config_file(const char *);

void load_tinyrcon_client_user_data(const char *);

// void parse_tempbans_data_file(const char *file_path, std::vector<player> &temp_banned_players, std::unordered_map<std::string, player> &ip_to_temp_banned_player);
// void parse_banned_ip_addresses_file(const char *file_path, std::vector<player> &banned_players, std::unordered_map<std::string, player> &ip_to_banned_player);
// void parse_banned_ip_address_ranges_file(const char *file_path, std::vector<player> &banned_ip_address_ranges, std::unordered_map<std::string, player> &ip_address_range_to_banned_player);
// void parse_banned_cities_file(const char *file_path, std::set<std::string> &banned_cities);
// void parse_banned_countries_file(const char *file_path, std::set<std::string> &banned_countries);
//
// void save_banned_ip_entries_to_file(const char *file_path, const std::vector<player> &banned_ip_entries);
// void save_banned_ip_address_range_entries_to_file(const char *file_path, const std::vector<player> &banned_ip_address_ranges);
// void save_banned_cities_to_file(const char *file_path, const std::set<std::string> &banned_cities);
// void save_banned_countries_to_file(const char *file_path, const std::set<std::string> &banned_countries);

// bool temp_ban_player_ip_address(player &player_data);
// bool global_ban_player_ip_address(player &player_data);

// bool add_temporarily_banned_ip_address(player &pd, std::vector<player> &temp_banned_players_data, std::unordered_map<std::string, player> &ip_to_temp_banned_player_data);
// bool add_permanently_banned_ip_address(player &pd, std::vector<player> &banned_players_data, std::unordered_map<std::string, player> &ip_to_banned_player_data);
// bool add_permanently_banned_ip_address_range(player &pd, std::vector<player> &banned_ip_address_ranges_vector, std::unordered_map<std::string, player> &banned_ip_address_ranges_map);
// bool remove_permanently_banned_ip_address_range(player &pd, std::vector<player> &banned_ip_address_ranges_vector, std::unordered_map<std::string, player> &banned_ip_address_ranges_map);
// bool add_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities);
// bool add_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries);
// bool remove_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities);
// bool remove_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries);
// std::pair<bool, player> remove_temp_banned_ip_address(const std::string &ip_address, std::string &message, const bool is_automatic_temp_ban_remove = true, const bool is_report_public_message = true);
// std::pair<bool, player> remove_permanently_banned_ip_address(std::string &ip_address, std::string &message, const bool is_report_public_message = true);

size_t print_colored_text(HWND re_control, const char *text, const is_append_message_to_richedit_control = is_append_message_to_richedit_control::yes, const is_log_message = is_log_message::yes, is_log_datetime = is_log_datetime::yes, const bool is_prevent_auto_vertical_scrolling = false, const bool is_remove_color_codes_for_log_message = true);
size_t print_message(HWND re_control, const std::string &text, const is_log_message log_to_file = is_log_message::yes, is_log_datetime is_log_current_date_time = is_log_datetime::yes, const bool is_remove_color_codes_for_log_message = true);
size_t print_colored_text_to_grid_cell(HDC hdc, RECT &rect, const char *text, DWORD formatting_style);

bool get_user_input();

void print_help_information(const std::vector<std::string> &);

std::string prepare_current_match_information();
void display_online_admins_information();
bool is_valid_decimal_whole_number(const std::string &str, int &number);

bool check_if_user_provided_argument_is_valid_for_specified_command(
  const char *cmd,
  const std::string &arg);

// bool check_if_user_provided_pid_is_valid(const std::string &);

void remove_all_color_codes(char *msg);
void remove_all_color_codes(std::string &);

std::pair<player, bool> get_online_player_for_specified_pid(const int);

// void kick_player(const int, const std::string &);
// void tempban_player(player &, std::string &);
// void ban_player(const int, std::string &);

void process_user_input(std::string &);

void process_user_command(const std::vector<std::string> &);

void process_rcon_command(const std::vector<std::string> &);

volatile bool should_program_terminate(const std::string & = "");

void sort_players_data(std::vector<player> &, const sort_type sort_method);

void display_banned_ip_address_ranges(const bool is_save_data_to_log_file = false);

void display_permanently_banned_ip_addresses(const bool is_save_data_to_log_file = false);

void display_temporarily_banned_ip_addresses(const bool is_save_data_to_log_file = false);

void display_admins_data();

const std::string &get_full_gametype_name(const std::string &);

const std::string &get_full_map_name(const std::string &, const game_name_t game_name);

void display_all_available_maps();

void import_geoip_data(std::vector<geoip_data> &, const char *);

void export_geoip_data(const std::vector<geoip_data> &, const char *);

void change_colors();

void strip_leading_and_trailing_quotes(std::string &);

void replace_all_escaped_new_lines_with_new_lines(std::string &);

bool change_server_setting(const std::vector<std::string> &);

void log_message(const std::string &, const is_log_datetime = is_log_datetime::yes);

int get_selected_players_pid_number(const int selected_row_in_players_grid, const int selected_col_in_players_grid);

std::string get_player_name_for_pid(const int);

player &get_player_data_for_pid(const int);

std::string get_player_information(const int, const bool is_every_property_on_new_line, std::string_view action_by_admin_message);
std::string get_player_information_for_player(player &, std::string_view action_by_admin_message);

bool specify_reason_for_player_pid(const int, const std::string &);

void build_tiny_rcon_message(std::string &);

std::string word_wrap(const char *, const size_t);

std::string get_time_interval_info_string_for_seconds(const time_t seconds);

template<typename... T>
void unused(T &&...) {}

void say_slow(HWND control, const char *msg, size_t const len);

template<typename... Args>
void say(HWND control, const char *szoveg, Args... args)
{
  static char outbuffer[8196];

  if (-1 == snprintf(outbuffer, std::size(outbuffer), szoveg, args...)) return;
  print_colored_text(control, outbuffer, is_append_message_to_richedit_control::yes);
}

template<typename... Args>
void csay(HWND control, const char *szoveg, Args... args)
{
  static char outbuffer[8196];

  if (-1 == snprintf(outbuffer, std::size(outbuffer), szoveg, args...)) return;

  print_colored_text(control, outbuffer, is_append_message_to_richedit_control::yes);
}

bool remove_dir_path_sep_char(char *);
void replace_backward_slash_with_forward_slash(std::string &);
void replace_backward_slash_with_forward_slash(std::wstring &);
void replace_forward_slash_with_backward_slash(std::string &);
void replace_forward_slash_with_backward_slash(std::wstring &);

const char *find_call_of_duty_1_installation_path(const bool is_show_browse_folder_dialog = true);

bool check_if_call_of_duty_1_game_is_running(DWORD &pid);

const char *find_call_of_duty_2_installation_path(const bool is_show_browse_folder_dialog = true);

std::pair<bool, std::string> check_if_call_of_duty_2_game_is_running(DWORD &pid);

const char *find_call_of_duty_4_installation_path(const bool is_show_browse_folder_dialog = true);

bool check_if_call_of_duty_4_game_is_running(DWORD &pid);

const char *find_call_of_duty_5_installation_path(const bool is_show_browse_folder_dialog = true);

bool check_if_call_of_duty_5_game_is_running(DWORD &pid);

const char *BrowseFolder(const char *, const char *);

bool connect_to_the_game_server(const std::string &, const game_name_t, const bool, const bool = true);

bool check_if_file_path_exists(const char *);
bool check_if_file_path_exists(const wchar_t *);

bool delete_temporary_game_file();

bool get_confirmation_message_from_user(const char *, const char *);
bool check_if_user_wants_to_quit(const char *);
// void process_user_input() ;
void process_key_down_message(const MSG &);

// void display_tempbanned_players_remaining_time_period();

void construct_tinyrcon_gui(HWND);
void initialize_players_grid(HWND hgrid, const size_t cols, const size_t rows, const bool is_for_rcon_status = true);
void initialize_servers_grid(HWND hgrid, const size_t cols, const size_t rows);
void display_players_data_in_players_grid(HWND hgrid);
void display_game_servers_data_in_servers_grid(HWND hgrid);
void display_game_server_data_in_servers_grid(HWND hgrid, const size_t game_server_index);
void clear_players_data_in_players_grid(HWND hgrid, const size_t start_row, const size_t last_row, const size_t cols);
void clear_servers_data_in_servers_grid(HWND hgrid, const size_t start_row, const size_t last_row, const size_t cols);
void PutCell(HWND, const int, const int, const char *);
void display_country_flag(HWND, const int, const int, const char *);
std::string GetCellContents(HWND, const int row, const int col);

bool is_alpha(const char ch);
bool is_decimal_digit(const char ch);
bool is_ws(const char ch);

void change_hdc_fg_color(HDC hdc, COLORREF fg_color);
bool check_if_selected_cell_indices_are_valid_for_players_grid(const int row_index, const int col_index);
bool check_if_selected_cell_indices_are_valid_for_game_servers_grid(const int row_index, const int col_index);
void CenterWindow(HWND hwnd);
bool show_user_confirmation_dialog(const char *msg, const char *title, const char *edit_label_text = "Reason:");

void process_sort_type_change_request(const sort_type);

class game_server;

void update_game_server_setting(game_server &gs, std::string, std::string);

std::pair<bool, game_name_t> check_if_specified_server_ip_port_and_rcon_password_are_valid(game_server& gs);

bool show_and_process_tinyrcon_configuration_panel(const char *title);
void process_button_save_changes_click_event(HWND);
void process_button_test_connection_click_event(HWND);
extern LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR);
// void display_context_menu_over_grid(const int mouse_x, const int mouse_y, const int selected_row);
inline std::pair<const char *, const char *> get_appropriate_rcon_status_response_header(const game_name_t game_name)
{
  static constexpr const char *cod1_rcon_status_response_header{ "num score ping name            lastmsg address               qport rate\n" };
  static constexpr const char *cod2_rcon_status_response_header{ "num score ping guid   name            lastmsg address               qport rate\n" };
  static constexpr const char *cod4_rcon_status_response_header{ "num score ping guid                             name            lastmsg address               qport rate\n" };
  static constexpr const char *cod5_rcon_status_response_header{ "num score ping guid       name            lastmsg address               qport  rate\n" };
  static constexpr const char *cod5_rcon_status_response_header_team{ "num score ping guid       name            team lastmsg address               qport  rate\n" };

  switch (game_name) {
  case game_name_t::cod1:
    return std::make_pair(cod1_rcon_status_response_header, nullptr);
  case game_name_t::cod2:
    return std::make_pair(cod2_rcon_status_response_header, nullptr);
  case game_name_t::cod4:
    return std::make_pair(cod4_rcon_status_response_header, nullptr);
  case game_name_t::cod5:
    return std::make_pair(cod5_rcon_status_response_header, cod5_rcon_status_response_header_team);
  default:
    return std::make_pair(cod2_rcon_status_response_header, nullptr);
  }
}

// const std::regex &get_appropriate_status_regex_for_specified_game_name(const game_name_t game_name);

const std::map<std::string, std::string> &get_rcon_map_names_to_full_map_names_for_specified_game_name(const game_name_t);

const std::map<std::string, std::string> &get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(const game_name_t);

const std::map<std::string, std::string> &get_full_map_names_to_rcon_map_names_for_specified_game_name(const game_name_t);

bool initialize_and_verify_server_connection_settings();

void initiate_sending_rcon_status_command_now();

void prepare_players_data_for_display(game_server &gs, const bool is_log_status_table = false);
void prepare_players_data_for_display_of_getstatus_response(game_server &gs, const bool is_log_status_table = false);

size_t get_file_size_in_bytes(const char *);
std::string get_date_and_time_for_time_t(const char *date_time_format_str, time_t t_c = 0);
const char *get_current_short_month_name(const size_t index);

bool parse_getstatus_response_for_specified_game_server(game_server &gs);
bool parse_getinfo_response_for_specified_game_server(game_server &gs, std::string &number_of_online_players, std::string &number_of_max_players);
void correct_truncated_player_names(game_server &gs, const char *ip_address, const uint_least16_t port_number);
void print_message_about_corrected_player_name(HWND re_hwnd, const char *truncated_name, const char *corrected_name);
void set_admin_actions_buttons_active(const BOOL is_enable = TRUE, const bool is_reset_to_default_sort_mode = true);

void set_available_sort_methods(const bool is_admin = true, const bool is_reset_to_default_sort_mode = true);
std::pair<bool, std::string> extract_7z_file_to_specified_path(const char *compressed_7z_file_path, const char *destination_path);
std::pair<bool, std::string> create_7z_file_file_at_specified_path(const std::vector<std::string> &file_to_add, const std::string &local_file_path);

void display_banned_cities(const std::set<std::string> &banned_cities);
void display_banned_countries(const std::set<std::string> &banned_countries);
void save_banned_entries_to_file(const char *file_path, const std::set<std::string> &banned_entries);

template<typename ContainerType, typename ElementValue>
void initialize_elements_of_container_to_specified_value(ContainerType &data, const ElementValue &value, const size_t start_index = 0)
{
  for (size_t i{ start_index }; i < data.size(); ++i) {
    data[i] = value;
  }
}

time_t get_current_time_stamp();

time_t get_number_of_seconds_from_date_and_time_string(const std::string &date_and_time);

std::string get_narrow_ip_address_range_for_specified_ip_address(const std::string &ip_address);
std::string get_wide_ip_address_range_for_specified_ip_address(const std::string &ip_address);
void check_if_admins_are_online_and_get_admins_player_names(const std::vector<player> &players, const size_t no_of_online_players);
bool save_current_user_data_to_json_file(const char *json_file_path);
bool validate_admin_and_show_missing_admin_privileges_message(const bool is_show_message_box, const is_log_message log_message = is_log_message::no, const is_log_datetime log_date_time = is_log_datetime::no);
void removed_disallowed_character_in_string(std::string &);
std::string remove_disallowed_character_in_string(const std::string &);
size_t ltrim_specified_characters(char *src, const size_t buffer_len, const char *needle_chars);
std::string get_cleaned_user_name(const std::string &name);
void replace_br_with_new_line(std::string &message);
void parse_protected_entries_file(const char *file_path, std::set<std::string> &protected_entries);
void save_protected_entries_file(const char *file_path, const std::set<std::string> &protected_entries);
void display_protected_entries(const char *table_title, const std::set<std::string> &protected_entries, const std::unordered_map<std::string, std::string> &online_player_names, const bool is_save_data_to_log_file = false);
// bool check_if_player_is_protected(const player &online_player, const char *admin_command, std::string &message);
void get_first_valid_ip_address_from_ip_address_range(std::string ip_range, player &pd);
bool run_executable(const char *file_path_for_executable);
void restart_tinyrcon_client();
size_t get_random_number();
bool parse_game_type_information_from_rcon_reply(const std::string &rcon_reply, game_server &gs);
std::string find_version_of_installed_cod2_game();
std::string find_users_player_name_for_installed_cod2_game(const std::shared_ptr<tiny_rcon_client_user> &user);
std::string find_game_version_number_of_running_cod2mp_s_executable(DWORD pid);
// bool download_missing_cod2_game_patch_files();
// bool check_if_cod2_v1_0_game_patch_files_are_missing_and_download_them();
// bool check_if_cod2_v1_01_game_patch_files_are_missing_and_download_them();
// bool check_if_cod2_v1_2_game_patch_files_are_missing_and_download_them();
// bool check_if_cod2_v1_3_game_patch_files_are_missing_and_download_them();
void view_game_servers(HWND grid);
void refresh_game_servers_data(HWND grid);
bool parse_and_display_downloaded_game_servers_data(std::string &game_servers_data, const char *version_number, const bool is_display_parsed_game_servers_data = true);
// BOOL enable_privileged_access();
// bool apply_cod2_patch(const std::string &patch_version_number);
bool terminate_running_game_instance(const game_name_t game_name);
// std::pair<bool, std::string> backup_original_call_of_duty_2_game_files();
// std::pair<bool, std::string> check_if_users_call_of_duty_2_game_supports_patch_and_repatch_commands();
game_name_t convert_game_name_to_game_name_t(const std::string &game_name);
std::string wstring_to_string(const wchar_t *s, const char dfault = '?', const std::locale &loc = std::locale());
std::string get_server_address_for_connect_command(const int selected_server_row);
void execute_at_exit();
bool check_if_exists_and_download_missing_custom_map_files_downloader(/*const char* downloader_program_file_path*/);
HRESULT CreateLink(const wchar_t *lpszPathObj, const wchar_t *lpszPathLink, const wchar_t *lpszDesc);
const std::string &get_current_map_image_name(const std::string &current_map);
void load_current_map_image(const std::string &rcon_map_name);
std::wstring str_to_wstr(const std::string &src);
std::string wstr_to_str(const std::wstring &src);
std::vector<std::string> get_file_name_matches_for_specified_file_path_pattern(const char *dir_path, const char *file_pattern);
std::string calculate_md5_checksum_of_file(const char *file_path);
bool check_if_cod1_multiplayer_game_launch_command_is_correct(const std::string &);
bool check_if_cod2_multiplayer_game_launch_command_is_correct(const std::string &);
bool check_if_cod4_multiplayer_game_launch_command_is_correct(const std::string &);
bool check_if_cod5_multiplayer_game_launch_command_is_correct(const std::string &);
bool fix_path_strings_in_json_config_file(const std::string &config_file_path);
std::string escape_backward_slash_characters_in_place(const std::string &line);
std::string get_file_name_from_path(const std::string &file_path);
// void load_image_files_information(const char *file_path);
bool download_bitmap_image_file(const char *bitmap_image_name, const char *destination_file_path);
bool is_rcon_game_server(const game_server &gs);
std::pair<bool, int> is_check_if_tinyrcon_user_is_online_and_get_their_pid_number(const std::string &ip_address);
void spectate_player_for_specified_pid(const int pid);
std::set<int> get_all_valid_online_pids(const game_server &gs);
int get_minimum_valid_pid(const game_server &gs);
int get_maximum_valid_pid(const game_server &gs);
LRESULT CALLBACK monitor_game_key_press_events(_In_ int code, _In_ WPARAM wParam, _In_ LPARAM lParam);
std::string get_last_error_as_string();
std::string convert_guid_to_ip_address(unsigned long guid);
