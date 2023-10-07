#pragma once

#include <string>
#include <utility>
#include <vector>
#include <CommCtrl.h>
#include <Richedit.h>
#include <regex>
#include <set>
#include "stl_helper_functions.hpp"

class tiny_cod2_rcon_client_application;

struct tiny_rcon_handles
{
  HINSTANCE hInstance;
  HWND hwnd_main_window;
  HWND hwnd_players_grid;
  HWND hwnd_match_information;
  HWND hwnd_re_messages_data;
  HWND hwnd_re_help_data;
  HWND hwnd_e_user_input;
  HWND hwnd_progress_bar;
  HWND hwnd_combo_box_map;
  HWND hwnd_combo_box_gametype;
  HWND hwnd_combo_box_sortmode;
  HWND hwnd_button_load;
  HWND hwnd_button_warn;
  HWND hwnd_button_kick;
  HWND hwnd_button_tempban;
  HWND hwnd_button_ipban;
  HWND hwnd_button_view_tempbans;
  HWND hwnd_button_view_ipbans;
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
  HWND hwnd_refresh_time_period;
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

struct player_data;

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

    size_t no_of_chars_to_copy = stl::helper::len(code);
    if (no_of_chars_to_copy > 3)
      no_of_chars_to_copy = 3;
    memcpy(country_code, code, no_of_chars_to_copy);
    country_code[no_of_chars_to_copy] = 0;

    no_of_chars_to_copy = stl::helper::len(country);
    if (no_of_chars_to_copy > 34)
      no_of_chars_to_copy = 34;
    memcpy(country_name, country, no_of_chars_to_copy);
    country_name[no_of_chars_to_copy] = 0;

    no_of_chars_to_copy = stl::helper::len(reg);
    if (no_of_chars_to_copy > 34)
      no_of_chars_to_copy = 34;
    memcpy(region, reg, no_of_chars_to_copy);
    region[no_of_chars_to_copy] = 0;

    no_of_chars_to_copy = stl::helper::len(ci);
    if (no_of_chars_to_copy > 34)
      no_of_chars_to_copy = 34;
    memcpy(city, ci, no_of_chars_to_copy);
    city[no_of_chars_to_copy] = 0;
  }

  constexpr const char *get_country_code() const noexcept
  {
    return country_code;
  }

  constexpr const char *get_country_name() const noexcept
  {
    return country_name;
  }
  constexpr const char *get_region() const noexcept
  {
    return region;
  }

  constexpr const char *get_city() const noexcept
  {
    return city;
  }
};

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

bool create_necessary_folders_and_files(const std::vector<std::string> &folder_file_paths);
void set_rich_edit_control_colors(HWND richEditCtrl, const COLORREF fg_color, const COLORREF bg_color = color::black, const char *font_face_name = "Consolas") noexcept;
CHARFORMAT get_char_fmt(HWND hwnd, DWORD range = SCF_SELECTION) noexcept;
void set_char_fmt(HWND hwnd, const CHARFORMAT2 &cf, DWORD range = SCF_SELECTION) noexcept;
void replace_sel(HWND hwnd, const char *str) noexcept;
void cursor_to_bottom(HWND hwnd) noexcept;
void scroll_to_beginning(HWND hwnd) noexcept;
void scroll_to(HWND hwnd, DWORD pos) noexcept;
void scroll_to_bottom(HWND hwnd) noexcept;
void append(HWND hwnd, const char *str) noexcept;

void show_error(HWND parent_window, const char *, const size_t) noexcept;
size_t get_number_of_lines_in_file(const char *file_path);
bool parse_geodata_lite_csv_file(const char *);

bool write_tiny_rcon_json_settings_to_file(const char *) noexcept;

bool check_ip_address_validity(std::string_view, unsigned long&);

void convert_guid_key_to_country_name(const std::vector<geoip_data> &geo_data,
  std::string_view player_ip,
  player_data &player_data);

size_t find_longest_player_name_length(
  const std::vector<player_data> &,
  const bool,
  const size_t number_of_players_to_process) noexcept;
size_t find_longest_player_country_city_info_length(
  const std::vector<player_data> &,
  const size_t number_of_players_to_process) noexcept;

void parse_tinyrcon_tool_config_file(const char *);

void parse_tempbans_data_file();

void parse_banned_ip_addresses_file();

