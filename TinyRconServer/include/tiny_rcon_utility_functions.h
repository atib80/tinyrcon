#pragma once

// #include <CommCtrl.h>
#include <Richedit.h>
#include <regex>
#include <set>
#include "tiny_rcon_utility_data_types.h"

#undef max

bool create_necessary_folders_and_files(const std::vector<std::string>& folder_file_paths);
void set_rich_edit_control_colors(HWND richEditCtrl, const COLORREF fg_color, const COLORREF bg_color = color::black, const char* font_face_name = "Consolas");
CHARFORMAT get_char_fmt(HWND hwnd, DWORD range = SCF_SELECTION);
void set_char_fmt(HWND hwnd, const CHARFORMAT2& cf, DWORD range = SCF_SELECTION);
void replace_sel(HWND hwnd, const char* str);
void cursor_to_bottom(HWND hwnd);
void scroll_to_beginning(HWND hwnd);
void scroll_to(HWND hwnd, DWORD pos);
void scroll_to_bottom(HWND hwnd);

void show_error(HWND parent_window, const char*, const size_t);
size_t get_number_of_lines_in_file(const char* file_path);
bool parse_geodata_lite_csv_file(const char*);

bool write_tiny_rcon_json_settings_to_file(const char*);
bool save_tiny_rcon_users_data_to_json_file(const char*);

bool check_ip_address_validity(std::string_view, unsigned long&);

struct geoip_data;
void convert_guid_key_to_country_name(const std::vector<geoip_data>& geo_data,
	std::string_view player_ip,
	player& player_data);

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
	const std::vector<player>&,
	const bool,
	const size_t number_of_players_to_process);
size_t find_longest_player_country_city_info_length(
	const std::vector<player>&,
	const size_t number_of_players_to_process);

void parse_tinyrcon_tool_config_file(const char*);
void parse_tinyrcon_server_users_data(const char*);

void parse_tempbans_data_file(const char* file_path, std::vector<player>& temp_banned_players, std::unordered_map<std::string, player>& ip_to_temp_banned_player, const bool is_skip_removed_check = false);

void parse_banned_ip_addresses_file(const char* file_path, std::vector<player>& banned_players, std::unordered_map<std::string, player>& ip_to_banned_player, const bool is_skip_removed_check = false);

void parse_banned_ip_address_ranges_file(const char* file_path, std::vector<player>& banned_ip_address_ranges, std::unordered_map<std::string, player>& ip_address_range_to_banned_player, const bool is_skip_removed_check = false);

void parse_banned_cities_file(const char* file_path, std::set<std::string>& banned_cities, const bool is_skip_removed_check = false);

void parse_banned_countries_file(const char* file_path, std::set<std::string>& banned_countries, const bool is_skip_removed_check = false);

void save_banned_ip_entries_to_file(const char* file_path, const std::vector<player>& banned_ip_entries);
void save_banned_ip_address_range_entries_to_file(const char* file_path, const std::vector<player>& banned_ip_address_ranges);
void save_banned_cities_to_file(const char* file_path, const std::set<std::string>& banned_cities);
void save_banned_countries_to_file(const char* file_path, const std::set<std::string>& banned_countries);
bool temp_ban_player_ip_address(player& player_data);
bool global_ban_player_ip_address(player& player_data);
bool add_temporarily_banned_ip_address(player& pd, std::vector<player>& temp_banned_players_data, std::unordered_map<std::string, player>& ip_to_temp_banned_player_data);
bool add_permanently_banned_ip_address(player& pd, std::vector<player>& banned_players_data, std::unordered_map<std::string, player>& ip_to_banned_player_data);
bool add_permanently_banned_ip_address_range(player& pd, std::vector<player>& banned_ip_address_ranges_vector, std::unordered_map<std::string, player>& banned_ip_address_ranges_map);
bool remove_permanently_banned_ip_address_range(player& pd, std::vector<player>& banned_ip_address_ranges_vector, std::unordered_map<std::string, player>& banned_ip_address_ranges_map);
bool add_permanently_banned_city(const std::string& city, std::set<std::string>& banned_cities);
bool add_permanently_banned_country(const std::string& country, std::set<std::string>& banned_countries);
bool remove_permanently_banned_city(const std::string& city, std::set<std::string>& banned_cities);
bool remove_permanently_banned_country(const std::string& country, std::set<std::string>& banned_countries);
std::pair<bool, std::string> remove_temp_banned_ip_address(const std::string& ip_address);
std::pair<bool, std::string> remove_permanently_banned_ip_address(std::string& ip_address);

