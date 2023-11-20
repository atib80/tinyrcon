#define WIN32_LEAN_AND_MEAN
#include <winapifamily.h>
#include <Windows.h>
#include "tiny_rcon_server_application.h"
#include "game_server.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <Psapi.h>
#include <regex>
#include <Shlobj.h>
#include <windowsx.h>
#include "json_parser.hpp"
#include "resource.h"
#include "simple_grid.h"
#include <bit7z.hpp>
#include <bit7zlibrary.hpp>
#include <bitextractor.hpp>
#include <bitexception.hpp>
#include <bittypes.hpp>
#include "stack_trace_element.h"

// Call of duty steam appid: 2620
// Call of duty 2 steam appid: 2630
// Call of duty 4: Modern Warfare steam appid: 7940
// Call of duty 5: World at war steam appid: 10090

#if defined(_DEBUG)
#pragma comment(lib, "bit7z_d.lib")
#else
#pragma comment(lib, "bit7z.lib")
#endif

#undef min

using namespace std;
using namespace stl::helper;
using namespace std::filesystem;

using stl::helper::trim_in_place;

extern tiny_rcon_server_application main_app;
extern tiny_rcon_handles app_handles;

extern char const *const tinyrcon_config_file_path;

extern char const *const tempbans_file_path;
extern char const *const removed_tempbans_file_path;

extern char const *const banned_ip_addresses_file_path;
extern char const *const removed_banned_ip_addresses_file_path;
extern char const *const protected_ip_addresses_file_path;

extern char const *const ip_range_bans_file_path;
extern char const *const removed_ip_range_bans_file_path;
extern char const *const protected_ip_address_ranges_file_path;

extern char const *const banned_countries_list_file_path;
extern char const *const removed_banned_countries_list_file_path;
extern char const *const protected_countries_list_file_path;

extern char const *const banned_cities_list_file_path;
extern char const *const removed_banned_cities_list_file_path;
extern char const *const protected_cities_list_file_path;

extern char const *const users_data_file_path;

extern const char *prompt_message;
extern PROCESS_INFORMATION pr_info;
extern sort_type type_of_sort;

extern WNDCLASSEX wcex_confirmation_dialog /*, wcex_configuration_dialog*/;
extern HFONT font_for_players_grid_data;
extern RECT client_rect;

extern const int screen_width;
extern const int screen_height;
extern int selected_row;
extern int selected_col;
extern const char *user_help_message;
extern const size_t max_users_grid_rows{ 100 };
extern bool is_tinyrcon_initialized;
std::atomic<bool> is_terminate_program{ false };
volatile std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window{ false };
extern std::atomic<int> admin_choice;
extern std::string admin_reason;

extern mutex mu;
extern condition_variable exit_flag;

recursive_mutex print_data_mutex;
recursive_mutex log_data_mutex;
recursive_mutex protect_get_user_input;

extern volatile atomic<size_t> atomic_counter;

bool sort_by_pid_asc{ true };
bool sort_by_score_asc{ false };
bool sort_by_ping_asc{ true };
bool sort_by_name_asc{ true };
bool sort_by_ip_asc{ true };
bool sort_by_geo_asc{ true };

extern bool is_process_combobox_item_selection_event;
extern bool is_first_left_mouse_button_click_in_reason_edit_control;

vector<string> commands_history;
size_t commands_history_index{};

string g_online_admins_information;
string g_message_data_contents;
string previous_map;
string match_information;

HIMAGELIST hImageList{};
row_of_player_data_to_display displayed_players_data[64]{};

static char path_buffer[32768];

extern const std::regex ip_address_and_port_regex{ R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(-?\d+))" };

extern const std::unordered_map<char, COLORREF> colors{
  { '0', color::black },
  { '1', color::red },
  { '2', color::green },
  { '3', color::yellow },
  { '4', color::blue },
  { '5', color::cyan },
  { '6', color::purple },
  { '7', color::black },
  { '8', color::grey },
  { '9', color::magenta },
};

extern const std::unordered_map<char, COLORREF> rich_edit_colors{
  { '0', color::white },
  { '1', color::red },
  { '2', color::green },
  { '3', color::yellow },
  { '4', color::blue },
  { '5', color::cyan },
  { '6', color::purple },
  { '7', color::white },
  { '8', color::grey },
  { '9', color::magenta },
};

extern const std::unordered_map<string, sort_type> sort_mode_names_dict{
  { "Sort by pid in ascending order", sort_type::pid_asc },
  { "Sort by pid in descending order", sort_type::pid_desc },
  { "Sort by score in ascending order", sort_type::score_asc },
  { "Sort by score in descending order", sort_type::score_desc },
  { "Sort by ping in ascending order", sort_type::ping_asc },
  { "Sort by ping in descending order", sort_type::ping_desc },
  { "Sort by player name in ascending order", sort_type::name_asc },
  { "Sort by player name in descending order", sort_type::name_desc },
  { "Sort by IP address in ascending order", sort_type::ip_asc },
  { "Sort by IP address in descending order", sort_type::ip_desc },
  { "Sort by geoinformation in ascending order", sort_type::geo_asc },
  { "Sort by geoinformation in descending order", sort_type::geo_desc }
};


extern const std::unordered_map<sort_type, string> sort_type_to_sort_names_dict{
  { sort_type::pid_asc, "Sort by pid in ascending order" },
  { sort_type::pid_desc, "Sort by pid in descending order" },
  { sort_type::score_asc, "Sort by score in ascending order" },
  { sort_type::score_desc, "Sort by score in descending order" },
  { sort_type::ping_asc, "Sort by ping in ascending order" },
  { sort_type::ping_desc, "Sort by ping in descending order" },
  { sort_type::name_asc, "Sort by player name in ascending order" },
  { sort_type::name_desc, "Sort by player name in descending order" },
  { sort_type::ip_asc, "Sort by IP address in ascending order" },
  { sort_type::ip_desc, "Sort by IP address in descending order" },
  { sort_type::geo_asc, "Sort by geoinformation in ascending order" },
  { sort_type::geo_desc, "Sort by geoinformation in descending order" }
};

extern const std::unordered_map<int, sort_type> sort_mode_col_index{ { 0, sort_type::pid_asc },
  { 1, sort_type::score_asc },
  { 2, sort_type::ping_asc },
  { 3, sort_type::name_asc },
  { 4, sort_type::ip_asc },
  { 5, sort_type::geo_asc } };

extern const char *black_bg_color{ "[[!black!]]" };
extern const char *grey_bg_color{ "[[!grey!]]" };
extern const size_t black_bg_color_length{ stl::helper::len(black_bg_color) };
extern const size_t grey_bg_color_length{ stl::helper::len(grey_bg_color) };

extern const map<string, string> user_commands_help{
  { "!cls",
    "^5!cls ^2-> clears the console screen." },
  { "!colors",
    "^5!colors ^2-> change colors of various displayed table entries and game information." },
  { "^5!warn",
    "^5!warn player_pid [reason] ^2-> warns the player whose pid number is equal to specified player_pid.\n A player who's been warned 2 times gets automatically kicked off the server." },
  { "^5!w",
    "^5!w player_pid [reason] ^2-> warns the player whose pid number is equal to specified player_pid.\n A player who's been warned 2 times gets automatically kicked off the server." },
  { "!kick",
    "^5!kick player_pid [reason] ^2-> kicks the player whose pid number is equal to "
    "specified player_pid." },
  { "!k",
    "^5!k player_pid [reason] ^2-> kicks the player whose pid number is equal to specified "
    "player_pid." },
  { "!tempban",
    "^5!tempban player_pid time_duration reason ^2-> temporarily bans (for time_duration hours, default 24 hours) "
    "IP address of player whose pid = ^112." },
  { "!tb",
    "^5!tb player_pid time_duration reason ^2-> temporarily bans (for time_duration hours, default 24 hours) "
    "IP address of player whose pid = ^112." },
  { "!ban",
    "^5!ban player_pid [reason] ^2-> bans the player whose pid number is equal to "
    "specified player_pid" },
  { "!b",
    "^5!b player_pid [reason] ^2-> bans the player whose pid number is equal to specified "
    "player_pid" },
  { "!gb",
    "^5!gb player_pid [reason] ^2-> permanently bans player's IP address whose pid number is "
    "equal to specified player_pid." },
  { "!globalban",
    "^5!globalban player_pid [reason] ^2-> permanently bans player's IP address whose pid "
    "number is equal to specified player_pid." },
  { "!status",
    "^5!status ^2-> retrieves current game state (players' data) from the server "
    "(pid, score, ping, name, guid, ip, qport, rate" },
  { "!s",
    "^5!s ^2-> retrieves current game state (players' data) from the server (pid, "
    "score, ping, name, guid, ip, qport, rate" },
  { "!time", "^5!time ^2-> prints current date/time information on the console." },
  { "!t", "^5!t ^2-> prints current date/time information on the console." },
  { "!sort",
    "^5!sort player_data asc|desc ^2-> examples: !sort pid asc, !sort pid desc, "
    "!sort score asc, !sort score desc\n !sort ping asc, !sort ping desc, "
    "!sort name asc, !sort name desc, !sort ip asc, !sort ip desc, !sort geo asc, !sort geo desc" },
  { "!list",
    "^5!list user|rcon ^2-> shows a list of available user or rcon level commands "
    "that are available for admin\n to type into the console program." },
  { "!bans",
    "^5!bans ^2-> displays a list of permanently banned IP addresses." },
  { "!tempbans", "^5!tempbans ^2-> displays a list of temporarily banned IP addresses." },
  { "!banip",
    "^5!banip pid|valid_ip_address [reason] ^2-> bans player whose 'pid' number or "
    "'ip_address' is equal to specified 'pid' or 'valid_ip_address', "
    "respectively." },
  { "!addip",
    "^5!addip pid|valid_ip_address [reason] ^2-> bans player whose 'pid' number or "
    "'ip_address' is equal to specified 'pid' or 'valid_ip_address', "
    "respectively." },
  { "!ub",
    "^5!ub valid_ip_address [reason] ^2-> removes temporarily and|or permanently banned IP address "
    "'valid_ip_address'." },
  { "!unban",
    "^5!unban valid_ip_address [reason] ^2-> removes temporarily and|or permanently banned IP address "
    "'valid_ip_address'." },
  { "^5!m map_name game_type", "^5!m map_name game_type ^2-> loads map 'map_name' in specified game type 'game_type'" },
  { "!maps", "^5!maps ^2-> displays all available playable maps. " },
  { "!c", "^5!c [IP:PORT] ^2-> launches your Call of Duty game and connects to currently configured game server or optionally specified game server address ^5[IP:PORT]" },
  { "!cp", "^5!cp [IP:PORT] ^2-> launches your Call of Duty game and connects to currently configured game server or optionally specified game server address ^5[IP:PORT] using a private slot." },
  { "!rt", "^5!rt time_period ^2-> sets time period (automatic checking for banned players) to time_period (1-30 seconds)." },
  { "!config", "^5!config [rcon|private|address|name] [new_rcon_password|new_private_password|new_server_address|new_name]\n ^2-> you change tinyrcon's ^1rcon ^2or ^1private slot password^2, registered server ^1IP:port ^2address or your ^1username ^2using this command.\nFor example ^1!config rcon abc123 ^2changes currently used ^1rcon_password ^2to ^1abc123^2\n ^1!config private abc123 ^2changes currently used ^1sv_privatepassword ^2to ^1abc123^2\n ^1!config address 123.101.102.103:28960 ^2changes currently used server ^1IP:port ^2to ^1123.101.102.103:28960\n ^1!config name Administrator ^2changes currently used ^1username ^2to ^1Administrator" },
  { "!border", "^5Turns ^3on^5|^3off ^5border lines around displayed ^3GUI controls^5." },
  { "!messages", "^5Turns ^3on^5|^3off ^5messages for temporarily and permanently banned players." },
  { "!banned cities", "^5Displays lists of all currently ^1banned cities" },
  { "!banned countries", "^5Displays lists of all currently ^1banned countries" },
  { "!ecb", "^2Enables ^5Tiny^6Rcon's ^2country ban feature!" },
  { "!dcb", "^3Disables ^5Tiny^6Rcon's ^3country ban feature!" },
  { "!bancountry", "^5!bancountry country name ^2-> ^5Adds player's ^3country ^5to list of banned countries!" },
  { "!unbancountry", "^5unbancountry country name ^2-> ^5Removes player's ^3country ^5from list of banned countries!" },
  { "!egb", "^2Enables ^5Tiny^6Rcon's ^2city ban feature!" },
  { "!dgb", "^3Disables ^5Tiny^6Rcon's ^3city ban feature!" },
  { "!bancity", "^5!bancity city name ^2-> ^5Adds player's ^3city ^5to list of banned cities!" },
  { "!unbancity", "^5!unbancity city name ^2-> ^5Removes player's ^3city ^5from list of banned cities!" },
  { "!restart", "^5!restart ^2-> ^3Remotely ^1restarts ^3logged in ^5Tiny^6Rcon ^3clients." }

};

extern const unordered_set<string> user_commands_set{
  "!c",
  "!cp",
  "cls",
  "!cls",
  "!colors",
  "colors",
  "!warn",
  "!w",
  "!kick",
  "!k",
  "!tempban",
  "!tb",
  "!ban",
  "!b",
  "!gb",
  "!globalban",
  "!m",
  "!map",
  "maps",
  "!maps",
  "!status",
  "s",
  "!s",
  "bans",
  "tempbans",
  "t",
  "!t",
  "!time",
  "sort",
  "!sort",
  "list",
  "!list",
  "!bans",
  "!tempbans",
  "!addip",
  "!banip",
  "!ub",
  "!unban",
  "gs",
  "!gs",
  "!getstatus",
  "!config",
  "!rt",
  "!refreshtime",
  "border",
  "!border",
  "messages",
  "!messages",
  "!egb",
  "!dgb",
  "!bancity",
  "!unbancity",
  "!ecb",
  "!dcb",
  "!bancountry",
  "!unbancountry",
  "!banned",
  "!bannedcities",
  "!bannedcountries",
  "!protectip",
  "!protectiprange",
  "!protectcity",
  "!protectcountry",
  "!unprotectip",
  "!unprotectiprange",
  "!unprotectcity",
  "!unprotectcountry",
  "!restart",
  "!ranges",
  "!admins"
};

extern unordered_map<size_t, string> users_table_column_header_titles;
extern unordered_map<string, game_name_t> game_name_to_game_name_t{
  { "unknown", game_name_t::unknown },
  { "cod1", game_name_t::cod1 },
  { "Call of Duty", game_name_t::cod1 },
  { "cod2", game_name_t::cod2 },
  { "Call of Duty 2", game_name_t::cod2 },
  { "cod4", game_name_t::cod4 },
  { "Call of Duty 4", game_name_t::cod4 },
  { "cod5", game_name_t::cod5 },
  { "Call of Duty 5", game_name_t::cod5 },
};

extern const unordered_map<string, int> flag_name_index{
  { "xy", 0 },
  { "ad", 1 },
  { "ae", 2 },
  { "af", 3 },
  { "ag", 4 },
  { "ai", 5 },
  { "al", 6 },
  { "am", 7 },
  { "ao", 8 },
  { "aq", 9 },
  { "ar", 10 },
  { "as", 11 },
  { "at", 12 },
  { "au", 13 },
  { "aw", 14 },
  { "ax", 15 },
  { "az", 16 },
  { "ba", 17 },
  { "bb", 18 },
  { "bd", 19 },
  { "be", 20 },
  { "bf", 21 },
  { "bg", 22 },
  { "bh", 23 },
  { "bi", 24 },
  { "bj", 25 },
  { "bl", 26 },
  { "bm", 27 },
  { "bn", 28 },
  { "bo", 29 },
  { "bq", 30 },
  { "br", 31 },
  { "bs", 32 },
  { "bt", 33 },
  { "bv", 34 },
  { "bw", 35 },
  { "by", 36 },
  { "bz", 37 },
  { "ca", 38 },
  { "cc", 39 },
  { "cd", 40 },
  { "cf", 41 },
  { "cg", 42 },
  { "ch", 43 },
  { "ci", 44 },
  { "ck", 45 },
  { "cl", 46 },
  { "cm", 47 },
  { "cn", 48 },
  { "co", 49 },
  { "cr", 50 },
  { "cu", 51 },
  { "cv", 52 },
  { "cw", 53 },
  { "cx", 54 },
  { "cy", 55 },
  { "cz", 56 },
  { "de", 57 },
  { "dj", 58 },
  { "dk", 59 },
  { "dm", 60 },
  { "do", 61 },
  { "dz", 62 },
  { "ec", 63 },
  { "ee", 64 },
  { "eg", 65 },
  { "eh", 66 },
  { "er", 67 },
  { "es", 68 },
  { "et", 69 },
  { "fi", 70 },
  { "fj", 71 },
  { "fk", 72 },
  { "fm", 73 },
  { "fo", 74 },
  { "fr", 75 },
  { "ga", 76 },
  { "gb", 77 },
  { "gb_eng", 78 },
  { "gb_nir", 79 },
  { "gb_sct", 80 },
  { "gb_wls", 81 },
  { "gd", 82 },
  { "ge", 83 },
  { "gf", 84 },
  { "gg", 85 },
  { "gh", 86 },
  { "gi", 87 },
  { "gl", 88 },
  { "gm", 89 },
  { "gn", 90 },
  { "gp", 91 },
  { "gq", 92 },
  { "gr", 93 },
  { "gs", 94 },
  { "gt", 95 },
  { "gu", 96 },
  { "gw", 97 },
  { "gy", 98 },
  { "hk", 99 },
  { "hm", 100 },
  { "hn", 101 },
  { "hr", 102 },
  { "ht", 103 },
  { "hu", 104 },
  { "id", 105 },
  { "ie", 106 },
  { "il", 107 },
  { "im", 108 },
  { "in", 109 },
  { "io", 110 },
  { "iq", 111 },
  { "ir", 112 },
  { "is", 113 },
  { "it", 114 },
  { "je", 115 },
  { "jm", 116 },
  { "jo", 117 },
  { "jp", 118 },
  { "ke", 119 },
  { "kg", 120 },
  { "kh", 121 },
  { "ki", 122 },
  { "km", 123 },
  { "kn", 124 },
  { "kp", 125 },
  { "kr", 126 },
  { "kw", 127 },
  { "ky", 128 },
  { "kz", 129 },
  { "la", 130 },
  { "lb", 131 },
  { "lc", 132 },
  { "li", 133 },
  { "lk", 134 },
  { "lr", 135 },
  { "ls", 136 },
  { "lt", 137 },
  { "lu", 138 },
  { "lv", 139 },
  { "ly", 140 },
  { "ma", 141 },
  { "mc", 142 },
  { "md", 143 },
  { "me", 144 },
  { "mf", 145 },
  { "mg", 146 },
  { "mh", 147 },
  { "mk", 148 },
  { "ml", 149 },
  { "mm", 150 },
  { "mn", 151 },
  { "mo", 152 },
  { "mp", 153 },
  { "mq", 154 },
  { "mr", 155 },
  { "ms", 156 },
  { "mt", 157 },
  { "mu", 158 },
  { "mv", 159 },
  { "mw", 160 },
  { "mx", 161 },
  { "my", 162 },
  { "mz", 163 },
  { "na", 164 },
  { "nc", 165 },
  { "ne", 166 },
  { "nf", 167 },
  { "ng", 168 },
  { "ni", 169 },
  { "nl", 170 },
  { "no", 171 },
  { "np", 172 },
  { "nr", 173 },
  { "nu", 174 },
  { "nz", 175 },
  { "om", 176 },
  { "pa", 177 },
  { "pe", 178 },
  { "pf", 179 },
  { "pg", 180 },
  { "ph", 181 },
  { "pk", 182 },
  { "pl", 183 },
  { "pm", 184 },
  { "pn", 185 },
  { "pr", 186 },
  { "ps", 187 },
  { "pt", 188 },
  { "pw", 189 },
  { "py", 190 },
  { "qa", 191 },
  { "re", 192 },
  { "ro", 193 },
  { "rs", 194 },
  { "ru", 195 },
  { "rw", 196 },
  { "sa", 197 },
  { "sb", 198 },
  { "sc", 199 },
  { "sd", 200 },
  { "se", 201 },
  { "sg", 202 },
  { "sh", 203 },
  { "si", 204 },
  { "sj", 205 },
  { "sk", 206 },
  { "sl", 207 },
  { "sm", 208 },
  { "sn", 209 },
  { "so", 210 },
  { "sr", 211 },
  { "ss", 212 },
  { "st", 213 },
  { "sv", 214 },
  { "sx", 215 },
  { "sy", 216 },
  { "sz", 217 },
  { "tc", 218 },
  { "td", 219 },
  { "tf", 220 },
  { "tg", 221 },
  { "th", 222 },
  { "tj", 223 },
  { "tk", 224 },
  { "tl", 225 },
  { "tm", 226 },
  { "tn", 227 },
  { "to", 228 },
  { "tr", 229 },
  { "tt", 230 },
  { "tv", 231 },
  { "tw", 232 },
  { "tz", 233 },
  { "ua", 234 },
  { "ug", 235 },
  { "um", 236 },
  { "us", 237 },
  { "uy", 238 },
  { "uz", 239 },
  { "va", 240 },
  { "vc", 241 },
  { "ve", 242 },
  { "vg", 243 },
  { "vi", 244 },
  { "vn", 245 },
  { "vu", 246 },
  { "wf", 247 },
  { "ws", 248 },
  { "xk", 249 },
  { "ye", 250 },
  { "yt", 251 },
  { "za", 252 },
  { "zm", 253 },
  { "zw", 254 }
};

void show_error(HWND parent_window, const char *error_message, const size_t error_level)
{
  switch (error_level) {
  case 0:
    MessageBox(parent_window, error_message, "Warning message", MB_OK | MB_ICONWARNING);
    break;
  case 1:
    MessageBox(parent_window, error_message, "Error message", MB_OK | MB_ICONERROR | MB_ICONEXCLAMATION);
    exit(1);
    break;
  case 2:
    MessageBox(parent_window, error_message, "Fatal error message", MB_OK | MB_ICONERROR | MB_ICONEXCLAMATION);
    exit(2);
    break;
  default:
    MessageBox(parent_window, error_message, "Unknown warning-level message", MB_OK | MB_ICONWARNING);
    break;
  }
}

size_t get_number_of_lines_in_file(const char *file_path)
{
  ifstream input{ file_path };
  if (!input)
    return 0;

  size_t lines_cnt{};
  for (string line; getline(input, line); ++lines_cnt)
    ;

  return lines_cnt + 1;
}

bool parse_geodata_lite_csv_file(const char *file_path)
{

  ifstream input{ file_path };
  if (!input)
    return false;

  vector<geoip_data> geoip_data;
  geoip_data.reserve(get_number_of_lines_in_file(file_path));

  for (string line; getline(input, line);) {
    stl::helper::trim_in_place(line);
    auto parts = stl::helper::str_split(line, ',', nullptr, split_on_whole_needle_t::yes);
    parts[0].pop_back();
    parts[0].erase(0, 1);
    parts[1].pop_back();
    parts[1].erase(0, 1);
    parts[2].pop_back();
    parts[2].erase(0, 1);
    parts[3].pop_back();
    parts[3].erase(0, 1);

    parts[2][0] = static_cast<char>(std::tolower(parts[2][0]));
    parts[2][1] = static_cast<char>(std::tolower(parts[2][1]));

    if (size_t pos = parts[3].find("United Kingdom"); pos != string::npos) {
      parts[3] = "UK";
    } else if (pos = parts[3].find("United States of America"); pos != string::npos) {
      parts[3] = "USA";
    } else if (pos = parts[3].find("United Arab Emirates"); pos != string::npos) {
      parts[3] = "UAE";
    }

    for (size_t current{}; (current = parts[3].find('(')) != string::npos;) {
      const size_t next{ parts[3].find(')', current + 1) };
      if (next == string::npos)
        break;
      parts[3].erase(current, next - current + 1);
      stl::helper::trim_in_place(parts[3]);
    }


    parts[4].pop_back();
    parts[4].erase(0, 1);
    parts[5].pop_back();
    parts[5].erase(0, 1);
    geoip_data.emplace_back(stoul(parts[0]), stoul(parts[1]), parts[2].c_str(), parts[3].c_str(), parts[4].c_str(), parts[5].c_str());
  }

  main_app.get_connection_manager_for_messages().get_geoip_data() = std::move(geoip_data);


  return true;
}

bool create_necessary_folders_and_files(const std::vector<string> &folder_file_paths)
{

  unordered_set<string> created_folders;

  for (const auto &file_path : folder_file_paths) {
    const directory_entry entry{ file_path };
    if (!check_if_file_path_exists(file_path.c_str())) {

      string file_name;

      size_t last_sep_pos{ file_path.find_last_of("./\\") };
      if (string::npos != last_sep_pos && '.' == file_path[last_sep_pos]) {
        last_sep_pos = file_path.find_last_of("/\\", last_sep_pos - 1);
        file_name = file_path.substr(last_sep_pos != string::npos ? last_sep_pos + 1 : 0);
      } else {
        last_sep_pos = file_path.length();
      }

      string parent_path{ string::npos != last_sep_pos ? file_path.substr(0, last_sep_pos) : ""s };

      if (!parent_path.empty() && !created_folders.contains(parent_path)) {
        error_code ec{};
        create_directories(parent_path, ec);
        if (ec.value() != 0)
          return false;
        created_folders.emplace(std::move(parent_path));
      }

      if (!file_name.empty() && !check_if_file_path_exists(entry.path().string().c_str())) {
        ofstream file_to_create{ entry.path().string().c_str() };
        if (!file_to_create)
          return false;
        if (file_name == "tinyrcon.json") {
          write_tiny_rcon_json_settings_to_file(entry.path().string().c_str());
        }
      }
    }
  }

  return true;
}

