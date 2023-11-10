#include "tiny_rcon_client_application.h"
#include "game_server.h"
#include "stl_helper_functions.hpp"
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
#include <bitfilecompressor.hpp>
#include <bit7zlibrary.hpp>
#include <bitextractor.hpp>
#include <bitexception.hpp>
#include <bittypes.hpp>
#include <crtdbg.h>

// Call of duty steam appid: 2620
// Call of duty 2 steam appid: 2630
// Call of duty 4: Modern Warfare steam appid: 7940
// Call of duty 5: World at war steam appid: 10090

#if defined(_DEBUG)
#pragma comment(lib, "bit7z_d.lib")
#else
#pragma comment(lib, "bit7z.lib")
#endif

using namespace std;
using namespace stl::helper;
using namespace std::filesystem;

using stl::helper::trim_in_place;

extern tiny_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;

extern char const *const tinyrcon_config_file_path;

extern char const *const tempbans_file_path;

extern char const *const banned_ip_addresses_file_path;
extern char const *const protected_ip_addresses_file_path;

extern char const *const ip_range_bans_file_path;
extern char const *const protected_ip_address_ranges_file_path;

extern char const *const banned_countries_list_file_path;
extern char const *const protected_countries_list_file_path;

extern char const *const banned_cities_list_file_path;
extern char const *const protected_cities_list_file_path;

extern const char *prompt_message;
extern const char *refresh_players_data_fmt_str;
extern PROCESS_INFORMATION pr_info;
extern sort_type type_of_sort;

extern WNDCLASSEX wcex_confirmation_dialog, wcex_configuration_dialog;
extern HFONT font_for_players_grid_data;
extern RECT client_rect;

extern const int screen_width;
extern const int screen_height;
extern int selected_row;
extern int selected_col;
extern const char *user_help_message;
extern const size_t max_players_grid_rows{ 64 };
extern bool is_tinyrcon_initialized;
std::atomic<bool> is_terminate_program{ false };
std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window{ false };
extern volatile std::atomic<bool> is_refresh_players_data_event;
extern volatile std::atomic<bool> is_display_permanently_banned_players_data_event;
extern volatile std::atomic<bool> is_display_temporarily_banned_players_data_event;
extern volatile std::atomic<bool> is_display_banned_cities_data_event;
extern volatile std::atomic<bool> is_display_banned_countries_data_event;
extern std::atomic<int> admin_choice;
extern std::string admin_reason;

// recursive_mutex re_messages_mutex;
// extern mutex mu;
// extern condition_variable exit_flag;

// recursive_mutex print_data_mutex;
recursive_mutex log_data_mutex;
recursive_mutex protect_get_user_input;
recursive_mutex protect_banned_players_data;

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
string online_admins_information;

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
  { "Sort by pid in asc. order", sort_type::pid_asc },
  { "Sort by pid in desc. order", sort_type::pid_desc },
  { "Sort by score in asc. order", sort_type::score_asc },
  { "Sort by score in desc. order", sort_type::score_desc },
  { "Sort by ping in asc. order", sort_type::ping_asc },
  { "Sort by ping in desc. order", sort_type::ping_desc },
  { "Sort by name in asc. order", sort_type::name_asc },
  { "Sort by name in desc. order", sort_type::name_desc },
  { "Sort by IP in asc. order", sort_type::ip_asc },
  { "Sort by IP in desc. order", sort_type::ip_desc },
  { "Sort by geoinfo in asc. order", sort_type::geo_asc },
  { "Sort by geoinfo in desc. order", sort_type::geo_desc }
};


extern const std::unordered_map<sort_type, string> sort_type_to_sort_names_dict{
  { sort_type::pid_asc, "Sort by pid in asc. order" },
  { sort_type::pid_desc, "Sort by pid in desc. order" },
  { sort_type::score_asc, "Sort by score in asc. order" },
  { sort_type::score_desc, "Sort by score in desc. order" },
  { sort_type::ping_asc, "Sort by ping in asc. order" },
  { sort_type::ping_desc, "Sort by ping in desc. order" },
  { sort_type::name_asc, "Sort by name in asc. order" },
  { sort_type::name_desc, "Sort by name in desc. order" },
  { sort_type::ip_asc, "Sort by IP in asc. order" },
  { sort_type::ip_desc, "Sort by IP in desc. order" },
  { sort_type::geo_asc, "Sort by geoinfo in asc. order" },
  { sort_type::geo_desc, "Sort by geoinfo in desc. order" }
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
  { "!t", "^5!t ^1public message ^2-> ^5sends ^1public message ^5to all logged in ^1admins." },
  { "!y", "^5!y ^3username ^1private message ^2-> ^5sends ^1private message ^5to ^1admin ^5whose username is ^1username." },
  { "!sort",
    "^5!sort player_data asc|desc ^2-> examples: !sort pid asc, !sort pid desc, "
    "!sort score asc, !sort score desc\n !sort ping asc, !sort ping desc, "
    "!sort name asc, !sort name desc, !sort ip asc, !sort ip desc, !sort geo asc, !sort geo desc" },
  { "!list",
    "^5!list user|rcon ^2-> shows a list of available user or rcon level commands "
    "that are available for admin\n to type into the console program." },
  { "!ranges",
    "^5!ranges ^2-> displays data about all ^1IP address range bans." },
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
  { "!banrange",
    "^5!banrange pid|ip_address_range [reason] ^2-> ^5bans IP address range of player whose 'pid' is equal to specified 'pid' number." },
  { "!unbanrange",
    "^5!unbanrange ip_address_range [reason] ^2-> ^5removes previously banned IP address range 'ip_address_range'." },
  { "!admins", "^5!admins ^2-> ^5Displays information about all registered admins." },
  { "!protectip",
    "^5!protectip pid|ip_address [reason] ^2-> protects player's IP address whose 'pid' number or "
    "'IP address' is equal to specified 'pid' or 'ip_address', respectively." },
  { "!unprotectip",
    "^5!unprotectip pid|ip_address [reason] ^2-> removes protected IP address of player whose 'pid' number or "
    "'IP address' is equal to specified 'pid' or 'ip_address', respectively." },
  { "!protectiprange",
    "^5!protectiprange pid|ip_address_range [reason] ^2-> protects player's IP address range whose 'pid' number or "
    "'IP address range' is equal to specified 'pid' or 'ip_address_range', respectively." },
  { "!unprotectiprange",
    "^5!unprotectiprange pid|ip_address_range [reason] ^2-> removes protected IP address range of player whose 'pid' number or "
    "'IP address range' is equal to specified 'pid' or 'ip_address_range', respectively." },
  { "!protectcity",
    "^5!protectcity pid|city_name ^2-> protects player's city whose 'pid' number or "
    "'city name' is equal to specified 'pid' or 'city_name', respectively." },
  { "!unprotectcity",
    "^5!unprotectcity pid|city_name ^2-> removes protected city of player whose 'pid' number or "
    "'city name' is equal to specified 'pid' or 'city_name', respectively." },
  { "!protectcountry",
    "^5!protectcountry pid|country_name ^2-> protects player's country whose 'pid' number or "
    "'country name' is equal to specified 'pid' or 'valid_ip_address', respectively." },
  { "!unprotectcountry",
    "^5!unprotectcountry pid|country_name ^2-> removes protected IP address of player whose 'pid' number or "
    "'country name' is equal to specified 'pid' or 'country_name', respectively." },
};

extern const unordered_set<string> user_commands_set{
  "!admins",
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
  "y",
  "!y",
  "sort",
  "!sort",
  "list",
  "!list",
  "!bans",
  "ranges",
  "!ranges",
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
  "!br",
  "!banrange",
  "!ubr",
  "!unbanrange",
  "!protectip",
  "!unprotectip",
  "!protectiprange",
  "!unprotectiprange",
  "!protectcity",
  "!unprotectcity",
  "!protectcountry",
  "!unprotectcountry"
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
  { "say", R"(^5say "public message to all players" ^2-> Sends "public message to all players" on the server.)" },
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

  main_app.get_connection_manager().get_geoip_data() = std::move(geoip_data);


  return true;
}

bool create_necessary_folders_and_files(const std::vector<string> &folder_file_paths)
{

  unordered_set<string> created_folders;

  for (const auto &file_ : folder_file_paths) {
    const directory_entry entry{ file_ };
    if (!entry.exists()) {

      string file_name;

      size_t last_sep_pos{ file_.find_last_of("./\\") };
      if (string::npos != last_sep_pos && '.' == file_[last_sep_pos]) {
        last_sep_pos = file_.find_last_of("/\\", last_sep_pos - 1);
        file_name = file_.substr(last_sep_pos != string::npos ? last_sep_pos + 1 : 0);
      } else {
        last_sep_pos = file_.length();
      }

      string parent_path{ string::npos != last_sep_pos ? file_.substr(0, last_sep_pos) : ""s };

      if (!parent_path.empty() && !created_folders.contains(parent_path)) {
        error_code ec{};
        create_directories(parent_path, ec);
        if (ec.value() != 0)
          return false;
        created_folders.emplace(std::move(parent_path));
      }

      if (!file_name.empty() && !check_if_file_path_exists(entry.path().string().c_str())) {

        if (file_name == "tinyrcon.json") {
          write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
        } else {
          ofstream file_to_create{ entry.path().string().c_str() };
          if (!file_to_create)
            return false;
          file_to_create.flush();
          file_to_create.close();
        }
      }
    }
  }

  return true;
}

bool write_tiny_rcon_json_settings_to_file(
  const char *file_path) noexcept
{
  std::ofstream config_file{ file_path };

  if (!config_file) {
    const string re_msg{ "^3Error writing default tiny rcon client's configuration data\nto specified tinyrcon.json file: ^1"s + file_path + "\n^5Please make sure that the main application folder's properties are not set to read-only mode!"s };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str());
    return false;
  }

  config_file << "{\n"
              << R"("username": ")" << main_app.get_username() << "\",\n";

  config_file << R"("program_title": ")" << main_app.get_program_title() << "\",\n";
  config_file << "\"check_for_banned_players_time_interval\": " << main_app.get_game_server().get_check_for_banned_players_time_period() << ",\n";
  config_file << R"("game_server_name": ")" << main_app.get_game_server_name() << "\",\n";
  config_file << R"("rcon_server_ip_address": ")" << main_app.get_game_server().get_server_ip_address() << "\",\n";
  config_file << "\"rcon_port\": " << main_app.get_game_server().get_server_port() << ",\n";
  config_file << R"("rcon_password": ")" << main_app.get_game_server().get_rcon_password() << "\",\n";
  config_file << R"("private_slot_password": ")" << main_app.get_game_server().get_private_slot_password() << "\",\n";
  config_file << R"("tiny_rcon_server_ip": ")" << main_app.get_tiny_rcon_server_ip_address() << "\",\n";
  config_file << "\"tiny_rcon_server_port\": " << main_app.get_tiny_rcon_server_port() << ",\n";
  config_file << R"("tiny_rcon_ftp_server_username": ")" << main_app.get_tiny_rcon_ftp_server_username() << "\",\n";
  config_file << R"("tiny_rcon_ftp_server_password": ")" << main_app.get_tiny_rcon_ftp_server_password() << "\",\n";
  config_file << R"("codmp_exe_path": ")" << main_app.get_codmp_exe_path() << "\",\n";
  config_file << R"("cod2mp_s_exe_path": ")" << main_app.get_cod2mp_exe_path() << "\",\n";
  config_file << R"("iw3mp_exe_path": ")" << main_app.get_iw3mp_exe_path() << "\",\n";
  config_file << R"("cod5mp_exe_path": ")" << main_app.get_cod5mp_exe_path() << "\",\n";
  config_file << "\"is_automatic_city_kick_enabled\": " << (main_app.get_game_server().get_is_automatic_city_kick_enabled() ? "true" : "false") << ",\n";
  config_file << "\"is_automatic_country_kick_enabled\": " << (main_app.get_game_server().get_is_automatic_country_kick_enabled() ? "true" : "false") << ",\n";
  config_file << "\"enable_automatic_connection_flood_ip_ban\": " << (main_app.get_game_server().get_is_enable_automatic_connection_flood_ip_ban() ? "true" : "false") << ",\n";
  config_file << "\"minimum_number_of_connections_from_same_ip_for_automatic_ban\": " << main_app.get_game_server().get_minimum_number_of_connections_from_same_ip_for_automatic_ban() << ",\n";
  config_file << "\"number_of_warnings_for_automatic_kick\": " << main_app.get_game_server().get_maximum_number_of_warnings_for_automatic_kick() << ",\n";
  config_file << "\"disable_automatic_kick_messages\": " << (main_app.get_is_disable_automatic_kick_messages() ? "true" : "false")
              << ",\n";
  config_file << "\"use_original_admin_messages\": " << (main_app.get_is_use_original_admin_messages() ? "true" : "false")
              << ",\n";
  config_file << R"("user_defined_warn_msg": ")" << main_app.get_admin_messages()["user_defined_warn_msg"] << "\",\n";
  config_file << R"("user_defined_kick_msg": ")" << main_app.get_admin_messages()["user_defined_kick_msg"] << "\",\n";
  config_file << R"("user_defined_temp_ban_msg": ")" << main_app.get_admin_messages()["user_defined_temp_ban_msg"] << "\",\n";
  config_file << R"("user_defined_ban_msg": ")" << main_app.get_admin_messages()["user_defined_ban_msg"] << "\",\n";
  config_file << R"("user_defined_ip_ban_msg": ")" << main_app.get_admin_messages()["user_defined_ip_ban_msg"] << "\",\n";
  config_file << R"("user_defined_ip_address_range_ban_msg": ")" << main_app.get_admin_messages()["user_defined_ip_address_range_ban_msg"] << "\",\n";
  config_file << R"("user_defined_city_ban_msg": ")" << main_app.get_admin_messages()["user_defined_city_ban_msg"] << "\",\n";
  config_file << R"("user_defined_city_unban_msg": ")" << main_app.get_admin_messages()["user_defined_city_unban_msg"] << "\",\n";
  config_file << R"("user_defined_enable_city_ban_feature_msg": ")" << main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] << "\",\n";
  config_file << R"("user_defined_disable_city_ban_feature_msg": ")" << main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] << "\",\n";
  config_file << R"("user_defined_country_ban_msg": ")" << main_app.get_admin_messages()["user_defined_country_ban_msg"] << "\",\n";
  config_file << R"("user_defined_country_unban_msg": ")" << main_app.get_admin_messages()["user_defined_country_unban_msg"] << "\",\n";
  config_file << R"("user_defined_enable_country_ban_feature_msg": ")" << main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] << "\",\n";
  config_file << R"("user_defined_disable_country_ban_feature_msg": ")" << main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] << "\",\n";
  config_file << R"("user_defined_protect_ip_address_message": ")" << main_app.get_admin_messages()["user_defined_protect_ip_address_message"] << "\",\n";
  config_file << R"("user_defined_unprotect_ip_address_message": ")" << main_app.get_admin_messages()["user_defined_unprotect_ip_address_message"] << "\",\n";
  config_file << R"("user_defined_protect_ip_address_range_message": ")" << main_app.get_admin_messages()["user_defined_protect_ip_address_range_message"] << "\",\n";
  config_file << R"("user_defined_unprotect_ip_address_range_message": ")" << main_app.get_admin_messages()["user_defined_unprotect_ip_address_range_message"] << "\",\n";
  config_file << R"("user_defined_protect_city_message": ")" << main_app.get_admin_messages()["user_defined_protect_city_message"] << "\",\n";
  config_file << R"("user_defined_unprotect_city_message": ")" << main_app.get_admin_messages()["user_defined_unprotect_city_message"] << "\",\n";
  config_file << R"("user_defined_protect_country_message": ")" << main_app.get_admin_messages()["user_defined_protect_country_message"] << "\",\n";
  config_file << R"("user_defined_unprotect_country_message": ")" << main_app.get_admin_messages()["user_defined_unprotect_country_message"] << "\",\n";
  config_file << R"("automatic_remove_temp_ban_msg": ")" << main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] << "\",\n";
  config_file << R"("automatic_kick_temp_ban_msg": ")" << main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] << "\",\n";
  config_file << R"("automatic_kick_ip_ban_msg": ")" << main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] << "\",\n";
  config_file << R"("automatic_kick_ip_address_range_ban_msg":  ")" << main_app.get_admin_messages()["automatic_kick_ip_address_range_ban_msg"] << "\",\n";
  config_file << R"("automatic_kick_city_ban_msg": ")" << main_app.get_admin_messages()["automatic_kick_city_ban_msg"] << "\",\n";
  config_file << R"("automatic_kick_country_ban_msg": ")" << main_app.get_admin_messages()["automatic_kick_country_ban_msg"] << "\",\n";
  config_file << R"("current_match_info": ")" << main_app.get_game_server().get_current_match_info() << "\",\n";
  config_file << "\"use_different_background_colors_for_even_and_odd_lines\": " << (main_app.get_game_server().get_is_use_different_background_colors_for_even_and_odd_lines() ? "true" : "false") << ",\n";
  config_file << R"("odd_player_data_lines_bg_color": ")" << main_app.get_game_server().get_odd_player_data_lines_bg_color() << "\",\n";
  config_file << R"("even_player_data_lines_bg_color": ")" << main_app.get_game_server().get_even_player_data_lines_bg_color() << "\",\n";
  config_file << "\"use_different_foreground_colors_for_even_and_odd_lines\": " << (main_app.get_game_server().get_is_use_different_foreground_colors_for_even_and_odd_lines() ? "true" : "false") << ",\n";
  config_file << R"("odd_player_data_lines_fg_color": ")" << main_app.get_game_server().get_odd_player_data_lines_fg_color() << "\",\n";
  config_file << R"("even_player_data_lines_fg_color": ")" << main_app.get_game_server().get_even_player_data_lines_fg_color() << "\",\n";
  config_file << R"("full_map_name_color": ")" << main_app.get_game_server().get_full_map_name_color() << "\",\n";
  config_file << R"("rcon_map_name_color": ")" << main_app.get_game_server().get_rcon_map_name_color() << "\",\n";
  config_file << R"("full_game_type_color": ")" << main_app.get_game_server().get_full_gametype_name_color() << "\",\n";
  config_file << R"("rcon_game_type_color": ")" << main_app.get_game_server().get_rcon_gametype_name_color() << "\",\n";

  config_file << R"("online_players_count_color": ")" << main_app.get_game_server().get_online_players_count_color() << "\",\n";
  config_file << R"("offline_players_count_color": ")" << main_app.get_game_server().get_offline_players_count_color() << "\",\n";
  config_file << R"("border_line_color": ")" << main_app.get_game_server().get_border_line_color() << "\",\n";
  config_file << R"("header_player_pid_color": ")" << main_app.get_game_server().get_header_player_pid_color() << "\",\n";
  config_file << R"("data_player_pid_color": ")" << main_app.get_game_server().get_data_player_pid_color() << "\",\n";
  config_file << R"("header_player_score_color": ")" << main_app.get_game_server().get_header_player_score_color() << "\",\n";
  config_file << R"("data_player_score_color": ")" << main_app.get_game_server().get_data_player_score_color() << "\",\n";
  config_file << R"("header_player_ping_color": ")" << main_app.get_game_server().get_header_player_ping_color() << "\",\n";
  config_file << R"("data_player_ping_color": ")" << main_app.get_game_server().get_data_player_ping_color() << "\",\n";
  config_file << R"("header_player_name_color": ")" << main_app.get_game_server().get_header_player_name_color() << "\",\n";
  config_file << R"("header_player_ip_color": ")" << main_app.get_game_server().get_header_player_ip_color() << "\",\n";
  config_file << R"("data_player_ip_color": ")" << main_app.get_game_server().get_data_player_ip_color() << "\",\n";
  config_file << R"("header_player_geoinfo_color": ")" << main_app.get_game_server().get_header_player_geoinfo_color() << "\",\n";
  config_file << R"("data_player_geoinfo_color": ")" << main_app.get_game_server().get_data_player_geoinfo_color() << "\",\n";
  config_file << "\"draw_border_lines\": " << (main_app.get_is_draw_border_lines() ? "true" : "false") << ",\n";
  config_file << R"("ftp_download_site_ip_address": ")" << main_app.get_ftp_download_site_ip_address() << "\",\n";
  config_file << R"("ftp_download_folder_path": ")" << main_app.get_ftp_download_folder_path() << "\",\n";
  config_file << R"("ftp_download_file_pattern": ")" << main_app.get_ftp_download_file_pattern() << "\",\n";
  config_file << R"("plugins_geoIP_geo_dat_md5": ")" << main_app.get_plugins_geoIP_geo_dat_md5() << "\"\n";
  config_file << "}" << flush;
  return true;
}