size_t print_colored_text(HWND re_control, const char* text, const is_append_message_to_richedit_control = is_append_message_to_richedit_control::yes, const is_log_message = is_log_message::yes, const is_log_datetime = is_log_datetime::yes, const bool is_prevent_auto_vertical_scrolling = false, const bool is_remove_color_codes_for_log_message = true);

size_t print_colored_text_to_grid_cell(HDC hdc, RECT& rect, const char* text, DWORD formatting_style);

bool get_user_input();

void print_help_information(const std::vector<std::string>&);

std::string prepare_current_match_information();
bool is_valid_decimal_whole_number(const std::string& str, int64_t& number);

bool check_if_user_provided_argument_is_valid_for_specified_command(
	const char* cmd,
	const std::string& arg);

bool check_if_user_provided_pid_is_valid(const std::string&);

void remove_all_color_codes(char* msg);
void remove_all_color_codes(std::string&);

void process_user_input(std::string&);

void process_user_command(const std::vector<std::string>&);

volatile bool should_program_terminate(const std::string & = "");

void display_permanently_banned_ip_addresses(const size_t number_of_last_bans_to_display = std::string::npos, const bool is_save_data_to_log_file = false);

void display_temporarily_banned_ip_addresses(const size_t number_of_last_bans_to_display = std::string::npos, const bool is_save_data_to_log_file = false);

void import_geoip_data(std::vector<geoip_data>&, const char*);

void export_geoip_data(const std::vector<geoip_data>&, const char*);

void change_colors();

void strip_leading_and_trailing_quotes(std::string&);

void replace_all_escaped_new_lines_with_new_lines(std::string&);

bool change_server_setting(const std::vector<std::string>&);

void log_message(const std::string&, const is_log_datetime = is_log_datetime::yes);

std::string get_player_information_for_player(player&);

bool specify_reason_for_player_pid(const int, const std::string&);
void tell_message(const char*, const int);

std::string word_wrap(const char*, const size_t);

std::string get_time_interval_info_string_for_seconds(const time_t seconds);
std::string get_time_interval_info_string_for_seconds_in_hours_and_minutes(const time_t seconds);

void change_game_type(const std::string& game_type, const bool = false);
void load_map(const std::string&, const std::string&, const bool = true);

template<typename... T>
void unused(T &&...) {}

template<typename... Args>
void say(HWND control, const char* szoveg, Args... args)
{
	static char outbuffer[8196];

	if (-1 == snprintf(outbuffer, std::size(outbuffer), szoveg, args...)) return;
	print_colored_text(control, outbuffer, is_append_message_to_richedit_control::yes);
}

template<typename... Args>
void csay(HWND control, const char* szoveg, Args... args)
{
	static char outbuffer[8196];

	if (-1 == snprintf(outbuffer, std::size(outbuffer), szoveg, args...)) return;

	print_colored_text(control, outbuffer, is_append_message_to_richedit_control::yes);
}

bool remove_dir_path_sep_char(char*);
void replace_forward_slash_with_backward_slash(std::string&);

bool connect_to_the_game_server(const std::string&, const game_name_t, const bool, const bool = true);

bool check_if_file_path_exists(const char*);
bool check_if_file_path_exists(const char*);

bool delete_temporary_game_file();

bool get_confirmation_message_from_user(const char*, const char*);
bool check_if_user_wants_to_quit(const char*);
// void process_user_input() ;
void process_key_down_message(const MSG&);