bool write_tiny_rcon_json_settings_to_file(
  const char *file_path)
{
  std::ofstream config_file{ file_path };

  if (!config_file) {
    const string re_msg{ "Error writing default tiny rcon too 's configuration data\nto specified tinyrcon.json file ("s + file_path + ".\nPlease make sure that the main application folder's properties are not set to read-only mode!"s };
    show_error(app_handles.hwnd_main_window, re_msg.c_str(), 0);
    return false;
  }

  config_file << "{\n"
              << "\"username\": \"" << main_app.get_username() << "\",\n";

  config_file << "\"program_title\": \"" << main_app.get_program_title() << "\",\n";
  config_file << "\"check_for_banned_players_time_interval\": " << main_app.get_game_server().get_check_for_banned_players_time_period() << ",\n";
  config_file << "\"game_server_name\": \"" << main_app.get_game_server_name() << "\",\n";
  config_file << "\"rcon_server_ip_address\": \"" << main_app.get_game_server().get_server_ip_address() << "\",\n";
  config_file << "\"rcon_port\": " << main_app.get_game_server().get_server_port() << ",\n";
  config_file << "\"rcon_password\": \"" << main_app.get_game_server().get_rcon_password() << "\",\n";
  config_file << "\"private_slot_password\": \"" << main_app.get_game_server().get_private_slot_password() << "\",\n";
  config_file << "\"tiny_rcon_server_ip\": \"" << main_app.get_tiny_rcon_server_ip_address() << "\",\n";
  config_file << "\"tiny_rcon_server_port\": " << main_app.get_tiny_rcon_server_port() << ",\n";
  config_file << "\"tiny_rcon_ftp_server_username\": \"" << main_app.get_tiny_rcon_ftp_server_username() << "\",\n";
  config_file << "\"tiny_rcon_ftp_server_password\": \"" << main_app.get_tiny_rcon_ftp_server_password() << "\",\n";
  config_file << "\"codmp_exe_path\": \"" << main_app.get_codmp_exe_path() << "\",\n";
  config_file << "\"cod2mp_s_exe_path\": \"" << main_app.get_cod2mp_exe_path() << "\",\n";
  config_file << "\"iw3mp_exe_path\": \"" << main_app.get_iw3mp_exe_path() << "\",\n";
  config_file << "\"cod5mp_exe_path\": \"" << main_app.get_cod5mp_exe_path() << "\",\n";
  config_file << "\"is_automatic_city_kick_enabled\": " << (main_app.get_game_server().get_is_automatic_city_kick_enabled() ? "true" : "false") << ",\n";
  config_file << "\"is_automatic_country_kick_enabled\": " << (main_app.get_game_server().get_is_automatic_country_kick_enabled() ? "true" : "false") << ",\n";
  config_file << "\"enable_automatic_connection_flood_ip_ban\": " << (main_app.get_game_server().get_is_enable_automatic_connection_flood_ip_ban() ? "true" : "false") << ",\n";
  config_file << "\"minimum_number_of_connections_from_same_ip_for_automatic_ban\": " << main_app.get_game_server().get_minimum_number_of_connections_from_same_ip_for_automatic_ban() << ",\n";
  config_file << "\"number_of_warnings_for_automatic_kick\": " << main_app.get_game_server().get_maximum_number_of_warnings_for_automatic_kick() << ",\n";
  config_file << "\"disable_automatic_kick_messages\": " << (main_app.get_game_server().get_is_disable_automatic_kick_messages() ? "true" : "false")
              << ",\n";
  config_file << "\"use_original_admin_messages\": " << (main_app.get_game_server().get_is_use_original_admin_messages() ? "true" : "false")
              << ",\n";
  config_file << "\"user_defined_warn_msg\": \"" << main_app.get_admin_messages()["user_defined_warn_msg"] << "\",\n";
  config_file << "\"user_defined_kick_msg\": \"" << main_app.get_admin_messages()["user_defined_kick_msg"] << "\",\n";
  config_file << "\"user_defined_temp_ban_msg\": \"" << main_app.get_admin_messages()["user_defined_temp_ban_msg"] << "\",\n";
  config_file << "\"user_defined_ban_msg\": \"" << main_app.get_admin_messages()["user_defined_ban_msg"] << "\",\n";
  config_file << "\"user_defined_ip_ban_msg\": \"" << main_app.get_admin_messages()["user_defined_ip_ban_msg"] << "\",\n";
  config_file << "\"user_defined_city_ban_msg\": \"" << main_app.get_admin_messages()["user_defined_city_ban_msg"] << "\",\n";
  config_file << "\"user_defined_city_unban_msg\": \"" << main_app.get_admin_messages()["user_defined_city_unban_msg"] << "\",\n";
  config_file << "\"user_defined_enable_city_ban_feature_msg\": \"" << main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] << "\",\n";
  config_file << "\"user_defined_disable_city_ban_feature_msg\": \"" << main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] << "\",\n";
  config_file << "\"user_defined_country_ban_msg\": \"" << main_app.get_admin_messages()["user_defined_country_ban_msg"] << "\",\n";
  config_file << "\"user_defined_country_unban_msg\": \"" << main_app.get_admin_messages()["user_defined_country_unban_msg"] << "\",\n";
  config_file << "\"user_defined_enable_country_ban_feature_msg\": \"" << main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] << "\",\n";
  config_file << "\"user_defined_disable_country_ban_feature_msg\": \"" << main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] << "\",\n";
  config_file << "\"automatic_remove_temp_ban_msg\": \"" << main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] << "\",\n";
  config_file << "\"automatic_kick_temp_ban_msg\": \"" << main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] << "\",\n";
  config_file << "\"automatic_kick_ip_ban_msg\": \"" << main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] << "\",\n";
  config_file << "\"automatic_kick_city_ban_msg\": \"" << main_app.get_admin_messages()["automatic_kick_city_ban_msg"] << "\",\n";
  config_file << "\"automatic_kick_country_ban_msg\": \"" << main_app.get_admin_messages()["automatic_kick_country_ban_msg"] << "\",\n";
  config_file << "\"current_match_info\": \"" << main_app.get_game_server().get_current_match_info() << "\",\n";
  config_file << "\"use_different_background_colors_for_even_and_odd_lines\": " << (main_app.get_game_server().get_is_use_different_background_colors_for_even_and_odd_lines() ? "true" : "false") << ",\n";
  config_file << "\"odd_player_data_lines_bg_color\": \"" << main_app.get_game_server().get_odd_player_data_lines_bg_color() << "\",\n";
  config_file << "\"even_player_data_lines_bg_color\": \"" << main_app.get_game_server().get_even_player_data_lines_bg_color() << "\",\n";
  config_file << "\"use_different_foreground_colors_for_even_and_odd_lines\": " << (main_app.get_game_server().get_is_use_different_foreground_colors_for_even_and_odd_lines() ? "true" : "false") << ",\n";
  config_file << "\"odd_player_data_lines_fg_color\": \"" << main_app.get_game_server().get_odd_player_data_lines_fg_color() << "\",\n";
  config_file << "\"even_player_data_lines_fg_color\": \"" << main_app.get_game_server().get_even_player_data_lines_fg_color() << "\",\n";
  config_file << "\"full_map_name_color\": \"" << main_app.get_game_server().get_full_map_name_color() << "\",\n";
  config_file << "\"rcon_map_name_color\": \"" << main_app.get_game_server().get_rcon_map_name_color() << "\",\n";
  config_file << "\"full_game_type_color\": \"" << main_app.get_game_server().get_full_gametype_name_color() << "\",\n";
  config_file << "\"rcon_game_type_color\": \"" << main_app.get_game_server().get_rcon_gametype_name_color() << "\",\n";

  config_file << "\"online_players_count_color\": \"" << main_app.get_game_server().get_online_players_count_color() << "\",\n";
  config_file << "\"offline_players_count_color\": \"" << main_app.get_game_server().get_offline_players_count_color() << "\",\n";
  config_file << "\"border_line_color\": \"" << main_app.get_game_server().get_border_line_color() << "\",\n";
  config_file << "\"header_player_pid_color\": \"" << main_app.get_game_server().get_header_player_pid_color() << "\",\n";
  config_file << "\"data_player_pid_color\": \"" << main_app.get_game_server().get_data_player_pid_color() << "\",\n";
  config_file << "\"header_player_score_color\": \"" << main_app.get_game_server().get_header_player_score_color() << "\",\n";
  config_file << "\"data_player_score_color\": \"" << main_app.get_game_server().get_data_player_score_color() << "\",\n";
  config_file << "\"header_player_ping_color\": \"" << main_app.get_game_server().get_header_player_ping_color() << "\",\n";
  config_file << "\"data_player_ping_color\": \"" << main_app.get_game_server().get_data_player_ping_color() << "\",\n";
  config_file << "\"header_player_name_color\": \"" << main_app.get_game_server().get_header_player_name_color() << "\",\n";
  config_file << "\"header_player_ip_color\": \"" << main_app.get_game_server().get_header_player_ip_color() << "\",\n";
  config_file << "\"data_player_ip_color\": \"" << main_app.get_game_server().get_data_player_ip_color() << "\",\n";
  config_file << "\"header_player_geoinfo_color\": \"" << main_app.get_game_server().get_header_player_geoinfo_color() << "\",\n";
  config_file << "\"data_player_geoinfo_color\": \"" << main_app.get_game_server().get_data_player_geoinfo_color() << "\",\n";
  config_file << "\"ftp_download_site_ip_address\": \"" << main_app.get_ftp_download_site_ip_address() << "\",\n";
  config_file << "\"ftp_download_folder_path\": \"" << main_app.get_ftp_download_folder_path() << "\",\n";
  config_file << "\"ftp_bans_folder_path\": \"" << main_app.get_ftp_bans_folder_path() << "\",\n";
  config_file << "\"ftp_download_file_pattern\": \"" << main_app.get_ftp_download_file_pattern() << "\",\n";
  config_file << "\"plugins_geoIP_geo_dat_md5\": \"" << main_app.get_plugins_geoIP_geo_dat_md5() << "\"\n";
  config_file << "}" << flush;
  return true;
}


bool save_tiny_rcon_users_data_to_json_file(const char *json_file_path)
{

  std::ofstream config_file{ json_file_path };

  if (!config_file) {
    const string re_msg{ "Error saving tinyrcon users's data\nto specified json file path ("s + json_file_path + ".\nPlease make sure that the main application folder's properties are not set to read-only mode!"s };
    show_error(app_handles.hwnd_main_window, re_msg.c_str(), 0);
    return false;
  }

  auto &users = main_app.get_users();
  if (!users.empty()) {

    for (size_t i{}; i < users.size(); ++i) {
      const string user_name{ remove_disallowed_character_in_string(users[i]->user_name) };
      config_file << format("{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\\{}\n", user_name, (users[i]->is_admin ? "true" : "false"), (users[i]->is_logged_in ? "true" : "false"), (users[i]->is_online ? "true" : "false"), users[i]->ip_address, users[i]->geo_information, users[i]->last_login_time_stamp, users[i]->last_logout_time_stamp, users[i]->no_of_logins, users[i]->no_of_warnings, users[i]->no_of_kicks, users[i]->no_of_tempbans, users[i]->no_of_guidbans, users[i]->no_of_ipbans, users[i]->no_of_iprangebans, users[i]->no_of_citybans, users[i]->no_of_countrybans);
    }
  }

  config_file << flush;

  return true;
}

bool check_ip_address_validity(string_view ip_address,
  unsigned long &ip_key)
{
  string ex_msg{ format(R"(^1Exception ^3thrown from ^1bool check_ip_address_validity("{}", "{}"))", ip_address, ip_key) };
  stack_trace_element ste{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };
  auto parts = stl::helper::str_split(ip_address, ".", nullptr, split_on_whole_needle_t::yes);
  if (parts.size() < 4U)
    return false;

  for (auto &part : parts)
    stl::helper::trim_in_place(part);

  const bool is_valid_ip_address =
    all_of(cbegin(parts), cend(parts), [](const string &part) {
      int number{};
      if (!is_valid_decimal_whole_number(part, number))
        return false;
      const int value{ stoi(part) };
      return value >= 0 && value <= 255;
    });

  if (is_valid_ip_address) {
    ip_key = 0UL;
    for (size_t i{}; i < 4; ++i) {
      ip_key <<= 8;
      ip_key += stoul(parts[i]);
    }
  }

  return is_valid_ip_address;
}

void convert_guid_key_to_country_name(const vector<geoip_data> &geo_data,
  string_view player_ip,
  player &player_data)
{
  unsigned long ip_key{};
  if (!check_ip_address_validity(player_ip, ip_key)) {
    player_data.ip_hash_key = 0UL;
    player_data.country_name = "Unknown";
    player_data.region = "Unknown";
    player_data.city = "Unknown";
    player_data.country_code = "xy";

  } else {
    player_data.ip_hash_key = ip_key;
    const size_t sizeOfElements{ geo_data.size() };
    size_t lower_bound{ 0 };
    size_t upper_bound{ sizeOfElements };

    bool is_found_match{};

    while (lower_bound <= upper_bound) {
      const size_t currentIndex = (lower_bound + upper_bound) / 2;

      if (currentIndex >= geo_data.size())
        break;

      if (ip_key >= geo_data.at(currentIndex).lower_ip_bound && ip_key <= geo_data.at(currentIndex).upper_ip_bound) {
        player_data.country_name = geo_data.at(currentIndex).get_country_name();
        if (strstr(player_data.country_name, "Russian") != nullptr)
          player_data.country_name = "Russia";
        player_data.region = geo_data.at(currentIndex).get_region();
        player_data.city = geo_data.at(currentIndex).get_city();
        player_data.country_code = geo_data.at(currentIndex).get_country_code();
        is_found_match = true;
        break;
      }

      if (ip_key > geo_data.at(currentIndex).upper_ip_bound) {
        lower_bound = currentIndex + 1;
      } else {
        upper_bound = currentIndex - 1;
      }
    }

    if (!is_found_match) {
      player_data.country_name = "Unknown";
      player_data.region = "Unknown";
      player_data.city = "Unknown";
      player_data.country_code = "xy";
    }
  }
}

size_t find_longest_player_name_length(
  const std::vector<player> &players,
  const bool count_color_codes,
  const size_t number_of_players_to_process)
{
  if (0 == number_of_players_to_process)
    return 0;
  size_t max_player_name_length{ 8 };
  for (size_t i{}; i < number_of_players_to_process; ++i) {
    max_player_name_length =
      max(count_color_codes ? len(players[i].player_name) : get_number_of_characters_without_color_codes(players[i].player_name), max_player_name_length);
  }

  return max_player_name_length;
}

size_t find_longest_player_country_city_info_length(
  const std::vector<player> &players,
  const size_t number_of_players_to_process)
{
  if (0 == number_of_players_to_process)
    return 0;

  size_t country_len{ 8 };
  size_t region_len{ 8 };
  size_t city_len{ 8 };

  size_t max_geodata_info_length = (country_len != 0 ? country_len : region_len) + city_len + 2;
  for (size_t i{}; i < number_of_players_to_process; ++i) {
    country_len = len(players[i].country_name);
    region_len = len(players[i].region);
    city_len = len(players[i].city);
    const size_t current_player_geodata_info_length = (country_len != 0 ? country_len : region_len) + city_len + 2;
    max_geodata_info_length = std::max(current_player_geodata_info_length, max_geodata_info_length);
  }

  return max_geodata_info_length;
}