bool check_ip_address_validity(string_view ip_address,
  unsigned long &ip_key)
{
  const auto parts = stl::helper::str_split(ip_address, ".", nullptr, split_on_whole_needle_t::yes);
  if (parts.size() < 4U)
    return false;

  const bool is_valid_ip_address =
    all_of(cbegin(parts), cend(parts), [](const string &part) {
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

bool check_ip_address_range_validity(const string &ip_address_range)
{
  static const regex ip_address_range_mask(R"(^\d{1,3}\.\d{1,3}\.(\d{1,3}|\*)\.(\d{1,3}|\*)$)");
  smatch matches;
  return regex_match(ip_address_range, matches, ip_address_range_mask);
}

void convert_guid_key_to_country_name(const vector<geoip_data> &geo_data,
  string_view player_ip,
  player_data &player_data)
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
        if (strstr(player_data.country_name, "Russia") != nullptr)
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
  const std::vector<player_data> &players,
  const bool count_color_codes,
  const size_t number_of_players_to_process) noexcept
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
  const std::vector<player_data> &players,
  const size_t number_of_players_to_process) noexcept
{
  if (0 == number_of_players_to_process)
    return 0;

  size_t max_geodata_info_length{ 18 };
  for (size_t i{}; i < number_of_players_to_process; ++i) {
    const size_t country_len{ len(players[i].country_name) };
    const size_t region_len{ len(players[i].region) };
    const size_t city_len{ len(players[i].city) };
    const size_t current_player_geodata_info_length = (country_len != 0 ? country_len : region_len) + city_len + 2;
    max_geodata_info_length = std::max(current_player_geodata_info_length, max_geodata_info_length);
  }

  return max_geodata_info_length;
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
    main_app.set_is_disable_automatic_kick_messages(json_resource["disable_automatic_kick_messages"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.set_is_disable_automatic_kick_messages(false);
  }

  if (json_resource["use_original_admin_messages"].exists()) {
    main_app.set_is_use_original_admin_messages(json_resource["use_original_admin_messages"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.set_is_use_original_admin_messages(true);
  }

  if (json_resource["user_defined_warn_msg"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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

  if (json_resource["user_defined_ip_address_range_ban_msg"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_ip_address_range_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_ip_address_range_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_ip_address_range_ban_msg"] = "^7Admin ^5{ADMINNAME} ^7has ^1banned IP address range ^5{IP_ADDRESS_RANGE}. ^3Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_ip_address_range_ban_msg"] = "^7Admin ^5{ADMINNAME} ^7has ^1banned IP address range ^5{IP_ADDRESS_RANGE}. ^3Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_city_ban_msg"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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

  if (json_resource["user_defined_protect_ip_address_message"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_protect_ip_address_message"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_protect_ip_address_message"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_protect_ip_address_message"] = "^1{ADMINNAME} ^7has protected ^1IP address ^7of {PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_protect_ip_address_message"] = "^1{ADMINNAME} ^7has protected ^1IP address ^7of {PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_unprotect_ip_address_message"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_unprotect_ip_address_message"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_unprotect_ip_address_message"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_unprotect_ip_address_message"] = "^1{ADMINNAME} ^7has removed a previously protected ^1IP address^7.{{br}}^5Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_unprotect_ip_address_message"] = "^1{ADMINNAME} ^7has removed a previously protected ^1IP address^7.{{br}}^5Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_protect_ip_address_range_message"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_protect_ip_address_range_message"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_protect_ip_address_range_message"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_protect_ip_address_range_message"] = "^1{ADMINNAME} ^7has protected ^1IP address range ^7of {PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_protect_ip_address_range_message"] = "^1{ADMINNAME} ^7has protected ^1IP address range ^7of {PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}";
  }

  if (json_resource["user_defined_unprotect_ip_address_range_message"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_unprotect_ip_address_range_message"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_unprotect_ip_address_range_message"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_unprotect_ip_address_range_message"] = "^1{ADMINNAME} ^7has removed a previously ^1protected IP address range^7.{{br}}^5Reason: ^1{REASON}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_unprotect_ip_address_range_message"] = "^1{ADMINNAME} ^7has removed a previously ^1protected IP address range^7.{{br}}^5Reason: ^1{REASON}";
  }


  if (json_resource["user_defined_protect_city_message"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_protect_city_message"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_protect_city_message"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_protect_city_message"] = "^1{ADMINNAME} ^7has protected ^1city {CITY_NAME} ^7of {PLAYERNAME}^7.";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_protect_city_message"] = "^1{ADMINNAME} ^7has protected ^1city {CITY_NAME} ^7of {PLAYERNAME}^7.";
  }

  if (json_resource["user_defined_unprotect_city_message"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_unprotect_city_message"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_unprotect_city_message"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_unprotect_city_message"] = "^1{ADMINNAME} ^7has removed a previously ^1protected city: {CITY_NAME}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_unprotect_city_message"] = "^1{ADMINNAME} ^7has removed a previously ^1protected city: {CITY_NAME}";
  }

  if (json_resource["user_defined_protect_country_message"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_protect_country_message"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_protect_country_message"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_protect_country_message"] = "^1{ADMINNAME} ^7has protected ^1country {COUNTRY_NAME} ^7of {PLAYERNAME}^7.";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_protect_country_message"] = "^1{ADMINNAME} ^7has protected ^1country {COUNTRY_NAME} ^7of {PLAYERNAME}^7.";
  }

  if (json_resource["user_defined_unprotect_country_message"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["user_defined_unprotect_country_message"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["user_defined_unprotect_country_message"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["user_defined_unprotect_country_message"] = "^1{ADMINNAME} ^7has removed a previously ^1protected country: {COUNTRY_NAME}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["user_defined_unprotect_country_message"] = "^1{ADMINNAME} ^7has removed a previously ^1protected country: {COUNTRY_NAME}";
  }

  if (json_resource["automatic_remove_temp_ban_msg"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["automatic_kick_temp_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] = "^1{BANNED_BY}: ^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}}^7Your temporary ban expires on ^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{TEMP_BAN_START_DATE}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] = "^1{BANNED_BY}: ^7Temporarily banned player {PLAYERNAME} ^7is being automatically ^1kicked.{{br}}^7Your temporary ban expires on ^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{TEMP_BAN_START_DATE}";
  }

  if (json_resource["automatic_kick_ip_ban_msg"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
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

  if (json_resource["automatic_kick_ip_address_range_ban_msg"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
      data_line = json_resource["automatic_kick_ip_address_range_ban_msg"].as_str();
      strip_leading_and_trailing_quotes(data_line);
      main_app.get_admin_messages()["automatic_kick_ip_address_range_ban_msg"] = std::move(data_line);
    } else {
      main_app.get_admin_messages()["automatic_kick_ip_address_range_ban_msg"] = "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an ^1IP address ^7from a previously ^1banned IP address range ^7is being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}";
    }
  } else {
    found_missing_config_setting = true;
    main_app.get_admin_messages()["automatic_kick_ip_address_range_ban_msg"] = "^1{ADMINNAME}: ^7Player {PLAYERNAME} ^7with an ^1IP address ^7from a previously ^1banned IP address range ^7is being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}";
  }

  if (json_resource["automatic_kick_city_ban_msg"].exists()) {
    if (!main_app.get_is_use_original_admin_messages()) {
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
    if (!main_app.get_is_use_original_admin_messages()) {
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
    main_app.get_game_server().set_even_player_data_lines_fg_color("^5");
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
    main_app.get_game_server().set_header_player_score_color("^4");
  }

  if (json_resource["data_player_score_color"].exists()) {
    data_line = json_resource["data_player_score_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_data_player_score_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_data_player_score_color("^4");
  }

  if (json_resource["header_player_ping_color"].exists()) {
    data_line = json_resource["header_player_ping_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_ping_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_ping_color("^4");
  }

  if (json_resource["data_player_ping_color"].exists()) {
    data_line = json_resource["data_player_ping_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_data_player_ping_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_data_player_ping_color("^4");
  }

  if (json_resource["header_player_name_color"].exists()) {
    data_line = json_resource["header_player_name_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_name_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_name_color("^4");
  }

  if (json_resource["header_player_ip_color"].exists()) {
    data_line = json_resource["header_player_ip_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_header_player_ip_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_header_player_ip_color("^4");
  }
  if (json_resource["data_player_ip_color"].exists()) {
    data_line = json_resource["data_player_ip_color"].as_str();
    strip_leading_and_trailing_quotes(data_line);
    main_app.get_game_server().set_data_player_ip_color(std::move(data_line));
  } else {
    found_missing_config_setting = true;
    main_app.get_game_server().set_data_player_ip_color("^4");
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
    main_app.get_game_server().set_data_player_geoinfo_color("^4");
  }

  if (json_resource["draw_border_lines"].exists()) {
    main_app.set_is_draw_border_lines(json_resource["draw_border_lines"].as<bool>());
  } else {
    found_missing_config_setting = true;
    main_app.set_is_draw_border_lines(true);
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
    main_app.set_ftp_download_file_pattern(R"(^_U_TinyRcon[\._-]?v?(\d{1,2}\.\d{1,2}\.\d{1,2}\.\d{1,2})\.exe$)");
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

  ifstream input_file1(main_app.get_banned_countries_list_file_path(), std::ios::in);
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

  ifstream input_file2(main_app.get_banned_cities_list_file_path(), std::ios::in);
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

void parse_protected_entries_file(const char *file_path, std::set<std::string> &protected_entries)
{
  lock_guard lg{ protect_banned_players_data };
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 1024 };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "^3Couldn't open file at specified path ^1("s + file_path + ")^3! "s + buffer
    };
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
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

void parse_tempbans_data_file(const char *file_path, std::vector<player_data> &temp_banned_players, std::unordered_map<std::string, player_data> &ip_to_temp_banned_player)
{
  string property_key, property_value;
  lock_guard lg{ protect_banned_players_data };
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 1024 };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "^3Couldn't open file at specified path ^1("s + file_path + ")^3! "s + buffer
    };
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
      print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    }
  } else {
    ip_to_temp_banned_player.clear();
    temp_banned_players.clear();
    string readData, information_about_protected_player;
    while (getline(input_file, readData)) {
      stl::helper::trim_in_place(readData, " \t\n");
      vector<string> parts{ stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no) };
      for (auto &part : parts)
        stl::helper::trim_in_place(part, " \t\n");
      if (parts.size() < 6)
        continue;
      if (!ip_to_temp_banned_player.contains(parts[0])) {
        player_data temp_banned_player_data{};
        strcpy_s(temp_banned_player_data.ip_address, std::size(temp_banned_player_data.ip_address), parts[0].c_str());
        strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name), parts[1].c_str());
        temp_banned_player_data.banned_start_time = stoll(parts[3]);
        const string converted_ban_date_and_time_info{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", temp_banned_player_data.banned_start_time) };
        strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time), converted_ban_date_and_time_info.c_str());
        temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
        temp_banned_player_data.reason = std::move(parts[5]);
        temp_banned_player_data.banned_by_user_name = (parts.size() >= 7) ? parts[6] : main_app.get_username();
        convert_guid_key_to_country_name(
          main_app.get_connection_manager().get_geoip_data(),
          temp_banned_player_data.ip_address,
          temp_banned_player_data);
        if (check_if_player_is_protected(temp_banned_player_data, "^7enable ^1tempban ^7for", information_about_protected_player)) {
          print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
          continue;
        }
        ip_to_temp_banned_player.emplace(temp_banned_player_data.ip_address, temp_banned_player_data);
        temp_banned_players.push_back(std::move(temp_banned_player_data));
      }
    }
  }
}

void parse_banned_ip_addresses_file(const char *file_path, std::vector<player_data> &banned_players, std::unordered_map<std::string, player_data> &ip_to_banned_player)
{
  string property_key, property_value;
  lock_guard lg{ protect_banned_players_data };
  ifstream input_file{ file_path };
  if (!input_file) {
    const size_t buffer_size{ 1024 };
    char buffer[buffer_size];
    strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
    string errorMessage{
      "^3Couldn't open file at specified path ^1("s + file_path + ")^3! "s + buffer
    };
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
      print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    }
  } else {
    ip_to_banned_player.clear();
    banned_players.clear();
    string readData, information_about_protected_player;
    while (getline(input_file, readData)) {
      stl::helper::trim_in_place(readData);
      vector<string> parts = stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
      for (auto &part : parts)
        stl::helper::trim_in_place(part);
      if (parts.size() < 5)
        continue;

      if (!ip_to_banned_player.contains(parts[0])) {
        player_data bannedPlayerData{};
        strcpy_s(bannedPlayerData.ip_address, std::size(bannedPlayerData.ip_address), parts[0].c_str());
        strcpy_s(bannedPlayerData.guid_key, std::size(bannedPlayerData.guid_key), parts[1].c_str());
        strcpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[2].c_str());
        bannedPlayerData.banned_start_time = get_number_of_seconds_from_date_and_time_string(parts[3]);
        const string converted_ban_date_and_time_info{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bannedPlayerData.banned_start_time) };
        strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time), converted_ban_date_and_time_info.c_str());
        bannedPlayerData.reason = std::move(parts[4]);
        bannedPlayerData.banned_by_user_name = (parts.size() >= 6) ? parts[5] : main_app.get_username();
        convert_guid_key_to_country_name(
          main_app.get_connection_manager().get_geoip_data(),
          bannedPlayerData.ip_address,
          bannedPlayerData);
        if (check_if_player_is_protected(bannedPlayerData, "^7enable ^1IP ban ^7for", information_about_protected_player)) {
          print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
          continue;
        }
        ip_to_banned_player.emplace(bannedPlayerData.ip_address, bannedPlayerData);
        banned_players.push_back(std::move(bannedPlayerData));
      }
    }

    sort(begin(banned_players), end(banned_players), [](const player_data &p1, const player_data &p2) {
      return p1.banned_start_time < p2.banned_start_time;
    });
  }
}


void parse_banned_ip_address_ranges_file(const char *file_path, std::vector<player_data> &banned_ip_address_ranges, std::unordered_map<std::string, player_data> &ip_address_range_to_banned_player)
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
    string readData, information_about_protected_player;
    while (getline(input_file, readData)) {
      stl::helper::trim_in_place(readData);
      vector<string> parts = stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
      for (auto &part : parts)
        stl::helper::trim_in_place(part);
      if (parts.size() < 5)
        continue;
      if (!ip_address_range_to_banned_player.contains(parts[0])) {
        player_data bannedPlayerData{};
        strcpy_s(bannedPlayerData.guid_key, std::size(bannedPlayerData.guid_key), parts[1].c_str());
        strcpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[2].c_str());
        bannedPlayerData.banned_start_time = get_number_of_seconds_from_date_and_time_string(parts[3]);
        const string converted_ban_date_and_time_info{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bannedPlayerData.banned_start_time) };
        strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time), converted_ban_date_and_time_info.c_str());
        bannedPlayerData.reason = std::move(parts[4]);
        bannedPlayerData.banned_by_user_name = (parts.size() >= 6) ? parts[5] : main_app.get_username();
        get_first_valid_ip_address_from_ip_address_range(parts[0], bannedPlayerData);

        if (check_if_player_is_protected(bannedPlayerData, "^7enable ^1IP address range ban ^7for", information_about_protected_player)) {
          print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
          continue;
        }
        strcpy_s(bannedPlayerData.ip_address, std::size(bannedPlayerData.ip_address), parts[0].c_str());
        ip_address_range_to_banned_player.emplace(parts[0], bannedPlayerData);
        banned_ip_address_ranges.push_back(std::move(bannedPlayerData));
      }
    }

    sort(begin(banned_ip_address_ranges), end(banned_ip_address_ranges), [](const player_data &p1, const player_data &p2) {
      return p1.banned_start_time < p2.banned_start_time;
    });
  }
}

void parse_banned_cities_file(const char *file_path, std::set<std::string> &banned_cities)
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
    print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    ofstream bannedIPsFileToWrite{ file_path };
    if (!bannedIPsFileToWrite) {
      strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
      errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
      print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
    }
  } else {
    const auto &protected_cities = main_app.get_game_server().get_protected_cities();
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
      } else {
        banned_cities.emplace(std::move(readData));
      }
    }
  }
}

void parse_banned_countries_file(const char *file_path, std::set<std::string> &banned_countries)
{
  string property_key, property_value;
  lock_guard lg{ protect_banned_players_data };
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
      } else {
        banned_countries.emplace(std::move(readData));
      }
    }
  }
}

void save_tempbans_to_file(const char *file_path, const std::vector<player_data> &temp_banned_players)
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

void save_banned_ip_entries_to_file(const char *file_path, const std::vector<player_data> &banned_ip_entries)
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

void save_banned_ip_address_range_entries_to_file(const char *file_path, const std::vector<player_data> &banned_ip_address_ranges)
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

bool temp_ban_player_ip_address(player_data &pd)
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
      main_app.get_connection_manager().get_geoip_data(),
      pd.ip_address,
      pd);
  }

  pd.banned_start_time = get_current_time_stamp();
  const string banned_date_time_str{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time) };
  strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
  pd.banned_by_user_name = main_app.get_username();

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


    temp_banned_data_file_to_write << pd.ip_address << '\\'
                                   << pd.player_name << '\\'
                                   << pd.banned_date_time << '\\'
                                   << pd.banned_start_time << '\\'
                                   << pd.ban_duration_in_hours << '\\'
                                   << remove_disallowed_character_in_string(pd.reason) << '\\'
                                   << main_app.get_username() << endl;
  } else {

    temp_banned_data_file_to_read << pd.ip_address << '\\'
                                  << pd.player_name << '\\'
                                  << pd.banned_date_time << '\\'
                                  << pd.banned_start_time << '\\'
                                  << pd.ban_duration_in_hours << '\\'
                                  << remove_disallowed_character_in_string(pd.reason) << '\\'
                                  << main_app.get_username() << endl;
  }

  main_app.get_game_server().add_ip_address_to_temp_banned_ip_addresses(
    pd.ip_address, pd);

  return true;
}

bool global_ban_player_ip_address(player_data &pd)
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
      main_app.get_connection_manager().get_geoip_data(),
      pd.ip_address,
      pd);
  }

  pd.banned_start_time = get_current_time_stamp();
  const string banned_date_time_str{ get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time) };
  strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
  pd.banned_by_user_name = main_app.get_username();

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

    bannedIPsFileToWrite << pd.ip_address << '\\'
                         << pd.guid_key << '\\'
                         << pd.player_name << '\\'
                         << pd.banned_date_time << '\\'
                         << remove_disallowed_character_in_string(pd.reason) << '\\'
                         << main_app.get_username() << endl;
  } else {
    bannedIPsFileToRead << pd.ip_address << '\\'
                        << pd.guid_key << '\\'
                        << pd.player_name << '\\'
                        << pd.banned_date_time << '\\'
                        << remove_disallowed_character_in_string(pd.reason) << '\\'
                        << main_app.get_username() << endl;
  }

  main_app.get_game_server().add_ip_address_to_banned_ip_addresses(
    pd.ip_address, pd);

  return true;
}

bool add_temporarily_banned_ip_address(player_data &pd, vector<player_data> &temp_banned_players_data, unordered_map<string, player_data> &ip_to_temp_banned_player_data)
{
  unsigned long guid_number{};
  if (!check_ip_address_validity(pd.ip_address, guid_number) || ip_to_temp_banned_player_data.contains(pd.ip_address))
    return false;

  ip_to_temp_banned_player_data.emplace(pd.ip_address, pd);
  temp_banned_players_data.push_back(std::move(pd));

  save_tempbans_to_file(tempbans_file_path, temp_banned_players_data);

  return true;
}

std::pair<bool, player_data> remove_temp_banned_ip_address(const std::string &ip_address, std::string &message, const bool is_automatic_temp_ban_remove, const bool is_report_public_message)
{
  lock_guard lg{ protect_banned_players_data };
  if (!main_app.get_game_server().get_temp_banned_ip_addresses_map().contains(
        ip_address)) {
    return { false, player_data{} };
  }

  auto &temp_banned_players = main_app.get_game_server().get_temp_banned_players_data();

  const auto found_iter = find_if(std::begin(temp_banned_players), std::end(temp_banned_players), [&ip_address](const player_data &p) {
    return ip_address == p.ip_address;
  });

  if (found_iter != std::end(temp_banned_players)) {
    if (is_report_public_message) {
      if (!is_automatic_temp_ban_remove) {
        const string buffer{ format("^7Admin ({}^7) has manually removed ^1temporarily banned IP address ^7for player {} ^7(reason: ^1{}^7)\n", main_app.get_username(), found_iter->player_name, remove_disallowed_character_in_string(found_iter->reason)) };
        message.assign(buffer);
      }

      rcon_say(message, true);
    }

    player_data pd{ std::move(main_app.get_game_server().get_temp_banned_ip_addresses_map().at(found_iter->ip_address)) };

    temp_banned_players.erase(remove_if(std::begin(temp_banned_players), std::end(temp_banned_players), [&ip_address](const player_data &p) {
      return ip_address == p.ip_address;
    }),
      std::end(temp_banned_players));

    main_app.get_game_server().get_temp_banned_ip_addresses_map().erase(pd.ip_address);

    save_tempbans_to_file(tempbans_file_path, temp_banned_players);

    return { true, pd };
  }

  return {
    true, player_data{}
  };
}


