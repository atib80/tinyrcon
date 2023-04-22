#include "tiny_cod2_rcon_client_application.h"
#include "game_server.h"
#include "stl_helper_functions.hpp"
#include <fstream>
#include <iomanip>
#include <Psapi.h>
#include <regex>
#include <Shlobj.h>
#include <windowsx.h>
#include "json_parser.hpp"
#include "resource.h"
#include "simple_grid.h"

// Call of duty steam appid: 2620
// Call of duty 2 steam appid: 2630
// Call of duty 4: Modern Warfare steam appid: 7940
// Call of duty 5: World at war steam appid: 10090

using namespace std;
using namespace stl::helper;

extern tiny_cod2_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;
extern char const *const tempbans_file_path;
extern char const *const banned_ip_addresses_file_path;
extern PROCESS_INFORMATION pr_info;
extern sort_type type_of_sort;

extern WNDCLASSEX wcex_confirmation_dialog, wcex_configuration_dialog;
extern HFONT hfontbody;

extern const int screen_width;
extern const int screen_height;
extern int selected_row;
extern int selected_col;
extern const char *user_help_message;
extern const size_t max_players_grid_rows{ 64 };
volatile std::atomic<bool> is_terminate_program{ false };
volatile std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window{ false };
// extern volatile std::atomic<bool> is_refreshing_players_data;
extern volatile std::atomic<bool> is_refresh_players_data_event;
extern volatile std::atomic<bool> is_display_permanently_banned_players_data_event;
extern volatile std::atomic<bool> is_display_temporarily_banned_players_data_event;
extern std::atomic<int> admin_choice;
extern std::string admin_reason;

extern mutex mu;
extern condition_variable exit_flag;

mutex print_data_mutex;
mutex log_data_mutex;
mutex protect_get_user_input;

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

string g_re_match_information_contents;
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
  { "Sort by geo information in ascending order", sort_type::geo_asc },
  { "Sort by geo information in descending order", sort_type::geo_desc }
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
  { sort_type::geo_asc, "Sort by geo information in ascending order" },
  { sort_type::geo_desc, "Sort by geo information in descending order" }
};

extern const std::unordered_map<int, sort_type> sort_mode_col_index{ { 0, sort_type::pid_asc },
  { 1, sort_type::score_asc },
  { 2, sort_type::ping_asc },
  { 3, sort_type::name_asc },
  { 4, sort_type::ip_asc },
  { 5, sort_type::geo_asc } };

static constexpr const char *black_bg_color{ "[[!black!]]" };
static constexpr const char *grey_bg_color{ "[[!grey!]]" };
static constexpr size_t black_bg_color_length{ stl::helper::len(black_bg_color) };
static constexpr size_t grey_bg_color_length{ stl::helper::len(grey_bg_color) };

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
  { "!border", "Turns ^3on^5|^3off border lines around displayed ^3GUI controls^5." }
};

extern const unordered_set<string> user_commands_set{
  "!c",
  "!cp",
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
  "!border"
};

extern const unordered_set<string> rcon_status_commands{ "s", "!s", "status", "!status", "gs", "!gs", "getstatus", "!getstatus" };

extern const map<string, string> rcon_commands_help{
  { "status",
    "^5status ^2-> Retrieves current game state (players' data) from the server "
    "(pid, score, ping, name, guid, ip, qport, rate" },
  { "clientkick",
    "^5clientkick player_pid [reason] ^2-> Kicks player whose pid number is equal to "
    "specified player_pid." },
  { "kick",
    "^5kick name [reason] ^2-> Kicks player whose name is equal to specified name." },
  { "onlykick",
    "^5onlykick name_no_color_codes [reason] ^2-> Kicks player whose name is equal to "
    "specified name_no_color_codes." },
  { "banclient",
    "^5banclient player_pid [reason] ^2-> Bans player whose pid number is equal to "
    "specified player_pid." },
  { "banuser",
    "^5banuser name [reason] ^2-> Bans player whose name is equal to specified name." },
  { "tempbanclient",
    "^5tempbanclient player_pid [reason] ^2-> Temporarily bans player whose pid number is "
    "equal to specified player_pid." },
  { "tempbanuser",
    "^5tempbanuser name [reason] ^2-> Temporarily bans player whose name is equal to "
    "specified name." },
  { "serverinfo", "^5serverinfo ^2-> Retrieves currently active server settings." },
  { "map_rotate", "^5map_rotate ^2-> Loads next map on the server." },
  { "map_restart",
    "^5map_restart ^2-> Reloads currently played map on the server." },
  { "map", "^5map map_name ^2-> Immediately loads map 'map_name' on the server." },
  { "fast_restart",
    "^5fast_restart ^2-> Quickly restarts currently played map on the server." },
  { "getinfo",
    "^5getinfo ^2-> Retrieves basic server information from the server." },
  { "mapname",
    "^5mapname ^2-> Retrieves and displays currently played map's rcon name on "
    "the console." },
  { "g_gametype",
    "^5g_gametype ^2-> Retrieves and displays currently played match's gametype "
    "on the console." },
  { "sv_maprotation",
    "^5sv_maprotation ^2-> Retrieves and displays server's original map rotation "
    "setting." },
  { "sv_maprotationcurrent",
    "^5sv_maprotationcurrent ^2-> Retrieves and displays server's current map "
    "rotation setting." },
  { "say", "^5say \"public message to all players\" ^2-> Sends \"public message to all players\" on the server." },
  { "tell", "^5tell player_pid \"private message\" ^2-> Sends a private message to player whose pid = player_pid" }
};

extern const unordered_set<string> rcon_commands_set{ "s",
  "status",
  "clientkick",
  "kick",
  "onlykick",
  "banclient",
  "banuser",
  "tempbanclient",
  "tempbanuser",
  "serverinfo",
  "map_rotate",
  "map_restart",
  "map",
  "fast_restart",
  "getinfo",
  "mapname",
  "g_gametype",
  "sv_maprotation",
  "sv_maprotationcurrent",
  "say",
  "tell",
  "!say",
  "!tell" };

extern const unordered_set<string> rcon_commands_wait_for_reply{ "!s", "!status", "s", "status", "gs", "!gs", "getstatus", "serverinfo", "map_rotate", "map_restart", "map", "fast_restart", "getinfo", "mapname", "g_gametype", "sv_maprotation", "sv_maprotationcurrent" };

extern unordered_map<size_t, string> rcon_status_grid_column_header_titles;
extern unordered_map<size_t, string> get_status_grid_column_header_titles;
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

void show_error(HWND parent_window, const char *error_message, const size_t error_level) noexcept
{
  switch (error_level) {
  case 0:
    MessageBoxA(parent_window, error_message, "Warning message", MB_OK | MB_ICONWARNING);
    break;
  case 1:
    MessageBoxA(parent_window, error_message, "Error message", MB_OK | MB_ICONERROR | MB_ICONEXCLAMATION);
    exit(1);
    break;
  case 2:
    MessageBoxA(parent_window, error_message, "Fatal error message", MB_OK | MB_ICONERROR | MB_ICONEXCLAMATION);
    exit(2);
    break;
  default:
    MessageBoxA(parent_window, error_message, "Unknown warning-level message", MB_OK | MB_ICONWARNING);
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
    auto parts = stl::helper::str_split(line, ',', nullptr, true);
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

    parts[4].pop_back();
    parts[4].erase(0, 1);
    parts[5].pop_back();
    parts[5].erase(0, 1);
    geoip_data.emplace_back(stoul(parts[0]), stoul(parts[1]), parts[2].c_str(), parts[3].c_str(), parts[4].c_str(), parts[5].c_str());
  }

  main_app.get_connection_manager().get_geoip_data() = std::move(geoip_data);


  return true;
}

bool write_tiny_rcon_json_settings_to_file(
  const char *file_path) noexcept
{
  std::ofstream config_file{ file_path };

  if (!config_file) {
    show_error(app_handles.hwnd_main_window, string{ "Error writing default tiny rcon too 's configuration data\nto specified tinyrcon.json file ("s + string{ file_path } + ".\nPlease make sure that the main application folder's properties are not set to read-only mode!"s }.c_str(), 0);
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
  config_file << "\"codmp_exe_path\": \"" << main_app.get_codmp_exe_path() << "\",\n";
  config_file << "\"cod2mp_s_exe_path\": \"" << main_app.get_cod2mp_exe_path() << "\",\n";
  config_file << "\"iw3mp_exe_path\": \"" << main_app.get_iw3mp_exe_path() << "\",\n";
  config_file << "\"cod5mp_exe_path\": \"" << main_app.get_cod5mp_exe_path() << "\",\n";
  config_file << "\"enable_automatic_connection_flood_ip_ban\": " << (main_app.get_is_enable_automatic_connection_flood_ip_ban() ? "true" : "false") << ",\n";
  config_file << "\"minimum_number_of_connections_from_same_ip_for_automatic_ban\": " << main_app.get_minimum_number_of_connections_from_same_ip_for_automatic_ban() << ",\n";
  config_file << "\"number_of_warnings_for_automatic_kick\": " << main_app.get_maximum_number_of_warnings_for_automatic_kick() << ",\n";
  config_file << "\"user_defined_warn_msg\": \"" << main_app.get_admin_messages()["user_defined_warn_msg"] << "\",\n";
  config_file << "\"user_defined_kick_msg\": \"" << main_app.get_admin_messages()["user_defined_kick_msg"] << "\",\n";
  config_file << "\"user_defined_temp_ban_msg\": \"" << main_app.get_admin_messages()["user_defined_temp_ban_msg"] << "\",\n";
  config_file << "\"user_defined_ban_msg\": \"" << main_app.get_admin_messages()["user_defined_ban_msg"] << "\",\n";
  config_file << "\"user_defined_ip_ban_msg\": \"" << main_app.get_admin_messages()["user_defined_ip_ban_msg"] << "\",\n";
  config_file << "\"automatic_remove_temp_ban_msg\": \"" << main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] << "\",\n";
  config_file << "\"automatic_kick_temp_ban_msg\": \"" << main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] << "\",\n";
  config_file << "\"automatic_kick_ip_ban_msg\": \"" << main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] << "\",\n";
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
  config_file << "\"draw_border_lines\": " << (main_app.get_is_draw_border_lines() ? "true" : "false") << '\n';
  config_file << "}" << flush;
  return true;
}

bool check_ip_address_validity(string_view ip_address,
  unsigned long &guid_key)
{
  const auto parts = stl::helper::str_split(ip_address, ".", "", true);
  if (parts.size() < 4U)
    return false;

  const bool is_valid_ip_address =
    all_of(cbegin(parts), cend(parts), [](const string &part) {
      const int value{ stoi(part) };
      return value >= 0 && value <= 255;
    });

  if (is_valid_ip_address) {
    guid_key = 0L;
    // 123.123.123.123 // 123 * 256 + 123 =
    // 1. 4 = 4
    // 2. 4 * 256 + 18 = 1042
    // 3. 1042 * 256 + 100 = 266852
    // 4. 266852 * 256 + 32 = 68314144
    for (size_t i{}; i < 4; ++i) {
      guid_key <<= 8;
      guid_key += stoul(parts[i]);
    }
  }

  return is_valid_ip_address;
}

void convert_guid_key_to_country_name(const vector<geoip_data> &geo_data,
  string_view player_ip,
  player_data &player_data)
{
  unsigned long playerGuidKey{};
  if (!check_ip_address_validity(player_ip, playerGuidKey)) {
    player_data.country_name = "Unknown";
    player_data.region = "Unknown";
    player_data.city = "Unknown";
    player_data.country_code = "xy";

  } else {
    if (len(player_data.guid_key) == 0) {
      snprintf(player_data.guid_key, std::size(player_data.guid_key), "%lu", playerGuidKey);
    }
    const size_t sizeOfElements{ geo_data.size() };
    size_t lower_bound{ 0 };
    size_t upper_bound{ sizeOfElements };

    bool is_found_match{};

    while (lower_bound <= upper_bound) {
      const size_t currentIndex = (lower_bound + upper_bound) / 2;

      if (currentIndex >= geo_data.size())
        break;

      if (playerGuidKey >= geo_data.at(currentIndex).lower_ip_bound && playerGuidKey <= geo_data.at(currentIndex).upper_ip_bound) {
        player_data.country_name = geo_data.at(currentIndex).get_country_name();
        player_data.region = geo_data.at(currentIndex).get_region();
        player_data.city = geo_data.at(currentIndex).get_city();
        player_data.country_code = geo_data.at(currentIndex).get_country_code();
        is_found_match = true;
        break;
      }

      if (playerGuidKey > geo_data.at(currentIndex).upper_ip_bound) {
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
  const std::vector<player_data> &players,
  const bool count_color_codes,
  const size_t number_of_players_to_process) noexcept
{
  if (0 == number_of_players_to_process)
    return 0;

  size_t max_player_name_length = count_color_codes ? len(players[0].player_name) : get_number_of_characters_without_color_codes(players[0].player_name);
  for (size_t i{ 1 }; i < number_of_players_to_process; ++i) {
    max_player_name_length =
      max(count_color_codes ? len(players[i].player_name) : get_number_of_characters_without_color_codes(players[i].player_name), max_player_name_length);
  }

  return max_player_name_length;
}

size_t find_longest_player_country_city_info_length(
  const std::vector<player_data> &players,
  const size_t number_of_players_to_process) noexcept
{
  if (0 == number_of_players_to_process)
    return 0;

  size_t country_len = len(players[0].country_name);
  size_t region_len = len(players[0].region);
  size_t city_len = len(players[0].city);

  size_t max_geodata_info_length = (country_len != 0 ? country_len : region_len) + city_len + 2;
  for (size_t i{ 1 }; i < number_of_players_to_process; ++i) {
    country_len = len(players[i].country_name);
    region_len = len(players[i].region);
    city_len = len(players[i].city);
    const size_t current_player_geodata_info_length = (country_len != 0 ? country_len : region_len) + city_len + 2;
    max_geodata_info_length = std::max(current_player_geodata_info_length, max_geodata_info_length);
  }

  return max_geodata_info_length;
}

void parse_tiny_cod2_rcon_tool_config_file(const char *configFileName)
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
    main_app.set_username(std::move(data_line));
    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
  } else {
    found_missing_config_setting = true;
    main_app.set_username("^1Administrator");
    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
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
    main_app.set_game_server_name("185.158.113.146:28995 CoD2 CTF");
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

    main_app.set_is_enable_automatic_connection_flood_ip_ban(json_resource["enable_automatic_connection_flood_ip_ban"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.set_is_enable_automatic_connection_flood_ip_ban(false);
  }

  if (json_resource["minimum_number_of_connections_from_same_ip_for_automatic_ban"].exists()) {
    main_app.set_minimum_number_of_connections_from_same_ip_for_automatic_ban(json_resource["minimum_number_of_connections_from_same_ip_for_automatic_ban"].as<int>());
  } else {
    found_missing_config_setting = true;
    main_app.set_minimum_number_of_connections_from_same_ip_for_automatic_ban(5);
  }

  if (json_resource["number_of_warnings_for_automatic_kick"].exists()) {

    main_app.set_maximum_number_of_warnings_for_automatic_kick(json_resource["number_of_warnings_for_automatic_kick"].as<int>());
  } else {
    found_missing_config_setting = true;
    main_app.set_maximum_number_of_warnings_for_automatic_kick(2);
  }

  if (json_resource["user_defined_warn_msg"].exists()) {
    data_line = json_resource["user_defined_warn_msg"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_admin_messages()["user_defined_warn_msg"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_warn_msg"] = "^3[^5Tiny^6Rcon^3] ^7> {PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_kick_msg"].exists()) {
    data_line = json_resource["user_defined_kick_msg"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_admin_messages()["user_defined_kick_msg"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_kick_msg"] = "^3[^5Tiny^6Rcon^3] ^7> {PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_temp_ban_msg"].exists()) {
    data_line = json_resource["user_defined_temp_ban_msg"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_admin_messages()["user_defined_temp_ban_msg"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_temp_ban_msg"] = "^3[^5Tiny^6Rcon^3] ^7> {PLAYERNAME} ^1you are being temporarily banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_ban_msg"].exists()) {
    data_line = json_resource["user_defined_ban_msg"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_admin_messages()["user_defined_ban_msg"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_ban_msg"] = "^3[^5Tiny^6Rcon^3] ^7> {PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_ip_ban_msg"].exists()) {
    data_line = json_resource["user_defined_ip_ban_msg"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_admin_messages()["user_defined_ip_ban_msg"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_ip_ban_msg"] = "^3[^5Tiny^6Rcon^3] ^7> {PLAYERNAME} ^1you are being permanently banned by admin ^5{ADMINNAME}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["automatic_remove_temp_ban_msg"].exists()) {
    data_line = json_resource["automatic_remove_temp_ban_msg"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] = "^7{PLAYERNAME}'s ^1tempban ^7[start date: ^3{TEMP_BAN_START_DATE} ^7expired on ^3{TEMP_BAN_END_DATE}] ^7has been automatically removed.";
  }

  if (json_resource["automatic_kick_temp_ban_msg"].exists()) {
    data_line = json_resource["automatic_kick_temp_ban_msg"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] = "^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}}^7Your temporary ban expires on ^1{TEMP_BAN_END_DATE}";
  }

  if (json_resource["automatic_kick_ip_ban_msg"].exists()) {
    data_line = json_resource["automatic_kick_ip_ban_msg"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] = std::move(data_line);
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] = "^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is being automatically ^1kicked.";
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

  if (json_resource["draw_border_lines"].exists()) {
    main_app.set_is_draw_border_lines(json_resource["draw_border_lines"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.set_is_draw_border_lines(true);
  }

  if (found_missing_config_setting) {
    write_tiny_rcon_json_settings_to_file(configFileName);
  }
}

void parse_tempbans_data_file()
{
  // lock_guard lg{ protect_banned_players_data };
  string property_key, property_value;

  ifstream banned_tempbans_file_read{ tempbans_file_path };
  if (!banned_tempbans_file_read) {
    const size_t buffer_size{ 256U };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "Couldn't open banned tempbans data file at specified path (" + string{ tempbans_file_path } + ")! " + string{ buffer }
    };
    show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    ofstream banned_tempbans_file_to_write{ tempbans_file_path };
    if (!banned_tempbans_file_to_write) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(
        "Couldn't create banned tempbans data file at "
        "data\\tempbans.txt!"
        + string{ buffer });
      show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    }
  } else {
    main_app.get_game_server().get_temp_banned_players_data().clear();
    string readData;
    const time_t current_time = std::time(nullptr);
    while (getline(banned_tempbans_file_read, readData)) {
      stl::helper::trim_in_place(readData, " \t\n");
      vector<string> parts = stl::helper::str_split(readData, "\\", "", true, false);
      for (auto &part : parts)
        stl::helper::trim_in_place(part, " \t\n");
      player_data temp_banned_player_data;
      if (parts.size() < 6)
        continue;
      strcpy_s(temp_banned_player_data.ip_address, std::size(temp_banned_player_data.ip_address), parts[0].c_str());
      strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name), parts[1].c_str());
      strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time), parts[2].c_str());
      temp_banned_player_data.banned_start_time = stoll(parts[3]);
      temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
      strcpy_s(temp_banned_player_data.reason, std::size(temp_banned_player_data.reason), parts[5].c_str());
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        { temp_banned_player_data.ip_address, len(temp_banned_player_data.ip_address) },
        temp_banned_player_data);
      const time_t ban_expires_time = temp_banned_player_data.banned_start_time + (temp_banned_player_data.ban_duration_in_hours * 3600);
      if (ban_expires_time <= current_time) {
        main_app.get_game_server().get_tempbanned_players_to_unban().push_back(std::move(temp_banned_player_data));
      } else {
        main_app.get_game_server().add_ip_address_to_set_of_temp_banned_ip_addresses(
          temp_banned_player_data.ip_address);
        main_app.get_game_server().get_temp_banned_players_data().emplace_back(std::move(temp_banned_player_data));
      }
    }
  }
}

void parse_banned_ip_addresses_file()
{
  // lock_guard lg{ protect_banned_players_data };
  string property_key, property_value;

  ifstream bannedIPsFileToRead{ banned_ip_addresses_file_path };
  if (!bannedIPsFileToRead) {
    const size_t buffer_size{ 256U };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "Couldn't open banned IP addresses data file at specified path (" + string{ banned_ip_addresses_file_path } + ")! " + string{ buffer }
    };
    show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    ofstream bannedIPsFileToWrite{ banned_ip_addresses_file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(
        "Couldn't create banned IP addresses data file at "
        "data\\bans.txt!"
        + string{ buffer });
      show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    }
  } else {
    main_app.get_game_server().get_banned_players_data().clear();
    string readData;
    while (getline(bannedIPsFileToRead, readData)) {
      stl::helper::trim_in_place(readData, " \t\n");
      vector<string> parts = stl::helper::str_split(readData, "\\", "", true, false);
      for (auto &part : parts)
        stl::helper::trim_in_place(part, " \t\n");
      player_data bannedPlayerData{};
      if (parts.size() < 5)
        continue;

      strcpy_s(bannedPlayerData.ip_address, std::size(bannedPlayerData.ip_address), parts[0].c_str());
      strcpy_s(bannedPlayerData.guid_key, std::size(bannedPlayerData.guid_key), parts[1].c_str());
      strcpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[2].c_str());
      strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time), parts[3].c_str());
      strcpy_s(bannedPlayerData.reason, std::size(bannedPlayerData.reason), parts[4].c_str());
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        bannedPlayerData.ip_address,
        bannedPlayerData);
      main_app.get_game_server().add_ip_address_to_set_of_banned_ip_addresses(
        bannedPlayerData.ip_address);
      main_app.get_game_server().get_banned_players_data().emplace_back(std::move(bannedPlayerData));
    }
  }
}

bool temp_ban_player_ip_address(player_data &pd)
{
  using namespace std::literals;

  // lock_guard lg{ protect_banned_players_data };
  if (main_app.get_game_server().get_set_of_temp_banned_ip_addresses().count(pd.ip_address) == 1U)
    return true;

  main_app.get_game_server().add_ip_address_to_set_of_temp_banned_ip_addresses(
    pd.ip_address);

  unsigned long guid{};
  if (!check_ip_address_validity(pd.ip_address, guid)) {
    return false;
  }

  if (pd.country_name == nullptr || len(pd.country_name) == 0) {
    convert_guid_key_to_country_name(
      main_app.get_connection_manager().get_geoip_data(),
      pd.ip_address,
      pd);
  }

  ofstream temp_banned_data_file_to_read{ tempbans_file_path, ios_base::app };
  if (!temp_banned_data_file_to_read) {
    const size_t buffer_size{ 256U };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "Couldn't open temporarily banned IP addresses data file at specified path (" + string{ tempbans_file_path } + ")! " + string{ buffer }
    };
    show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    ofstream temp_banned_data_file_to_write{ tempbans_file_path };
    if (!temp_banned_data_file_to_write) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(
        "Couldn't create temporarily banned IP addresses data file at "
        "data\\tempbans.txt!"
        + string{ buffer });
      show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
      return false;
    }

    const time_t time_of_ban_in_seconds = std::time(nullptr);
    pd.banned_start_time = time_of_ban_in_seconds;
    const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();

    const time_t t_c = std::chrono::system_clock::to_time_t(now);
    ostringstream oss;
    tm time_info{};
    localtime_s(&time_info, &t_c);
    oss << put_time(&time_info, "%Y-%b-%d %T");
    const string banned_date_time_str{ oss.str() };
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    temp_banned_data_file_to_write << pd.ip_address << '\\'
                                   << pd.player_name << '\\'
                                   << pd.banned_date_time << '\\'
                                   << time_of_ban_in_seconds << '\\'
                                   << pd.ban_duration_in_hours << '\\'
                                   << pd.reason << endl;
  } else {
    const time_t time_of_ban_in_seconds = std::time(nullptr);
    pd.banned_start_time = time_of_ban_in_seconds;
    const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();

    const time_t t_c = std::chrono::system_clock::to_time_t(now);
    ostringstream oss;
    tm time_info{};
    localtime_s(&time_info, &t_c);
    oss << put_time(&time_info, "%Y-%b-%d %T");
    const string banned_date_time_str{ oss.str() };
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    temp_banned_data_file_to_read << pd.ip_address << '\\'
                                  << pd.player_name << '\\'
                                  << pd.banned_date_time << '\\'
                                  << time_of_ban_in_seconds << '\\'
                                  << pd.ban_duration_in_hours << '\\'
                                  << pd.reason << endl;
  }

  main_app.get_game_server().get_temp_banned_players_data().emplace_back(
    pd);
  return true;
}

bool global_ban_player_ip_address(player_data &pd)
{
  using namespace std::literals;

  // lock_guard lg{ protect_banned_players_data };
  if (main_app.get_game_server().get_set_of_banned_ip_addresses().count(pd.ip_address) == 1U)
    return true;

  main_app.get_game_server().add_ip_address_to_set_of_banned_ip_addresses(
    pd.ip_address);

  unsigned long guid{};
  if (!check_ip_address_validity(pd.ip_address, guid)) {
    return false;
  }

  if (pd.country_name == nullptr || len(pd.country_name) == 0) {
    convert_guid_key_to_country_name(
      main_app.get_connection_manager().get_geoip_data(),
      pd.ip_address,
      pd);
  }

  ofstream bannedIPsFileToRead{ banned_ip_addresses_file_path, ios_base::app };
  if (!bannedIPsFileToRead) {
    const size_t buffer_size{ 256U };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "Couldn't open banned IP addresses data file at specified path (" + string{ banned_ip_addresses_file_path } + ")! " + string{ buffer }
    };
    show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
    ofstream bannedIPsFileToWrite{ banned_ip_addresses_file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(
        "Couldn't create banned IP addresses data file at "
        "data\\bans.txt!"
        + string{ buffer });
      show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
      return false;
    }

    const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();

    const time_t t_c = std::chrono::system_clock::to_time_t(now);
    ostringstream oss;
    tm time_info{};
    localtime_s(&time_info, &t_c);
    oss << put_time(&time_info, "%Y-%b-%d %T");
    const string banned_date_time_str{ oss.str() };
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    bannedIPsFileToWrite << pd.ip_address << '\\'
                         << pd.guid_key << '\\'
                         << pd.player_name << '\\'
                         << pd.banned_date_time << '\\'
                         << pd.reason << endl;
  } else {
    const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();

    const time_t t_c = std::chrono::system_clock::to_time_t(now);
    ostringstream oss;
    tm time_info{};
    localtime_s(&time_info, &t_c);
    oss << put_time(&time_info, "%Y-%b-%d %T");
    const string banned_date_time_str{ oss.str() };
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    bannedIPsFileToRead << pd.ip_address << '\\'
                        << pd.guid_key << '\\'
                        << pd.player_name << '\\' << pd.banned_date_time << '\\' << pd.reason << endl;
  }

  main_app.get_game_server().get_banned_players_data().emplace_back(
    pd);
  return true;
}

bool remove_temp_banned_ip_address(const std::string &ip_address, std::string &message, const bool is_automatic_temp_ban_remove)
{
  // lock_guard lg{ protect_banned_players_data };
  if (main_app.get_game_server().get_set_of_temp_banned_ip_addresses().find(
        ip_address)
      == cend(main_app.get_game_server().get_set_of_temp_banned_ip_addresses())) {
    return false;
  }

  main_app.get_game_server().remove_ip_address_from_set_of_temp_banned_ip_addresses(
    ip_address);

  auto &temp_banned_players = main_app.get_game_server().get_temp_banned_players_data();

  const auto found_iter = find_if(std::begin(temp_banned_players), std::end(temp_banned_players), [&ip_address](const player_data &p) {
    return ip_address == p.ip_address;
  });

  if (found_iter != std::end(temp_banned_players)) {
    if (!is_automatic_temp_ban_remove) {
      char buffer[512]{};
      std::snprintf(buffer, std::size(buffer), "^7Admin (%s^7) has manually removed temporarily banned IP address ^1%s ^7for player %s ^7(reason: ^1%s^7)\n", main_app.get_username().c_str(), found_iter->ip_address, found_iter->player_name, found_iter->reason);
      message.assign(buffer);
    }

    rcon_say(message);

    temp_banned_players.erase(remove_if(std::begin(temp_banned_players), std::end(temp_banned_players), [&ip_address](const player_data &p) {
      return ip_address == p.ip_address;
    }),
      std::end(temp_banned_players));


    ofstream output{ tempbans_file_path };

    if (output) {
      for (const auto &tb_player : temp_banned_players) {
        output << tb_player.ip_address << '\\'
               << tb_player.player_name << '\\'
               << tb_player.banned_date_time << '\\'
               << tb_player.banned_start_time << '\\'
               << tb_player.ban_duration_in_hours << '\\'
               << tb_player.reason << '\n';
      }

      output << flush;
    }
  }

  return true;
}

bool remove_permanently_banned_ip_address(const std::string &ip_address, std::string &message)
{
  // lock_guard lg{ protect_banned_players_data };
  if (main_app.get_game_server().get_set_of_banned_ip_addresses().find(
        ip_address)
      == cend(main_app.get_game_server().get_set_of_banned_ip_addresses())) {
    return false;
  }

  main_app.get_game_server().remove_ip_address_from_set_of_banned_ip_addresses(
    ip_address);

  auto &banned_players = main_app.get_game_server().get_banned_players_data();

  const auto found_iter = find_if(std::begin(banned_players), std::end(banned_players), [&ip_address](const player_data &p) {
    return ip_address == p.ip_address;
  });

  if (found_iter != std::end(banned_players)) {
    char buffer[512]{};
    snprintf(buffer, std::size(buffer), "^7Admin (%s^7) has manually removed permanently banned IP address ^1%s ^7for player %s ^7(reason: ^1%s^7)\n", main_app.get_username().c_str(), found_iter->ip_address, found_iter->player_name, found_iter->reason);
    message.assign(buffer);

    rcon_say(message);

    banned_players.erase(remove_if(std::begin(banned_players), std::end(banned_players), [&ip_address](const player_data &p) {
      return ip_address == p.ip_address;
    }),
      std::end(banned_players));

    ofstream output{ banned_ip_addresses_file_path };

    if (output) {
      for (const auto &banned_player : banned_players) {
        output << banned_player.ip_address << '\\'
               << banned_player.guid_key << '\\'
               << banned_player.player_name << '\\'
               << banned_player.banned_date_time << '\\'
               << banned_player.reason << '\n';
      }

      output << flush;
    }
  }

  return true;
}

bool is_valid_decimal_whole_number(const std::string &number_str, int &number) noexcept
{
  try {
    number = stoi(number_str);
  } catch (const std::invalid_argument &) {
    return false;
  } catch (const std::out_of_range &) {
    return false;
  } catch (const std::exception &) {
    return false;
  } catch (...) {
    return false;
  }

  return true;
}

size_t get_number_of_characters_without_color_codes(const char *text) noexcept
{
  size_t printed_chars_count{};
  const size_t text_len = strlen(text);
  const char *last = text + text_len;
  for (; *text; ++text) {
    if (text + black_bg_color_length <= last && strncmp(text, black_bg_color, black_bg_color_length) == 0) {
      text += (black_bg_color_length - 1);
    } else if (text + grey_bg_color_length <= last && strncmp(text, grey_bg_color, grey_bg_color_length) == 0) {
      text += (grey_bg_color_length - 1);
    } else if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') && (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3)) {
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
  Edit_SetText(app_handles.hwnd_e_user_input, "");
  return should_program_terminate(user_input);
}

void print_help_information(const std::vector<std::string> &input_parts)
{
  if (input_parts.size() >= 2 && (input_parts[0] == "!list" || input_parts[0] == "list" || input_parts[0] == "!l" || input_parts[0] == "!help" || input_parts[0] == "help" || input_parts[0] == "!h" || input_parts[0] == "h") && str_starts_with(input_parts[1], "user", true)) {
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
 ^1!border on|off ^5-> Turns ^3on^5|^3off border lines around displayed ^3GUI controls^5. 
)"
    };
    print_colored_text(app_handles.hwnd_re_messages_data, help_message.c_str(), true, false);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", true, false);
  } else if (input_parts.size() >= 2 && (input_parts[0] == "!list" || input_parts[0] == "list" || input_parts[0] == "!l" || input_parts[0] == "!help" || input_parts[0] == "help" || input_parts[0] == "!h" || input_parts[0] == "h") && str_starts_with(input_parts[1], "rcon", true)) {
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
    print_colored_text(app_handles.hwnd_re_messages_data, help_message.c_str(), true, false);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", true, false);
  } else {
    print_colored_text(app_handles.hwnd_re_messages_data,
      "^5********************************************************************"
      "**"
      "********************\n",
      true,
      false);
    print_colored_text(app_handles.hwnd_re_messages_data,
      "Type '^2!list rcon^5' in the console to get more information about "
      "the "
      "available rcon commands\nthat you can run in the console window.\n",
      true,
      false);
    print_colored_text(app_handles.hwnd_re_messages_data,
      "Type '^2!list user^5' in the console to get more information about "
      "the "
      "available user commands\nthat you can run in the console window.\n",
      true,
      false);
    print_colored_text(app_handles.hwnd_re_messages_data,
      "**********************************************************************"
      "********************\n",
      true,
      false);
  }
}

string prepare_current_match_information()
{
  string current_match_info = main_app.get_game_server().get_current_match_info();

  size_t start = current_match_info.find("{MAP_FULL_NAME}");
  if (string::npos != start) {
    current_match_info.replace(start, stl::helper::len("{MAP_FULL_NAME}"), main_app.get_game_server().get_full_map_name_color() + get_full_map_name(main_app.get_game_server().get_current_map()));
  }

  start = current_match_info.find("{MAP_RCON_NAME}");
  if (string::npos != start) {
    current_match_info.replace(start, stl::helper::len("{MAP_RCON_NAME}"), main_app.get_game_server().get_rcon_map_name_color() + main_app.get_game_server().get_current_map());
  }

  start = current_match_info.find("{GAMETYPE_FULL_NAME}");
  if (string::npos != start) {
    current_match_info.replace(start, stl::helper::len("{GAMETYPE_FULL_NAME}"), main_app.get_game_server().get_full_gametype_name_color() + get_full_gametype_name(main_app.get_game_server().get_current_game_type()));
  }

  start = current_match_info.find("{GAMETYPE_RCON_NAME}");
  if (string::npos != start) {
    current_match_info.replace(start, stl::helper::len("{GAMETYPE_RCON_NAME}"), main_app.get_game_server().get_rcon_gametype_name_color() + main_app.get_game_server().get_current_game_type());
  }

  const int online_players_count = main_app.get_game_server().get_number_of_online_players();
  const int offline_players_count = main_app.get_game_server().get_number_of_offline_players();

  start = current_match_info.find("{ONLINE_PLAYERS_COUNT}");
  if (string::npos != start) {
    current_match_info.replace(start, stl::helper::len("{ONLINE_PLAYERS_COUNT}"), main_app.get_game_server().get_online_players_count_color() + to_string(online_players_count));
  }

  start = current_match_info.find("{OFFLINE_PLAYERS_COUNT}");
  if (string::npos != start) {
    current_match_info.replace(start, stl::helper::len("{OFFLINE_PLAYERS_COUNT}"), main_app.get_game_server().get_offline_players_count_color() + to_string(offline_players_count));
  }

  return current_match_info;
}

bool check_if_user_provided_argument_is_valid_for_specified_command(
  const char *cmd,
  const string &arg)
{
  int number{};
  if ((str_compare_i(cmd, "!k") == 0) || (str_compare_i(cmd, "!kick") == 0) || (str_compare_i(cmd, "clientkick") == 0) || (str_compare_i(cmd, "!tb") == 0) || (str_compare_i(cmd, "!tempban") == 0) || (str_compare_i(cmd, "tempbanclient") == 0) || (str_compare_i(cmd, "!b") == 0) || (str_compare_i(cmd, "!ban") == 0) || (str_compare_i(cmd, "banclient") == 0)) {
    return is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg);
  }

  if ((str_compare_i(cmd, "!gb") == 0) || (str_compare_i(cmd, "!globalban") == 0) || (str_compare_i(cmd, "!banip") == 0) || (str_compare_i(cmd, "!addip") == 0)) {
    unsigned long guid{};
    return ((is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg)) || check_ip_address_validity(arg, guid));
  }

  return true;
}

