#pragma once

#include <string>
#include <utility>
#include <vector>
#include <CommCtrl.h>
#include <Richedit.h>
#include <regex>
#include <set>
#include <connection_manager_for_messages.h>

#undef max

class tiny_rcon_server_application;

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

bool create_necessary_folders_and_files(const std::vector<std::string> &folder_file_paths);
void set_rich_edit_control_colors(HWND richEditCtrl, const COLORREF fg_color, const COLORREF bg_color = color::black, const char *font_face_name = "Consolas");
CHARFORMAT get_char_fmt(HWND hwnd, DWORD range = SCF_SELECTION);
void set_char_fmt(HWND hwnd, const CHARFORMAT2 &cf, DWORD range = SCF_SELECTION);
void replace_sel(HWND hwnd, const char *str);
void cursor_to_bottom(HWND hwnd);
void scroll_to_beginning(HWND hwnd);
void scroll_to(HWND hwnd, DWORD pos);
void scroll_to_bottom(HWND hwnd);
void append(HWND hwnd, const char *str, const bool is_prevent_auto_vertical_scrolling = false);

void show_error(HWND parent_window, const char *, const size_t);
size_t get_number_of_lines_in_file(const char *file_path);
bool parse_geodata_lite_csv_file(const char *);

bool write_tiny_rcon_json_settings_to_file(const char *);
bool save_tiny_rcon_users_data_to_json_file(const char *);

bool check_ip_address_validity(std::string_view, unsigned long &);

void convert_guid_key_to_country_name(const std::vector<geoip_data> &geo_data,
  std::string_view player_ip,
  player &player_data);

size_t get_number_of_characters_without_color_codes(const char *) noexcept;

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

size_t find_longest_player_name_length(
  const std::vector<player> &,
  const bool,
  const size_t number_of_players_to_process);
size_t find_longest_player_country_city_info_length(
  const std::vector<player> &,
  const size_t number_of_players_to_process);

void parse_tinyrcon_tool_config_file(const char *);
void parse_tinyrcon_server_users_data(const char *);

void parse_tempbans_data_file(const char *file_path, std::vector<player> &temp_banned_players, std::unordered_map<std::string, player> &ip_to_temp_banned_player, const bool is_skip_removed_check = false);

void parse_banned_ip_addresses_file(const char *file_path, std::vector<player> &banned_players, std::unordered_map<std::string, player> &ip_to_banned_player, const bool is_skip_removed_check = false);

void parse_banned_ip_address_ranges_file(const char *file_path, std::vector<player> &banned_ip_address_ranges, std::unordered_map<std::string, player> &ip_address_range_to_banned_player, const bool is_skip_removed_check = false);

void parse_banned_cities_file(const char *file_path, std::set<std::string> &banned_cities, const bool is_skip_removed_check = false);

void parse_banned_countries_file(const char *file_path, std::set<std::string> &banned_countries, const bool is_skip_removed_check = false);

void save_banned_ip_entries_to_file(const char *file_path, const std::vector<player> &banned_ip_entries);
void save_banned_ip_address_range_entries_to_file(const char *file_path, const std::vector<player> &banned_ip_address_ranges);
void save_banned_cities_to_file(const char *file_path, const std::set<std::string> &banned_cities);
void save_banned_countries_to_file(const char *file_path, const std::set<std::string> &banned_countries);
bool temp_ban_player_ip_address(player &player_data);
bool global_ban_player_ip_address(player &player_data);
bool add_temporarily_banned_ip_address(player &pd, std::vector<player> &temp_banned_players_data, std::unordered_map<std::string, player> &ip_to_temp_banned_player_data);
bool add_permanently_banned_ip_address(player &pd, std::vector<player> &banned_players_data, std::unordered_map<std::string, player> &ip_to_banned_player_data);
bool add_permanently_banned_ip_address_range(player &pd, std::vector<player> &banned_ip_address_ranges_vector, std::unordered_map<std::string, player> &banned_ip_address_ranges_map);
bool remove_permanently_banned_ip_address_range(player &pd, std::vector<player> &banned_ip_address_ranges_vector, std::unordered_map<std::string, player> &banned_ip_address_ranges_map);
bool add_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities);
bool add_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries);
bool remove_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities);
bool remove_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries);
std::pair<bool, std::string> remove_temp_banned_ip_address(const std::string &ip_address);
std::pair<bool, std::string> remove_permanently_banned_ip_address(std::string &ip_address);

size_t print_colored_text(HWND re_control, const char *text, const is_append_message_to_richedit_control = is_append_message_to_richedit_control::yes, const is_log_message = is_log_message::yes, const is_log_datetime = is_log_datetime::yes, const bool is_prevent_auto_vertical_scrolling = false, const bool is_remove_color_codes_for_log_message = true);

size_t print_colored_text_to_grid_cell(HDC hdc, RECT &rect, const char *text, DWORD formatting_style);

bool get_user_input();

void print_help_information(const std::vector<std::string> &);

std::string prepare_current_match_information();
bool is_valid_decimal_whole_number(const std::string &str, int &number);

bool check_if_user_provided_argument_is_valid_for_specified_command(
  const char *cmd,
  const std::string &arg);

bool check_if_user_provided_pid_is_valid(const std::string &);

void remove_all_color_codes(char *msg);
void remove_all_color_codes(std::string &);

void process_user_input(std::string &);

void process_user_command(const std::vector<std::string> &);

volatile bool should_program_terminate(const std::string & = "");

void display_permanently_banned_ip_addresses(const bool is_save_data_to_log_file = false);