bool add_permanently_banned_ip_address(player_data &pd, vector<player_data> &banned_players_data, unordered_map<string, player_data> &ip_to_banned_player_data)
{
  lock_guard lg{ protect_banned_players_data };
  unsigned long guid_number{};
  if (!check_ip_address_validity(pd.ip_address, guid_number) || ip_to_banned_player_data.contains(pd.ip_address))
    return false;

  ip_to_banned_player_data.emplace(pd.ip_address, pd);
  banned_players_data.push_back(std::move(pd));

  save_banned_ip_entries_to_file(banned_ip_addresses_file_path, banned_players_data);

  return true;
}

bool add_permanently_banned_ip_address_range(player_data &pd, std::vector<player_data> &banned_ip_address_ranges_vector, std::unordered_map<std::string, player_data> &banned_ip_address_ranges_map)
{
  lock_guard lg{ protect_banned_players_data };
  const string banned_ip_range{ pd.ip_address };
  if (!banned_ip_range.ends_with(".*.*") && !banned_ip_range.ends_with(".*"))
    return false;
  if (banned_ip_address_ranges_map.contains(banned_ip_range))
    return false;

  banned_ip_address_ranges_map.emplace(banned_ip_range, pd);
  banned_ip_address_ranges_vector.push_back(std::move(pd));

  save_banned_ip_address_range_entries_to_file(ip_range_bans_file_path, banned_ip_address_ranges_vector);

  return true;
}

bool remove_permanently_banned_ip_address_range(player_data &pd, std::vector<player_data> &banned_ip_address_ranges_vector, std::unordered_map<std::string, player_data> &banned_ip_address_ranges_map)
{
  lock_guard lg{ protect_banned_players_data };
  const string banned_ip_range{ pd.ip_address };
  if (!banned_ip_range.ends_with(".*.*") && !banned_ip_range.ends_with(".*"))
    return false;
  if (!banned_ip_address_ranges_map.contains(banned_ip_range))
    return false;

  banned_ip_address_ranges_vector.erase(remove_if(std::begin(banned_ip_address_ranges_vector), std::end(banned_ip_address_ranges_vector), [&pd](const player_data &p) {
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
  lock_guard lg{ protect_banned_players_data };
  if (banned_cities.contains(city))
    return false;

  banned_cities.emplace(city);

  save_banned_cities_to_file(banned_cities_list_file_path, banned_cities);

  return true;
}

bool remove_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities)
{
  lock_guard lg{ protect_banned_players_data };
  if (!banned_cities.contains(city))
    return false;

  banned_cities.erase(city);

  save_banned_cities_to_file(banned_cities_list_file_path, banned_cities);

  return true;
}

bool add_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries)
{
  lock_guard lg{ protect_banned_players_data };
  if (banned_countries.contains(country))
    return false;

  banned_countries.emplace(country);

  save_banned_cities_to_file(banned_countries_list_file_path, banned_countries);

  return true;
}

bool remove_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries)
{
  lock_guard lg{ protect_banned_players_data };
  if (!banned_countries.contains(country))
    return false;

  banned_countries.erase(country);

  save_banned_cities_to_file(banned_countries_list_file_path, banned_countries);

  return true;
}

std::pair<bool, player_data> remove_permanently_banned_ip_address(std::string &ip_address, std::string &message, const bool is_report_public_message)
{
  int number{};
  const size_t no{ is_valid_decimal_whole_number(ip_address, number) ? static_cast<size_t>(number - 1) : std::string::npos };

  lock_guard lg{ protect_banned_players_data };

  auto &banned_players = main_app.get_game_server().get_banned_ip_addresses_vector();

  if (main_app.get_game_server().get_banned_ip_addresses_map().find(
        ip_address)
        == cend(main_app.get_game_server().get_banned_ip_addresses_map())
      && no >= banned_players.size()) {
    return { false, player_data{} };
  }

  if (no < banned_players.size()) {
    ip_address = banned_players[no].ip_address;
  }

  const auto found_iter = find_if(std::begin(banned_players), std::end(banned_players), [&ip_address](const player_data &p) {
    return ip_address == p.ip_address;
  });

  if (found_iter != std::end(banned_players)) {
    if (is_report_public_message) {
      const string buffer{ format("^7Admin ({}^7) has manually removed ^1permanently banned IP address ^7for player {} ^7(reason: ^1{}^7)\n", main_app.get_username().c_str(), found_iter->player_name, remove_disallowed_character_in_string(found_iter->reason)) };
      message.assign(buffer);
      rcon_say(message);
    }

    player_data pd{ std::move(main_app.get_game_server().get_banned_ip_addresses_map().at(found_iter->ip_address)) };

    banned_players.erase(remove_if(std::begin(banned_players), std::end(banned_players), [&ip_address](const player_data &p) {
      return ip_address == p.ip_address;
    }),
      std::end(banned_players));

    main_app.get_game_server().get_banned_ip_addresses_map().erase(pd.ip_address);

    save_banned_ip_entries_to_file(banned_ip_addresses_file_path, banned_players);

    return { true, pd };
  }

  return { true, player_data{} };
}

bool is_valid_decimal_whole_number(std::string_view str, int &number) noexcept
{
  if (str.empty()) return false;
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
 ^1!t ^1public message ^5-> ^2-> ^5sends ^1public message ^5to all logged in ^1admins.
 ^1!y ^3username ^1private message ^2-> ^5sends ^1private message ^5to ^1admin ^5whose username is ^1username.
 ^1!sort player_data asc|desc ^3-> !sorts players data (for example: !sort pid asc, !sort pid desc, !sort score asc,
   !sort score desc, !sort ping asc, !sort ping desc, !sort name asc, !sort name desc, !sort ip asc, !sort ip desc,
   !sort geo asc, !sort geo desc
 ^1!list user|rcon ^5-> shows a list of available user or rcon level commands that are available for admin to type
                    into the console program.
 ^1!ranges ^5-> displays data about all ^1IP address range bans.
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
 ^1!banrange 12| 111.122.133.* [reason] ^5-> ^3bans ^1IP address range ^3of player whose ^1pid ^3is equal to ^1'12'^3.
 ^1!unbanrange 111.122.133.*  [reason] ^5-> ^2removes previously banned IP address range ^1111.122.133.*^2.
 ^1!admins ^5-> ^3Displays information about ^1admins.
 ^1!protectip pid|ip_address [reason] ^5-> ^2protects player's ^1IP address ^2whose ^1'pid' ^2number or
  ^1'IP address' ^2is equal to specified ^1'pid' ^2or ^1'ip_address'^2, respectively.
 ^1!unprotectip pid|ip_address [reason] ^5-> ^3removes protected ^1IP address ^3of player whose ^1'pid' ^3number or
  ^1'IP address' ^3is equal to specified ^1'pid' ^3or ^1'ip_address'^3, respectively.
 ^1!protectiprange pid|ip_address_range [reason] ^5-> ^2protects player's ^1IP address range ^2whose ^1'pid' ^2number or
  ^1'IP address range' ^2is equal to specified ^1'pid' ^2or ^1'ip_address_range'^2, respectively.
 ^1!unprotectiprange pid|ip_address_range [reason] ^5-> ^3removes protected ^1IP address range ^3of player whose ^1'pid' ^3number 
   or ^1'IP address range' ^3is equal to specified ^1'pid' ^3or ^1'ip_address_range'^3, respectively.
 ^1!protectcity pid|city_name ^5-> ^2protects player's ^1city ^2whose ^1'pid' ^2number or ^1'city name' ^2is equal to 
  specified ^1'pid' ^2or ^1'city_name'^2, respectively.
 ^1!unprotectcity pid|city_name ^5-> ^3removes protected city of player whose ^1'pid' ^3number or ^1'city name' ^3is equal 
  to specified ^1'pid' ^3or ^1'city_name'^3, respectively.
 ^1!protectcountry pid|country_name ^5-> ^2protects player's country whose ^1'pid' ^2number or ^1'country name' ^2is equal to 
 specified ^1'pid' ^2or ^1'valid_ip_address'^2, respectively.
 ^1!unprotectcountry pid|country_name ^5-> ^3removes protected ^1IP address ^3of player whose ^1'pid' ^3number or ^1'country name' 
 ^3is equal to specified ^1'pid' ^3or ^1'country_name'^3, respectively.
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

string prepare_current_match_information()
{
  string current_match_info = main_app.get_game_server().get_current_match_info();

  size_t start = current_match_info.find("{MAP_FULL_NAME}");
  if (string::npos != start) {
    current_match_info.replace(start, len("{MAP_FULL_NAME}"), main_app.get_game_server().get_full_map_name_color() + get_full_map_name(main_app.get_game_server().get_current_map()));
  }

  start = current_match_info.find("{MAP_RCON_NAME}");
  if (string::npos != start) {
    current_match_info.replace(start, len("{MAP_RCON_NAME}"), main_app.get_game_server().get_rcon_map_name_color() + main_app.get_game_server().get_current_map());
  }

  start = current_match_info.find("{GAMETYPE_FULL_NAME}");
  if (string::npos != start) {
    current_match_info.replace(start, len("{GAMETYPE_FULL_NAME}"), main_app.get_game_server().get_full_gametype_name_color() + get_full_gametype_name(main_app.get_game_server().get_current_game_type()));
  }

  start = current_match_info.find("{GAMETYPE_RCON_NAME}");
  if (string::npos != start) {
    current_match_info.replace(start, len("{GAMETYPE_RCON_NAME}"), main_app.get_game_server().get_rcon_gametype_name_color() + main_app.get_game_server().get_current_game_type());
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

void display_online_admins_information()
{
  ostringstream oss;

  oss << "^2Online admins: ";

  if (!main_app.get_users().empty()) {

    for (auto &u : main_app.get_users()) {
      if (u->is_online) {
        oss << format("^7{} ^3(^7{}^3), ", u->user_name, u->player_name);
      }
    }
  }

  online_admins_information = oss.str();
  if (' ' == online_admins_information.back())
    online_admins_information.pop_back();
  if (',' == online_admins_information.back())
    online_admins_information.pop_back();

  Edit_SetText(app_handles.hwnd_online_admins_information, "");
  print_colored_text(app_handles.hwnd_online_admins_information, online_admins_information.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
  SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);
}

bool check_if_user_provided_argument_is_valid_for_specified_command(
  const char *cmd,
  const string &arg)
{
  int number{};
  if ((str_compare_i(cmd, "!k") == 0) || (str_compare_i(cmd, "!kick") == 0) || (str_compare_i(cmd, "clientkick") == 0) || (str_compare_i(cmd, "!tb") == 0) || (str_compare_i(cmd, "!tempban") == 0) || (str_compare_i(cmd, "tempbanclient") == 0) || (str_compare_i(cmd, "!b") == 0) || (str_compare_i(cmd, "!ban") == 0) || (str_compare_i(cmd, "banclient") == 0)) {
    return is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg);
  }

  if ((str_compare_i(cmd, "!gb") == 0) || (str_compare_i(cmd, "!globalban") == 0) || (str_compare_i(cmd, "!banip") == 0) || (str_compare_i(cmd, "!addip") == 0) || (str_compare_i(cmd, "!protectip") == 0) || (str_compare_i(cmd, "!unprotectip") == 0)) {
    unsigned long ip_key{};
    return (is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg)) || check_ip_address_validity(arg, ip_key);
  }

  if ((str_compare_i(cmd, "!br") == 0) || (str_compare_i(cmd, "!banrange") == 0) || (str_compare_i(cmd, "!protectiprange") == 0) || (str_compare_i(cmd, "!unprotectiprange") == 0)) {
    return (is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg)) || check_ip_address_range_validity(arg);
  }

  if ((str_compare_i(cmd, "!ubr") == 0) || (str_compare_i(cmd, "!unbanrange") == 0)) {
    return check_ip_address_range_validity(arg);
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

    if (start + 4 <= msg.length() && msg[start + 1] == '^' && (msg[start + 2] >= '0' && msg[start + 2] <= '9') && (msg[start + 3] >= '0' && msg[start + 3] <= '9') /*&& msg[start + 2] == msg[start + 3]*/) {
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
    main_app.get_game_server().get_temp_banned_ip_addresses_map();
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
          if (main_app.get_is_disable_automatic_kick_messages()) {
            message.clear();
          } else {
            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
            main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time);
            main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] = get_time_interval_info_string_for_seconds(ban_expires_time - now_in_seconds);
            main_app.get_tinyrcon_dict()["{REASON}"] = remove_disallowed_character_in_string(pd.reason);
            main_app.get_tinyrcon_dict()["{BANNED_BY}"] = remove_disallowed_character_in_string(pd.banned_by_user_name);
            build_tiny_rcon_message(message);
          }
          kick_player(online_player.pid, message);
          replace_br_with_new_line(message);
          const string inform_msg{ format("{}\\{}", main_app.get_username(), message) };
          main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));

        } else {
          string message{ main_app.get_automatic_remove_temp_ban_msg() };
          main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
          main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time);
          main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", ban_expires_time);
          main_app.get_tinyrcon_dict()["{REASON}"] = remove_disallowed_character_in_string(pd.reason);
          main_app.get_tinyrcon_dict()["{BANNED_BY}"] = remove_disallowed_character_in_string(pd.banned_by_user_name);
          build_tiny_rcon_message(message);
          remove_temp_banned_ip_address(online_player.ip_address, message, true);
          replace_br_with_new_line(message);
          const string inform_msg{ format("{}\\{}", main_app.get_username(), message) };
          main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
        }
      }
    }
  }
}

void check_for_banned_ip_addresses()
{
  const bool is_enable_automatic_connection_flood_ip_ban{ main_app.get_game_server().get_is_enable_automatic_connection_flood_ip_ban() };
  lock_guard lg{ protect_banned_players_data };
  const auto &banned_ip_addresses =
    main_app.get_game_server().get_banned_ip_addresses_map();
  const auto &protected_ip_address = main_app.get_game_server().get_protected_ip_addresses();
  const auto &banned_ip_address_ranges = main_app.get_game_server().get_banned_ip_address_ranges_map();
  const auto &protected_ip_address_ranges = main_app.get_game_server().get_protected_ip_address_ranges();
  const auto &banned_countries = main_app.get_game_server().get_banned_countries_set();
  const auto &protected_countries = main_app.get_game_server().get_protected_countries();
  const auto &banned_cities = main_app.get_game_server().get_banned_cities_set();
  const auto &protected_cities = main_app.get_game_server().get_protected_cities();
  auto &ip_address_frequency = main_app.get_game_server().get_ip_address_frequency();
  auto &players_data = main_app.get_game_server().get_players_data();
  string player_protected_message;
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    auto &online_player = players_data[i];
    const string narrow_ip_address_range{ get_narrow_ip_address_range_for_specified_ip_address(online_player.ip_address) };
    const string wide_ip_address_range{ get_wide_ip_address_range_for_specified_ip_address(online_player.ip_address) };
    const bool is_ip_protected{ protected_ip_address.contains(online_player.ip_address) };
    const bool is_ip_range_protected{ protected_ip_address_ranges.contains(narrow_ip_address_range) || protected_ip_address_ranges.contains(wide_ip_address_range) };
    const bool is_city_protected{ protected_cities.contains(online_player.city) };
    const bool is_country_protected{ protected_countries.contains(online_player.country_name) };

    if (is_ip_protected || is_ip_range_protected || is_city_protected || is_country_protected) continue;

    if (is_enable_automatic_connection_flood_ip_ban && ip_address_frequency[online_player.ip_address] >= main_app.get_game_server().get_minimum_number_of_connections_from_same_ip_for_automatic_ban()) {
      main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
      main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(online_player.pid);
      string reason{ "^1DoS attack" };
      specify_reason_for_player_pid(online_player.pid, reason);
      string public_message{ "^1{ADMINNAME}: ^5Tiny^6Rcon ^7has successfully automatically banned ^1IP address: "s + online_player.ip_address + " ^7| ^5Player name: ^7{PLAYERNAME} ^7| ^5Reason: ^1{REASON} ^7| ^5Date of ban: ^1{IP_BAN_DATE}\n"s };
      main_app.get_tinyrcon_dict()["{IP_BAN_DATE}"] = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}");
      main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
      build_tiny_rcon_message(public_message);
      rcon_say(public_message, true);
      string private_message{
        "^5Tiny^6Rcon ^2has successfully automatically executed command ^1!gb ^2on player ("s + get_player_information(online_player.pid) + "^3)\n"s
      };
      print_colored_text(app_handles.hwnd_re_messages_data, private_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      global_ban_player_ip_address(online_player);
      replace_br_with_new_line(public_message);
      replace_br_with_new_line(private_message);
      const string inform_message{ format("{}\\{}\\{}", main_app.get_username(), public_message, private_message) };
      main_app.add_message_to_queue(message_t("inform-message", inform_message, true));
    }

    if (banned_ip_addresses.contains(online_player.ip_address)) {
      string message{ main_app.get_automatic_kick_ip_ban_msg() };
      player_data pd{};
      if (main_app.get_is_disable_automatic_kick_messages()) {
        message.clear();
      } else {
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
        if (main_app.get_game_server().get_banned_player_data_for_ip_address(online_player.ip_address, &pd)) {
          main_app.get_tinyrcon_dict()["{IP_BAN_DATE}"] = pd.banned_date_time;
          pd.reason = remove_disallowed_character_in_string(pd.reason);
          main_app.get_tinyrcon_dict()["{REASON}"] = pd.reason;
        }
        build_tiny_rcon_message(message);
      }
      const string banned_player_information{ get_player_information(online_player.pid) };
      string msg{ format("^1Automatic kick ^5for previously ^1banned IP: {} ^5| Reason: ^1{}\n{}\n", online_player.ip_address, pd.reason, banned_player_information) };
      kick_player(online_player.pid, message);
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      replace_br_with_new_line(message);
      replace_br_with_new_line(msg);
      const string inform_message{ format("{}\\{}\\{}", main_app.get_username(), message, msg) };
      main_app.add_message_to_queue(message_t("inform-message", inform_message, true));
    }

    if (!banned_ip_address_ranges.empty()) {
      const bool is_short_ip_address_range_ban{ banned_ip_address_ranges.contains(narrow_ip_address_range) && !is_ip_range_protected && !is_city_protected && !is_country_protected };
      const bool is_wider_ip_address_range_ban{ banned_ip_address_ranges.contains(wide_ip_address_range) && !is_ip_range_protected && !is_city_protected && !is_country_protected };

      if (is_short_ip_address_range_ban || is_wider_ip_address_range_ban) {
        string public_message{ main_app.get_automatic_kick_ip_address_range_ban_msg() };
        player_data pd{};
        if (main_app.get_is_disable_automatic_kick_messages()) {
          public_message.clear();
        } else {
          main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
          if (main_app.get_game_server().get_banned_player_data_for_ip_address_range(online_player.ip_address, &pd)) {
            main_app.get_tinyrcon_dict()["{IP_BAN_DATE}"] = pd.banned_date_time;
            pd.reason = remove_disallowed_character_in_string(pd.reason);
            main_app.get_tinyrcon_dict()["{REASON}"] = pd.reason;
          }
          build_tiny_rcon_message(public_message);
        }
        const string banned_player_information{ get_player_information(online_player.pid) };
        string private_message{ format("^1Automatic kick ^5for player with an ^1IP address ^5from a previously ^1banned IP address range: {}\n ^5| Reason: ^1{}\n{}\n", (is_short_ip_address_range_ban ? narrow_ip_address_range : wide_ip_address_range), pd.reason, banned_player_information) };
        print_colored_text(app_handles.hwnd_re_messages_data, private_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        kick_player(online_player.pid, public_message);
        replace_br_with_new_line(public_message);
        replace_br_with_new_line(private_message);
        const string inform_message{ format("{}\\{}\\{}", main_app.get_username(), public_message, private_message) };
        main_app.add_message_to_queue(message_t("inform-message", inform_message, true));
      }
    }

    if (main_app.get_game_server().get_is_automatic_city_kick_enabled() && !banned_cities.empty()) {
      if (banned_cities.contains(online_player.city)) {
        string message{ main_app.get_automatic_kick_city_ban_msg() };
        if (main_app.get_is_disable_automatic_kick_messages()) {
          message.clear();
        } else {
          main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
          main_app.get_tinyrcon_dict()["{CITY_NAME}"] = online_player.city;
          build_tiny_rcon_message(message);
        }
        const string banned_player_information{ get_player_information(online_player.pid) };
        string msg{
          format("^1Automatic kick ^5for player ^7{} ^5with a previously ^1banned city name: {}\n{}\n", online_player.player_name, online_player.city, banned_player_information)
        };
        print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        kick_player(online_player.pid, message);
        replace_br_with_new_line(message);
        replace_br_with_new_line(msg);
        const string inform_message{ format("{}\\{}\\{}", main_app.get_username(), message, msg) };
        main_app.add_message_to_queue(message_t("inform-message", inform_message, true));
      }
    }

    if (main_app.get_game_server().get_is_automatic_country_kick_enabled() && !banned_countries.empty()) {
      if (banned_countries.contains(online_player.country_name)) {
        string message{ main_app.get_automatic_kick_country_ban_msg() };
        if (main_app.get_is_disable_automatic_kick_messages()) {
          message.clear();
        } else {
          main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
          main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = online_player.country_name;
          build_tiny_rcon_message(message);
        }
        const string banned_player_information{ get_player_information(online_player.pid) };
        string msg{
          format("^1Automatic kick ^5for player ^7{} ^5with a previously ^1banned country name: {}\n{}\n", online_player.player_name, online_player.country_name, banned_player_information)
        };
        kick_player(online_player.pid, message);
        print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        replace_br_with_new_line(message);
        replace_br_with_new_line(msg);
        const string inform_message{ format("{}\\{}\\{}", main_app.get_username(), message, msg) };
        main_app.add_message_to_queue(message_t("inform-message", inform_message, true));
      }
    }
  }
}