bool check_if_user_provided_pid_is_valid(const string &pid) noexcept
{
  if (int number{ -1 }; is_valid_decimal_whole_number(pid, number)) {
    const auto &players_data = main_app.get_game_server().get_players_data();
    for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
      if (players_data[i].pid == number)
        return true;
    }
  }
  return false;
}

void remove_all_color_codes(char *msg)
{
  const size_t msg_len{ stl::helper::len(msg) };
  size_t start{};
  while (start < msg_len) {
    start = stl::helper::str_index_of(msg, '^', start);
    if (string::npos == start)
      break;

    if (start + 4 <= msg_len && msg[start + 1] == '^' && (msg[start + 2] >= '0' && msg[start + 2] <= '9') && (msg[start + 3] >= '0' && msg[start + 3] <= '9') && msg[start + 2] == msg[start + 3]) {
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

pair<player_data, bool> get_online_player_for_specified_pid(const int pid)
{
  const auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    if (pid == players_data[i].pid)
      return make_pair(players_data[i], true);
  }
  return make_pair(player_data{}, false);
}

void check_for_warned_players()
{
  unordered_map<int, player_data> &warned_players = main_app.get_game_server().get_warned_players_data();
  unordered_map<int, player_data> refreshed_warned_players_data;
  refreshed_warned_players_data.reserve(warned_players.size());
  const auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    const auto &online_player = players_data[i];
    if (warned_players.find(online_player.pid) != end(warned_players)) {

      if (strcmp(warned_players.at(online_player.pid).ip_address, online_player.ip_address) == 0) {
        refreshed_warned_players_data[online_player.pid] = std::move(warned_players.at(online_player.pid));
      } else {
        warned_players.erase(online_player.pid);
      }
    }
  }
  main_app.get_game_server().set_warned_players_data(std::move(refreshed_warned_players_data));
}

void check_for_temp_banned_ip_addresses()
{
  const auto &temp_banned_ip_address =
    main_app.get_game_server().get_set_of_temp_banned_ip_addresses();
  if (temp_banned_ip_address.empty())
    return;
  const time_t now_in_seconds = std::time(nullptr);
  auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    auto &online_player = players_data[i];

    if (temp_banned_ip_address.count(online_player.ip_address) != 0U) {
      player_data pd{};
      if (main_app.get_game_server().get_temp_banned_player_data_for_ip_address(online_player.ip_address, &pd)) {
        const time_t ban_expires_time = pd.banned_start_time + (pd.ban_duration_in_hours * 3600);
        if (ban_expires_time > now_in_seconds) {
          string message{ main_app.get_automatic_kick_temp_ban_msg() };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
          main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] = get_date_and_time_for_time_t(pd.banned_start_time);
          main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] = get_time_interval_info_string_for_seconds(ban_expires_time - now_in_seconds);
          main_app.get_tinyrcon_dict()["{REASON}"] = pd.reason;
          build_tiny_rcon_message(message);
          kick_player(online_player.pid, message);
        } else {
          string message{ main_app.get_automatic_remove_temp_ban_msg() };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
          main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] = get_date_and_time_for_time_t(pd.banned_start_time);
          main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] = get_date_and_time_for_time_t(ban_expires_time);
          main_app.get_tinyrcon_dict()["{REASON}"] = pd.reason;
          build_tiny_rcon_message(message);
          remove_temp_banned_ip_address(online_player.ip_address, message, true);
        }
      }
    }
  }
}

void check_for_banned_ip_addresses()
{
  char msg[512];
  const bool is_enable_automatic_connection_flood_ip_ban{ main_app.get_is_enable_automatic_connection_flood_ip_ban() };

  const auto &banned_ip_address =
    main_app.get_game_server().get_set_of_banned_ip_addresses();
  const auto &ip_address_frequency = main_app.get_game_server().get_ip_address_frequency();
  auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    auto &online_player = players_data[i];
    if (is_enable_automatic_connection_flood_ip_ban && ip_address_frequency.find(online_player.ip_address) != cend(ip_address_frequency) && ip_address_frequency.at(online_player.ip_address) >= main_app.get_minimum_number_of_connections_from_same_ip_for_automatic_ban()) {
      main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(online_player.pid);
      string reason{ "^1DoS attack" };
      specify_reason_for_player_pid(online_player.pid, reason);
      string public_msg{ "^5Tiny^6Rcon ^2has successfully automatically banned ^1IP address: "s + online_player.ip_address + " ^2Reason: " + reason + "\n"s };
      rcon_say(public_msg, true);
      main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
      const string message{
        "^5Tiny^6Rcon ^2has successfully automatically executed command ^1!gb ^2on player ("s + get_player_information(online_player.pid) + "^3)\n"s
      };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
      global_ban_player_ip_address(online_player);
      is_display_permanently_banned_players_data_event.store(true);
    }

    if (banned_ip_address.contains(online_player.ip_address)) {
      string message{ main_app.get_automatic_kick_ip_ban_msg() };
      main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
      player_data pd{};
      if (main_app.get_game_server().get_banned_player_data_for_ip_address(online_player.ip_address, &pd)) {
        main_app.get_tinyrcon_dict()["{REASON}"] = pd.reason;
      }
      build_tiny_rcon_message(message);
      const string banned_player_information{ get_player_information(online_player.pid) };
      snprintf(msg, std::size(msg), "^1Automatic kick ^5for previously ^1banned IP: %s ^5| Reason: ^1%s\n%s\n", online_player.ip_address, pd.reason, banned_player_information.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, msg, true, true, true);
      kick_player(online_player.pid, message);
    }
  }
}

void kick_player(const int pid, string &custom_message)
{
  static char buffer[256];
  static string reply;
  string message{ custom_message };
  rcon_say(message);
  snprintf(buffer, std::size(buffer), "clientkick %d", pid);
  main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
  auto &warned_players = main_app.get_game_server().get_warned_players_data();
  if (warned_players.find(pid) != end(warned_players)) {
    warned_players.erase(pid);
  }
}

void tempban_player(player_data &pd, std::string &custom_message)
{
  temp_ban_player_ip_address(pd);
  kick_player(pd.pid, custom_message);
}

void ban_player(const int pid, std::string &custom_message)
{
  static char buffer[256];
  static string reply;
  snprintf(buffer, std::size(buffer), "banclient %d", pid);
  main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
  kick_player(pid, custom_message);
}

void say_message(const char *message)
{
  static char buffer[256];
  static string reply;
  snprintf(buffer, std::size(buffer), "say \"%s\"", message);
  main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
}