bool temp_ban_player_ip_address(player_data &player_data);

bool global_ban_player_ip_address(player_data &player_data);

bool remove_temp_banned_ip_address(const std::string &ip_address, std::string &message, const bool is_automatic_temp_ban_remove = true);

bool remove_permanently_banned_ip_address(const std::string &ip_address, std::string &message);

size_t print_colored_text(HWND re_control, const char *text, const is_append_message_to_richedit_control = is_append_message_to_richedit_control::yes, const is_log_message = is_log_message::yes, const is_log_datetime = is_log_datetime::yes);

size_t print_colored_text_to_grid_cell(HDC hdc, RECT &rect, const char *text, DWORD formatting_style);

size_t get_number_of_characters_without_color_codes(const char *) noexcept;

bool get_user_input();

void print_help_information(const std::vector<std::string> &);

std::string prepare_current_match_information();
bool is_valid_decimal_whole_number(const std::string &number_str, int &number) noexcept;

bool check_if_user_provided_argument_is_valid_for_specified_command(
  const char *cmd,
  const std::string &arg);

bool check_if_user_provided_pid_is_valid(const std::string &) noexcept;

void remove_all_color_codes(char *msg);
void remove_all_color_codes(std::string &);

void check_for_warned_players();

void check_for_temp_banned_ip_addresses();

void check_for_banned_ip_addresses();

std::pair<player_data, bool> get_online_player_for_specified_pid(const int);

void kick_player(const int, std::string &);

void tempban_player(player_data &, std::string &);

void ban_player(const int, std::string &);

void process_user_input(std::string &);

void process_user_command(const std::vector<std::string> &);

void process_rcon_command(const std::vector<std::string> &, const bool);

volatile bool should_program_terminate(const std::string & = "") noexcept;

void sort_players_data(std::vector<player_data> &, const sort_type sort_method);

void display_permanently_banned_ip_addresses();

void display_temporarily_banned_ip_addresses();

const std::string &get_full_gametype_name(const std::string &);

const std::string &get_full_map_name(const std::string &);

void display_all_available_maps();

void import_geoip_data(std::vector<geoip_data> &, const char *);

void export_geoip_data(const std::vector<geoip_data> &, const char *) noexcept;

void change_colors();

void strip_leading_and_trailing_quotes(std::string &) noexcept;

void replace_all_escaped_new_lines_with_new_lines(std::string &) noexcept;

bool change_server_setting(const std::vector<std::string> &) noexcept;

void log_message(const std::string &, const is_log_datetime = is_log_datetime::yes);

std::string get_player_name_for_pid(const int);

player_data &get_player_data_for_pid(const int);

std::string get_player_information(const int, const bool is_every_property_on_new_line = false);

std::string get_player_information_for_player(player_data &);

bool specify_reason_for_player_pid(const int, const std::string &);

void build_tiny_rcon_message(std::string &);

void say_message(const char *);
void rcon_say(std::string &, const bool = true);
void tell_message(const char *, const int);
std::string word_wrap(const char *, const size_t);

std::string get_time_interval_info_string_for_seconds(const time_t seconds);

std::string get_date_and_time_for_time_t(const time_t);

void change_game_type(const std::string &game_type, const bool = false) noexcept;
void load_map(const std::string &, const std::string &, const bool = true) noexcept;

template<typename... T>
void unused(T &&...) noexcept {}

void say_slow(HWND control, const char *msg, size_t const len) noexcept;

template<typename... Args>
void say(HWND control, const char *szoveg, Args... args) noexcept
{
  static char outbuffer[8196];

  if (-1 == snprintf(outbuffer, std::size(outbuffer), szoveg, args...)) return;
  print_colored_text(control, outbuffer, is_append_message_to_richedit_control::yes);
}

template<typename... Args>
void csay(HWND control, const char *szoveg, Args... args) noexcept
{
  static char outbuffer[8196];

  if (-1 == snprintf(outbuffer, std::size(outbuffer), szoveg, args...)) return;

  print_colored_text(control, outbuffer, is_append_message_to_richedit_control::yes);
}

bool remove_dir_path_sep_char(char *) noexcept;
void replace_forward_slash_with_backward_slash(std::string &);

const char *find_call_of_duty_1_installation_path(const bool is_show_browse_folder_dialog = true) noexcept;