bool check_if_player_is_protected(const player_data &online_player, const char *admin_command, string &message)
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
      message.assign(format("^3Discarded ^1{} ^3player ^7{} ^3whose IP address ^1{} ^3is protected!\n^5Please, remove their ^1protected IP address ^5first.", admin_command, online_player.player_name, online_player.ip_address));
      return true;
    }

    if (is_ip_range_protected) {
      message.assign(format("^3Discarded ^1{} ^3player ^7{} ^3whose IP address range ^1{} ^3is protected!\n^5Please, remove their ^1protected IP address range ^5first.", admin_command, online_player.player_name, main_app.get_game_server().get_protected_ip_address_ranges().contains(narrow_ip_address_range) ? narrow_ip_address_range : wide_ip_address_range));
      return true;
    }
  }
  const bool is_city_protected{ len(online_player.city) != 0 && main_app.get_game_server().get_protected_cities().contains(online_player.city) };
  const bool is_country_protected{ len(online_player.country_name) != 0 && main_app.get_game_server().get_protected_countries().contains(online_player.country_name) };


  if (is_city_protected) {
    message.assign(format("^3Discarded ^1{} ^3player ^7{} ^3whose city ^1{} ^3is protected!\n^5Please, remove their ^1protected city ^5first.", admin_command, online_player.player_name, online_player.city));
    return true;
  }

  if (is_country_protected) {

    message.assign(format("^3Discarded ^1{} ^3player ^7{} ^3whose country ^1{} ^3is protected!\n^5Please, remove their ^1protected country ^5first.", admin_command, online_player.player_name, online_player.country_name));
    return true;
  }

  return false;
}

void kick_player(const int pid, string &custom_message)
{
  static char buffer[128];
  static string reply;

  const auto &pd = get_player_data_for_pid(pid);
  int ping{};
  const string ping_str{ stl::helper::trim(pd.ping) };
  const bool is_send_public_message_to_server{ is_valid_decimal_whole_number(ping_str, ping) && ping != -1 && ping != 999 };
  if (!is_send_public_message_to_server) {
    const string inform_msg{ format("^5Skipping sending indentical ^1public message ^5to the server because player ^7{}\n ^5has already been ^1kicked.\n", pd.player_name) };
    print_colored_text(app_handles.hwnd_re_messages_data, inform_msg.c_str());
  } else if (!custom_message.empty()) {
    string message{ custom_message };
    rcon_say(message);
  }

  (void)snprintf(buffer, std::size(buffer), "clientkick %d", pid);
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
  (void)snprintf(buffer, std::size(buffer), "banclient %d", pid);
  main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
  kick_player(pid, custom_message);
}

void say_message(const char *message)
{
  static char buffer[256];
  static string reply;
  (void)snprintf(buffer, std::size(buffer), "say \"%s\"", message);
  main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
}