void display_temporarily_banned_ip_addresses(const bool is_save_data_to_log_file = false);

void import_geoip_data(std::vector<geoip_data> &, const char *);

void export_geoip_data(const std::vector<geoip_data> &, const char *);

void change_colors();

void strip_leading_and_trailing_quotes(std::string &);

void replace_all_escaped_new_lines_with_new_lines(std::string &);

bool change_server_setting(const std::vector<std::string> &);

void log_message(const std::string &, const is_log_datetime = is_log_datetime::yes);

std::string get_player_information_for_player(player &);

bool specify_reason_for_player_pid(const int, const std::string &);

std::string word_wrap(const char *, const size_t);

std::string get_time_interval_info_string_for_seconds(const time_t seconds);

void change_game_type(const std::string &game_type, const bool = false);
void load_map(const std::string &, const std::string &, const bool = true);

template<typename... T>
void unused(T &&...) {}

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
void replace_forward_slash_with_backward_slash(std::string &);

const char *find_call_of_duty_1_installation_path(const bool is_show_browse_folder_dialog = true);

bool check_if_call_of_duty_1_game_is_running();

const char *find_call_of_duty_2_installation_path(const bool is_show_browse_folder_dialog = true);

bool check_if_call_of_duty_2_game_is_running();

const char *find_call_of_duty_4_installation_path(const bool is_show_browse_folder_dialog = true);

bool check_if_call_of_duty_4_game_is_running();

const char *find_call_of_duty_5_installation_path(const bool is_show_browse_folder_dialog = true);

bool check_if_call_of_duty_5_game_is_running();

const char *BrowseFolder(const char *, const char *);

bool connect_to_the_game_server(const std::string &, const game_name_t, const bool, const bool = true);

bool check_if_file_path_exists(const char *);
bool check_if_file_path_exists(const char *);

bool delete_temporary_game_file();

bool get_confirmation_message_from_user(const char *, const char *);
bool check_if_user_wants_to_quit(const char *);
// void process_user_input() ;
void process_key_down_message(const MSG &);

void construct_tinyrcon_gui(HWND);
void initialize_users_grid(HWND hgrid, const size_t cols, const size_t rows, const bool is_for_rcon_status = true);
void display_users_data_in_users_table(HWND playersGrid);
void clear_data_in_users_table(HWND playersGrid, const size_t start_row, const size_t last_row, const size_t cols);
void PutCell(HWND, const int, const int, const char *);
void set_check_box(HWND hgrid, const int row, const int col, const BOOL is_checked);
void display_country_flag(HWND, const int, const int, const char *);
std::string GetCellContents(HWND, const int row, const int col);

bool is_alpha(const char ch);
bool is_decimal_digit(const char ch);
bool is_ws(const char ch);

void change_hdc_fg_color(HDC hdc, COLORREF fg_color);
bool check_if_selected_cell_indices_are_valid(const int row_index, const int col_index);
void CenterWindow(HWND hwnd);
bool show_user_confirmation_dialog(const char *msg, const char *title, const char *edit_label_text = "Reason:");

extern LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR);
void display_context_menu_over_grid(HWND control, const int mouse_x, const int mouse_y);

size_t get_file_size_in_bytes(const char *);
std::string get_date_and_time_for_time_t(const char *date_time_format_str, time_t t_c = 0);
const char *get_current_short_month_name(const size_t index);

std::pair<bool, std::string> create_7z_file_file_at_specified_path(const std::vector<std::string> &files_to_add, const std::string &local_file_path);
std::pair<bool, std::string> extract_7z_file_to_specified_path(const char *compressed_7z_file_path, const char *destination_path);

void display_banned_cities(const std::set<std::string> &banned_cities);
void display_banned_countries(const std::set<std::string> &banned_countries);
void save_banned_entries_to_file(const char *file_path, const std::set<std::string> &banned_entries);
void save_tempbans_to_file(const char *file_path, const std::vector<player> &temp_banned_players);

template<typename ContainerType, typename ElementValue>
void initialize_elements_of_container_to_specified_value(ContainerType &data, const ElementValue &value, const size_t start_index = 0)
{
  for (size_t i{ start_index }; i < data.size(); ++i) {
    data[i] = value;
  }
}

time_t get_current_time_stamp();

time_t get_number_of_seconds_from_date_and_time_string(const std::string &date_and_time);
void display_online_admins_information();
void removed_disallowed_character_in_string(std::string &input);
std::string remove_disallowed_character_in_string(const std::string &);
std::string get_cleaned_user_name(const std::string &name);
void replace_br_with_new_line(std::string &message);
void parse_protected_entries_file(const char *file_path, std::set<std::string> &protected_ip_addresses);
void save_protected_entries_file(const char *file_path, const std::set<std::string> &protected_entries);
void display_protected_entries(const char *table_title, const std::set<std::string> &protected_entries);
void get_first_valid_ip_address_from_ip_address_range(std::string ip_range, player &pd);
std::string get_narrow_ip_address_range_for_specified_ip_address(const std::string &ip_address);
std::string get_wide_ip_address_range_for_specified_ip_address(const std::string &ip_address);
bool check_if_player_is_protected(const player &online_player, const char *admin_command, std::string &message);
size_t get_random_number();
size_t find_longest_user_name_length(
  const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users,
  const bool count_color_codes,
  const size_t number_of_users_to_process) noexcept;

size_t find_longest_user_country_city_info_length(
  const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users,
  const size_t number_of_users_to_process) noexcept;
void display_admins_data();
void display_banned_ip_address_ranges(const bool is_save_data_to_log_file = false);