void rcon_say(string &msg, const bool is_print_to_rich_edit_messages_box)
{
  using namespace stl::helper;
  str_replace_all(msg, "{{br}}", "\n");
  str_replace_all(msg, "\\", "|");
  str_replace_all(msg, "/", "|");

  msg = word_wrap(msg.c_str(), 140);
  const auto lines = str_split(msg, "\n", nullptr, true);
  for (const auto &line : lines) {
    say_message(line.c_str());
    if (is_print_to_rich_edit_messages_box)
      print_colored_text(app_handles.hwnd_re_messages_data, line.c_str(), true, true, true);
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

void tell_message(const char *message, const int pid)
{
  using namespace stl::helper;
  string msg{ message };
  str_replace_all(msg, "{{br}}", "\n");
  str_replace_all(msg, "\\", "|");
  str_replace_all(msg, "/", "|");

  msg = word_wrap(msg.c_str(), 140);
  const auto lines = str_split(msg, "\n", "", true);

  static char buffer[256]{};
  static string reply;
  for (const auto &line : lines) {
    snprintf(buffer, std::size(buffer), "tell %d \"%s\"", pid, line.c_str());
    main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
  }
}

void process_user_input(std::string &user_input)
{
  trim_in_place(user_input, " \t\n\f\v\r");
  if (!user_input.empty()) {

    auto command_parts = str_split(user_input, " \t\n\f\v", "", false, true);

    if (!command_parts.empty()) {
      to_lower_case_in_place(command_parts[0]);
      commands_history.emplace_back(user_input);
      commands_history_index = commands_history.size() - 1;
      if (rcon_status_commands.contains(command_parts[0])) {
        initiate_sending_rcon_status_command_now();
      } else if (command_parts[0] == "q" || command_parts[0] == "!q" || command_parts[0] == "!quit" || command_parts[0] == "e" || command_parts[0] == "!e" || command_parts[0] == "exit") {
        user_input = "q";
      } else if (command_parts[0] == "h" || command_parts[0] == "!h" || command_parts[0] == "!help" || command_parts[0] == "help" || command_parts[0] == "list") {
        print_help_information(command_parts);
      } else if (command_parts[0] == "!cls" || command_parts[0] == "cls") {
        Edit_SetText(app_handles.hwnd_re_messages_data, "");
        g_message_data_contents.clear();
      } else if (command_parts[0] == "bans" || command_parts[0] == "!bans") {
        is_display_permanently_banned_players_data_event.store(true);
      } else if (command_parts[0] == "tempbans" || command_parts[0] == "!tempbans") {
        is_display_temporarily_banned_players_data_event.store(true);
      } else if (user_commands_set.find(command_parts[0]) != cend(user_commands_set)) {

        main_app.add_command_to_queue(std::move(command_parts), command_type::user, false);
      } else if (rcon_commands_set.find(command_parts[0]) != cend(rcon_commands_set)) {
        const bool is_wait_for_reply{ rcon_commands_wait_for_reply.find(command_parts[0]) != std::cend(rcon_commands_wait_for_reply) };
        main_app.add_command_to_queue(std::move(command_parts), command_type::rcon, is_wait_for_reply);
      }
    }
  }
}

void process_user_command(const std::vector<string> &user_cmd)
{
  if (!user_cmd.empty()) {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2Executing user command: ^5"s + str_join(user_cmd, " ") + "\n"s }.c_str(), true, true, true);
    if (user_cmd[0] == "!w" || user_cmd[0] == "!warn") {

      if (user_cmd.size() > 1 && !user_cmd[1].empty()) {


        if (check_if_user_provided_argument_is_valid_for_specified_command(
              user_cmd[0].c_str(), user_cmd[1])) {
          const int pid{ stoi(user_cmd[1]) };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
          string reason{ user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified" };
          stl::helper::trim_in_place(reason);
          specify_reason_for_player_pid(pid, reason);
          main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
          string command{ main_app.get_user_defined_warn_message() };
          build_tiny_rcon_message(command);
          rcon_say(command);
          auto &warned_players = main_app.get_game_server().get_warned_players_data();
          auto [player, is_online] = get_online_player_for_specified_pid(pid);
          if (is_online) {
            const auto iter = warned_players.find(pid);
            if (iter == end(warned_players)) {
              warned_players[pid] = move(player);
              warned_players[pid].warned_times = 1;
            } else {
              ++warned_players[pid].warned_times;
            }

            const string message{ "^3You have successfully executed ^5!warn ^3on player ("s + get_player_information(pid) + "^3)\n"s };
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);

            const size_t number_of_warnings_for_automatic_kick = main_app.get_maximum_number_of_warnings_for_automatic_kick();
            if (warned_players[player.pid].warned_times >= number_of_warnings_for_automatic_kick) {
              main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
              string reason2{ "^1Received "s + to_string(main_app.get_maximum_number_of_warnings_for_automatic_kick()) + " warnings from admin: "s + main_app.get_username() };
              stl::helper::trim_in_place(reason2);
              specify_reason_for_player_pid(player.pid, reason2);
              main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason2);
              string command2{ main_app.get_user_defined_kick_message() };
              build_tiny_rcon_message(command2);
              /*print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, true, false);
              print_colored_text(app_handles.hwnd_re_messages_data, command2.c_str(), true, true, true);
              print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, true, false);*/
              kick_player(player.pid, command2);
              warned_players.erase(player.pid);
            }
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + user_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }

    } else if (user_cmd[0] == "!k" || user_cmd[0] == "!kick") {
      if (user_cmd.size() > 1 && !user_cmd[1].empty()) {


        if (check_if_user_provided_argument_is_valid_for_specified_command(
              user_cmd[0].c_str(), user_cmd[1])) {
          const int pid{ stoi(user_cmd[1]) };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
          string reason{ user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified" };
          stl::helper::trim_in_place(reason);
          specify_reason_for_player_pid(pid, reason);
          main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
          const string message{ "^3You have successfully executed ^5clientkick ^3on player ("s + get_player_information(pid) + "^3)\n"s };
          print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
          string command{ main_app.get_user_defined_kick_message() };
          build_tiny_rcon_message(command);
          kick_player(pid, command);

        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2"s + user_cmd[1] + " ^3is not a valid pid number for the ^2!k ^3(^2!kick^3) command!\n"s }.c_str(), true, true, true);
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + user_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (user_cmd[0] == "!tb" || user_cmd[0] == "!tempban") {
      if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
        if (check_if_user_provided_argument_is_valid_for_specified_command(
              user_cmd[0].c_str(), user_cmd[1])) {
          const int pid{ stoi(user_cmd[1]) };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
          int number{};
          size_t temp_ban_duration{ 24 };
          string reason{
            "not specified"
          };
          if (user_cmd.size() > 2) {
            if (is_valid_decimal_whole_number(user_cmd[2], number)) {
              temp_ban_duration = number > 0 && number <= 9999 ? number : 24;
              if (user_cmd.size() > 3) {
                reason = str_join(cbegin(user_cmd) + 3, cend(user_cmd), " ");
                stl::helper::trim_in_place(reason);
              }
            } else {
              reason = str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ");
              stl::helper::trim_in_place(reason);
            }
          }
          main_app.get_tinyrcon_dict()["{REASON}"] = reason;
          main_app.get_tinyrcon_dict()["{TEMPBAN_DURATION}"] = to_string(temp_ban_duration);
          const string message{ "^3You have successfully executed ^5!tempban ^3on player ("s + get_player_information(pid) + "^3)\n"s };
          print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
          string command{ main_app.get_user_defined_tempban_message() };
          build_tiny_rcon_message(command);
          player_data &pd = get_player_data_for_pid(pid);
          pd.ban_duration_in_hours = temp_ban_duration;
          strcpy_s(pd.reason, std::size(pd.reason), reason.c_str());
          tempban_player(pd, command);
        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2"s + user_cmd[1] + " ^3is not a valid pid number for the ^2!tb ^3(^2!tempban^3) command!\n"s }.c_str(), true, true, true);
        }
        is_display_temporarily_banned_players_data_event.store(true);
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + user_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (user_cmd[0] == "!b" || user_cmd[0] == "!ban") {
      if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
        if (check_if_user_provided_argument_is_valid_for_specified_command(
              user_cmd[0].c_str(), user_cmd[1])) {
          const int pid{ stoi(user_cmd[1]) };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
          string reason{ user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified" };
          stl::helper::trim_in_place(reason);
          specify_reason_for_player_pid(pid, reason);
          main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
          const string message{ "^3You have successfully executed ^5banclient ^3on player ("s + get_player_information(pid) + "^3)\n"s };
          print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
          string command{ main_app.get_user_defined_ban_message() };
          build_tiny_rcon_message(command);
          ban_player(pid, command);

        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2"s + user_cmd[1] + " ^3is not a valid pid number for the ^2!b ^3(^2!ban^3) command!\n"s }.c_str(), true, true, true);
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + user_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (user_cmd[0] == "!gb" || user_cmd[0] == "!globalban" || user_cmd[0] == "!banip" || user_cmd[0] == "!addip") {
      if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
        if (check_if_user_provided_argument_is_valid_for_specified_command(
              "!gb", user_cmd[1])) {
          const auto &banned_ip_addresses =
            main_app.get_game_server().get_set_of_banned_ip_addresses();
          if (int pid{ -1 }; is_valid_decimal_whole_number(user_cmd[1], pid)) {
            auto &player = main_app.get_game_server().get_player_data(pid);
            if (pid == player.pid) {
              unsigned long guid{};
              if (check_ip_address_validity(player.ip_address, guid) && banned_ip_addresses.find(player.ip_address) == cend(banned_ip_addresses)) {

                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
                string reason{ user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified" };
                stl::helper::trim_in_place(reason);
                specify_reason_for_player_pid(player.pid, reason);
                global_ban_player_ip_address(player);
                print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully banned IP address: ^1"s + player.ip_address + "\n"s }.c_str(), true, true, true);
                main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                const string message{
                  "^3You have successfully executed ^5"s + user_cmd[0] + " ^3on player ("s + get_player_information(pid) + "^3)\n"s
                };
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
                string command{ main_app.get_user_defined_ipban_message() };
                build_tiny_rcon_message(command);
                kick_player(player.pid, command);
              }
            }
          } else {
            const auto &ip_address = user_cmd[1];
            if (banned_ip_addresses.find(ip_address) == cend(banned_ip_addresses)) {
              bool is_ip_address_already_banned{};
              auto &players_data = main_app.get_game_server().get_players_data();
              for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
                auto &player = players_data[i];
                if (player.ip_address == ip_address) {
                  main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
                  string reason{ user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified" };
                  stl::helper::trim_in_place(reason);
                  specify_reason_for_player_pid(player.pid, reason);
                  global_ban_player_ip_address(player);
                  print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully banned IP address: ^1"s + ip_address + "\n"s }.c_str(), true, true, true);
                  main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                  const string message{
                    "^3You have successfully executed ^5"s + user_cmd[0] + " ^3on player ("s + get_player_information(player.pid) + "^3)\n"s
                  };
                  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
                  string command{ main_app.get_user_defined_ipban_message() };
                  build_tiny_rcon_message(command);
                  kick_player(player.pid, command);
                  is_ip_address_already_banned = true;
                }
              }

              if (!is_ip_address_already_banned) {
                player_data player_offline{};
                player_offline.pid = -1;
                strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                strcpy_s(player_offline.ip_address, std::size(player_offline.ip_address), ip_address.c_str());
                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                string reason{ user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified" };
                strcpy_s(player_offline.reason, std::size(player_offline.reason), reason.c_str());
                global_ban_player_ip_address(player_offline);
                main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                string command{ main_app.get_user_defined_ipban_message() };
                build_tiny_rcon_message(command);
                rcon_say(command);
                print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully banned IP address: ^1"s + ip_address + "\n"s }.c_str(), true, true, true);
                const string message{
                  "^2You have successfully executed ^5"s + user_cmd[0] + "^2on player ("s + get_player_information_for_player(player_offline) + " ^ 2)\n"s
                };
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
              }
            }
          }
          is_display_permanently_banned_players_data_event.store(true);
        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3You specified an invalid non-existing ^1pid ^3or invalid ^1IP address ^3for the ^1" + user_cmd[0] + " ^3command: ^1"s + user_cmd[1] + "\n"s }.c_str(), true, true, true);
          if (user_commands_help.contains(user_cmd[0])) {
            print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
            print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + user_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (user_cmd[0] == "s" || user_cmd[0] == "!s" || user_cmd[0] == "status" || user_cmd[0] == "!status") {
      initiate_sending_rcon_status_command_now();
    } else if (user_cmd[0] == "gs" || user_cmd[0] == "!gs" || user_cmd[0] == "getstatus" || user_cmd[0] == "!getstatus") {
      main_app.add_command_to_queue({ "getstatus" }, command_type::rcon, true);
    } else if (user_cmd[0] == "!t" || user_cmd[0] == "!time") {
      const std::chrono::time_point<std::chrono::system_clock> now =
        std::chrono::system_clock::now();
      const time_t t_c = std::chrono::system_clock::to_time_t(now);
      tm time_info{};
      localtime_s(&time_info, &t_c);
      ostringstream oss;
      oss << "^2Current time: ^1" << put_time(&time_info, "%Y-%b-%d %T\n");
      const string time_str{ oss.str() };
      print_colored_text(app_handles.hwnd_re_messages_data, time_str.c_str(), true, true, true);
    } else if (user_cmd[0] == "!list") {
      if (user_cmd.size() == 2) {
        print_help_information(user_cmd);
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^1"s + user_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (user_cmd[0] == "sort" || user_cmd[0] == "!sort") {
      if (user_cmd.size() == 3) {
        sort_type new_sort_type{ sort_type::pid_asc };
        if (user_cmd[1] == "pid" && user_cmd[2] == "asc")
          new_sort_type = sort_type::pid_asc;
        else if (user_cmd[1] == "pid" && user_cmd[2] == "desc")
          new_sort_type = sort_type::pid_desc;
        else if (user_cmd[1] == "score" && user_cmd[2] == "asc")
          new_sort_type = sort_type::score_asc;
        else if (user_cmd[1] == "score" && user_cmd[2] == "desc")
          new_sort_type = sort_type::score_desc;
        else if (user_cmd[1] == "ping" && user_cmd[2] == "asc")
          new_sort_type = sort_type::ping_asc;
        else if (user_cmd[1] == "ping" && user_cmd[2] == "desc")
          new_sort_type = sort_type::ping_desc;
        else if (user_cmd[1] == "ip" && user_cmd[2] == "asc")
          new_sort_type = sort_type::ip_asc;
        else if (user_cmd[1] == "ip" && user_cmd[2] == "desc")
          new_sort_type = sort_type::ip_desc;
        else if (user_cmd[1] == "name" && user_cmd[2] == "asc")
          new_sort_type = sort_type::name_asc;
        else if (user_cmd[1] == "name" && user_cmd[2] == "desc")
          new_sort_type = sort_type::name_desc;
        else if (user_cmd[1] == "geo" && user_cmd[2] == "asc")
          new_sort_type = sort_type::geo_asc;
        else if (user_cmd[1] == "geo" && user_cmd[2] == "desc")
          new_sort_type = sort_type::geo_desc;

        type_of_sort = new_sort_type;
        if (main_app.get_is_connection_settings_valid()) {
          initiate_sending_rcon_status_command_now();
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^1"s + user_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (user_cmd[0] == "bans" || user_cmd[0] == "!bans") {
      if (user_cmd.size() == 3U && user_cmd[1] == "clear" && user_cmd[2] == "all") {
        for (const auto &ip : main_app.get_game_server().get_set_of_banned_ip_addresses()) {
          string message;
          remove_permanently_banned_ip_address(ip, message);
        }
      }
      is_display_permanently_banned_players_data_event.store(true);

    } else if (user_cmd[0] == "tempbans" || user_cmd[0] == "!tempbans") {
      if (user_cmd.size() == 3U && user_cmd[1] == "clear" && user_cmd[2] == "all") {
        for (const auto &ip : main_app.get_game_server().get_set_of_temp_banned_ip_addresses()) {
          string message;
          remove_temp_banned_ip_address(ip, message, false);
        }
      }

      is_display_temporarily_banned_players_data_event.store(true);

    } else if (user_cmd[0] == "!ub" || user_cmd[0] == "!unban") {
      if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
        unsigned long guid{};
        if (!check_ip_address_validity(user_cmd[1], guid)) {
          print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Provided IP address (^1"s + user_cmd[1] + "^3) is not a valid IP address!\n"s }.c_str(), true, true, true);
        } else {
          string message;
          if (remove_temp_banned_ip_address(user_cmd[1], message, false)) {
            print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully removed previously temporarily banned IP address: ^1"s + user_cmd[1] + "\n"s }.c_str(), true, true, true);
          } else {
            print_colored_text(app_handles.hwnd_re_messages_data,
              string{ "^3Provided IP address (^1"s + user_cmd[1] + "^3) hasn't been temporarily banned yet!\n"s }
                .c_str(),
              true,
              true,
              true);
          }

          if (remove_permanently_banned_ip_address(user_cmd[1], message)) {
            print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully removed previously permanently banned IP address: ^1"s + user_cmd[1] + "\n"s }.c_str(), true, true, true);
          } else {
            print_colored_text(app_handles.hwnd_re_messages_data,
              string{ "^3Provided IP address (^1"s + user_cmd[1] + "^3) hasn't been permanently banned yet!\n"s }
                .c_str(),
              true,
              true,
              true);
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^1"s + user_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (user_cmd[0] == "!c" || user_cmd[0] == "!cp") {
      const bool use_private_slot{ user_cmd[0] == "!cp" && !main_app.get_game_server().get_private_slot_password().empty() };
      smatch ip_port_match{};
      const string ip_port_server_address{
        (user_cmd.size() > 1 && regex_search(user_cmd[1], ip_port_match, ip_address_and_port_regex)) ? (ip_port_match[1].str() + ":"s + ip_port_match[2].str()) : (main_app.get_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_game_server().get_server_port()))
      };
      const size_t sep_pos{ ip_port_server_address.find(':') };
      const string ip_address{ ip_port_server_address.substr(0, sep_pos) };
      const uint16_t port_number{ static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1))) };
      const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(ip_address.c_str(), port_number, main_app.get_game_server().get_rcon_password().c_str());

      const game_name_t game_name{ result.second != game_name_t::unknown ? result.second : main_app.get_game_name() };

      connect_to_the_game_server(ip_port_server_address, game_name, use_private_slot, true);
    }

    else if ((user_cmd[0] == "!m" || user_cmd[0] == "!map") && user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      const string map_name{ stl::helper::trim(user_cmd[1], " \t\n") };
      const string game_type{ user_cmd.size() >= 3 ? stl::helper::trim(user_cmd[2], " \t\n") : "ctf" };
      load_map(map_name, game_type, true);
    } else if (user_cmd[0] == "maps" || user_cmd[0] == "!maps") {
      display_all_available_maps();
    } else if (user_cmd[0] == "colors" || user_cmd[0] == "!colors") {
      change_colors();
    } else if (user_cmd[0] == "config" || user_cmd[0] == "!config") {
      change_server_setting(user_cmd);
    } else if (int number{}; ((user_cmd[0] == "!rt" || user_cmd[0] == "!refreshtime") && (user_cmd.size() == 2) && is_valid_decimal_whole_number(user_cmd[1], number))) {
      main_app.get_game_server().set_check_for_banned_players_time_period(number);
      KillTimer(app_handles.hwnd_main_window, ID_TIMER);
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, main_app.get_game_server().get_check_for_banned_players_time_period()));
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, 0, 0);
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETSTEP, 1, 0);
      SetTimer(app_handles.hwnd_main_window, ID_TIMER, 1000, nullptr);

      print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully changed the ^1time period\n for automatic checking for banned IP addresses ^2to ^5"s + user_cmd[1] + "\n"s }.c_str(), true, true, true);
      write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
    } else if ((user_cmd.size() >= 2) && (user_cmd[0] == "border" || user_cmd[0] == "!border") && (user_cmd[1] == "on" || user_cmd[1] == "off")) {

      const bool new_setting{ user_cmd[1] == "on" ? true : false };
      print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully executed command: ^1" + user_cmd[0] + " " + user_cmd[1] + "\n"s }.c_str(), true, true, true);
      if (main_app.get_is_draw_border_lines() != new_setting) {
        main_app.set_is_draw_border_lines(new_setting);


        if (new_setting) {
          print_colored_text(app_handles.hwnd_re_messages_data, "^2You have successfully turned on the border lines\n around displayed ^3GUI controls^2.\n", true, true, true);
        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, "^2You have successfully turned off the border lines\n around displayed ^3GUI controls^2.\n", true, true, true);
        }

        construct_tinyrcon_gui(app_handles.hwnd_main_window);

        write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
      }
    }
  }
}

void process_rcon_command(const std::vector<string> &rcon_cmd, const bool)
{
  string reply;
  if (!rcon_cmd.empty()) {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2Executing rcon command: ^5"s + str_join(rcon_cmd, " ") + "\n"s }.c_str(), true, true, true);
    if (rcon_cmd[0] == "s" || rcon_cmd[0] == "status") {
      initiate_sending_rcon_status_command_now();
    } else if ((rcon_cmd[0] == "say" || rcon_cmd[0] == "!say") && rcon_cmd.size() >= 2) {
      string command{ stl::helper::str_join(rcon_cmd, " ") };
      if (!command.empty() && '!' == command[0])
        command.erase(0, 1);
      main_app.get_connection_manager().send_and_receive_rcon_data(command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
    } else if ((rcon_cmd[0] == "tell" || rcon_cmd[0] == "!tell") && rcon_cmd.size() >= 3 && !rcon_cmd[1].empty()) {
      string player_pid{ rcon_cmd[1] };
      stl::helper::trim_in_place(player_pid);
      if (int pid{ -1 }; is_valid_decimal_whole_number(player_pid, pid)) {
        const auto &players_data = main_app.get_game_server().get_players_data();
        for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
          if (players_data[i].pid == pid) {
            string command{ "tell "s + player_pid };
            for (size_t j{ 2 }; j < rcon_cmd.size(); ++j) {
              command.append(" ").append(rcon_cmd[j]);
            }
            main_app.get_connection_manager().send_and_receive_rcon_data(command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
            break;
          }
        }
      }
    } else if (rcon_cmd[0] == "clientkick") {
      if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {

        if (check_if_user_provided_argument_is_valid_for_specified_command(
              rcon_cmd[0].c_str(), rcon_cmd[1])) {
          const int pid{ stoi(rcon_cmd[1]) };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
          string reason{ rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified" };
          stl::helper::trim_in_place(reason);
          specify_reason_for_player_pid(pid, reason);
          main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
          const string message{ "^3You have successfully executed ^5clientkick ^3on player ("s + get_player_information(pid) + "^3)\n"s };
          print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
          string command{ main_app.get_user_defined_kick_message() };
          build_tiny_rcon_message(command);
          kick_player(pid, command);

        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2"s + rcon_cmd[1] + " ^3is not a valid pid number for the ^2!k ^3(^2!kick^3) command!\n"s }.c_str(), true, true, true);
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + rcon_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(rcon_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (rcon_cmd[0] == "tempbanclient") {
      if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
        if (check_if_user_provided_argument_is_valid_for_specified_command(
              rcon_cmd[0].c_str(), rcon_cmd[1])) {
          const int pid{ stoi(rcon_cmd[1]) };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
          int number{};
          size_t temp_ban_duration{ 24 };
          string reason{
            "not specified"
          };
          if (rcon_cmd.size() > 2) {
            if (is_valid_decimal_whole_number(rcon_cmd[2], number)) {
              temp_ban_duration = number > 0 && number <= 9999 ? number : 24;
              if (rcon_cmd.size() > 3) {
                reason = str_join(cbegin(rcon_cmd) + 3, cend(rcon_cmd), " ");
                stl::helper::trim_in_place(reason);
              }
            } else {
              reason = str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ");
              stl::helper::trim_in_place(reason);
            }
          }
          main_app.get_tinyrcon_dict()["{REASON}"] = reason;
          main_app.get_tinyrcon_dict()["{TEMPBAN_DURATION}"] = to_string(temp_ban_duration);
          const string message{ "^3You have successfully executed ^5!tempban ^3on player ("s + get_player_information(pid) + "^3)\n"s };
          print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
          string command{ main_app.get_user_defined_tempban_message() };
          build_tiny_rcon_message(command);
          player_data &pd = get_player_data_for_pid(pid);
          pd.ban_duration_in_hours = temp_ban_duration;
          strcpy_s(pd.reason, std::size(pd.reason), reason.c_str());
          tempban_player(pd, command);
          is_display_temporarily_banned_players_data_event.store(true);
        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2"s + rcon_cmd[1] + " ^3is not a valid pid number for the ^2!tb ^3(^2!tempban^3) command!\n"s }.c_str(), true, true, true);
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + rcon_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(rcon_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (rcon_cmd[0] == "banclient") {
      if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
        if (check_if_user_provided_argument_is_valid_for_specified_command(
              rcon_cmd[0].c_str(), rcon_cmd[1])) {
          const int pid{ stoi(rcon_cmd[1]) };
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
          string reason{ rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified" };
          stl::helper::trim_in_place(reason);
          specify_reason_for_player_pid(pid, reason);
          main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
          const string message{ "^3You have successfully executed ^5banclient ^3on player ("s + get_player_information(pid) + "^3)\n"s };
          print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
          string command{ main_app.get_user_defined_ban_message() };
          build_tiny_rcon_message(command);
          ban_player(pid, command);
        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2"s + rcon_cmd[1] + " ^3is not a valid pid number for the ^2!b ^3(^2!ban^3) command!\n"s }.c_str(), true, true, true);
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + rcon_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(rcon_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else if (rcon_cmd[0] == "kick" || rcon_cmd[0] == "onlykick" || rcon_cmd[0] == "tempbanuser" || rcon_cmd[0] == "banuser") {
      if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
        bool is_player_found{};
        const bool remove_player_color_codes{ rcon_cmd[0] == "onlykick" };
        const auto &players_data = main_app.get_game_server().get_players_data();
        for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
          const player_data &player{ players_data[i] };
          string player_name{ player.player_name };
          if (remove_player_color_codes)
            remove_all_color_codes(player_name);
          if (rcon_cmd[1] == player_name) {
            is_player_found = true;
            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
            string reason{ rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified" };
            stl::helper::trim_in_place(reason);
            specify_reason_for_player_pid(player.pid, reason);
            main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
            const string message{ "^3You have successfully executed ^5"s + rcon_cmd[0] + " ^3on player ("s + get_player_information(player.pid) + "^3)\n"s };
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
            string public_message{ rcon_cmd[0] == "kick" || rcon_cmd[0] == "onlykick" ? main_app.get_user_defined_kick_message() : main_app.get_user_defined_tempban_message() };
            build_tiny_rcon_message(public_message);
            const string command{ rcon_cmd[0] + " "s + rcon_cmd[1] };
            main_app.get_connection_manager().send_and_receive_rcon_data(
              command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
            kick_player(player.pid, public_message);
            break;
          }
        }

        if (!is_player_found) {
          if (string player_name{ rcon_cmd[1] }; remove_player_color_codes) {
            remove_all_color_codes(player_name);
            ostringstream oss;
            oss << "^3Could not find player with specified name (^5" << player_name << "^3) to use with the ^2" << rcon_cmd[0] << " ^3command!\n";
            print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(), true, true, true);
          } else {
            ostringstream oss;
            oss << "^3Could not find player with specified name (^5" << rcon_cmd[1] << "^3) to use with the ^2" << rcon_cmd[0] << " ^3command!\n";
            print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(), true, true, true);
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Invalid command syntax for user command: ^2"s + rcon_cmd[0] + "\n"s }.c_str(), true, true, true);
        if (user_commands_help.contains(rcon_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(), true, false);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, false);
        }
      }
    } else {
      if (rcon_cmd.size() > 1) {
        const string command{ str_join(rcon_cmd, " ") };
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^5Sending rcon command '"s + command + "' to the server.\n"s }.c_str(), true, true, true);
        main_app.get_connection_manager().send_and_receive_rcon_data(
          command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), true);
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, string{ "^5Sending rcon command '"s + rcon_cmd[0] + "' to the server.\n"s }.c_str(), true, true, true);
        main_app.get_connection_manager().send_and_receive_rcon_data(
          rcon_cmd[0].c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), true);
      }
      if (!reply.empty()) {
        while (true) {
          const size_t pos =
            stl::helper::str_index_of(reply, "\377\377\377\377print\n");
          if (string::npos == pos)
            break;
          reply.erase(pos, 10);
        }
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, true, true);
        print_colored_text(app_handles.hwnd_re_messages_data, reply.c_str(), true, true, false);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", true, true, false);
      }
    }
  }
}

atomic<bool> should_program_terminate(const string &user_input) noexcept
{
  if (user_input.empty())
    return is_terminate_program;
  is_terminate_program.store(user_input == "q" || user_input == "!q" || user_input == "!quit" || user_input == "quit" || user_input == "e" || user_input == "!e" || user_input == "exit" || user_input == "!exit");
  return is_terminate_program;
}

void sort_players_data(std::vector<player_data> &players_data, const sort_type sort_method)
{
  const size_t number_of_players{ main_app.get_game_server().get_number_of_players() };
  switch (sort_method) {
  case sort_type::pid_asc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      return pl1.pid < pl2.pid;
    });
    break;

  case sort_type::pid_desc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      return pl1.pid > pl2.pid;
    });
    break;

  case sort_type::score_asc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      return pl1.score < pl2.score;
    });
    break;

  case sort_type::score_desc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      return pl1.score > pl2.score;
    });
    break;

  case sort_type::ping_asc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      return strcmp(pl1.ping, pl2.ping) < 0;
    });
    break;

  case sort_type::ping_desc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      return strcmp(pl1.ping, pl2.ping) > 0;
    });
    break;

  case sort_type::ip_asc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      unsigned long pl1_guid{}, pl2_guid{};
      if (!check_ip_address_validity(pl1.ip_address, pl1_guid))
        return true;
      if (!check_ip_address_validity(pl2.ip_address, pl2_guid))
        return false;
      return pl1_guid < pl2_guid;
    });
    break;

  case sort_type::ip_desc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      unsigned long pl1_guid{}, pl2_guid{};
      if (!check_ip_address_validity(pl1.ip_address, pl1_guid))
        return false;
      if (!check_ip_address_validity(pl2.ip_address, pl2_guid))
        return true;
      return pl1_guid > pl2_guid;
    });
    break;

  case sort_type::name_asc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      string pl1_cleaned_lc_name{ pl1.player_name };
      string pl2_cleaned_lc_name{ pl2.player_name };
      remove_all_color_codes(pl1_cleaned_lc_name);
      remove_all_color_codes(pl2_cleaned_lc_name);
      to_lower_case_in_place(pl1_cleaned_lc_name);
      to_lower_case_in_place(pl2_cleaned_lc_name);
      return pl1_cleaned_lc_name < pl2_cleaned_lc_name;
    });
    break;

  case sort_type::name_desc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      string pl1_cleaned_lc_name{ pl1.player_name };
      string pl2_cleaned_lc_name{ pl2.player_name };
      remove_all_color_codes(pl1_cleaned_lc_name);
      remove_all_color_codes(pl2_cleaned_lc_name);
      to_lower_case_in_place(pl1_cleaned_lc_name);
      to_lower_case_in_place(pl2_cleaned_lc_name);
      return pl1_cleaned_lc_name > pl2_cleaned_lc_name;
    });
    break;

  case sort_type::geo_asc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      char buffer1[256];
      snprintf(buffer1, std::size(buffer1), "%s, %s", (len(pl1.country_name) > 0 ? pl1.country_name : pl1.region), pl1.city);
      char buffer2[256];
      snprintf(buffer2, std::size(buffer2), "%s, %s", (len(pl2.country_name) > 0 ? pl2.country_name : pl2.region), pl2.city);
      string pl1_cleaned_geo{ buffer1 };
      string pl2_cleaned_geo{ buffer2 };
      to_lower_case_in_place(pl1_cleaned_geo);
      to_lower_case_in_place(pl2_cleaned_geo);
      return pl1_cleaned_geo < pl2_cleaned_geo;
    });
    break;

  case sort_type::geo_desc:
    std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
      char buffer1[256];
      snprintf(buffer1, std::size(buffer1), "%s, %s", (len(pl1.country_name) > 0 ? pl1.country_name : pl1.region), pl1.city);
      char buffer2[256];
      snprintf(buffer2, std::size(buffer2), "%s, %s", (len(pl2.country_name) > 0 ? pl2.country_name : pl2.region), pl2.city);
      string pl1_cleaned_geo{ buffer1 };
      string pl2_cleaned_geo{ buffer2 };
      to_lower_case_in_place(pl1_cleaned_geo);
      to_lower_case_in_place(pl2_cleaned_geo);
      return pl2_cleaned_geo < pl1_cleaned_geo;
    });
    break;

  case sort_type::unknown:
    break;
  }
}

void display_temporarily_banned_ip_addresses()
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
  const string decoration_line(98 + longest_name_length + longest_country_length, '=');
  oss << "^5\n"
      << decoration_line << "\n"
      << "^5| ";
  log << "\n"
      << decoration_line << "\n"
      << "| ";
  oss << left << setw(15) << "IP address"
      << " | " << left << setw(longest_name_length)
      << "Player name"
      << " | " << left << setw(longest_country_length) << "Country, city"
      << " | " << left << setw(20) << "Tempban issued on"
      << " | " << left << setw(20) << "Tempban expires on"
      << " | " << left << setw(25) << "Reason"
      << "|";
  log << left << setw(15) << "IP address"
      << " | " << left << setw(longest_name_length)
      << "Player name"
      << " | " << left << setw(longest_country_length) << "Country, city"
      << " | " << left << setw(20) << "Tempban issued on"
      << " | " << left << setw(20) << "Tempban expires on"
      << " | " << left << setw(25) << "Reason"
      << "|";
  oss << "^5\n"
      << decoration_line << "\n";
  log << "\n"
      << decoration_line << "\n";
  if (temp_banned_players.empty()) {
    const size_t message_len = stl::helper::len("| There are no players temporarily banned by their IP addresses yet.");
    oss << "^5| ^3There are no players temporarily banned by their IP addresses yet.";
    log << "| There are no players temporarily banned by their IP addresses yet.";

    if (message_len + 2 < decoration_line.length()) {
      oss << string(decoration_line.length() - 2 - message_len, ' ');
      log << string(decoration_line.length() - 2 - message_len, ' ');
    }
    oss << " ^5|\n";
    log << " |\n";
  } else {
    bool is_first_color{ true };
    for (auto &bp : temp_banned_players) {
      const char *next_color{ is_first_color ? "^5" : "^3" };
      oss << next_color << "| " << left << setw(15) << bp.ip_address << " | ^7";
      log << "| " << left << setw(15) << bp.ip_address << " | ";
      stl::helper::trim_in_place(bp.player_name);
      char name[33];
      strcpy_s(name, 33, bp.player_name);
      remove_all_color_codes(name);
      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(bp.player_name) };
      const size_t printed_name_char_count2{ get_number_of_characters_without_color_codes(name) };
      if (printed_name_char_count1 < longest_name_length) {
        oss << left << setw(longest_name_length) << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
      } else {
        oss << left << setw(longest_name_length) << bp.player_name;
      }
      if (printed_name_char_count2 < longest_name_length) {
        log << left << setw(longest_name_length) << name + string(longest_name_length - printed_name_char_count2, ' ');
      } else {
        log << left << setw(longest_name_length) << name;
      }
      oss << next_color << " | ";
      log << " | ";
      char buffer2[256];
      snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region), bp.city);
      char reason[128];
      const size_t no_of_chars_to_copy = std::min<size_t>(127U, len(bp.reason));
      strncpy_s(reason, std::size(reason), bp.reason, no_of_chars_to_copy);
      reason[no_of_chars_to_copy] = '\0';
      // stl::helper::trim_in_place(reason);
      const time_t ban_expires_time = bp.banned_start_time + (bp.ban_duration_in_hours * 3600);
      const string ban_start_date_str = get_date_and_time_for_time_t(bp.banned_start_time);
      const string ban_expires_date_str = get_date_and_time_for_time_t(ban_expires_time);
      oss << left << setw(longest_country_length) << buffer2 << " ^5| " << left << setw(20) << ban_start_date_str
          << " | " << left << setw(20) << ban_expires_date_str << " | ";
      const size_t printed_reason_char_count1{ get_number_of_characters_without_color_codes(reason) };
      if (printed_reason_char_count1 < 25) {
        oss << left << setw(25) << (reason + string(25 - printed_reason_char_count1, ' '));
      } else {
        oss << reason;
      }
      oss << "^5|\n";

      strncpy_s(reason, std::size(reason), bp.reason, no_of_chars_to_copy);
      reason[no_of_chars_to_copy] = '\0';
      remove_all_color_codes(reason);
      // stl::helper::trim_in_place(reason);
      log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20) << ban_start_date_str
          << " | " << left << setw(20) << ban_expires_date_str << " | ";
      const size_t printed_reason_char_count2{ get_number_of_characters_without_color_codes(reason) };
      if (printed_reason_char_count2 < 25) {
        log << left << setw(25) << (reason + string(25 - printed_reason_char_count2, ' '));
      } else {
        log << reason;
      }
      log << "|\n";
      is_first_color = !is_first_color;
    }
  }
  oss << string{ "^5"s + decoration_line + "\n\n"s };
  log << string{ decoration_line + "\n\n"s };
  const string message{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
  log_message(log.str(), true);
}