void rcon_say(string &msg, const bool is_print_to_rich_edit_messages_box)
{
  using namespace stl::helper;
  str_replace_all(msg, "{{br}}", "\n");
  str_replace_all(msg, "\\", "|");
  str_replace_all(msg, "/", "|");

  msg = word_wrap(msg.c_str(), 140);
  const auto lines = str_split(msg, "\n", nullptr, split_on_whole_needle_t::yes);
  for (const auto &line : lines) {
    say_message(line.c_str());
    if (is_print_to_rich_edit_messages_box) {
      print_colored_text(app_handles.hwnd_re_messages_data, line.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
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

void tell_message(const char *message, const int pid)
{
  using namespace stl::helper;
  string msg{ message };
  str_replace_all(msg, "{{br}}", "\n");
  str_replace_all(msg, "\\", "|");
  str_replace_all(msg, "/", "|");

  msg = word_wrap(msg.c_str(), 140);
  const auto lines = str_split(msg, "\n", nullptr, split_on_whole_needle_t::yes);

  static char buffer[256]{};
  static string reply;
  for (const auto &line : lines) {
    (void)snprintf(buffer, std::size(buffer), "tell %d \"%s\"", pid, line.c_str());
    main_app.get_connection_manager().send_and_receive_rcon_data(buffer, reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
  }
}

void process_user_input(std::string &user_input)
{
  trim_in_place(user_input);
  if (!user_input.empty()) {

    auto command_parts = str_split(user_input, " \t\n\f\v", "", split_on_whole_needle_t::no);

    if (!command_parts.empty()) {
      to_lower_case_in_place(command_parts[0]);
      commands_history.emplace_back(user_input);
      commands_history_index = commands_history.size() - 1;
      if (rcon_status_commands.contains(command_parts[0])) {
        initiate_sending_rcon_status_command_now();
      } else if (command_parts[0] == "q" || command_parts[0] == "!q" || command_parts[0] == "!quit" || command_parts[0] == "e" || command_parts[0] == "!e" || command_parts[0] == "exit") {
        user_input = "q";
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
  if (!user_cmd.empty() && main_app.get_command_handlers().contains(user_cmd[0])) {
    const string re_msg{ format("^2Executing user command: ^5{}\n", str_join(user_cmd, " ")) };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    main_app.get_command_handlers().at(user_cmd[0])(user_cmd);
  }
}

void process_rcon_command(const std::vector<string> &rcon_cmd)
{
  if (!rcon_cmd.empty() && main_app.get_command_handlers().contains(rcon_cmd[0])) {
    const string re_msg{ format("^2Executing rcon command: ^5{}\n", str_join(rcon_cmd, " ")) };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    main_app.get_command_handlers().at(rcon_cmd[0])(rcon_cmd);
  }
}

volatile bool should_program_terminate(const string &user_input) noexcept
{
  if (user_input.empty())
    return is_terminate_program.load();
  is_terminate_program.store(user_input == "q" || user_input == "!q" || user_input == "!quit" || user_input == "quit" || user_input == "e" || user_input == "!e" || user_input == "exit" || user_input == "!exit");
  return is_terminate_program.load();
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
    if (main_app.get_game_server().get_is_connection_settings_valid()) {
      std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
        unsigned long ip_key1{}, ip_key2{};
        if (!check_ip_address_validity(pl1.ip_address, ip_key1))
          return true;
        if (!check_ip_address_validity(pl2.ip_address, ip_key2))
          return false;
        return ip_key1 < ip_key2;
      });
    }
    break;

  case sort_type::ip_desc:
    if (main_app.get_game_server().get_is_connection_settings_valid()) {
      std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
        unsigned long ip_key1{}, ip_key2{};
        if (!check_ip_address_validity(pl1.ip_address, ip_key1))
          return false;
        if (!check_ip_address_validity(pl2.ip_address, ip_key2))
          return true;
        return ip_key1 > ip_key2;
      });
    }
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
    if (main_app.get_game_server().get_is_connection_settings_valid()) {
      std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
        char buffer1[256];
        (void)snprintf(buffer1, std::size(buffer1), "%s, %s", (len(pl1.country_name) > 0 ? pl1.country_name : pl1.region), pl1.city);
        char buffer2[256];
        (void)snprintf(buffer2, std::size(buffer2), "%s, %s", (len(pl2.country_name) > 0 ? pl2.country_name : pl2.region), pl2.city);
        string pl1_cleaned_geo{ buffer1 };
        string pl2_cleaned_geo{ buffer2 };
        to_lower_case_in_place(pl1_cleaned_geo);
        to_lower_case_in_place(pl2_cleaned_geo);
        return pl1_cleaned_geo < pl2_cleaned_geo;
      });
    }
    break;

  case sort_type::geo_desc:
    if (main_app.get_game_server().get_is_connection_settings_valid()) {
      std::sort(std::begin(players_data), std::begin(players_data) + number_of_players, [](const player_data &pl1, const player_data &pl2) {
        char buffer1[256];
        (void)snprintf(buffer1, std::size(buffer1), "%s, %s", (len(pl1.country_name) > 0 ? pl1.country_name : pl1.region), pl1.city);
        char buffer2[256];
        (void)snprintf(buffer2, std::size(buffer2), "%s, %s", (len(pl2.country_name) > 0 ? pl2.country_name : pl2.region), pl2.city);
        string pl1_cleaned_geo{ buffer1 };
        string pl2_cleaned_geo{ buffer2 };
        to_lower_case_in_place(pl1_cleaned_geo);
        to_lower_case_in_place(pl2_cleaned_geo);
        return pl2_cleaned_geo < pl1_cleaned_geo;
      });
    }
    break;

  case sort_type::unknown:
    break;
  }
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
      << " | " << left << setw(20) << "Tempban issued on"
      << " | " << left << setw(20) << "Tempban expires on"
      << " | " << left << setw(25) << "Reason"
      << " | " << left << setw(32) << "Banned by admin"
      << "|";
  if (is_save_data_to_log_file) {
    log << left << setw(15) << "IP address"
        << " | " << left << setw(longest_name_length)
        << "Player name"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Tempban issued on"
        << " | " << left << setw(20) << "Tempban expires on"
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
      const char *next_color{ is_first_color ? "^5" : "^3" };
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
      const char *next_color{ is_first_color ? "^3" : "^3" };
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

void display_protected_entries(const char *table_title, const std::set<std::string> &protected_entries, const bool is_save_data_to_log_file)
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
  if (is_save_data_to_log_file) {
    log << "\n"
        << decoration_line << "\n"
        << "| ";
  }
  oss << left << setw(longest_name_length) << table_title << " |";
  if (is_save_data_to_log_file) {
    log << left << setw(longest_name_length) << table_title << " |";
  }
  oss << "^5\n"
      << decoration_line << "\n";
  if (is_save_data_to_log_file) {
    log << "\n"
        << decoration_line << "\n";
  }
  if (protected_entries.empty()) {
    const size_t message_len = stl::helper::len("| There are no protected entries.");
    oss << "^5| ^3There are no protected entries.";
    if (is_save_data_to_log_file) {
      log << "| There are no protected entries.";
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
      if (is_save_data_to_log_file) {
        if (printed_name_char_count2 < longest_name_length) {
          log << left << setw(longest_name_length) << entry + string(longest_name_length - printed_name_char_count2, ' ');
        } else {
          log << left << setw(longest_name_length) << entry;
        }
      }
      oss << " ^5|\n";
      if (is_save_data_to_log_file) {
        log << " |\n";
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
      const char *next_color{ is_first_color ? "^5" : "^3" };
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
      const char *next_color{ is_first_color ? "^3" : "^3" };
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
  print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
}

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

void export_geoip_data(const vector<geoip_data> &geo_data, const char *file_path) noexcept
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

  write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
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

    print_colored_text(app_handles.hwnd_re_messages_data, correct_cmd_usage.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    return false;
  }

  if (command[0] == "!config") {

    if (command[1] == "rcon") {
      const string &new_rcon{ command[2] };
      if (command[2].empty() || main_app.get_game_server().get_rcon_password() == new_rcon) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3The provided ^1rcon password ^3is too short or it is the same as\n the currently used ^1rcon password^3!\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        return false;
      }
      const string re_msg{ "^2You have successfully changed the ^1rcon password ^2to \"^5"s + command[2] + "^2\"\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      main_app.get_game_server().set_rcon_password(new_rcon);
      initiate_sending_rcon_status_command_now();

    } else if (command[1] == "private") {
      const string &new_private_password{ command[2] };
      if (command[2].empty() || main_app.get_game_server().get_private_slot_password() == new_private_password) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3The provided ^1private password ^3is too short or it is the same\n as the currently used ^1private password^3!\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        return false;
      }
      const string re_msg{
        "^2You have successfully changed the ^1private password ^2to \"^5"s + command[2] + "^2\"\n"s
      };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      main_app.get_game_server().set_private_slot_password(new_private_password);

    } else if (command[1] == "address") {
      const size_t colon_pos{ command[2].find(':') };
      if (colon_pos == string::npos)
        return false;
      const string ip{ command[2].substr(0, colon_pos) };
      const uint_least16_t port{ static_cast<uint_least16_t>(stoul(command[2].substr(colon_pos + 1))) };
      unsigned long ip_key{};
      if (!check_ip_address_validity(ip, ip_key) || (ip == main_app.get_game_server().get_server_ip_address() && port == main_app.get_game_server().get_server_port()))
        return false;
      const string re_msg{ "^2You have successfully changed the ^1game server address ^2to ^5"s + command[2] + "^2\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      main_app.get_game_server().set_server_ip_address(ip);
      main_app.get_game_server().set_server_port(port);
      initiate_sending_rcon_status_command_now();
    } else if (command[1] == "name") {
      if (command[2].length() < 3U) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3The provided ^1user name ^3is too short.\n It has to be at least ^13 characters ^3long!\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        return false;
      }
      const string re_msg{ "^2You have successfully changed your ^1user name ^2to ^5"s + command[2] + "^2\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
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

      print_colored_text(app_handles.hwnd_re_messages_data, correct_cmd_usage.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return false;
    }
  }

  write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
  return true;
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

std::string get_player_information(const int pid, const bool is_every_property_on_new_line)
{
  char buffer[512];
  const auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    if (pid == players_data[i].pid) {
      const auto &p = players_data[i];
      (void)snprintf(buffer, std::size(buffer), "^3Player name: ^7%s %s ^3PID: ^1%d %s ^3GUID: ^1%s %s ^3Score: ^5%d %s ^3Ping: ^5%s\n ^3IP: ^5%s %s ^3Country, region, city: ^5%s, %s, %s", p.player_name, (is_every_property_on_new_line ? "\n" : "^3|"), p.pid, (is_every_property_on_new_line ? "\n" : "^3|"), p.guid_key, (is_every_property_on_new_line ? "\n" : "^3|"), p.score, (is_every_property_on_new_line ? "\n" : "^3|"), p.ping, p.ip_address, (is_every_property_on_new_line ? "\n" : "^3|"), p.country_name, p.region, p.city);
      return buffer;
    }
  }

  (void)snprintf(buffer, std::size(buffer), "^3Player name: ^5n/a %s ^1PID: ^1%d %s ^3Score: ^5n/a %s ^3Ping: ^5n/a\n ^3IP: ^5n/a %s ^3Country, region, city: ^5n/a", (is_every_property_on_new_line ? "\n" : "^3|"), pid, (is_every_property_on_new_line ? "\n" : "^3|"), (is_every_property_on_new_line ? "\n" : "^3|"), (is_every_property_on_new_line ? "\n" : "^3|"));
  return buffer;
}

std::string get_player_information_for_player(player_data &p)
{
  char buffer[512];
  if (strlen(p.country_name) == 0 || strcmp(p.country_name, "n/a") == 0) {
    convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), p.ip_address, p);
  }

  (void)snprintf(buffer, std::size(buffer), "^3Player name: ^7%s ^3| ^1PID: ^1%d ^3| ^3Score: ^5%d ^3| ^3Ping: ^5%s\n ^3GUID: ^5%s ^3|IP: ^5%s ^3| Country, region, city: ^5%s, %s, %s", p.player_name, p.pid, p.score, p.ping, p.guid_key, p.ip_address, p.country_name, p.region, p.city);
  return buffer;
}

bool specify_reason_for_player_pid(const int pid, const std::string &reason)
{
  auto &players_data = main_app.get_game_server().get_players_data();
  for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
    if (pid == players_data[i].pid) {
      players_data[i].reason = remove_disallowed_character_in_string(reason);
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
  const auto &full_gametype_names = get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());
  const string &full_map_name = get_full_map_name(rcon_map_name);

  string public_msg{ is_change_game_type ? format("^7{} ^7has changed map to ^1{} ^3({}^3)", main_app.get_username(), full_map_name, full_gametype_names.contains(game_type) ? get_full_gametype_name(game_type) : game_type) : format("^7{} ^7has changed map to ^1{}", main_app.get_username(), full_map_name) };
  const string admin_msg{ format("{}\\{}", main_app.get_username(), public_msg) };
  main_app.add_message_to_queue(message_t("public-message", admin_msg, true));
  rcon_say(public_msg, true);

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

  BROWSEINFO bi{};
  bi.lpszTitle = user_info;
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
  bi.lpfn = BrowseCallbackProc;
  bi.lParam = reinterpret_cast<LPARAM>(saved_path);

  LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

  if (pidl != nullptr) {
    // get the name of the folder and put it in path
    SHGetPathFromIDList(pidl, path);

    // free memory used
    IMalloc *imalloc = nullptr;
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
    R"(SOFTWARE\Wow6432Node\Activision\Call of Duty)",
    "SOFTWARE\\Activision\\Call of Duty(R)",
    R"(SOFTWARE\WOW6432Node\Activision\Call of Duty(R))",
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

  LRESULT status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Valve\Steam\Apps\2620)", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {
    cch = sizeof(is_steam_game_installed);
    status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

    if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
      HKEY steam_installation_key{};
      status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
      if (status == ERROR_SUCCESS) {
        cch = sizeof(install_path);
        status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          string steam_exe_path{ install_path };
          replace_forward_slash_with_backward_slash(steam_exe_path);
          if (check_if_file_path_exists(steam_exe_path.c_str())) {

            (void)snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 2620", steam_exe_path.c_str());

            found = true;
            char buffer[1024];
            (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty (^4Steam version^3) ^2game's launch command: ^5%s\n", exe_file_path);
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

    status = RegOpenKeyEx(HKEY_CURRENT_USER, R"(SOFTWARE\Valve\Steam\Apps\2620)", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

      if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
        HKEY steam_installation_key{};
        status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
        if (status == ERROR_SUCCESS) {
          cch = sizeof(install_path);
          status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

          if (status == ERROR_SUCCESS) {

            string steam_exe_path{ install_path };
            replace_forward_slash_with_backward_slash(steam_exe_path);
            if (check_if_file_path_exists(steam_exe_path.c_str())) {

              (void)snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 2620", steam_exe_path.c_str());

              found = true;
              char buffer[1024];
              (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty (^4Steam version^3) ^2game's launch command: ^5%s\n", exe_file_path);
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

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

      if (status == ERROR_SUCCESS) {

        WIN32_FIND_DATA find_data{};

        remove_dir_path_sep_char(install_path);

        (void)snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", install_path);

        HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

        if (search_handle != INVALID_HANDLE_VALUE) {

          found = true;
          found_reg_location = *def_game_reg_key;
          char buffer[1024];
          (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty ^2game's launch command: ^5%s\n", exe_file_path);
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

      status = RegOpenKeyEx(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

      if (status == ERROR_SUCCESS) {

        status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          WIN32_FIND_DATA find_data{};

          remove_dir_path_sep_char(install_path);

          (void)snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", install_path);

          HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

          if (search_handle != INVALID_HANDLE_VALUE) {

            found = true;
            found_reg_location = *def_game_reg_key;
            char buffer[1024];
            (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty ^2game's launch command: ^5%s\n", exe_file_path);
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

    str_copy(install_path, "C:\\");
    found_reg_location = nullptr;

    static char msgbuff[1024];
    (void)snprintf(msgbuff, std::size(msgbuff), "Please, select your Call of Duty game's installation folder and click OK.");

    const char *cod1_game_path = BrowseFolder(install_path, msgbuff);

    if (lstrcmp(cod1_game_path, "") != 0 && lstrcmp(cod1_game_path, "C:\\") != 0) {
      (void)snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", cod1_game_path);
    }
  }

  if (!str_contains(exe_file_path, "-applaunch 2620")) {
    string exe_path{ exe_file_path };
    replace_forward_slash_with_backward_slash(exe_path);
    str_copy(exe_file_path, exe_path.c_str());
  }

  if (!check_if_file_path_exists(exe_file_path) && !str_contains(exe_file_path, "-applaunch 2620")) {
    main_app.set_codmp_exe_path("");
  } else {
    main_app.set_codmp_exe_path(exe_file_path);
    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
  }

  return exe_file_path;
}

bool check_if_call_of_duty_1_game_is_running() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[512];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));
      CloseHandle(ph);
      if (n == 0) continue;
      const string module_base_name{ buffer };
      if (str_index_of(module_base_name, "codmp.exe", 0, true) != string::npos)
        return true;
    }

  DWORD cch{};
  HKEY game_installation_reg_key;

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_running{};
  cch = sizeof(is_steam_game_running);

  auto status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 2620 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  is_steam_game_running = 0;
  cch = sizeof(is_steam_game_running);

  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

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
    R"(SOFTWARE\WOW6432Node\Activision\Call of Duty 2)",
    "SOFTWARE\\Activision\\Call of Duty(R) 2",
    R"(SOFTWARE\WOW6432Node\Activision\Call of Duty(R) 2)",
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

  LRESULT status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Valve\Steam\Apps\2630)", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {
    cch = sizeof(is_steam_game_installed);
    status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

    if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
      HKEY steam_installation_key{};
      status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
      if (status == ERROR_SUCCESS) {
        cch = sizeof(install_path);
        status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          string steam_exe_path{ install_path };
          replace_forward_slash_with_backward_slash(steam_exe_path);
          if (check_if_file_path_exists(steam_exe_path.c_str())) {

            (void)snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 2630", steam_exe_path.c_str());

            found = true;
            char buffer[1024];
            (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 2 (^4Steam version^3) ^2game's launch command: ^5%s\n", exe_file_path);
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

    status = RegOpenKeyEx(HKEY_CURRENT_USER, R"(SOFTWARE\Valve\Steam\Apps\2630)", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

      if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
        HKEY steam_installation_key{};
        status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
        if (status == ERROR_SUCCESS) {
          cch = sizeof(install_path);
          status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

          if (status == ERROR_SUCCESS) {

            string steam_exe_path{ install_path };
            replace_forward_slash_with_backward_slash(steam_exe_path);
            if (check_if_file_path_exists(steam_exe_path.c_str())) {

              (void)snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 2630", steam_exe_path.c_str());

              found = true;
              char buffer[1024];
              (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 2 (^4Steam version^3) ^2game's launch command: ^5%s\n", exe_file_path);
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

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

      if (status == ERROR_SUCCESS) {
        WIN32_FIND_DATA find_data{};
        remove_dir_path_sep_char(install_path);
        (void)snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", install_path);

        HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

        if (search_handle != INVALID_HANDLE_VALUE) {

          found = true;
          found_reg_location = *def_game_reg_key;
          char buffer[1024];
          (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 2 ^2game's launch command: ^5%s\n", exe_file_path);
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

      status = RegOpenKeyEx(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

      if (status == ERROR_SUCCESS) {

        status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          WIN32_FIND_DATA find_data{};
          remove_dir_path_sep_char(install_path);
          (void)snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", install_path);
          HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

          if (search_handle != INVALID_HANDLE_VALUE) {

            found = true;
            found_reg_location = *def_game_reg_key;
            char buffer[1024];
            (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 2 ^2game's launch command: ^5%s\n", exe_file_path);
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

    str_copy(install_path, "C:\\");
    found_reg_location = nullptr;

    static char msgbuff[1024];
    (void)snprintf(msgbuff, std::size(msgbuff), "Please, select your Call of Duty 2 game's installation folder and click OK.");

    const char *cod2_game_path = BrowseFolder(install_path, msgbuff);

    if (lstrcmp(cod2_game_path, "") != 0 && lstrcmp(cod2_game_path, "C:\\") != 0) {
      (void)snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", cod2_game_path);
    }
  }

  if (!str_contains(exe_file_path, "-applaunch 2630")) {
    string exe_path{ exe_file_path };
    replace_forward_slash_with_backward_slash(exe_path);
    str_copy(exe_file_path, exe_path.c_str());
  }

  if (!check_if_file_path_exists(exe_file_path) && !str_contains(exe_file_path, "-applaunch 2630")) {
    main_app.set_cod2mp_exe_path("");
  } else {
    main_app.set_cod2mp_exe_path(exe_file_path);
    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
  }

  return exe_file_path;
}

bool check_if_call_of_duty_2_game_is_running() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[512];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));
      CloseHandle(ph);
      if (n == 0) continue;
      const string module_base_name{ buffer };
      if (str_index_of(module_base_name, "cod2mp_s.exe", 0, true) != string::npos)
        return true;
    }

  DWORD cch{};
  HKEY game_installation_reg_key;

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_running{};
  cch = sizeof(is_steam_game_running);

  auto status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 2630 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  is_steam_game_running = 0;
  cch = sizeof(is_steam_game_running);

  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

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
    R"(SOFTWARE\Wow6432Node\Activision\Call of Duty 4)",
    "SOFTWARE\\Activision\\Call of Duty(R) 4",
    R"(SOFTWARE\WOW6432Node\Activision\Call of Duty(R) 4)",
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

  LRESULT status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Valve\Steam\Apps\7940)", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {
    cch = sizeof(is_steam_game_installed);
    status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

    if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
      HKEY steam_installation_key{};
      status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
      if (status == ERROR_SUCCESS) {
        cch = sizeof(install_path);
        status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          string steam_exe_path{ install_path };
          replace_forward_slash_with_backward_slash(steam_exe_path);
          if (check_if_file_path_exists(steam_exe_path.c_str())) {

            (void)snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 7940", steam_exe_path.c_str());

            found = true;
            char buffer[1024];
            (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 4: Modern Warfare (^4Steam version^3) ^2game's launch command: ^5%s\n", exe_file_path);
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

    status = RegOpenKeyEx(HKEY_CURRENT_USER, R"(SOFTWARE\Valve\Steam\Apps\7940)", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

      if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
        HKEY steam_installation_key{};
        status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
        if (status == ERROR_SUCCESS) {
          cch = sizeof(install_path);
          status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

          if (status == ERROR_SUCCESS) {

            string steam_exe_path{ install_path };
            replace_forward_slash_with_backward_slash(steam_exe_path);
            if (check_if_file_path_exists(steam_exe_path.c_str())) {

              (void)snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 7940", steam_exe_path.c_str());

              found = true;
              char buffer[1024];
              (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 4: Modern Warfare (^4Steam version^3) ^2game's launch command: ^5%s\n", exe_file_path);
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

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

      if (status == ERROR_SUCCESS) {
        WIN32_FIND_DATA find_data{};
        remove_dir_path_sep_char(install_path);
        (void)snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", install_path);
        HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

        if (search_handle != INVALID_HANDLE_VALUE) {

          found = true;
          found_reg_location = *def_game_reg_key;
          char buffer[1024];
          (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 4: Modern Warfare ^2game's launch command: ^5%s\n", exe_file_path);
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

      status = RegOpenKeyEx(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

      if (status == ERROR_SUCCESS) {

        status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {
          WIN32_FIND_DATA find_data{};
          remove_dir_path_sep_char(install_path);
          (void)snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", install_path);
          HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

          if (search_handle != INVALID_HANDLE_VALUE) {

            found = true;
            found_reg_location = *def_game_reg_key;
            char buffer[1024];
            (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 4: Modern Warfare ^2game's launch command: ^5%s\n", exe_file_path);
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

    str_copy(install_path, "C:\\");
    found_reg_location = nullptr;

    static char msgbuff[1024];
    (void)snprintf(msgbuff, std::size(msgbuff), "Please, select your Call of Duty 4: Modern Warfare game's installation folder and click OK.");

    const char *cod4_game_path = BrowseFolder(install_path, msgbuff);

    if (lstrcmp(cod4_game_path, "") != 0 && lstrcmp(cod4_game_path, "C:\\") != 0) {
      (void)snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", cod4_game_path);
    }
  }

  if (!str_contains(exe_file_path, "-applaunch 7940")) {
    string exe_path{ exe_file_path };
    replace_forward_slash_with_backward_slash(exe_path);
    str_copy(exe_file_path, exe_path.c_str());
  }

  if (!check_if_file_path_exists(exe_file_path) && !str_contains(exe_file_path, "-applaunch 7940")) {
    main_app.set_iw3mp_exe_path("");
  } else {
    main_app.set_iw3mp_exe_path(exe_file_path);
    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
  }

  return exe_file_path;
}

bool check_if_call_of_duty_4_game_is_running() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[512];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));
      CloseHandle(ph);
      if (n == 0) continue;
      const string module_base_name{ buffer };
      if (str_index_of(module_base_name, "iw3mp.exe", 0, true) != string::npos)
        return true;
    }

  DWORD cch{};
  HKEY game_installation_reg_key;

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_running{};
  cch = sizeof(is_steam_game_running);

  auto status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 7940 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  is_steam_game_running = 0;
  cch = sizeof(is_steam_game_running);

  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

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
    R"(SOFTWARE\Wow6432Node\Activision\Call of Duty WAW)",
    "SOFTWARE\\Activision\\Call of Duty(R) WAW",
    R"(SOFTWARE\WOW6432Node\Activision\Call of Duty(R) WAW)",
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

  LRESULT status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Valve\Steam\Apps\10090)", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {
    cch = sizeof(is_steam_game_installed);
    status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

    if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
      HKEY steam_installation_key{};
      status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
      if (status == ERROR_SUCCESS) {
        cch = sizeof(install_path);
        status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          string steam_exe_path{ install_path };
          replace_forward_slash_with_backward_slash(steam_exe_path);
          if (check_if_file_path_exists(steam_exe_path.c_str())) {

            (void)snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 10090", steam_exe_path.c_str());

            found = true;
            char buffer[1024];
            (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 5: World at War (^4Steam version^3) ^2game's launch command: ^5%s\n", exe_file_path);
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

    status = RegOpenKeyEx(HKEY_CURRENT_USER, R"(SOFTWARE\Valve\Steam\Apps\10090)", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

      if (status == ERROR_SUCCESS && is_steam_game_installed == 1) {
        HKEY steam_installation_key{};
        status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
        if (status == ERROR_SUCCESS) {
          cch = sizeof(install_path);
          status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

          if (status == ERROR_SUCCESS) {

            string steam_exe_path{ install_path };
            replace_forward_slash_with_backward_slash(steam_exe_path);
            if (check_if_file_path_exists(steam_exe_path.c_str())) {

              (void)snprintf(exe_file_path, max_path_length, "\"%s\" -applaunch 10090", steam_exe_path.c_str());

              found = true;
              char buffer[1024];
              (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 5: World at War (^4Steam version^3) ^2game's launch command: ^5%s\n", exe_file_path);
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

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS) {

      status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

      if (status == ERROR_SUCCESS) {
        WIN32_FIND_DATA find_data{};
        remove_dir_path_sep_char(install_path);
        (void)snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", install_path);
        HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

        if (search_handle != INVALID_HANDLE_VALUE) {

          found = true;
          found_reg_location = *def_game_reg_key;
          char buffer[1024];
          (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 5: World at War ^2game's launch command: ^5%s\n", exe_file_path);
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

      status = RegOpenKeyEx(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

      if (status == ERROR_SUCCESS) {

        status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr, reinterpret_cast<LPBYTE>(install_path), &cch);

        if (status == ERROR_SUCCESS) {

          WIN32_FIND_DATA find_data{};
          remove_dir_path_sep_char(install_path);
          (void)snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", install_path);
          HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

          if (search_handle != INVALID_HANDLE_VALUE) {

            found = true;
            found_reg_location = *def_game_reg_key;
            char buffer[1024];
            (void)snprintf(buffer, std::size(buffer), "^2Successfully built your ^3Call of Duty 5: World at War ^2game's launch command: ^5%s\n", exe_file_path);
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

    str_copy(install_path, "C:\\");
    found_reg_location = nullptr;

    static char msgbuff[1024];
    (void)snprintf(msgbuff, std::size(msgbuff), "Please, select your Call of Duty 5: World at War game's installation folder and click OK.");

    const char *cod5_game_path = BrowseFolder(install_path, msgbuff);

    if (lstrcmp(cod5_game_path, "") != 0 && lstrcmp(cod5_game_path, "C:\\") != 0) {
      (void)snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", cod5_game_path);
    }
  }

  if (!str_contains(exe_file_path, "-applaunch 10090")) {
    string exe_path{ exe_file_path };
    replace_forward_slash_with_backward_slash(exe_path);
    str_copy(exe_file_path, exe_path.c_str());
  }

  if (!check_if_file_path_exists(exe_file_path) && !str_contains(exe_file_path, "-applaunch 10090")) {
    main_app.set_cod5mp_exe_path("");
  } else {
    main_app.set_cod5mp_exe_path(exe_file_path);
    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
  }

  return exe_file_path;
}

bool check_if_call_of_duty_5_game_is_running() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[512];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));
      CloseHandle(ph);
      if (n == 0) continue;
      const string module_base_name{ buffer };
      if (str_index_of(module_base_name, "cod5mp.exe", 0, true) != string::npos)
        return true;
    }

  DWORD cch{};
  HKEY game_installation_reg_key;

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  DWORD is_steam_game_running{};
  cch = sizeof(is_steam_game_running);

  auto status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

    if (status == ERROR_SUCCESS && 10090 == is_steam_game_running)
      return true;
  }

  RegCloseKey(game_installation_reg_key);

  ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
  is_steam_game_running = 0;
  cch = sizeof(is_steam_game_running);

  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

  if (status == ERROR_SUCCESS) {

    status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr, reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

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
      print_colored_text(app_handles.hwnd_re_messages_data, "Call of Duty Multiplayer game is already running in the background.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return false;
    }

    game_path = main_app.get_codmp_exe_path().c_str();
    if (!check_if_file_path_exists(game_path) && !stl::helper::str_contains(game_path, "-applaunch 2620")) {
      game_path = find_call_of_duty_1_installation_path();
    }
    break;

  case game_name_t::cod2:
    if (check_if_call_of_duty_2_game_is_running()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "Call of Duty 2 Multiplayer game is already running in the background.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return false;
    }

    game_path = main_app.get_cod2mp_exe_path().c_str();
    if (!check_if_file_path_exists(game_path) && !stl::helper::str_contains(game_path, "-applaunch 2630")) {
      game_path = find_call_of_duty_2_installation_path();
    }
    break;

  case game_name_t::cod4:
    if (check_if_call_of_duty_4_game_is_running()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "Call of Duty 4: Modern Warfare Multiplayer game is already running in the background.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return false;
    }

    game_path = main_app.get_iw3mp_exe_path().c_str();
    if (!check_if_file_path_exists(game_path) && !stl::helper::str_contains(game_path, "-applaunch 7940")) {
      game_path = find_call_of_duty_4_installation_path();
    }
    break;

  case game_name_t::cod5:
    if (check_if_call_of_duty_5_game_is_running()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "Call of Duty 5: World at War Multiplayer game is already running in the background.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return false;
    }

    game_path = main_app.get_cod5mp_exe_path().c_str();
    if (!check_if_file_path_exists(game_path) && !stl::helper::str_contains(game_path, "-applaunch 10090")) {
      game_path = find_call_of_duty_5_installation_path();
    }
    break;

  default:
    print_colored_text(app_handles.hwnd_re_messages_data, "^1Invalid ^1'game_name_t' enum value has been specified!\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    return false;
  }


  assert(game_path != nullptr && stl::helper::len(game_path) > 0U);

  const bool is_game_installed_via_steam{
    stl::helper::str_contains(game_path, "-applaunch 2620") || stl::helper::str_contains(game_path, "-applaunch 2630") || stl::helper::str_contains(game_path, "-applaunch 7940") || stl::helper::str_contains(game_path, "-applaunch 10090")
  };

  if (game_path == nullptr || len(game_path) == 0 || (!str_ends_with(game_path, ".exe", true) && !is_game_installed_via_steam))
    return false;

  if (minimize_tinyrcon) {
    ShowWindow(GetConsoleWindow(), SW_MINIMIZE);
  }

  if (is_game_installed_via_steam) {
    if (use_private_slot) {
      (void)snprintf(command_line, max_path_length, "%s +password %s +connect %s", game_path, main_app.get_game_server().get_private_slot_password().c_str(), server_ip_port_address.c_str());
    } else {
      (void)snprintf(command_line, max_path_length, "%s +connect %s", game_path, server_ip_port_address.c_str());
    }
  } else {
    if (use_private_slot) {
      (void)snprintf(command_line, max_path_length, "\"%s\" +password %s +connect %s", game_path, main_app.get_game_server().get_private_slot_password().c_str(), server_ip_port_address.c_str());
    } else {
      (void)snprintf(command_line, max_path_length, "\"%s\" +connect %s", game_path, server_ip_port_address.c_str());
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

    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

    STARTUPINFO process_startup_info{};
    process_startup_info.cb = sizeof(STARTUPINFO);
    process_startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK;

    if (!CreateProcess(
          nullptr,
          command_line,
          nullptr,// Process handle not inheritable
          nullptr,// Thread handle not inheritable
          FALSE,// Set handle inheritance to FALSE
          HIGH_PRIORITY_CLASS | DETACHED_PROCESS,// creation flags
          nullptr,// Use parent's environment block
          game_folder.c_str(),// Use parent's starting directory
          &process_startup_info,// Pointer to STARTUPINFO structure
          &pr_info)) {
      char buffer[512]{};
      strerror_s(buffer, 512, GetLastError());
      const string error_message{ "^1Launching game failed: "s + buffer + "\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return false;
    }
  } else {

    ostringstream oss;
    oss << "^2Launching ^1" << main_app.get_game_title() << " ^2via Steam and connecting to ^1" << main_app.get_game_server_name() << " ^2game server...\n";
    const string message{ oss.str() };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

    STARTUPINFO process_startup_info{};
    process_startup_info.cb = sizeof(STARTUPINFO);
    process_startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK;

    if (!CreateProcess(
          nullptr,
          command_line,
          nullptr,// Process handle not inheritable
          nullptr,// Thread handle not inheritable
          FALSE,// Set handle inheritance to FALSE
          ABOVE_NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,// creation flags
          nullptr,// Use parent's environment block
          nullptr,// Use parent's starting directory
          &process_startup_info,// Pointer to STARTUPINFO structure
          &pr_info)) {
      char buffer[512]{};
      strerror_s(buffer, std::size(buffer), GetLastError());
      const string error_message{ "^1Launching game via Steam failed: "s + buffer + "\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return false;
    }
  }

  initiate_sending_rcon_status_command_now();
  return true;
}

bool check_if_file_path_exists(const char *file_path) noexcept
{
  const directory_entry dir_entry{ file_path };
  return dir_entry.exists();
}

bool delete_temporary_game_file() noexcept
{
  DWORD processes[1024], processes_size{};
  char buffer[512];

  memset(processes, 0, 1024 * sizeof(DWORD));

  if (EnumProcesses(processes, sizeof(processes), &processes_size))
    for (size_t i{}; i < processes_size; ++i) {
      if (processes[i] == 0) continue;
      auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
      if (nullptr == ph) continue;
      const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));

      if (n != 0) {
        string module_base_name{ buffer };
        if (module_base_name.find("Call of Duty 2") != string::npos || module_base_name.find("Call of Duty(R) 2") != string::npos || stl::helper::str_index_of(module_base_name, "cod2mp_s.exe", true) != string::npos) {
          GetModuleFileNameEx(ph, nullptr, buffer, (DWORD)std::size(buffer));
          module_base_name.assign(buffer);
          const auto start = stl::helper::str_last_index_of(module_base_name, "cod2mp_s.exe", string::npos, true);
          if (start != string::npos) {
            module_base_name.replace(start, len("cod2mp_s.exe"), "__CoD2MP_s");
            DeleteFile(module_base_name.c_str());
          }
          return true;
        }
      }

      CloseHandle(ph);
    }

  return false;
}

size_t print_colored_text(HWND re_control, const char *text, const is_append_message_to_richedit_control print_to_richedit_control, const is_log_message log_to_file, const is_log_datetime is_log_current_date_time, const bool is_prevent_auto_vertical_scrolling)
{
  const char *message{ text };
  size_t text_len{ stl::helper::len(text) };
  if (nullptr == text || 0U == text_len)
    return 0U;
  const char *last = text + text_len;
  size_t printed_chars_count{};
  bool is_last_char_new_line{ text[text_len - 1] == '\n' };

  if (print_to_richedit_control == is_append_message_to_richedit_control::yes) {
    // lock_guard lg{ re_messages_mutex };

    COLORREF bg_color{ color::black };
    COLORREF fg_color{ color::white };
    set_rich_edit_control_colors(re_control, fg_color, bg_color);

    string msg;
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

    /*if (is_prevent_auto_vertical_scrolling) {
      auto re_messages_no_autovscroll = GetWindowStyle(re_control);
      re_messages_no_autovscroll = re_messages_no_autovscroll | ES_AUTOVSCROLL;
      SetWindowLong(re_control, -16, re_messages_no_autovscroll);
    }*/

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


bool get_confirmation_message_from_user(const char *message, const char *caption) noexcept
{
  const auto answer = MessageBox(app_handles.hwnd_main_window, message, caption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);
  return answer == IDYES;
}

bool check_if_user_wants_to_quit(const char *msg) noexcept
{

  return str_compare(msg, "q") == 0 || str_compare(msg, "!q") == 0 || str_compare(msg, "exit") == 0 || str_compare(msg, "quit") == 0 || str_compare(msg, "!exit") == 0 || str_compare(msg, "!quit") == 0;
}

void set_rich_edit_control_colors(HWND richEditCtrl, const COLORREF fg_color, const COLORREF bg_color, const char *font_face_name) noexcept
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


void append(HWND hwnd, const char *str, const bool is_prevent_auto_vertical_scrolling) noexcept
{
  // lock_guard lg{ print_data_mutex };
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
      // {
      // lock_guard<mutex> l{ mu };
      // exit_flag.notify_all();
      // }
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
      /*{
        lock_guard<mutex> l{ mu };
        exit_flag.notify_all();
      }*/
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

  for (auto &pl : tembanned_players) {
    pl.reason = remove_disallowed_character_in_string(pl.reason);
    const time_t ban_expires_time = pl.banned_start_time + (pl.ban_duration_in_hours * 3600);
    if (ban_expires_time > now_in_seconds) {
      ostringstream oss;
      oss << "^3Player's (^7" << pl.player_name << "^3) temporary ban (banned on ^1" << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pl.banned_start_time)
          << " ^3with reason: ^1" << pl.reason << "^3)\n will automatically be removed in ^1" << get_time_interval_info_string_for_seconds(ban_expires_time - now_in_seconds) << '\n';
      const string message{ oss.str() };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    } else {
      main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = pl.player_name;
      main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
      main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pl.banned_start_time);
      main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", ban_expires_time);
      main_app.get_tinyrcon_dict()["{REASON}"] = pl.reason;
      main_app.get_tinyrcon_dict()["{BANNED_BY}"] = remove_disallowed_character_in_string(pl.banned_by_user_name);
      string message{ main_app.get_automatic_remove_temp_ban_msg() };
      main_app.get_connection_manager_for_messages().process_and_send_message("remove-tempban", format("{}\\{}\\{}\\{}\\{}\\{}\\{}", pl.ip_address, pl.player_name, pl.banned_date_time, pl.banned_start_time, pl.ban_duration_in_hours, pl.reason, pl.banned_by_user_name), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      build_tiny_rcon_message(message);
      remove_temp_banned_ip_address(pl.ip_address, message, true);
      /*replace_br_with_new_line(message);
      const string inform_msg{ format("{}\\{}", main_app.get_username(), message) };
      main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));*/
    }
  }
}

void construct_tinyrcon_gui(HWND hWnd) noexcept
{
  const long draw_border_lines_flag{
    main_app.get_is_draw_border_lines() ? WS_BORDER : 0L
  };

  if (app_handles.hwnd_online_admins_information != nullptr) {
    ShowWindow(app_handles.hwnd_online_admins_information, SW_HIDE);
    DestroyWindow(app_handles.hwnd_online_admins_information);
  }

  app_handles.hwnd_online_admins_information = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY, 10, 13, screen_width / 2 + 150, 25, hWnd, nullptr, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_online_admins_information)
    FatalAppExit(0, "Couldn't create 'app_handles.hwnd_online_admins_information' richedit control!");

  assert(app_handles.hwnd_online_admins_information != nullptr);
  SendMessage(app_handles.hwnd_online_admins_information, EM_SETBKGNDCOLOR, NULL, color::black);
  scroll_to_beginning(app_handles.hwnd_online_admins_information);
  set_rich_edit_control_colors(app_handles.hwnd_online_admins_information, color::white, color::black, "Lucida Console");
  print_colored_text(app_handles.hwnd_online_admins_information, "^2Online admins:", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
  SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);

  if (app_handles.hwnd_match_information != nullptr) {
    ShowWindow(app_handles.hwnd_match_information, SW_HIDE);
    DestroyWindow(app_handles.hwnd_match_information);
  }

  app_handles.hwnd_match_information = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY, 10, 50, screen_width / 2 + 150, 30, hWnd, nullptr, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_match_information)
    FatalAppExit(0, "Couldn't create 'app_handles.hwnd_match_information' richedit control!");

  assert(app_handles.hwnd_match_information != nullptr);
  SendMessage(app_handles.hwnd_match_information, EM_SETBKGNDCOLOR, NULL, color::black);
  scroll_to_beginning(app_handles.hwnd_match_information);
  set_rich_edit_control_colors(app_handles.hwnd_match_information, color::white, color::black, "Lucida Console");
  if (g_re_match_information_contents.empty())
    Edit_SetText(app_handles.hwnd_match_information, "");
  else
    print_colored_text(app_handles.hwnd_match_information, g_re_match_information_contents.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);

  if (app_handles.hwnd_re_messages_data != nullptr) {
    ShowWindow(app_handles.hwnd_re_messages_data, SW_HIDE);
    DestroyWindow(app_handles.hwnd_re_messages_data);
  }

  app_handles.hwnd_re_messages_data = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, draw_border_lines_flag | WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_LEFT /*| ES_AUTOVSCROLL*/ | ES_NOHIDESEL | ES_READONLY, screen_width / 2 + 170, 45, screen_width / 2 - 190, screen_height / 2 - 30, hWnd, reinterpret_cast<HMENU>(ID_BANNEDEDIT), app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_re_messages_data)
    FatalAppExit(0, "Couldn't create 'g_banned_players_data' richedit control!");

  if (app_handles.hwnd_re_messages_data != nullptr) {
    SendMessage(app_handles.hwnd_re_messages_data, EM_SETBKGNDCOLOR, NULL, color::black);
    scroll_to_beginning(app_handles.hwnd_re_messages_data);
    set_rich_edit_control_colors(app_handles.hwnd_re_messages_data, color::white, color::black, "Lucida Console");
    if (g_message_data_contents.empty())
      Edit_SetText(app_handles.hwnd_re_messages_data, "");
    else
      print_colored_text(app_handles.hwnd_re_messages_data, g_message_data_contents.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  }

  if (app_handles.hwnd_re_help_data != nullptr) {
    ShowWindow(app_handles.hwnd_re_help_data, SW_HIDE);
    DestroyWindow(app_handles.hwnd_re_help_data);
  }

  app_handles.hwnd_re_help_data = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, draw_border_lines_flag | WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_LEFT | ES_AUTOVSCROLL | ES_READONLY, screen_width / 2 + 170, screen_height / 2 + 60, screen_width - 20 - (screen_width / 2 + 170), screen_height - 120 - (screen_height / 2 + 90), hWnd, nullptr, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_re_help_data)
    FatalAppExit(0, "Couldn't create 'g_re_help' richedit control!");
  if (app_handles.hwnd_re_help_data != nullptr) {
    SendMessage(app_handles.hwnd_re_help_data, EM_SETBKGNDCOLOR, NULL, color::black);
    scroll_to_beginning(app_handles.hwnd_re_help_data);
    set_rich_edit_control_colors(app_handles.hwnd_re_help_data, color::white, color::black, "Lucida Console");
    print_colored_text(app_handles.hwnd_re_help_data, user_help_message, is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  }

  app_handles.hwnd_e_user_input = CreateWindowEx(0, "Edit", nullptr, WS_GROUP | WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 140, screen_height - 77, 250, 20, hWnd, reinterpret_cast<HMENU>(ID_USEREDIT), app_handles.hInstance, nullptr);

  app_handles.hwnd_button_warn = CreateWindowEx(NULL, "Button", "&Warn", WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 10, screen_height - 123, 80, 30, hWnd, reinterpret_cast<HMENU>(ID_WARNBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_button_kick = CreateWindowEx(NULL, "Button", "&Kick", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 110, screen_height - 123, 80, 30, hWnd, reinterpret_cast<HMENU>(ID_KICKBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_button_tempban = CreateWindowEx(NULL, "Button", "&Tempban", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 210, screen_height - 123, 100, 30, hWnd, reinterpret_cast<HMENU>(ID_TEMPBANBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_button_ipban = CreateWindowEx(NULL, "Button", "Ban &IP", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 330, screen_height - 123, 100, 30, hWnd, reinterpret_cast<HMENU>(ID_IPBANBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_button_view_tempbans = CreateWindowEx(NULL, "Button", "View temporary bans", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 450, screen_height - 123, 180, 30, hWnd, reinterpret_cast<HMENU>(ID_VIEWTEMPBANSBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_button_view_ipbans = CreateWindowEx(NULL, "Button", "View IP bans", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 650, screen_height - 123, 140, 30, hWnd, reinterpret_cast<HMENU>(ID_VIEWIPBANSBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_button_view_adminsdata = CreateWindowEx(NULL, "Button", "View admins' data", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 810, screen_height - 123, 150, 30, hWnd, reinterpret_cast<HMENU>(ID_VIEWADMINSDATA), app_handles.hInstance, nullptr);

  app_handles.hwnd_connect_button = CreateWindowEx(NULL, "Button", "Join server", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 413, screen_height - 82, 100, 30, hWnd, reinterpret_cast<HMENU>(ID_CONNECTBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_connect_private_slot_button = CreateWindowEx(NULL, "Button", "Join server (private slot)", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 533, screen_height - 82, 200, 30, hWnd, reinterpret_cast<HMENU>(ID_CONNECTPRIVATESLOTBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_say_button = CreateWindowEx(NULL, "Button", "Send public message", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 753, screen_height - 82, 170, 30, hWnd, reinterpret_cast<HMENU>(ID_SAY_BUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_tell_button = CreateWindowEx(NULL, "Button", "Send private message", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 943, screen_height - 82, 170, 30, hWnd, reinterpret_cast<HMENU>(ID_TELL_BUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_quit_button = CreateWindowEx(NULL, "Button", "E&xit", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width - 100, screen_height - 80, 70, 25, hWnd, reinterpret_cast<HMENU>(ID_QUITBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_clear_messages_button = CreateWindowEx(NULL, "Button", "Clear messages", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 170, 8, 120, 28, hWnd, reinterpret_cast<HMENU>(ID_CLEARMESSAGESCREENBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_configure_server_settings_button = CreateWindowEx(NULL, "Button", "Configure settings", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 310, 8, 130, 28, hWnd, reinterpret_cast<HMENU>(ID_BUTTON_CONFIGURE_SERVER_SETTINGS), app_handles.hInstance, nullptr);

  app_handles.hwnd_refresh_players_data_button = CreateWindowEx(NULL, "Button", "&Refresh data", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 460, 8, 120, 28, hWnd, reinterpret_cast<HMENU>(ID_REFRESHDATABUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_combo_box_sortmode = CreateWindowEx(NULL, "Combobox", nullptr, WS_GROUP | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_HSCROLL, screen_width - 470, screen_height - 140, 260, 210, hWnd, reinterpret_cast<HMENU>(ID_COMBOBOX_SORTMODE), app_handles.hInstance, nullptr);

  SetWindowSubclass(app_handles.hwnd_combo_box_sortmode, ComboProc, 0, 0);

  app_handles.hwnd_combo_box_map = CreateWindowEx(NULL, "Combobox", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_HSCROLL, screen_width / 2 + 210, screen_height / 2 + 25, 150, 200, hWnd, reinterpret_cast<HMENU>(ID_COMBOBOX_MAP), app_handles.hInstance, nullptr);

  SetWindowSubclass(app_handles.hwnd_combo_box_map, ComboProc, 0, 0);

  app_handles.hwnd_combo_box_gametype = CreateWindowEx(NULL, "Combobox", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_HSCROLL, screen_width / 2 + 460, screen_height / 2 + 25, 60, 130, hWnd, reinterpret_cast<HMENU>(ID_COMBOBOX_GAMETYPE), app_handles.hInstance, nullptr);

  SetWindowSubclass(app_handles.hwnd_combo_box_gametype, ComboProc, 0, 0);

  app_handles.hwnd_button_load = CreateWindowEx(NULL, "Button", "Load map", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 530, screen_height / 2 + 23, 80, 25, hWnd, reinterpret_cast<HMENU>(ID_LOADBUTTON), app_handles.hInstance, nullptr);

  app_handles.hwnd_progress_bar = CreateWindowEx(0, PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_MARQUEE | PBS_SMOOTH, screen_width - 200, screen_height - 140, 170, 23, hWnd, nullptr, app_handles.hInstance, nullptr);

  if (app_handles.hwnd_players_grid) {
    DestroyWindow(app_handles.hwnd_players_grid);
  }

  app_handles.hwnd_players_grid = CreateWindowExA(WS_EX_CLIENTEDGE, WC_SIMPLEGRIDA, "", WS_VISIBLE | WS_CHILD, 10, 85, screen_width / 2 + 140, screen_height - 195, hWnd, reinterpret_cast<HMENU>(501), app_handles.hInstance, nullptr);

  hImageList = ImageList_Create(32, 24, ILC_COLOR32, flag_name_index.size(), 1);
  for (size_t i{}; i < flag_name_index.size(); ++i) {
    auto hbmp = static_cast<HBITMAP>(LoadImage(app_handles.hInstance, MAKEINTRESOURCE(151 + i), IMAGE_BITMAP, 32, 24, LR_CREATEDIBSECTION | LR_DEFAULTSIZE));
    ImageList_Add(hImageList, hbmp, nullptr);
  }

  initialize_players_grid(app_handles.hwnd_players_grid, 7, max_players_grid_rows, true);

  const auto &full_map_names_to_rcon_names = get_full_map_names_to_rcon_map_names_for_specified_game_name(main_app.get_game_name());

  for (const auto &[long_map_name, short_map_name] : full_map_names_to_rcon_names) {
    SendMessage(app_handles.hwnd_combo_box_map, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(long_map_name.c_str()));
  }

  const auto &full_gametype_names = get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());

  for (const auto &[short_gametype_name, long_gametype_name] : full_gametype_names) {
    SendMessage(app_handles.hwnd_combo_box_gametype, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(short_gametype_name.c_str()));
  }

  set_available_sort_methods(true);

  const auto &rcon_map_names_to_full_map_names = get_rcon_map_names_to_full_map_names_for_specified_game_name(main_app.get_game_name());

  SendMessage(app_handles.hwnd_combo_box_map, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(rcon_map_names_to_full_map_names.at("mp_toujane").c_str()));
  SendMessage(app_handles.hwnd_combo_box_gametype, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>("ctf"));

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

  Grid_OnSetFont(app_handles.hwnd_players_grid, font_for_players_grid_data, TRUE);
  ShowHscroll(app_handles.hwnd_players_grid);

  const int players_grid_width{ screen_width / 2 + 130 };

  SimpleGrid_SetColWidth(hgrid, 0, 50);
  SimpleGrid_SetColWidth(hgrid, 1, 70);
  SimpleGrid_SetColWidth(hgrid, 2, 60);
  SimpleGrid_SetColWidth(hgrid, 3, 270);
  if (is_for_rcon_status) {
    SimpleGrid_SetColWidth(hgrid, 4, 190);
    SimpleGrid_SetColWidth(hgrid, 5, players_grid_width - 690);
    SimpleGrid_SetColWidth(hgrid, 6, 50);
  }
  SimpleGrid_SetSelectionMode(app_handles.hwnd_players_grid, GSO_FULLROW);
}

void clear_players_data_in_players_grid(HWND hgrid, const size_t start_row, const size_t last_row, const size_t cols) noexcept
{
  for (size_t i{ start_row }; i < last_row; ++i) {
    for (size_t j{}; j < cols; ++j) {
      if (6 == j) {
        display_country_flag(hgrid, i, 6, "xy");
      } else {
        PutCell(hgrid, i, j, "");
      }
    }
  }
}

void display_players_data_in_players_grid(HWND hgrid) noexcept
{
  const size_t number_of_players{ main_app.get_game_server().get_number_of_players() };

  Edit_SetText(app_handles.hwnd_match_information, "");
  print_colored_text(app_handles.hwnd_match_information, g_re_match_information_contents.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
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

  const int players_grid_width{ screen_width / 2 + 130 };
  const int player_name_column_width{ findLongestTextWidthInColumn(hgrid, 3) };
  SimpleGrid_SetColWidth(hgrid, 0, 50);
  SimpleGrid_SetColWidth(hgrid, 1, 70);
  SimpleGrid_SetColWidth(hgrid, 2, 60);
  SimpleGrid_SetColWidth(hgrid, 3, player_name_column_width);
  SimpleGrid_SetColWidth(hgrid, 4, 190);
  SimpleGrid_SetColWidth(hgrid, 5, players_grid_width - (player_name_column_width + 420));
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

  assert(app_handles.hwnd_re_confirmation_message != nullptr);
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
  app_handles.hwnd_yes_button = CreateWindowEx(NULL, "Button", "Yes", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 100, 250, 50, 20, app_handles.hwnd_confirmation_dialog, reinterpret_cast<HMENU>(ID_YES_BUTTON), app_handles.hInstance, nullptr);

  if (app_handles.hwnd_no_button) {
    DestroyWindow(app_handles.hwnd_no_button);
  }
  app_handles.hwnd_no_button = CreateWindowEx(NULL, "Button", "No", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 170, 250, 50, 20, app_handles.hwnd_confirmation_dialog, reinterpret_cast<HMENU>(ID_NO_BUTTON), app_handles.hInstance, nullptr);

  print_colored_text(app_handles.hwnd_re_confirmation_message, msg, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  Edit_SetText(app_handles.hwnd_e_reason, admin_reason.c_str());
  CenterWindow(app_handles.hwnd_confirmation_dialog);
  ShowWindow(app_handles.hwnd_confirmation_dialog, SW_SHOW);
  SetFocus(app_handles.hwnd_no_button);

  MSG wnd_msg{};
  while (GetMessage(&wnd_msg, nullptr, NULL, NULL) != 0) {
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
  initiate_sending_rcon_status_command_now();
  is_process_combobox_item_selection_event = true;
}

void update_game_server_setting(std::string key,
  std::string value)
{
  if (key == "pswrd") {
    main_app.get_game_server().set_is_password_protected('0' != value[0]);
  } else if (key == "protoco") {
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
    if (!value.empty() && value != "main")
      main_app.get_game_server().set_is_mod_enabled(true);
    else
      main_app.get_game_server().set_is_mod_enabled(false);
  } else if (key == "g_antilag") {
    main_app.get_game_server().set_is_anti_lag_enabled('1' == value[0]);
  } else if (key == "g_gametype") {
    main_app.get_game_server().set_current_game_type(value);
  } else if (key == "gamename") {
    main_app.get_game_server().set_game_name(value);
  } else if (key == "protoco") {
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

  if (reply.find("Invalid password.") != string::npos || reply.find("rconpassword") != string::npos || reply.find("Bad rcon") != string::npos) {
    cm.send_and_receive_rcon_data("getstatus", reply, ip_address, port_number, rcon_password, true);
    const string &gn{ main_app.get_game_server().get_game_name() };
    return { false, game_name_to_game_name_t.contains(gn) ? game_name_to_game_name_t.at(gn) : game_name_t::unknown };
  }


  if (stl::helper::str_contains(reply, "CoD1", 0, true))
    return { true, game_name_t::cod1 };
  if (stl::helper::str_contains(reply, "CoD2", 0, true))
    return { true, game_name_t::cod2 };
  if (stl::helper::str_contains(reply, "CoD4", 0, true))
    return { true, game_name_t::cod4 };
  if (stl::helper::str_contains(reply, "CoD5", 0, true))
    return { true, game_name_t::cod5 };

  return { false, game_name_t::unknown };
}

bool show_and_process_tinyrcon_configuration_panel(const char *title)
{
  is_terminate_tinyrcon_settings_configuration_dialog_window.store(false);
  if (app_handles.hwnd_configuration_dialog) {
    DestroyWindow(app_handles.hwnd_configuration_dialog);
  }
  app_handles.hwnd_configuration_dialog = CreateWindowEx(NULL, wcex_configuration_dialog.lpszClassName, title, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME /*WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX*/, 0, 0, 920, 730, app_handles.hwnd_main_window, nullptr, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_configuration_dialog)
    return false;

  (void)CreateWindowEx(0, "Static", "Admin name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 10, 120, 20, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  if (app_handles.hwnd_user_name) {
    DestroyWindow(app_handles.hwnd_user_name);
  }

  app_handles.hwnd_user_name = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 10, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_ADMIN_NAME, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_user_name)
    return false;

  (void)CreateWindowEx(0, "Static", "Server name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 40, 120, 20, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  if (app_handles.hwnd_server_name) {
    DestroyWindow(app_handles.hwnd_server_name);
  }

  app_handles.hwnd_server_name = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 40, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_NAME, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_server_name)
    return false;

  if (app_handles.hwnd_server_ip_address) {
    DestroyWindow(app_handles.hwnd_server_ip_address);
  }
  (void)CreateWindowEx(0, "Static", "Server IP address:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 70, 130, 20, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_server_ip_address = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 70, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_IP, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_server_ip_address)
    return false;

  if (app_handles.hwnd_server_port) {
    DestroyWindow(app_handles.hwnd_server_port);
  }
  (void)CreateWindowEx(0, "Static", "Server port:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 100, 120, 20, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_server_port = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 100, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_PORT, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_server_port)
    return false;

  if (app_handles.hwnd_rcon_password) {
    DestroyWindow(app_handles.hwnd_rcon_password);
  }
  (void)CreateWindowEx(0, "Static", "Rcon password:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 130, 140, 20, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_rcon_password = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 130, 450, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_RCON, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_rcon_password)
    return false;

  if (app_handles.hwnd_enable_city_ban) {
    DestroyWindow(app_handles.hwnd_enable_city_ban);
  }

  if (app_handles.hwnd_enable_country_ban) {
    DestroyWindow(app_handles.hwnd_enable_country_ban);
  }

  (void)CreateWindowEx(0, "Static", "Enable city ban feature:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 160, 180, 20, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_enable_city_ban = CreateWindowEx(WS_EX_TRANSPARENT, "BUTTON", nullptr, WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 210, 160, 15, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_ENABLE_CITY_BAN_CHECKBOX, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_enable_city_ban)
    return false;

  (void)CreateWindowEx(0, "Static", "Enable country ban feature:", WS_CHILD | WS_VISIBLE | SS_LEFT, 260, 160, 180, 20, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_enable_country_ban = CreateWindowEx(WS_EX_TRANSPARENT, "BUTTON", nullptr, WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 480, 160, 15, 20, app_handles.hwnd_configuration_dialog, (HMENU)ID_ENABLE_COUNTRY_BAN_CHECKBOX, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_enable_country_ban)
    return false;

  if (app_handles.hwnd_cod1_path_edit) {
    DestroyWindow(app_handles.hwnd_cod1_path_edit);
  }
  (void)CreateWindowEx(0, "Static", "Call of Duty installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 195, 200, 25, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_cod1_path_edit = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 190, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD1_PATH, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_cod1_path_edit)
    return false;

  SendMessage(app_handles.hwnd_cod1_path_edit, EM_SETLIMITTEXT, 1024, 0);

  if (app_handles.hwnd_cod1_path_button) {
    DestroyWindow(app_handles.hwnd_cod1_path_button);
  }
  app_handles.hwnd_cod1_path_button = CreateWindowEx(NULL, "Button", "Browse for codmp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680, 200, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD1_PATH, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_cod1_path_button)
    return false;

  if (app_handles.hwnd_cod2_path_edit) {
    DestroyWindow(app_handles.hwnd_cod2_path_edit);
  }
  (void)CreateWindowEx(0, "Static", "Call of Duty 2 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 245, 200, 25, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_cod2_path_edit = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 240, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD2_PATH, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_cod2_path_edit)
    return false;

  if (app_handles.hwnd_cod2_path_button) {
    DestroyWindow(app_handles.hwnd_cod2_path_button);
  }
  app_handles.hwnd_cod2_path_button = CreateWindowEx(NULL, "Button", "Browse for cod2mp_s.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680, 250, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD2_PATH, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_cod2_path_button)
    return false;

  if (app_handles.hwnd_cod4_path_edit) {
    DestroyWindow(app_handles.hwnd_cod4_path_edit);
  }
  (void)CreateWindowEx(0, "Static", "Call of Duty 4 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 295, 200, 25, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_cod4_path_edit = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 290, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD4_PATH, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_cod4_path_edit)
    return false;

  if (app_handles.hwnd_cod4_path_button) {
    DestroyWindow(app_handles.hwnd_cod4_path_button);
  }
  app_handles.hwnd_cod4_path_button = CreateWindowEx(NULL, "Button", "Browse for iw3mp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680, 300, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD4_PATH, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_cod4_path_button)
    return false;

  if (app_handles.hwnd_cod5_path_edit) {
    DestroyWindow(app_handles.hwnd_cod5_path_edit);
  }
  (void)CreateWindowEx(0, "Static", "Call of Duty 5 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 345, 200, 25, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

  app_handles.hwnd_cod5_path_edit = CreateWindowEx(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 340, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD5_PATH, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_cod5_path_edit)
    return false;

  if (app_handles.hwnd_cod5_path_button) {
    DestroyWindow(app_handles.hwnd_cod5_path_button);
  }
  app_handles.hwnd_cod5_path_button = CreateWindowEx(NULL, "Button", "Browse for cod5mp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680, 350, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD5_PATH, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_cod5_path_button)
    return false;

  if (app_handles.hwnd_re_confirmation_message) {
    DestroyWindow(app_handles.hwnd_re_confirmation_message);
  }

  app_handles.hwnd_re_confirmation_message = CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_LEFT | ES_AUTOVSCROLL | ES_READONLY, 220, 390, 450, 260, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_re_confirmation_message)
    return false;

  assert(app_handles.hwnd_re_confirmation_message != nullptr);
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
  app_handles.hwnd_save_settings_button = CreateWindowEx(NULL, "Button", "Save changes", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 220, 660, 120, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_SAVE_CHANGES, app_handles.hInstance, nullptr);
  if (!app_handles.hwnd_save_settings_button)
    return false;

  if (app_handles.hwnd_test_connection_button) {
    DestroyWindow(app_handles.hwnd_test_connection_button);
  }

  app_handles.hwnd_test_connection_button = CreateWindowEx(NULL, "Button", "Test connection", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 360, 660, 140, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_TEST_CONNECTION, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_test_connection_button)
    return false;

  if (app_handles.hwnd_close_button) {
    DestroyWindow(app_handles.hwnd_close_button);
  }
  app_handles.hwnd_close_button = CreateWindowEx(NULL, "Button", "Cancel", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 520, 660, 60, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CANCEL, app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_close_button)
    return false;

  if (app_handles.hwnd_exit_tinyrcon_button) {
    DestroyWindow(app_handles.hwnd_exit_tinyrcon_button);
  }
  app_handles.hwnd_exit_tinyrcon_button = CreateWindowEx(NULL, "Button", "Exit TinyRcon", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 600, 660, 120, 25, app_handles.hwnd_configuration_dialog, reinterpret_cast<HMENU>(ID_BUTTON_CONFIGURATION_EXIT_TINYRCON), app_handles.hInstance, nullptr);

  if (!app_handles.hwnd_exit_tinyrcon_button)
    return false;

  SetWindowText(app_handles.hwnd_user_name, main_app.get_username().c_str());
  SetWindowText(app_handles.hwnd_server_name, main_app.get_game_server_name().c_str());
  SetWindowText(app_handles.hwnd_server_ip_address, main_app.get_game_server().get_server_ip_address().c_str());
  char buffer_port[8];
  (void)snprintf(buffer_port, std::size(buffer_port), "%d", main_app.get_game_server().get_server_port());
  SetWindowText(app_handles.hwnd_server_port, buffer_port);
  SetWindowText(app_handles.hwnd_rcon_password, main_app.get_game_server().get_rcon_password().c_str());

  SendMessage(app_handles.hwnd_enable_city_ban, BM_SETCHECK, main_app.get_game_server().get_is_automatic_city_kick_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);
  SendMessage(app_handles.hwnd_enable_country_ban, BM_SETCHECK, main_app.get_game_server().get_is_automatic_country_kick_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);

  /* CheckDlgButton(app_handles.hwnd_configuration_dialog, ID_ENABLE_CITY_BAN_CHECKBOX, main_app.get_game_server().get_is_automatic_city_kick_enabled() ? BST_CHECKED : BST_UNCHECKED);
   CheckDlgButton(app_handles.hwnd_configuration_dialog, ID_ENABLE_COUNTRY_BAN_CHECKBOX, main_app.get_game_server().get_is_automatic_country_kick_enabled() ? BST_CHECKED : BST_UNCHECKED);*/

  if (!main_app.get_codmp_exe_path().empty() && (str_contains(main_app.get_cod2mp_exe_path(), "-applaunch 2620") || check_if_file_path_exists(main_app.get_codmp_exe_path().c_str()))) {
    SetWindowText(app_handles.hwnd_cod1_path_edit, main_app.get_codmp_exe_path().c_str());
  } else {
    const char *found_cod1_path = find_call_of_duty_1_installation_path(false);
    if (found_cod1_path != nullptr && len(found_cod1_path) > 0 && (str_contains(found_cod1_path, "-applaunch 2620") || check_if_file_path_exists(found_cod1_path))) {
      SetWindowText(app_handles.hwnd_cod1_path_edit, found_cod1_path);
    } else {
      SetWindowText(app_handles.hwnd_cod1_path_edit, "");
    }
  }

  if (!main_app.get_cod2mp_exe_path().empty() && (str_contains(main_app.get_cod2mp_exe_path(), "-applaunch 2630") || check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str()))) {
    SetWindowText(app_handles.hwnd_cod2_path_edit, main_app.get_cod2mp_exe_path().c_str());
  } else {
    const char *found_cod2_path = find_call_of_duty_2_installation_path(false);
    if (found_cod2_path != nullptr && len(found_cod2_path) > 0 && (str_contains(found_cod2_path, "-applaunch 2630") || check_if_file_path_exists(found_cod2_path))) {
      SetWindowText(app_handles.hwnd_cod2_path_edit, found_cod2_path);
    } else {
      SetWindowText(app_handles.hwnd_cod2_path_edit, "");
    }
  }

  if (!main_app.get_iw3mp_exe_path().empty() && (str_contains(main_app.get_cod2mp_exe_path(), "-applaunch 7940") || check_if_file_path_exists(main_app.get_iw3mp_exe_path().c_str()))) {
    SetWindowText(app_handles.hwnd_cod4_path_edit, main_app.get_iw3mp_exe_path().c_str());
  } else {

    const char *found_cod4_path = find_call_of_duty_4_installation_path(false);
    if (found_cod4_path != nullptr && len(found_cod4_path) > 0 && (str_contains(found_cod4_path, "-applaunch 7940") || check_if_file_path_exists(found_cod4_path))) {
      SetWindowText(app_handles.hwnd_cod4_path_edit, found_cod4_path);
    } else {
      SetWindowText(app_handles.hwnd_cod4_path_edit, "");
    }
  }

  if (!main_app.get_cod5mp_exe_path().empty() && (str_contains(main_app.get_cod2mp_exe_path(), "-applaunch 10090") || check_if_file_path_exists(main_app.get_cod5mp_exe_path().c_str()))) {
    SetWindowText(app_handles.hwnd_cod5_path_edit, main_app.get_cod5mp_exe_path().c_str());
  } else {
    const char *found_cod5_path = find_call_of_duty_5_installation_path(false);
    if (found_cod5_path != nullptr && len(found_cod5_path) > 0 && (str_contains(found_cod5_path, "-applaunch 10090") || check_if_file_path_exists(found_cod5_path))) {
      SetWindowText(app_handles.hwnd_cod5_path_edit, found_cod5_path);
    } else {
      SetWindowText(app_handles.hwnd_cod5_path_edit, "");
    }
  }

  print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Please enter and verify the correctness of your game server's input settings by clicking on the ^3Test connection ^2button.\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  CenterWindow(app_handles.hwnd_configuration_dialog);
  ShowWindow(app_handles.hwnd_configuration_dialog, SW_SHOW);
  SetFocus(app_handles.hwnd_close_button);

  MSG wnd_msg{};
  while (GetMessage(&wnd_msg, nullptr, NULL, NULL) != 0) {
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
        /*{
          lock_guard<mutex> l{ mu };
          exit_flag.notify_all();
        }*/
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
    TranslateMessage(&wnd_msg);
    DispatchMessage(&wnd_msg);
  }

  return true;
}

void process_button_save_changes_click_event(HWND hwnd)
{

  static char msg_buffer[1024];
  string new_user_name;
  string new_server_name;
  string new_server_ip;
  uint_least16_t new_port{};
  string new_rcon_password;
  int new_refresh_time_period{ 5 };
  bool is_invalid_entry{};

  GetWindowText(app_handles.hwnd_user_name, msg_buffer, std::size(msg_buffer));
  if (stl::helper::len(msg_buffer) > 0) {
    new_user_name.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Admin name cannot be left empty!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  }

  if (!is_invalid_entry) {

    GetWindowText(app_handles.hwnd_server_name, msg_buffer, std::size(msg_buffer));
    if (stl::helper::len(msg_buffer) > 0) {
      new_server_name.assign(msg_buffer);
    } else {
      is_invalid_entry = true;
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Server name cannot be left empty!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }
  }

  if (!is_invalid_entry) {
    GetWindowText(app_handles.hwnd_server_ip_address, msg_buffer, std::size(msg_buffer));
    const auto ip_len{ stl::helper::len(msg_buffer) };
    unsigned long ip_key{};
    const string server_ip{ msg_buffer };
    if (ip_len > 0 && check_ip_address_validity(server_ip, ip_key)) {
      new_server_ip.assign(server_ip);
    } else {
      is_invalid_entry = true;
      const string user_message{ "^1You have entered an invalid IP address! ^5("s + msg_buffer + ")\n"s };
      print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }
  }

  if (!is_invalid_entry) {

    GetWindowText(app_handles.hwnd_server_port, msg_buffer, std::size(msg_buffer));
    if (int number{}; stl::helper::len(msg_buffer) > 0 && is_valid_decimal_whole_number(msg_buffer, number) && number >= 1024 && number <= 65535) {
      new_port = static_cast<uint_least16_t>(number);
    } else {
      is_invalid_entry = true;
      const string user_message{ "^1You have entered an invalid server port number! ^5("s + msg_buffer + ")\n^1Port number ^3must be >= ^11024 ^3and <= ^165535\n"s };
      print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }
  }

  if (!is_invalid_entry) {

    GetWindowText(app_handles.hwnd_rcon_password, msg_buffer, std::size(msg_buffer));
    if (stl::helper::len(msg_buffer) > 3) {
      new_rcon_password.assign(msg_buffer);
    } else {
      is_invalid_entry = true;
      const string user_message{ "^1You have entered an invalid rcon password! ^5("s + msg_buffer + ")\n^1Rcon password ^3must be at least ^14 characters ^3long.\n"s };
      print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }
  }

  if (!is_invalid_entry) {
    (void)snprintf(msg_buffer, std::size(msg_buffer), "\n^2Testing connection with the specified game server (^5%s^2) at ^5%s:%d ^2using the following ^5Tiny^6Rcon ^2settings:\n^1Admin name: ^5%s\n^1Server name: ^5%s\n^1Server IP address: ^5%s\n^1Server port number: ^5%d\n^1Server rcon password: ^5%s\n^1Refresh time period: ^5%d second(s)\n", new_server_name.c_str(), new_server_ip.c_str(), new_port, new_user_name.c_str(), new_server_name.c_str(), new_server_ip.c_str(), new_port, new_rcon_password.c_str(), new_refresh_time_period);
    print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer, is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    auto [test_result, game_name] = check_if_specified_server_ip_port_and_rcon_password_are_valid(new_server_ip.c_str(), new_port, new_rcon_password.c_str());
    if (test_result) {
      main_app.get_game_server().set_is_connection_settings_valid(true);
      set_admin_actions_buttons_active(TRUE, false);
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Testing connection SUCCEEDED!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    } else {
      main_app.get_game_server().set_is_connection_settings_valid(false);
      set_admin_actions_buttons_active(FALSE, false);
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Testing connection FAILED!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }

    (void)snprintf(msg_buffer, std::size(msg_buffer), "\n^2Saving the following ^5Tiny^6Rcon ^2settings:\n^1Admin name: ^5%s\n^1Server name: ^5%s\n^1Server IP address: ^5%s\n^1Server port number: ^5%d\n^1Server rcon password: ^5%s\n^1Refresh time period: ^5%d\n", new_user_name.c_str(), new_server_name.c_str(), new_server_ip.c_str(), new_port, new_rcon_password.c_str(), new_refresh_time_period);
    print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer, is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);

    main_app.set_username(std::move(new_user_name));
    main_app.set_game_name(game_name);
    main_app.set_game_server_name(std::move(new_server_name));
    main_app.get_game_server().set_server_ip_address(std::move(new_server_ip));
    main_app.get_game_server().set_server_port(new_port);
    main_app.get_game_server().set_rcon_password(std::move(new_rcon_password));
    main_app.get_game_server().set_check_for_banned_players_time_period(new_refresh_time_period);

    GetWindowText(app_handles.hwnd_cod1_path_edit, path_buffer, std::size(path_buffer));
    trim_in_place(path_buffer);
    if (str_contains(path_buffer, "-applaunch 2620") || check_if_file_path_exists(path_buffer)) {
      main_app.set_codmp_exe_path(path_buffer);
    }

    GetWindowText(app_handles.hwnd_cod2_path_edit, path_buffer, std::size(path_buffer));
    trim_in_place(path_buffer);
    if (str_contains(path_buffer, "-applaunch 2630") || check_if_file_path_exists(path_buffer)) {
      main_app.set_cod2mp_exe_path(path_buffer);
    }

    GetWindowText(app_handles.hwnd_cod4_path_edit, path_buffer, std::size(path_buffer));
    trim_in_place(path_buffer);
    if (str_contains(path_buffer, "-applaunch 7940") || check_if_file_path_exists(path_buffer)) {
      main_app.set_iw3mp_exe_path(path_buffer);
    }

    GetWindowText(app_handles.hwnd_cod5_path_edit, path_buffer, std::size(path_buffer));
    trim_in_place(path_buffer);
    if (str_contains(path_buffer, "-applaunch 10090") || check_if_file_path_exists(path_buffer)) {
      main_app.set_cod5mp_exe_path(path_buffer);
    }

    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Displayed server configuration settings have been successfully saved!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    MessageBox(hwnd, "Displayed server configuration settings have been successfully saved!", "Successfully saved server settings", MB_ICONINFORMATION | MB_OK);
    EnableWindow(app_handles.hwnd_main_window, TRUE);
    SetFocus(app_handles.hwnd_e_user_input);
    DestroyWindow(hwnd);
  } else {
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Failed to save displayed server configuration settings!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    MessageBox(hwnd, "Failed to save displayed server configuration settings!", "Failed to save server settings!", MB_ICONWARNING | MB_OK);
  }
}

void process_button_test_connection_click_event(HWND)
{
  static char msg_buffer[1024];
  string new_user_name;
  string new_server_name;
  string new_server_ip;
  uint_least16_t new_port{};
  string new_rcon_password;
  int new_refresh_time_period{ 5 };
  bool is_invalid_entry{};

  GetWindowText(app_handles.hwnd_user_name, msg_buffer, std::size(msg_buffer));
  if (stl::helper::len(msg_buffer) > 0) {
    new_user_name.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Admin name text field cannot be empty!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  }

  GetWindowText(app_handles.hwnd_server_name, msg_buffer, std::size(msg_buffer));
  if (stl::helper::len(msg_buffer) > 0) {
    new_server_name.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Server name text field cannot be empty!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  }


  GetWindowText(app_handles.hwnd_server_ip_address, msg_buffer, std::size(msg_buffer));
  const auto ip_len{ stl::helper::len(msg_buffer) };
  unsigned long ip_key{};
  const string server_ip{ msg_buffer };
  if (ip_len > 0 && check_ip_address_validity(server_ip, ip_key)) {
    new_server_ip.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    const string user_message1{ "^1You have entered an invalid IP address! ^5("s + msg_buffer + ")\n"s };
    print_colored_text(app_handles.hwnd_re_confirmation_message, user_message1.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  }


  GetWindowText(app_handles.hwnd_server_port, msg_buffer, std::size(msg_buffer));
  int number{};
  if (stl::helper::len(msg_buffer) > 0 && is_valid_decimal_whole_number(msg_buffer, number)) {
    new_port = static_cast<uint_least16_t>(number);
  } else {
    is_invalid_entry = true;
    const string user_message2{ "^1You have entered an invalid server port number! ^5("s + msg_buffer + ")\n"s };
    print_colored_text(app_handles.hwnd_re_confirmation_message, user_message2.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  }


  GetWindowText(app_handles.hwnd_rcon_password, msg_buffer, std::size(msg_buffer));
  if (stl::helper::len(msg_buffer) > 0) {
    new_rcon_password.assign(msg_buffer);
  } else {
    is_invalid_entry = true;
    const string user_message3{ "^1You have entered an invalid rcon password! ^5("s + msg_buffer + ")\n"s };
    print_colored_text(app_handles.hwnd_re_confirmation_message, user_message3.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
  }

  main_app.get_game_server().set_is_automatic_city_kick_enabled(Button_GetCheck(app_handles.hwnd_enable_city_ban) != BST_UNCHECKED);
  main_app.get_game_server().set_is_automatic_country_kick_enabled(Button_GetCheck(app_handles.hwnd_enable_country_ban) != BST_UNCHECKED);

  if (!is_invalid_entry) {
    (void)snprintf(msg_buffer, std::size(msg_buffer), "\n^2Testing connection with the specified game server (^5%s^2) at ^5%s:%d ^2using the following ^5Tiny^6Rcon ^2settings:\n^1Admin name: ^5%s\n^1Server name: ^5%s\n^1Server IP address: ^5%s\n^1Server port number: ^5%d\n^1Server rcon password: ^5%s\n^1Refresh time period: ^5%d second(s)\n", new_server_name.c_str(), new_server_ip.c_str(), new_port, new_user_name.c_str(), new_server_name.c_str(), new_server_ip.c_str(), new_port, new_rcon_password.c_str(), new_refresh_time_period);
    print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer, is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    auto [test_result, game_name] = check_if_specified_server_ip_port_and_rcon_password_are_valid(new_server_ip.c_str(), new_port, new_rcon_password.c_str());
    if (test_result) {
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Testing connection SUCCEEDED!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    } else {
      print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Testing connection FAILED!\n", is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }
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
    { "mp_downfal", "Downfal" },
    { "mp_hangar", "Hangar" },
    { "mp_makin", "Makin" },
    { "mp_outskirts", "Outskirts" },
    { "mp_roundhouse", "Roundhouse" },
    { "mp_seelow", "Seelow" },
    { "mp_suburban", "Upheava" }
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
    { "re", "Retrieva" },
    { "be", "Behind Enemy Lines" },
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
    { "dm", "Free for al" },
    { "war", "Team Deathmatch" },
    { "sab", "Sabotage" },
    { "sd", "Search and Destroy" },
    { "dom", "Domination" },
    { "koth", "Headquarters" }
  };

  static const map<string, string> cod5_full_gametype_names{
    { "dm", "Free for al" },
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
    { "Downfal", "mp_downfal" },
    { "Hangar", "mp_hangar" },
    { "Makin", "mp_makin" },
    { "Outskirts", "mp_outskirts" },
    { "Roundhouse", "mp_roundhouse" },
    { "Seelow", "mp_seelow" },
    { "Upheava", "mp_suburban" }
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
  const string ip{ main_app.get_game_server().get_server_ip_address() };
  const uint_least16_t port{ main_app.get_game_server().get_server_port() };
  const string rcon{ main_app.get_game_server().get_rcon_password() };
  const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(ip.c_str(), port, rcon.c_str());
  main_app.set_game_name(result.second);
  if (result.first) {
    main_app.get_game_server().set_is_connection_settings_valid(true);
    set_admin_actions_buttons_active(TRUE);
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Initialization of network settings has successfully completed.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  } else {
    main_app.get_game_server().set_is_connection_settings_valid(false);
    set_admin_actions_buttons_active(FALSE);
    print_colored_text(app_handles.hwnd_re_messages_data, "^3Initialization of network settings has failed!\n^5The provided ^1rcon password ^5is incorrect!\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  }

  return true;
}

void initiate_sending_rcon_status_command_now()
{
  if (main_app.get_game_server().get_is_connection_settings_valid()) {
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing rcon command: ^5!s\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5Sending status rcon command to the server.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  } else {
    print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing rcon command: ^5!gs\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    print_colored_text(app_handles.hwnd_re_messages_data, "^5Sending getstatus rcon command to the server.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  }
  atomic_counter.store(main_app.get_game_server().get_check_for_banned_players_time_period());
  RECT bounding_rectangle{
    screen_width - 270,
    screen_height - 105,
    screen_width - 5,
    screen_height - 85,
  };
  InvalidateRect(app_handles.hwnd_main_window, &bounding_rectangle, TRUE);
  is_refresh_players_data_event.store(true);
}

void prepare_players_data_for_display(const bool is_log_status_table)
{

  char buffer[256];
  size_t longest_name_length{ 32 };
  size_t longest_country_length{ 32 };

  auto &players = main_app.get_game_server().get_players_data();
  const size_t number_of_players{ main_app.get_game_server().get_number_of_players() };
  check_if_admins_are_online_and_get_admins_player_names(players, number_of_players);
  display_online_admins_information();

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
  g_re_match_information_contents = prepare_current_match_information();
  string match_information{ g_re_match_information_contents };


  if (is_log_status_table) {
    string admins_information{ online_admins_information };
    remove_all_color_codes(admins_information);
    remove_all_color_codes(match_information);
    log << admins_information << '\n';
    log << match_information;
  }

  const auto &pd = main_app.get_game_server().get_data_player_pid_color();
  const auto &sd = main_app.get_game_server().get_data_player_score_color();
  const auto &pgd = main_app.get_game_server().get_data_player_ping_color();
  const auto &ipd = main_app.get_game_server().get_data_player_ip_color();
  const auto &gd = main_app.get_game_server().get_data_player_geoinfo_color();

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
      string server_message{ main_app.get_game_server().get_server_message() };
      remove_all_color_codes(server_message);
      log << server_message;
      const size_t printed_chars_count = get_number_of_characters_without_color_codes(main_app.get_game_server().get_server_message().c_str());
      const string filler{
        string(decoration_line.length() - 1 - printed_chars_count, ' ') + "|\n"s
      };
      log << filler;
    }
  } else {

    for (size_t i{}; i < number_of_players; ++i) {

      const player_data &p{ players[i] };

      snprintf(buffer, std::size(buffer), "%s%d", pd.c_str(), p.pid);
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
        string name{ p.player_name };
        remove_all_color_codes(name);
        const size_t printed_name_char_count{ name.length() };
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
    log_message(log.str(), is_log_datetime::yes);
  }
}

void prepare_players_data_for_display_of_getstatus_response(const bool is_log_status_table)
{
  char buffer[256];
  size_t longest_name_length{ 32 };

  auto &players = main_app.get_game_server().get_players_data();
  const size_t number_of_players{ main_app.get_game_server().get_number_of_players() };
  check_if_admins_are_online_and_get_admins_player_names(players, number_of_players);
  display_online_admins_information();

  if (number_of_players > 0) {
    sort_players_data(players, type_of_sort);
    if (!players.empty()) {
      longest_name_length =
        std::max(longest_name_length, find_longest_player_name_length(players, false, number_of_players));
    }
  }

  ostringstream log;

  g_re_match_information_contents = prepare_current_match_information();
  string match_information{ g_re_match_information_contents };

  if (is_log_status_table) {
    string admins_information{ online_admins_information };
    remove_all_color_codes(admins_information);
    remove_all_color_codes(match_information);
    log << admins_information << '\n';
    log << match_information;
  }

  const auto &pd = main_app.get_game_server().get_data_player_pid_color();
  const auto &sd = main_app.get_game_server().get_data_player_score_color();
  const auto &pgd = main_app.get_game_server().get_data_player_ping_color();

  const string decoration_line(24 + longest_name_length, '=');
  if (is_log_status_table) {
    log << string{ "\n"s + decoration_line + "\n"s };
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

      snprintf(buffer, std::size(buffer), "%s%d", pd.c_str(), p.pid);
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
        const size_t printed_name_char_count{ name.length() };
        if (printed_name_char_count < longest_name_length) {
          log << string(longest_name_length - printed_name_char_count, ' ');
        }
        log << "|\n";
      }
    }
  }

  if (is_log_status_table) {
    log << string{ decoration_line + "\n"s };
    log_message(log.str(), is_log_datetime::yes);
  }
}

size_t get_file_size_in_bytes(const char *file_path) noexcept
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

  if (t_c <= 0) {
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

const char *get_current_short_month_name(const size_t month_index) noexcept
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

void correct_truncated_player_names(const char *ip_address, const uint_least16_t port_number, const char *rcon_password)
{
  connection_manager cm;
  string reply;
  cm.send_and_receive_rcon_data("getstatus", reply, ip_address, port_number, rcon_password, true, false);

  if (reply.find("statusResponse") != string::npos && reply[4] == 's') {

    constexpr size_t off{ 20U };

    const size_t new_line_pos = reply.find('\n', off);

    unordered_map<string, int> unique_player_names;

    auto &players_data = main_app.get_game_server().get_players_data();

    size_t start{ new_line_pos + 1 }, pl_index{};

    while (start < reply.length()) {

      // Extracting player's score value from received UDP data packet for
      // getstatus command
      while (' ' == reply[start])
        ++start;
      size_t last{ start };
      while (reply[last] != ' ')
        ++last;
      string temp_score{ reply.substr(start, last - start) };
      trim_in_place(temp_score);
      int player_score;
      if (!is_valid_decimal_whole_number(temp_score, player_score))
        player_score = 0;

      // Extracting player_name information from received UDP data packet
      // for getstatus command
      start = last;
      while (reply[start] != '"')
        ++start;
      last = ++start;
      while (reply[last] != '"')
        ++last;

      string unique_player_name{ reply.substr(start, last - start) };
      trim_in_place(unique_player_name);
      if (unique_player_name == players_data[pl_index].player_name) {
        players_data[pl_index].score = player_score;
      }
      unique_player_names[std::move(unique_player_name)]++;
      ++pl_index;
      start = ++last;
      if (reply[start] == '\n')
        ++start;
    }

    vector<pair<size_t, string>> truncated_player_names;

    for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {

      auto &p = players_data[i];

      string key{ p.player_name };
      const auto found_iter = unique_player_names.find(key);
      if (found_iter != end(unique_player_names)) {
        --found_iter->second;
        if (found_iter->second <= 0)
          unique_player_names.erase(found_iter);
      } else {
        truncated_player_names.emplace_back(i, std::move(key));
      }
    }

    if (!truncated_player_names.empty()) {

      for (const auto &pd : truncated_player_names) {
        for (auto &unique_player_name : unique_player_names) {
          if (unique_player_name.second > 0) {

            if ((unique_player_name.first.length() - pd.second.length() <= 2) && (unique_player_name.first.ends_with(pd.second) || unique_player_name.first.starts_with(pd.second))) {
              // print_message_about_corrected_player_name(app_handles.hwnd_re_messages_data, pd.second.c_str(), unique_player_name.first.c_str());
              strcpy_s(players_data[pd.first].player_name, std::size(players_data[pd.first].player_name), unique_player_name.first.c_str());
              --unique_player_name.second;
              break;
            }

            if (unique_player_name.first.length() > pd.second.length() && !pd.second.empty()) {
              size_t i{}, k{ unique_player_name.first.length() }, j{ pd.second.length() };

              for (; i < pd.second.length(); ++i) {
                if (pd.second[i] != unique_player_name.first[i])
                  break;
              }

              for (; j >= i; --j, --k) {
                if (pd.second[j - 1] != unique_player_name.first[k - 1])
                  break;
              }

              if (j - i <= 1) {
                // print_message_about_corrected_player_name(app_handles.hwnd_re_messages_data, pd.second.c_str(), unique_player_name.first.c_str());
                strcpy_s(players_data[pd.first].player_name, std::size(players_data[pd.first].player_name), unique_player_name.first.c_str());
                --unique_player_name.second;
                break;
              }
            }
          }
        }
      }
    }
  }
}

void print_message_about_corrected_player_name(HWND re_hwnd, const char *truncated_name, const char *corrected_name) noexcept
{
  if (truncated_name && corrected_name) {
    char buffer[128]{};
    (void)snprintf(buffer, std::size(buffer), "^2Corrected truncated player name from ^7%s ^2to ^7%s\n", truncated_name, corrected_name);
    print_colored_text(re_hwnd, buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
  }
}

void set_admin_actions_buttons_active(const BOOL is_enable, const bool is_reset_to_default_sort_mode) noexcept
{
  EnableWindow(app_handles.hwnd_button_warn, is_enable);
  EnableWindow(app_handles.hwnd_button_kick, is_enable);
  EnableWindow(app_handles.hwnd_button_tempban, is_enable);
  EnableWindow(app_handles.hwnd_button_ipban, is_enable);
  EnableWindow(app_handles.hwnd_say_button, is_enable);
  EnableWindow(app_handles.hwnd_tell_button, is_enable);
  EnableWindow(app_handles.hwnd_button_load, is_enable);
  set_available_sort_methods(is_enable, is_reset_to_default_sort_mode);
}

void set_available_sort_methods(const bool is_admin, const bool is_reset_to_default_sort_mode)
{
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_RESETCONTENT, 0, 0);
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by pid in asc. order"));
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by pid in desc. order"));
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by score in asc. order"));
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by score in desc. order"));
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by ping in asc. order"));
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by ping in desc. order"));
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by name in asc. order"));
  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by name in desc. order"));
  if (is_admin) {
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by IP in asc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by IP in desc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by geoinfo in asc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Sort by geoinfo in desc. order"));
    if (is_reset_to_default_sort_mode) {
      type_of_sort = sort_type::geo_asc;
    }
  } else if (is_reset_to_default_sort_mode) {
    type_of_sort = sort_type::score_desc;
  }

  SendMessage(app_handles.hwnd_combo_box_sortmode, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(sort_type_to_sort_names_dict.at(type_of_sort).c_str()));
}

std::pair<bool, std::string> extract_7z_file_to_specified_path(const char *compressed_7z_file_path, const char *destination_path)
{
  try {
    using namespace bit7z;
    Bit7zLibrary lib{ "7za.dll" };
    BitFileExtractor extractor{ lib, BitFormat::SevenZip };
    extractor.setOverwriteMode(bit7z::OverwriteMode::Overwrite);
    extractor.extract(compressed_7z_file_path, destination_path);
    return make_pair(true, string{});

  } catch (const std::exception &ex) {
    return make_pair(false, ex.what());
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

  oss << (main_app.get_game_server().get_is_automatic_city_kick_enabled() ? "\n^5The ^1automatic city ban ^5feature is ^2currently enabled^5.\n" : "\n^5The ^1automatic city ban ^5feature is ^1currently disabled^5.\n");
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

  oss << (main_app.get_game_server().get_is_automatic_city_kick_enabled() ? "\n^5The ^1automatic country ban ^5feature is ^2currently enabled^5.\n" : "\n^5The ^1automatic country ban ^5feature is ^1currently disabled^5.\n");

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

void check_if_admins_are_online_and_get_admins_player_names(const std::vector<player_data> &players, size_t no_of_online_players)
{
  no_of_online_players = std::min(no_of_online_players, players.size());
  for (auto &u : main_app.get_users()) {
    u->is_online = false;
    for (size_t i{}; i < no_of_online_players; ++i) {
      if (u->ip_address == players[i].ip_address) {
        u->is_online = true;
        u->player_name = players[i].player_name;
        break;
      }
    }
  }
}

bool save_current_user_data_to_json_file(const char *json_file_path) noexcept
{

  std::ofstream config_file{ json_file_path };

  if (!config_file) {
    const string re_msg{ "Error saving tinyrcon admin's data\nto specified json file path ("s + json_file_path + ".\nPlease make sure that the main application folder's properties are not set to read-only mode!"s };
    show_error(app_handles.hwnd_main_window, re_msg.c_str(), 0);
    return false;
  }

  auto &me = main_app.get_current_user();
  const string user_name{ remove_disallowed_character_in_string(main_app.get_username()) };
  config_file << format(R"({}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{})", user_name, (me->is_admin ? "true" : "false"), (me->is_logged_in ? "true" : "false"), (me->is_online ? "true" : "false"), me->ip_address, me->geo_information, me->last_login_time_stamp, me->last_logout_time_stamp, me->no_of_logins, me->no_of_warnings, me->no_of_kicks, me->no_of_tempbans, me->no_of_guidbans, me->no_of_ipbans, me->no_of_iprangebans, me->no_of_citybans, me->no_of_countrybans);


  config_file << flush;
  return true;
}

void load_tinyrcon_client_user_data(const char *file_path)
{
  ifstream configFile{ file_path };

  if (!configFile) {
    save_current_user_data_to_json_file(file_path);
    configFile.open(file_path, std::ios_base::in);
  }

  string data;
  getline(configFile, data);
  auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
  for (auto &part : parts)
    stl::helper::trim_in_place(part);

  auto &u = main_app.get_current_user();

  u->user_name = std::move(parts[0]);
  u->is_admin = parts[1] == "true";
  u->is_logged_in = true;
  u->is_online = false;
  unsigned long guid{};
  if (check_ip_address_validity(parts[4], guid)) {
    player_data admin{};
    convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), parts[4], admin);
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
  u->no_of_logins = stoul(parts[8]);
  u->no_of_warnings = stoul(parts[9]);
  u->no_of_kicks = stoul(parts[10]);
  u->no_of_tempbans = stoul(parts[11]);
  u->no_of_guidbans = stoul(parts[12]);
  u->no_of_ipbans = stoul(parts[13]);
  u->no_of_iprangebans = stoul(parts[14]);
  u->no_of_citybans = stoul(parts[15]);
  u->no_of_countrybans = stoul(parts[16]);
}

bool validate_admin_and_show_missing_admin_privileges_message(const bool is_show_message_box, const is_log_message log_message, const is_log_datetime log_date_time)
{
  if (!main_app.get_game_server().get_is_connection_settings_valid()) {
    string warning_msg{ format("^7{}^3, you need to have the correct ^1rcon password ^3to be able to execute ^1admin-level ^3commands.\n", main_app.get_username()) };
    print_colored_text(app_handles.hwnd_re_messages_data, warning_msg.c_str(), is_append_message_to_richedit_control::yes, log_message, log_date_time);
    if (is_show_message_box) {
      remove_all_color_codes(warning_msg);
      MessageBoxA(app_handles.hwnd_main_window, "Warning", warning_msg.c_str(), MB_ICONWARNING | MB_OK);
    }
    return false;
  }

  return true;
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
  return admin_name;
}

void replace_br_with_new_line(std::string &message)
{
  for (size_t next{ string::npos }; (next = message.find("{{br}}")) != string::npos;) {
    message.replace(next, 6, "\n");
  }
}

void get_first_valid_ip_address_from_ip_address_range(std::string ip_range, player_data &pd)
{

  if (ip_range.ends_with("*.*")) {
    ip_range.replace(ip_range.length() - 3, 3, "1.1");
  } else if (ip_range.ends_with(".*")) {
    ip_range.back() = '1';
  }

  strcpy_s(pd.ip_address, std::size(pd.ip_address), ip_range.c_str());
  convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), ip_range, pd);
}

bool run_executable(const char *file_path_for_executable)
{
  PROCESS_INFORMATION ProcessInfo{};
  STARTUPINFO StartupInfo{};
  StartupInfo.cb = sizeof(StartupInfo);

  if (CreateProcessA(file_path_for_executable, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &StartupInfo, &ProcessInfo)) {
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
    return true;
  }

  return false;
}

void restart_tinyrcon_client()
{
  const string tinyrcon_exe_file_path{ main_app.get_current_working_directory() + "TinyRcon.exe" };

  if (run_executable(tinyrcon_exe_file_path.c_str())) {
    is_terminate_program.store(true);
    PostQuitMessage(0);
    CloseWindow(app_handles.hwnd_main_window);
    DestroyWindow(app_handles.hwnd_main_window);
    if (pr_info.hProcess != nullptr) {
      CloseHandle(pr_info.hProcess);
      pr_info.hProcess = nullptr;
    }
    if (pr_info.hThread != nullptr) {
      CloseHandle(pr_info.hThread);
      pr_info.hThread = nullptr;
    }
    ExitProcess(0);
  }
}

size_t get_random_number()
{
  static std::mt19937_64 rand_engine(get_current_time_stamp());
  static std::uniform_int_distribution<size_t> number_range(1000ULL, std::numeric_limits<size_t>::max());
  return number_range(rand_engine);
}