void construct_tinyrcon_gui(HWND);
void initialize_users_grid(HWND hgrid, const size_t cols, const size_t rows, const bool is_for_rcon_status = true);
void display_users_data_in_users_table(HWND playersGrid);
void clear_data_in_users_table(HWND playersGrid, const size_t start_row, const size_t last_row, const size_t cols);
void PutCell(HWND, const int, const int, const char*);
void set_check_box(HWND hgrid, const int row, const int col, const BOOL is_checked);
void display_country_flag(HWND, const int, const int, const char*);
std::string GetCellContents(HWND, const int row, const int col);

bool is_alpha(const char ch);
bool is_decimal_digit(const char ch);
bool is_ws(const char ch);

void change_hdc_fg_color(HDC hdc, COLORREF fg_color);
bool check_if_selected_cell_indices_are_valid_for_players_grid(const int row_index, const int col_index);
void CenterWindow(HWND hwnd);
bool show_user_confirmation_dialog(const char* msg, const char* title, const char* edit_label_text = "Reason:");

extern LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR);
void display_context_menu_over_grid(HWND control, const int mouse_x, const int mouse_y);

size_t get_file_size_in_bytes(const char*);
std::string get_date_and_time_for_time_t(const char* date_time_format_str, time_t t_c = 0);
const char* get_current_short_month_name(const size_t index);

std::pair<bool, std::string> create_7z_file_file_at_specified_path(const std::vector<std::string>& files_to_add, const std::string& local_file_path);
std::pair<bool, std::string> extract_7z_file_to_specified_path(const char* compressed_7z_file_path, const char* destination_path);

void display_banned_cities(const std::set<std::string>& banned_cities);
void display_banned_countries(const std::set<std::string>& banned_countries);
void save_banned_entries_to_file(const char* file_path, const std::set<std::string>& banned_entries);
void save_tempbans_to_file(const char* file_path, const std::vector<player>& temp_banned_players);

template<typename ContainerType, typename ElementValue>
void initialize_elements_of_container_to_specified_value(ContainerType& data, const ElementValue& value, const size_t start_index = 0)
{
	for (size_t i{ start_index }; i < data.size(); ++i) {
		data[i] = value;
	}
}

time_t get_current_time_stamp();

time_t get_number_of_seconds_from_date_and_time_string(const std::string& date_and_time);
void display_online_admins_information();
void removed_disallowed_character_in_string(std::string& input);
std::string remove_disallowed_character_in_string(const std::string&);
std::string get_cleaned_user_name(const std::string& name);
void replace_br_with_new_line(std::string& message);
void parse_protected_entries_file(const char* file_path, std::set<std::string>& protected_ip_addresses);
void save_protected_entries_file(const char* file_path, const std::set<std::string>& protected_entries);
void display_protected_entries(const char* table_title, const std::set<std::string>& protected_entries);
void get_first_valid_ip_address_from_ip_address_range(std::string ip_range, player& pd);
std::string get_narrow_ip_address_range_for_specified_ip_address(const std::string& ip_address);
std::string get_wide_ip_address_range_for_specified_ip_address(const std::string& ip_address);
bool check_if_player_is_protected(const player& online_player, const char* admin_command, std::string& message);
size_t get_random_number();
size_t find_longest_user_name_length(
	const std::vector<std::shared_ptr<tiny_rcon_client_user>>& users,
	const bool count_color_codes,
	const size_t number_of_users_to_process) noexcept;

size_t find_longest_user_country_city_info_length(
	const std::vector<std::shared_ptr<tiny_rcon_client_user>>& users,
	const size_t number_of_users_to_process) noexcept;