void parse_tinyrcon_tool_config_file(const char *configFileName)
{
  ifstream configFile{ configFileName };

  RSJresource json_resource = RSJresource(configFile);

  if (!json_resource.exists()) {
    configFile.clear();
    configFile.close();
    write_tiny_rcon_json_settings_to_file(configFileName);
    configFile.open(configFileName, std::ios_base::in);
    json_resource = RSJresource(configFile);
  }


  string data_line;
  bool found_missing_config_setting{};

  if (json_resource["username"].exists()) {

    data_line = json_resource["username"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    data_line = remove_disallowed_character_in_string(data_line);
    main_app.set_username(data_line);
    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.set_username("^1Admin");
    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = "^1Admin";
  }

  if (json_resource["program_title"].exists()) {
    data_line = json_resource["program_title"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_program_title(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_program_title("Welcome to TinyRcon");
  }

  if (json_resource["check_for_banned_players_time_interval"].exists()) {
    main_app.get_game_server().set_check_for_banned_players_time_period(json_resource["check_for_banned_players_time_interval"].as<int>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_check_for_banned_players_time_period(5);
  }

  if (json_resource["game_server_name"].exists()) {
    data_line = json_resource["game_server_name"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_game_server_name(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_game_server_name("TinyRcon game server");
  }

  if (json_resource["rcon_server_ip_address"].exists()) {
    data_line = json_resource["rcon_server_ip_address"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_server_ip_address(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_server_ip_address("185.158.113.146");
  }

  if (json_resource["rcon_port"].exists()) {
    const int port_number{ json_resource["rcon_port"].as<int>() };
    main_app.get_game_server().set_server_port(static_cast<uint_least16_t>(port_number));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_server_port(28995);
  }

  if (json_resource["rcon_password"].exists()) {
    data_line = json_resource["rcon_password"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_rcon_password(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_rcon_password("abc123");
  }

  if (json_resource["private_slot_password"].exists()) {
    data_line = json_resource["private_slot_password"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_private_slot_password(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_private_slot_password("abc123");
  }

  if (json_resource["tiny_rcon_server_ip"].exists()) {
    data_line = json_resource["tiny_rcon_server_ip"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_tiny_rcon_server_ip_address(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_tiny_rcon_server_ip_address("85.222.189.119");
  }

  if (json_resource["tiny_rcon_server_port"].exists()) {
    const int port_number{ json_resource["tiny_rcon_server_port"].as<int>() };
    main_app.set_tiny_rcon_server_port(static_cast<uint_least16_t>(port_number));
  } else {
    found_missing_config_setting = true;
    main_app.set_tiny_rcon_server_port(27015);
  }

  if (json_resource["tiny_rcon_ftp_server_username"].exists()) {
    data_line = json_resource["tiny_rcon_ftp_server_username"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_tiny_rcon_ftp_server_username(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_tiny_rcon_ftp_server_username("tinyrcon");
  }

  if (json_resource["tiny_rcon_ftp_server_password"].exists()) {
    data_line = json_resource["tiny_rcon_ftp_server_password"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_tiny_rcon_ftp_server_password(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_tiny_rcon_ftp_server_password("08021980");
  }

  if (json_resource["codmp_exe_path"].exists()) {
    data_line = json_resource["codmp_exe_path"].as_str();
    replace_forward_slash_with_backward_slash(data_line);
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_codmp_exe_path(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_codmp_exe_path("");
  }


  if (!check_if_file_path_exists(main_app.get_codmp_exe_path().c_str())) {
    main_app.set_codmp_exe_path("");
  }

  if (json_resource["cod2mp_s_exe_path"].exists()) {
    data_line = json_resource["cod2mp_s_exe_path"].as_str();
    replace_forward_slash_with_backward_slash(data_line);
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_cod2mp_exe_path(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_cod2mp_exe_path("");
  }


  if (!check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str())) {
    main_app.set_cod2mp_exe_path("");
  }

  if (json_resource["iw3mp_exe_path"].exists()) {
    data_line = json_resource["iw3mp_exe_path"].as_str();
    replace_forward_slash_with_backward_slash(data_line);
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_iw3mp_exe_path(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_iw3mp_exe_path("");
  }


  if (!check_if_file_path_exists(main_app.get_iw3mp_exe_path().c_str())) {
    main_app.set_iw3mp_exe_path("");
  }

  if (json_resource["cod5mp_exe_path"].exists()) {
    data_line = json_resource["cod5mp_exe_path"].as_str();
    replace_forward_slash_with_backward_slash(data_line);
    strip_leading_and_trailing_quotes(data_line);
    main_app.set_cod5mp_exe_path(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_cod5mp_exe_path("");
  }


  if (!check_if_file_path_exists(main_app.get_cod5mp_exe_path().c_str())) {
    main_app.set_cod5mp_exe_path("");
  }

  if (json_resource["enable_automatic_connection_flood_ip_ban"].exists()) {

    main_app.get_game_server().set_is_enable_automatic_connection_flood_ip_ban(json_resource["enable_automatic_connection_flood_ip_ban"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_is_enable_automatic_connection_flood_ip_ban(false);
  }

  if (json_resource["minimum_number_of_connections_from_same_ip_for_automatic_ban"].exists()) {
    main_app.get_game_server().set_minimum_number_of_connections_from_same_ip_for_automatic_ban(json_resource["minimum_number_of_connections_from_same_ip_for_automatic_ban"].as<int>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_minimum_number_of_connections_from_same_ip_for_automatic_ban(5);
  }

  if (json_resource["number_of_warnings_for_automatic_kick"].exists()) {

    main_app.get_game_server().set_maximum_number_of_warnings_for_automatic_kick(json_resource["number_of_warnings_for_automatic_kick"].as<int>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_maximum_number_of_warnings_for_automatic_kick(2);
  }

  if (json_resource["disable_automatic_kick_messages"].exists()) {
    main_app.get_game_server().set_is_disable_automatic_kick_messages(json_resource["disable_automatic_kick_messages"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_is_disable_automatic_kick_messages(false);
  }

  if (json_resource["use_original_admin_messages"].exists()) {
    main_app.get_game_server().set_is_use_original_admin_messages(json_resource["use_original_admin_messages"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_is_use_original_admin_messages(true);
  }

  if (json_resource["user_defined_warn_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_warn_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_warn_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_warn_msg"] = "^7{PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_warn_msg"] = "^7{PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_kick_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_kick_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_kick_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_kick_msg"] = "^7{PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_kick_msg"] = "^7{PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_temp_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_temp_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_temp_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_temp_ban_msg"] = "^7{PLAYERNAME} ^7you are being ^1temporarily banned ^7for ^1{TEMPBAN_DURATION} hours ^7by ^1admin {ADMINNAME}.{{br}}^3Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_temp_ban_msg"] = "^7{PLAYERNAME} ^7you are being ^1temporarily banned ^7for ^1{TEMPBAN_DURATION} hours ^7by ^1admin {ADMINNAME}.{{br}}^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_ban_msg"] = "^7{PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_ban_msg"] = "^7{PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_ip_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_ip_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_ip_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_ip_ban_msg"] = "^7{PLAYERNAME} ^1you are being permanently banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_ip_ban_msg"] = "^7{PLAYERNAME} ^1you are being permanently banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_city_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_city_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_city_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_city_ban_msg"] = "^7Admin ^5{ADMINNAME} ^1has globally banned city: ^5{CITY_NAME}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_city_ban_msg"] = "^7Admin ^5{ADMINNAME} ^1has globally banned city: ^5{CITY_NAME}";
    ;
  }

  if (json_resource["user_defined_city_unban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_city_unban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_city_unban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_city_unban_msg"] = "^7Admin ^5{ADMINNAME} ^1has removed previously banned city: ^5{CITY_NAME}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_city_unban_msg"] = "^7Admin ^5{ADMINNAME} ^1has removed previously banned city: ^5{CITY_NAME}";
  }


  if (json_resource["user_defined_enable_city_ban_feature_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_enable_city_ban_feature_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] = "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned cities.";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] = "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned cities.";
  }

  if (json_resource["user_defined_disable_city_ban_feature_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_disable_city_ban_feature_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] = "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned cities.";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] = "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned cities.";
  }

  if (json_resource["user_defined_country_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_country_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_country_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_country_ban_msg"] = "^7Admin ^5{ADMINNAME} ^1has globally banned country: ^5{COUNTRY_NAME}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_country_ban_msg"] = "^7Admin ^5{ADMINNAME} ^1has globally banned country: ^5{COUNTRY_NAME}";
    ;
  }

  if (json_resource["user_defined_country_unban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_country_unban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_country_unban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_country_unban_msg"] = "^7Admin ^5{ADMINNAME} ^1has removed previously banned country: ^5{COUNTRY_NAME}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_country_unban_msg"] = "^7Admin ^5{ADMINNAME} ^1has removed previously banned country: ^5{COUNTRY_NAME}";
  }


  if (json_resource["user_defined_enable_country_ban_feature_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_enable_country_ban_feature_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] = "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned countries.";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] = "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned countries.";
  }

  if (json_resource["user_defined_disable_country_ban_feature_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_disable_country_ban_feature_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] = "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned countries.";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] = "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players with ^1IP addresses ^7from banned countries.";
  }

  if (json_resource["automatic_remove_temp_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["automatic_remove_temp_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] = "^1{BANNED_BY}: ^7{PLAYERNAME}'s ^1temporary ban ^7[start date: ^3{TEMP_BAN_START_DATE} ^7expired on ^3{TEMP_BAN_END_DATE}]{{br}}^7has automatically been removed. ^5Reason of ban: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] = "^1{BANNED_BY}: ^7{PLAYERNAME}'s ^1temporary ban ^7[start date: ^3{TEMP_BAN_START_DATE} ^7expired on ^3{TEMP_BAN_END_DATE}]{{br}}^7has automatically been removed. ^5Reason of ban: ^1{REASON}";
  }

  if (json_resource["automatic_kick_temp_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["automatic_kick_temp_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] = "^1{BANNED_BY}: ^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}}^7Your temporary ban expires in ^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{TEMP_BAN_START_DATE}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] = "^1{BANNED_BY}: ^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}}^7Your temporary ban expires in ^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{TEMP_BAN_START_DATE}";
  }

  if (json_resource["automatic_kick_ip_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["automatic_kick_ip_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] = "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] = "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}";
  }

  if (json_resource["automatic_kick_city_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["automatic_kick_city_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["automatic_kick_city_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["automatic_kick_city_ban_msg"] = "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an IP address from a ^1banned city: ^5{CITY_NAME} ^7is being automatically ^1kicked.";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_kick_city_ban_msg"] = "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an IP address from a ^1banned city: ^5{CITY_NAME} ^7is being automatically ^1kicked.";
  }

  if (json_resource["automatic_kick_country_ban_msg"].exists()) {
    if (!main_app.get_game_server().get_is_use_original_admin_messages()) {
      data_line = json_resource["automatic_kick_country_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["automatic_kick_country_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["automatic_kick_country_ban_msg"] = "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an IP address from a ^1banned country:  ^5{COUNTRY_NAME} ^7is being automatically ^1kicked.";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_kick_country_ban_msg"] = "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an IP address from a ^1banned country:  ^5{COUNTRY_NAME} ^7is being automatically ^1kicked.";
  }

  if (json_resource["current_match_info"].exists()) {
    data_line = json_resource["current_match_info"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_current_match_info(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_current_match_info("^3Map: {MAP_FULL_NAME} ^1({MAP_RCON_NAME}^1) ^3| Gametype: {GAMETYPE_FULL_NAME} ^3| Online/Offline players: {ONLINE_PLAYERS_COUNT}^3|{OFFLINE_PLAYERS_COUNT}");
  }

  if (json_resource["use_different_background_colors_for_even_and_odd_lines"].exists()) {
    main_app.get_game_server().set_is_use_different_background_colors_for_even_and_odd_lines(json_resource["use_different_background_colors_for_even_and_odd_lines"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_is_use_different_background_colors_for_even_and_odd_lines(true);
  }

  if (json_resource["odd_player_data_lines_bg_color"].exists()) {
    data_line = json_resource["odd_player_data_lines_bg_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_odd_player_data_lines_bg_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_odd_player_data_lines_bg_color("^0");
  }

  if (json_resource["even_player_data_lines_bg_color"].exists()) {
    data_line = json_resource["even_player_data_lines_bg_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_even_player_data_lines_bg_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_even_player_data_lines_bg_color("^8");
  }

  if (json_resource["use_different_foreground_colors_for_even_and_odd_lines"].exists()) {
    main_app.get_game_server().set_is_use_different_foreground_colors_for_even_and_odd_lines(json_resource["use_different_foreground_colors_for_even_and_odd_lines"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_is_use_different_foreground_colors_for_even_and_odd_lines(false);
  }

  if (json_resource["odd_player_data_lines_fg_color"].exists()) {
    data_line = json_resource["odd_player_data_lines_fg_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_odd_player_data_lines_fg_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_odd_player_data_lines_fg_color("^5");
  }

  if (json_resource["even_player_data_lines_fg_color"].exists()) {
    data_line = json_resource["even_player_data_lines_fg_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_even_player_data_lines_fg_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_even_player_data_lines_fg_color("^3");
  }


  if (json_resource["full_map_name_color"].exists()) {

    data_line = json_resource["full_map_name_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_full_map_name_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_full_map_name_color("^2");
  }

  if (json_resource["rcon_map_name_color"].exists()) {
    data_line = json_resource["rcon_map_name_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_rcon_map_name_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_rcon_map_name_color("^1");
  }

  if (json_resource["full_game_type_color"].exists()) {
    data_line = json_resource["full_game_type_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_full_gametype_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_full_gametype_color("^2");
  }

  if (json_resource["rcon_game_type_color"].exists()) {
    data_line = json_resource["rcon_game_type_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_rcon_gametype_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_rcon_gametype_color("^1");
  }

  if (json_resource["online_players_count_color"].exists()) {
    data_line = json_resource["online_players_count_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_online_players_count_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_online_players_count_color("^2");
  }

  if (json_resource["offline_players_count_color"].exists()) {
    data_line = json_resource["offline_players_count_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_offline_players_count_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_offline_players_count_color("^1");
  }

  if (json_resource["border_line_color"].exists()) {
    data_line = json_resource["border_line_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_border_line_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_border_line_color("^5");
  }

  if (json_resource["header_player_pid_color"].exists()) {
    data_line = json_resource["header_player_pid_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_pid_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_pid_color("^1");
  }

  if (json_resource["data_player_pid_color"].exists()) {
    data_line = json_resource["data_player_pid_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_data_player_pid_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_data_player_pid_color("^1");
  }

  if (json_resource["header_player_score_color"].exists()) {
    data_line = json_resource["header_player_score_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_score_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_score_color("^3");
  }

  if (json_resource["data_player_score_color"].exists()) {
    data_line = json_resource["data_player_score_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_data_player_score_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_data_player_score_color("^3");
  }

  if (json_resource["header_player_ping_color"].exists()) {
    data_line = json_resource["header_player_ping_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_ping_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_ping_color("^3");
  }

  if (json_resource["data_player_ping_color"].exists()) {
    data_line = json_resource["data_player_ping_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_data_player_ping_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_data_player_ping_color("^3");
  }

  if (json_resource["header_player_name_color"].exists()) {
    data_line = json_resource["header_player_name_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_name_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_name_color("^3");
  }

  if (json_resource["header_player_ip_color"].exists()) {
    data_line = json_resource["header_player_ip_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_ip_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_ip_color("^3");
  }
  if (json_resource["data_player_ip_color"].exists()) {
    data_line = json_resource["data_player_ip_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_data_player_ip_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_data_player_ip_color("^3");
  }

  if (json_resource["header_player_geoinfo_color"].exists()) {
    data_line = json_resource["header_player_geoinfo_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_geoinfo_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_geoinfo_color("^3");
  }

  if (json_resource["data_player_geoinfo_color"].exists()) {
    data_line = json_resource["data_player_geoinfo_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_data_player_geoinfo_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_data_player_geoinfo_color("^3");
  }

  if (json_resource["ftp_download_site_ip_address"].exists()) {
    data_line = json_resource["ftp_download_site_ip_address"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    trim_in_place(data_line, "/\\ \t\n\f\v");
    if (data_line.starts_with("fttp://") || data_line.starts_with("http://")) {
      data_line.erase(0, 7);
    }
    main_app.set_ftp_download_site_ip_address(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_ftp_download_site_ip_address("85.222.189.119");
  }

  if (json_resource["ftp_download_folder_path"].exists()) {
    data_line = json_resource["ftp_download_folder_path"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    trim_in_place(data_line, "/\\ \t\n\f\v");
    main_app.set_ftp_download_folder_path(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_ftp_download_folder_path("tinyrcon");
  }

  if (json_resource["ftp_download_file_pattern"].exists()) {
    data_line = json_resource["ftp_download_file_pattern"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    trim_in_place(data_line);
    main_app.set_ftp_download_file_pattern(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_ftp_download_file_pattern("^_U_TinyRcon[\\._-]?v?(\\d{1,2}\\.\\d{1,2}\\.\\d{1,2}\\.\\d{1,2})\\.exe$");
  }

  if (json_resource["ftp_bans_folder_path"].exists()) {
    data_line = json_resource["ftp_bans_folder_path"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    trim_in_place(data_line, "/\\ \t\n\f\v");
    main_app.set_ftp_bans_folder_path(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_ftp_bans_folder_path("C:\\tinyrcon\\bans");
  }

  if (json_resource["plugins_geoIP_geo_dat_md5"].exists()) {
    data_line = json_resource["plugins_geoIP_geo_dat_md5"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    trim_in_place(data_line);
    main_app.set_plugins_geoIP_geo_dat_md5(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.set_plugins_geoIP_geo_dat_md5("");
  }

  if (json_resource["is_automatic_country_kick_enabled"].exists()) {
    main_app.get_game_server().set_is_automatic_country_kick_enabled(json_resource["is_automatic_country_kick_enabled"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_is_automatic_country_kick_enabled(false);
  }

  auto &banned_countries = main_app.get_game_server().get_banned_countries_set();

  ifstream input_file1(banned_countries_list_file_path, std::ios::in);
  if (input_file1) {
    for (string banned_country; getline(input_file1, banned_country);) {
      trim_in_place(banned_country);
      banned_countries.emplace(std::move(banned_country));
    }
  }

  if (json_resource["is_automatic_city_kick_enabled"].exists()) {
    main_app.get_game_server().set_is_automatic_city_kick_enabled(json_resource["is_automatic_city_kick_enabled"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_is_automatic_city_kick_enabled(false);
  }

  auto &banned_cities = main_app.get_game_server().get_banned_cities_set();

  ifstream input_file2(banned_cities_list_file_path, std::ios::in);
  if (input_file2) {
    for (string banned_city; getline(input_file2, banned_city);) {
      trim_in_place(banned_city);
      banned_cities.emplace(std::move(banned_city));
    }
  }


  if (found_missing_config_setting) {
    write_tiny_rcon_json_settings_to_file(configFileName);
  }
}

void parse_tinyrcon_server_users_data(const char *file_path)
{
  const string users_file_path{ format("{}{}", main_app.get_current_working_directory(), file_path) };
  ifstream configFile{ users_file_path };

  if (!configFile) {
    save_tiny_rcon_users_data_to_json_file(users_file_path.c_str());
    configFile.open(file_path);
  }

  if (!configFile) return;

  for (string data; std::getline(configFile, data);) {
    trim_in_place(data);
    if (data.empty()) continue;
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    if (parts.size() >= 17) {
      for (auto &part : parts)
        stl::helper::trim_in_place(part);

      const auto &u = main_app.get_user_for_name(parts[0], parts[4]);
      u->user_name = std::move(parts[0]);
      u->is_admin = parts[1] == "true";
      u->is_logged_in = parts[2] == "true";
      u->is_online = parts[3] == "true";
      unsigned long guid{};
      if (check_ip_address_validity(parts[4], guid)) {
        player admin{};
        convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), parts[4], admin);
        u->ip_address = std::move(parts[4]);
        u->country_code = admin.country_code;
        u->geo_information = format("{}, {}", admin.country_name, admin.city);
      } else {
        u->ip_address = "n/a";
        u->geo_information = "n/a";
        u->country_code = "xy";
      }

      u->last_login_time_stamp = stoll(parts[6]);
      u->last_logout_time_stamp = stoll(parts[7]);
      u->no_of_logins += stoul(parts[8]);
      u->no_of_warnings += stoul(parts[9]);
      u->no_of_kicks += stoul(parts[10]);
      u->no_of_tempbans += stoul(parts[11]);
      u->no_of_guidbans += stoul(parts[12]);
      u->no_of_ipbans += stoul(parts[13]);
      u->no_of_iprangebans += stoul(parts[14]);
      u->no_of_citybans += stoul(parts[15]);
      u->no_of_countrybans += stoul(parts[16]);
    }
  }
}

void parse_tempbans_data_file(const char *file_path, std::vector<player> &temp_banned_players, std::unordered_map<std::string, player> &ip_to_temp_banned_player, const bool is_skip_removed_check)
{
  string property_key, property_value;
  // lock_guard lg{ protect_banned_players_data };
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 1024 };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "^3Couldn't open file at specified path ^1("s + file_path + ")^3! "s + buffer
    };
    // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
      // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
      print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    }
  } else {
    ip_to_temp_banned_player.clear();
    temp_banned_players.clear();
    const auto &removed_temp_bans_map = main_app.get_game_server().get_removed_temp_banned_players_map();
    string readData, information_about_protected_player;
    while (getline(input_file, readData)) {
      stl::helper::trim_in_place(readData, " \t\n");
      vector<string> parts{ stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no) };
      for (auto &part : parts)
        stl::helper::trim_in_place(part, " \t\n");
      if (parts.size() < 6)
        continue;
      if ((is_skip_removed_check || !removed_temp_bans_map.contains(parts[0])) && !ip_to_temp_banned_player.contains(parts[0])) {
        player temp_banned_player_data{};
        strcpy_s(temp_banned_player_data.ip_address, std::size(temp_banned_player_data.ip_address), parts[0].c_str());
        strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name), parts[1].c_str());
        // strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time), parts[2].c_str());
        temp_banned_player_data.banned_start_time = stoll(parts[3]);
        const string converted_ban_date_and_time_info{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", temp_banned_player_data.banned_start_time) };
        strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time), converted_ban_date_and_time_info.c_str());
        temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
        temp_banned_player_data.reason = std::move(parts[5]);
        temp_banned_player_data.banned_by_user_name = (parts.size() >= 7) ? parts[6] : main_app.get_username();
        convert_guid_key_to_country_name(
          main_app.get_connection_manager_for_messages().get_geoip_data(),
          temp_banned_player_data.ip_address,
          temp_banned_player_data);
        if (check_if_player_is_protected(temp_banned_player_data, "!tb", information_about_protected_player)) {
          print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
          continue;
        }
        ip_to_temp_banned_player.emplace(temp_banned_player_data.ip_address, temp_banned_player_data);
        temp_banned_players.push_back(std::move(temp_banned_player_data));
      }
    }
  }
}

void parse_banned_ip_addresses_file(const char *file_path, std::vector<player> &banned_players, std::unordered_map<std::string, player> &ip_to_banned_player, const bool is_skip_removed_check)
{
  string property_key, property_value;
  // lock_guard lg{ protect_banned_players_data };
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 1024 };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "^3Couldn't open file at specified path ^1("s + file_path + ")^3! "s + buffer
    };
    // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
      // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
      print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    }
  } else {
    ip_to_banned_player.clear();
    banned_players.clear();
    const auto &removed_ip_bans_map = main_app.get_game_server().get_removed_banned_ip_addresses_map();
    string readData, information_about_protected_player;
    while (getline(input_file, readData)) {
      stl::helper::trim_in_place(readData);
      vector<string> parts = stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
      for (auto &part : parts)
        stl::helper::trim_in_place(part);
      if (parts.size() < 5)
        continue;

      if ((is_skip_removed_check || !removed_ip_bans_map.contains(parts[0])) && !ip_to_banned_player.contains(parts[0])) {
        player bannedPlayerData{};
        strcpy_s(bannedPlayerData.ip_address, std::size(bannedPlayerData.ip_address), parts[0].c_str());
        strcpy_s(bannedPlayerData.guid_key, std::size(bannedPlayerData.guid_key), parts[1].c_str());
        strcpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[2].c_str());
        // strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time), parts[3].c_str());
        bannedPlayerData.banned_start_time = get_number_of_seconds_from_date_and_time_string(parts[3]);
        const string converted_ban_date_and_time_info{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bannedPlayerData.banned_start_time) };
        strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time), converted_ban_date_and_time_info.c_str());
        bannedPlayerData.reason = std::move(parts[4]);
        bannedPlayerData.banned_by_user_name = (parts.size() >= 6) ? parts[5] : main_app.get_username();
        convert_guid_key_to_country_name(
          main_app.get_connection_manager_for_messages().get_geoip_data(),
          bannedPlayerData.ip_address,
          bannedPlayerData);
        if (check_if_player_is_protected(bannedPlayerData, "!gb", information_about_protected_player)) {
          print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
          continue;
        }
        ip_to_banned_player.emplace(bannedPlayerData.ip_address, bannedPlayerData);
        banned_players.push_back(std::move(bannedPlayerData));
      }
    }
    sort(begin(banned_players), end(banned_players), [](const player &p1, const player &p2) {
      return p1.banned_start_time < p2.banned_start_time;
    });
  }
}

void parse_protected_entries_file(const char *file_path, std::set<std::string> &protected_entries)
{
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 1024 };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "^3Couldn't open file at specified path ^1("s + file_path + ")^3! "s + buffer
    };
    // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
      // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
      print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    }
  } else {
    protected_entries.clear();
    string readData;
    while (getline(input_file, readData)) {
      stl::helper::trim_in_place(readData);
      string entry;
      for (const auto ch : readData) {
        if (isprint(ch) || ' ' == ch)
          entry.push_back(ch);
      }
      readData = std::move(entry);
      if (!protected_entries.contains(readData)) {
        protected_entries.emplace(readData);
      }
    }
  }
}

void save_protected_entries_file(const char *file_path, const std::set<std::string> &protected_entries)
{
  ofstream output{ file_path };
  if (output) {

    for (const auto &entry : protected_entries) {
      output << entry << '\n';
    }
    output << flush;
    output.close();
  }
}

void parse_banned_ip_address_ranges_file(const char *file_path, std::vector<player> &banned_ip_address_ranges, std::unordered_map<std::string, player> &ip_address_range_to_banned_player, const bool is_skip_removed_check)
{
  string property_key, property_value;
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 1024 };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "^3Couldn't open file at specified path ^1("s + file_path + ")^3! "s + buffer
    };
    // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
      print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    }
  } else {
    banned_ip_address_ranges.clear();
    ip_address_range_to_banned_player.clear();
    const auto &removed_ip_address_ranges_map = main_app.get_game_server().get_removed_banned_ip_address_ranges_map();
    string readData, information_about_protected_player;
    while (getline(input_file, readData)) {
      stl::helper::trim_in_place(readData);
      vector<string> parts = stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
      for (auto &part : parts)
        stl::helper::trim_in_place(part);
      if (parts.size() < 5)
        continue;
      if ((is_skip_removed_check || !removed_ip_address_ranges_map.contains(parts[0])) && !ip_address_range_to_banned_player.contains(parts[0])) {
        player bannedPlayerData{};
        strcpy_s(bannedPlayerData.guid_key, std::size(bannedPlayerData.guid_key), parts[1].c_str());
        strcpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[2].c_str());
        bannedPlayerData.banned_start_time = get_number_of_seconds_from_date_and_time_string(parts[3]);
        const string converted_ban_date_and_time_info{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bannedPlayerData.banned_start_time) };
        strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time), converted_ban_date_and_time_info.c_str());
        // strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time), parts[3].c_str());
        bannedPlayerData.reason = std::move(parts[4]);
        bannedPlayerData.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : main_app.get_username();
        get_first_valid_ip_address_from_ip_address_range(parts[0], bannedPlayerData);

        if (check_if_player_is_protected(bannedPlayerData, "!br", information_about_protected_player)) {
          print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
          continue;
        }
        strcpy_s(bannedPlayerData.ip_address, std::size(bannedPlayerData.ip_address), parts[0].c_str());
        ip_address_range_to_banned_player.emplace(parts[0], bannedPlayerData);
        banned_ip_address_ranges.push_back(std::move(bannedPlayerData));
      }
    }
    sort(begin(banned_ip_address_ranges), end(banned_ip_address_ranges), [](const player &p1, const player &p2) {
      return p1.banned_start_time < p2.banned_start_time;
    });
  }
}

void parse_banned_cities_file(const char *file_path, std::set<std::string> &banned_cities, const bool is_skip_removed_check)
{
  string property_key, property_value;
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 1024 };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "^3Couldn't open file at specified path ^1("s + file_path + ")^3! "s + buffer
    };
    // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
      // show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
      print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    }
  } else {
    const auto &protected_cities = main_app.get_game_server().get_protected_cities();
    const auto &removed_cities = main_app.get_game_server().get_removed_banned_cities_set();
    banned_cities.clear();
    for (string readData; getline(input_file, readData);) {
      stl::helper::trim_in_place(readData);
      string entry;
      for (const auto ch : readData) {
        if (isprint(ch) || ' ' == ch)
          entry.push_back(ch);
      }
      readData = std::move(entry);
      if (protected_cities.contains(readData)) {
        const string information_about_removed_entry{ format("^3Banned city ^1{} ^3has been protected by an ^1admin^3!\n^5If you want you can ^1unprotect ^5it using the ^1!unprotectcity {} ^5command.", readData, readData) };
        print_colored_text(app_handles.hwnd_re_messages_data, information_about_removed_entry.c_str());
      } else if (!is_skip_removed_check && removed_cities.contains(readData)) {
        const string information_about_removed_entry{ format("^3Banned city ^1{} ^3has been unbanned by an ^1admin^3!\n^5If you want you can ^1ban ^5it again using the ^1!bancity {} ^5command.", readData, readData) };
        print_colored_text(app_handles.hwnd_re_messages_data, information_about_removed_entry.c_str());
      } else {
        banned_cities.emplace(std::move(readData));
      }
    }
  }
}

void parse_banned_countries_file(const char *file_path, std::set<std::string> &banned_countries, const bool is_skip_removed_check)
{
  string property_key, property_value;
  // lock_guard lg{ protect_banned_players_data };
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 256U };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "Couldn't open file at specified path ("s + file_path + ")! "s + buffer
    };
    show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format(
        "Couldn't create file at {}!\nError: {}", file_path, buffer));

      show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    }
  } else {
    const auto &protected_countries = main_app.get_game_server().get_protected_countries();
    const auto &removed_countries = main_app.get_game_server().get_removed_banned_countries_set();
    banned_countries.clear();
    for (string readData; getline(input_file, readData);) {
      stl::helper::trim_in_place(readData);
      string entry;
      for (const auto ch : readData) {
        if (isprint(ch) || ' ' == ch)
          entry.push_back(ch);
      }
      readData = std::move(entry);
      if (protected_countries.contains(readData)) {
        const string information_about_removed_entry{ format("^3Banned country ^1{} ^3has been protected by an ^1admin^3!\n^5If you want you can ^1unprotect ^5it using the ^1!unprotectcountry {} ^5command.", readData, readData) };
        print_colored_text(app_handles.hwnd_re_messages_data, information_about_removed_entry.c_str());
      } else if (!is_skip_removed_check && removed_countries.contains(readData)) {
        const string information_about_removed_entry{ format("^3Banned country ^1{} ^3has been unbanned by an ^1admin^3!\n^5If you want you can ^1ban ^5it again using the ^1!bancountry {} ^5command.", readData, readData) };
        print_colored_text(app_handles.hwnd_re_messages_data, information_about_removed_entry.c_str());
      } else {
        banned_countries.emplace(std::move(readData));
      }
    }
  }
}

void save_tempbans_to_file(const char *file_path, const std::vector<player> &temp_banned_players)
{
  ofstream output{ file_path };
  if (output) {

    for (const auto &tb_player : temp_banned_players) {
      output << tb_player.ip_address << '\\'
             << tb_player.player_name << '\\'
             << tb_player.banned_date_time << '\\'
             << tb_player.banned_start_time << '\\'
             << tb_player.ban_duration_in_hours << '\\'
             << remove_disallowed_character_in_string(tb_player.reason) << '\\'
             << tb_player.banned_by_user_name << '\n';
    }

    output << flush;
  }
}

void save_banned_ip_entries_to_file(const char *file_path, const std::vector<player> &banned_ip_entries)
{
  ofstream output{ file_path };
  if (output) {

    for (const auto &banned_player : banned_ip_entries) {
      output << banned_player.ip_address << '\\'
             << banned_player.guid_key << '\\'
             << banned_player.player_name << '\\'
             << banned_player.banned_date_time << '\\'
             << remove_disallowed_character_in_string(banned_player.reason) << '\\'
             << banned_player.banned_by_user_name << '\n';
    }

    output << flush;
  }
}

void save_banned_ip_address_range_entries_to_file(const char *file_path, const std::vector<player> &banned_ip_address_ranges)
{
  ofstream output{ file_path };

  if (output) {
    for (const auto &banned_player : banned_ip_address_ranges) {
      string ip{ banned_player.ip_address };
      if (!ip.ends_with(".*")) {
        const size_t last_dot_pos{ ip.rfind('.') };
        if (last_dot_pos != string::npos) {
          ip.replace(cbegin(ip) + last_dot_pos, cend(ip), ".*");
        }
      }
      output << ip << '\\'
             << banned_player.guid_key << '\\'
             << banned_player.player_name << '\\'
             << banned_player.banned_date_time << '\\'
             << remove_disallowed_character_in_string(banned_player.reason) << '\\'
             << banned_player.banned_by_user_name << '\n';
    }

    output << flush;
  }
}

void save_banned_cities_to_file(const char *file_path, const std::set<std::string> &banned_cities)
{
  ofstream output{ file_path };

  if (output) {
    for (const auto &banned_city : banned_cities) {
      output << banned_city << '\n';
    }

    output << flush;
  }
}

void save_banned_countries_to_file(const char *file_path, const std::set<std::string> &banned_countries)
{
  ofstream output{ file_path };

  if (output) {
    for (const auto &banned_country : banned_countries) {
      output << banned_country << '\n';
    }

    output << flush;
  }
}

bool temp_ban_player_ip_address(player &pd)
{
  using namespace std::literals;

  if (main_app.get_game_server().get_temp_banned_ip_addresses_map().contains(pd.ip_address))
    return true;

  unsigned long ip_key{};
  if (!check_ip_address_validity(pd.ip_address, ip_key)) {
    return false;
  }

  if (pd.country_name == nullptr || len(pd.country_name) == 0) {
    convert_guid_key_to_country_name(
      main_app.get_connection_manager_for_messages().get_geoip_data(),
      pd.ip_address,
      pd);
  }

  ofstream temp_banned_data_file_to_read{ tempbans_file_path, ios_base::app };
  if (!temp_banned_data_file_to_read) {
    const size_t buffer_size{ 256U };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "Couldn't open 'tempbans.txt' data file at specified path ("s + tempbans_file_path + ")! "s + buffer
    };
    show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    ofstream temp_banned_data_file_to_write{ tempbans_file_path };
    if (!temp_banned_data_file_to_write) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(
        "Couldn't create 'tempbans.txt' file for holding temporarily banned IP addresses at "s
        "data\\tempbans.txt!"s
        + buffer);
      show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
      return false;
    }

    const time_t time_of_ban_in_seconds = std::time(nullptr);
    pd.banned_start_time = time_of_ban_in_seconds;
    const string banned_date_time_str{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}") };
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    temp_banned_data_file_to_write << pd.ip_address << '\\'
                                   << pd.player_name << '\\'
                                   << pd.banned_date_time << '\\'
                                   << time_of_ban_in_seconds << '\\'
                                   << pd.ban_duration_in_hours << '\\'
                                   << remove_disallowed_character_in_string(pd.reason) << '\\'
                                   << main_app.get_username() << endl;
  } else {
    const time_t time_of_ban_in_seconds = std::time(nullptr);
    pd.banned_start_time = time_of_ban_in_seconds;
    const string banned_date_time_str{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}") };
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    temp_banned_data_file_to_read << pd.ip_address << '\\'
                                  << pd.player_name << '\\'
                                  << pd.banned_date_time << '\\'
                                  << time_of_ban_in_seconds << '\\'
                                  << pd.ban_duration_in_hours << '\\'
                                  << remove_disallowed_character_in_string(pd.reason) << '\\'
                                  << main_app.get_username() << endl;
  }

  main_app.get_game_server().add_ip_address_to_temp_banned_ip_addresses(
    pd.ip_address, pd);

  main_app.get_game_server().get_temp_banned_players_data().emplace_back(pd);

  return true;
}

bool global_ban_player_ip_address(player &pd)
{
  using namespace std::literals;

  if (main_app.get_game_server().get_banned_ip_addresses_map().contains(pd.ip_address))
    return true;

  unsigned long ip_key{};
  if (!check_ip_address_validity(pd.ip_address, ip_key)) {
    return false;
  }

  if (pd.country_name == nullptr || len(pd.country_name) == 0) {
    convert_guid_key_to_country_name(
      main_app.get_connection_manager_for_messages().get_geoip_data(),
      pd.ip_address,
      pd);
  }

  ofstream bannedIPsFileToRead{ banned_ip_addresses_file_path, ios_base::app };
  if (!bannedIPsFileToRead) {
    const size_t buffer_size{ 256U };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "Couldn't open banned IP addresses data file at specified path ("s + banned_ip_addresses_file_path + ")! "s + buffer
    };
    show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    ofstream bannedIPsFileToWrite{ banned_ip_addresses_file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(
        "Couldn't create banned IP addresses data file at "s
        "data\\bans.txt!"s
        + buffer);
      show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
      return false;
    }

    const string banned_date_time_str{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}") };
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    bannedIPsFileToWrite << pd.ip_address << '\\'
                         << pd.guid_key << '\\'
                         << pd.player_name << '\\'
                         << pd.banned_date_time << '\\'
                         << pd.reason << endl;
  } else {
    const string banned_date_time_str{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}") };
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    bannedIPsFileToRead << pd.ip_address << '\\'
                        << pd.guid_key << '\\'
                        << pd.player_name << '\\' << pd.banned_date_time << '\\' << pd.reason << endl;
  }

  main_app.get_game_server().add_ip_address_to_banned_ip_addresses(
    pd.ip_address, pd);

  main_app.get_game_server().get_banned_ip_addresses_vector().emplace_back(
    pd);
  return true;
}

bool add_temporarily_banned_ip_address(player &pd, vector<player> &temp_banned_players_data, unordered_map<string, player> &ip_to_temp_banned_player_data)
{
  unsigned long guid_number{};
  if (!check_ip_address_validity(pd.ip_address, guid_number) || ip_to_temp_banned_player_data.contains(pd.ip_address))
    return false;

  if (main_app.get_game_server().get_removed_temp_banned_players_map().contains(pd.ip_address)) {
    main_app.get_game_server().get_removed_temp_banned_players_map().erase(pd.ip_address);
    auto &entries = main_app.get_game_server().get_removed_temp_banned_players_vector();
    entries.erase(std::remove_if(begin(entries), end(entries), [&pd](const player &p) {
      return strcmp(p.ip_address, pd.ip_address) == 0;
    }),
      end(entries));

    save_tempbans_to_file(removed_tempbans_file_path, main_app.get_game_server().get_removed_temp_banned_players_vector());
  }

  ip_to_temp_banned_player_data.emplace(pd.ip_address, pd);
  temp_banned_players_data.push_back(std::move(pd));

  save_tempbans_to_file(tempbans_file_path, temp_banned_players_data);

  return true;
}

std::pair<bool, std::string> remove_temp_banned_ip_address(const std::string &ip_address)
{
  if (!main_app.get_game_server().get_temp_banned_ip_addresses_map().contains(
        ip_address)) {
    return { false, {} };
  }

  auto &temp_banned_players = main_app.get_game_server().get_temp_banned_players_data();

  const auto found_iter = find_if(std::begin(temp_banned_players), std::end(temp_banned_players), [&ip_address](const player &p) {
    return ip_address == p.ip_address;
  });

  if (found_iter != std::end(temp_banned_players)) {

    if (!main_app.get_game_server().get_removed_temp_banned_players_map().contains(ip_address)) {
      main_app.get_game_server().get_removed_temp_banned_players_map().emplace(ip_address, *found_iter);
      main_app.get_game_server().get_removed_temp_banned_players_vector().emplace_back(*found_iter);
      save_tempbans_to_file(removed_tempbans_file_path, main_app.get_game_server().get_removed_temp_banned_players_vector());
    }

    string removed_tempban_player_name{ found_iter->player_name };

    main_app.get_game_server().get_temp_banned_ip_addresses_map().erase(ip_address);

    temp_banned_players.erase(remove_if(std::begin(temp_banned_players), std::end(temp_banned_players), [&ip_address](const player &p) {
      return ip_address == p.ip_address;
    }),
      std::end(temp_banned_players));

    save_tempbans_to_file(tempbans_file_path, temp_banned_players);

    return { true, removed_tempban_player_name };
  }

  return { true, "Unknown Soldier" };
}

bool add_permanently_banned_ip_address(player &pd, vector<player> &banned_players_data, unordered_map<string, player> &ip_to_banned_player_data)
{
  // lock_guard lg{ protect_banned_players_data };
  unsigned long guid_number{};
  if (!check_ip_address_validity(pd.ip_address, guid_number) || ip_to_banned_player_data.contains(pd.ip_address))
    return false;

  if (main_app.get_game_server().get_removed_banned_ip_addresses_map().contains(pd.ip_address)) {
    main_app.get_game_server().get_removed_banned_ip_addresses_map().erase(pd.ip_address);
    auto &entries = main_app.get_game_server().get_removed_banned_ip_addresses_vector();
    entries.erase(std::remove_if(begin(entries), end(entries), [&pd](const player &p) {
      return strcmp(p.ip_address, pd.ip_address) == 0;
    }),
      end(entries));
    save_banned_ip_entries_to_file(removed_banned_ip_addresses_file_path, main_app.get_game_server().get_removed_banned_ip_addresses_vector());
  }

  ip_to_banned_player_data.emplace(pd.ip_address, pd);
  banned_players_data.push_back(std::move(pd));

  save_banned_ip_entries_to_file(banned_ip_addresses_file_path, banned_players_data);

  return true;
}

bool add_permanently_banned_ip_address_range(player &pd, std::vector<player> &banned_ip_address_ranges_vector, std::unordered_map<std::string, player> &banned_ip_address_ranges_map)
{
  // lock_guard lg{ protect_banned_players_data };
  const string banned_ip_range{ pd.ip_address };
  if (!banned_ip_range.ends_with(".*.*") && !banned_ip_range.ends_with(".*"))
    return false;
  if (banned_ip_address_ranges_map.contains(banned_ip_range))
    return false;

  if (main_app.get_game_server().get_removed_banned_ip_address_ranges_map().contains(banned_ip_range)) {
    main_app.get_game_server().get_removed_banned_ip_address_ranges_map().erase(banned_ip_range);
    auto &entries = main_app.get_game_server().get_removed_banned_ip_address_ranges_vector();
    entries.erase(std::remove_if(begin(entries), end(entries), [&banned_ip_range](const player &p) {
      return banned_ip_range == p.ip_address;
    }),
      end(entries));
    save_banned_ip_address_range_entries_to_file(removed_ip_range_bans_file_path, entries);
  }

  banned_ip_address_ranges_map.emplace(banned_ip_range, pd);
  banned_ip_address_ranges_vector.push_back(std::move(pd));

  save_banned_ip_address_range_entries_to_file(ip_range_bans_file_path, banned_ip_address_ranges_vector);

  return true;
}

bool remove_permanently_banned_ip_address_range(player &pd, std::vector<player> &banned_ip_address_ranges_vector, std::unordered_map<std::string, player> &banned_ip_address_ranges_map)
{
  // lock_guard lg{ protect_banned_players_data };
  const string banned_ip_range{ pd.ip_address };
  if (!banned_ip_range.ends_with(".*.*") && !banned_ip_range.ends_with(".*"))
    return false;
  if (!banned_ip_address_ranges_map.contains(banned_ip_range))
    return false;

  if (!main_app.get_game_server().get_removed_banned_ip_address_ranges_map().contains(banned_ip_range)) {
    main_app.get_game_server().get_removed_banned_ip_address_ranges_map().emplace(banned_ip_range, pd);
    main_app.get_game_server().get_removed_banned_ip_address_ranges_vector().emplace_back(pd);
    save_banned_ip_address_range_entries_to_file(removed_ip_range_bans_file_path, main_app.get_game_server().get_removed_banned_ip_address_ranges_vector());
  }

  banned_ip_address_ranges_vector.erase(remove_if(std::begin(banned_ip_address_ranges_vector), std::end(banned_ip_address_ranges_vector), [&pd](const player &p) {
    return strcmp(pd.ip_address, p.ip_address) == 0;
  }),
    std::end(banned_ip_address_ranges_vector));

  pd = std::move(banned_ip_address_ranges_map.at(pd.ip_address));
  banned_ip_address_ranges_map.erase(pd.ip_address);

  save_banned_ip_address_range_entries_to_file(ip_range_bans_file_path, banned_ip_address_ranges_vector);

  return true;
}

bool add_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities)
{
  // lock_guard lg{ protect_banned_players_data };
  if (banned_cities.contains(city))
    return false;

  banned_cities.emplace(city);

  if (main_app.get_game_server().get_removed_banned_cities_set().contains(city)) {
    main_app.get_game_server().get_removed_banned_cities_set().erase(city);
    save_banned_cities_to_file(removed_banned_cities_list_file_path, main_app.get_game_server().get_removed_banned_cities_set());
  }

  save_banned_cities_to_file(banned_cities_list_file_path, banned_cities);

  return true;
}

bool remove_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities)
{
  // lock_guard lg{ protect_banned_players_data };
  if (!banned_cities.contains(city))
    return false;

  banned_cities.erase(city);

  if (!main_app.get_game_server().get_removed_banned_cities_set().contains(city)) {
    main_app.get_game_server().get_removed_banned_cities_set().emplace(city);
    save_banned_cities_to_file(removed_banned_cities_list_file_path, main_app.get_game_server().get_removed_banned_cities_set());
  }

  save_banned_cities_to_file(banned_cities_list_file_path, banned_cities);

  return true;
}

bool add_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries)
{
  // lock_guard lg{ protect_banned_players_data };
  if (banned_countries.contains(country))
    return false;

  banned_countries.emplace(country);

  if (main_app.get_game_server().get_removed_banned_countries_set().contains(country)) {
    main_app.get_game_server().get_removed_banned_countries_set().erase(country);
    save_banned_countries_to_file(removed_banned_countries_list_file_path, main_app.get_game_server().get_removed_banned_countries_set());
  }

  save_banned_cities_to_file(banned_countries_list_file_path, banned_countries);

  return true;
}

bool remove_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries)
{
  // lock_guard lg{ protect_banned_players_data };
  if (!banned_countries.contains(country))
    return false;

  banned_countries.erase(country);

  if (!main_app.get_game_server().get_removed_banned_countries_set().contains(country)) {
    main_app.get_game_server().get_removed_banned_countries_set().emplace(country);
    save_banned_countries_to_file(removed_banned_countries_list_file_path, main_app.get_game_server().get_removed_banned_countries_set());
  }

  save_banned_cities_to_file(banned_countries_list_file_path, banned_countries);

  return true;
}

std::pair<bool, std::string> remove_permanently_banned_ip_address(std::string &ip_address)
{
  int number{};
  const size_t no{ is_valid_decimal_whole_number(ip_address, number) ? static_cast<size_t>(number - 1) : std::string::npos };

  // lock_guard lg{ protect_banned_players_data };

  auto &banned_players = main_app.get_game_server().get_banned_ip_addresses_vector();

  if (!main_app.get_game_server().get_banned_ip_addresses_map().contains(
        ip_address)
      && no >= banned_players.size()) {
    return { false, {} };
  }

  if (no < banned_players.size()) {
    ip_address = banned_players[no].ip_address;
  }

  const auto found_iter = find_if(std::begin(banned_players), std::end(banned_players), [&ip_address](const player &p) {
    return ip_address == p.ip_address;
  });

  if (found_iter != std::end(banned_players)) {

    string removed_ipban_player_name{ found_iter->player_name };
    if (!main_app.get_game_server().get_removed_banned_ip_addresses_map().contains(ip_address)) {
      main_app.get_game_server().get_removed_banned_ip_addresses_map().emplace(ip_address, *found_iter);
      main_app.get_game_server().get_removed_banned_ip_addresses_vector().emplace_back(*found_iter);
      save_banned_ip_entries_to_file(removed_banned_ip_addresses_file_path, main_app.get_game_server().get_removed_banned_ip_addresses_vector());
    }

    banned_players.erase(remove_if(std::begin(banned_players), std::end(banned_players), [&ip_address](const player &p) {
      return ip_address == p.ip_address;
    }),
      std::end(banned_players));

    main_app.get_game_server().get_banned_ip_addresses_map().erase(ip_address);

    save_banned_ip_entries_to_file(banned_ip_addresses_file_path, banned_players);

    return { true, std::move(removed_ipban_player_name) };
  }

  return { true, "Unknown Soldier" };
}

bool is_valid_decimal_whole_number(const std::string &str, int &number)
{
  if (str.empty()) return false;

  string ex_msg{ format(R"(^1Exception ^3thrown from ^1bool is_valid_decimal_whole_number("{}", "{}"))", str, number) };
  stack_trace_element ste{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };

  const bool is_negative{ '-' == str[0] };
  size_t index{ is_negative || '+' == str[0] ? 1U : 0U };
  number = 0;
  for (; index < str.length(); ++index) {
    if (!isdigit(str[index]))
      return false;
    number *= 10;
    number += static_cast<int>(str[index] - '0');
  }
  if (is_negative)
    number = -number;

  return true;

  // try {
  //   number = stoi(str);
  // } catch (const std::invalid_argument &) {
  //   return false;
  // } catch (const std::out_of_range &) {
  //   return false;
  // } catch (const std::exception &) {
  //   return false;
  // } catch (...) {
  //   return false;
  // }

  // return true;
}

size_t get_number_of_characters_without_color_codes(const char *text) noexcept
{
  size_t printed_chars_count{};
  const size_t text_len = len(text);
  const char *last = text + text_len;
  for (; *text; ++text) {
    if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') && (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3)) {
      text += 3;
      printed_chars_count += 2;
    } else if (text + 2 <= last && *text == '^' && (*(text + 1) >= '0' && *(text + 1) <= '9')) {
      ++text;
    } else {
      ++printed_chars_count;
    }
  }

  return printed_chars_count;
}

bool get_user_input()
{
  static constexpr size_t buffer_size{ 2048 };
  static char user_command[buffer_size];
  lock_guard lg{ protect_get_user_input };
  GetWindowText(app_handles.hwnd_e_user_input, user_command, buffer_size);
  string user_input{ user_command };
  process_user_input(user_input);
  SetWindowText(app_handles.hwnd_e_user_input, "");
  return should_program_terminate(user_input);
}

void print_help_information(const std::vector<std::string> &input_parts)
{
  if (input_parts.size() >= 2 && (input_parts[0] == "!list" || input_parts[0] == "list" || input_parts[0] == "!" || input_parts[0] == "!help" || input_parts[0] == "help" || input_parts[0] == "!h" || input_parts[0] == "h") && str_starts_with(input_parts[1], "user", true)) {
    const string help_message{
      R"(
 ^5You can use the following user commands:
 ^1!cls ^3-> clears the screen.
 ^1!colors ^5-> change colors of various displayed table entries and game information.
 ^1!c [IP:PORT] ^5-> launches your Call of Duty game and connect to currently configured game server 
 or optionally specified game server address ^1[IP:PORT]^5.
 ^1!cp [IP:PORT] ^5-> launches your Call of Duty game and connect to currently configured game server 
 or optionally specified game server address ^1[IP:PORT] ^5using a private slot.
 ^1!warn player_pid optional_reason ^3-> warns the player whose pid number is equal to specified player_pid.
  A player who's been warned 2 times gets automatically kicked off the server.
 ^1!w player_pid optional_reason ^5-> warns the player whose pid number is equal to specified player_pid.
  A player who's been warned 2 times gets automatically kicked off the server.
 ^1!kick player_pid optional_reason ^3-> kicks the player whose pid number is equal to specified player_pid.
 ^1!k player_pid optional_reason ^5-> kicks the player whose pid number is equal to specified player_pid.
 ^1!tempban player_pid time_duration reason ^3-> temporarily bans (for time_duration hours, default 24 hours) IP address 
    of player whose pid = ^112.
 ^1!tb player_pid time_duration reason ^5-> temporarily bans (for time_duration hours, default 24 hours) IP address 
    of player whose pid = ^112.
 ^1!ban player_pid optional_reason ^3-> bans the player whose pid number is equal to specified player_pid.
 ^1!b player_pid optional_reason ^5-> bans the player whose pid number is equal to specified player_pid.
 ^1!gb player_pid optional_reason ^3-> permanently bans player's IP address whose pid number is equal to specified player_pid.
 ^1!globalban player_pid optional_reason ^5-> permanently bans player's IP address whose pid number is equal to specified player_pid.
 ^1!status ^3-> retrieves current game state (players' data) from the server (pid, score, ping, name, guid, ip, qport, rate
 ^1!s ^5-> retrieves current game state (players' data) from the server (pid, score, ping, name, guid, ip, qport, rate
 ^1!time ^3-> prints current date/time information on the console.
 ^1!t ^5-> prints current date/time information on the console.
 ^1!sort player_data asc|desc ^3-> !sorts players data (for example: !sort pid asc, !sort pid desc, !sort score asc,
   !sort score desc, !sort ping asc, !sort ping desc, !sort name asc, !sort name desc, !sort ip asc, !sort ip desc,
   !sort geo asc, !sort geo desc
 ^1!list user|rcon ^5-> shows a list of available user or rcon level commands that are available for admin to type
                    into the console program.
 ^1!bans ^3-> displays a list of permanently banned IP addresses.
 ^1!tempbans ^5-> displays a list of temporarily banned IP addresses.
 ^1!banip pid|valid_ip_address optional_reason ^3-> bans player whose 'pid' number or 'ip_address' is equal to specified 'pid' 
    or 'valid_ip_address', respectively.
 ^1!addip pid|valid_ip_address optional_reason ^5-> bans player whose 'pid' number or 'ip_address' is equal to specified 'pid' 
    or 'valid_ip_address', respectively.
 ^1!ub valid_ip_address optional_reason ^3-> removes temporarily and|or permanently banned IP address 'valid_ip_address'.
 ^1!unban valid_ip_address optional_reason ^5-> removes temporarily and|or permanently banned IP address 'valid_ip_address'.
 ^1!m map_name game_type ^3-> loads map 'map_name' in specified game mode 'game_type', example: !m mp_toujane hq
 ^1!map map_name game_type ^5-> loads map 'map_name' in specified game mode 'game_type', example: !map mp_burgundy ctf
 ^1!maps ^5-> displays all available playable maps
 ^1!rt time_period ^3-> sets time period (automatic checking for banned players) to time_period (1-30 seconds). 
 ^1!config rcon abc123 ^5-> changes currently used ^1rcon_password ^5to ^1abc123
 ^1!config private abc123 ^5-> changes currently used ^1sv_privatepassword ^5to ^1abc123
 ^1!config address 123.101.102.103:28960 ^3-> changes currently used server ^1IP:port ^5to ^1123.101.102.103:28960
 ^1!config name Administrator ^5-> changes currently used ^1username ^5to ^1Administrator
 ^1!border on|off ^5-> Turns ^3on^5|^3off ^5border lines around displayed ^3GUI controls^5. 
 ^1!messages on|off ^5-> Turns ^3on^5|^3off ^5messages for temporarily and permanently banned players.
 ^1!egb ^5-> ^2Enables ^5Tiny^6Rcon's ^2city ban feature.
 ^1!dgb ^5-> ^3Disables ^5Tiny^6Rcon's ^3city ban feature.
 ^1!bancity city ^5-> ^3Adds player's ^1city ^3to list of banned cities.
 ^1!unbancity city ^5-> ^2Removes player's ^1city ^2from list of banned cities.
 ^1!banned cities ^5-> ^5Displays list of currently ^1banned cities^5.
 ^1!ecb ^5-> ^2Enables ^5Tiny^6Rcon's ^2country ban feature.
 ^1!dcb ^5-> ^3Disables ^5Tiny^6Rcon's ^3country ban feature.
 ^1!bancountry country ^5-> ^3Adds player's ^1country ^3to list of banned countries.
 ^1!unbancountry country ^5-> ^2Removes player's ^1country ^2from list of banned countries.
 ^1!banned countries ^5-> ^5Displays list of currently ^1banned countries^5.
 ^1!restart ^5-> ^3Remotely ^1restarts ^3logged in ^5Tiny^6Rcon ^3clients.
)"
    };
    print_colored_text(app_handles.hwnd_re_messages_data, help_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
  } else if (input_parts.size() >= 2 && (input_parts[0] == "!list" || input_parts[0] == "list" || input_parts[0] == "!" || input_parts[0] == "!help" || input_parts[0] == "help" || input_parts[0] == "!h" || input_parts[0] == "h") && str_starts_with(input_parts[1], "rcon", true)) {
    const string help_message{
      R"(
 ^5You can use the following rcon commands:
 ^1status ^3-> Retrieves current game state (players' data) from the server (pid, score, ping, 
               name, guid, ip, qport, rate
 ^1clientkick player_pid optional_reason ^5-> Kicks player whose pid number is equal to specified player_pid.
 ^1kick name optional_reason ^3-> Kicks player whose name is equal to specified name.
 ^1onlykick name_no_color_codes optional_reason ^5-> Kicks player whose name is equal to specified name_no_color_codes.
 ^1banclient player_pid optional_reason ^3-> Bans player whose pid number is equal to specified player_pid.
 ^1banuser name optional_reason ^5-> Bans player whose name is equal to specified name.
 ^1tempbanclient player_pid optional_reason ^3-> Temporarily bans player whose pid number is equal to specified player_pid.
 ^1tempbanuser name optional_reason ^5-> Temporarily bans player whose name is equal to specified name.
 ^1serverinfo ^3-> Retrieves currently active server settings.
 ^1map_rotate ^5-> Loads next map on the server.
 ^1map_restart ^3-> Reloads currently played map on the server.
 ^1map map_name ^5-> Immediately loads map 'map_name' on the server.
 ^1fast_restart ^3-> Quickly restarts currently played map on the server.
 ^1getinfo ^5-> Retrieves basic server information from the server.
 ^1mapname ^5-> Retrieves and displays currently played map's rcon name on the console.
 ^1g_gametype ^3-> Retrieves and displays currently played match's gametype on the console.
 ^1sv_maprotation ^5-> Retrieves and displays server's original map rotation setting.
 ^1sv_maprotationcurrent ^3-> Retrieves and displays server's current map rotation setting.
 ^1say "public message" ^5-> Displays "public message" to all players on the server. 
                             The sent message is seen by all players.
 ^1tell player_pid "private message" ^3-> Sends "private message" to player whose pid = player_pid
)"
    };
    print_colored_text(app_handles.hwnd_re_messages_data, help_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
  } else {
    print_colored_text(app_handles.hwnd_re_messages_data,
      "^5********************************************************************"
      "**"
      "********************\n",
      is_append_message_to_richedit_control::yes,
      is_log_message::yes,
      is_log_datetime::no);
    print_colored_text(app_handles.hwnd_re_messages_data,
      "Type '^2!list rcon^5' in the console to get more information about "
      "the "
      "available rcon commands\nthat you can run in the console window.\n",
      is_append_message_to_richedit_control::yes,
      is_log_message::yes,
      is_log_datetime::yes);
    print_colored_text(app_handles.hwnd_re_messages_data,
      "Type '^2!list user^5' in the console to get more information about "
      "the "
      "available user commands\nthat you can run in the console window.\n",
      is_append_message_to_richedit_control::yes,
      is_log_message::yes,
      is_log_datetime::yes);
    print_colored_text(app_handles.hwnd_re_messages_data,
      "**********************************************************************"
      "********************\n",
      is_append_message_to_richedit_control::yes,
      is_log_message::yes,
      is_log_datetime::no);
  }
}


void remove_all_color_codes(char *msg)
{
  const size_t msg_len{ stl::helper::len(msg) };
  size_t start{};
  while (start < msg_len) {
    start = stl::helper::str_index_of(msg, '^', start);
    if (string::npos == start)
      break;

    if (start + 4 <= msg_len && msg[start + 1] == '^' && (msg[start + 2] >= '0' && msg[start + 2] <= '9') && (msg[start + 3] >= '0' && msg[start + 3] <= '9') /*&& msg[start + 2] == msg[start + 3]*/) {
      stl::helper::str_erase_n_chars(msg, start, 4);
    } else if (start + 2 <= msg_len && (msg[start + 1] >= '0' && msg[start + 1] <= '9')) {
      stl::helper::str_erase_n_chars(msg, start, 2);
    } else {
      ++start;
    }
  }
}

void remove_all_color_codes(std::string &msg)
{
  size_t start{};
  while (start < msg.length()) {

    start = msg.find('^', start);
    if (string::npos == start)
      break;

    if (start + 4 <= msg.length() && msg[start + 1] == '^' && (msg[start + 2] >= '0' && msg[start + 2] <= '9') && (msg[start + 3] >= '0' && msg[start + 3] <= '9') && msg[start + 2] == msg[start + 3]) {
      msg.erase(start, 4);
    } else if (start + 2 <= msg.length() && (msg[start + 1] >= '0' && msg[start + 1] <= '9')) {
      msg.erase(start, 2);
    } else {
      ++start;
    }
  }
}

std::string word_wrap(const char *src, const size_t line_width)
{
  char buffer[4096];

  const size_t src_len{ std::min<size_t>(stl::helper::len(src), std::size(buffer) - 1) };
  size_t i{};

  while (i < src_len) {
    // copy src until the end of the line is reached
    for (size_t counter{ 1 }; counter <= line_width; ++counter) {
      // check if end of src reached
      if (i == src_len) {
        buffer[i] = 0;
        return buffer;
      }
      buffer[i] = src[i];
      // check for newlines embedded in the original input
      // and reset the index
      if (buffer[i] == '\n') {
        counter = 1;
      }
      ++i;
    }
    // check for whitespace
    if (isspace(src[i])) {
      buffer[i] = '\n';
      ++i;
    } else {
      // check for nearest whitespace back in src
      for (size_t k{ i }; k > 0; k--) {
        if (isspace(src[k])) {
          buffer[k] = '\n';
          // set src index back to character after this one
          i = k + 1;
          break;
        }
      }
    }
  }
  buffer[i] = 0;
  string result{ buffer };
  return result;
}

void process_user_input(std::string &user_input)
{
  trim_in_place(user_input, " \t\n\f\v\r");
  if (!user_input.empty()) {

    auto command_parts = str_split(user_input, " \t\n\f\v", "", split_on_whole_needle_t::no);

    if (!command_parts.empty()) {
      to_lower_case_in_place(command_parts[0]);
      commands_history.emplace_back(user_input);
      commands_history_index = commands_history.size() - 1;
      if (command_parts[0] == "q" || command_parts[0] == "!q" || command_parts[0] == "!quit" || command_parts[0] == "e" || command_parts[0] == "!e" || command_parts[0] == "exit") {
        user_input = "q";
      } else if (user_commands_set.find(command_parts[0]) != cend(user_commands_set)) {
        main_app.add_command_to_queue(std::move(command_parts), command_type::user, false);
      }
    }
  }
}

void process_user_command(const std::vector<string> &user_cmd)
{
  if (!user_cmd.empty() && main_app.get_command_handlers().contains(user_cmd[0])) {
    const string re_msg{ format("^2Executing user command: ^5{}\n", str_join(user_cmd, " ")) };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    main_app.get_command_handlers().at(user_cmd[0])(user_cmd);
  }
}

volatile bool should_program_terminate(const string &user_input)
{
  if (user_input.empty())
    return is_terminate_program.load();
  is_terminate_program.store(user_input == "q" || user_input == "!q" || user_input == "!quit" || user_input == "quit" || user_input == "e" || user_input == "!e" || user_input == "exit" || user_input == "!exit");
  return is_terminate_program.load();
}

void display_temporarily_banned_ip_addresses(const bool is_save_data_to_log_file)
{
  size_t longest_name_length{ 12 };
  size_t longest_country_length{ 20 };
  auto &temp_banned_players =
    main_app.get_game_server().get_temp_banned_players_data();
  if (!temp_banned_players.empty()) {
    longest_name_length = std::max(longest_name_length, find_longest_player_name_length(temp_banned_players, false, temp_banned_players.size()));
    longest_country_length =
      std::max(longest_country_length,
        find_longest_player_country_city_info_length(temp_banned_players, temp_banned_players.size()));
  }

  ostringstream oss;
  ostringstream log;
  const string decoration_line(133 + longest_name_length + longest_country_length, '=');
  oss << "^5\n"
      << decoration_line << "\n"
      << "^5| ";
  if (is_save_data_to_log_file) {
    log << "\n"
        << decoration_line << "\n"
        << "| ";
  }
  oss << left << setw(15) << "IP address"
      << " | " << left << setw(longest_name_length)
      << "Player name"
      << " | " << left << setw(longest_country_length) << "Country, city"
      << " | " << left << setw(20) << "Tempban issued at"
      << " | " << left << setw(20) << "Tempban expires in"
      << " | " << left << setw(25) << "Reason"
      << " | " << left << setw(32) << "Banned by admin"
      << "|";
  if (is_save_data_to_log_file) {
    log << left << setw(15) << "IP address"
        << " | " << left << setw(longest_name_length)
        << "Player name"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Tempban issued at"
        << " | " << left << setw(20) << "Tempban expires in"
        << " | " << left << setw(25) << "Reason"
        << " | " << left << setw(32) << "Banned by admin"
        << "|";
  }
  oss << "^5\n"
      << decoration_line << "\n";
  if (is_save_data_to_log_file) {
    log << "\n"
        << decoration_line << "\n";
  }
  if (temp_banned_players.empty()) {
    const size_t message_len = stl::helper::len("| There are no players temporarily banned by their IP addresses yet.");
    oss << "^5| ^3There are no players temporarily banned by their IP addresses yet.";
    if (is_save_data_to_log_file) {
      log << "| There are no players temporarily banned by their IP addresses yet.";
    }

    if (message_len + 2 < decoration_line.length()) {
      oss << string(decoration_line.length() - 2 - message_len, ' ');
      if (is_save_data_to_log_file) {
        log << string(decoration_line.length() - 2 - message_len, ' ');
      }
    }
    oss << " ^5|\n";
    if (is_save_data_to_log_file) {
      log << " |\n";
    }
  } else {
    bool is_first_color{ true };
    for (auto &bp : temp_banned_players) {
      const char *next_color{ is_first_color ? "^3" : "^5" };
      oss << "^5| " << next_color << left << setw(15) << bp.ip_address << " ^5| ^7";
      if (is_save_data_to_log_file) {
        log << "| " << left << setw(15) << bp.ip_address << " | ";
      }
      stl::helper::trim_in_place(bp.player_name);
      string name{ bp.player_name };
      remove_all_color_codes(name);
      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(bp.player_name) };
      const size_t printed_name_char_count2{ name.length() };
      if (printed_name_char_count1 < longest_name_length) {
        oss << left << setw(longest_name_length) << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
      } else {
        oss << left << setw(longest_name_length) << bp.player_name;
      }
      if (is_save_data_to_log_file) {
        if (printed_name_char_count2 < longest_name_length) {
          log << left << setw(longest_name_length) << name + string(longest_name_length - printed_name_char_count2, ' ');
        } else {
          log << left << setw(longest_name_length) << name;
        }
      }
      oss << " ^5| ";
      if (is_save_data_to_log_file) {
        log << " | ";
      }
      char buffer2[256];
      (void)snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region), bp.city);
      stl::helper::trim_in_place(bp.reason);
      const time_t ban_expires_time = bp.banned_start_time + (bp.ban_duration_in_hours * 3600);
      const string ban_start_date_str = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bp.banned_start_time);
      const string ban_expires_date_str = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", ban_expires_time);
      oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left << setw(20) << ban_start_date_str
          << " ^5| " << next_color << left << setw(20) << ban_expires_date_str << " ^5| ";
      const size_t printed_reason_char_count1{ get_number_of_characters_without_color_codes(bp.reason.c_str()) };
      if (printed_reason_char_count1 < 25) {
        oss << next_color << left << setw(25) << (bp.reason + string(25 - printed_reason_char_count1, ' '));
      } else {
        oss << next_color << left << bp.reason;
      }
      oss << " ^5| ";
      stl::helper::trim_in_place(bp.banned_by_user_name);
      string banned_by_admin{ bp.banned_by_user_name };
      remove_all_color_codes(banned_by_admin);
      const size_t printed_reason_char_count3{ get_number_of_characters_without_color_codes(bp.banned_by_user_name.c_str()) };
      if (printed_reason_char_count3 < 32) {
        oss << next_color << left << setw(32) << (bp.banned_by_user_name + string(32 - printed_reason_char_count3, ' '));
      } else {
        oss << next_color << left << bp.banned_by_user_name;
      }
      oss << "^5|\n";
      if (is_save_data_to_log_file) {
        string reason{ bp.reason };
        remove_all_color_codes(reason);
        log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20) << ban_start_date_str
            << " | " << left << setw(20) << ban_expires_date_str << " | ";
        const size_t printed_reason_char_count2{ reason.length() };
        if (printed_reason_char_count2 < 25) {
          log << left << setw(25) << (reason + string(25 - printed_reason_char_count2, ' '));
        } else {
          log << reason;
        }
        log << " | ";
        const size_t printed_reason_char_count4{ banned_by_admin.length() };
        if (printed_reason_char_count4 < 32) {
          log << next_color << left << setw(32) << (banned_by_admin + string(32 - printed_reason_char_count4, ' '));
        } else {
          log << next_color << left << banned_by_admin;
        }
        log << "|\n";
      }
      is_first_color = !is_first_color;
    }
  }
  oss << string{ "^5"s + decoration_line + "\n\n"s };
  if (is_save_data_to_log_file) {
    log << string{ decoration_line + "\n\n"s };
  }
  const string message{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true);
  if (is_save_data_to_log_file) {
    log_message(log.str(), is_log_datetime::yes);
  }
}

void display_banned_ip_address_ranges(const bool is_save_data_to_log_file)
{
  size_t longest_name_length{ 12 };
  size_t longest_country_length{ 20 };
  auto &banned_players =
    main_app.get_game_server().get_banned_ip_address_ranges_vector();
  if (!banned_players.empty()) {
    longest_name_length = std::max(longest_name_length, find_longest_player_name_length(banned_players, false, banned_players.size()));
    longest_country_length =
      std::max(longest_country_length,
        find_longest_player_country_city_info_length(banned_players, banned_players.size()));
  }

  ostringstream oss;
  ostringstream log;
  const string decoration_line(119 + longest_name_length + longest_country_length, '=');
  oss << "^5\n"
      << decoration_line << "\n";
  oss << "^5| ";
  if (is_save_data_to_log_file) {
    log << "\n"
        << decoration_line << "\n"
        << "| ";
  }
  oss << left << setw(20) << "IP address range"
      << " | " << left << setw(longest_name_length)
      << "Player name"
      << " | " << left << setw(longest_country_length) << "Country, city"
      << " | " << left << setw(20) << "Date/time of ban"
      << " | " << left << setw(29) << "Reason"
      << " | " << left << setw(32) << "Banned by admin"
      << "|";
  if (is_save_data_to_log_file) {
    log << left << setw(20) << "IP address range"
        << " | " << left << setw(longest_name_length)
        << "Player name"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Date/time of IP ban"
        << " | " << left << setw(29) << "Reason"
        << " | " << left << setw(32) << "Banned by admin"
        << "|";
  }
  oss << "^5\n"
      << decoration_line << "\n";
  if (is_save_data_to_log_file) {
    log << "\n"
        << decoration_line << "\n";
  }
  if (banned_players.empty()) {
    const size_t message_len = stl::helper::len("| You haven't banned any IP address ranges yet.");
    oss << "^5| ^3You haven't banned any IP address ranges yet.";
    if (is_save_data_to_log_file) {
      log << "| You haven't banned any IP address ranges yet.";
    }

    if (message_len + 2 < decoration_line.length()) {
      oss << string(decoration_line.length() - 2 - message_len, ' ');
      if (is_save_data_to_log_file) {
        log << string(decoration_line.length() - 2 - message_len, ' ');
      }
    }
    oss << " ^5|\n";
    if (is_save_data_to_log_file) {
      log << " |\n";
    }
  } else {
    bool is_first_color{ true };
    size_t no{};
    for (auto &bp : banned_players) {
      ++no;
      const char *next_color{ is_first_color ? "^3" : "^5" };
      oss << "^5| " << next_color << left << setw(20) << bp.ip_address << " ^5| ^7";
      if (is_save_data_to_log_file) {
        log << "| " << left << setw(20) << bp.ip_address << " | ";
      }
      stl::helper::trim_in_place(bp.player_name);
      string name{ bp.player_name };
      remove_all_color_codes(name);
      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(bp.player_name) };
      const size_t printed_name_char_count2{ name.length() };
      if (printed_name_char_count1 < longest_name_length) {
        oss << left << setw(longest_name_length) << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
      } else {
        oss << left << setw(longest_name_length) << bp.player_name;
      }
      if (is_save_data_to_log_file) {
        if (printed_name_char_count2 < longest_name_length) {
          log << left << setw(longest_name_length) << name + string(longest_name_length - printed_name_char_count2, ' ');
        } else {
          log << left << setw(longest_name_length) << name;
        }
      }

      oss << " ^5| ";

      if (is_save_data_to_log_file) {
        log << " | ";
      }
      char buffer2[256];
      snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region), bp.city);
      stl::helper::trim_in_place(bp.reason);
      string reason{ bp.reason };
      oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left << setw(20) << bp.banned_date_time
          << " ^5| ";
      const size_t printed_reason_char_count1{ get_number_of_characters_without_color_codes(reason.c_str()) };
      if (printed_reason_char_count1 < 29) {
        oss << next_color << left << setw(29) << (reason + string(29 - printed_reason_char_count1, ' '));
      } else {
        oss << next_color << left << reason;
      }
      oss << " ^5| ";
      stl::helper::trim_in_place(bp.banned_by_user_name);
      string banned_by_user{ bp.banned_by_user_name };
      const size_t printed_reason_char_count3{ get_number_of_characters_without_color_codes(banned_by_user.c_str()) };
      if (printed_reason_char_count3 < 32) {
        oss << next_color << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count3, ' '));
      } else {
        oss << next_color << left << banned_by_user;
      }
      oss << "^5|\n";
      if (is_save_data_to_log_file) {
        remove_all_color_codes(reason);
        remove_all_color_codes(banned_by_user);
        log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20) << bp.banned_date_time
            << " | ";
        const size_t printed_reason_char_count2{ reason.length() };
        if (printed_reason_char_count2 < 29) {
          log << left << setw(29) << (reason + string(29 - printed_reason_char_count2, ' '));
        } else {
          log << reason;
        }
        log << " | ";
        const size_t printed_reason_char_count4{ banned_by_user.length() };
        if (printed_reason_char_count4 < 32) {
          log << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count4, ' '));
        } else {
          log << banned_by_user;
        }
        log << "|\n";
      }
      is_first_color = !is_first_color;
    }
  }
  oss << string{ "^5"s + decoration_line + "\n\n"s };
  if (is_save_data_to_log_file) {
    log << string{ decoration_line + "\n\n"s };
  }
  const string message{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true);
  if (is_save_data_to_log_file) {
    log_message(log.str(), is_log_datetime::yes);
  }
}

//void display_temporarily_banned_ip_addresses()
//{
//  size_t longest_name_length{ 12 };
//  size_t longest_country_length{ 20 };
//  auto &temp_banned_players =
//    main_app.get_game_server().get_temp_banned_players_data();
//  if (!temp_banned_players.empty()) {
//    longest_name_length = std::max(longest_name_length, find_longest_player_name_length(temp_banned_players, false, temp_banned_players.size()));
//    longest_country_length =
//      std::max(longest_country_length,
//        find_longest_player_country_city_info_length(temp_banned_players, temp_banned_players.size()));
//  }
//
//  ostringstream oss;
//  ostringstream log;
//  const string decoration_line(98 + longest_name_length + longest_country_length, '=');
//  oss << "^5\n"
//      << decoration_line << "\n"
//      << "^5| ";
//  log << "\n"
//      << decoration_line << "\n"
//      << "| ";
//  oss << left << setw(15) << "IP address"
//      << " | " << left << setw(longest_name_length)
//      << "Player name"
//      << " | " << left << setw(longest_country_length) << "Country, city"
//      << " | " << left << setw(20) << "Tempban issued on"
//      << " | " << left << setw(20) << "Tempban expires in"
//      << " | " << left << setw(25) << "Reason"
//      << "|";
//  log << left << setw(15) << "IP address"
//      << " | " << left << setw(longest_name_length)
//      << "Player name"
//      << " | " << left << setw(longest_country_length) << "Country, city"
//      << " | " << left << setw(20) << "Tempban issued on"
//      << " | " << left << setw(20) << "Tempban expires in"
//      << " | " << left << setw(25) << "Reason"
//      << "|";
//  oss << "^5\n"
//      << decoration_line << "\n";
//  log << "\n"
//      << decoration_line << "\n";
//  if (temp_banned_players.empty()) {
//    const size_t message_len = stl::helper::len("| There are no players temporarily banned by their IP addresses yet.");
//    oss << "^5| ^3There are no players temporarily banned by their IP addresses yet.";
//    log << "| There are no players temporarily banned by their IP addresses yet.";
//
//    if (message_len + 2 < decoration_line.length()) {
//      oss << string(decoration_line.length() - 2 - message_len, ' ');
//      log << string(decoration_line.length() - 2 - message_len, ' ');
//    }
//    oss << " ^5|\n";
//    log << " |\n";
//  } else {
//    bool is_first_color{ true };
//    for (auto &bp : temp_banned_players) {
//      const char *next_color{ is_first_color ? "^5" : "^3" };
//      oss << "^5| " << next_color << left << setw(15) << bp.ip_address << " ^5| ^7";
//      log << "| " << left << setw(15) << bp.ip_address << " | ";
//      stl::helper::trim_in_place(bp.player_name);
//      string name{ bp.player_name };
//      remove_all_color_codes(name);
//      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(bp.player_name) };
//      // const size_t printed_name_char_count2{ get_number_of_characters_without_color_codes(name.c_str()) };
//      const size_t printed_name_char_count2{ name.length() };
//      if (printed_name_char_count1 < longest_name_length) {
//        oss << left << setw(longest_name_length) << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
//      } else {
//        oss << left << setw(longest_name_length) << bp.player_name;
//      }
//      if (printed_name_char_count2 < longest_name_length) {
//        log << left << setw(longest_name_length) << name + string(longest_name_length - printed_name_char_count2, ' ');
//      } else {
//        log << left << setw(longest_name_length) << name;
//      }
//      oss << " ^5| ";
//      log << " | ";
//      char buffer2[256];
//      (void)snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region), bp.city);
//      stl::helper::trim_in_place(bp.reason);
//      const time_t ban_expires_time = bp.banned_start_time + (bp.ban_duration_in_hours * 3600);
//      const string ban_start_date_str = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bp.banned_start_time);
//      const string ban_expires_date_str = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", ban_expires_time);
//      oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left << setw(20) << ban_start_date_str
//          << " ^5| " << next_color << left << setw(20) << ban_expires_date_str << " ^5| ";
//      const size_t printed_reason_char_count1{ get_number_of_characters_without_color_codes(bp.reason.c_str()) };
//      if (printed_reason_char_count1 < 25) {
//        oss << next_color << left << setw(25) << (bp.reason + string(25 - printed_reason_char_count1, ' '));
//      } else {
//        oss << next_color << left << bp.reason;
//      }
//      oss << "^5|\n";
//
//      string reason{ bp.reason };
//      remove_all_color_codes(reason);
//      // stl::helper::trim_in_place(reason);
//      log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20) << ban_start_date_str
//          << " | " << left << setw(20) << ban_expires_date_str << " | ";
//      const size_t printed_reason_char_count2{ reason.length() };
//      if (printed_reason_char_count2 < 25) {
//        log << left << setw(25) << (reason + string(25 - printed_reason_char_count2, ' '));
//      } else {
//        log << reason;
//      }
//      log << "|\n";
//      is_first_color = !is_first_color;
//    }
//  }
//  oss << string{ "^5"s + decoration_line + "\n\n"s };
//  log << string{ decoration_line + "\n\n"s };
//  const string message{ oss.str() };
//  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true);
//  log_message(log.str(), is_log_datetime::yes);
//}

void display_protected_entries(const char *table_title, const std::set<std::string> &protected_entries)
{
  size_t longest_name_length{ 32 };

  auto &banned_players =
    main_app.get_game_server().get_banned_ip_addresses_vector();
  if (!banned_players.empty()) {
    longest_name_length = std::max(longest_name_length, find_longest_entry_length(cbegin(protected_entries), cend(protected_entries), false));
  }

  ostringstream oss;
  ostringstream log;
  const string decoration_line(4 + longest_name_length, '=');
  oss << "^5\n"
      << decoration_line << "\n";
  oss << "^5| ";
  log << "\n"
      << decoration_line << "\n"
      << "| ";
  oss << left << setw(longest_name_length) << table_title << " |";

  log << left << setw(longest_name_length) << table_title << " |";
  oss << "^5\n"
      << decoration_line << "\n";
  log << "\n"
      << decoration_line << "\n";
  if (protected_entries.empty()) {
    const size_t message_len = stl::helper::len("| There are no protected entries.");
    oss << "^5| ^3There are no protected entries.";
    log << "| There are no protected entries.";

    if (message_len + 2 < decoration_line.length()) {
      oss << string(decoration_line.length() - 2 - message_len, ' ');
      log << string(decoration_line.length() - 2 - message_len, ' ');
    }
    oss << " ^5|\n";
    log << " |\n";
  } else {
    bool is_first_color{ true };
    size_t no{};
    for (const auto &protected_entry : protected_entries) {
      ++no;
      const char *next_color{ is_first_color ? "^3" : "^5" };
      oss << "^5| " << next_color;

      string entry{ protected_entry };
      remove_all_color_codes(entry);
      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(protected_entry.c_str()) };
      const size_t printed_name_char_count2{ entry.length() };
      if (printed_name_char_count1 < longest_name_length) {
        oss << left << setw(longest_name_length) << protected_entry + string(longest_name_length - printed_name_char_count1, ' ');
      } else {
        oss << left << setw(longest_name_length) << protected_entry;
      }
      if (printed_name_char_count2 < longest_name_length) {
        log << left << setw(longest_name_length) << entry + string(longest_name_length - printed_name_char_count2, ' ');
      } else {
        log << left << setw(longest_name_length) << entry;
      }
      oss << " ^5|\n";
      log << " |\n";

      is_first_color = !is_first_color;
    }
  }
  oss << string{ "^5"s + decoration_line + "\n\n"s };
  log << string{ decoration_line + "\n\n"s };
  const string message{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true);
  log_message(log.str(), is_log_datetime::yes);
}

void display_admins_data()
{
  size_t longest_name_length{ 12 };
  size_t longest_geoinfo_length{ 20 };
  const auto &users =
    main_app.get_users();
  if (!users.empty()) {
    longest_name_length = std::max(longest_name_length, find_longest_user_name_length(users, false, users.size()));
    longest_geoinfo_length =
      std::max(longest_geoinfo_length,
        find_longest_user_country_city_info_length(users, users.size()));
  }

  ostringstream oss;
  const string decoration_line(224 + longest_name_length + longest_geoinfo_length, '=');
  oss << "^5\n"
      << decoration_line << "\n";
  oss << "^5| ";
  oss << left << setw(longest_name_length) << "User name"
      << " | " << left << setw(13) << "Is logged in?"
      << " | " << left << setw(11) << "Is online?"
      << " | " << left << setw(16) << "IP address"
      << " | " << left << setw(longest_geoinfo_length) << "Country, city"
      << " | " << left << setw(20) << "Last login"
      << " | " << left << setw(20) << "Last logout"
      << " | " << left << setw(10) << "Logins"
      << " | " << left << setw(10) << "Warnings"
      << " | " << left << setw(10) << "Kicks"
      << " | " << left << setw(10) << "Tempbans"
      << " | " << left << setw(10) << "GUID bans"
      << " | " << left << setw(10) << "IP bans"
      << " | " << left << setw(13) << "IP range bans"
      << " | " << left << setw(10) << "City bans"
      << " | " << left << setw(13) << "Country bans"
      << "|";
  oss << "^5\n"
      << decoration_line << "\n";
  if (users.empty()) {
    const size_t message_len = stl::helper::len("| There is no received administrator (user) data.");
    oss << "^5| ^3There is no received administrator (user) data.";

    if (message_len + 2 < decoration_line.length()) {
      oss << string(decoration_line.length() - 2 - message_len, ' ');
    }
    oss << " ^5|\n";

  } else {
    bool is_first_color{ true };
    for (auto &user : users) {
      /*if (!main_app.get_is_user_data_received_for_user(user->user_name, user->ip_address))
        continue;*/
      const char *next_color{ is_first_color ? "^3" : "^5" };
      oss << "^5| ";

      stl::helper::trim_in_place(user->user_name);
      string name{ user->user_name };
      remove_all_color_codes(name);
      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(user->user_name.c_str()) };
      if (printed_name_char_count1 < longest_name_length) {
        oss << left << setw(longest_name_length) << user->user_name + string(longest_name_length - printed_name_char_count1, ' ');
      } else {
        oss << left << setw(longest_name_length) << user->user_name;
      }

      oss << " ^5| ";

      string is_logged_in{ user->is_logged_in ? "^2yes" : "^1no" };
      string is_logged_in_without_color_codes{ is_logged_in };
      remove_all_color_codes(is_logged_in_without_color_codes);
      size_t printed_field_char_count{ get_number_of_characters_without_color_codes(is_logged_in.c_str()) };
      size_t printed_field_char_count2{ is_logged_in_without_color_codes.length() };
      if (printed_field_char_count < 13) {
        oss << left << setw(13) << is_logged_in + string(13 - printed_field_char_count, ' ');
      } else {
        oss << left << setw(13) << is_logged_in;
      }

      oss << " ^5| ";

      const string is_online_in{ user->is_online ? "^2yes" : "^1no" };
      const string &is_online_in_without_color_codes{ is_online_in };
      remove_all_color_codes(is_logged_in_without_color_codes);
      printed_field_char_count = get_number_of_characters_without_color_codes(is_online_in.c_str());
      printed_field_char_count2 = is_online_in_without_color_codes.length();
      if (printed_field_char_count < 11) {
        oss << left << setw(11) << is_online_in + string(11 - printed_field_char_count, ' ');
      } else {
        oss << left << setw(11) << is_online_in;
      }

      oss << " ^5| ";

      oss << next_color << left << setw(16) << user->ip_address << " ^5| " << next_color << left << setw(longest_geoinfo_length) << user->geo_information << " ^5| " << next_color << left << setw(20) << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", user->last_login_time_stamp) << " ^5| " << next_color << left << setw(20) << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", user->last_logout_time_stamp) << " ^5| " << next_color << left << setw(10) << user->no_of_logins << " ^5| " << next_color << left << setw(10) << user->no_of_warnings << " ^5| " << next_color << left << setw(10) << user->no_of_kicks << " ^5| " << next_color << left << setw(10) << user->no_of_tempbans << " ^5| " << next_color << left << setw(10) << user->no_of_guidbans << " ^5| " << next_color << left << setw(10) << user->no_of_ipbans
          << " ^5| " << next_color << left << setw(13) << user->no_of_iprangebans
          << " ^5| " << next_color << left << setw(10) << user->no_of_citybans << " ^5| " << next_color << left << setw(13) << user->no_of_countrybans << "^5|\n";

      is_first_color = !is_first_color;
    }
  }
  oss << string{ "^5"s + decoration_line + "\n\n"s };

  const string message{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true);
}

void display_permanently_banned_ip_addresses(const bool is_save_data_to_log_file)
{
  size_t longest_name_length{ 12 };
  size_t longest_country_length{ 20 };
  auto &banned_players =
    main_app.get_game_server().get_banned_ip_addresses_vector();
  if (!banned_players.empty()) {
    longest_name_length = std::max(longest_name_length, find_longest_player_name_length(banned_players, false, banned_players.size()));
    longest_country_length =
      std::max(longest_country_length,
        find_longest_player_country_city_info_length(banned_players, banned_players.size()));
  }

  ostringstream oss;
  ostringstream log;
  const string decoration_line(122 + longest_name_length + longest_country_length, '=');
  oss << "^5\n"
      << decoration_line << "\n";
  oss << "^5| ";
  if (is_save_data_to_log_file) {
    log << "\n"
        << decoration_line << "\n"
        << "| ";
  }
  oss << left << setw(5) << "No."
      << " | " << left << setw(15) << "IP address"
      << " | " << left << setw(longest_name_length)
      << "Player name"
      << " | " << left << setw(longest_country_length) << "Country, city"
      << " | " << left << setw(20) << "Date/time of IP ban"
      << " | " << left << setw(29) << "Reason"
      << " | " << left << setw(32) << "Banned by admin"
      << "|";
  if (is_save_data_to_log_file) {
    log << left << setw(5) << "No." << left << setw(15) << "IP address"
        << " | " << left << setw(longest_name_length)
        << "Player name"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Date/time of IP ban"
        << " | " << left << setw(29) << "Reason"
        << " | " << left << setw(32) << "Banned by admin"
        << "|";
  }
  oss << "^5\n"
      << decoration_line << "\n";
  if (is_save_data_to_log_file) {
    log << "\n"
        << decoration_line << "\n";
  }
  if (banned_players.empty()) {
    const size_t message_len = stl::helper::len("| There are no players permanently banned by their IP addresses yet.");
    oss << "^5| ^3There are no players permanently banned by their IP addresses yet.";
    if (is_save_data_to_log_file) {
      log << "| There are no players permanently banned by their IP addresses yet.";
    }

    if (message_len + 2 < decoration_line.length()) {
      oss << string(decoration_line.length() - 2 - message_len, ' ');
      if (is_save_data_to_log_file) {
        log << string(decoration_line.length() - 2 - message_len, ' ');
      }
    }
    oss << " ^5|\n";
    if (is_save_data_to_log_file) {
      log << " |\n";
    }
  } else {
    bool is_first_color{ true };
    size_t no{};
    for (auto &bp : banned_players) {
      ++no;
      const char *next_color{ is_first_color ? "^3" : "^5" };
      oss << "^5| " << next_color << left << setw(5) << no << " ^5| " << left << setw(15) << bp.ip_address << " ^5| ^7";
      if (is_save_data_to_log_file) {
        log << "| " << left << setw(5) << no << " | " << left << setw(15) << bp.ip_address << " | ";
      }
      stl::helper::trim_in_place(bp.player_name);
      string name{ bp.player_name };
      remove_all_color_codes(name);
      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(bp.player_name) };
      const size_t printed_name_char_count2{ name.length() };
      if (printed_name_char_count1 < longest_name_length) {
        oss << left << setw(longest_name_length) << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
      } else {
        oss << left << setw(longest_name_length) << bp.player_name;
      }
      if (is_save_data_to_log_file) {
        if (printed_name_char_count2 < longest_name_length) {
          log << left << setw(longest_name_length) << name + string(longest_name_length - printed_name_char_count2, ' ');
        } else {
          log << left << setw(longest_name_length) << name;
        }
      }
      oss << " ^5| ";
      if (is_save_data_to_log_file) {
        log << " | ";
      }
      char buffer2[256];
      snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region), bp.city);
      stl::helper::trim_in_place(bp.reason);
      string reason{ bp.reason };
      oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left << setw(20) << bp.banned_date_time
          << " ^5| ";
      const size_t printed_reason_char_count1{ get_number_of_characters_without_color_codes(reason.c_str()) };
      if (printed_reason_char_count1 < 29) {
        oss << next_color << left << setw(29) << (reason + string(29 - printed_reason_char_count1, ' '));
      } else {
        oss << next_color << left << reason;
      }
      oss << " ^5| ";
      stl::helper::trim_in_place(bp.banned_by_user_name);
      string banned_by_user{ bp.banned_by_user_name };
      const size_t printed_reason_char_count3{ get_number_of_characters_without_color_codes(banned_by_user.c_str()) };
      if (printed_reason_char_count3 < 32) {
        oss << next_color << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count3, ' '));
      } else {
        oss << next_color << left << banned_by_user;
      }
      oss << "^5|\n";
      if (is_save_data_to_log_file) {
        remove_all_color_codes(reason);
        remove_all_color_codes(banned_by_user);
        log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20) << bp.banned_date_time
            << " | ";
        const size_t printed_reason_char_count2{ reason.length() };
        if (printed_reason_char_count2 < 29) {
          log << left << setw(29) << (reason + string(29 - printed_reason_char_count2, ' '));
        } else {
          log << reason;
        }
        log << " | ";
        const size_t printed_reason_char_count4{ banned_by_user.length() };
        if (printed_reason_char_count4 < 32) {
          log << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count4, ' '));
        } else {
          log << banned_by_user;
        }
        log << "|\n";
      }
      is_first_color = !is_first_color;
    }
  }
  oss << string{ "^5"s + decoration_line + "\n\n"s };
  if (is_save_data_to_log_file) {
    log << string{ decoration_line + "\n\n"s };
  }
  const string message{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true);
  if (is_save_data_to_log_file) {
    log_message(log.str(), is_log_datetime::yes);
  }
}