void display_permanently_banned_ip_addresses()
{
  size_t longest_name_length{ 12 };
  size_t longest_country_length{ 20 };
  auto &banned_players =
    main_app.get_game_server().get_banned_players_data();
  if (!banned_players.empty()) {
    longest_name_length = std::max(longest_name_length, find_longest_player_name_length(banned_players, false, banned_players.size()));
    longest_country_length =
      std::max(longest_country_length,
        find_longest_player_country_city_info_length(banned_players, banned_players.size()));
  }

  ostringstream oss;
  ostringstream log;
  const string decoration_line(79 + longest_name_length + longest_country_length, '=');
  string buffer{ "^5\n"s + decoration_line + "\n"s };
  print_colored_text(app_handles.hwnd_re_messages_data,
    buffer.c_str(),
    true,
    false,
    false);
  oss << "^5| ";
  log << "\n"
      << decoration_line << "\n"
      << "| ";
  oss << left << setw(15) << "IP address"
      << " | " << left << setw(longest_name_length)
      << "Player name"
      << " | " << left << setw(longest_country_length) << "Country, city"
      << " | " << left << setw(20) << "Date/time of IP ban"
      << " | " << left << setw(29) << "Reason"
      << "|";
  log << left << setw(15) << "IP address"
      << " | " << left << setw(longest_name_length)
      << "Player name"
      << " | " << left << setw(longest_country_length) << "Country, city"
      << " | " << left << setw(20) << "Date/time of IP ban"
      << " | " << left << setw(29) << "Reason"
      << "|";
  oss << "^5\n"
      << decoration_line << "\n";
  log << "\n"
      << decoration_line << "\n";
  if (banned_players.empty()) {
    const size_t message_len = stl::helper::len("| There are no players permanently banned by their IP addresses yet.");
    oss << "^5| ^3There are no players permanently banned by their IP addresses yet.";
    log << "| There are no players permanently banned by their IP addresses yet.";

    if (message_len + 2 < decoration_line.length()) {
      oss << string(decoration_line.length() - 2 - message_len, ' ');
      log << string(decoration_line.length() - 2 - message_len, ' ');
    }
    oss << " ^5|\n";
    log << " |\n";
  } else {
    bool is_first_color{ true };
    for (auto &bp : banned_players) {
      const char *next_color{ is_first_color ? "^5" : "^3" };
      oss << next_color << "| " << left << setw(15) << bp.ip_address << " | ^7";
      log << "| " << left << setw(15) << bp.ip_address << " | ";
      stl::helper::trim_in_place(bp.player_name);
      char name[33];
      strcpy_s(name, 33, bp.player_name);
      remove_all_color_codes(name);
      const size_t printed_name_char_count1{ get_number_of_characters_without_color_codes(bp.player_name) };
      const size_t printed_name_char_count2{ get_number_of_characters_without_color_codes(name) };
      if (printed_name_char_count1 < longest_name_length) {
        oss << left << setw(longest_name_length) << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
      } else {
        oss << left << setw(longest_name_length) << bp.player_name;
      }
      if (printed_name_char_count2 < longest_name_length) {
        log << left << setw(longest_name_length) << name + string(longest_name_length - printed_name_char_count2, ' ');
      } else {
        log << left << setw(longest_name_length) << name;
      }
      oss << next_color << " | ";
      log << " | ";
      char buffer2[256];
      snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region), bp.city);
      char reason[128];
      const size_t no_of_chars_to_copy = std::min<size_t>(127U, len(bp.reason));
      strncpy_s(reason, std::size(reason), bp.reason, no_of_chars_to_copy);
      reason[no_of_chars_to_copy] = '\0';
      // stl::helper::trim_in_place(reason);
      oss << left << setw(longest_country_length) << buffer2 << " ^5| " << left << setw(20) << bp.banned_date_time
          << " | ";
      const size_t printed_reason_char_count1{ get_number_of_characters_without_color_codes(reason) };
      if (printed_reason_char_count1 < 29) {
        oss << left << setw(29) << (reason + string(29 - printed_reason_char_count1, ' '));
      } else {
        oss << reason;
      }
      oss << "^5|\n";

      strncpy_s(reason, std::size(reason), bp.reason, no_of_chars_to_copy);
      reason[no_of_chars_to_copy] = '\0';
      remove_all_color_codes(reason);
      // stl::helper::trim_in_place(reason);
      log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20) << bp.banned_date_time
          << " | ";
      const size_t printed_reason_char_count2{ get_number_of_characters_without_color_codes(reason) };
      if (printed_reason_char_count2 < 29) {
        log << left << setw(29) << (reason + string(29 - printed_reason_char_count2, ' '));
      } else {
        log << reason;
      }
      log << "|\n";
      is_first_color = !is_first_color;
    }
  }
  oss << string{ "^5"s + decoration_line + "\n\n"s };
  log << string{ decoration_line + "\n\n"s };
  const string message{ oss.str() };
  print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, false, false);
  log_message(log.str(), true);
}

const std::string &get_full_gametype_name(const std::string &rcon_gametype_name)
{
  const auto &full_gametype_names = get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());
  return full_gametype_names.find(rcon_gametype_name) != cend(full_gametype_names) ? full_gametype_names.at(rcon_gametype_name) : rcon_gametype_name;
}


const std::string &get_full_map_name(const std::string &rcon_map_name)
{
  const auto &rcon_map_names_to_full_map_names = get_rcon_map_names_to_full_map_names_for_specified_game_name(main_app.get_game_name());
  return rcon_map_names_to_full_map_names.find(rcon_map_name) != cend(rcon_map_names_to_full_map_names) ? rcon_map_names_to_full_map_names.at(rcon_map_name) : rcon_map_name;
}

void display_all_available_maps()
{
  ostringstream oss;
  size_t index{ 1 };
  const auto &rcon_map_names_to_full_map_names = get_rcon_map_names_to_full_map_names_for_specified_game_name(main_app.get_game_name());
  for (const auto &[rcon_map_name, full_map_name] : rcon_map_names_to_full_map_names) {
    oss << "^2" << rcon_map_name << " ^5-> " << full_map_name << '\n';
    ++index;
  }
  print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(), true, true, true);
}

void import_geoip_data(vector<geoip_data> &geo_data, const char *file_path)
{

  log_message(string{ "Importing geological binary data from specified file ("s + file_path + ")."s }, true);
  geo_data.clear();

  ifstream is{ file_path, std::ios::binary };

  if (is) {
    const size_t file_size{ get_file_size_in_bytes(file_path) };
    geo_data.resize(file_size / sizeof(geoip_data));
    is.read(reinterpret_cast<char *>(geo_data.data()), file_size);
  }
}

void export_geoip_data(const vector<geoip_data> &geo_data, const char *file_path) noexcept
{
  log_message(string{ "Exporting geological binary data to specified file ("s + file_path + ")."s }, true);
  ofstream os{ file_path, std::ios::binary };

  if (os) {

    os.write(reinterpret_cast<const char *>(geo_data.data()), static_cast<std::streamsize>(geo_data.size()) * static_cast<std::streamsize>(sizeof(geoip_data)));
  }
  os << flush;
}

void change_colors()
{
  static const std::regex color_code_regex{ R"(\^?\d{1})" };
  const string &fmc = main_app.get_game_server().get_full_map_name_color();
  const string &rmc = main_app.get_game_server().get_rcon_map_name_color();
  const string &fgc = main_app.get_game_server().get_full_gametype_name_color();
  const string &rgc = main_app.get_game_server().get_rcon_gametype_name_color();
  const string onp = main_app.get_game_server().get_online_players_count_color();
  const string ofp = main_app.get_game_server().get_offline_players_count_color();
  const string &bc = main_app.get_game_server().get_border_line_color();
  const string &ph = main_app.get_game_server().get_header_player_pid_color();
  const string &pd = main_app.get_game_server().get_data_player_pid_color();

  const string &sh = main_app.get_game_server().get_header_player_score_color();
  const string &sd = main_app.get_game_server().get_data_player_score_color();

  const string &pgh = main_app.get_game_server().get_header_player_ping_color();
  const string &pgd = main_app.get_game_server().get_data_player_ping_color();

  const string &pnh = main_app.get_game_server().get_header_player_name_color();

  const string &iph = main_app.get_game_server().get_header_player_ip_color();
  const string &ipd = main_app.get_game_server().get_data_player_ip_color();

  const string &gh = main_app.get_game_server().get_header_player_geoinfo_color();
  const string &gd = main_app.get_game_server().get_data_player_geoinfo_color();

  print_colored_text(app_handles.hwnd_re_messages_data, "^5\nType a number ^1(color code: 0-9) ^5for each color setting\n or press ^3(Enter) ^5to accept the default value for it.\n", true, false);

  string color_code;

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ fmc + "Full map name color (0-9), press enter to accept default (2): " }.c_str(), true, false);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^2";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_full_map_name_color(color_code);

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ rmc + "Rcon map name color (0-9), press enter to accept default (1): " }.c_str(), true, false);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^1";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_rcon_map_name_color(color_code);

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ fgc + "Full gametype name color (0-9), press enter to accept default (2): " }.c_str(), true, false);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^2";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_full_gametype_color(color_code);

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ rgc + "Rcon gametype name color (0-9), press enter to accept default (1): " }.c_str(), true, false);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^1";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_rcon_gametype_color(color_code);

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ onp + "Online players count color (0-9), press enter to accept default (2): " }.c_str(), true, false);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));
  if (color_code.empty())
    color_code = "^2";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_online_players_count_color(color_code);

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ ofp + "Offline players count color (0-9), press enter to accept default (1): " }.c_str(), true, false);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^1";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_offline_players_count_color(color_code);

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ bc + "Border line color (0-9), press enter to accept default (5): " }.c_str(), true, false);
    getline(cin, color_code);
  } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

  if (color_code.empty())
    color_code = "^5";
  else if (color_code[0] != '^')
    color_code.insert(0, 1, '^');
  main_app.get_game_server().set_border_line_color(color_code);

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, "^3Use different alternating background colors for even and odd lines? (yes|no, enter for default (yes): ", true, false);
    getline(cin, color_code);
    stl::helper::to_lower_case_in_place(color_code);
  } while (!color_code.empty() && color_code != "yes" && color_code != "no");

  main_app.get_game_server().set_is_use_different_background_colors_for_even_and_odd_lines(color_code.empty() || color_code == "yes");

  if (main_app.get_game_server().get_is_use_different_background_colors_for_even_and_odd_lines()) {
    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ "^5Background color of odd player data lines (0-9), press enter to accept default (0 - black): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^0";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_odd_player_data_lines_bg_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ "^3Background color of even player data lines (0-9), press enter to accept default (8 - gray): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^8";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_even_player_data_lines_bg_color(color_code);
  }

  do {
    print_colored_text(app_handles.hwnd_re_messages_data, string{ "^5Use different alternating foreground colors for even and odd lines? (yes|no, enter for default (yes): " }.c_str(), true, false);
    getline(cin, color_code);
    stl::helper::to_lower_case_in_place(color_code);
  } while (!color_code.empty() && color_code != "yes" && color_code != "no");

  main_app.get_game_server().set_is_use_different_foreground_colors_for_even_and_odd_lines(color_code.empty() || color_code == "yes");

  if (main_app.get_game_server().get_is_use_different_foreground_colors_for_even_and_odd_lines()) {
    do {
      print_colored_text(app_handles.hwnd_re_messages_data, "Foreground color of odd player data lines (0-9), press enter to accept default (5): ", true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_odd_player_data_lines_fg_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, "Foreground color of even player data lines (0-9), press enter to accept default (3): ", true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^3";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_even_player_data_lines_fg_color(color_code);

  } else {

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ ph + "Player pid header color (0-9), press enter to accept default (1): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^1";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_pid_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ pd + "Player pid data color (0-9), press enter to accept default (1): " }.c_str(), true, false);

      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^1";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_pid_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ sh + "Player score header color (0-9), press enter to accept default (5): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_score_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ sd + "Player score data color (0-9), press enter to accept default (5): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_score_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ pgh + "Player ping header color (0-9), press enter to accept default (3): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^3";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_ping_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ pgd + "Player ping data color (0-9), press enter to accept default (3): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^3";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_ping_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ pnh + "Player name header color (0-9), press Enter to accept default (5): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_name_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ iph + "Player IP header color (0-9), press Enter to accept default (5): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_ip_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ ipd + "Player IP data color (0-9), press Enter to accept default (5): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^5";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_ip_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ gh + "Player country/city header color (0-9), press Enter to accept default (2): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^2";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_header_player_geoinfo_color(color_code);

    do {
      print_colored_text(app_handles.hwnd_re_messages_data, string{ gd + "Player country/city data color (0-9), press Enter to accept default (2): " }.c_str(), true, false);
      getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
      color_code = "^2";
    else if (color_code[0] != '^')
      color_code.insert(0, 1, '^');
    main_app.get_game_server().set_data_player_geoinfo_color(color_code);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", true, false);
  }

  write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
}

void strip_leading_and_trailing_quotes(std::string &data) noexcept
{
  if (data.length() >= 2U && data.front() == '"' && data.back() == '"') {
    data.erase(cbegin(data));
    data.pop_back();
  }
}

void replace_all_escaped_new_lines_with_new_lines(std::string &data) noexcept
{
  while (true) {
    const size_t start{ data.find("\\n") };
    if (string::npos == start) break;
    data.replace(start, 2, "\n");
  }
}

bool change_server_setting(const std::vector<std::string> &command) noexcept
{

  if (command.size() <= 2) {
    const string correct_cmd_usage{
      R"(
 ^1!config rcon ^5new_rcon_password
 ^1!config private ^5new_private_password
 ^1!config address ^5new_server_address ^3(valid ip:port: ^1123.101.102.53:28960^3)
 ^1!config name ^5new_user_name (for example: ^1config name Admin)
)"
    };

    print_colored_text(app_handles.hwnd_re_messages_data, correct_cmd_usage.c_str(), true, true, true);
    return false;
  }

  if (command[0] == "!config") {

    if (command[1] == "rcon") {
      if (command[2].empty() || main_app.get_game_server().get_rcon_password() == command[2]) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3The provided ^1rcon password ^3is too short or it is the same as\n the currently used ^1rcon password^3!\n", true, true, true);
        return false;
      }
      print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully changed the ^1rcon password ^2to \"^5"s + command[2] + "^2\"\n"s }.c_str(), true, true, true);
      main_app.get_game_server().set_rcon_password(command[2]);
      initiate_sending_rcon_status_command_now();

    } else if (command[1] == "private") {
      if (command[2].empty() || main_app.get_game_server().get_private_slot_password() == command[2]) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3The provided ^1private password ^3is too short or it is the same\n as the currently used ^1private password^3!\n", true, true, true);
        return false;
      }
      print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully changed the ^1private password ^2to \"^5"s + command[2] + "^2\"\n"s }.c_str(), true, true, true);
      main_app.get_game_server().set_private_slot_password(command[2]);

    } else if (command[1] == "address") {
      const size_t colon_pos{ command[2].find(':') };
      if (colon_pos == string::npos)
        return false;
      const string ip{ command[2].substr(0, colon_pos) };
      const uint_least16_t port{ static_cast<uint_least16_t>(stoul(command[2].substr(colon_pos + 1))) };
      unsigned long guid{};
      if (!check_ip_address_validity(command[2], guid) || (ip == main_app.get_game_server().get_server_ip_address() && port == main_app.get_game_server().get_server_port()))
        return false;
      print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully changed the ^1game server address ^2to ^5"s + command[2] + "^2\n"s }.c_str(), true, true, true);
      main_app.get_game_server().set_server_ip_address(ip);
      main_app.get_game_server().set_server_port(port);
      initiate_sending_rcon_status_command_now();
    } else if (command[1] == "name") {
      if (command[2].length() < 3U) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3The provided ^1user name ^3is too short.\n It has to be at least ^13 characters ^3long!\n", true, true, true);
        return false;
      }
      print_colored_text(app_handles.hwnd_re_messages_data, string{ "^2You have successfully changed your ^1user name ^2to ^5"s + command[2] + "^2\n"s }.c_str(), true, true, true);
      main_app.set_username(command[2]);
    } else {
      const string correct_cmd_usage{
        R"(
 ^1!config rcon ^5new_rcon_password
 ^1!config private ^5new_private_password
 ^1!config address ^5new_server_address ^3(valid ip:port: ^1123.101.102.53:28960^3)
 ^1!config name ^5new_user_name (for example: ^1config name Admin)
)"
      };

      print_colored_text(app_handles.hwnd_re_messages_data, correct_cmd_usage.c_str(), true, true, true);
      return false;
    }
  }

  write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
  return true;
}

void log_message(const string &msg, const bool is_log_current_date_time)
{
  ostringstream os;
  if (is_log_current_date_time) {
    const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();
    const time_t t_c = std::chrono::system_clock::to_time_t(now);
    tm time_info{};
    localtime_s(&time_info, &t_c);
    os << "[" << put_time(&time_info, "%Y-%b-%d %T") << "] ";
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

std::string get_player_name_for_pid(const int pid)
{
  const auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    if (pid == players_data[i].pid)
      return players_data[i].player_name;
  }

  return { "Unknown player" };
}

player_data &get_player_data_for_pid(const int pid)
{
  static player_data pd{};
  auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    if (pid == players_data[i].pid)
      return players_data[i];
  }

  return pd;
}

std::string get_player_information(const int pid)
{
  static char buffer[512];
  const auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    if (pid == players_data[i].pid) {
      const auto &p = players_data[i];
      snprintf(buffer, 512, "^3player name: ^7%s ^3| ^1pid: %d ^3| score: ^5%d ^3| ping: ^5%s\n ^3guid: ^5%s ^3|IP: ^5%s ^3| geoinfo: ^5%s, %s, %s", p.player_name, p.pid, p.score, p.ping, p.guid_key, p.ip_address, p.country_name, p.region, p.city);
      return buffer;
    }
  }

  snprintf(buffer, 512, "(^3player name: ^5n/a ^3| ^1pid: %d ^3| score: ^5n/a ^3| ping: ^5n/a\n ^3guid: ^5n/a ^3| IP: ^5n/a ^3| Country/Region/City: ^5n/a)", pid);
  return buffer;
}

std::string get_player_information_for_player(player_data &p)
{
  static char buffer[512];
  if (strlen(p.country_name) == 0 || strcmp(p.country_name, "n/a") == 0) {
    convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), p.ip_address, p);
  }

  snprintf(buffer, 512, "^3player name: ^7%s ^3| ^1pid: %d ^3| score: ^5%d ^3| ping: ^5%s\n ^3guid: ^5%s ^3|IP: ^5%s ^3| geoinfo: ^5%s, %s, %s", p.player_name, p.pid, p.score, p.ping, p.guid_key, p.ip_address, p.country_name, p.region, p.city);
  return buffer;
}

bool specify_reason_for_player_pid(const int pid, const std::string &reason)
{
  auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    if (pid == players_data[i].pid) {
      strcpy_s(players_data[i].reason, std::size(players_data[i].reason), reason.c_str());
      return true;
    }
  }

  return false;
}

void build_tiny_rcon_message(std::string &msg)
{

  for (const auto &[key, value] : main_app.get_tinyrcon_dict()) {

    for (size_t pos{}; (pos = msg.find(key, pos)) != string::npos; pos += value.length()) {

      msg.replace(pos, key.length(), value);
    }
  }
}

string get_time_interval_info_string_for_seconds(const time_t seconds)
{
  constexpr time_t seconds_in_day{ 24 * 3600 };
  const time_t days = seconds / seconds_in_day;
  const time_t hours = (seconds - days * seconds_in_day) / 3600;
  const time_t minutes = (seconds - (days * seconds_in_day + hours * 3600)) / 60;
  ostringstream oss;
  if (days != 0) {
    oss << days << (days != 1 ? " days : " : " day : ");
  }

  if (hours != 0) {
    oss << hours << (hours != 1 ? " hours : " : " hour : ");
  }

  if (minutes != 0) {
    oss << minutes << (minutes != 1 ? " minutes" : " minute");
  }

  return oss.str();
}

string get_date_and_time_for_time_t(const time_t t_c)
{

  ostringstream oss;
  tm time_info{};
  localtime_s(&time_info, &t_c);
  oss << put_time(&time_info, "%Y-%b-%d %T");
  return oss.str();
}

void change_game_type(const string &game_type, const bool is_reload_map) noexcept
{
  const auto &full_gametype_names = get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());
  if (full_gametype_names.find(game_type) != cend(full_gametype_names)) {
    char buffer[128]{};
    snprintf(buffer, std::size(buffer), "g_gametype %s", game_type.c_str());
    string reply;
    main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), true);
    main_app.get_game_server().set_current_game_type(game_type);
    if (is_reload_map) {
      this_thread::sleep_for(std::chrono::milliseconds(50));
      main_app.get_connection_manager().send_and_receive_rcon_data("map_restart", reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), true);
      initiate_sending_rcon_status_command_now();
    }
  }
}

void load_map(const string &rcon_map_name, const string &game_type, const bool is_change_game_type) noexcept
{
  if (is_change_game_type) {
    change_game_type(game_type, false);
  }

  char buffer[128]{};
  snprintf(buffer, std::size(buffer), "map %s", rcon_map_name.c_str());
  string reply;
  main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), true);
  main_app.get_game_server().set_current_map(rcon_map_name);
  initiate_sending_rcon_status_command_now();
}

bool remove_dir_path_sep_char(char *dir_path) noexcept
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

bool remove_dir_path_sep_char(wchar_t *dir_path) noexcept
{

  const size_t dir_path_len{ stl::helper::len(dir_path) };
  if (dir_path_len == 0)
    return false;
  size_t index{ dir_path_len - 1 };
  while (index > 0 && (dir_path[index] == L'\\' || dir_path[index] == L'/')) {
    dir_path[index] = L'\0';
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

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  unused(lParam);

  if (uMsg == BFFM_INITIALIZED) {
    csay(app_handles.hwnd_re_messages_data, "^3\n> Currently selected path: %s\n", (const char *)lpData);
    SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    SendMessage(hwnd, BFFM_SETEXPANDED, TRUE, lpData);
    SendMessage(hwnd, BFFM_SETSTATUSTEXT, NULL, lpData);
    ShowWindow(hwnd, SW_NORMAL);
    SetFocus(hwnd);
    SetForegroundWindow(hwnd);
  }

  return 0;
}

const char *BrowseFolder(const char *saved_path, const char *user_info) noexcept
{
  constexpr size_t max_path_length{ 32768 };
  static char path[max_path_length];

  BROWSEINFOA bi{};
  bi.lpszTitle = user_info;
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
  bi.lpfn = BrowseCallbackProc;
  bi.lParam = reinterpret_cast<LPARAM>(saved_path);

  LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

  if (pidl != 0) {
    // get the name of the folder and put it in path
    SHGetPathFromIDListA(pidl, path);

    // free memory used
    IMalloc *imalloc = 0;
    if (SUCCEEDED(SHGetMalloc(&imalloc))) {
      imalloc->Free(pidl);
      imalloc->Release();
    }

    return path;
  }

  path[0] = '\0';
  return path;
}

const char *find_call_of_duty_1_installation_path(const bool is_show_browse_folder_dialog) noexcept
{
  constexpr size_t max_path_length{ 32768 };
  static char install_path[max_path_length]{};
  static char exe_file_path[max_path_length]{};
  static const char *def_cod1_registry_location_subkeys[] = {
    "SOFTWARE\\Activision\\Call of Duty",
    "SOFTWARE\\Wow6432Node\\Activision\\Call of Duty",
    "SOFTWARE\\Activision\\Call of Duty(R)",
    "SOFTWARE\\WOW6432Node\\Activision\\Call of Duty(R)",
    nullptr
  };
  const char *game_install_path_key = "InstallPath";
  DWORD cch{};
  HKEY game_installation_reg_key{};
  bool found{};

  const char *found_reg_location{};
  const char **def_game_reg_key{ def_cod1_registry_location_subkeys };

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_installed{};

  LRESULT status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam\\Apps\\2620", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {
    cch = sizeof(is_steam_game_installed);
    status = RegQueryValueExA(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

    if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
      HKEY steam_installation_key{};
      status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
      if (status == ERROR_SUCCESS) {
        cch = sizeof(install_path);
        status = RegQueryValueExA(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          string steam_exe_path{ install_path };
          replace_forward_slash_with_backward_slash(steam_exe_path);
          if (check_if_file_path_exists(steam_exe_path.c_str())) {

            snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 2620", steam_exe_path.c_str());

            found = true;
            char buffer[1024];
            snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty (Steam) ^2game's launch command: ^5%s\n", exe_file_path);
            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
            *def_game_reg_key = nullptr;
          }
        }

        RegCloseKey(steam_installation_key);
      }
    }

    RegCloseKey(game_installation_reg_key);
  }

  if (!found) {
    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    is_steam_game_installed = 0;
    cch = sizeof(is_steam_game_installed);

    status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\Apps\\2620", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueExA(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

      if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
        HKEY steam_installation_key{};
        status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
        if (status == ERROR_SUCCESS) {
          cch = sizeof(install_path);
          status = RegQueryValueExA(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

          if (status == ERROR_SUCCESS) {

            string steam_exe_path{ install_path };
            replace_forward_slash_with_backward_slash(steam_exe_path);
            if (check_if_file_path_exists(steam_exe_path.c_str())) {

              snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 2620", steam_exe_path.c_str());

              found = true;
              char buffer[1024];
              snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty (Steam) ^2game's launch command: ^5%s\n", exe_file_path);
              print_colored_text(app_handles.hwnd_re_messages_data, buffer);
              *def_game_reg_key = nullptr;
            }
          }

          RegCloseKey(steam_installation_key);
        }
      }

      RegCloseKey(game_installation_reg_key);
    }
  }

  while (!found && *def_game_reg_key) {

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));

    cch = sizeof(install_path);

    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueExA(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

      if (status == ERROR_SUCCESS) {

        WIN32_FIND_DATAA find_data{};

        remove_dir_path_sep_char(install_path);

        snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", install_path);

        HANDLE search_handle = FindFirstFileA(exe_file_path, &find_data);

        if (search_handle != INVALID_HANDLE_VALUE) {

          found = true;
          found_reg_location = *def_game_reg_key;
          char buffer[1024];
          snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty ^2game's launch command: ^5%s\n", exe_file_path);
          print_colored_text(app_handles.hwnd_re_messages_data, buffer);
          *def_game_reg_key = nullptr;
          break;
        }
      }

      RegCloseKey(game_installation_reg_key);
    }

    def_game_reg_key++;
  }

  if (!found) {

    def_game_reg_key = def_cod1_registry_location_subkeys;

    while (!found && *def_game_reg_key) {

      ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
      cch = sizeof(install_path);

      status = RegOpenKeyExA(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

      if (status == ERROR_SUCCESS) {

        status = RegQueryValueExA(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          WIN32_FIND_DATAA find_data{};

          remove_dir_path_sep_char(install_path);

          snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", install_path);

          HANDLE search_handle = FindFirstFileA(exe_file_path, &find_data);

          if (search_handle != INVALID_HANDLE_VALUE) {

            found = true;
            found_reg_location = *def_game_reg_key;
            char buffer[1024];
            snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty ^2game's launch command: ^5%s\n", exe_file_path);
            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
            *def_game_reg_key = nullptr;
            break;
          }
        }

        RegCloseKey(game_installation_reg_key);
      }

      def_game_reg_key++;
    }
  }

  if (!found && is_show_browse_folder_dialog) {

    strcpy_s(install_path, max_path_length, "C:\\");
    found_reg_location = nullptr;

    static char msgbuff[1024];
    snprintf(msgbuff, std::size(msgbuff), "Please, select your Call of Duty game's installation folder and click OK.");

    const char *cod1_game_path = BrowseFolder(install_path, msgbuff);

    if (lstrcmpA(cod1_game_path, "") == 0 || lstrcmpA(cod1_game_path, "C:\\") == 0) {
      print_colored_text(app_handles.hwnd_re_messages_data, "^1\n> Error! You haven't selected a valid folder for your game installation.");
      print_colored_text(app_handles.hwnd_re_messages_data, "^3\n> You have to select your game's installation directory and click the OK button.");
      print_colored_text(app_handles.hwnd_re_messages_data, "^2\n> Restart the program and try again.\n");
    } else {
      snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", cod1_game_path);
    }
  }

  if (!stl::helper::str_contains(exe_file_path, "-applaunch 2620")) {
    string exe_path{ exe_file_path };
    replace_forward_slash_with_backward_slash(exe_path);
    strcpy_s(exe_file_path, max_path_length, exe_path.c_str());
  }

  if (!check_if_file_path_exists(exe_file_path) && !stl::helper::str_contains(exe_file_path, "-applaunch 2620")) {
    main_app.set_codmp_exe_path("");
  } else {
    main_app.set_codmp_exe_path(exe_file_path);
    write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
  }

  return exe_file_path;
}