void display_admins_data();
void display_banned_ip_address_ranges(const size_t number_of_last_bans_to_display = std::string::npos, const bool is_save_data_to_log_file = false);
bool load_tinyrcon_statistics_data(const char* file_path);
bool save_tinyrcon_statistics_data(const char* file_path);
void parse_banned_names_file(const char* file_path, std::vector<player>& banned_names_vector, std::unordered_map<std::string, player>& banned_names_map, const bool is_skip_removed_check = false);
bool add_permanently_banned_player_name(player& pd, std::vector<player>& banned_players_names_vector, std::unordered_map<std::string, player>& banned_players_names_map);
bool remove_permanently_banned_player_name(player& pd, std::vector<player>& banned_names_vector, std::unordered_map<std::string, player>& banned_names_map);
std::string get_file_name_from_path(const std::string& file_path);
bool load_available_map_names(const char* map_names_file_path);
void send_user_available_map_names(const std::shared_ptr<tiny_rcon_client_user>&);
bool is_stock_cod2_map(const std::string& mapname);
bool fix_path_strings_in_json_config_file(const std::string& config_file_path);
std::string escape_backward_slash_characters_in_place(const std::string& line);
void sort_players_data(std::vector<player>&, const sort_type sort_method);
class game_server;
void update_game_server_setting(game_server& gs, std::string, std::string);
void prepare_players_data_for_display(game_server& gs, const bool is_log_status_table = false);
void prepare_players_data_for_display_of_getstatus_response(game_server& gs, const bool is_log_status_table = false);
class stats;
class stats_data;
std::string get_top_players_stats_data(std::vector<player_stats>& stats_data, std::unordered_map<std::string, player_stats>& stats_data_map,
	const size_t number_of_top_players, std::string& public_message, const char* title,
	std::string partial_or_full_player_name = "", const bool find_exact_player_name_match = false);
std::string get_online_players_stats_data_report(std::vector<player_stats>& stats_data, std::unordered_map<std::string, player_stats>& stats_data_map, const char* title);
void update_player_scores(stats& tinyrcon_stats);
void sort_players_stats_data(std::vector<player_stats>& stats_data_vec, std::unordered_map<std::string, player_stats>& stats_data_map);
void save_players_stats_data(const char* file_path, std::vector<player_stats>& stats_data, std::unordered_map<std::string, player_stats>& stats_data_map);
void load_players_stats_data(const char* file_path, std::vector<player_stats>& stats_data, std::unordered_map<std::string, player_stats>& stats_data_map);
void process_topplayers_request(const std::string& data);
[[maybe_unused]] bool tell_player_their_stats_data_in_a_private_message(const char* title, const player& pd);
std::string get_player_stats_information_message(const std::string& player_name);
bool remove_stats_for_player_name(const std::string& player_name_index);

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

inline std::pair<const char*, const char*> get_appropriate_rcon_status_response_header(const std::string& game_name)
{
	static constexpr const char* cod1_rcon_status_response_header{ "num score ping name            lastmsg address               qport rate\n" };
	static constexpr const char* cod2_rcon_status_response_header{ "num score ping guid   name            lastmsg address               qport rate\n" };
	static constexpr const char* cod4_rcon_status_response_header{ "num score ping guid                             name            lastmsg address               qport rate\n" };
	static constexpr const char* cod5_rcon_status_response_header{ "num score ping guid       name            lastmsg address               qport  rate\n" };
	static constexpr const char* cod5_rcon_status_response_header_team{ "num score ping guid       name            team lastmsg address               qport  rate\n" };

	if (game_name == "cod1")
		return std::make_pair(cod1_rcon_status_response_header, nullptr);
	if (game_name == "cod2")
		return std::make_pair(cod2_rcon_status_response_header, nullptr);
	if (game_name == "cod4")
		return std::make_pair(cod4_rcon_status_response_header, nullptr);
	if (game_name == "cod5")
		return std::make_pair(cod5_rcon_status_response_header, cod5_rcon_status_response_header_team);

	return std::make_pair(cod2_rcon_status_response_header, nullptr);
}

game_name_t convert_game_name_to_game_name_t(const std::string& game_name);
void correct_truncated_player_names(game_server& gs, const char* ip_address, const uint_least16_t port_number);
bool parse_game_type_information_from_rcon_reply(const std::string& rcon_reply, game_server& gs);
const std::string& get_full_gametype_name(const std::string&);
const std::string& get_full_map_name(const std::string&, const game_name_t game_name);
const std::map<std::string, std::string>& get_rcon_map_names_to_full_map_names_for_specified_game_name(const game_name_t);
const std::map<std::string, std::string>& get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(const game_name_t);