//void display_admins_data()
//{
//  size_t longest_name_length{ 12 };
//  size_t longest_geoinfo_length{ 20 };
//  const auto &users =
//    main_app.get_users();
//  if (!users.empty()) {
//    longest_name_length = std::max(longest_name_length, find_longest_user_name_length(users, false, users.size()));
//    longest_geoinfo_length =
//      std::max(longest_geoinfo_length,
//        find_longest_user_country_city_info_length(users, users.size()));
//  }
//
//  ostringstream oss;
//  const string decoration_line(224 + longest_name_length + longest_geoinfo_length, '=');
//  oss << "^5\n"
//      << decoration_line << "\n";
//  oss << "^5| ";
//  oss << left << setw(longest_name_length) << "User name"
//      << " | " << left << setw(13) << "Is logged in?"
//      << " | " << left << setw(11) << "Is online?"
//      << " | " << left << setw(16) << "IP address"
//      << " | " << left << setw(longest_geoinfo_length) << "Country, city"
//      << " | " << left << setw(20) << "Last login"
//      << " | " << left << setw(20) << "Last logout"
//      << " | " << left << setw(10) << "Logins"
//      << " | " << left << setw(10) << "Warnings"
//      << " | " << left << setw(10) << "Kicks"
//      << " | " << left << setw(10) << "Tempbans"
//      << " | " << left << setw(10) << "GUID bans"
//      << " | " << left << setw(10) << "IP bans"
//      << " | " << left << setw(13) << "IP range bans"
//      << " | " << left << setw(10) << "City bans"
//      << " | " << left << setw(13) << "Country bans"
//      << "|";
//  oss << "^5\n"
//      << decoration_line << "\n";
//  if (users.empty()) {
//    const size_t message_len = stl::helper::len("| There is no received administrator (user) data.");
//    oss << "^5| ^3There is no received administrator (user) data.";
//
//    if (message_len + 2 < decoration_line.length()) {
//      oss << string(decoration_line.length() - 2 - message_len, ' ');
//    }
//    oss << " ^5|\n";
//
//  } else {
//    bool is_first_color{ true };
//    for (auto &user : users) {
//      if (!main_app.get_is_user_data_received_for_user(user->user_name, user->ip_address))
//        continue;
//      const char *next_color{ is_first_color ? "^3" : "^5" };
//      oss << "^5| ";
//
//      stl::helper::trim_in_place(user->user_name);
//      string name{ user->user_name };
//      remove_all_color_codes(name);
//      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(user->user_name.c_str()) };
//      if (printed_name_char_count1 < longest_name_length) {
//        oss << left << setw(longest_name_length) << user->user_name + string(longest_name_length - printed_name_char_count1, ' ');
//      } else {
//        oss << left << setw(longest_name_length) << user->user_name;
//      }
//
//      oss << " ^5| ";
//
//      string is_logged_in{ user->is_logged_in ? "^2yes" : "^1no" };
//      string is_logged_in_without_color_codes{ is_logged_in };
//      remove_all_color_codes(is_logged_in_without_color_codes);
//      size_t printed_field_char_count{ get_number_of_characters_without_color_codes(is_logged_in.c_str()) };
//      size_t printed_field_char_count2{ is_logged_in_without_color_codes.length() };
//      if (printed_field_char_count < 13) {
//        oss << left << setw(13) << is_logged_in + string(13 - printed_field_char_count, ' ');
//      } else {
//        oss << left << setw(13) << is_logged_in;
//      }
//
//      oss << " ^5| ";
//
//      const string is_online_in{ user->is_online ? "^2yes" : "^1no" };
//      const string &is_online_in_without_color_codes{ is_online_in };
//      remove_all_color_codes(is_logged_in_without_color_codes);
//      printed_field_char_count = get_number_of_characters_without_color_codes(is_online_in.c_str());
//      printed_field_char_count2 = is_online_in_without_color_codes.length();
//      if (printed_field_char_count < 11) {
//        oss << left << setw(11) << is_online_in + string(11 - printed_field_char_count, ' ');
//      } else {
//        oss << left << setw(11) << is_online_in;
//      }
//
//      oss << " ^5| ";
//
//      oss << next_color << left << setw(16) << user->ip_address << " ^5| " << next_color << left << setw(longest_geoinfo_length) << user->geo_information << " ^5| " << next_color << left << setw(20) << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", user->last_login_time_stamp) << " ^5| " << next_color << left << setw(20) << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", user->last_logout_time_stamp) << " ^5| " << next_color << left << setw(10) << user->no_of_logins << " ^5| " << next_color << left << setw(10) << user->no_of_warnings << " ^5| " << next_color << left << setw(10) << user->no_of_kicks << " ^5| " << next_color << left << setw(10) << user->no_of_tempbans << " ^5| " << next_color << left << setw(10) << user->no_of_guidbans << " ^5| " << next_color << left << setw(10) << user->no_of_ipbans
//          << " ^5| " << next_color << left << setw(13) << user->no_of_iprangebans
//          << " ^5| " << next_color << left << setw(10) << user->no_of_citybans << " ^5| " << next_color << left << setw(13) << user->no_of_countrybans << "^5|\n";
//
//      is_first_color = !is_first_color;
//    }
//  }
//  oss << string{ "^5"s + decoration_line + "\n\n"s };
//
//  const string message{ oss.str() };
//  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true);
//}