bool check_if_call_of_duty_1_game_is_running() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[256];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      const auto n = GetModuleBaseNameA(ph, 0, buffer, sizeof(buffer));
      CloseHandle(ph);
      if (n == 0) continue;
      const string module_base_name{ buffer };
      if (stl::helper::str_index_of(module_base_name, "codmp.exe", 0, true) != string::npos)
        return true;
    }

  DWORD cch{};
  HKEY game_installation_reg_key;

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_running{};
  cch = sizeof(is_steam_game_running);

  auto status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueExA(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 2620 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  is_steam_game_running = 0;
  cch = sizeof(is_steam_game_running);

  status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueExA(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 2620 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  return false;
}

const char *find_call_of_duty_2_installation_path(const bool is_show_browse_folder_dialog) noexcept
{
  constexpr size_t max_path_length{ 32768 };
  static char install_path[max_path_length]{};
  static char exe_file_path[max_path_length]{};
  static const char *def_cod2_registry_location_subkeys[] = {
    "SOFTWARE\\Activision\\Call of Duty 2",
    "SOFTWARE\\WOW6432Node\\Activision\\Call of Duty 2",
    "SOFTWARE\\Activision\\Call of Duty(R) 2",
    "SOFTWARE\\WOW6432Node\\Activision\\Call of Duty(R) 2",
    nullptr
  };
  const char *game_install_path_key = "InstallPath";
  DWORD cch{};
  HKEY game_installation_reg_key{};
  bool found{};

  const char *found_reg_location{};
  const char **def_game_reg_key{ def_cod2_registry_location_subkeys };

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_installed{};

  LRESULT status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam\\Apps\\2630", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {
    cch = sizeof(is_steam_game_installed);
    status = RegQueryValueExA(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

    if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
      HKEY steam_installation_key{};
      status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
      if (status == ERROR_SUCCESS) {
        cch = sizeof(install_path);
        status = RegQueryValueExA(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          string steam_exe_path{ install_path };
          replace_forward_slash_with_backward_slash(steam_exe_path);
          if (check_if_file_path_exists(steam_exe_path.c_str())) {

            snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 2630", steam_exe_path.c_str());

            found = true;
            char buffer[1024];
            snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 2 (Steam) ^2game's launch command: ^5%s\n", exe_file_path);
            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
            *def_game_reg_key = nullptr;
          }
        }

        RegCloseKey(steam_installation_key);
      }
    }

    RegCloseKey(game_installation_reg_key);
  }

  if (!found) {
    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    is_steam_game_installed = 0;
    cch = sizeof(is_steam_game_installed);

    status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\Apps\\2630", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueExA(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

      if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
        HKEY steam_installation_key{};
        status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
        if (status == ERROR_SUCCESS) {
          cch = sizeof(install_path);
          status = RegQueryValueExA(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

          if (status == ERROR_SUCCESS) {

            string steam_exe_path{ install_path };
            replace_forward_slash_with_backward_slash(steam_exe_path);
            if (check_if_file_path_exists(steam_exe_path.c_str())) {

              snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 2630", steam_exe_path.c_str());

              found = true;
              char buffer[1024];
              snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 2 (Steam) ^2game's launch command: ^5%s\n", exe_file_path);
              print_colored_text(app_handles.hwnd_re_messages_data, buffer);
              *def_game_reg_key = nullptr;
            }
          }

          RegCloseKey(steam_installation_key);
        }
      }

      RegCloseKey(game_installation_reg_key);
    }
  }

  while (!found && *def_game_reg_key) {

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));

    cch = sizeof(install_path);

    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueExA(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

      if (status == ERROR_SUCCESS) {
        WIN32_FIND_DATAA find_data{};
        remove_dir_path_sep_char(install_path);
        snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", install_path);

        HANDLE search_handle = FindFirstFileA(exe_file_path, &find_data);

        if (search_handle != INVALID_HANDLE_VALUE) {

          found = true;
          found_reg_location = *def_game_reg_key;
          char buffer[1024];
          snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 2 ^2game's launch command: ^5%s\n", exe_file_path);
          print_colored_text(app_handles.hwnd_re_messages_data, buffer);
          *def_game_reg_key = nullptr;
          break;
        }
      }

      RegCloseKey(game_installation_reg_key);
    }

    ++def_game_reg_key;
  }

  if (!found) {

    def_game_reg_key = def_cod2_registry_location_subkeys;

    while (!found && *def_game_reg_key) {

      ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
      cch = sizeof(install_path);

      status = RegOpenKeyExA(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

      if (status == ERROR_SUCCESS) {

        status = RegQueryValueExA(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          WIN32_FIND_DATAA find_data{};
          remove_dir_path_sep_char(install_path);
          snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", install_path);
          HANDLE search_handle = FindFirstFileA(exe_file_path, &find_data);

          if (search_handle != INVALID_HANDLE_VALUE) {

            found = true;
            found_reg_location = *def_game_reg_key;
            char buffer[1024];
            snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 2 ^2game's launch command: ^5%s\n", exe_file_path);
            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
            *def_game_reg_key = nullptr;
            break;
          }
        }

        RegCloseKey(game_installation_reg_key);
      }

      ++def_game_reg_key;
    }
  }

  if (!found && is_show_browse_folder_dialog) {

    strcpy_s(install_path, max_path_length, "C:\\");
    found_reg_location = nullptr;

    static char msgbuff[1024];
    snprintf(msgbuff, std::size(msgbuff), "Please, select your Call of Duty 2 game's installation folder and click OK.");

    const char *cod2_game_path = BrowseFolder(install_path, msgbuff);

    if (lstrcmpA(cod2_game_path, "") == 0 || lstrcmpA(cod2_game_path, "C:\\") == 0) {
      print_colored_text(app_handles.hwnd_re_messages_data, "^1\n> Error! You haven't selected a valid folder for your game installation.");
      print_colored_text(app_handles.hwnd_re_messages_data, "^3\n> You have to select your game's installation directory and click the OK button.");
      print_colored_text(app_handles.hwnd_re_messages_data, "^2\n> Restart the program and try again.\n");
    } else {
      snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", cod2_game_path);
    }
  }

  if (!stl::helper::str_contains(exe_file_path, "-applaunch 2630")) {
    string exe_path{ exe_file_path };
    replace_forward_slash_with_backward_slash(exe_path);
    strcpy_s(exe_file_path, max_path_length, exe_path.c_str());
  }

  if (!check_if_file_path_exists(exe_file_path) && !stl::helper::str_contains(exe_file_path, "-applaunch 2630")) {
    main_app.set_cod2mp_exe_path("");
  } else {
    main_app.set_cod2mp_exe_path(exe_file_path);
    write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
  }

  return exe_file_path;
}

bool check_if_call_of_duty_2_game_is_running() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[256];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      const auto n = GetModuleBaseNameA(ph, 0, buffer, sizeof(buffer));
      CloseHandle(ph);
      if (n == 0) continue;
      const string module_base_name{ buffer };
      if (stl::helper::str_index_of(module_base_name, "cod2mp_s.exe", 0, true) != string::npos)
        return true;
    }

  DWORD cch{};
  HKEY game_installation_reg_key;

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_running{};
  cch = sizeof(is_steam_game_running);

  auto status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueExA(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 2630 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  is_steam_game_running = 0;
  cch = sizeof(is_steam_game_running);

  status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueExA(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 2630 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  return false;
}

const char *find_call_of_duty_4_installation_path(const bool is_show_browse_folder_dialog) noexcept
{
  constexpr size_t max_path_length{ 32768 };
  static char install_path[max_path_length]{};
  static char exe_file_path[max_path_length]{};
  static const char *def_cod4_registry_location_subkeys[] = {
    "SOFTWARE\\Activision\\Call of Duty 4",
    "SOFTWARE\\Wow6432Node\\Activision\\Call of Duty 4",
    "SOFTWARE\\Activision\\Call of Duty(R) 4",
    "SOFTWARE\\WOW6432Node\\Activision\\Call of Duty(R) 4",
    nullptr
  };
  const char *game_install_path_key = "InstallPath";
  DWORD cch{};
  HKEY game_installation_reg_key{};
  bool found{};

  const char *found_reg_location{};
  const char **def_game_reg_key{ def_cod4_registry_location_subkeys };

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_installed{};

  LRESULT status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam\\Apps\\7940", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {
    cch = sizeof(is_steam_game_installed);
    status = RegQueryValueExA(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

    if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
      HKEY steam_installation_key{};
      status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
      if (status == ERROR_SUCCESS) {
        cch = sizeof(install_path);
        status = RegQueryValueExA(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          string steam_exe_path{ install_path };
          replace_forward_slash_with_backward_slash(steam_exe_path);
          if (check_if_file_path_exists(steam_exe_path.c_str())) {

            snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 7940", steam_exe_path.c_str());

            found = true;
            char buffer[1024];
            snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 4: Modern Warfare (Steam) ^2game's launch command: ^5%s\n", exe_file_path);
            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
            *def_game_reg_key = nullptr;
          }
        }

        RegCloseKey(steam_installation_key);
      }
    }

    RegCloseKey(game_installation_reg_key);
  }

  if (!found) {
    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    is_steam_game_installed = 0;
    cch = sizeof(is_steam_game_installed);

    status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\Apps\\7940", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueExA(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

      if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
        HKEY steam_installation_key{};
        status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
        if (status == ERROR_SUCCESS) {
          cch = sizeof(install_path);
          status = RegQueryValueExA(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

          if (status == ERROR_SUCCESS) {

            string steam_exe_path{ install_path };
            replace_forward_slash_with_backward_slash(steam_exe_path);
            if (check_if_file_path_exists(steam_exe_path.c_str())) {

              snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 7940", steam_exe_path.c_str());

              found = true;
              char buffer[1024];
              snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 4: Modern Warfare (Steam) ^2game's launch command: ^5%s\n", exe_file_path);
              print_colored_text(app_handles.hwnd_re_messages_data, buffer);
              *def_game_reg_key = nullptr;
            }
          }

          RegCloseKey(steam_installation_key);
        }
      }

      RegCloseKey(game_installation_reg_key);
    }
  }

  while (!found && *def_game_reg_key) {

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));

    cch = sizeof(install_path);

    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueExA(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

      if (status == ERROR_SUCCESS) {
        WIN32_FIND_DATAA find_data{};
        remove_dir_path_sep_char(install_path);
        snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", install_path);
        HANDLE search_handle = FindFirstFileA(exe_file_path, &find_data);

        if (search_handle != INVALID_HANDLE_VALUE) {

          found = true;
          found_reg_location = *def_game_reg_key;
          char buffer[1024];
          snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 4: Modern Warfare ^2game's launch command: ^5%s\n", exe_file_path);
          print_colored_text(app_handles.hwnd_re_messages_data, buffer);
          *def_game_reg_key = nullptr;
          break;
        }
      }

      RegCloseKey(game_installation_reg_key);
    }

    def_game_reg_key++;
  }

  if (!found) {

    def_game_reg_key = def_cod4_registry_location_subkeys;

    while (!found && *def_game_reg_key) {

      ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
      cch = sizeof(install_path);

      status = RegOpenKeyExA(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

      if (status == ERROR_SUCCESS) {

        status = RegQueryValueExA(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {
          WIN32_FIND_DATAA find_data{};
          remove_dir_path_sep_char(install_path);
          snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", install_path);
          HANDLE search_handle = FindFirstFileA(exe_file_path, &find_data);

          if (search_handle != INVALID_HANDLE_VALUE) {

            found = true;
            found_reg_location = *def_game_reg_key;
            char buffer[1024];
            snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 4: Modern Warfare ^2game's launch command: ^5%s\n", exe_file_path);
            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
            *def_game_reg_key = nullptr;
            break;
          }
        }

        RegCloseKey(game_installation_reg_key);
      }

      def_game_reg_key++;
    }
  }

  if (!found && is_show_browse_folder_dialog) {

    strcpy_s(install_path, max_path_length, "C:\\");
    found_reg_location = nullptr;

    static char msgbuff[1024];
    snprintf(msgbuff, std::size(msgbuff), "Please, select your Call of Duty 4: Modern Warfare game's installation folder and click OK.");

    const char *cod4_game_path = BrowseFolder(install_path, msgbuff);

    if (lstrcmpA(cod4_game_path, "") == 0 || lstrcmpA(cod4_game_path, "C:\\") == 0) {
      print_colored_text(app_handles.hwnd_re_messages_data, "^1\n> Error! You haven't selected a valid folder for your game installation.");
      print_colored_text(app_handles.hwnd_re_messages_data, "^3\n> You have to select your game's installation directory and click the OK button.");
      print_colored_text(app_handles.hwnd_re_messages_data, "^2\n> Restart the program and try again.\n");
    } else {
      snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", cod4_game_path);
    }
  }

  if (!stl::helper::str_contains(exe_file_path, "-applaunch 7940")) {
    string exe_path{ exe_file_path };
    replace_forward_slash_with_backward_slash(exe_path);
    strcpy_s(exe_file_path, max_path_length, exe_path.c_str());
  }

  if (!check_if_file_path_exists(exe_file_path) && !stl::helper::str_contains(exe_file_path, "-applaunch 7940")) {
    main_app.set_iw3mp_exe_path("");
  } else {
    main_app.set_iw3mp_exe_path(exe_file_path);
    write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
  }

  return exe_file_path;
}

bool check_if_call_of_duty_4_game_is_running() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[256];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      const auto n = GetModuleBaseNameA(ph, 0, buffer, sizeof(buffer));
      CloseHandle(ph);
      if (n == 0) continue;
      const string module_base_name{ buffer };
      if (stl::helper::str_index_of(module_base_name, "iw3mp.exe", 0, true) != string::npos)
        return true;
    }

  DWORD cch{};
  HKEY game_installation_reg_key;

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_running{};
  cch = sizeof(is_steam_game_running);

  auto status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueExA(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 7940 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  is_steam_game_running = 0;
  cch = sizeof(is_steam_game_running);

  status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueExA(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 7940 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  return false;
}

const char *find_call_of_duty_5_installation_path(const bool is_show_browse_folder_dialog) noexcept
{
  constexpr size_t max_path_length{ 32768 };
  static char install_path[max_path_length]{};
  static char exe_file_path[max_path_length]{};
  static const char *def_cod5_registry_location_subkeys[] = {
    "SOFTWARE\\Activision\\Call of Duty WAW",
    "SOFTWARE\\Wow6432Node\\Activision\\Call of Duty WAW",
    "SOFTWARE\\Activision\\Call of Duty(R) WAW",
    "SOFTWARE\\WOW6432Node\\Activision\\Call of Duty(R) WAW",
    nullptr
  };
  const char *game_install_path_key = "InstallPath";
  DWORD cch{};
  HKEY game_installation_reg_key{};
  bool found{};

  const char *found_reg_location{};
  const char **def_game_reg_key{ def_cod5_registry_location_subkeys };

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_installed{};

  LRESULT status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam\\Apps\\10090", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {
    cch = sizeof(is_steam_game_installed);
    status = RegQueryValueExA(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

    if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
      HKEY steam_installation_key{};
      status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
      if (status == ERROR_SUCCESS) {
        cch = sizeof(install_path);
        status = RegQueryValueExA(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          string steam_exe_path{ install_path };
          replace_forward_slash_with_backward_slash(steam_exe_path);
          if (check_if_file_path_exists(steam_exe_path.c_str())) {

            snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 10090", steam_exe_path.c_str());

            found = true;
            char buffer[1024];
            snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 5: World at War (Steam) ^2game's launch command: ^5%s\n", exe_file_path);
            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
            *def_game_reg_key = nullptr;
          }
        }

        RegCloseKey(steam_installation_key);
      }
    }

    RegCloseKey(game_installation_reg_key);
  }

  if (!found) {
    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    is_steam_game_installed = 0;
    cch = sizeof(is_steam_game_installed);

    status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam\\Apps\\10090", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueExA(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

      if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
        HKEY steam_installation_key{};
        status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
        if (status == ERROR_SUCCESS) {
          cch = sizeof(install_path);
          status = RegQueryValueExA(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

          if (status == ERROR_SUCCESS) {

            string steam_exe_path{ install_path };
            replace_forward_slash_with_backward_slash(steam_exe_path);
            if (check_if_file_path_exists(steam_exe_path.c_str())) {

              snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 10090", steam_exe_path.c_str());

              found = true;
              char buffer[1024];
              snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 5: World at War (Steam) ^2game's launch command: ^5%s\n", exe_file_path);
              print_colored_text(app_handles.hwnd_re_messages_data, buffer);
              *def_game_reg_key = nullptr;
            }
          }

          RegCloseKey(steam_installation_key);
        }
      }

      RegCloseKey(game_installation_reg_key);
    }
  }

  while (!found && *def_game_reg_key) {

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));

    cch = sizeof(install_path);

    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueExA(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

      if (status == ERROR_SUCCESS) {
        WIN32_FIND_DATAA find_data{};
        remove_dir_path_sep_char(install_path);
        snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", install_path);
        HANDLE search_handle = FindFirstFileA(exe_file_path, &find_data);

        if (search_handle != INVALID_HANDLE_VALUE) {

          found = true;
          found_reg_location = *def_game_reg_key;
          char buffer[1024];
          snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 5: World at War ^2game's launch command: ^5%s\n", exe_file_path);
          print_colored_text(app_handles.hwnd_re_messages_data, buffer);
          *def_game_reg_key = nullptr;
          break;
        }
      }

      RegCloseKey(game_installation_reg_key);
    }

    def_game_reg_key++;
  }

  if (!found) {

    def_game_reg_key = def_cod5_registry_location_subkeys;

    while (!found && *def_game_reg_key) {

      ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
      cch = sizeof(install_path);

      status = RegOpenKeyExA(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

      if (status == ERROR_SUCCESS) {

        status = RegQueryValueExA(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          WIN32_FIND_DATAA find_data{};
          remove_dir_path_sep_char(install_path);
          snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", install_path);
          HANDLE search_handle = FindFirstFileA(exe_file_path, &find_data);

          if (search_handle != INVALID_HANDLE_VALUE) {

            found = true;
            found_reg_location = *def_game_reg_key;
            char buffer[1024];
            snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 5: World at War ^2game's launch command: ^5%s\n", exe_file_path);
            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
            *def_game_reg_key = nullptr;
            break;
          }
        }

        RegCloseKey(game_installation_reg_key);
      }

      def_game_reg_key++;
    }
  }

  if (!found && is_show_browse_folder_dialog) {

    strcpy_s(install_path, max_path_length, "C:\\");
    found_reg_location = nullptr;

    static char msgbuff[1024];
    snprintf(msgbuff, std::size(msgbuff), "Please, select your Call of Duty 5: World at War game's installation folder and click OK.");

    const char *cod5_game_path = BrowseFolder(install_path, msgbuff);

    if (lstrcmpA(cod5_game_path, "") == 0 || lstrcmpA(cod5_game_path, "C:\\") == 0) {
      print_colored_text(app_handles.hwnd_re_messages_data, "^1\n> Error! You haven't selected a valid folder for your game installation.");
      print_colored_text(app_handles.hwnd_re_messages_data, "^3\n> You have to select your game's installation directory and click the OK button.");
      print_colored_text(app_handles.hwnd_re_messages_data, "^2\n> Restart the program and try again.\n");
    } else {
      snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", cod5_game_path);
    }
  }

  if (!stl::helper::str_contains(exe_file_path, "-applaunch 10090")) {
    string exe_path{ exe_file_path };
    replace_forward_slash_with_backward_slash(exe_path);
    strcpy_s(exe_file_path, max_path_length, exe_path.c_str());
  }

  if (!check_if_file_path_exists(exe_file_path) && !stl::helper::str_contains(exe_file_path, "-applaunch 10090")) {
    main_app.set_cod5mp_exe_path("");
  } else {
    main_app.set_cod5mp_exe_path(exe_file_path);
    write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
  }

  return exe_file_path;
}

bool check_if_call_of_duty_5_game_is_running() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[256];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      const auto n = GetModuleBaseNameA(ph, 0, buffer, sizeof(buffer));
      CloseHandle(ph);
      if (n == 0) continue;
      const string module_base_name{ buffer };
      if (stl::helper::str_index_of(module_base_name, "cod5mp.exe", 0, true) != string::npos)
        return true;
    }

  DWORD cch{};
  HKEY game_installation_reg_key;

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_running{};
  cch = sizeof(is_steam_game_running);

  auto status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueExA(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 10090 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  is_steam_game_running = 0;
  cch = sizeof(is_steam_game_running);

  status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueExA(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 10090 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  return false;
}

bool connect_to_the_game_server(const std::string &server_ip_port_address, const game_name_t game_name, const bool use_private_slot, const bool minimize_tinyrcon)
{
  constexpr size_t max_path_length{ 32768 };
  static char command_line[max_path_length]{};

  const char *game_path{};

  switch (game_name) {
  case game_name_t::cod1:
    if (check_if_call_of_duty_1_game_is_running()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "Call of Duty Multiplayer game is already running in the background.\n", true, true, true);
      return false;
    }

    game_path = main_app.get_codmp_exe_path().c_str();
    if (!check_if_file_path_exists(game_path) && !stl::helper::str_contains(game_path, "-applaunch 2620")) {
      game_path = find_call_of_duty_1_installation_path();
    }
    break;

  case game_name_t::cod2:
    if (check_if_call_of_duty_2_game_is_running()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "Call of Duty 2 Multiplayer game is already running in the background.\n", true, true, true);
      return false;
    }

    game_path = main_app.get_cod2mp_exe_path().c_str();
    if (!check_if_file_path_exists(game_path) && !stl::helper::str_contains(game_path, "-applaunch 2630")) {
      game_path = find_call_of_duty_2_installation_path();
    }
    break;

  case game_name_t::cod4:
    if (check_if_call_of_duty_4_game_is_running()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "Call of Duty 4: Modern Warfare Multiplayer game is already running in the background.\n", true, true, true);
      return false;
    }

    game_path = main_app.get_iw3mp_exe_path().c_str();
    if (!check_if_file_path_exists(game_path) && !stl::helper::str_contains(game_path, "-applaunch 7940")) {
      game_path = find_call_of_duty_4_installation_path();
    }
    break;

  case game_name_t::cod5:
    if (check_if_call_of_duty_5_game_is_running()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "Call of Duty 5: World at War Multiplayer game is already running in the background.\n", true, true, true);
      return false;
    }

    game_path = main_app.get_cod5mp_exe_path().c_str();
    if (!check_if_file_path_exists(game_path) && !stl::helper::str_contains(game_path, "-applaunch 10090")) {
      game_path = find_call_of_duty_5_installation_path();
    }
    break;

  default:
    print_colored_text(app_handles.hwnd_re_messages_data, "^1Invalid ^1'game_name_t' enum value has been specified!\n", true, true, true);
    return false;
  }


  assert(game_path != nullptr && stl::helper::len(game_path) > 0U);

  const bool is_game_installed_via_steam{
    stl::helper::str_contains(game_path, "-applaunch 2620") || stl::helper::str_contains(game_path, "-applaunch 2630") || stl::helper::str_contains(game_path, "-applaunch 7940") || stl::helper::str_contains(game_path, "-applaunch 10090")
  };

  if (game_path == nullptr || strlen(game_path) == 0 || (!stl::helper::str_ends_with(game_path, ".exe", true) && !is_game_installed_via_steam))
    return false;

  if (minimize_tinyrcon) {
    ShowWindow(GetConsoleWindow(), SW_MINIMIZE);
  }

  if (is_game_installed_via_steam) {
    if (use_private_slot) {
      snprintf(command_line, max_path_length, "%s +password %s +connect %s", game_path, main_app.get_game_server().get_private_slot_password().c_str(), server_ip_port_address.c_str());
    } else {
      snprintf(command_line, max_path_length, "%s +connect %s", game_path, server_ip_port_address.c_str());
    }
  } else {
    if (use_private_slot) {
      snprintf(command_line, max_path_length, "\"%s\" +password %s +connect %s", game_path, main_app.get_game_server().get_private_slot_password().c_str(), server_ip_port_address.c_str());
    } else {
      snprintf(command_line, max_path_length, "\"%s\" +connect %s", game_path, server_ip_port_address.c_str());
    }
  }

  if (!is_game_installed_via_steam) {
    string game_folder{ game_path };
    game_folder.erase(cbegin(game_folder) + game_folder.rfind('\\'), cend(game_folder));
    if (!game_folder.empty() && '\\' == game_folder.back())
      game_folder.pop_back();

    ostringstream oss;
    oss << "^2Launching ^1" << main_app.get_game_title() << " ^2and connecting to ^1" << main_app.get_game_server_name() << " ^2game server...\n";

    const string message{ oss.str() };

    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);

    STARTUPINFOA process_startup_info{};
    process_startup_info.cb = sizeof(STARTUPINFOA);
    process_startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK;

    if (!CreateProcess(
          nullptr,
          command_line,
          NULL,// Process handle not inheritable
          NULL,// Thread handle not inheritable
          FALSE,// Set handle inheritance to FALSE
          HIGH_PRIORITY_CLASS | DETACHED_PROCESS,// creation flags
          NULL,// Use parent's environment block
          game_folder.c_str(),// Use parent's starting directory
          &process_startup_info,// Pointer to STARTUPINFO structure
          &pr_info)) {
      char buffer[512]{};
      strerror_s(buffer, 512, GetLastError());
      const string error_message{ "^1Launching game failed: "s + buffer + "\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str(), true, true, true);
      return false;
    }
  } else {

    ostringstream oss;
    oss << "^2Launching ^1" << main_app.get_game_title() << " ^2via Steam and connecting to ^1" << main_app.get_game_server_name() << " ^2game server...\n";
    const string message{ oss.str() };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);

    STARTUPINFOA process_startup_info{};
    process_startup_info.cb = sizeof(STARTUPINFOA);
    process_startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK;

    if (!CreateProcess(
          nullptr,
          command_line,
          NULL,// Process handle not inheritable
          NULL,// Thread handle not inheritable
          FALSE,// Set handle inheritance to FALSE
          ABOVE_NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,// creation flags
          NULL,// Use parent's environment block
          nullptr,// Use parent's starting directory
          &process_startup_info,// Pointer to STARTUPINFO structure
          &pr_info)) {
      char buffer[512]{};
      strerror_s(buffer, std::size(buffer), GetLastError());
      const string error_message{ "^1Launching game via Steam failed: "s + buffer + "\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str(), true, true, true);
      return false;
    }
  }

  initiate_sending_rcon_status_command_now();
  return true;
}

bool check_if_file_path_exists(const char *file_path) noexcept
{
  ifstream input{ file_path, ios::binary };
  return !input.fail();
}


bool delete_temporary_game_file() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[256];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      if (NULL == ph) continue;
      const auto n = GetModuleBaseNameA(ph, 0, buffer, sizeof(buffer));

      if (n != 0) {
        string module_base_name{ buffer };
        if (module_base_name.find("Call of Duty 2") != string::npos || module_base_name.find("Call of Duty(R) 2") != string::npos || stl::helper::str_index_of(module_base_name, "cod2mp_s.exe", true) != string::npos) {
          GetModuleFileNameExA(ph, NULL, buffer, (DWORD)std::size(buffer));
          module_base_name.assign(buffer);
          const auto start = stl::helper::str_last_index_of(module_base_name, "cod2mp_s.exe", string::npos, true);
          if (start != string::npos) {
            module_base_name.replace(start, stl::helper::len("cod2mp_s.exe"), "__CoD2MP_s");
            DeleteFileA(module_base_name.c_str());
          }
          return true;
        }
      }

      CloseHandle(ph);
    }

  return false;
}