bool check_if_call_of_duty_1_game_is_running() noexcept;

const char *find_call_of_duty_2_installation_path(const bool is_show_browse_folder_dialog = true) noexcept;

bool check_if_call_of_duty_2_game_is_running() noexcept;

const char *find_call_of_duty_4_installation_path(const bool is_show_browse_folder_dialog = true) noexcept;

bool check_if_call_of_duty_4_game_is_running() noexcept;

const char *find_call_of_duty_5_installation_path(const bool is_show_browse_folder_dialog = true) noexcept;

bool check_if_call_of_duty_5_game_is_running() noexcept;

const char *BrowseFolder(const char *, const char *) noexcept;

bool connect_to_the_game_server(const std::string &, const game_name_t, const bool, const bool = true);

bool check_if_file_path_exists(const char *) noexcept;
bool check_if_file_path_exists(const char *) noexcept;

bool delete_temporary_game_file() noexcept;

bool get_confirmation_message_from_user(const char *, const char *) noexcept;
bool check_if_user_wants_to_quit(const char *) noexcept;
// void process_user_input() noexcept;
void process_key_down_message(const MSG &);

void display_tempbanned_players_remaining_time_period();

void construct_tinyrcon_gui(HWND) noexcept;
void initialize_players_grid(HWND hgrid, const size_t cols, const size_t rows, const bool is_for_rcon_status = true) noexcept;
void display_players_data_in_players_grid(HWND playersGrid) noexcept;
void clear_players_data_in_players_grid(HWND playersGrid, const size_t start_row, const size_t last_row, const size_t cols) noexcept;
void PutCell(HWND, const int, const int, const char *) noexcept;
void display_country_flag(HWND, const int, const int, const char *) noexcept;
std::string GetCellContents(HWND, const int row, const int col);

bool is_alpha(const char ch) noexcept;
bool is_decimal_digit(const char ch) noexcept;
bool is_ws(const char ch) noexcept;

void change_hdc_fg_color(HDC hdc, COLORREF fg_color) noexcept;
bool check_if_selected_cell_indices_are_valid(const int row_index, const int col_index) noexcept;
void CenterWindow(HWND hwnd) noexcept;
bool show_user_confirmation_dialog(const char *msg, const char *title, const char *edit_label_text = "Reason:");

void process_sort_type_change_request(const sort_type) noexcept;

void update_game_server_setting(std::string, std::string);

std::pair<bool, game_name_t> check_if_specified_server_ip_port_and_rcon_password_are_valid(const char *ip_address, const uint_least16_t port, const char *rcon_password);

bool show_and_process_tinyrcon_configuration_panel(const char *title);
void process_button_save_changes_click_event(HWND);
void process_button_test_connection_click_event(HWND);
extern LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR);
void display_context_menu_over_grid(const int mouse_x, const int mouse_y, const int selected_row);
inline std::pair<const char *, const char *> get_appropriate_rcon_status_response_header(const game_name_t game_name) noexcept
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

const std::map<std::string, std::string> &get_rcon_map_names_to_full_map_names_for_specified_game_name(const game_name_t) noexcept;

const std::map<std::string, std::string> &get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(const game_name_t) noexcept;

const std::map<std::string, std::string> &get_full_map_names_to_rcon_map_names_for_specified_game_name(const game_name_t) noexcept;

bool initialize_and_verify_server_connection_settings();

void initiate_sending_rcon_status_command_now();

void prepare_players_data_for_display(const bool is_log_status_table = false);
void prepare_players_data_for_display_of_getstatus_response(const bool is_log_status_table = false);

size_t get_file_size_in_bytes(const char *) noexcept;
std::string get_current_date_time_str(const char *date_time_format_str);

void correct_truncated_player_names(const char *ip_address, const uint_least16_t port_number, const char *rcon_password);
void print_message_about_corrected_player_name(HWND re_hwnd, const char *truncated_name, const char *corrected_name) noexcept;
void set_admin_actions_buttons_active(const BOOL is_enable = TRUE, const bool is_reset_to_default_sort_mode = true) noexcept;

void set_available_sort_methods(const bool is_admin = true, const bool is_reset_to_default_sort_mode = true);
std::pair<bool, std::string> extract_7z_file_to_specified_path(const wchar_t *compressed_7z_file_path, const wchar_t *destination_path);

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