//void display_permanently_banned_ip_addresses()
//{
//  size_t longest_name_length{ 12 };
//  size_t longest_country_length{ 20 };
//  auto &banned_players =
//    main_app.get_game_server().get_banned_ip_addresses_vector();
//  if (!banned_players.empty()) {
//    longest_name_length = std::max(longest_name_length, find_longest_player_name_length(banned_players, false, banned_players.size()));
//    longest_country_length =
//      std::max(longest_country_length,
//        find_longest_player_country_city_info_length(banned_players, banned_players.size()));
//  }
//
//  ostringstream oss;
//  ostringstream log;
//  const string decoration_line(87 + longest_name_length + longest_country_length, '=');
//  oss << "^5\n"
//      << decoration_line << "\n";
//  oss << "^5| ";
//  log << "\n"
//      << decoration_line << "\n"
//      << "| ";
//  oss << left << setw(5) << "No."
//      << " | " << left << setw(15) << "IP address"
//      << " | " << left << setw(longest_name_length)
//      << "Player name"
//      << " | " << left << setw(longest_country_length) << "Country, city"
//      << " | " << left << setw(20) << "Date/time of IP ban"
//      << " | " << left << setw(29) << "Reason"
//      << "|";
//  log << left << setw(5) << "No." << left << setw(15) << "IP address"
//      << " | " << left << setw(longest_name_length)
//      << "Player name"
//      << " | " << left << setw(longest_country_length) << "Country, city"
//      << " | " << left << setw(20) << "Date/time of IP ban"
//      << " | " << left << setw(29) << "Reason"
//      << "|";
//  oss << "^5\n"
//      << decoration_line << "\n";
//  log << "\n"
//      << decoration_line << "\n";
//  if (banned_players.empty()) {
//    const size_t message_len = stl::helper::len("| There are no players permanently banned by their IP addresses yet.");
//    oss << "^5| ^3There are no players permanently banned by their IP addresses yet.";
//    log << "| There are no players permanently banned by their IP addresses yet.";
//
//    if (message_len + 2 < decoration_line.length()) {
//      oss << string(decoration_line.length() - 2 - message_len, ' ');
//      log << string(decoration_line.length() - 2 - message_len, ' ');
//    }
//    oss << " ^5|\n";
//    log << " |\n";
//  } else {
//    bool is_first_color{ true };
//    size_t no{};
//    for (auto &bp : banned_players) {
//      ++no;
//      const char *next_color{ is_first_color ? "^5" : "^3" };
//      oss << "^5| " << next_color << left << setw(5) << no << " ^5| " << left << setw(15) << bp.ip_address << " ^5| ^7";
//      log << "| " << left << setw(5) << no << " | " << left << setw(15) << bp.ip_address << " | ";
//      stl::helper::trim_in_place(bp.player_name);
//      string name{ bp.player_name };
//      remove_all_color_codes(name);
//      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(bp.player_name) };
//      // const size_t printed_name_char_count2{ get_number_of_characters_without_color_codes(name) };
//      const size_t printed_name_char_count2{ name.length() };
//      if (printed_name_char_count1 < longest_name_length) {
//        oss << left << setw(longest_name_length) << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
//      } else {
//        oss << left << setw(longest_name_length) << bp.player_name;
//      }
//      if (printed_name_char_count2 < longest_name_length) {
//        log << left << setw(longest_name_length) << name + string(longest_name_length - printed_name_char_count2, ' ');
//      } else {
//        log << left << setw(longest_name_length) << name;
//      }
//      oss << " ^5| ";
//      log << " | ";
//      char buffer2[256];
//      snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region), bp.city);
//      stl::helper::trim_in_place(bp.reason);
//      string reason{ bp.reason };
//      oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left << setw(20) << bp.banned_date_time
//          << " ^5| ";
//      const size_t printed_reason_char_count1{ get_number_of_characters_without_color_codes(reason.c_str()) };
//      if (printed_reason_char_count1 < 29) {
//        oss << next_color << left << setw(29) << (reason + string(29 - printed_reason_char_count1, ' '));
//      } else {
//        oss << next_color << left << reason;
//      }
//      oss << "^5|\n";
//      remove_all_color_codes(reason);
//      // stl::helper::trim_in_place(reason);
//      log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20) << bp.banned_date_time
//          << " | ";
//      const size_t printed_reason_char_count2{ reason.length() };
//      if (printed_reason_char_count2 < 29) {
//        log << left << setw(29) << (reason + string(29 - printed_reason_char_count2, ' '));
//      } else {
//        log << reason;
//      }
//      log << "|\n";
//      is_first_color = !is_first_color;
//    }
//  }
//  oss << string{ "^5"s + decoration_line + "\n\n"s };
//  log << string{ decoration_line + "\n\n"s };
//  const string message{ oss.str() };
//  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true);
//  log_message(log.str(), is_log_datetime::yes);
//}