size_t print_colored_text(HWND re_control, const char *text, const bool print_to_richedit_control, const bool log_to_file, const bool is_log_current_date_time)
{
  const char *message{ text };
  const size_t text_len{ stl::helper::len(text) };
  if (text == nullptr || text_len == 0)
    return 0;
  const char *last = text + text_len;
  size_t printed_chars_count{};
  const bool is_last_char_new_line{ text != nullptr && text_len > 0 && text[text_len - 1] == '\n' };

  if (print_to_richedit_control) {

    string msg;
    COLORREF bg_color{ color::black };
    COLORREF fg_color{ color::white };
    set_rich_edit_control_colors(re_control, fg_color, bg_color);

    for (; *text; ++text) {
      if (text + black_bg_color_length <= last && strncmp(text, black_bg_color, black_bg_color_length) == 0) {
        if (!msg.empty()) {
          append(re_control, msg.c_str());
          msg.clear();
        }
        bg_color = color::black;
        set_rich_edit_control_colors(re_control, fg_color, bg_color);
        text += (black_bg_color_length - 1);
      } else if (text + grey_bg_color_length <= last && strncmp(text, grey_bg_color, grey_bg_color_length) == 0) {
        if (!msg.empty()) {
          append(re_control, msg.c_str());
          msg.clear();
        }
        bg_color = color::grey;
        set_rich_edit_control_colors(re_control, fg_color, bg_color);
        text += (grey_bg_color_length - 1);
      } else if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') && (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3)) {
        text += 3;
        if (!msg.empty()) {
          append(re_control, msg.c_str());
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
          append(re_control, msg.c_str());
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
      append(re_control, msg.c_str());
      msg.clear();
    }

    if (!is_last_char_new_line) {
      append(re_control, "\n");
      ++printed_chars_count;
    }
  }

  // if (re_control == app_handles.hwnd_re_messages_data)
  //   g_message_data_contents.append(message);


  if (log_to_file) {
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
  const size_t text_len = strlen(text);
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


bool get_confirmation_message_from_user(const char *message, const char *caption) noexcept
{
  const auto answer = MessageBox(app_handles.hwnd_main_window, message, caption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);
  return answer == IDYES;
}

bool check_if_user_wants_to_quit(const char *msg) noexcept
{

  return strcmp(msg, "q") == 0 || strcmp(msg, "!q") == 0 || strcmp(msg, "exit") == 0 || strcmp(msg, "quit") == 0 || strcmp(msg, "!exit") == 0 || strcmp(msg, "!quit") == 0;
}

void set_rich_edit_control_colors(HWND richEditCtrl, const COLORREF fg_color, const COLORREF bg_color, const char *font_face_name) noexcept
{
  CHARFORMAT2 cf{};
  cf.cbSize = sizeof(CHARFORMAT2);
  cf.dwMask = CFM_CHARSET | CFM_FACE | CFM_COLOR | CFM_BACKCOLOR | CFM_WEIGHT;
  strcpy_s(cf.szFaceName, font_face_name);
  cf.wWeight = 800;
  cf.bCharSet = RUSSIAN_CHARSET;
  cf.crTextColor = fg_color;
  cf.crBackColor = bg_color;
  SendMessage(richEditCtrl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

CHARFORMAT get_char_fmt(HWND hwnd, DWORD range) noexcept
{
  CHARFORMAT cf;
  SendMessage(hwnd, EM_GETCHARFORMAT, range, (LPARAM)&cf);
  return cf;
}
void set_char_fmt(HWND hwnd, const CHARFORMAT2 &cf, DWORD range) noexcept
{
  SendMessage(hwnd, EM_SETCHARFORMAT, range, (LPARAM)&cf);
}
void replace_sel(HWND hwnd, const char *str) noexcept
{
  SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)str);
}
void cursor_to_bottom(HWND hwnd) noexcept
{
  SendMessage(hwnd, EM_SETSEL, (WPARAM)-2, -1);
}
void scroll_to_beginning(HWND hwnd) noexcept
{
  SendMessage(hwnd, WM_VSCROLL, 0, 0);
  SendMessage(hwnd, WM_HSCROLL, SB_LEFT, 0);
}
void scroll_to(HWND hwnd, DWORD pos) noexcept
{
  SendMessage(hwnd, WM_VSCROLL, pos, 0);
}
void scroll_to_bottom(HWND hwnd) noexcept
{
  scroll_to(hwnd, SB_BOTTOM);
}


void append(HWND hwnd, const char *str) noexcept
{
  lock_guard lg{ print_data_mutex };
  cursor_to_bottom(hwnd);
  replace_sel(hwnd, str);
  scroll_to_bottom(hwnd);
}

void process_key_down_message(const MSG &msg)
{
  if (msg.wParam == VK_ESCAPE) {
    if (show_user_confirmation_dialog("^3Do you really want to quit?", "Exit program?", "Reason")) {
      is_terminate_program.store(true);
      {
        lock_guard<mutex> ul{ mu };
        exit_flag.notify_one();
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
        lock_guard<mutex> ul{ mu };
        exit_flag.notify_one();
      }
      PostQuitMessage(0);
    }

  } else if (msg.wParam == VK_F1) {
    is_process_combobox_item_selection_event = false;
    type_of_sort = sort_by_pid_asc ? sort_type::pid_asc : sort_type::pid_desc;
    sort_by_pid_asc = !sort_by_pid_asc;
    process_sort_type_change_request(type_of_sort);
  } else if (msg.wParam == VK_F2) {
    is_process_combobox_item_selection_event = false;
    type_of_sort = sort_by_score_asc ? sort_type::score_asc : sort_type::score_desc;
    sort_by_score_asc = !sort_by_score_asc;
    process_sort_type_change_request(type_of_sort);
  } else if (msg.wParam == VK_F3) {
    is_process_combobox_item_selection_event = false;
    type_of_sort = sort_by_ping_asc ? sort_type::ping_asc : sort_type::ping_desc;
    sort_by_ping_asc = !sort_by_ping_asc;
    process_sort_type_change_request(type_of_sort);
  } else if (msg.wParam == VK_F4) {
    is_process_combobox_item_selection_event = false;
    type_of_sort = sort_by_name_asc ? sort_type::name_asc : sort_type::name_desc;
    sort_by_name_asc = !sort_by_name_asc;
    process_sort_type_change_request(type_of_sort);
  } else if (msg.wParam == VK_F5) {
    is_process_combobox_item_selection_event = false;
    type_of_sort = sort_by_ip_asc ? sort_type::ip_asc : sort_type::ip_desc;
    sort_by_ip_asc = !sort_by_ip_asc;
    process_sort_type_change_request(type_of_sort);
  } else if (msg.wParam == VK_F6) {
    is_process_combobox_item_selection_event = false;
    type_of_sort = sort_by_geo_asc ? sort_type::geo_asc : sort_type::geo_desc;
    sort_by_geo_asc = !sort_by_geo_asc;
    process_sort_type_change_request(type_of_sort);
  } else if (msg.wParam == VK_F8) {
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Manually refreshing players data.\n");
    initiate_sending_rcon_status_command_now();
  }
}

void display_tempbanned_players_remaining_time_period()
{
  auto &tembanned_players = main_app.get_game_server().get_temp_banned_players_data();
  if (tembanned_players.empty())
    return;
  std::sort(std::begin(tembanned_players), std::end(tembanned_players), [](const player_data &pl1, const player_data &pl2) {
    return pl1.banned_start_time + (pl1.ban_duration_in_hours * 3600) < pl2.banned_start_time + (pl2.ban_duration_in_hours * 3600);
  });

  const time_t now_in_seconds = std::time(nullptr);

  for (const auto &pl : tembanned_players) {
    const time_t ban_expires_time = pl.banned_start_time + (pl.ban_duration_in_hours * 3600);
    if (ban_expires_time > now_in_seconds) {
      ostringstream oss;
      oss << "^3Player's (^7" << pl.player_name << "^3) tempban (banned on ^1" << get_date_and_time_for_time_t(pl.banned_start_time)
          << " ^3with reason: ^1" << pl.reason << "^3)\n will automatically be removed in ^1" << get_time_interval_info_string_for_seconds(ban_expires_time - now_in_seconds) << '\n';
      const string message{ oss.str() };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), true, true, true);
    } else {
      main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = pl.player_name;
      main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] = get_date_and_time_for_time_t(pl.banned_start_time);
      main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] = get_date_and_time_for_time_t(ban_expires_time);
      main_app.get_tinyrcon_dict()["{REASON}"] = pl.reason;
      string message;
      build_tiny_rcon_message(message);
      remove_temp_banned_ip_address(pl.ip_address, message, true);
    }
  }
}