void import_geoip_data(vector<geoip_data> &geo_data, const char *file_path)
{
  const string log_msg{ "Importing geological binary data from specified file ("s + file_path + ")."s };
  log_message(log_msg, is_log_datetime::yes);
  geo_data.clear();

  ifstream is{ file_path, std::ios::binary };

  if (is) {
    const size_t file_size{ get_file_size_in_bytes(file_path) };
    const size_t num_of_elements{ file_size / sizeof(geoip_data) };
    geo_data.reserve(num_of_elements + 5);
    geo_data.resize(num_of_elements);
    is.read(reinterpret_cast<char *>(geo_data.data()), file_size);
  }
}

void export_geoip_data(const vector<geoip_data> &geo_data, const char *file_path)
{
  const string log_msg{ "Exporting geological binary data to specified file ("s + file_path + ")."s };
  log_message(log_msg, is_log_datetime::yes);
  ofstream os{ file_path, std::ios::binary };

  if (os) {

    os.write(reinterpret_cast<const char *>(geo_data.data()), static_cast<std::streamsize>(geo_data.size()) * static_cast<std::streamsize>(sizeof(geoip_data)));
  }
  os << flush;
}

void change_colors()
{
  static const std::regex color_code_regex{ R"(\^?\d{1})" };
  const auto &fmc = main_app.get_game_server().get_full_map_name_color();
  const auto &rmc = main_app.get_game_server().get_rcon_map_name_color();
  const auto &fgc = main_app.get_game_server().get_full_gametype_name_color();
  const auto &rgc = main_app.get_game_server().get_rcon_gametype_name_color();
  const auto &onp = main_app.get_game_server().get_online_players_count_color();
  const auto &ofp = main_app.get_game_server().get_offline_players_count_color();
  const auto &bc = main_app.get_game_server().get_border_line_color();
  const auto &ph = main_app.get_game_server().get_header_player_pid_color();
  const auto &pd = main_app.get_game_server().get_data_player_pid_color();

  const auto &sh = main_app.get_game_server().get_header_player_score_color();
  const auto &sd = main_app.get_game_server().get_data_player_score_color();

  const auto &pgh = main_app.get_game_server().get_header_player_ping_color();
  const auto &pgd = main_app.get_game_server().get_data_player_ping_color();

  const auto &pnh = main_app.get_game_server().get_header_player_name_color();

  const auto &iph = main_app.get_game_server().get_header_player_ip_color();
  const auto &ipd = main_app.get_game_server().get_data_player_ip_color();

  const auto &gh = main_app.get_game_server().get_header_player_geoinfo_color();
  const auto &gd = main_app.get_game_server().get_data_player_geoinfo_color();

  print_colored_text(app_handles.hwnd_re_messages_data, "^5\nType a number ^1(color code: 0-9) ^5for each color setting\n or press ^3(Enter) ^5to accept the default value for it.\n", is_append_message_to_richedit_control::yes, is_log_message::no);

  string color_code;

  do {
    const string re_msg{ fmc + "Full map name color (0-9), press enter to accept default (2): " };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^2";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_full_map_name_color(color_code);

  do {
    const string re_msg{ rmc + "Rcon map name color (0-9), press enter to accept default (1): "s };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^1";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_rcon_map_name_color(color_code);

  do {
    const string re_msg{ fgc + "Full gametype name color (0-9), press enter to accept default (2): " };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^2";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_full_gametype_color(color_code);

  do {
    const string re_msg{ rgc + "Rcon gametype name color (0-9), press enter to accept default (1): " };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^1";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_rcon_gametype_color(color_code);

  do {
    const string re_msg{ onp + "Online players count color (0-9), press enter to accept default (2): " };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));
  if (color_code.empty())
    color_code = "^2";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_online_players_count_color(color_code);

  do {
    const string re_msg{ ofp + "Offline players count color (0-9), press enter to accept default (1): " };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^1";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_offline_players_count_color(color_code);

  do {
    const string re_msg{ bc + "Border line color (0-9), press enter to accept default (5): " };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^5";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_border_line_color(color_code);

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, "^3Use different alternating background colors for even and odd lines? (yes|no, enter for default (yes): ", is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
    stl::helper::to_lower_case_in_place(color_code);
  } while (!color_code.empty() && color_code != "yes" && color_code != "no");

  main_app.get_game_server().set_is_use_different_background_colors_for_even_and_odd_lines(color_code.empty() || color_code == "yes");

  if (main_app.get_game_server().get_is_use_different_background_colors_for_even_and_odd_lines()) {
    do {
      print_colored_text(app_handles.hwnd_re_messages_data, "^5Background color of odd player data lines (0-9), press enter to accept default (0 - black): ", is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^0";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_odd_player_data_lines_bg_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Background color of even player data lines (0-9), press enter to accept default (8 - gray): ", is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^8";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_even_player_data_lines_bg_color(color_code);
  }

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, "^5Use different alternating foreground colors for even and odd lines? (yes|no, enter for default (yes): ", is_append_message_to_richedit_control::yes, is_log_message::no);
    getline(cin, color_code);
    stl::helper::to_lower_case_in_place(color_code);
  } while (!color_code.empty() && color_code != "yes" && color_code != "no");

  main_app.get_game_server().set_is_use_different_foreground_colors_for_even_and_odd_lines(color_code.empty() || color_code == "yes");

  if (main_app.get_game_server().get_is_use_different_foreground_colors_for_even_and_odd_lines()) {
    do {
      print_colored_text(app_handles.hwnd_re_messages_data, "Foreground color of odd player data lines (0-9), press enter to accept default (5): ", is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_odd_player_data_lines_fg_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, "Foreground color of even player data lines (0-9), press enter to accept default (3): ", is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^3";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_even_player_data_lines_fg_color(color_code);

  } else {

    do {
      const string re_msg{ ph + "Player pid header color (0-9), press enter to accept default (1): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^1";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_pid_color(color_code);

    do {
      const string re_msg{ pd + "Player pid data color (0-9), press enter to accept default (1): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);

      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^1";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_pid_color(color_code);

    do {
      const string re_msg{ sh + "Player score header color (0-9), press enter to accept default (5): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_score_color(color_code);

    do {
      const string re_msg{ sd + "Player score data color (0-9), press enter to accept default (5): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_score_color(color_code);

    do {
      const string re_msg{ pgh + "Player ping header color (0-9), press enter to accept default (3): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^3";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_ping_color(color_code);

    do {
      const string re_msg{ pgd + "Player ping data color (0-9), press enter to accept default (3): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^3";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_ping_color(color_code);

    do {
      const string re_msg{ pnh + "Player name header color (0-9), press Enter to accept default (5): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_name_color(color_code);

    do {
      const string re_msg{ iph + "Player IP header color (0-9), press Enter to accept default (5): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_ip_color(color_code);

    do {
      const string re_msg{ ipd + "Player IP data color (0-9), press Enter to accept default (5): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_ip_color(color_code);

    do {
      const string re_msg{ gh + "Player country/city header color (0-9), press Enter to accept default (2): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^2";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_geoinfo_color(color_code);

    do {
      const string re_msg{ gd + "Player country/city data color (0-9), press Enter to accept default (2): " };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^2";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_geoinfo_color(color_code);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", is_append_message_to_richedit_control::yes, is_log_message::no);
  }

  write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
}

void strip_leading_and_trailing_quotes(std::string &data)
{
  if (data.length() >= 2U && data.front() == '"' && data.back() == '"') {
    data.erase(cbegin(data));
    data.pop_back();
  }
}

void replace_all_escaped_new_lines_with_new_lines(std::string &data)
{
  while (true) {
    const size_t start{ data.find("\\n") };
    if (string::npos == start) break;
    data.replace(start, 2, "\n");
  }
}

void log_message(const string &msg, const is_log_datetime is_log_current_date_time)
{
  ostringstream os;
  if (is_log_current_date_time == is_log_datetime::yes) {
    os << "[" << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}") << "] ";
  }
  os << msg;
  if (msg.back() != '\n') {
    os << endl;
  }
  {
    const string logged_msg{ os.str() };
    lock_guard lg{ log_data_mutex };
    main_app.log_message(logged_msg);
  }
}

string get_time_interval_info_string_for_seconds(const time_t seconds)
{
  constexpr time_t seconds_in_day{ 3600LL * 24 };
  const time_t days = seconds / seconds_in_day;
  const time_t hours = (seconds - days * seconds_in_day) / 3600;
  const time_t minutes = (seconds - (days * seconds_in_day + hours * 3600)) / 60;
  const time_t remaining_seconds = seconds - (days * seconds_in_day + hours * 3600 + minutes * 60);
  ostringstream oss;
  if (days != 0) {
    oss << days << (days != 1 ? " days" : " day");
    if (hours != 0 || minutes != 0)
      oss << " : ";
  }

  if (hours != 0) {
    oss << hours << (hours != 1 ? " hours" : " hour");
    if (minutes != 0)
      oss << " : ";
  }

  if (minutes != 0) {
    oss << minutes << (minutes != 1 ? " minutes" : " minute");
  }

  string result{ oss.str() };
  if (!result.empty())
    return result;
  if (remaining_seconds == 0)
    return "just now";
  oss << remaining_seconds << (remaining_seconds != 1 ? " seconds" : " second");
  return oss.str();
}

bool remove_dir_path_sep_char(char *dir_path)
{
  const size_t dir_path_len{ stl::helper::len(dir_path) };
  if (dir_path_len == 0)
    return false;
  size_t index{ dir_path_len - 1 };
  while (index > 0 && (dir_path[index] == '\\' || dir_path[index] == '/')) {
    dir_path[index] = '\0';
    --index;
  }

  return index < dir_path_len - 1;
}

void replace_forward_slash_with_backward_slash(std::string &path)
{
  if (path.starts_with("steam://"))
    return;

  string output;
  output.reserve(path.size());
  char prev{};
  for (size_t i{}; i < path.length(); ++i) {
    if ('/' == path[i] || '\\' == path[i]) {
      if (prev != '\\')
        output.push_back('\\');
      prev = '\\';
    } else {
      output.push_back(path[i]);
      prev = path[i];
    }
  }
  path = std::move(output);
}

bool check_if_file_path_exists(const char *file_path)
{
  const directory_entry dir_entry{ file_path };
  return dir_entry.exists();
}

size_t print_colored_text(HWND re_control, const char *text, const is_append_message_to_richedit_control print_to_richedit_control, const is_log_message log_to_file, const is_log_datetime is_log_current_date_time, const bool is_prevent_auto_vertical_scrolling, const bool is_remove_color_codes_for_log_message)
{
  const char *message{ text };
  size_t text_len{ stl::helper::len(text) };
  if (nullptr == text || 0U == text_len)
    return 0U;
  const char *last = text + text_len;
  size_t printed_chars_count{};
  bool is_last_char_new_line{ text[text_len - 1] == '\n' };

  if (print_to_richedit_control == is_append_message_to_richedit_control::yes) {

    string msg;
    COLORREF bg_color{ color::black };
    COLORREF fg_color{ color::white };
    set_rich_edit_control_colors(re_control, fg_color, bg_color);

    ostringstream os;
    string text_str;
    if (is_log_current_date_time == is_log_datetime::yes) {
      os << get_date_and_time_for_time_t("^7{DD}.{MM}.{Y} {hh}:{mm}:\n");
      os << text;
      text_str = os.str();
      text = text_str.c_str();
      last = text + text_str.length();
      text_len = text_str.length();
      is_last_char_new_line = '\n' == text[text_len - 1];
    }

    for (; *text; ++text) {
      if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') && (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3)) {
        text += 3;
        if (!msg.empty()) {
          append(re_control, msg.c_str(), is_prevent_auto_vertical_scrolling);
          msg.clear();
        }

        fg_color = rich_edit_colors.at(*text);
        set_rich_edit_control_colors(re_control, fg_color, bg_color);
        msg.push_back('^');
        msg.push_back(*text);
        printed_chars_count += 2;

      } else if (text + 2 <= last && *text == '^' && (*(text + 1) >= '0' && *(text + 1) <= '9')) {
        ++text;
        if (!msg.empty()) {
          append(re_control, msg.c_str(), is_prevent_auto_vertical_scrolling);
          msg.clear();
        }
        fg_color = rich_edit_colors.at(*text);
        set_rich_edit_control_colors(re_control, fg_color, bg_color);

      } else {
        msg.push_back(*text);
        ++printed_chars_count;
      }
    }

    if (!msg.empty()) {
      append(re_control, msg.c_str(), is_prevent_auto_vertical_scrolling);
      msg.clear();
    }

    if (!is_last_char_new_line) {
      append(re_control, "\n", is_prevent_auto_vertical_scrolling);
      ++printed_chars_count;
    }
  }

  if (log_to_file == is_log_message::yes) {
    string message_without_color_codes{ message };
    remove_all_color_codes(message_without_color_codes);
    if (message_without_color_codes.length() > 0 && message_without_color_codes.back() != '\n') {
      message_without_color_codes.push_back('\n');
    }
    log_message(message_without_color_codes, is_log_current_date_time);
  }

  return printed_chars_count;
}

size_t print_colored_text_to_grid_cell(HDC hdc, RECT &rect, const char *text, DWORD formatting_style)
{
  const size_t text_len{ stl::helper::len(text) };
  const char *last = text + text_len;
  size_t printed_chars_count{};

  string msg;
  COLORREF fg_color{ color::white };

  for (; *text; ++text) {
    if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') && (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3)) {
      text += 3;
      if (!msg.empty()) {
        RECT calculate_rect{ rect };
        DrawTextExA(hdc, msg.data(), msg.length(), &calculate_rect, DT_CALCRECT, nullptr);
        DrawTextExA(hdc, msg.data(), msg.length(), &rect, formatting_style, nullptr);
        rect.left += (calculate_rect.right - calculate_rect.left);
        msg.clear();
      }

      fg_color = colors.at(*text);
      SetTextColor(hdc, fg_color);
      msg.push_back('^');
      msg.push_back(*text);
      printed_chars_count += 2;

    } else if (text + 2 <= last && *text == '^' && (*(text + 1) >= '0' && *(text + 1) <= '9')) {
      ++text;
      if (!msg.empty()) {
        RECT calculate_rect{ rect };
        DrawTextExA(hdc, msg.data(), msg.length(), &calculate_rect, DT_CALCRECT, nullptr);
        DrawTextExA(hdc, msg.data(), msg.length(), &rect, formatting_style, nullptr);
        rect.left += (calculate_rect.right - calculate_rect.left);
        msg.clear();
      }
      fg_color = colors.at(*text);
      SetTextColor(hdc, fg_color);

    } else {
      msg.push_back(*text);
      ++printed_chars_count;
    }
  }

  if (!msg.empty()) {
    RECT calculate_rect{ rect };
    DrawTextExA(hdc, msg.data(), msg.length(), &calculate_rect, DT_CALCRECT, nullptr);
    DrawTextExA(hdc, msg.data(), msg.length(), &rect, formatting_style, nullptr);
    rect.left += (calculate_rect.right - calculate_rect.left);
    msg.clear();
  }

  return printed_chars_count;
}


bool get_confirmation_message_from_user(const char *message, const char *caption)
{
  const auto answer = MessageBox(app_handles.hwnd_main_window, message, caption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);
  return answer == IDYES;
}

bool check_if_user_wants_to_quit(const char *msg)
{

  return str_compare(msg, "q") == 0 || str_compare(msg, "!q") == 0 || str_compare(msg, "exit") == 0 || str_compare(msg, "quit") == 0 || str_compare(msg, "!exit") == 0 || str_compare(msg, "!quit") == 0;
}

void set_rich_edit_control_colors(HWND richEditCtrl, const COLORREF fg_color, const COLORREF bg_color, const char *font_face_name)
{
  CHARFORMAT2 cf{};
  cf.cbSize = sizeof(CHARFORMAT2);
  cf.dwMask = CFM_CHARSET | CFM_FACE | CFM_COLOR | CFM_BACKCOLOR | CFM_WEIGHT;
  str_copy(cf.szFaceName, font_face_name);
  cf.wWeight = 800;
  cf.bCharSet = RUSSIAN_CHARSET;
  cf.crTextColor = fg_color;
  cf.crBackColor = bg_color;

  SendMessage(richEditCtrl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

CHARFORMAT get_char_fmt(HWND hwnd, DWORD range)
{
  CHARFORMAT cf;
  SendMessage(hwnd, EM_GETCHARFORMAT, range, (LPARAM)&cf);
  return cf;
}
void set_char_fmt(HWND hwnd, const CHARFORMAT2 &cf, DWORD range)
{
  SendMessage(hwnd, EM_SETCHARFORMAT, range, (LPARAM)&cf);
}
void replace_sel(HWND hwnd, const char *str)
{
  SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)str);
}
void cursor_to_bottom(HWND hwnd)
{
  SendMessage(hwnd, EM_SETSEL, (WPARAM)-2, -1);
}
void scroll_to_beginning(HWND hwnd)
{
  SendMessage(hwnd, WM_VSCROLL, 0, 0);
  SendMessage(hwnd, WM_HSCROLL, SB_LEFT, 0);
}
void scroll_to(HWND hwnd, DWORD pos)
{
  SendMessage(hwnd, WM_VSCROLL, pos, 0);
}
void scroll_to_bottom(HWND hwnd)
{
  scroll_to(hwnd, SB_BOTTOM);
}


void append(HWND hwnd, const char *str, const bool is_prevent_auto_vertical_scrolling)
{
  lock_guard lg{ print_data_mutex };
  cursor_to_bottom(hwnd);
  SETTEXTEX stx{};
  stx.flags = ST_DEFAULT | ST_SELECTION | ST_NEWCHARS;
  stx.codepage = 1251;
  SendMessage(hwnd, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx), reinterpret_cast<LPARAM>(str));
  // replace_sel(hwnd, str);
  if (!is_prevent_auto_vertical_scrolling)
    scroll_to_bottom(hwnd);
  /* else
     cursor_to_bottom(hwnd);*/
}

void process_key_down_message(const MSG &msg)
{
  if (msg.wParam == VK_ESCAPE) {
    if (show_user_confirmation_dialog("^3Do you really want to quit?", "Exit program?", "Reason")) {
      is_terminate_program.store(true);
      {
        lock_guard<mutex> l{ mu };
        exit_flag.notify_all();
      }
      PostQuitMessage(0);
    }
  } else if (msg.wParam == VK_UP) {
    if (!commands_history.empty()) {
      if (commands_history_index > 0) {
        --commands_history_index;
        if (commands_history_index < commands_history.size()) {
          Edit_SetText(app_handles.hwnd_e_user_input, commands_history.at(commands_history_index).c_str());
          cursor_to_bottom(app_handles.hwnd_e_user_input);
        }
      }
    }
  } else if (msg.wParam == VK_DOWN) {
    if (!commands_history.empty()) {
      if (commands_history_index < commands_history.size() - 1) {
        ++commands_history_index;
        if (commands_history_index < commands_history.size()) {
          Edit_SetText(app_handles.hwnd_e_user_input, commands_history.at(commands_history_index).c_str());
          cursor_to_bottom(app_handles.hwnd_e_user_input);
        }
      }
    }
  } else if (app_handles.hwnd_e_user_input == msg.hwnd && msg.wParam == VK_RETURN) {
    if (get_user_input()) {
      is_terminate_program.store(true);
      {
        lock_guard<mutex> l{ mu };
        exit_flag.notify_all();
      }
      PostQuitMessage(0);
    }
  }
}


void construct_tinyrcon_gui(HWND hWnd)
{
  if (app_handles.hwnd_online_admins_information != NULL) {
    ShowWindow(app_handles.hwnd_online_admins_information, SW_HIDE);
    DestroyWindow(app_handles.hwnd_online_admins_information);
  }

  app_handles.hwnd_online_admins_information = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY, 10, 13, screen_width - 20, 30, hWnd, nullptr, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_online_admins_information)
    FatalAppExit(0, "Couldn't create 'app_handles.hwnd_online_admins_information' richedit control!");

  assert(app_handles.hwnd_online_admins_information != 0);
  SendMessage(app_handles.hwnd_online_admins_information, EM_SETBKGNDCOLOR, NULL, color::black);
  scroll_to_beginning(app_handles.hwnd_online_admins_information);
  set_rich_edit_control_colors(app_handles.hwnd_online_admins_information, color::white, color::black, "Lucida Console");
  if (g_online_admins_information.empty())
    Edit_SetText(app_handles.hwnd_online_admins_information, "");
  else
    print_colored_text(app_handles.hwnd_online_admins_information, g_online_admins_information.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);

  if (app_handles.hwnd_re_messages_data != NULL) {
    ShowWindow(app_handles.hwnd_re_messages_data, SW_HIDE);
    DestroyWindow(app_handles.hwnd_re_messages_data);
  }

  app_handles.hwnd_re_messages_data = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_LEFT /*| ES_AUTOVSCROLL*/ | ES_NOHIDESEL | ES_READONLY, 10, screen_height / 2 + 5, screen_width - 30, screen_height / 2 - 90, hWnd, reinterpret_cast<HMENU>(ID_BANNEDEDIT), app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_re_messages_data)
    FatalAppExit(0, "Couldn't create 'hwnd_re_messages_data' richedit control!");

  if (app_handles.hwnd_re_messages_data != 0) {
    SendMessage(app_handles.hwnd_re_messages_data, EM_SETBKGNDCOLOR, NULL, color::black);
    scroll_to_beginning(app_handles.hwnd_re_messages_data);
    set_rich_edit_control_colors(app_handles.hwnd_re_messages_data, color::white, color::black, "Lucida Console");
    if (g_message_data_contents.empty())
      Edit_SetText(app_handles.hwnd_re_messages_data, "");
    else
      print_colored_text(app_handles.hwnd_re_messages_data, g_message_data_contents.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  }

  app_handles.hwnd_e_user_input = CreateWindowEx(0, "Edit", nullptr, WS_GROUP | WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 130, screen_height - 75, screen_width - 150, 20, hWnd, reinterpret_cast<HMENU>(ID_USEREDIT), app_handles.hInstance, nullptr);

  if (app_handles.hwnd_users_table) {
    DestroyWindow(app_handles.hwnd_users_table);
  }

  app_handles.hwnd_users_table = CreateWindowExA(WS_EX_CLIENTEDGE, WC_SIMPLEGRIDA, "", WS_VISIBLE | WS_CHILD, 10, 55, screen_width - 30, screen_height / 2 - 65, hWnd, reinterpret_cast<HMENU>(501), app_handles.hInstance, nullptr);

  hImageList = ImageList_Create(32, 24, ILC_COLOR32, flag_name_index.size(), 1);
  for (size_t i{}; i < flag_name_index.size(); ++i) {
    HBITMAP hbmp = static_cast<HBITMAP>(LoadImage(app_handles.hInstance, MAKEINTRESOURCE(151 + i), IMAGE_BITMAP, 32, 24, LR_CREATEDIBSECTION | LR_DEFAULTSIZE));
    ImageList_Add(hImageList, hbmp, NULL);
  }

  initialize_users_grid(app_handles.hwnd_users_table, 18, max_users_grid_rows, true);

  UpdateWindow(hWnd);
}

void PutCell(HWND hgrid, const int row, const int col, const char *text)
{
  SGITEM item{};
  item.row = row;
  item.col = col;
  item.lpCurValue = (LPARAM)text;
  SimpleGrid_SetItemData(hgrid, &item);
}

void set_check_box(HWND hgrid, const int row, const int col, const BOOL is_checked)
{
  SGITEM item{};
  item.col = col;
  item.row = row;
  item.lpCurValue = (LPARAM)is_checked;
  SimpleGrid_SetItemData(hgrid, &item);
}

void display_country_flag(HWND hgrid, const int row, const int col, const char *country_code)
{
  SGITEM item{};
  item.col = col;
  item.row = row;
  const int country_flag_index{ flag_name_index.contains(country_code) ? flag_name_index.at(country_code) : 0 };
  item.lpCurValue = (LPARAM)country_flag_index;
  SimpleGrid_SetItemData(hgrid, &item);
}

string GetCellContents(HWND hgrid, const int row, const int col)
{
  static char cell_buffer[256]{};
  SGITEM item{};
  item.row = row;
  item.col = col;
  item.lpCurValue = (LPARAM)cell_buffer;
  SimpleGrid_GetItemData(hgrid, &item);
  return string{ cell_buffer };
}

void initialize_users_grid(HWND hgrid, const size_t cols, const size_t rows, const bool is_for_rcon_status)
{

  selected_row = 0;
  SimpleGrid_ResetContent(hgrid);
  SimpleGrid_SetAllowColResize(app_handles.hwnd_users_table, TRUE);
  SimpleGrid_Enable(app_handles.hwnd_users_table, TRUE);
  SimpleGrid_ExtendLastColumn(app_handles.hwnd_users_table, TRUE);
  SimpleGrid_SetColAutoWidth(app_handles.hwnd_users_table, TRUE);
  SimpleGrid_SetDoubleBuffer(app_handles.hwnd_users_table, TRUE);
  SimpleGrid_SetEllipsis(app_handles.hwnd_users_table, TRUE);
  SimpleGrid_SetGridLineColor(app_handles.hwnd_users_table, colors.at(main_app.get_game_server().get_border_line_color()[1]));
  SimpleGrid_SetTitleHeight(app_handles.hwnd_users_table, 0);
  SimpleGrid_SetHilightColor(app_handles.hwnd_users_table, color::grey);
  SimpleGrid_SetHilightTextColor(app_handles.hwnd_users_table, color::red);
  SimpleGrid_SetRowHeaderWidth(app_handles.hwnd_users_table, 0);
  SimpleGrid_SetColsNumbered(app_handles.hwnd_users_table, FALSE);
  SimpleGrid_SetRowHeight(app_handles.hwnd_users_table, 28);

  for (size_t col_id{}; col_id < cols; ++col_id) {
    SGCOLUMN column{};
    if (col_id >= 1 && col_id <= 3) {
      column.dwType = GCT_CHECK;
    } else if (6 == col_id) {
      column.dwType = GCT_IMAGE;
      column.pOptional = hImageList;
    } else {
      column.dwType = GCT_EDIT;
    }
    column.lpszHeader = users_table_column_header_titles.at(col_id).c_str();
    SimpleGrid_AddColumn(app_handles.hwnd_users_table, &column);
  }

  for (size_t i{}; i < rows; ++i) {
    SimpleGrid_AddRow(hgrid, "");
  }

  for (size_t i{}; i < rows; ++i) {
    for (size_t j{}; j < cols; ++j) {
      if (j >= 1 && j <= 3) {
        set_check_box(hgrid, i, j, FALSE);
      } else if (6 == j) {
        display_country_flag(hgrid, i, 6, "xy");
      } else {
        PutCell(hgrid, i, j, "");
      }
    }
  }

  Grid_OnSetFont(app_handles.hwnd_users_table, font_for_players_grid_data, TRUE);
  ShowHscroll(app_handles.hwnd_users_table);

  SimpleGrid_SetColWidth(hgrid, 0, 160);
  SimpleGrid_SetColWidth(hgrid, 1, 100);
  SimpleGrid_SetColWidth(hgrid, 2, 100);
  SimpleGrid_SetColWidth(hgrid, 3, 100);
  SimpleGrid_SetColWidth(hgrid, 4, 170);
  SimpleGrid_SetColWidth(hgrid, 5, 250);
  SimpleGrid_SetColWidth(hgrid, 6, 50);
  SimpleGrid_SetColWidth(hgrid, 7, 210);
  SimpleGrid_SetColWidth(hgrid, 8, 210);
  SimpleGrid_SetColWidth(hgrid, 9, 90);
  SimpleGrid_SetColWidth(hgrid, 10, 90);
  SimpleGrid_SetColWidth(hgrid, 11, 90);
  SimpleGrid_SetColWidth(hgrid, 12, 90);
  SimpleGrid_SetColWidth(hgrid, 13, 90);
  SimpleGrid_SetColWidth(hgrid, 14, 90);
  SimpleGrid_SetColWidth(hgrid, 15, 90);
  SimpleGrid_SetColWidth(hgrid, 16, 90);
  SimpleGrid_SetColWidth(hgrid, 17, 90);

  SimpleGrid_SetSelectionMode(app_handles.hwnd_users_table, GSO_FULLROW);
}

void clear_data_in_users_table(HWND hgrid, const size_t start_row, const size_t last_row, const size_t cols)
{
  for (size_t i{ start_row }; i < last_row; ++i) {
    for (size_t j{}; j < cols; ++j) {
      if (j >= 1 && j <= 3) {
        set_check_box(hgrid, i, j, FALSE);
      } else if (6 == j) {
        display_country_flag(hgrid, i, 6, "xy");
      } else {
        PutCell(hgrid, i, j, "");
      }
    }
  }
}

void display_users_data_in_users_table(HWND hgrid)
{
  const auto &users = main_app.get_users();
  char buffer[16];
  if (!users.empty()) {
    for (size_t i{}; i < users.size(); ++i) {
      PutCell(hgrid, i, 0, users[i]->user_name.c_str());
      // PutCell(hgrid, i, 1, (users[i]->is_admin ? "TRUE" : "FALSE"));
      set_check_box(hgrid, i, 1, users[i]->is_admin ? TRUE : FALSE);
      // PutCell(hgrid, i, 2, (users[i]->is_logged_in ? "TRUE" : "FALSE"));
      set_check_box(hgrid, i, 2, users[i]->is_logged_in ? TRUE : FALSE);
      // PutCell(hgrid, i, 3, (users[i]->is_online ? "TRUE" : "FALSE"));
      set_check_box(hgrid, i, 3, users[i]->is_online ? TRUE : FALSE);
      PutCell(hgrid, i, 4, users[i]->ip_address.c_str());
      PutCell(hgrid, i, 5, users[i]->geo_information.c_str());
      display_country_flag(hgrid, i, 6, users[i]->country_code);
      const string last_login_date_time = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", users[i]->last_login_time_stamp);
      PutCell(hgrid, i, 7, last_login_date_time.c_str());
      const string last_logout_date_time = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", users[i]->last_logout_time_stamp);
      PutCell(hgrid, i, 8, last_logout_date_time.c_str());
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_logins);
      PutCell(hgrid, i, 9, buffer);
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_warnings);
      PutCell(hgrid, i, 10, buffer);
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_kicks);
      PutCell(hgrid, i, 11, buffer);
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_tempbans);
      PutCell(hgrid, i, 12, buffer);
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_guidbans);
      PutCell(hgrid, i, 13, buffer);
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_ipbans);
      PutCell(hgrid, i, 14, buffer);
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_iprangebans);
      PutCell(hgrid, i, 15, buffer);
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_citybans);
      PutCell(hgrid, i, 16, buffer);
      snprintf(buffer, std::size(buffer), "%lu", users[i]->no_of_countrybans);
      PutCell(hgrid, i, 17, buffer);
    }
  }

  // print_colored_text(app_handles.hwnd_online_admins_information, g_online_admins_information.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  // SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
  // SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);

  clear_data_in_users_table(hgrid, users.size(), max_users_grid_rows, 18);
  // const int users_table_width{ screen_width - 20 };
  // const int user_name_column_width{ findLongestTextWidthInColumn(hgrid, 0) };
  SimpleGrid_SetColWidth(hgrid, 0, 160);
  SimpleGrid_SetColWidth(hgrid, 1, 100);
  SimpleGrid_SetColWidth(hgrid, 2, 100);
  SimpleGrid_SetColWidth(hgrid, 3, 100);
  SimpleGrid_SetColWidth(hgrid, 4, 170);
  SimpleGrid_SetColWidth(hgrid, 5, 250);
  SimpleGrid_SetColWidth(hgrid, 6, 50);
  SimpleGrid_SetColWidth(hgrid, 7, 210);
  SimpleGrid_SetColWidth(hgrid, 8, 210);
  SimpleGrid_SetColWidth(hgrid, 9, 90);
  SimpleGrid_SetColWidth(hgrid, 10, 90);
  SimpleGrid_SetColWidth(hgrid, 11, 90);
  SimpleGrid_SetColWidth(hgrid, 12, 90);
  SimpleGrid_SetColWidth(hgrid, 13, 90);
  SimpleGrid_SetColWidth(hgrid, 14, 90);
  SimpleGrid_SetColWidth(hgrid, 15, 90);
  SimpleGrid_SetColWidth(hgrid, 16, 90);
  SimpleGrid_SetColWidth(hgrid, 17, 90);
  SimpleGrid_EnableEdit(hgrid, FALSE);
}

bool is_alpha(const char ch)
{
  return !is_decimal_digit(ch) && !is_ws(ch);
}

bool is_decimal_digit(const char ch)
{
  return ch >= '0' && ch <= '9';
}

bool is_ws(const char ch)
{
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\f' || ch == '\v';
}

void change_hdc_fg_color(HDC hdc, COLORREF fg_color)
{
  SetTextColor(hdc, fg_color);
}

bool check_if_selected_cell_indices_are_valid(const int row_index, const int col_index)
{
  return row_index >= 0 && row_index < (int)main_app.get_game_server().get_number_of_players() && col_index >= 0 && col_index < 7;
}

void CenterWindow(HWND hwnd)
{
  RECT rc{};

  GetWindowRect(hwnd, &rc);
  const int win_w{ rc.right - rc.left };
  const int win_h{ rc.bottom - rc.top };

  const int screen_w{ GetSystemMetrics(SM_CXSCREEN) };
  const int screen_h{ GetSystemMetrics(SM_CYSCREEN) };

  SetWindowPos(hwnd, HWND_TOP, (screen_w - win_w) / 2, (screen_h - win_h) / 2, 0, 0, SWP_NOSIZE);
}

bool show_user_confirmation_dialog(const char *msg, const char *title, const char *)
{
  static char msg_buffer[1024];
  admin_choice.store(0);
  if (app_handles.hwnd_confirmation_dialog) {
    DestroyWindow(app_handles.hwnd_confirmation_dialog);
  }
  app_handles.hwnd_confirmation_dialog = CreateWindowEx(NULL, wcex_confirmation_dialog.lpszClassName, title, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME /*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX*/, 0, 0, 600, 320, app_handles.hwnd_main_window, nullptr, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_confirmation_dialog)
    return false;

  if (app_handles.hwnd_re_confirmation_message) {
    DestroyWindow(app_handles.hwnd_re_confirmation_message);
  }

  app_handles.hwnd_re_confirmation_message = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_LEFT | ES_AUTOVSCROLL | ES_READONLY, 10, 10, 560, 200, app_handles.hwnd_confirmation_dialog, nullptr, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_re_confirmation_message)
    return false;

  assert(app_handles.hwnd_re_confirmation_message != 0);
  SendMessage(app_handles.hwnd_re_confirmation_message, EM_SETBKGNDCOLOR, NULL, color::black);
  SendMessage(app_handles.hwnd_re_confirmation_message, EM_SETBKGNDCOLOR, NULL, color::black);
  SendMessage(app_handles.hwnd_re_confirmation_message, EM_SETWORDWRAPMODE, WBF_WORDWRAP, 0);
  SendMessage(app_handles.hwnd_re_confirmation_message, EM_SETWORDWRAPMODE, WBF_WORDBREAK, 0);
  SendMessage(app_handles.hwnd_re_confirmation_message, EM_SETWORDWRAPMODE, WBF_WORDWRAP | WBF_WORDBREAK, 0);
  scroll_to_beginning(app_handles.hwnd_re_confirmation_message);
  set_rich_edit_control_colors(app_handles.hwnd_re_confirmation_message, color::white, color::black);

  if (app_handles.hwnd_e_reason) {
    DestroyWindow(app_handles.hwnd_e_reason);
  }
  app_handles.hwnd_e_reason = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 100, 220, 470, 20, app_handles.hwnd_confirmation_dialog, reinterpret_cast<HMENU>(ID_EDIT_ADMIN_REASON), app_handles.hInstance, nullptr);

  if (app_handles.hwnd_yes_button) {
    DestroyWindow(app_handles.hwnd_yes_button);
  }
  app_handles.hwnd_yes_button = CreateWindowEx(NULL, "Button", "Yes", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 100, 250, 50, 20, app_handles.hwnd_confirmation_dialog, reinterpret_cast<HMENU>(ID_YES_BUTTON), app_handles.hInstance, NULL);

  if (app_handles.hwnd_no_button) {
    DestroyWindow(app_handles.hwnd_no_button);
  }
  app_handles.hwnd_no_button = CreateWindowEx(NULL, "Button", "No", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 170, 250, 50, 20, app_handles.hwnd_confirmation_dialog, reinterpret_cast<HMENU>(ID_NO_BUTTON), app_handles.hInstance, NULL);

  print_colored_text(app_handles.hwnd_re_confirmation_message, msg, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  Edit_SetText(app_handles.hwnd_e_reason, admin_reason.c_str());
  CenterWindow(app_handles.hwnd_confirmation_dialog);
  ShowWindow(app_handles.hwnd_confirmation_dialog, SW_SHOW);
  SetFocus(app_handles.hwnd_no_button);

  MSG wnd_msg{};
  while (GetMessage(&wnd_msg, NULL, NULL, NULL) != 0) {
    TranslateMessage(&wnd_msg);
    if (WM_KEYDOWN == wnd_msg.message && VK_ESCAPE == wnd_msg.wParam) {
      admin_choice.store(0);
      admin_reason.assign("not specified");
      EnableWindow(app_handles.hwnd_main_window, TRUE);
      SetFocus(app_handles.hwnd_e_user_input);
      DestroyWindow(app_handles.hwnd_confirmation_dialog);

    } else if (WM_KEYDOWN == wnd_msg.message && VK_RETURN == wnd_msg.wParam) {
      if (GetFocus() == app_handles.hwnd_no_button) {
        admin_choice.store(0);
        admin_reason.assign("not specified");
        EnableWindow(app_handles.hwnd_main_window, TRUE);
        SetFocus(app_handles.hwnd_e_user_input);
        DestroyWindow(app_handles.hwnd_confirmation_dialog);
      } else if (GetFocus() == app_handles.hwnd_yes_button) {
        admin_choice.store(1);
        Edit_GetText(app_handles.hwnd_e_reason, msg_buffer, std::size(msg_buffer));
        if (stl::helper::len(msg_buffer) > 0) {
          admin_reason.assign(msg_buffer);
        } else {
          admin_reason.assign("not specified");
        }
        EnableWindow(app_handles.hwnd_main_window, TRUE);
        SetFocus(app_handles.hwnd_e_user_input);
        DestroyWindow(app_handles.hwnd_confirmation_dialog);
      }
    } else if (WM_KEYDOWN == wnd_msg.message && VK_LEFT == wnd_msg.wParam) {
      const auto focused_hwnd = GetFocus();
      if (focused_hwnd == app_handles.hwnd_yes_button)
        SetFocus(app_handles.hwnd_no_button);
      else if (focused_hwnd == app_handles.hwnd_no_button)
        SetFocus(app_handles.hwnd_yes_button);
    } else if (WM_KEYDOWN == wnd_msg.message && VK_TAB == wnd_msg.wParam) {
      const auto focused_hwnd = GetFocus();
      if (focused_hwnd == app_handles.hwnd_yes_button)
        SetFocus(app_handles.hwnd_no_button);
      else if (focused_hwnd == app_handles.hwnd_no_button)
        SetFocus(app_handles.hwnd_yes_button);
    } else if (WM_KEYDOWN == wnd_msg.message && VK_RIGHT == wnd_msg.wParam) {
      const auto focused_hwnd = GetFocus();
      if (focused_hwnd == app_handles.hwnd_yes_button)
        SetFocus(app_handles.hwnd_no_button);
      else if (focused_hwnd == app_handles.hwnd_no_button)
        SetFocus(app_handles.hwnd_yes_button);
    } else if (WM_LBUTTONDOWN == wnd_msg.message && is_first_left_mouse_button_click_in_reason_edit_control) {
      const int x{ GET_X_LPARAM(wnd_msg.lParam) };
      const int y{ GET_Y_LPARAM(wnd_msg.lParam) };
      RECT rect{};
      GetClientRect(app_handles.hwnd_e_reason, &rect);
      if (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom) {
        Edit_SetText(app_handles.hwnd_e_reason, "");
        admin_reason.assign("not specified");
        is_first_left_mouse_button_click_in_reason_edit_control = false;
      }
    }
    DispatchMessage(&wnd_msg);
  }

  return wnd_msg.wParam != 0;
}

void display_context_menu_over_grid(HWND control, const int mouse_x, const int mouse_y)
{

  HMENU hPopupMenu = CreatePopupMenu();

  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWTEMPBANSBUTTON, "View temporarily banned IP addresses");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPBANSBUTTON, "View permanently banned IP addresses");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCITIES, "View banned cities");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCOUNTRIES, "View banned countries");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSES, "View protected IP addresses");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSRANGES, "View protected IP address ranges");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCITIES, "View protected cities");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCOUNTRIES, "View protected countries");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "&Clear messages");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
  SetForegroundWindow(app_handles.hwnd_main_window);
  POINT p{ mouse_x, mouse_y };
  ClientToScreen(control, &p);
  TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, p.x, p.y, 0, app_handles.hwnd_main_window, NULL);
}


size_t get_file_size_in_bytes(const char *file_path)
{

  ifstream input{ file_path, std::ios::binary | std::ios::in | std::ios::ate };
  if (!input) {
    return 0;
  }
  const std::fstream::pos_type file_size = input.tellg();
  return static_cast<size_t>(file_size);
}

std::string get_date_and_time_for_time_t(const char *date_time_format_str, time_t t_c)
{
  static constexpr const char *time_formatters[]{
    "{Y}", "{MMM}", "{MM}", "{M}", "{D}", "{DD}", "{h}", "{hh}", "{m}", "{mm}", "{s}", "{ss}"
  };

  if (0 == t_c) {
    const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();
    t_c = std::chrono::system_clock::to_time_t(now);
  }
  tm time_info{};
  localtime_s(&time_info, &t_c);
  string time_format_str{ date_time_format_str };
  char element[16];
  for (const auto &fmt : time_formatters) {
    const auto key_start = time_format_str.find(fmt);
    if (key_start != string::npos) {
      if (strcmp(fmt, "{Y}") == 0) {
        time_format_str.replace(key_start, 3, to_string(time_info.tm_year + 1900));
      } else if (strcmp(fmt, "{MMM}") == 0) {
        time_format_str.replace(key_start, 5, get_current_short_month_name(time_info.tm_mon + 1));
      } else if (strcmp(fmt, "{MM}") == 0) {
        snprintf(element, 16, "%02d", time_info.tm_mon + 1);
        time_format_str.replace(key_start, 4, element);
      } else if (strcmp(fmt, "{M}") == 0) {
        snprintf(element, 16, "%d", time_info.tm_mon + 1);
        time_format_str.replace(key_start, 3, element);
      } else if (strcmp(fmt, "{D}") == 0) {
        time_format_str.replace(key_start, 3, to_string(time_info.tm_mday));
      } else if (strcmp(fmt, "{DD}") == 0) {
        snprintf(element, 16, "%02d", time_info.tm_mday);
        time_format_str.replace(key_start, 4, element);
      } else if (strcmp(fmt, "{h}") == 0) {
        time_format_str.replace(key_start, 3, to_string(time_info.tm_hour));
      } else if (strcmp(fmt, "{hh}") == 0) {
        snprintf(element, 16, "%02d", time_info.tm_hour);
        time_format_str.replace(key_start, 4, element);
      } else if (strcmp(fmt, "{m}") == 0) {
        time_format_str.replace(key_start, 3, to_string(time_info.tm_min));
      } else if (strcmp(fmt, "{mm}") == 0) {
        snprintf(element, 16, "%02d", time_info.tm_min);
        time_format_str.replace(key_start, 4, element);
      } else if (strcmp(fmt, "{s}") == 0) {
        time_format_str.replace(key_start, 3, to_string(time_info.tm_sec));
      } else if (strcmp(fmt, "{ss}") == 0) {
        snprintf(element, 16, "%02d", time_info.tm_sec);
        time_format_str.replace(key_start, 4, element);
      }
    }
  }

  return time_format_str;
}

const char *get_current_short_month_name(const size_t month_index)
{

  switch (month_index) {
  case 1:
    return "Jan";
  case 2:
    return "Feb";
  case 3:
    return "Mar";
  case 4:
    return "Apr";
  case 5:
    return "May";
  case 6:
    return "Jun";
  case 7:
    return "Jul";
  case 8:
    return "Aug";
  case 9:
    return "Sep";
  case 10:
    return "Oct";
  case 11:
    return "Nov";
  case 12:
    return "Dec";
  default:
    return "Unknown month";
  }
}

std::pair<bool, std::string> create_7z_file_file_at_specified_path(const std::vector<std::string> &files_to_add, const std::string &local_file_path)
{
  try {

    DeleteFileA(local_file_path.c_str());
    using namespace bit7z;
    Bit7zLibrary lib{ "7za.dll" };
    BitFileCompressor compressor{ lib, BitFormat::SevenZip };
    compressor.setOverwriteMode(bit7z::OverwriteMode::Overwrite);
    compressor.compress(files_to_add, local_file_path);
    return make_pair(true, string{ "SUCCESS" });

  } catch (const std::exception &ex) {
    return make_pair(false, ex.what());
  }
}

std::pair<bool, std::string> extract_7z_file_to_specified_path(const char *compressed_7z_file_path, const char *destination_path)
{
  try {
    using namespace bit7z;
    Bit7zLibrary lib{ "7za.dll" };
    BitFileExtractor extractor{ lib, BitFormat::SevenZip };
    extractor.setOverwriteMode(bit7z::OverwriteMode::Overwrite);
    extractor.extract(compressed_7z_file_path, destination_path);// extracting a simple archive
    return make_pair(true, string{});

  } catch (const std::exception &ex) {
    return make_pair(false, ex.what());
  }
}

void save_banned_entries_to_file(const char *file_path, const set<string> &banned_entries)
{
  ofstream output_file(file_path, std::ios::out);

  for (const auto &banned_entry : banned_entries) {
    output_file << banned_entry << '\n';
  }

  output_file << flush;
  output_file.close();
}

void display_banned_cities(const std::set<std::string> &banned_cities)
{

  ostringstream oss;
  if (banned_cities.empty()) {
    oss << "\n^3You haven't banned any ^1cities ^3yet.\n";
  } else {
    oss << "\n^5Banned cities are:\n";

    for (const auto &banned_city : banned_cities) {
      oss << "^1" << banned_city << '\n';
    }
  }
  const string information{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, information.c_str());
}

void display_banned_countries(const std::set<std::string> &banned_countries)
{
  ostringstream oss;
  if (banned_countries.empty()) {
    oss << "\n^3You haven't banned any ^1countries ^3yet.\n";
  } else {
    oss << "\n^5Banned countries are:\n";

    for (const auto &banned_country : banned_countries) {
      oss << "^1" << banned_country << '\n';
    }
  }
  const string information{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, information.c_str());
}

time_t get_current_time_stamp()
{
  const std::chrono::time_point<std::chrono::system_clock> now =
    std::chrono::system_clock::now();
  return std::chrono::system_clock::to_time_t(now);
}

time_t get_number_of_seconds_from_date_and_time_string(const std::string &date_and_time)
{
  std::tm t{};
  static const regex date_format1{ R"((\d{1,2})/(\d{1,2})/(\d{4})\s+(\d{1,2}):(\d{1,2}))" };
  static const regex date_format2{ R"((\d{1,2})\.(\d{1,2})\.(\d{4})\s+(\d{1,2}):(\d{1,2}))" };
  static const regex date_format3{ R"((\d{4})-(\w+)-(\d{1,2})\s+(\d{1,2}):(\d{1,2}))" };
  static const unordered_map<string, int> short_month_name_index{
    { "Jan", 0 },
    { "Feb", 1 },
    { "Mar", 2 },
    { "Apr", 3 },
    { "May", 4 },
    { "Jun", 5 },
    { "Jul", 6 },
    { "Aug", 7 },
    { "Sep", 8 },
    { "Oct", 9 },
    { "Nov", 10 },
    { "Dec", 11 },
  };
  smatch matches1{}, matches2{}, matches3{};
  string day, month, hour, minute;

  string ex_msg{ format(R"(^1Exception ^3thrown from ^1time_t get_number_of_seconds_from_date_and_time_string("{}"))", date_and_time) };
  stack_trace_element ste{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };

  if (regex_match(date_and_time, matches1, date_format1)) {
    day = matches1[1].str();
    if (!day.empty() && '0' == day[0]) day.erase(0, 1);
    t.tm_mday = stoi(day);
    month = matches1[2].str();
    if (!month.empty() && '0' == month[0]) month.erase(0, 1);
    t.tm_mon = stoi(month) - 1;
    t.tm_year = stoi(matches1[3].str()) - 1900;

    hour = matches1[4].str();
    if (!hour.empty() && '0' == hour[0]) hour.erase(0, 1);
    t.tm_hour = stoi(hour);

    minute = matches1[5].str();
    if (!minute.empty() && '0' == minute[0]) minute.erase(0, 1);
    t.tm_min = stoi(minute);
  } else if (regex_match(date_and_time, matches2, date_format2)) {
    day = matches2[1].str();
    if (!day.empty() && '0' == day[0]) day.erase(0, 1);
    t.tm_mday = stoi(day);
    month = matches2[2].str();
    if (!month.empty() && '0' == month[0]) month.erase(0, 1);
    t.tm_mon = stoi(month) - 1;
    t.tm_year = stoi(matches2[3].str()) - 1900;

    hour = matches2[4].str();
    if (!hour.empty() && '0' == hour[0]) hour.erase(0, 1);
    t.tm_hour = stoi(hour);

    minute = matches2[5].str();
    if (!minute.empty() && '0' == minute[0]) minute.erase(0, 1);
    t.tm_min = stoi(minute);
  } else if (regex_match(date_and_time, matches3, date_format3)) {
    t.tm_year = stoi(matches3[1].str()) - 1900;
    month = matches3[2].str();
    if (short_month_name_index.contains(month))
      t.tm_mon = short_month_name_index.at(month);
    else
      t.tm_mon = 0;
    day = matches3[3].str();
    if (!day.empty() && '0' == day[0]) day.erase(0, 1);
    t.tm_mday = stoi(day);

    hour = matches3[4].str();
    if (!hour.empty() && '0' == hour[0]) hour.erase(0, 1);
    t.tm_hour = stoi(hour);

    minute = matches3[5].str();
    if (!minute.empty() && '0' == minute[0]) minute.erase(0, 1);
    t.tm_min = stoi(minute);
  }

  return mktime(&t);
}

void display_online_admins_information()
{
  ostringstream oss;

  oss << "^2Online admins: ";

  if (!main_app.get_users().empty()) {

    for (auto &u : main_app.get_users()) {
      if (u->is_online) {
        oss << format("^7{}^7, ", u->user_name);
      }
    }
  }

  string information{ oss.str() };
  if (' ' == information.back())
    information.pop_back();
  if (',' == information.back())
    information.pop_back();

  Edit_SetText(app_handles.hwnd_online_admins_information, "");
  print_colored_text(app_handles.hwnd_online_admins_information, information.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
  SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);
}

void removed_disallowed_character_in_string(std::string &input)
{
  // const string disallowed_chars{ "\\/-[]\"'{|}%:;=+\b\f\n\r\t " };
  const string disallowed_chars{ "\\" };
  const std::unordered_set<char> disallowed_characters(cbegin(disallowed_chars), cend(disallowed_chars));
  string cleaned_input{};
  for (const char ch : input) {
    if (!disallowed_characters.contains(ch))
      cleaned_input.push_back(ch);
  }
  input = std::move(cleaned_input);
}

std::string remove_disallowed_character_in_string(const std::string &input)
{
  // const string disallowed_chars{ "\\/-[]\"'{|}%:;=+\b\f\n\r\t " };
  const string disallowed_chars{ "\\[]:" };
  const std::unordered_set<char> disallowed_characters(cbegin(disallowed_chars), cend(disallowed_chars));
  string cleaned_input{};
  for (const char ch : input) {
    if (!disallowed_characters.contains(ch))
      cleaned_input.push_back(ch);
    else
      cleaned_input.push_back('_');
  }

  return cleaned_input;
}


std::string get_cleaned_user_name(const std::string &name)
{
  string admin_name{ remove_disallowed_character_in_string(name) };
  remove_all_color_codes(admin_name);
  trim_in_place(admin_name);
  to_lower_case_in_place(admin_name);
  // const string info_message{ format("^5Cleaned name for ^1admin '{}' ^5is ^1'{}'\n", name, admin_name) };
  // print_colored_text(app_handles.hwnd_re_messages_data, info_message.c_str());
  return admin_name;
}

void replace_br_with_new_line(std::string &message)
{
  for (size_t next{ string::npos }; (next = message.find("{{br}}")) != string::npos;) {
    message.replace(next, 6, "\n");
  }
}

bool check_if_player_is_protected(const player &online_player, const char *admin_command, string &message)
{
  bool is_ip_protected{};
  bool is_ip_range_protected{};
  unsigned long guid{};
  if (check_ip_address_validity({ online_player.ip_address, len(online_player.ip_address) }, guid)) {
    const string narrow_ip_address_range{ get_narrow_ip_address_range_for_specified_ip_address(online_player.ip_address) };
    const string wide_ip_address_range{ get_wide_ip_address_range_for_specified_ip_address(online_player.ip_address) };
    is_ip_protected = main_app.get_game_server().get_protected_ip_addresses().contains(online_player.ip_address);
    is_ip_range_protected = main_app.get_game_server().get_protected_ip_address_ranges().contains(narrow_ip_address_range) || main_app.get_game_server().get_protected_ip_address_ranges().contains(wide_ip_address_range);

    if (is_ip_protected) {
      message.assign(format("^3Discarded ^1{} ^3player ^7{} ^3whose IP address ^1{} ^3is protected! ^5Please, remove their ^1protected IP address ^5first.", admin_command, online_player.player_name, online_player.ip_address));
      return true;
    }

    if (is_ip_range_protected) {
      message.assign(format("^3Discarded ^1{} ^3player ^7{} ^3whose IP address range ^1{} ^3is protected! ^5Please, remove their ^1protected IP address range ^5first.", admin_command, online_player.player_name, main_app.get_game_server().get_protected_ip_address_ranges().contains(narrow_ip_address_range) ? narrow_ip_address_range : wide_ip_address_range));
      return true;
    }
  }
  const bool is_city_protected{ len(online_player.city) != 0 && main_app.get_game_server().get_protected_cities().contains(online_player.city) };
  const bool is_country_protected{ len(online_player.country_name) != 0 && main_app.get_game_server().get_protected_countries().contains(online_player.country_name) };


  if (is_city_protected) {
    message.assign(format("^3Discarded ^1{} ^3player ^7{} ^3whose city ^1{} ^3is protected! ^5Please, remove their ^1protected city ^5first.", admin_command, online_player.player_name, online_player.city));
    return true;
  }

  if (is_country_protected) {

    message.assign(format("^3Discarded ^1{} ^3player ^7{} ^3whose country ^1{} ^3is protected! ^5Please, remove their ^1protected country ^5first.", admin_command, online_player.player_name, online_player.country_name));
    return true;
  }

  return false;
}

std::string get_narrow_ip_address_range_for_specified_ip_address(const std::string &ip_address)
{
  static const regex ip_range_regex{ R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\*$)" };
  smatch matches;
  if (regex_match(ip_address, matches, ip_range_regex))
    return ip_address;
  unsigned long guid{};
  if (!check_ip_address_validity(ip_address, guid))
    return { "n/a" };
  size_t last{ ip_address.length() - 1 };
  while (ip_address[last] != '.') --last;
  return { string(ip_address.cbegin(), ip_address.cbegin() + last) + ".*" };
}

std::string get_wide_ip_address_range_for_specified_ip_address(const std::string &ip_address)
{
  static const regex ip_range_regex{ R"(^\d{1,3}\.\d{1,3}\.\*\.\*$)" };
  smatch matches;
  if (regex_match(ip_address, matches, ip_range_regex))
    return ip_address;
  unsigned long guid{};
  if (!check_ip_address_validity(ip_address, guid))
    return { "n/a" };
  size_t last{ ip_address.length() - 1 };

  while (ip_address[last] != '.') --last;
  --last;
  while (ip_address[last] != '.') --last;
  return { string(ip_address.cbegin(), ip_address.cbegin() + last) + ".*.*" };
}

void get_first_valid_ip_address_from_ip_address_range(std::string ip_range, player &pd)
{

  if (ip_range.ends_with("*.*")) {
    ip_range.replace(ip_range.length() - 3, 3, "1.1");
  } else if (ip_range.ends_with(".*")) {
    ip_range.back() = '1';
  }

  strcpy_s(pd.ip_address, std::size(pd.ip_address), ip_range.c_str());
  convert_guid_key_to_country_name(
    main_app.get_connection_manager_for_messages().get_geoip_data(),
    ip_range,
    pd);
}

size_t get_random_number()
{
  static std::mt19937_64 rand_engine(get_current_time_stamp());
  static std::uniform_int_distribution<size_t> number_range(1000ULL, std::numeric_limits<size_t>::max());
  return number_range(rand_engine);
}

size_t find_longest_user_name_length(
  const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users,
  const bool count_color_codes,
  const size_t number_of_users_to_process) noexcept
{
  size_t max_player_name_length{ 8 };

  if (0 == number_of_users_to_process)
    return max_player_name_length;

  for (size_t i{}; i < number_of_users_to_process; ++i) {
    max_player_name_length =
      max(count_color_codes ? users[i]->user_name.length() : get_number_of_characters_without_color_codes(users[i]->user_name.c_str()), max_player_name_length);
  }

  return max_player_name_length;
}

size_t find_longest_user_country_city_info_length(
  const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users,
  const size_t number_of_users_to_process) noexcept
{
  size_t max_geodata_info_length{ 20 };

  if (0 == number_of_users_to_process)
    return max_geodata_info_length;

  for (size_t i{}; i < number_of_users_to_process; ++i) {
    const size_t current_player_geodata_info_length{ users[i]->geo_information.length() };
    max_geodata_info_length = std::max(current_player_geodata_info_length, max_geodata_info_length);
  }

  return max_geodata_info_length;
}