void construct_tinyrcon_gui(HWND hWnd) noexcept
{
  const long draw_border_lines_flag{
    main_app.get_is_draw_border_lines() ? WS_BORDER : 0L
  };

  if (app_handles.hwnd_match_information != NULL) {
    ShowWindow(app_handles.hwnd_match_information, SW_HIDE);
    DestroyWindow(app_handles.hwnd_match_information);
  }

  app_handles.hwnd_match_information = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY, 10, 13, screen_width / 2 + 140, 30, hWnd, nullptr, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_match_information)
    FatalAppExitA(0, "Couldn't create 'app_handles.hwnd_match_information' richedit control!");

  assert(app_handles.hwnd_match_information != 0);
  SendMessage(app_handles.hwnd_match_information, EM_SETBKGNDCOLOR, NULL, color::black);
  scroll_to_beginning(app_handles.hwnd_match_information);
  set_rich_edit_control_colors(app_handles.hwnd_match_information, color::white, color::black, "Lucida Console");
  if (g_re_match_information_contents.empty())
    Edit_SetText(app_handles.hwnd_match_information, "");
  else
    print_colored_text(app_handles.hwnd_match_information, g_re_match_information_contents.c_str(), true, true, true);

  if (app_handles.hwnd_re_messages_data != NULL) {
    ShowWindow(app_handles.hwnd_re_messages_data, SW_HIDE);
    DestroyWindow(app_handles.hwnd_re_messages_data);
  }

  app_handles.hwnd_re_messages_data = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, draw_border_lines_flag | WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_LEFT | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_READONLY, screen_width / 2 + 170, 45, screen_width / 2 - 190, screen_height / 2 + 55, hWnd, (HMENU)ID_BANNEDEDIT, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_re_messages_data)
    FatalAppExitA(0, "Couldn't create 'g_banned_players_data' richedit control!");

  if (app_handles.hwnd_re_messages_data != 0) {
    SendMessage(app_handles.hwnd_re_messages_data, EM_SETBKGNDCOLOR, NULL, color::black);
    scroll_to_beginning(app_handles.hwnd_re_messages_data);
    set_rich_edit_control_colors(app_handles.hwnd_re_messages_data, color::white, color::black);
    if (g_message_data_contents.empty())
      Edit_SetText(app_handles.hwnd_re_messages_data, "");
    else
      print_colored_text(app_handles.hwnd_re_messages_data, g_message_data_contents.c_str(), true, true, true);
  }

  if (app_handles.hwnd_re_help_data != NULL) {
    ShowWindow(app_handles.hwnd_re_help_data, SW_HIDE);
    DestroyWindow(app_handles.hwnd_re_help_data);
  }

  app_handles.hwnd_re_help_data = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, draw_border_lines_flag | WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_LEFT | ES_AUTOVSCROLL | ES_READONLY, screen_width / 2 + 170, screen_height / 2 + 145, screen_width - 20 - (screen_width / 2 + 170), 280, hWnd, nullptr, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_re_help_data)
    FatalAppExitA(0, "Couldn't create 'g_re_help' richedit control!");
  if (app_handles.hwnd_re_help_data != 0) {
    SendMessage(app_handles.hwnd_re_help_data, EM_SETBKGNDCOLOR, NULL, color::black);
    scroll_to_beginning(app_handles.hwnd_re_help_data);
    set_rich_edit_control_colors(app_handles.hwnd_re_help_data, color::white, color::black);
    print_colored_text(app_handles.hwnd_re_help_data, user_help_message, true, false, false);
  }

  app_handles.hwnd_e_user_input = CreateWindowEx(0, "Edit", nullptr, WS_GROUP | WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 125, screen_height - 82, 450, 20, app_handles.hwnd_main_window, (HMENU)ID_USEREDIT, app_handles.hInstance, nullptr);

  app_handles.hwnd_button_warn = CreateWindowEx(NULL, "Button", "Warn", WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 10, screen_height - 125, 80, 30, app_handles.hwnd_main_window, (HMENU)ID_WARNBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_button_kick = CreateWindowEx(NULL, "Button", "Kick", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 110, screen_height - 125, 80, 30, app_handles.hwnd_main_window, (HMENU)ID_KICKBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_button_tempban = CreateWindowEx(NULL, "Button", "Tempban", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 210, screen_height - 125, 100, 30, app_handles.hwnd_main_window, (HMENU)ID_TEMPBANBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_button_ipban = CreateWindowEx(NULL, "Button", "Ban IP", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 330, screen_height - 125, 100, 30, app_handles.hwnd_main_window, (HMENU)ID_IPBANBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_button_view_tempbans = CreateWindowEx(NULL, "Button", "View temporary bans", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 450, screen_height - 125, 180, 30, app_handles.hwnd_main_window, (HMENU)ID_VIEWTEMPBANSBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_button_view_ipbans = CreateWindowEx(NULL, "Button", "View IP bans", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 650, screen_height - 125, 160, 30, app_handles.hwnd_main_window, (HMENU)ID_VIEWIPBANSBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_connect_button = CreateWindowEx(NULL, "Button", "Join server", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 590, screen_height - 87, 140, 30, app_handles.hwnd_main_window, (HMENU)ID_CONNECTBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_connect_private_slot_button = CreateWindowEx(NULL, "Button", "Join server (private slot)", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 750, screen_height - 87, 200, 30, app_handles.hwnd_main_window, (HMENU)ID_CONNECTPRIVATESLOTBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_say_button = CreateWindowEx(NULL, "Button", "Send public message", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 965, screen_height - 87, 150, 30, app_handles.hwnd_main_window, (HMENU)ID_SAY_BUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_tell_button = CreateWindowEx(NULL, "Button", "Send private message", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 1132, screen_height - 87, 160, 30, app_handles.hwnd_main_window, (HMENU)ID_TELL_BUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_quit_button = CreateWindowEx(NULL, "Button", "Exit", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 1312, screen_height - 87, 70, 30, app_handles.hwnd_main_window, (HMENU)ID_QUITBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_clear_messages_button = CreateWindowEx(NULL, "Button", "Clear messages screen", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 340, 8, 180, 28, app_handles.hwnd_main_window, (HMENU)ID_CLEARMESSAGESCREENBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_configure_server_settings_button = CreateWindowEx(NULL, "Button", "Configure server settings", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 540, 8, 180, 28, app_handles.hwnd_main_window, (HMENU)ID_BUTTON_CONFIGURE_SERVER_SETTINGS, app_handles.hInstance, NULL);

  app_handles.hwnd_refresh_players_data_button = CreateWindowEx(NULL, "Button", "Refresh players data", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 740, 8, 180, 28, app_handles.hwnd_main_window, (HMENU)ID_REFRESHDATABUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_combo_box_sortmode = CreateWindowEx(NULL, "Combobox", NULL, WS_GROUP | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_HSCROLL, 830, screen_height - 122, 275, 210, app_handles.hwnd_main_window, (HMENU)ID_COMBOBOX_SORTMODE, app_handles.hInstance, NULL);

  SetWindowSubclass(app_handles.hwnd_combo_box_sortmode, ComboProc, 0, 0);

  app_handles.hwnd_combo_box_map = CreateWindowEx(NULL, "Combobox", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_HSCROLL, screen_width / 2 + 220, screen_height / 2 + 110, 240, 200, app_handles.hwnd_main_window, (HMENU)ID_COMBOBOX_MAP, app_handles.hInstance, NULL);

  SetWindowSubclass(app_handles.hwnd_combo_box_map, ComboProc, 0, 0);

  app_handles.hwnd_combo_box_gametype = CreateWindowEx(NULL, "Combobox", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_HSCROLL, screen_width / 2 + 580, screen_height / 2 + 110, 60, 130, app_handles.hwnd_main_window, (HMENU)ID_COMBOBOX_GAMETYPE, app_handles.hInstance, NULL);

  SetWindowSubclass(app_handles.hwnd_combo_box_gametype, ComboProc, 0, 0);

  app_handles.hwnd_button_load = CreateWindowEx(NULL, "Button", "Load selected map", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 670, screen_height / 2 + 107, 160, 30, app_handles.hwnd_main_window, (HMENU)ID_LOADBUTTON, app_handles.hInstance, NULL);

  app_handles.hwnd_progress_bar = CreateWindowEx(0, PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_MARQUEE | PBS_SMOOTH, screen_width - 200, screen_height - 80, 170, 20, app_handles.hwnd_main_window, nullptr, app_handles.hInstance, nullptr);

  if (app_handles.hwnd_players_grid) {
    DestroyWindow(app_handles.hwnd_players_grid);
  }

  app_handles.hwnd_players_grid = CreateWindowEx(WS_EX_CLIENTEDGE, WC_SIMPLEGRIDA, "", WS_VISIBLE | WS_CHILD, 10, 45, screen_width / 2 + 140, screen_height - 180, hWnd, (HMENU)501, app_handles.hInstance, nullptr);

  hImageList = ImageList_Create(32, 24, ILC_COLOR32, flag_name_index.size(), 1);
  for (size_t i{}; i < flag_name_index.size(); ++i) {
    HBITMAP hbmp = (HBITMAP)LoadImage(app_handles.hInstance, MAKEINTRESOURCE(151 + i), IMAGE_BITMAP, 32, 24, LR_CREATEDIBSECTION | LR_DEFAULTSIZE);
    ImageList_Add(hImageList, hbmp, NULL);
  }

  initialize_players_grid(app_handles.hwnd_players_grid, 7, max_players_grid_rows, true);

  const auto &full_map_names_to_rcon_names = get_full_map_names_to_rcon_map_names_for_specified_game_name(main_app.get_game_name());

  for (const auto &[long_map_name, short_map_name] : full_map_names_to_rcon_names) {
    SendMessage(app_handles.hwnd_combo_box_map, CB_ADDSTRING, 0, (LPARAM)long_map_name.c_str());
  }

  const auto &full_gametype_names = get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());

  for (const auto &[short_gametype_name, long_gametype_name] : full_gametype_names) {
    SendMessage(app_handles.hwnd_combo_box_gametype, CB_ADDSTRING, 0, (LPARAM)short_gametype_name.c_str());
  }

  for (const auto &[sort_mode_name, sort_mode] : sort_mode_names_dict) {
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, (LPARAM)sort_mode_name.c_str());
  }

  const auto &rcon_map_names_to_full_map_names = get_rcon_map_names_to_full_map_names_for_specified_game_name(main_app.get_game_name());

  SendMessage(app_handles.hwnd_combo_box_map, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)rcon_map_names_to_full_map_names.at("mp_toujane").c_str());
  SendMessage(app_handles.hwnd_combo_box_gametype, CB_SELECTSTRING, (WPARAM)-1, (LPARAM) "ctf");
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_SELECTSTRING, (WPARAM)-1, (LPARAM) "Sort by pid in ascending order");

  UpdateWindow(hWnd);
}

void PutCell(HWND hgrid, const int row, const int col, const char *text) noexcept
{
  SGITEM item{};
  item.row = row;
  item.col = col;
  item.lpCurValue = (LPARAM)text;
  SimpleGrid_SetItemData(hgrid, &item);
}

void display_country_flag(HWND hgrid, const int row, const int col, const char *country_code) noexcept
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

void initialize_players_grid(HWND hgrid, const size_t cols, const size_t rows, const bool is_for_rcon_status) noexcept
{

  selected_row = 0;
  SimpleGrid_ResetContent(hgrid);
  SimpleGrid_SetAllowColResize(app_handles.hwnd_players_grid, TRUE);
  SimpleGrid_Enable(app_handles.hwnd_players_grid, TRUE);
  SimpleGrid_ExtendLastColumn(app_handles.hwnd_players_grid, TRUE);
  SimpleGrid_SetColAutoWidth(app_handles.hwnd_players_grid, TRUE);
  SimpleGrid_SetDoubleBuffer(app_handles.hwnd_players_grid, TRUE);
  SimpleGrid_SetEllipsis(app_handles.hwnd_players_grid, TRUE);
  SimpleGrid_SetGridLineColor(app_handles.hwnd_players_grid, colors.at(main_app.get_game_server().get_border_line_color()[1]));
  SimpleGrid_SetTitleHeight(app_handles.hwnd_players_grid, 0);
  SimpleGrid_SetHilightColor(app_handles.hwnd_players_grid, color::grey);
  SimpleGrid_SetHilightTextColor(app_handles.hwnd_players_grid, color::red);
  SimpleGrid_SetRowHeaderWidth(app_handles.hwnd_players_grid, 0);
  SimpleGrid_SetColsNumbered(app_handles.hwnd_players_grid, FALSE);
  SimpleGrid_SetRowHeight(app_handles.hwnd_players_grid, 28);

  for (size_t col_id{}; col_id < cols; ++col_id) {
    SGCOLUMN column{};
    column.dwType = GCT_EDIT;
    if (6 == col_id) {
      column.dwType = GCT_IMAGE;
      column.pOptional = hImageList;
    }
    column.lpszHeader = is_for_rcon_status ? rcon_status_grid_column_header_titles.at(col_id).c_str() : get_status_grid_column_header_titles.at(col_id).c_str();
    SimpleGrid_AddColumn(app_handles.hwnd_players_grid, &column);
  }

  for (size_t i{}; i < rows; ++i) {
    SimpleGrid_AddRow(hgrid, "");
  }

  for (size_t i{}; i < rows; ++i) {
    for (size_t j{}; j < cols; ++j) {
      if (6 == j) {
        SGITEM item{};
        item.row = i;
        item.col = j;
        item.lpCurValue = (LPARAM)0;
        SimpleGrid_SetItemData(hgrid, &item);
      } else {
        PutCell(hgrid, i, j, "");
      }
    }
  }

  Grid_OnSetFont(app_handles.hwnd_players_grid, hfontbody, TRUE);
  ShowHscroll(app_handles.hwnd_players_grid);

  SimpleGrid_SetColWidth(hgrid, 1, 60);
  SimpleGrid_SetColWidth(hgrid, 3, 300);
  if (is_for_rcon_status) {
    SimpleGrid_SetColWidth(hgrid, 5, 400);
    SimpleGrid_SetColWidth(hgrid, 6, 50);
  }
  SimpleGrid_SetSelectionMode(app_handles.hwnd_players_grid, GSO_FULLROW);
}

void clear_players_data_in_players_grid(HWND hgrid, const size_t start_row, const size_t last_row, const size_t cols) noexcept
{

  static char buffer[256];
  bool is_found_an_empty_cell{};
  for (size_t i{ start_row }; i < last_row && !is_found_an_empty_cell; ++i) {
    for (size_t j{}; j < cols; ++j) {
      if (6 == j) {
        SGITEM item{};
        item.row = i;
        item.col = j;
        item.lpCurValue = (LPARAM)0;
        SimpleGrid_SetItemData(hgrid, &item);
      } else {
        if (!is_found_an_empty_cell) {
          const string cell_text{ GetCellContents(hgrid, i, j) };
          if (cell_text.empty())
            is_found_an_empty_cell = true;
        }
        PutCell(hgrid, i, j, "");
      }
    }
  }
}

void display_players_data_in_players_grid(HWND hgrid) noexcept
{
  const size_t number_of_players{ main_app.get_game_server().get_number_of_players() };
  Edit_SetText(app_handles.hwnd_match_information, "");
  print_colored_text(app_handles.hwnd_match_information, match_information.c_str(), true, false, false);
  SendMessage(app_handles.hwnd_match_information, EM_SETSEL, 0, -1);
  SendMessage(app_handles.hwnd_match_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);

  if (number_of_players > 0) {
    for (size_t i{}; i < number_of_players; ++i) {
      PutCell(hgrid, i, 0, displayed_players_data[i].pid);
      PutCell(hgrid, i, 1, displayed_players_data[i].score);
      PutCell(hgrid, i, 2, displayed_players_data[i].ping);
      PutCell(hgrid, i, 3, displayed_players_data[i].player_name);
      PutCell(hgrid, i, 4, displayed_players_data[i].ip_address);
      PutCell(hgrid, i, 5, displayed_players_data[i].geo_info);
      display_country_flag(hgrid, i, 6, displayed_players_data[i].country_code);
    }
  }

  clear_players_data_in_players_grid(hgrid, number_of_players, max_players_grid_rows, 7);

  SimpleGrid_SetColWidth(hgrid, 1, 60);
  SimpleGrid_SetColWidth(hgrid, 3, 300);
  SimpleGrid_SetColWidth(hgrid, 5, 400);
  SimpleGrid_SetColWidth(hgrid, 6, 50);
  SimpleGrid_EnableEdit(hgrid, FALSE);
}

bool is_alpha(const char ch) noexcept
{
  return !is_decimal_digit(ch) && !is_ws(ch);
}

bool is_decimal_digit(const char ch) noexcept
{
  return ch >= '0' && ch <= '9';
}

bool is_ws(const char ch) noexcept
{
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\f' || ch == '\v';
}

void change_hdc_fg_color(HDC hdc, COLORREF fg_color) noexcept
{
  SetTextColor(hdc, fg_color);
}

bool check_if_selected_cell_indices_are_valid(const int row_index, const int col_index) noexcept
{
  return row_index >= 0 && row_index < (int)main_app.get_game_server().get_number_of_players() && col_index >= 0 && col_index < 7;
}

void CenterWindow(HWND hwnd) noexcept
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
  app_handles.hwnd_e_reason = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 100, 220, 470, 20, app_handles.hwnd_confirmation_dialog, (HMENU)ID_EDIT_ADMIN_REASON, app_handles.hInstance, nullptr);

  if (app_handles.hwnd_yes_button) {
    DestroyWindow(app_handles.hwnd_yes_button);
  }
  app_handles.hwnd_yes_button = CreateWindowEx(NULL, "Button", "Yes", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 100, 250, 50, 20, app_handles.hwnd_confirmation_dialog, (HMENU)ID_YES_BUTTON, app_handles.hInstance, NULL);

  if (app_handles.hwnd_no_button) {
    DestroyWindow(app_handles.hwnd_no_button);
  }
  app_handles.hwnd_no_button = CreateWindowEx(NULL, "Button", "No", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 170, 250, 50, 20, app_handles.hwnd_confirmation_dialog, (HMENU)ID_NO_BUTTON, app_handles.hInstance, NULL);

  print_colored_text(app_handles.hwnd_re_confirmation_message, msg, true, true, true);
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

void process_sort_type_change_request(const sort_type new_sort_type) noexcept
{
  type_of_sort = new_sort_type;

  switch (type_of_sort) {
  case sort_type::pid_asc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort pid asc ^2command and refreshing players data.\n");
    break;
  case sort_type::pid_desc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort pid desc ^2command and refreshing players data.\n");
    break;
  case sort_type::score_asc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort score asc ^2command and refreshing players data.\n");
    break;
  case sort_type::score_desc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort score desc ^2command and refreshing players data.\n");
    break;
  case sort_type::ping_asc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort ping asc ^2command and refreshing players data.\n");
    break;
  case sort_type::ping_desc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort ping desc ^2command and refreshing players data.\n");
    break;
  case sort_type::name_asc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort name asc ^2command and refreshing players data.\n");
    break;
  case sort_type::name_desc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort name desc ^2command and refreshing players data.\n");
    break;
  case sort_type::ip_asc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort ip asc ^2command and refreshing players data.\n");
    break;
  case sort_type::ip_desc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort ip desc ^2command and refreshing players data.\n");
    break;
  case sort_type::geo_asc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort geo asc ^2command and refreshing players data.\n");
    break;
  case sort_type::geo_desc:
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort geo desc ^2command and refreshing players data.\n");
    break;
  }

  ComboBox_SelectString(app_handles.hwnd_combo_box_sortmode, 0, sort_type_to_sort_names_dict.at(type_of_sort).c_str());

  if (main_app.get_is_connection_settings_valid()) {
    initiate_sending_rcon_status_command_now();
  }
  is_process_combobox_item_selection_event = true;
}

void update_game_server_setting(std::string key,
  std::string value)
{
  if (key == "pswrd") {
    main_app.get_game_server().set_is_password_protected('0' != value[0]);
  } else if (key == "protocol") {
    main_app.get_game_server().set_protocol_info(stoi(value));
  } else if (key == "hostname") {
    main_app.get_game_server().set_server_name(value);
  } else if (key == "mapname") {
    main_app.get_game_server().set_current_map(value);
  } else if (key == "clients") {
    main_app.get_game_server().set_current_number_of_players(stoi(value));
  } else if (key == "sv_maxclients") {
    main_app.get_game_server().set_max_number_of_players(stoi(value));
  } else if (key == "gametype") {
    main_app.get_game_server().set_current_game_type(value);
  } else if (key == "is_pure") {
    main_app.get_game_server().set_is_pure('0' != value[0]);
  } else if (key == "max_ping") {
    main_app.get_game_server().set_max_ping(stoi(value));
  } else if (key == "game") {
    main_app.get_game_server().set_game_mod_name(value);
  } else if (key == "con_disabled") {
    main_app.get_game_server().set_is_console_disabled('0' != value[0]);
  } else if (key == "kc") {
    main_app.get_game_server().set_is_kill_cam_enabled('0' != value[0]);
  } else if (key == "hw") {
    main_app.get_game_server().set_hw_info(stoi(value));
  } else if (key == "mod") {
    main_app.get_game_server().set_is_mod_enabled('0' != value[0]);
  } else if (key == "voice") {
    main_app.get_game_server().set_is_voice_enabled('0' != value[0]);
  } else if (key == "fs_game") {
    main_app.get_game_server().set_game_mod_name(value);
    if (value != "" && value != "main")
      main_app.get_game_server().set_is_mod_enabled(true);
    else
      main_app.get_game_server().set_is_mod_enabled(false);
  } else if (key == "g_antilag") {
    main_app.get_game_server().set_is_anti_lag_enabled('1' == value[0]);
  } else if (key == "g_gametype") {
    main_app.get_game_server().set_current_game_type(value);
  } else if (key == "gamename") {
    main_app.get_game_server().set_game_name(value);
  } else if (key == "mapname") {
    main_app.get_game_server().set_current_map(value);
  } else if (key == "protocol") {
    main_app.get_game_server().set_protocol_info(stoi(value));
  } else if (key == "scr_friendlyfire") {
    main_app.get_game_server().set_is_friendly_fire_enabled('0' != value[0]);
  } else if (key == "scr_killcam") {
    main_app.get_game_server().set_is_kill_cam_enabled('0' != value[0]);
  } else if (key == "shortversion") {
    main_app.get_game_server().set_server_short_version(value);
  } else if (key == "sv_allowAnonymous") {
    main_app.get_game_server().set_is_allow_anonymous_players('0' != value[0]);
  } else if (key == "sv_floodProtect") {
    main_app.get_game_server().set_is_flood_protected('0' != value[0]);
  } else if (key == "sv_hostname") {
    main_app.get_game_server().set_server_name(value);
  } else if (key == "sv_maxclients") {
    main_app.get_game_server().set_max_number_of_players(stoi(value));
  } else if (key == "sv_maxPing") {
    main_app.get_game_server().set_max_ping(stoi(value));
  } else if (key == "sv_maxRate") {
    main_app.get_game_server().set_max_server_rate(stoi(value));
  } else if (key == "sv_minPing") {
    main_app.get_game_server().set_min_ping(stoi(value));
  } else if (key == "sv_privateClients") {
    main_app.get_game_server().set_max_private_clients(stoi(value));
  } else if (key == "sv_pure") {
    main_app.get_game_server().set_is_pure('0' != value[0]);
  } else if (key == "sv_voice") {
    main_app.get_game_server().set_is_voice_enabled('0' != value[0]);
  }

  main_app.get_game_server().get_server_settings().emplace(move(key),
    move(value));
}

std::pair<bool, game_name_t> check_if_specified_server_ip_port_and_rcon_password_are_valid(const char *ip_address, const uint_least16_t port_number, const char *rcon_password)
{
  connection_manager cm;
  string reply;
  cm.send_and_receive_rcon_data("version", reply, ip_address, port_number, rcon_password, true);

  game_name_t game_name{ game_name_t::unknown };

  if (reply.find("Invalid password.") != string::npos || reply.find("rconpassword") != string::npos || reply.find("Bad rcon") != string::npos) {
    cm.send_and_receive_rcon_data("getstatus", reply, ip_address, port_number, rcon_password, true);
    const string &gn{ main_app.get_game_server().get_game_name() };
    return { false, game_name_to_game_name_t.contains(gn) ? game_name_to_game_name_t.at(gn) : game_name_t::unknown };
  }


  if (stl::helper::str_contains(reply, "CoD1", 0, true))
    game_name = game_name_t::cod1;
  else if (stl::helper::str_contains(reply, "CoD2", 0, true))
    game_name = game_name_t::cod2;
  else if (stl::helper::str_contains(reply, "CoD4", 0, true))
    game_name = game_name_t::cod4;
  else if (stl::helper::str_contains(reply, "CoD5", 0, true))
    game_name = game_name_t::cod5;
  else
    return { false, game_name };

  return { true, game_name };
}

bool show_and_process_tinyrcon_configuration_panel(const char *title)
{
  is_terminate_tinyrcon_settings_configuration_dialog_window.store(false);
  if (app_handles.hwnd_configuration_dialog) {
    DestroyWindow(app_handles.hwnd_configuration_dialog);
  }
  app_handles.hwnd_configuration_dialog = CreateWindowEx(NULL, wcex_configuration_dialog.lpszClassName, title, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME /*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX*/, 0, 0, 920, 820, app_handles.hwnd_main_window, nullptr, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_configuration_dialog)
    return false;

  (void)CreateWindowEx(0, "Static", "Server name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 10, 120, 20, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  if (app_handles.hwnd_server_name) {
    DestroyWindow(app_handles.hwnd_server_name);
  }

  app_handles.hwnd_server_name = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 10, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_NAME, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_server_name)
    return false;

  if (app_handles.hwnd_server_ip_address) {
    DestroyWindow(app_handles.hwnd_server_ip_address);
  }
  (void)CreateWindowEx(0, "Static", "Server IP address:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 40, 130, 20, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  app_handles.hwnd_server_ip_address = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 40, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_IP, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_server_ip_address)
    return false;

  if (app_handles.hwnd_server_port) {
    DestroyWindow(app_handles.hwnd_server_port);
  }
  (void)CreateWindowEx(0, "Static", "Server port:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 70, 120, 20, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  app_handles.hwnd_server_port = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 70, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_PORT, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_server_port)
    return false;

  if (app_handles.hwnd_rcon_password) {
    DestroyWindow(app_handles.hwnd_rcon_password);
  }
  (void)CreateWindowEx(0, "Static", "Rcon password:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 100, 140, 20, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  app_handles.hwnd_rcon_password = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 100, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_RCON, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_rcon_password)
    return false;

  if (app_handles.hwnd_refresh_time_period) {
    DestroyWindow(app_handles.hwnd_refresh_time_period);
  }
  (void)CreateWindowEx(0, "Static", "Refresh time period:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 130, 140, 20, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  app_handles.hwnd_refresh_time_period = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 130, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_REFRESH_TIME_PERIOD, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_refresh_time_period)
    return false;

  if (app_handles.hwnd_cod1_path_edit) {
    DestroyWindow(app_handles.hwnd_cod1_path_edit);
  }
  (void)CreateWindowEx(0, "Static", "Call of Duty installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 165, 200, 25, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  app_handles.hwnd_cod1_path_edit = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 160, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD1_PATH, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_cod1_path_edit)
    return false;

  SendMessage(app_handles.hwnd_cod1_path_edit, EM_SETLIMITTEXT, 1024, 0);

  if (app_handles.hwnd_cod1_path_button) {
    DestroyWindow(app_handles.hwnd_cod1_path_button);
  }
  app_handles.hwnd_cod1_path_button = CreateWindowEx(NULL, "Button", "Browse for codmp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680, 170, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD1_PATH, app_handles.hInstance, NULL);
  if (!app_handles.hwnd_cod1_path_button)
    return false;

  if (app_handles.hwnd_cod2_path_edit) {
    DestroyWindow(app_handles.hwnd_cod2_path_edit);
  }
  (void)CreateWindowEx(0, "Static", "Call of Duty 2 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 215, 200, 25, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  app_handles.hwnd_cod2_path_edit = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 210, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD2_PATH, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_cod2_path_edit)
    return false;

  if (app_handles.hwnd_cod2_path_button) {
    DestroyWindow(app_handles.hwnd_cod2_path_button);
  }
  app_handles.hwnd_cod2_path_button = CreateWindowEx(NULL, "Button", "Browse for cod2mp_s.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680, 220, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD2_PATH, app_handles.hInstance, NULL);
  if (!app_handles.hwnd_cod2_path_button)
    return false;

  if (app_handles.hwnd_cod4_path_edit) {
    DestroyWindow(app_handles.hwnd_cod4_path_edit);
  }
  (void)CreateWindowEx(0, "Static", "Call of Duty 4 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 265, 200, 25, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  app_handles.hwnd_cod4_path_edit = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 260, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD4_PATH, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_cod4_path_edit)
    return false;

  if (app_handles.hwnd_cod4_path_button) {
    DestroyWindow(app_handles.hwnd_cod4_path_button);
  }
  app_handles.hwnd_cod4_path_button = CreateWindowEx(NULL, "Button", "Browse for iw3mp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680, 270, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD4_PATH, app_handles.hInstance, NULL);
  if (!app_handles.hwnd_cod4_path_button)
    return false;

  if (app_handles.hwnd_cod5_path_edit) {
    DestroyWindow(app_handles.hwnd_cod5_path_edit);
  }
  (void)CreateWindowEx(0, "Static", "Call of Duty 5 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 315, 200, 25, app_handles.hwnd_configuration_dialog, NULL, app_handles.hInstance, nullptr);

  app_handles.hwnd_cod5_path_edit = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 310, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD5_PATH, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_cod5_path_edit)
    return false;

  if (app_handles.hwnd_cod5_path_button) {
    DestroyWindow(app_handles.hwnd_cod5_path_button);
  }
  app_handles.hwnd_cod5_path_button = CreateWindowEx(NULL, "Button", "Browse for cod5mp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680, 320, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD5_PATH, app_handles.hInstance, NULL);
  if (!app_handles.hwnd_cod5_path_button)
    return false;

  if (app_handles.hwnd_re_confirmation_message) {
    DestroyWindow(app_handles.hwnd_re_confirmation_message);
  }

  app_handles.hwnd_re_confirmation_message = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_LEFT | ES_AUTOVSCROLL | ES_READONLY, 220, 360, 450, 360, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);
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

  if (app_handles.hwnd_save_settings_button) {
    DestroyWindow(app_handles.hwnd_save_settings_button);
  }
  app_handles.hwnd_save_settings_button = CreateWindowEx(NULL, "Button", "Save changes", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 220, 740, 120, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_SAVE_CHANGES, app_handles.hInstance, NULL);
  if (!app_handles.hwnd_save_settings_button)
    return false;

  if (app_handles.hwnd_test_connection_button) {
    DestroyWindow(app_handles.hwnd_test_connection_button);
  }

  app_handles.hwnd_test_connection_button = CreateWindowEx(NULL, "Button", "Test connection", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 360, 740, 140, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_TEST_CONNECTION, app_handles.hInstance, NULL);

  if (!app_handles.hwnd_test_connection_button)
    return false;

  if (app_handles.hwnd_close_button) {
    DestroyWindow(app_handles.hwnd_close_button);
  }
  app_handles.hwnd_close_button = CreateWindowEx(NULL, "Button", "Cancel", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 520, 740, 60, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CANCEL, app_handles.hInstance, NULL);

  if (!app_handles.hwnd_close_button)
    return false;

  if (app_handles.hwnd_exit_tinyrcon_button) {
    DestroyWindow(app_handles.hwnd_exit_tinyrcon_button);
  }
  app_handles.hwnd_exit_tinyrcon_button = CreateWindowEx(NULL, "Button", "Exit TinyRcon", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 600, 740, 120, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_EXIT_TINYRCON, app_handles.hInstance, NULL);

  if (!app_handles.hwnd_exit_tinyrcon_button)
    return false;

  Edit_SetText(app_handles.hwnd_server_name, main_app.get_game_server_name().c_str());
  Edit_SetText(app_handles.hwnd_server_ip_address, main_app.get_game_server().get_server_ip_address().c_str());
  char buffer_port[8];
  snprintf(buffer_port, 8, "%d", main_app.get_game_server().get_server_port());
  Edit_SetText(app_handles.hwnd_server_port, buffer_port);
  Edit_SetText(app_handles.hwnd_rcon_password, main_app.get_game_server().get_rcon_password().c_str());
  char buffer_rt[8];
  snprintf(buffer_rt, 8, "%zu", main_app.get_game_server().get_check_for_banned_players_time_period());
  Edit_SetText(app_handles.hwnd_refresh_time_period, buffer_rt);

  if (!main_app.get_codmp_exe_path().empty() && (str_contains(main_app.get_cod2mp_exe_path(), "-applaunch 2620") || check_if_file_path_exists(main_app.get_codmp_exe_path().c_str()))) {
    Edit_SetText(app_handles.hwnd_cod1_path_edit, main_app.get_codmp_exe_path().c_str());
  } else {
    const char *found_cod1_path = find_call_of_duty_1_installation_path(false);
    if (found_cod1_path != nullptr && len(found_cod1_path) > 0 && (str_contains(found_cod1_path, "-applaunch 2620") || check_if_file_path_exists(found_cod1_path))) {
      Edit_SetText(app_handles.hwnd_cod1_path_edit, found_cod1_path);
    } else {
      Edit_SetText(app_handles.hwnd_cod1_path_edit, "");
    }
  }

  if (!main_app.get_cod2mp_exe_path().empty() && (str_contains(main_app.get_cod2mp_exe_path(), "-applaunch 2630") || check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str()))) {
    Edit_SetText(app_handles.hwnd_cod2_path_edit, main_app.get_cod2mp_exe_path().c_str());
  } else {
    const char *found_cod2_path = find_call_of_duty_2_installation_path(false);
    if (found_cod2_path != nullptr && len(found_cod2_path) > 0 && (str_contains(found_cod2_path, "-applaunch 2630") || check_if_file_path_exists(found_cod2_path))) {
      Edit_SetText(app_handles.hwnd_cod2_path_edit, found_cod2_path);
    } else {
      Edit_SetText(app_handles.hwnd_cod2_path_edit, "");
    }
  }

  if (!main_app.get_iw3mp_exe_path().empty() && (str_contains(main_app.get_cod2mp_exe_path(), "-applaunch 7940") || check_if_file_path_exists(main_app.get_iw3mp_exe_path().c_str()))) {
    Edit_SetText(app_handles.hwnd_cod4_path_edit, main_app.get_iw3mp_exe_path().c_str());
  } else {

    const char *found_cod4_path = find_call_of_duty_4_installation_path(false);
    if (found_cod4_path != nullptr && len(found_cod4_path) > 0 && (str_contains(found_cod4_path, "-applaunch 7940") || check_if_file_path_exists(found_cod4_path))) {
      Edit_SetText(app_handles.hwnd_cod4_path_edit, found_cod4_path);
    } else {
      Edit_SetText(app_handles.hwnd_cod4_path_edit, "");
    }
  }

  if (!main_app.get_cod5mp_exe_path().empty() && (str_contains(main_app.get_cod2mp_exe_path(), "-applaunch 10090") || check_if_file_path_exists(main_app.get_cod5mp_exe_path().c_str()))) {
    Edit_SetText(app_handles.hwnd_cod5_path_edit, main_app.get_cod5mp_exe_path().c_str());
  } else {
    const char *found_cod5_path = find_call_of_duty_5_installation_path(false);
    if (found_cod5_path != nullptr && len(found_cod5_path) > 0 && (str_contains(found_cod5_path, "-applaunch 10090") || check_if_file_path_exists(found_cod5_path))) {
      Edit_SetText(app_handles.hwnd_cod5_path_edit, found_cod5_path);
    } else {
      Edit_SetText(app_handles.hwnd_cod5_path_edit, "");
    }
  }

  print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Please enter and verify the correctness of your game server's input settings by clicking on the ^3Test connection ^2button.\n", true, false, false);
  CenterWindow(app_handles.hwnd_configuration_dialog);
  ShowWindow(app_handles.hwnd_configuration_dialog, SW_SHOW);
  SetFocus(app_handles.hwnd_close_button);

  MSG wnd_msg{};
  while (GetMessage(&wnd_msg, NULL, NULL, NULL) != 0) {

    if (WM_KEYDOWN == wnd_msg.message && VK_ESCAPE == wnd_msg.wParam) {
      EnableWindow(app_handles.hwnd_main_window, TRUE);
      SetFocus(app_handles.hwnd_e_user_input);
      DestroyWindow(app_handles.hwnd_configuration_dialog);

    } else if (WM_KEYDOWN == wnd_msg.message && VK_RETURN == wnd_msg.wParam) {
      const auto focused_hwnd = GetFocus();
      if (focused_hwnd == app_handles.hwnd_close_button) {
        is_terminate_tinyrcon_settings_configuration_dialog_window.store(true);
        EnableWindow(app_handles.hwnd_main_window, TRUE);
        SetFocus(app_handles.hwnd_e_user_input);
        DestroyWindow(app_handles.hwnd_configuration_dialog);
      } else if (focused_hwnd == app_handles.hwnd_test_connection_button) {
        process_button_test_connection_click_event(app_handles.hwnd_configuration_dialog);
      } else if (focused_hwnd == app_handles.hwnd_save_settings_button) {
        process_button_save_changes_click_event(app_handles.hwnd_configuration_dialog);
      } else if (focused_hwnd == app_handles.hwnd_exit_tinyrcon_button) {
        EnableWindow(app_handles.hwnd_main_window, TRUE);
        SetFocus(app_handles.hwnd_e_user_input);
        DestroyWindow(app_handles.hwnd_configuration_dialog);
        is_terminate_program.store(true);
        {
          lock_guard<mutex> ul{ mu };
          exit_flag.notify_one();
        }
      }

    } else if (WM_KEYDOWN == wnd_msg.message && VK_TAB == wnd_msg.wParam) {
      const auto focused_hwnd = GetFocus();
      if (focused_hwnd == app_handles.hwnd_save_settings_button)
        SetFocus(app_handles.hwnd_test_connection_button);
      else if (focused_hwnd == app_handles.hwnd_test_connection_button)
        SetFocus(app_handles.hwnd_close_button);
      else if (focused_hwnd == app_handles.hwnd_close_button)
        SetFocus(app_handles.hwnd_exit_tinyrcon_button);
      else if (focused_hwnd == app_handles.hwnd_exit_tinyrcon_button)
        SetFocus(app_handles.hwnd_save_settings_button);
    } else if (WM_KEYDOWN == wnd_msg.message && VK_LEFT == wnd_msg.wParam) {
      const auto focused_hwnd = GetFocus();
      if (focused_hwnd == app_handles.hwnd_save_settings_button)
        SetFocus(app_handles.hwnd_exit_tinyrcon_button);
      else if (focused_hwnd == app_handles.hwnd_test_connection_button)
        SetFocus(app_handles.hwnd_save_settings_button);
      else if (focused_hwnd == app_handles.hwnd_close_button)
        SetFocus(app_handles.hwnd_test_connection_button);
      else if (focused_hwnd == app_handles.hwnd_exit_tinyrcon_button)
        SetFocus(app_handles.hwnd_close_button);
    } else if (WM_KEYDOWN == wnd_msg.message && VK_RIGHT == wnd_msg.wParam) {
      const auto focused_hwnd = GetFocus();
      if (focused_hwnd == app_handles.hwnd_save_settings_button)
        SetFocus(app_handles.hwnd_test_connection_button);
      else if (focused_hwnd == app_handles.hwnd_test_connection_button)
        SetFocus(app_handles.hwnd_close_button);
      else if (focused_hwnd == app_handles.hwnd_close_button)
        SetFocus(app_handles.hwnd_exit_tinyrcon_button);
      else if (focused_hwnd == app_handles.hwnd_exit_tinyrcon_button)
        SetFocus(app_handles.hwnd_save_settings_button);
    }
    DispatchMessage(&wnd_msg);
  }

  return true;
}

void process_button_save_changes_click_event(HWND hwnd)
{

  static char msg_buffer[1024];
  string new_server_name;
  string new_server_ip;
  uint_least16_t new_port{};
  string new_rcon_password;
  int new_refresh_time_period{ 5 };
  bool is_invalid_entry{};

  Edit_GetText(app_handles.hwnd_server_name, msg_buffer, std::size(msg_buffer));
  if (stl::helper::len(msg_buffer) > 0) {
    new_server_name.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Server name cannot be left empty!\n", true, false, false);
  }

  if (!is_invalid_entry) {
    Edit_GetText(app_handles.hwnd_server_ip_address, msg_buffer, std::size(msg_buffer));
    const auto ip_len{ stl::helper::len(msg_buffer) };
    unsigned long guid{};
    if (ip_len > 0 && check_ip_address_validity({ msg_buffer, ip_len }, guid)) {
      new_server_ip.assign(msg_buffer);
    } else {
      is_invalid_entry = true;
      const string user_message{ "^1You have entered an invalid IP address! ^5("s + msg_buffer + ")\n"s };
      print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(), true, false, false);
    }
  }

  if (!is_invalid_entry) {

    Edit_GetText(app_handles.hwnd_server_port, msg_buffer, std::size(msg_buffer));
    if (int number{}; stl::helper::len(msg_buffer) > 0 && is_valid_decimal_whole_number(msg_buffer, number) && number >= 1024 && number <= 65535) {
      new_port = static_cast<uint_least16_t>(number);
    } else {
      is_invalid_entry = true;
      const string user_message{ "^1You have entered an invalid server port number! ^5("s + msg_buffer + ")\n^1Port number ^3must be >= ^11024 ^3and <= ^165535\n"s };
      print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(), true, false, false);
    }
  }

  if (!is_invalid_entry) {

    Edit_GetText(app_handles.hwnd_rcon_password, msg_buffer, std::size(msg_buffer));
    if (stl::helper::len(msg_buffer) > 3) {
      new_rcon_password.assign(msg_buffer);
    } else {
      is_invalid_entry = true;
      const string user_message{ "^1You have entered an invalid rcon password! ^5("s + msg_buffer + ")\n^1Rcon password ^3must be at least ^14 characters ^3long.\n"s };
      print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(), true, false, false);
    }
  }

  if (!is_invalid_entry) {

    Edit_GetText(app_handles.hwnd_refresh_time_period, msg_buffer, std::size(msg_buffer));
    if (int number{}; stl::helper::len(msg_buffer) > 0 && is_valid_decimal_whole_number(msg_buffer, number) && number >= 5 && number <= 120) {
      new_refresh_time_period = number;
    } else {
      is_invalid_entry = true;
      const string user_message{ "^3You have entered an ^1invalid time period ^3value ^1("s + msg_buffer + ")\n^2Valid time period is ^15 <= ^2time period ^1<= 120\n"s };
      print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(), true, false, false);
    }
  }

  if (!is_invalid_entry) {
    snprintf(msg_buffer, std::size(msg_buffer), "\n^2Testing connection with the specified game server (^5%s^2) at ^5%s:%d ^2using the following ^5Tiny^6Rcon ^2settings:\n^1Server name: ^5%s\n^1Server IP address: ^5%s\n^1Server port number: ^5%d\n^1Server rcon password: ^5%s\n^1Refresh time period: ^5%d second(s)\n", new_server_name.c_str(), new_server_ip.c_str(), new_port, new_server_name.c_str(), new_server_ip.c_str(), new_port, new_rcon_password.c_str(), new_refresh_time_period);
    print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer, true, false, false);
    const auto [test_result, game_name] = check_if_specified_server_ip_port_and_rcon_password_are_valid(new_server_ip.c_str(), new_port, new_rcon_password.c_str());
    if (test_result) {
      main_app.set_is_connection_settings_valid(true);
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Testing connection SUCCEEDED!\n", true, false, false);
    } else {
      main_app.set_is_connection_settings_valid(false);
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Testing connection FAILED!\n", true, false, false);
    }

    snprintf(msg_buffer, std::size(msg_buffer), "\n^2Saving the following ^5Tiny^6Rcon ^2settings:\n^1Server name: ^5%s\n^1Server IP address: ^5%s\n^1Server port number: ^5%d\n^1Server rcon password: ^5%s\n^1Refresh time period: ^5%d\n", new_server_name.c_str(), new_server_ip.c_str(), new_port, new_rcon_password.c_str(), new_refresh_time_period);
    print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer, true, false, false);

    main_app.set_game_name(game_name);
    main_app.set_game_server_name(std::move(new_server_name));
    main_app.get_game_server().set_server_ip_address(std::move(new_server_ip));
    main_app.get_game_server().set_server_port(new_port);
    main_app.get_game_server().set_rcon_password(std::move(new_rcon_password));
    main_app.get_game_server().set_check_for_banned_players_time_period(new_refresh_time_period);

    Edit_GetText(app_handles.hwnd_cod1_path_edit, path_buffer, std::size(path_buffer));
    trim_in_place(path_buffer);
    if (str_contains(path_buffer, "-applaunch 2620") || check_if_file_path_exists(path_buffer)) {
      main_app.set_codmp_exe_path(path_buffer);
    }

    Edit_GetText(app_handles.hwnd_cod2_path_edit, path_buffer, std::size(path_buffer));
    trim_in_place(path_buffer);
    if (str_contains(path_buffer, "-applaunch 2630") || check_if_file_path_exists(path_buffer)) {
      main_app.set_cod2mp_exe_path(path_buffer);
    }

    Edit_GetText(app_handles.hwnd_cod4_path_edit, path_buffer, std::size(path_buffer));
    trim_in_place(path_buffer);
    if (str_contains(path_buffer, "-applaunch 7940") || check_if_file_path_exists(path_buffer)) {
      main_app.set_iw3mp_exe_path(path_buffer);
    }

    Edit_GetText(app_handles.hwnd_cod5_path_edit, path_buffer, std::size(path_buffer));
    trim_in_place(path_buffer);
    if (str_contains(path_buffer, "-applaunch 10090") || check_if_file_path_exists(path_buffer)) {
      main_app.set_cod5mp_exe_path(path_buffer);
    }

    write_tiny_rcon_json_settings_to_file("config\\tinyrcon.json");
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Displayed server configuration settings have been successfully saved!\n", true, false, false);
    MessageBox(hwnd, "Displayed server configuration settings have been successfully saved!", "Successfully saved server settings", MB_ICONINFORMATION | MB_OK);
    EnableWindow(app_handles.hwnd_main_window, TRUE);
    SetFocus(app_handles.hwnd_e_user_input);
    DestroyWindow(hwnd);
  } else {
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Failed to save displayed server configuration settings!\n", true, false, false);
    MessageBox(hwnd, "Failed to save displayed server configuration settings!", "Failed to save server settings!", MB_ICONWARNING | MB_OK);
  }
}

void process_button_test_connection_click_event(HWND)
{
  static char msg_buffer[1024];
  string new_server_name;
  string new_server_ip;
  uint_least16_t new_port{};
  string new_rcon_password;
  int new_refresh_time_period{ 5 };
  bool is_invalid_entry{};

  Edit_GetText(app_handles.hwnd_server_name, msg_buffer, std::size(msg_buffer));
  if (stl::helper::len(msg_buffer) > 0) {
    new_server_name.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Server name text field cannot be empty!\n", true, false, false);
  }

  Edit_GetText(app_handles.hwnd_server_ip_address, msg_buffer, std::size(msg_buffer));
  const auto ip_len{ stl::helper::len(msg_buffer) };
  unsigned long guid{};
  if (ip_len > 0 && check_ip_address_validity({ msg_buffer, ip_len }, guid)) {
    new_server_ip.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    const string user_message1{ "^1You have entered an invalid IP address! ^5("s + msg_buffer + ")\n"s };
    print_colored_text(app_handles.hwnd_re_confirmation_message, user_message1.c_str(), true, false, false);
  }


  Edit_GetText(app_handles.hwnd_server_port, msg_buffer, std::size(msg_buffer));
  int number{};
  if (stl::helper::len(msg_buffer) > 0 && is_valid_decimal_whole_number(msg_buffer, number)) {
    new_port = static_cast<uint_least16_t>(number);
  } else {
    is_invalid_entry = true;
    const string user_message2{ "^1You have entered an invalid server port number! ^5("s + msg_buffer + ")\n"s };
    print_colored_text(app_handles.hwnd_re_confirmation_message, user_message2.c_str(), true, false, false);
  }


  Edit_GetText(app_handles.hwnd_rcon_password, msg_buffer, std::size(msg_buffer));
  if (stl::helper::len(msg_buffer) > 0) {
    new_rcon_password.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    const string user_message3{ "^1You have entered an invalid rcon password! ^5("s + msg_buffer + ")\n"s };
    print_colored_text(app_handles.hwnd_re_confirmation_message, user_message3.c_str(), true, false, false);
  }

  Edit_GetText(app_handles.hwnd_refresh_time_period, msg_buffer, std::size(msg_buffer));
  number = 0;
  if (stl::helper::len(msg_buffer) > 0 && is_valid_decimal_whole_number(msg_buffer, number)) {
    new_refresh_time_period = number;
  } else {
    is_invalid_entry = true;
    const string user_message4{ "^1You have entered an invalid time period value (5 <= time period <= 120)! ^5("s + msg_buffer + ")\n"s };
    print_colored_text(app_handles.hwnd_re_confirmation_message, user_message4.c_str(), true, false, false);
  }

  if (!is_invalid_entry) {
    snprintf(msg_buffer, std::size(msg_buffer), "\n^2Testing connection with the specified game server (^5%s^2) at ^5%s:%d ^2using the following ^5Tiny^6Rcon ^2settings:\n^1Server name: ^5%s\n^1Server IP address: ^5%s\n^1Server port number: ^5%d\n^1Server rcon password: ^5%s\n^1Refresh time period: ^5%d second(s)\n", new_server_name.c_str(), new_server_ip.c_str(), new_port, new_server_name.c_str(), new_server_ip.c_str(), new_port, new_rcon_password.c_str(), new_refresh_time_period);
    print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer, true, false, false);
    const auto [test_result, game_name] = check_if_specified_server_ip_port_and_rcon_password_are_valid(new_server_ip.c_str(), new_port, new_rcon_password.c_str());
    if (test_result) {
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Testing connection SUCCEEDED!\n", true, false, false);
    } else {
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Testing connection FAILED!\n", true, false, false);
    }
  }
}

void display_context_menu_over_grid(const int mouse_x, const int mouse_y, const int row_index)
{
  static char warn_player_command[128]{};
  static char kick_player_command[128]{};
  static char tempban_player_command[128]{};
  static char ipban_player_command[128]{};

  HMENU hPopupMenu = CreatePopupMenu();
  if (check_if_selected_cell_indices_are_valid(row_index, 0)) {
    string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, row_index, 0) };
    if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
      selected_pid_str.erase(0, 2);
    if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
      auto &player_data = get_player_data_for_pid(pid);
      if (player_data.pid == pid) {
        snprintf(warn_player_command, 128, "Warn player (name: %s | pid: %d)", player_data.player_name, pid);
        remove_all_color_codes(warn_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_WARNBUTTON, warn_player_command);
        snprintf(kick_player_command, 128, "Kick player (name: %s | pid: %d)", player_data.player_name, pid);
        remove_all_color_codes(kick_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_KICKBUTTON, kick_player_command);
        snprintf(tempban_player_command, 128, "Temporarily ban player's IP (name: %s | pid: %d)", player_data.player_name, pid);
        remove_all_color_codes(tempban_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TEMPBANBUTTON, tempban_player_command);
        snprintf(ipban_player_command, 128, "Ban player's IP (name: %s | pid: %d)", player_data.player_name, pid);
        remove_all_color_codes(ipban_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_IPBANBUTTON, ipban_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      }
    }
  }
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWTEMPBANSBUTTON, "View temporary bans");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPBANSBUTTON, "View permanent bans");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_REFRESHDATABUTTON, "Refresh players data");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
  if (main_app.get_game_server().get_number_of_players() > 0) {
    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_PID, "Sort players data by PID");
    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_SCORE, "Sort players data by score");
    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_PING, "Sort players data by ping");
    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_NAME, "Sort players data by player name");
    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_IP, "Sort players data by IP address");
    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_GEO, "Sort players data by geoinformation");
    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
  }
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CONNECTBUTTON, "Join the game server");
  InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CONNECTPRIVATESLOTBUTTON, "Join the game server using a private slot");
  SetForegroundWindow(app_handles.hwnd_main_window);
  POINT p{ mouse_x, mouse_y };
  ClientToScreen(app_handles.hwnd_players_grid, &p);
  TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, p.x, p.y, 0, app_handles.hwnd_main_window, NULL);
}

const std::regex &get_appropriate_status_regex_for_specified_game_name(const game_name_t game_name)
{

  static const std::regex status_regex_for_cod1{ R"(^\s*(\d+)\s+(-?\d+)\s+(-?\d+|[a-zA-Z]{4})\s+(\d*)\s+([^\n]+?)\s+(\d+)\s+(\d+\.\d+\.\d+\.\d+):(-?\d+)\s+(-?\d+)\s+(\d+)$)" };

  static const std::regex status_regex_for_cod2{ R"(^\s*(\d+)\s+(-?\d+)\s+(-?\d+|[a-zA-Z]{4})\s+(\d*)\s+([^\n]+?)\s+(\d+)\s+(\d+\.\d+\.\d+\.\d+):(-?\d+)\s+(-?\d+)\s+(\d+)$)" };

  static const std::regex status_regex_for_cod4{ R"(^\s*(\d+)\s+(-?\d+)\s+(-?\d+|[a-zA-Z]{4})\s+([0-9a-fA-F]{32}?)\s+([^\n]+?)\s+(\d+)\s+(\d+\.\d+\.\d+\.\d+):(-?\d+)\s+(-?\d+)\s+(\d+)$)" };

  static const std::regex status_regex_for_cod5{ R"(^\s*(\d+)\s+(-?\d+)\s+(-?\d+|[a-zA-Z]{4})\s+([0-9a-fA-F]{32}?)\s+([^\n]+?)\s+(\d+)\s+(\d+\.\d+\.\d+\.\d+):(-?\d+)\s+(-?\d+)\s+(\d+)$)" };

  switch (game_name) {
  case game_name_t::unknown:
    return status_regex_for_cod2;
  case game_name_t::cod1:
    return status_regex_for_cod1;
  case game_name_t::cod2:
    return status_regex_for_cod2;
  case game_name_t::cod4:
    return status_regex_for_cod4;
  case game_name_t::cod5:
    return status_regex_for_cod5;
  default:
    return status_regex_for_cod2;
  }
}

const std::map<std::string, std::string> &get_rcon_map_names_to_full_map_names_for_specified_game_name(const game_name_t game_name) noexcept
{

  static const map<string, string> cod1_rcon_map_name_full_map_names{
    { "mp_brecourt", "Brecourt, France" },
    { "mp_carentan", "Carentan, France" },
    { "mp_chateau", "Chateau, Germany" },
    { "mp_dawnville", "St. Mere Eglise, France" },
    { "mp_depot", "Depot, Poland" },
    { "mp_harbor", "Harbor, Germany" },
    { "mp_hurtgen", "Hurtgen, Germany" },
    { "mp_pavlov", "Pavlov, Russia" },
    { "mp_powcamp", "Powcamp, Poland" },
    { "mp_railyard", "Stalingrad, Russia" },
    { "mp_rocket", "Rocket, Germany" },
    { "mp_ship", "Ship, England" }
  };

  static const map<string, string> cod2_rcon_map_name_full_map_names{
    { "mp_breakout", "Villers-Bocage, France" },
    { "mp_brecourt", "Brecourt, France" },
    { "mp_burgundy", "Burgundy, France" },
    { "mp_carentan", "Carentan, France" },
    { "mp_dawnville", "St. Mere Eglise, France" },
    { "mp_decoy", "El Alamein, Egypt" },
    { "mp_downtown", "Moscow, Russia" },
    { "mp_farmhouse", "Beltot, France" },
    { "mp_leningrad", "Leningrad, Russia" },
    { "mp_matmata", "Matmata, Tunisia" },
    { "mp_railyard", "Stalingrad, Russia" },
    { "mp_toujane", "Toujane, Tunisia" },
    { "mp_trainstation", "Caen, France" },
    { "mp_rhine", "Wallendar, Germany" },
    { "mp_harbor", "Rostov, Russia" }
  };


  static const map<string, string> cod4_rcon_map_name_full_map_names{
    { "mp_backlot", "Backlot" },
    { "mp_bloc", "Bloc" },
    { "mp_bog", "Bog" },
    { "mp_broadcast", "Broadcast" },
    { "mp_carentan", "Chinatown" },
    { "mp_cargoship", "Wet Work" },
    { "mp_citystreets", "District" },
    { "mp_convoy", "Ambush" },
    { "mp_countdown", "Countdown" },
    { "mp_crash", "Crash" },
    { "mp_crash_snow", "Crash Snow" },
    { "mp_creek", "Creek" },
    { "mp_crossfire", "Crossfire" },
    { "mp_farm", "Downpour" },
    { "mp_killhouse", "Killhouse" },
    { "mp_overgrown", "Overgrown" },
    { "mp_pipeline", "Pipeline" },
    { "mp_shipment", "Shipment" },
    { "mp_showdown", "Showdown" },
    { "mp_strike", "Strike" },
    { "mp_vacant", "Vacant" }
  };

  static const map<string, string> cod5_rcon_map_name_full_map_names{
    { "mp_airfield", "Airfield" },
    { "mp_asylum", "Asylum" },
    { "mp_castle", "Castle" },
    { "mp_shrine", "Cliffside" },
    { "mp_courtyard", "Courtyard" },
    { "mp_dome", "Dome" },
    { "mp_downfall", "Downfall" },
    { "mp_hangar", "Hangar" },
    { "mp_makin", "Makin" },
    { "mp_outskirts", "Outskirts" },
    { "mp_roundhouse", "Roundhouse" },
    { "mp_seelow", "Seelow" },
    { "mp_suburban", "Upheaval" }
  };

  switch (game_name) {
  case game_name_t::cod1:
    return cod1_rcon_map_name_full_map_names;
  case game_name_t::cod2:
    return cod2_rcon_map_name_full_map_names;
  case game_name_t::cod4:
    return cod4_rcon_map_name_full_map_names;
  case game_name_t::cod5:
    return cod5_rcon_map_name_full_map_names;
  default:
    return cod2_rcon_map_name_full_map_names;
  }
}

const std::map<std::string, std::string> &get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(const game_name_t game_name) noexcept
{
  static const map<string, string> cod1_full_gametype_names{
    { "dm", "Deathmatch" },
    { "tdm", "Team Deathmatch" },
    { "sd", "Search and Destroy" },
    { "re", "Retrieval" },
    { "bel", "Behind Enemy Lines" },
    { "hq", "Headquarters" }
  };

  static const map<string, string> cod2_full_gametype_names{
    { "dm", "Deathmatch" },
    { "tdm", "Team-Deathmatch" },
    { "ctf", "Capture The Flag" },
    { "hq", "Headquarters" },
    { "sd", "Search & Destroy" },
    { "dom", "Domination" }
  };

  static const map<string, string> cod4_full_gametype_names{
    { "dm", "Free for all" },
    { "war", "Team Deathmatch" },
    { "sab", "Sabotage" },
    { "sd", "Search and Destroy" },
    { "dom", "Domination" },
    { "koth", "Headquarters" }
  };

  static const map<string, string> cod5_full_gametype_names{
    { "dm", "Free for all" },
    { "tdm", "Team Deathmatch" },
    { "sab", "Sabotage" },
    { "sd", "Search and Destroy" },
    { "dom", "Domination" },
    { "ctf", "Capture the Flag" },
    { "twar", "War" },
    { "koth", "Headquarters" }
  };

  switch (game_name) {
  case game_name_t::cod1:
    return cod1_full_gametype_names;
  case game_name_t::cod2:
    return cod2_full_gametype_names;
  case game_name_t::cod4:
    return cod4_full_gametype_names;
  case game_name_t::cod5:
    return cod5_full_gametype_names;
  default:
    return cod2_full_gametype_names;
  }
}

const std::map<std::string, std::string> &get_full_map_names_to_rcon_map_names_for_specified_game_name(const game_name_t game_name) noexcept
{
  static const map<string, string> cod1_full_map_name_rcon_map_names{
    { "Brecourt, France", "mp_brecourt" },
    { "Carentan, France", "mp_carentan" },
    { "Chateau, Germany", "mp_chateau" },
    { "St. Mere Eglise, France", "mp_dawnville" },
    { "Depot, Poland", "mp_depot" },
    { "Harbor, Germany", "mp_harbor" },
    { "Hurtgen, Germany", "mp_hurtgen" },
    { "Pavlov, Russia", "mp_pavlov" },
    { "Powcamp, Poland", "mp_powcamp" },
    { "Stalingrad, Russia", "mp_railyard" },
    { "Rocket, Germany", "mp_rocket" },
    { "Ship, England", "mp_ship" }
  };

  static const map<string, string> cod2_full_map_name_rcon_map_names{
    { "Villers-Bocage, France", "mp_breakout" },
    { "Brecourt, France", "mp_brecourt" },
    { "Burgundy, France", "mp_burgundy" },
    { "Carentan, France", "mp_carentan" },
    { "St. Mere Eglise, France", "mp_dawnville" },
    { "El Alamein, Egypt", "mp_decoy" },
    { "Moscow, Russia", "mp_downtown" },
    { "Beltot, France", "mp_farmhouse" },
    { "Leningrad, Russia", "mp_leningrad" },
    { "Matmata, Tunisia", "mp_matmata" },
    { "Stalingrad, Russia", "mp_railyard" },
    { "Toujane, Tunisia", "mp_toujane" },
    { "Caen, France", "mp_trainstation" },
    { "Wallendar, Germany", "mp_rhine" },
    { "Rostov, Russia", "mp_harbor" }
  };

  static const map<string, string> cod4_full_map_name_rcon_map_names{
    { "Backlot", "mp_backlot" },
    { "Bloc", "mp_bloc" },
    { "Bog", "mp_bog" },
    { "Broadcast", "mp_broadcast" },
    { "Chinatown", "mp_carentan" },
    { "Wet Work", "mp_cargoship" },
    { "District", "mp_citystreets" },
    { "Ambush", "mp_convoy" },
    { "Countdown", "mp_countdown" },
    { "Crash", "mp_crash" },
    { "Crash Snow", "mp_crash_snow" },
    { "Creek", "mp_creek" },
    { "Crossfire", "mp_crossfire" },
    { "Downpour", "mp_farm" },
    { "Killhouse", "mp_killhouse" },
    { "Overgrown", "mp_overgrown" },
    { "Pipeline", "mp_pipeline" },
    { "Shipment", "mp_shipment" },
    { "Showdown", "mp_showdown" },
    { "Strike", "mp_strike" },
    { "Vacant", "mp_vacant" }
  };


  static const map<string, string> cod5_full_map_name_rcon_map_names{
    { "Airfield", "mp_airfield" },
    { "Asylum", "mp_asylum" },
    { "Castle", "mp_castle" },
    { "Cliffside", "mp_shrine" },
    { "Courtyard", "mp_courtyard" },
    { "Dome", "mp_dome" },
    { "Downfall", "mp_downfall" },
    { "Hangar", "mp_hangar" },
    { "Makin", "mp_makin" },
    { "Outskirts", "mp_outskirts" },
    { "Roundhouse", "mp_roundhouse" },
    { "Seelow", "mp_seelow" },
    { "Upheaval", "mp_suburban" }
  };

  switch (game_name) {
  case game_name_t::cod1:
    return cod1_full_map_name_rcon_map_names;
  case game_name_t::cod2:
    return cod2_full_map_name_rcon_map_names;
  case game_name_t::cod4:
    return cod4_full_map_name_rcon_map_names;
  case game_name_t::cod5:
    return cod5_full_map_name_rcon_map_names;
  default:
    return cod2_full_map_name_rcon_map_names;
  }
}

bool initialize_and_verify_server_connection_settings()
{
  do {
    const string ip{ main_app.get_game_server().get_server_ip_address() };
    const uint_least16_t port{ main_app.get_game_server().get_server_port() };
    const string rcon{ main_app.get_game_server().get_rcon_password() };
    const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(ip.c_str(), port, rcon.c_str());
    main_app.set_game_name(result.second);
    if (result.first) {
      main_app.set_is_connection_settings_valid(true);
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Initializing network settings for communicating with the game server.\n", true, true, true);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Initializing network settings has completed.\n", true, true, true);
    } else {
      main_app.set_is_connection_settings_valid(false);
      /*show_error(app_handles.hwnd_main_window, "Failed to establish test connection with the specified game server!\nPlease verify your game server's IP address, port and rcon password settings.", 0);*/
      if (!show_and_process_tinyrcon_configuration_panel("Configure and verify your game server's settings.")) {
        show_error(app_handles.hwnd_main_window, "Failed to construct and show TinyRcon configuration dialog!", 0);
      }

      if (is_terminate_program.load())
        return false;

      if (is_terminate_tinyrcon_settings_configuration_dialog_window.load())
        break;
    }
  } while (main_app.get_game_name() == game_name_t::unknown);

  return true;
}

void initiate_sending_rcon_status_command_now()
{
  if (main_app.get_is_connection_settings_valid()) {
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing rcon command: ^5!s\n", true, true, true);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5Sending status rcon command to the server.\n", true, true, true);
  } else {
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing rcon command: ^5!gs\n", true, true, true);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5Sending getstatus rcon command to the server.\n", true, true, true);
  }
  atomic_counter.store(main_app.get_game_server().get_check_for_banned_players_time_period());
  RECT bounding_rectangle{
    screen_width - 480,
    screen_height - 78,
    screen_width - 200,
    screen_height - 58,
  };
  InvalidateRect(app_handles.hwnd_main_window, &bounding_rectangle, TRUE);
  is_refresh_players_data_event.store(true);
}

void prepare_players_data_for_display(const bool is_log_status_table)
{

  static char buffer[256];
  size_t longest_name_length{ 32 };
  size_t longest_country_length{ 32 };

  auto &players = main_app.get_game_server().get_players_data();
  const size_t number_of_players{ main_app.get_game_server().get_number_of_players() };

  if (number_of_players > 0) {
    sort_players_data(players, type_of_sort);
    if (!players.empty()) {
      longest_name_length =
        std::max(longest_name_length, find_longest_player_name_length(players, false, number_of_players));
      longest_country_length =
        std::max(longest_country_length,
          find_longest_player_country_city_info_length(players, number_of_players));
    }
  }

  ostringstream log;
  match_information = prepare_current_match_information();

  if (is_log_status_table) {
    remove_all_color_codes(match_information);
    log << match_information;
  }

  const string &pd = main_app.get_game_server().get_data_player_pid_color();
  const string &sd = main_app.get_game_server().get_data_player_score_color();
  const string &pgd = main_app.get_game_server().get_data_player_ping_color();
  const string &ipd = main_app.get_game_server().get_data_player_ip_color();
  const string &gd = main_app.get_game_server().get_data_player_geoinfo_color();

  const string decoration_line(46 + longest_name_length + longest_country_length, '=');
  if (is_log_status_table) {
    log << string{ "\n"s + decoration_line + "\n"s };
    log << "|";
    log << right << setw(3) << "Pid"
        << " | "
        << setw(6) << "Score"
        << " | "
        << setw(4) << "Ping"
        << " | "
        << left << setw(longest_name_length)
        << "Player name"
        << " | "
        << left << setw(16) << "IP address"
        << " | "
        << left << setw(longest_country_length)
        << "Country, region, city"
        << "|";
    log << string{ "\n"s + decoration_line + "\n"s };
  }
  if (0 == number_of_players) {
    if (is_log_status_table) {
      string server_message{ main_app.get_server_message() };
      remove_all_color_codes(server_message);
      log << server_message;
      const size_t printed_chars_count = get_number_of_characters_without_color_codes(main_app.get_server_message().c_str());
      const string filler{
        string(decoration_line.length() - 1 - printed_chars_count, ' ') + "|\n"s
      };
      log << filler;
    }
  } else {

    for (size_t i{}; i < number_of_players; ++i) {

      const player_data &p{ players[i] };

      snprintf(buffer, 5, "%s%d", pd.c_str(), p.pid);
      strcpy_s(displayed_players_data[i].pid, std::size(displayed_players_data[i].pid), buffer);

      snprintf(buffer, std::size(buffer), "%s%d", sd.c_str(), p.score);
      strcpy_s(displayed_players_data[i].score, std::size(displayed_players_data[i].score), buffer);

      snprintf(buffer, std::size(buffer), "%s%s", pgd.c_str(), p.ping);
      strcpy_s(displayed_players_data[i].ping, std::size(displayed_players_data[i].ping), buffer);

      strcpy_s(displayed_players_data[i].player_name, std::size(displayed_players_data[i].player_name), p.player_name);

      snprintf(buffer, std::size(buffer), "%s%s", ipd.c_str(), p.ip_address);
      strcpy_s(displayed_players_data[i].ip_address, std::size(displayed_players_data[i].ip_address), buffer);

      snprintf(buffer, std::size(buffer), "%s%s, %s", gd.c_str(), (strlen(p.country_name) != 0 ? p.country_name : p.region), p.city);
      strcpy_s(displayed_players_data[i].geo_info, std::size(displayed_players_data[i].geo_info), buffer);
      displayed_players_data[i].country_code = p.country_code;

      if (is_log_status_table) {
        log << "|";
        log << right << setw(3) << p.pid << " | " << setw(6)
            << p.score << " | "
            << setw(4) << p.ping << " | ";
        const size_t printed_name_char_count{ get_number_of_characters_without_color_codes(p.player_name) };
        char name[33];
        strcpy_s(name, 33, p.player_name);
        remove_all_color_codes(name);
        log << name;
        if (printed_name_char_count < longest_name_length) {
          log << string(longest_name_length - printed_name_char_count, ' ');
        }
        log << " | ";
        snprintf(buffer, std::size(buffer), "%s, %s", (strlen(p.country_name) != 0 ? p.country_name : p.region), p.city);
        log << left << setw(16) << p.ip_address << " | " << left
            << setw(longest_country_length) << buffer
            << "|" << '\n';
      }
    }
  }

  if (is_log_status_table) {
    log << string{ decoration_line + "\n"s };
    log_message(log.str(), true);
  }
}

void prepare_players_data_for_display_of_getstatus_response(const bool is_log_status_table)
{
  static char buffer[256];
  size_t longest_name_length{ 32 };

  auto &players = main_app.get_game_server().get_players_data();
  const size_t number_of_players{ main_app.get_game_server().get_number_of_players() };

  if (number_of_players > 0) {
    sort_players_data(players, sort_type::score_desc);
    if (!players.empty()) {
      longest_name_length =
        std::max(longest_name_length, find_longest_player_name_length(players, false, number_of_players));
    }
  }

  ostringstream log;
  match_information = prepare_current_match_information();

  if (is_log_status_table) {
    remove_all_color_codes(match_information);
    log << match_information;
  }

  const string &pd = main_app.get_game_server().get_data_player_pid_color();
  const string &sd = main_app.get_game_server().get_data_player_score_color();
  const string &pgd = main_app.get_game_server().get_data_player_ping_color();

  const string decoration_line(24 + longest_name_length, '=');
  if (is_log_status_table) {
    log << string{ "\n"s + decoration_line + "\n"s }.c_str();
    log << "|" << setw(6) << " score"
        << " | " << setw(4) << "ping"
        << " | " << setw(longest_name_length)
        << left << "Player name" << '|' << string{ "\n"s + decoration_line + "\n"s };
    if (number_of_players == 0) {
      log << string{ "| Server is empty." };
      const string filler{
        string(decoration_line.length() - 1 - stl::helper::len("| Server is empty."), ' ') + "|\n"s
      };
      log << filler;
    }
  }

  if (number_of_players > 0) {

    for (size_t i{}; i < number_of_players; ++i) {

      const player_data &p{ players[i] };

      snprintf(buffer, 5, "%s%d", pd.c_str(), p.pid);
      strcpy_s(displayed_players_data[i].pid, std::size(displayed_players_data[i].pid), buffer);

      snprintf(buffer, std::size(buffer), "%s%d", sd.c_str(), p.score);
      strcpy_s(displayed_players_data[i].score, std::size(displayed_players_data[i].score), buffer);

      snprintf(buffer, std::size(buffer), "%s%s", pgd.c_str(), p.ping);
      strcpy_s(displayed_players_data[i].ping, std::size(displayed_players_data[i].ping), buffer);

      strcpy_s(displayed_players_data[i].player_name, std::size(displayed_players_data[i].player_name), p.player_name);

      displayed_players_data[i].ip_address[0] = 0;
      displayed_players_data[i].geo_info[0] = 0;
      displayed_players_data[i].country_code = "xy";

      if (is_log_status_table) {
        log << "|" << right << setw(3) << p.pid << " | " << setw(6) << p.score << " | " << setw(4) << p.ping << " | ";
        string name{ p.player_name };
        remove_all_color_codes(name);
        log << name;
        const size_t printed_name_char_count{ get_number_of_characters_without_color_codes(p.player_name) };
        if (printed_name_char_count < longest_name_length) {
          log << string(longest_name_length - printed_name_char_count, ' ');
        }
        log << "|\n";
      }
    }
  }

  if (is_log_status_table) {
    log << string{ decoration_line + "\n"s };
    log_message(log.str(), true);
  }
}

size_t get_file_size_in_bytes(const char *file_path) noexcept
{

  ifstream input{ file_path, std::ios::binary | std::ios::in | std::ios::ate };
  if (!input) {
    perror(file_path);
    return 0;
  }
  const std::fstream::pos_type file_size = input.tellg();
  return static_cast<size_t>(file_size);
}
