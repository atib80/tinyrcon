#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#define NOMINMAX

#include "tiny_rcon_client_application.h"
#include "md5.h"
#include "stack_trace_element.h"
#include "resource.h"
#include "simple_grid.h"

#include <codecvt>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <Psapi.h>
#include <regex>
#include <Shlobj.h>
#include <windowsx.h>
#include <nlohmann/json.hpp>
#include <bit7z.hpp>
#include <bitfilecompressor.hpp>
#include <bit7zlibrary.hpp>
#include <bitextractor.hpp>
#include <bitexception.hpp>
#include <bittypes.hpp>

// Call of duty steam appid: 2620
// Call of duty 2 steam appid: 2630
// Call of duty 4: Modern Warfare steam appid: 7940
// Call of duty 5: World at war steam appid: 10090

extern const DWORD cod1_steam_appid{2620};
extern const DWORD cod2_steam_appid{2630};
extern const DWORD cod4_steam_appid{7940};
extern const DWORD cod5_steam_appid{10090};

#if defined(_DEBUG)
#pragma comment(lib, "bit7z_d.lib")
#else
#pragma comment(lib, "bit7z.lib")
#endif

using namespace std;
using namespace stl::helper;
using namespace std::filesystem;
namespace fs = std::filesystem;

using stl::helper::trim_in_place;

extern const string program_version;
extern tiny_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;
extern shared_ptr<tiny_rcon_client_user> me;

extern const char *prompt_message;
extern const char *refresh_players_data_fmt_str;
// extern PROCESS_INFORMATION pr_info;
extern sort_type type_of_sort;

extern WNDCLASSEX wcex_confirmation_dialog, wcex_configuration_dialog;
extern HFONT font_for_players_grid_data;
extern RECT client_rect;

extern const int screen_width;
extern const int screen_height;
extern size_t selected_server_index;
extern int selected_player_row;
extern int selected_player_col;
extern int selected_server_row;
extern int selected_server_col;
extern const char *user_help_message;
extern const size_t max_players_grid_rows{64};
extern const size_t max_servers_grid_rows{4096};
std::atomic<bool> is_terminate_program{false};
std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window{false};
extern volatile std::atomic<bool> is_refresh_players_data_event;
extern volatile std::atomic<bool> is_refreshed_players_data_ready_event;
extern volatile std::atomic<bool> is_display_permanently_banned_players_data_event;
extern volatile std::atomic<bool> is_display_temporarily_banned_players_data_event;
extern volatile std::atomic<bool> is_display_banned_cities_data_event;
extern volatile std::atomic<bool> is_display_banned_countries_data_event;
extern volatile std::atomic<bool> is_refreshing_game_servers_data_event;
extern volatile std::atomic<bool> is_display_geoinformation_data_for_players;
extern std::atomic<int> admin_choice;
extern std::string admin_reason;

extern volatile atomic<size_t> atomic_counter;

extern HBITMAP g_hBitMap;

bool sort_by_pid_asc{true};
bool sort_by_score_asc{false};
bool sort_by_ping_asc{true};
bool sort_by_name_asc{true};
bool sort_by_ip_asc{true};
bool sort_by_geo_asc{true};

extern bool is_process_combobox_item_selection_event;
extern bool is_first_left_mouse_button_click_in_reason_edit_control;

vector<string> commands_history;
size_t commands_history_index{};

string g_re_match_information_contents;
string g_message_data_contents;
string previous_map;
string online_admins_information;

HIMAGELIST hImageList{};
row_of_player_data_to_display displayed_players_data[max_players_grid_rows]{};

static char path_buffer[32768];

extern const std::regex ip_address_and_port_regex{R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(-?\d+))"};

extern const std::unordered_map<char, COLORREF> colors{
    {'0', color::black}, {'1', color::red},    {'2', color::green}, {'3', color::yellow}, {'4', color::blue},
    {'5', color::cyan},  {'6', color::purple}, {'7', color::black}, {'8', color::grey},   {'9', color::magenta},
};

extern const std::unordered_map<char, COLORREF> rich_edit_colors{
    {'0', color::white}, {'1', color::red},    {'2', color::green}, {'3', color::yellow}, {'4', color::blue},
    {'5', color::cyan},  {'6', color::purple}, {'7', color::white}, {'8', color::grey},   {'9', color::magenta},
};

extern const std::unordered_map<string, sort_type> sort_mode_names_dict{
    {"Sort by pid in asc. order", sort_type::pid_asc},     {"Sort by pid in desc. order", sort_type::pid_desc},
    {"Sort by score in asc. order", sort_type::score_asc}, {"Sort by score in desc. order", sort_type::score_desc},
    {"Sort by ping in asc. order", sort_type::ping_asc},   {"Sort by ping in desc. order", sort_type::ping_desc},
    {"Sort by name in asc. order", sort_type::name_asc},   {"Sort by name in desc. order", sort_type::name_desc},
    {"Sort by IP in asc. order", sort_type::ip_asc},       {"Sort by IP in desc. order", sort_type::ip_desc},
    {"Sort by geoinfo in asc. order", sort_type::geo_asc}, {"Sort by geoinfo in desc. order", sort_type::geo_desc}};

extern const std::unordered_map<sort_type, string> sort_type_to_sort_names_dict{
    {sort_type::pid_asc, "Sort by pid in asc. order"},     {sort_type::pid_desc, "Sort by pid in desc. order"},
    {sort_type::score_asc, "Sort by score in asc. order"}, {sort_type::score_desc, "Sort by score in desc. order"},
    {sort_type::ping_asc, "Sort by ping in asc. order"},   {sort_type::ping_desc, "Sort by ping in desc. order"},
    {sort_type::name_asc, "Sort by name in asc. order"},   {sort_type::name_desc, "Sort by name in desc. order"},
    {sort_type::ip_asc, "Sort by IP in asc. order"},       {sort_type::ip_desc, "Sort by IP in desc. order"},
    {sort_type::geo_asc, "Sort by geoinfo in asc. order"}, {sort_type::geo_desc, "Sort by geoinfo in desc. order"}};

extern const std::unordered_map<int, sort_type> sort_mode_col_index{{0, sort_type::pid_asc},  {1, sort_type::score_asc},
                                                                    {2, sort_type::ping_asc}, {3, sort_type::name_asc},
                                                                    {4, sort_type::ip_asc},   {5, sort_type::geo_asc}};

extern const char *black_bg_color{"[[!black!]]"};
extern const char *grey_bg_color{"[[!grey!]]"};
extern const size_t black_bg_color_length{stl::helper::len(black_bg_color)};
extern const size_t grey_bg_color_length{stl::helper::len(grey_bg_color)};

extern const map<string, string> user_commands_help{
    {"!cls", "^1!cls ^7-> ^3clears the console screen."},
    {"!colors", "^1!colors ^7-> ^5changes colors of various displayed table entries and "
                "game information."},
    {"!report", "^1!report 12 optional_reason ^7-> ^3reports player (whose ^1pid ^3number "
                "is equal to specified ^1'pid'^3) with reason ^1'optional_reason'."},
    {"!unreport", "^5!unreport ip_address optional_reason ^7-> ^5removes previously reported "
                  "player whose ^1IP address is ^1'ip_address'.\n^3You can also specify an "
                  "optional reason."},
    {"!reports", "^1!reports optional_number ^7-> ^3displays ^1all ^3or last "
                 "^1'optional_number' ^3reported players."},
    {"!warn", "^1!warn player_pid optional_reason ^7-> ^5warns the player whose pid "
              "number is equal to specified player_pid.\n A player who's been warned 2 "
              "times gets automatically kicked off the server."},
    {"!w", "^1!w player_pid optional_reason ^7-> ^3warns the player whose pid number "
           "is equal to specified player_pid.\n A player who's been warned 2 times "
           "gets automatically kicked off the server."},
    {"!kick", "^1!kick player_pid optional_reason ^7-> ^5kicks the player whose pid "
              "number is equal to "
              "specified player_pid."},
    {"!k", "^1!k player_pid optional_reason ^7-> ^3kicks the player whose pid number "
           "is equal to specified "
           "player_pid."},
    {"!tempban", "^1!tempban player_pid time_duration optional_reason ^7-> ^5temporarily "
                 "bans (for time_duration hours, default 24 hours) "
                 "IP address of player whose pid = ^112."},
    {"!tb", "^1!tb player_pid time_duration optional_reason ^7-> ^3temporarily bans "
            "(for time_duration hours, default 24 hours) "
            "IP address of player whose pid = ^112."},
    {"!ban", "^1!ban player_pid optional_reason ^7-> ^5bans the player whose pid number "
             "is equal to "
             "specified player_pid"},
    {"!b", "^1!b player_pid optional_reason ^7-> ^3bans the player whose pid number "
           "is equal to specified "
           "player_pid"},
    {"!gb", "^1!gb player_pid optional_reason ^7-> ^5permanently bans player's IP "
            "address whose pid number is "
            "equal to specified player_pid."},
    {"!globalban", "^1!globalban player_pid optional_reason ^7-> ^3permanently bans player's "
                   "IP address whose pid "
                   "number is equal to specified player_pid."},
    {"!status", "^1!status ^7-> ^5retrieves current game state (players' data) from the "
                "server "
                "(pid, score, ping, name, guid, ip, qport, rate"},
    {"!s", "^1!s name ^7-> ^3displays stats data for player with partially or fully "
           "specified player name ^1name. "
           "^3You can omit ^1color codes ^3in partially or fully specified ^1player "
           "name^3."},
    {"!t", "^1!t public message ^7-> ^5sends public message to all logged in "
           "^1admins."},
    {"!y", "^1!y username private message ^7-> ^3sends ^1private message ^3to ^1admin "
           "^3whose username is ^1username."},
    {"!sort", "^1!sort player_data asc|desc ^7-> ^5examples: !sort pid asc, !sort pid "
              "desc, "
              "!sort score asc, !sort score desc\n !sort ping asc, !sort ping desc, "
              "!sort name asc, !sort name desc, !sort ip asc, !sort ip desc, !sort geo "
              "asc, !sort geo desc"},
    {"!list", "^1!list user|rcon ^7-> ^3shows a list of available user or rcon level "
              "commands "
              "that are available for admins\n to type into the console program."},
    {"!ranges", "^1!ranges ^7-> ^5displays data about all ^1IP address range bans."},
    {"!bans", "^1!bans ^7-> ^3displays a list of permanently banned IP addresses."},
    {"!tempbans", "^1!tempbans ^7-> ^5displays a list of temporarily banned IP addresses."},
    {"!banip", "^1!banip pid|valid_ip_address optional_reason ^7-> ^3bans player whose "
               "^1pid number ^3or "
               "^1IP address ^3is equal to specified ^1pid ^3or ^1valid_ip_address^3, "
               "respectively."},
    {"!addip", "^1!addip pid|valid_ip_address optional_reason ^7-> ^5bans player whose "
               "^1pid number ^5or "
               "^1IP address ^5is equal to specified ^1pid ^5or ^1valid_ip_address, "
               "respectively."},
    {"!ub", "^1!ub valid_ip_address optional_reason ^7-> ^3removes temporarily and|or "
            "permanently banned IP address "
            "^1valid_ip_address."},
    {"!unban", "^1!unban valid_ip_address optional_reason ^7-> ^5removes temporarily "
               "and|or permanently banned IP address "
               "^1valid_ip_address."},
    {"!m", "^1!m map_name game_type ^7-> ^3loads map ^1map_name ^3in specified game "
           "type ^1game_type."},
    {"!maps", "^1!maps ^7-> ^5displays all available ^3playable maps. "},
    {"!c", "^1!c IP:PORT ^7-> ^3launches your Call of Duty game and connects to "
           "currently configured game server or optionally specified game server "
           "address ^1IP:PORT."},
    {"!cp", "^1!cp IP:PORT ^7-> ^5launches your Call of Duty game and connects to "
            "currently configured game server or optionally specified game server "
            "address ^1IP:PORT ^5using a private slot."},
    {"!config", "^1!config [rcon|private|address|name] "
                "[new_rcon_password|new_private_password|new_server_address|new_name]\n "
                "^7-> ^3you change tinyrcon's ^1rcon ^3or ^1private slot password^3, "
                "registered server ^1IP:port ^3address or your ^1username ^3using this "
                "command.\nFor example ^1!config rcon abc123 ^3changes currently used "
                "^1rcon_password ^3to ^1abc123^3\n ^1!config private abc123 ^3changes "
                "currently used ^1sv_privatepassword ^3to ^1abc123^3\n ^1!config address "
                "123.101.102.103:28960 ^3changes currently used server ^1IP:port ^3to "
                "^1123.101.102.103:28960\n ^1!config name Administrator ^3changes "
                "currently used ^1username ^3to ^1Administrator"},
    {"!messages", "^1!messages on|off ^7-> ^5turns ^3on^5|^3off ^5messages for temporarily "
                  "and permanently banned players."},
    {"!banned cities", "^1banned cities ^7-> ^3displays all currently ^1banned cities."},
    {"!banned countries", "^1!banned countries ^7-> ^5displays all currently ^1banned countries."},
    {"!ecb", "^1!ecb ^7-> ^3enables ^5Tiny^6Rcon's ^3country ban feature."},
    {"!dcb", "^1!dcb ^7-> ^5disables ^5Tiny^6Rcon's ^5country ban feature."},
    {"!bancountry", "^1!bancountry country name ^7-> ^3Adds player's ^1country ^3to list of "
                    "banned countries."},
    {"!unbancountry", "^1unbancountry country name ^7-> ^5Removes player's ^1country ^5from list "
                      "of banned countries."},
    {"!egb", "^1!egb ^7-> ^3enables ^5Tiny^6Rcon's ^3city ban feature."},
    {"!dgb", "^1!dgb ^7-> ^5disables ^5Tiny^6Rcon's ^5city ban feature."},
    {"!bancity", "^1!bancity city name  ^7->  ^3adds player's ^1city ^3to list of banned "
                 "cities."},
    {"!unbancity", "^1!unbancity city name ^7-> ^5removes player's ^1city ^5from list of "
                   "banned cities."},
    {"!stats", "^1!stats ^7-> ^3displays up-to-date ^5Tiny^6Rcon ^1admins' ^3related "
               "stats data."},
    {"!banrange", "^1!banrange pid|ip_address_range optional_reason ^7-> ^5bans IP address "
                  "range of player whose ^1pid number ^5or\n ^1IP address range ^5is equal "
                  "to specified ^1pid ^5or ^1ip_address_range^5, respectively."},
    {"!unbanrange", "^1!unbanrange ip_address_range optional_reason ^7-> ^3removes previously "
                    "banned IP address range ^1ip_address_range."},
    {"!admins", "^1!admins ^7-> ^5Displays information about all registered admins."},
    {"!protectip", "^1!protectip pid|ip_address optional_reason ^7-> ^3protects player's ^1IP "
                   "address ^3whose ^1pid number ^3or "
                   "^1IP address ^3is equal to specified ^1pid ^3or ^1ip_address^3, "
                   "respectively."},
    {"!unprotectip", "^1!unprotectip pid|ip_address optional_reason ^7-> ^5removes protected "
                     "^1IP address ^5of player whose ^1pid number ^5or "
                     "^1IP address ^5is equal to specified ^1pid ^5or ^1ip_address^5, "
                     "respectively."},
    {"!protectiprange", "^1!protectiprange pid|ip_address_range optional_reason ^7-> ^3protects "
                        "player's ^1IP address range ^3whose ^1pid number ^3or "
                        "^1IP address range ^3is equal to specified ^1pid ^3or "
                        "^1ip_address_range^3, respectively."},
    {"!unprotectiprange", "^1!unprotectiprange pid|ip_address_range optional_reason ^7-> ^5removes "
                          "protected ^1IP address range ^5of player whose ^1pid number ^5or "
                          "^1IP address range ^5is equal to specified ^1pid ^5or "
                          "^1ip_address_range^5, respectively."},
    {"!protectcity", "^1!protectcity pid|city_name ^7-> ^3protects player's ^1city ^3whose "
                     "^1pid number ^3or "
                     "^1city name ^3is equal to specified ^1pid ^3or ^1city_name^3, "
                     "respectively."},
    {"!unprotectcity", "^1!unprotectcity pid|city_name ^7-> ^5removes protected ^1city ^5of "
                       "player whose ^1pid number ^5or "
                       "^1city name ^5is equal to specified ^1pid ^5or ^1city_name^5, "
                       "respectively."},
    {"!protectcountry", "^1!protectcountry pid|country_name ^7-> ^3protects player's ^1country "
                        "^3whose ^1pid number ^3or "
                        "^1country name ^3is equal to specified ^1pid ^3or ^1valid_ip_address^3, "
                        "respectively."},
    {"!unprotectcountry", "^1!unprotectcountry pid|country_name ^7-> ^5removes protected ^1IP "
                          "address ^5of player whose ^1pid number ^5or "
                          "^1country name ^5is equal to specified ^1pid ^5or ^1country_name^5, "
                          "respectively."},
    /*{ "!patch", "^1!patch game_version_number ^7-> ^3patches your game (^1Call
       of duty 2 English version only^3) ^3to specified
       ^1game_version_number\n^3Allowed version numbers for ^1game_version_number
       ^3are ^11.0 1.01 1.2 ^3and ^11.3\n^5Example: ^1!patch 1.3" },*/
    {"!banname", "^1!banname pid|player_name optional_reason ^7-> ^5bans ^1player name ^5of "
                 "online player whose ^1pid ^5number is equal to specified ^1'pid' ^5or "
                 "bans specified player name ^1'player_name'"},
    {"!unbanname", "^1!unbanname player_name optional_reason ^7-> ^3removes previously banned "
                   "^1'player name'"},
    {"!names", "^1!names player_name optional_reason ^7-> ^5displays all currently banned "
               "names."},

    {"!rc", "^1!rc custom_rcon_command [optional_parameters] ^7-> ^3sends "
            "^1custom_rcon_command [optional_parameters] ^3to game server.\n  "
            "^7Examples: ^1!rc map_restart ^7| ^1!rc fast_restart ^7| ^1!rc "
            "sv_iwdnames"}};

extern const unordered_set<string> user_commands_set{"list",
                                                     "!list",
                                                     "help",
                                                     "!help",
                                                     "!l",
                                                     "!h",
                                                     "!admins",
                                                     "!c",
                                                     "!cp",
                                                     "cls",
                                                     "!cls",
                                                     "!colors",
                                                     "colors",
                                                     "!r",
                                                     "!report",
                                                     "!ur",
                                                     "!unreport",
                                                     "!reports",
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
                                                     "!bn",
                                                     "!banname",
                                                     "!ubn",
                                                     "!unbanname",
                                                     "tempbans",
                                                     "!tempbans",
                                                     "bans",
                                                     "!bans",
                                                     "ranges",
                                                     "!ranges",
                                                     "names",
                                                     "!names",
                                                     "!m",
                                                     "!map",
                                                     "maps",
                                                     "!maps",
                                                     "!geoinfo",
                                                     "!status",
                                                     "!s",
                                                     "t",
                                                     "!t",
                                                     "y",
                                                     "!y",
                                                     "sort",
                                                     "!sort",
                                                     "list",
                                                     "!list",
                                                     "!addip",
                                                     "!banip",
                                                     "!ub",
                                                     "!unban",
                                                     "gs",
                                                     "!gs",
                                                     "!getstatus",
                                                     "!rc",
                                                     "!config",
                                                     "!stats",
                                                     "tp",
                                                     "!tp",
                                                     "tpy",
                                                     "!tpy",
                                                     "tpm",
                                                     "!tpm",
                                                     "tpd",
                                                     "!tpd",
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
                                                     "!unprotectcountry",
                                                     "!protectedipaddresses",
                                                     "!protectedipaddressranges",
                                                     "!protectedcities",
                                                     "!protectedcountries"};

extern const unordered_set<string> rcon_status_commands{"status", "!status"};

extern const unordered_set<string> non_rcon_commands{"gs", "!gs", "getstatus", "!getstatus", "getinfo"};

extern const map<string, string> rcon_commands_help{
    {"status", "^1status ^7-> ^3Retrieves current game state (^1players' data^3) from the "
               "server "
               "^3(^1pid, score, ping, name, guid, ip, qport, rate^3)"},
    {"clientkick", "^1clientkick player_pid optional_reason ^7-> ^5Kicks player whose ^1pid "
                   "^5number is equal to "
                   "specified ^1player_pid"},
    {"kick", "^1kick name optional_reason ^7-> ^3Kicks player whose ^1name ^3is equal "
             "to specified ^1name"},
    {"onlykick", "^1onlykick name_no_color_codes optional_reason ^7-> ^5Kicks player whose "
                 "^1name ^5is equal to "
                 "specified ^1name_no_color_codes"},
    {"banclient", "^1banclient player_pid optional_reason ^7-> ^3Bans player whose ^1pid "
                  "^3number is equal to "
                  "specified ^1player_pid."},
    {"banuser", "^1banuser name optional_reason ^7-> ^5Bans player whose ^1name ^5is equal "
                "to specified ^1name."},
    {"tempbanclient", "^1tempbanclient player_pid optional_reason ^7-> ^3Temporarily bans player "
                      "whose ^1pid ^3number is "
                      "equal to specified ^1player_pid"},
    {"tempbanuser", "^1tempbanuser name optional_reason ^7-> ^5Temporarily bans player whose "
                    "^1name ^5is equal to "
                    "specified ^1name"},
    {"serverinfo", "^1serverinfo ^7-> ^3Retrieves currently active server settings."},
    {"map_rotate", "^1map_rotate ^7-> ^5Loads next map on the server."},
    {"map_restart", "^1map_restart ^7-> ^3Reloads currently played map on the server."},
    {"map", "^1map map_name ^7-> ^5Immediately loads map ^1map_name ^5on the server."},
    {"fast_restart", "^1fast_restart ^7-> ^3Quickly restarts currently played ^1map ^3on the "
                     "server."},
    {"getinfo", "^1getinfo ^7-> ^5Retrieves basic server information from the server."},
    {"mapname", "^1mapname ^7-> ^3Retrieves and displays currently played map's ^1rcon map "
                "name ^3on "
                "the console."},
    {"g_gametype", "^1g_gametype ^7-> ^5Retrieves and displays currently played match's "
                   "^1gametype "
                   "^5on the console."},
    {"sv_maprotation", "^1sv_maprotation ^7-> ^3Retrieves and displays server's original ^1map "
                       "rotation "
                       "^3setting."},
    {"sv_maprotationcurrent", "^1sv_maprotationcurrent ^7-> ^5Retrieves and displays server's current "
                              "^1map "
                              "rotation ^5setting."},
    {"say", R"(^1say public_message ^7-> ^3Sends ^1public_message ^3to all players on the server.)"},
    {"tell", "^1tell player_pid private_message ^7-> ^5Sends ^1private_message ^5to "
             "player whose ^1pid ^5= ^1player_pid"}};

extern const unordered_set<string> rcon_commands_set{"status",
                                                     "!status",
                                                     "gs",
                                                     "!gs",
                                                     "getstatus",
                                                     "!getstatus",
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
                                                     "!tell"};

extern const unordered_set<string> rcon_commands_wait_for_reply{
    "!status",      "status",         "gs",
    "!gs",          "getstatus",      "serverinfo",
    "map_rotate",   "map_restart",    "map",
    "fast_restart", "getinfo",        "mapname",
    "g_gametype",   "sv_maprotation", "sv_maprotationcurrent"};

// extern const unordered_set<string> non_rcon_commands_wait_for_reply{ "gs",
// "!gs", "getstatus", "!getstatus", "getinfo" };

extern unordered_map<size_t, string> rcon_status_grid_column_header_titles;
extern unordered_map<size_t, string> get_status_grid_column_header_titles;
extern unordered_map<size_t, string> servers_grid_column_header_titles;
extern unordered_map<string, game_name_t> game_name_to_game_name_t{
    {"unknown", game_name_t::unknown},     {"cod1", game_name_t::cod1},           {"Call of Duty", game_name_t::cod1},
    {"cod2", game_name_t::cod2},           {"Call of Duty 2", game_name_t::cod2}, {"cod4", game_name_t::cod4},
    {"Call of Duty 4", game_name_t::cod4}, {"cod5", game_name_t::cod5},           {"Call of Duty 5", game_name_t::cod5},
};

extern const unordered_map<string, int> flag_name_index{
    {"xy", 0},      {"ad", 1},      {"ae", 2},   {"af", 3},   {"ag", 4},   {"ai", 5},   {"al", 6},      {"am", 7},
    {"ao", 8},      {"aq", 9},      {"ar", 10},  {"as", 11},  {"at", 12},  {"au", 13},  {"aw", 14},     {"ax", 15},
    {"az", 16},     {"ba", 17},     {"bb", 18},  {"bd", 19},  {"be", 20},  {"bf", 21},  {"bg", 22},     {"bh", 23},
    {"bi", 24},     {"bj", 25},     {"bl", 26},  {"bm", 27},  {"bn", 28},  {"bo", 29},  {"bq", 30},     {"br", 31},
    {"bs", 32},     {"bt", 33},     {"bv", 34},  {"bw", 35},  {"by", 36},  {"bz", 37},  {"ca", 38},     {"cc", 39},
    {"cd", 40},     {"cf", 41},     {"cg", 42},  {"ch", 43},  {"ci", 44},  {"ck", 45},  {"cl", 46},     {"cm", 47},
    {"cn", 48},     {"co", 49},     {"cr", 50},  {"cu", 51},  {"cv", 52},  {"cw", 53},  {"cx", 54},     {"cy", 55},
    {"cz", 56},     {"de", 57},     {"dj", 58},  {"dk", 59},  {"dm", 60},  {"do", 61},  {"dz", 62},     {"ec", 63},
    {"ee", 64},     {"eg", 65},     {"eh", 66},  {"er", 67},  {"es", 68},  {"et", 69},  {"fi", 70},     {"fj", 71},
    {"fk", 72},     {"fm", 73},     {"fo", 74},  {"fr", 75},  {"ga", 76},  {"gb", 77},  {"gb_eng", 78}, {"gb_nir", 79},
    {"gb_sct", 80}, {"gb_wls", 81}, {"gd", 82},  {"ge", 83},  {"gf", 84},  {"gg", 85},  {"gh", 86},     {"gi", 87},
    {"gl", 88},     {"gm", 89},     {"gn", 90},  {"gp", 91},  {"gq", 92},  {"gr", 93},  {"gs", 94},     {"gt", 95},
    {"gu", 96},     {"gw", 97},     {"gy", 98},  {"hk", 99},  {"hm", 100}, {"hn", 101}, {"hr", 102},    {"ht", 103},
    {"hu", 104},    {"id", 105},    {"ie", 106}, {"il", 107}, {"im", 108}, {"in", 109}, {"io", 110},    {"iq", 111},
    {"ir", 112},    {"is", 113},    {"it", 114}, {"je", 115}, {"jm", 116}, {"jo", 117}, {"jp", 118},    {"ke", 119},
    {"kg", 120},    {"kh", 121},    {"ki", 122}, {"km", 123}, {"kn", 124}, {"kp", 125}, {"kr", 126},    {"kw", 127},
    {"ky", 128},    {"kz", 129},    {"la", 130}, {"lb", 131}, {"lc", 132}, {"li", 133}, {"lk", 134},    {"lr", 135},
    {"ls", 136},    {"lt", 137},    {"lu", 138}, {"lv", 139}, {"ly", 140}, {"ma", 141}, {"mc", 142},    {"md", 143},
    {"me", 144},    {"mf", 145},    {"mg", 146}, {"mh", 147}, {"mk", 148}, {"ml", 149}, {"mm", 150},    {"mn", 151},
    {"mo", 152},    {"mp", 153},    {"mq", 154}, {"mr", 155}, {"ms", 156}, {"mt", 157}, {"mu", 158},    {"mv", 159},
    {"mw", 160},    {"mx", 161},    {"my", 162}, {"mz", 163}, {"na", 164}, {"nc", 165}, {"ne", 166},    {"nf", 167},
    {"ng", 168},    {"ni", 169},    {"nl", 170}, {"no", 171}, {"np", 172}, {"nr", 173}, {"nu", 174},    {"nz", 175},
    {"om", 176},    {"pa", 177},    {"pe", 178}, {"pf", 179}, {"pg", 180}, {"ph", 181}, {"pk", 182},    {"pl", 183},
    {"pm", 184},    {"pn", 185},    {"pr", 186}, {"ps", 187}, {"pt", 188}, {"pw", 189}, {"py", 190},    {"qa", 191},
    {"re", 192},    {"ro", 193},    {"rs", 194}, {"ru", 195}, {"rw", 196}, {"sa", 197}, {"sb", 198},    {"sc", 199},
    {"sd", 200},    {"se", 201},    {"sg", 202}, {"sh", 203}, {"si", 204}, {"sj", 205}, {"sk", 206},    {"sl", 207},
    {"sm", 208},    {"sn", 209},    {"so", 210}, {"sr", 211}, {"ss", 212}, {"st", 213}, {"sv", 214},    {"sx", 215},
    {"sy", 216},    {"sz", 217},    {"tc", 218}, {"td", 219}, {"tf", 220}, {"tg", 221}, {"th", 222},    {"tj", 223},
    {"tk", 224},    {"tl", 225},    {"tm", 226}, {"tn", 227}, {"to", 228}, {"tr", 229}, {"tt", 230},    {"tv", 231},
    {"tw", 232},    {"tz", 233},    {"ua", 234}, {"ug", 235}, {"um", 236}, {"us", 237}, {"uy", 238},    {"uz", 239},
    {"va", 240},    {"vc", 241},    {"ve", 242}, {"vg", 243}, {"vi", 244}, {"vn", 245}, {"vu", 246},    {"wf", 247},
    {"ws", 248},    {"xk", 249},    {"ye", 250}, {"yt", 251}, {"za", 252}, {"zm", 253}, {"zw", 254}};

void show_error(HWND parent_window, const char *error_message, const size_t error_level)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    main_app.get_connection_manager_for_messages().process_and_send_message(
        "tinyrcon-error",
        format("{}\\{}\\Error level={}\\Error message: '{}'", main_app.get_username(), main_app.get_user_ip_address(),
               error_level, error_message),
        true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);

    switch (error_level)
    {
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
    //// print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ifstream input{file_path};
    if (!input)
        return 0;

    size_t lines_cnt{};
    for (string line; getline(input, line); ++lines_cnt)
        ;

    return lines_cnt + 1;
}

bool parse_geodata_lite_csv_file(const char *file_path)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ifstream input{file_path};
    if (!input)
        return false;

    vector<geoip_data> geoip_data;
    geoip_data.reserve(get_number_of_lines_in_file(file_path));

    for (string line; getline(input, line);)
    {
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

        if (size_t pos = parts[3].find("United Kingdom"); pos != string::npos)
        {
            parts[3] = "UK";
        }
        else if (pos = parts[3].find("United States of America"); pos != string::npos)
        {
            parts[3] = "USA";
        }
        else if (pos = parts[3].find("United Arab Emirates"); pos != string::npos)
        {
            parts[3] = "UAE";
        }

        for (size_t current{}; (current = parts[3].find('(')) != string::npos;)
        {
            const size_t next{parts[3].find(')', current + 1)};
            if (next == string::npos)
                break;
            parts[3].erase(current, next - current + 1);
            stl::helper::trim_in_place(parts[3]);
        }

        parts[4].pop_back();
        parts[4].erase(0, 1);
        parts[5].pop_back();
        parts[5].erase(0, 1);
        geoip_data.emplace_back(stoul(parts[0]), stoul(parts[1]), parts[2].c_str(), parts[3].c_str(), parts[4].c_str(),
                                parts[5].c_str());
    }

    main_app.get_connection_manager().get_geoip_data() = std::move(geoip_data);

    return true;
}

std::pair<bool, std::string> create_necessary_folders_and_files(const std::vector<string> &folder_file_paths)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    unordered_set<string> created_folders;

    for (const auto &file_ : folder_file_paths)
    {
        const directory_entry entry{file_};
        if (!entry.exists())
        {

            string file_name;

            size_t last_sep_pos{file_.find_last_of("./\\")};
            if (string::npos != last_sep_pos && '.' == file_[last_sep_pos])
            {
                last_sep_pos = file_.find_last_of("/\\", last_sep_pos - 1);
                file_name = file_.substr(last_sep_pos != string::npos ? last_sep_pos + 1 : 0);
            }
            else
            {
                last_sep_pos = file_.length();
            }

            string parent_path{string::npos != last_sep_pos ? file_.substr(0, last_sep_pos) : ""s};

            if (!parent_path.empty() && !created_folders.contains(parent_path))
            {
                error_code ec{};
                create_directories(parent_path, ec);
                if (ec.value() != 0)
                {
                    replace_backward_slash_with_forward_slash(parent_path);
                    return {false, parent_path};
                }
                created_folders.emplace(parent_path);
            }

            if (!file_name.empty() && !check_if_file_path_exists(entry.path().string().c_str()))
            {

                if (file_name == "tinyrcon.json")
                {
                    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
                }
                else
                {
                    // ofstream file_to_create{ entry.path().string().c_str() };
                    ofstream file_to_create{file_};
                    if (!file_to_create)
                    {
                        replace_backward_slash_with_forward_slash(file_name);
                        return {false, parent_path};
                    }
                    file_to_create.flush();
                    file_to_create.close();
                }
            }
        }
    }

    return {true, string{}};
}

std::pair<bool, std::wstring> create_necessary_folders_and_files(const std::vector<wstring> &folder_file_paths)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    unordered_set<wstring> created_folders;

    for (const auto &file_ : folder_file_paths)
    {
        const directory_entry entry{file_};
        if (!entry.exists())
        {

            wstring file_name;

            size_t last_sep_pos{file_.find_last_of(L"./\\")};
            if (wstring::npos != last_sep_pos && '.' == file_[last_sep_pos])
            {
                last_sep_pos = file_.find_last_of(L"/\\", last_sep_pos - 1);
                file_name = file_.substr(last_sep_pos != wstring::npos ? last_sep_pos + 1 : 0);
            }
            else
            {
                last_sep_pos = file_.length();
            }

            wstring parent_path{wstring::npos != last_sep_pos ? file_.substr(0, last_sep_pos) : L""s};

            if (!parent_path.empty() && !created_folders.contains(parent_path))
            {
                error_code ec{};
                create_directories(parent_path, ec);
                if (ec.value() != 0)
                {
                    replace_backward_slash_with_forward_slash(parent_path);
                    return {false, parent_path};
                }
                created_folders.emplace(parent_path);
            }

            if (!file_name.empty() && !check_if_file_path_exists(entry.path().wstring().c_str()))
            {

                if (file_name == L"tinyrcon.json")
                {
                    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
                }
                else
                {
                    // ofstream file_to_create{ entry.path().string().c_str() };
                    ofstream file_to_create{file_};
                    if (!file_to_create)
                    {
                        replace_backward_slash_with_forward_slash(file_name);
                        return {false, parent_path};
                    }
                    file_to_create.flush();
                    file_to_create.close();
                }
            }
        }
    }

    return {true, wstring{}};
}

std::pair<bool, std::string> create_necessary_file_path(const std::string &file_)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const directory_entry entry{file_};
    if (!entry.exists())
    {

        string file_name;

        size_t last_sep_pos{file_.find_last_of("./\\")};
        if (string::npos != last_sep_pos && '.' == file_[last_sep_pos])
        {
            last_sep_pos = file_.find_last_of("/\\", last_sep_pos - 1);
            file_name = file_.substr(last_sep_pos != string::npos ? last_sep_pos + 1 : 0);
        }
        else
        {
            last_sep_pos = file_.length();
        }

        string parent_path{string::npos != last_sep_pos ? file_.substr(0, last_sep_pos) : ""s};

        if (!parent_path.empty())
        {
            error_code ec{};
            create_directories(parent_path, ec);
            if (ec.value() != 0)
            {
                replace_backward_slash_with_forward_slash(parent_path);
                return {false, parent_path};
            }
        }

        if (!file_name.empty() && !check_if_file_path_exists(entry.path().string().c_str()))
        {

            if (file_name == "tinyrcon.json")
            {
                write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
            }
            else
            {
                // ofstream file_to_create{ entry.path().string().c_str() };
                ofstream file_to_create{file_};
                if (!file_to_create)
                {
                    replace_backward_slash_with_forward_slash(file_name);
                    return {false, parent_path};
                }
                file_to_create.flush();
                file_to_create.close();
            }
        }
    }

    return {true, string{}};
}

std::pair<bool, std::wstring> create_necessary_file_path(const std::wstring &file_)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const directory_entry entry{file_};
    if (!entry.exists())
    {

        wstring file_name;

        size_t last_sep_pos{file_.find_last_of(L"./\\")};
        if (wstring::npos != last_sep_pos && L'.' == file_[last_sep_pos])
        {
            last_sep_pos = file_.find_last_of(L"/\\", last_sep_pos - 1);
            file_name = file_.substr(last_sep_pos != wstring::npos ? last_sep_pos + 1 : 0);
        }
        else
        {
            last_sep_pos = file_.length();
        }

        wstring parent_path{wstring::npos != last_sep_pos ? file_.substr(0, last_sep_pos) : L""s};

        if (!parent_path.empty())
        {
            error_code ec{};
            create_directories(parent_path, ec);
            if (ec.value() != 0)
            {
                replace_backward_slash_with_forward_slash(parent_path);
                return {false, parent_path};
            }
        }

        if (!file_name.empty() && !check_if_file_path_exists(entry.path().wstring().c_str()))
        {

            if (file_name == L"tinyrcon.json")
            {
                write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
            }
            else
            {
                // ofstream file_to_create{ entry.path().string().c_str() };
                ofstream file_to_create{file_};
                if (!file_to_create)
                {
                    replace_backward_slash_with_forward_slash(file_name);
                    return {false, parent_path};
                }
                file_to_create.flush();
                file_to_create.close();
            }
        }
    }

    return {true, wstring{}};
}

bool write_tiny_rcon_json_settings_to_file(const char *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    std::ofstream config_file{file_path};

    if (!config_file)
    {
        const string re_msg{
            "^3Error writing default tiny rcon client's configuration data\nto specified tinyrcon.json file: ^1"s +
            string{file_path} +
            "\n^5Please make sure that the main application folder's properties are not set to read-only mode!"s};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str());
        return false;
    }

    config_file << "{\n"
                << R"("username": ")" << main_app.get_username() << "\",\n";
    config_file << "\"check_for_banned_players_time_interval\": " << main_app.get_check_for_banned_players_time_period()
                << ",\n";
    config_file << R"("rcon_game_servers": [)" << '\n';
    for (size_t i{}; i < main_app.get_rcon_game_servers_count(); ++i)
    {
        auto &gs = main_app.get_game_servers()[i];
        config_file << "{\n";
        config_file << R"("game_server_name": ")" << gs.get_server_name() << "\",\n";
        config_file << R"("rcon_server_ip_address": ")" << gs.get_server_ip_address() << "\",\n";
        config_file << R"("rcon_port": )" << gs.get_server_port() << ",\n";
        config_file << R"("rcon_password": ")" << gs.get_rcon_password() << "\",\n";
        config_file << R"("private_slot_password": ")" << gs.get_private_slot_password() << "\"\n";
        config_file << (i < main_app.get_rcon_game_servers_count() - 1 ? "},\n" : "}\n");
    }
    config_file << "],\n";

    config_file << R"("tiny_rcon_server_ip": ")" << main_app.get_tiny_rcon_server_ip_address() << "\",\n";
    config_file << "\"tiny_rcon_server_port\": " << main_app.get_tiny_rcon_server_port() << ",\n";
    config_file << R"("tiny_rcon_server_ip_for_players": ")" << main_app.get_tiny_rcon_server_ip_address_for_players()
                << "\",\n";
    config_file << "\"tiny_rcon_server_port_for_players\": " << main_app.get_tiny_rcon_server_port_for_players()
                << ",\n";
    config_file << R"("tiny_rcon_ftp_server_username": ")" << main_app.get_tiny_rcon_ftp_server_username() << "\",\n";
    config_file << R"("tiny_rcon_ftp_server_password": ")" << main_app.get_tiny_rcon_ftp_server_password() << "\",\n";
    config_file << R"("game_executable_paths" : {)"
                << "\n";
    config_file << R"(  "codmp_exe_path": ")"
                << escape_backward_slash_characters_in_place(main_app.get_codmp_exe_path()) << "\",\n";
    config_file << R"(  "cod2mp_s_exe_path": ")"
                << escape_backward_slash_characters_in_place(main_app.get_cod2mp_exe_path()) << "\",\n";
    config_file << R"(  "iw3mp_exe_path": ")"
                << escape_backward_slash_characters_in_place(main_app.get_iw3mp_exe_path()) << "\",\n";
    config_file << R"(  "cod5mp_exe_path": ")"
                << escape_backward_slash_characters_in_place(main_app.get_cod5mp_exe_path()) << "\"\n";
    config_file << "},\n";
    config_file << "\"is_automatic_city_kick_enabled\": "
                << (main_app.get_is_automatic_city_kick_enabled() ? "true" : "false") << ",\n";
    config_file << "\"is_automatic_country_kick_enabled\": "
                << (main_app.get_is_automatic_country_kick_enabled() ? "true" : "false") << ",\n";
    config_file << "\"enable_automatic_connection_flood_ip_ban\": "
                << (main_app.get_is_enable_automatic_connection_flood_ip_ban() ? "true" : "false") << ",\n";
    config_file << "\"minimum_number_of_connections_from_same_ip_for_automatic_ban\": "
                << main_app.get_current_game_server().get_minimum_number_of_connections_from_same_ip_for_automatic_ban()
                << ",\n";
    config_file << "\"number_of_warnings_for_automatic_kick\": "
                << main_app.get_current_game_server().get_maximum_number_of_warnings_for_automatic_kick() << ",\n";
    config_file << "\"disable_automatic_kick_messages\": "
                << (main_app.get_is_disable_automatic_kick_messages() ? "true" : "false") << ",\n";
    config_file << "\"use_original_admin_messages\": "
                << (main_app.get_is_use_original_admin_messages() ? "true" : "false") << ",\n";
    config_file << R"("user_defined_warn_msg": ")" << main_app.get_admin_messages()["user_defined_warn_msg"] << "\",\n";
    config_file << R"("user_defined_kick_msg": ")" << main_app.get_admin_messages()["user_defined_kick_msg"] << "\",\n";
    config_file << R"("user_defined_temp_ban_msg": ")" << main_app.get_admin_messages()["user_defined_temp_ban_msg"]
                << "\",\n";
    config_file << R"("user_defined_ban_msg": ")" << main_app.get_admin_messages()["user_defined_ban_msg"] << "\",\n";
    config_file << R"("user_defined_ip_ban_msg": ")" << main_app.get_admin_messages()["user_defined_ip_ban_msg"]
                << "\",\n";
    config_file << R"("user_defined_ip_address_range_ban_msg": ")"
                << main_app.get_admin_messages()["user_defined_ip_address_range_ban_msg"] << "\",\n";
    config_file << R"("user_defined_name_ban_msg": ")" << main_app.get_admin_messages()["user_defined_name_ban_msg"]
                << "\",\n";
    config_file << R"("user_defined_name_unban_msg": ")" << main_app.get_admin_messages()["user_defined_name_unban_msg"]
                << "\",\n";
    config_file << R"("user_defined_city_ban_msg": ")" << main_app.get_admin_messages()["user_defined_city_ban_msg"]
                << "\",\n";
    config_file << R"("user_defined_city_unban_msg": ")" << main_app.get_admin_messages()["user_defined_city_unban_msg"]
                << "\",\n";
    config_file << R"("user_defined_enable_city_ban_feature_msg": ")"
                << main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] << "\",\n";
    config_file << R"("user_defined_disable_city_ban_feature_msg": ")"
                << main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] << "\",\n";
    config_file << R"("user_defined_country_ban_msg": ")"
                << main_app.get_admin_messages()["user_defined_country_ban_msg"] << "\",\n";
    config_file << R"("user_defined_country_unban_msg": ")"
                << main_app.get_admin_messages()["user_defined_country_unban_msg"] << "\",\n";
    config_file << R"("user_defined_enable_country_ban_feature_msg": ")"
                << main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] << "\",\n";
    config_file << R"("user_defined_disable_country_ban_feature_msg": ")"
                << main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] << "\",\n";
    config_file << R"("user_defined_protect_ip_address_message": ")"
                << main_app.get_admin_messages()["user_defined_protect_ip_address_message"] << "\",\n";
    config_file << R"("user_defined_unprotect_ip_address_message": ")"
                << main_app.get_admin_messages()["user_defined_unprotect_ip_address_message"] << "\",\n";
    config_file << R"("user_defined_protect_ip_address_range_message": ")"
                << main_app.get_admin_messages()["user_defined_protect_ip_address_range_message"] << "\",\n";
    config_file << R"("user_defined_unprotect_ip_address_range_message": ")"
                << main_app.get_admin_messages()["user_defined_unprotect_ip_address_range_message"] << "\",\n";
    config_file << R"("user_defined_protect_city_message": ")"
                << main_app.get_admin_messages()["user_defined_protect_city_message"] << "\",\n";
    config_file << R"("user_defined_unprotect_city_message": ")"
                << main_app.get_admin_messages()["user_defined_unprotect_city_message"] << "\",\n";
    config_file << R"("user_defined_protect_country_message": ")"
                << main_app.get_admin_messages()["user_defined_protect_country_message"] << "\",\n";
    config_file << R"("user_defined_unprotect_country_message": ")"
                << main_app.get_admin_messages()["user_defined_unprotect_country_message"] << "\",\n";
    config_file << R"("automatic_remove_temp_ban_msg": ")"
                << main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] << "\",\n";
    config_file << R"("automatic_kick_temp_ban_msg": ")" << main_app.get_admin_messages()["automatic_kick_temp_ban_msg"]
                << "\",\n";
    config_file << R"("automatic_kick_ip_ban_msg": ")" << main_app.get_admin_messages()["automatic_kick_ip_ban_msg"]
                << "\",\n";
    config_file << R"("automatic_kick_ip_address_range_ban_msg":  ")"
                << main_app.get_admin_messages()["automatic_kick_ip_address_range_ban_msg"] << "\",\n";
    config_file << R"("automatic_kick_city_ban_msg": ")" << main_app.get_admin_messages()["automatic_kick_city_ban_msg"]
                << "\",\n";
    config_file << R"("automatic_kick_country_ban_msg": ")"
                << main_app.get_admin_messages()["automatic_kick_country_ban_msg"] << "\",\n";
    config_file << R"("automatic_kick_name_ban_msg": ")" << main_app.get_admin_messages()["automatic_kick_name_ban_msg"]
                << "\",\n";
    config_file << R"("current_match_info": ")" << main_app.get_current_match_info() << "\",\n";
    config_file << "\"use_different_background_colors_for_even_and_odd_lines\": "
                << (main_app.get_is_use_different_background_colors_for_even_and_odd_lines() ? "true" : "false")
                << ",\n";
    config_file << R"("odd_player_data_lines_bg_color": ")" << main_app.get_odd_player_data_lines_bg_color() << "\",\n";
    config_file << R"("even_player_data_lines_bg_color": ")" << main_app.get_even_player_data_lines_bg_color()
                << "\",\n";
    config_file << "\"use_different_foreground_colors_for_even_and_odd_lines\": "
                << (main_app.get_is_use_different_foreground_colors_for_even_and_odd_lines() ? "true" : "false")
                << ",\n";
    config_file << R"("odd_player_data_lines_fg_color": ")" << main_app.get_odd_player_data_lines_fg_color() << "\",\n";
    config_file << R"("even_player_data_lines_fg_color": ")" << main_app.get_even_player_data_lines_fg_color()
                << "\",\n";
    config_file << R"("full_map_name_color": ")" << main_app.get_full_map_name_color() << "\",\n";
    config_file << R"("rcon_map_name_color": ")" << main_app.get_rcon_map_name_color() << "\",\n";
    config_file << R"("full_game_type_color": ")" << main_app.get_full_gametype_name_color() << "\",\n";
    config_file << R"("rcon_game_type_color": ")" << main_app.get_rcon_gametype_name_color() << "\",\n";

    config_file << R"("online_players_count_color": ")" << main_app.get_online_players_count_color() << "\",\n";
    config_file << R"("offline_players_count_color": ")" << main_app.get_offline_players_count_color() << "\",\n";
    config_file << R"("border_line_color": ")" << main_app.get_border_line_color() << "\",\n";
    config_file << R"("header_player_pid_color": ")" << main_app.get_header_player_pid_color() << "\",\n";
    config_file << R"("data_player_pid_color": ")" << main_app.get_data_player_pid_color() << "\",\n";
    config_file << R"("header_player_score_color": ")" << main_app.get_header_player_score_color() << "\",\n";
    config_file << R"("data_player_score_color": ")" << main_app.get_data_player_score_color() << "\",\n";
    config_file << R"("header_player_ping_color": ")" << main_app.get_header_player_ping_color() << "\",\n";
    config_file << R"("data_player_ping_color": ")" << main_app.get_data_player_ping_color() << "\",\n";
    config_file << R"("header_player_name_color": ")" << main_app.get_header_player_name_color() << "\",\n";
    config_file << R"("header_player_ip_color": ")" << main_app.get_header_player_ip_color() << "\",\n";
    config_file << R"("data_player_ip_color": ")" << main_app.get_data_player_ip_color() << "\",\n";
    config_file << R"("header_player_geoinfo_color": ")" << main_app.get_header_player_geoinfo_color() << "\",\n";
    config_file << R"("data_player_geoinfo_color": ")" << main_app.get_data_player_geoinfo_color() << "\",\n";
    config_file << "\"draw_border_lines\": " << (main_app.get_is_draw_border_lines() ? "true" : "false") << ",\n";
    config_file << R"("ftp_download_site_ip_address": ")" << main_app.get_ftp_download_site_ip_address() << "\",\n";
    config_file << R"("ftp_download_folder_path": ")" << main_app.get_ftp_download_folder_path() << "\",\n";
    config_file << R"("ftp_download_file_pattern": ")"
                << escape_backward_slash_characters_in_place(main_app.get_ftp_download_file_pattern()) << "\",\n";
    config_file << R"("plugins_geoIP_geo_dat_md5": ")" << main_app.get_plugins_geoIP_geo_dat_md5() << "\",\n";
    config_file << R"("images_data_md5": ")" << main_app.get_images_data_md5() << "\",\n";
    config_file << R"("enable_automatic_program_updates": )"
                << (main_app.get_is_enable_automatic_program_updates() ? "true" : "false") << ",\n";
    config_file << R"("enable_automatic_missing_map_image_download": )"
                << (main_app.get_is_enable_automatic_missing_map_image_download() ? "true" : "false") << "\n";
    /*config_file << R"("players_stats_feature": {)" << '\n';
    config_file << R"("enabled": )" <<
    (main_app.get_is_enable_players_stats_feature() ? "true" : "false") << ",\n";
    config_file << R"("enable_tracking_of_player_stats_data_for_day": )" <<
    (main_app.get_is_enable_tracking_of_player_stats_data_for_day() ? "true" :
    "false") << ",\n"; config_file <<
    R"("enable_tracking_of_player_stats_data_for_month": )" <<
    (main_app.get_is_enable_tracking_of_player_stats_data_for_month() ? "true" :
    "false") << ",\n"; config_file <<
    R"("enable_tracking_of_player_stats_data_for_year": )" <<
    (main_app.get_is_enable_tracking_of_player_stats_data_for_year() ? "true" :
    "false") << ",\n"; config_file <<
    R"("enable_tracking_of_player_stats_data_permanently": )" <<
    (main_app.get_is_enable_tracking_of_player_stats_data_permanently() ? "true" :
    "false") << ",\n"; config_file <<
    R"("number_of_top_players_to_display_in_game_chat": )" <<
    main_app.get_number_of_top_players_to_display_in_game_chat() << ",\n";
    config_file << R"("number_of_top_players_to_display_in_tinyrcon": )" <<
    main_app.get_number_of_top_players_to_display_in_tinyrcon() << ",\n";
    config_file <<
    R"("time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat":
    )" <<
    main_app.get_time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat()
    << ",\n"; config_file <<
    R"("time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon":
    )" <<
    main_app.get_time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon()
    << ",\n"; config_file <<
    R"("time_period_in_minutes_for_saving_players_stats_data": )" <<
    main_app.get_time_period_in_minutes_for_saving_players_stats_data() <<
    "\n}\n";*/
    config_file << "}" << flush;
    return true;
}

bool check_ip_address_validity(string_view ip_address, unsigned long &ip_key)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string ex_msg{
        format(R"(^1Exception ^3thrown from ^1bool check_ip_address_validity("{}", "{}"))", ip_address, ip_key)};
    stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
    auto parts = stl::helper::str_split(ip_address, ".", nullptr, split_on_whole_needle_t::yes);
    if (parts.size() < 4U)
        return false;

    for (auto &part : parts)
        stl::helper::trim_in_place(part);

    const bool is_valid_ip_address = all_of(cbegin(parts), cend(parts), [](const string &part) {
        if (part.length() > 3)
            return false;
        for (const auto digit : part)
        {
            if (!isdigit(digit))
                return false;
        }

        const int value{stoi(part)};
        return value >= 0 && value <= 255;
    });

    if (is_valid_ip_address)
    {
        ip_key = 0UL;
        for (size_t i{}; i < 4; ++i)
        {
            ip_key <<= 8;
            ip_key += stoul(parts[i]);
        }
    }

    return is_valid_ip_address;
}

bool check_ip_address_range_validity(const string &ip_address_range)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const regex ip_address_range_mask(R"(^\d{1,3}\.\d{1,3}\.(\d{1,3}|\*)\.(\d{1,3}|\*)$)");
    smatch matches;
    return regex_match(ip_address_range, matches, ip_address_range_mask);
}

void convert_guid_key_to_country_name(const vector<geoip_data> &geo_data, string_view player_ip, player &player_data)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    unsigned long ip_key{};
    if (!check_ip_address_validity(player_ip, ip_key))
    {
        player_data.ip_hash_key = 0UL;
        player_data.country_name = "Unknown";
        player_data.region = "Unknown";
        player_data.city = "Unknown";
        player_data.country_code = "xy";
    }
    else
    {
        player_data.ip_hash_key = ip_key;
        const size_t sizeOfElements{geo_data.size()};
        size_t lower_bound{0};
        size_t upper_bound{sizeOfElements};

        bool is_found_match{};

        while (lower_bound <= upper_bound)
        {
            const size_t currentIndex = (lower_bound + upper_bound) / 2;

            if (currentIndex >= geo_data.size())
                break;

            if (ip_key >= geo_data.at(currentIndex).lower_ip_bound &&
                ip_key <= geo_data.at(currentIndex).upper_ip_bound)
            {
                player_data.country_name = geo_data.at(currentIndex).get_country_name();
                if (strstr(player_data.country_name, "Russia") != nullptr)
                    player_data.country_name = "Russia";
                player_data.region = geo_data.at(currentIndex).get_region();
                player_data.city = geo_data.at(currentIndex).get_city();
                player_data.country_code = geo_data.at(currentIndex).get_country_code();
                is_found_match = true;
                break;
            }

            if (ip_key > geo_data.at(currentIndex).upper_ip_bound)
            {
                lower_bound = currentIndex + 1;
            }
            else
            {
                upper_bound = currentIndex - 1;
            }
        }

        if (!is_found_match)
        {
            player_data.country_name = "Unknown";
            player_data.region = "Unknown";
            player_data.city = "Unknown";
            player_data.country_code = "xy";
        }
    }
}

// size_t find_longest_player_stats_name_length(
//	const std::vector<player_stats>& players,
//	const bool count_color_codes,
//	const size_t number_of_players_to_process)
//{
//	// print_trace_message(__FILE__, __LINE__, __FUNCTION__);
//	if (0 == number_of_players_to_process)
//		return 0;
//	size_t max_player_name_length{ 8 };
//	for (size_t i{}; i < number_of_players_to_process; ++i) {
//		max_player_name_length =
//			max(count_color_codes ? len(players[i].name) :
// get_number_of_characters_without_color_codes(players[i].name),
// max_player_name_length);
//	}
//
//	return max_player_name_length;
// }

size_t find_longest_player_country_city_info_length(const std::vector<player> &players,
                                                    const size_t number_of_players_to_process)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (0 == number_of_players_to_process)
        return 0;

    size_t max_geodata_info_length{18};
    for (size_t i{}; i < number_of_players_to_process; ++i)
    {
        const size_t country_len{len(players[i].country_name)};
        const size_t region_len{len(players[i].region)};
        const size_t city_len{len(players[i].city)};
        const size_t current_player_geodata_info_length = (country_len != 0 ? country_len : region_len) + city_len + 2;
        max_geodata_info_length = std::max(current_player_geodata_info_length, max_geodata_info_length);
    }

    return max_geodata_info_length;
}

size_t find_longest_user_name_length(const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users,
                                     const bool count_color_codes, const size_t number_of_users_to_process)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t max_player_name_length{8};

    if (0 == number_of_users_to_process)
        return max_player_name_length;

    for (size_t i{}; i < number_of_users_to_process; ++i)
    {
        max_player_name_length =
            max(count_color_codes ? users[i]->user_name.length()
                                  : get_number_of_characters_without_color_codes(users[i]->user_name.c_str()),
                max_player_name_length);
    }

    return max_player_name_length;
}

size_t find_longest_user_country_city_info_length(const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users,
                                                  const size_t number_of_users_to_process)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t max_geodata_info_length{20};

    if (0 == number_of_users_to_process)
        return max_geodata_info_length;

    for (size_t i{}; i < number_of_users_to_process; ++i)
    {
        const size_t current_player_geodata_info_length{users[i]->geo_information.length()};
        max_geodata_info_length = std::max(current_player_geodata_info_length, max_geodata_info_length);
    }

    return max_geodata_info_length;
}

void parse_tinyrcon_tool_config_file(const char *configFileName)
{
    using namespace nlohmann;

    fix_path_strings_in_json_config_file(configFileName);

    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ifstream configFile{configFileName};

    if (!configFile)
    {
        configFile.close();
        configFile.clear();
        write_tiny_rcon_json_settings_to_file(configFileName);
        configFile.open(configFileName, std::ios_base::in);
    }

    if (!configFile)
        return;

    json json_resource;
    try
    {
        configFile >> json_resource;
    }
    catch (std::exception &ex)
    {
        const std::string exception_message{
            format("^1Exception ^3occurred while parsing tinyrcon.json file!\n^5{}", ex.what())};
        print_colored_text(app_handles.hwnd_re_messages_data, exception_message.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        // show_error(app_handles.hwnd_main_window, exception_message.c_str(), 0);
    }
    catch (...)
    {
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^3Unknown exception occurred while parsing tinyrcon.json file!",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        // show_error(app_handles.hwnd_main_window, "Exception occurred while
        // parsing tinyrcon.json file!", 0);
    }

    string data_line;
    bool found_missing_config_setting{};

    if (json_resource.contains("username"))
    {
        data_line = json_resource["username"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_username(data_line);
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = std::move(data_line);
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_username("^3Player");
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = "^3Player";
    }

    size_t parsed_rcon_game_server_index{};

    if (json_resource.contains("rcon_game_servers") && json_resource.at("rcon_game_servers").is_array())
    {
        auto &rcon_game_servers_json_array = json_resource["rcon_game_servers"];
        for (size_t i{}; i < rcon_game_servers_json_array.size(); ++i)
        {
            auto &&json_object = rcon_game_servers_json_array[i];
            if (json_object.contains("game_server_name") && json_object.contains("rcon_server_ip_address") &&
                json_object.contains("rcon_port") && json_object.contains("rcon_password") &&
                json_object.contains("private_slot_password"))
            {
                game_server gs{};
                data_line = json_object["game_server_name"];
                strip_leading_and_trailing_quotes(data_line);
                gs.set_server_name(std::move(data_line));

                data_line = json_object["rcon_server_ip_address"];
                strip_leading_and_trailing_quotes(data_line);
                gs.set_server_ip_address(std::move(data_line));

                const int port_number{json_object["rcon_port"].template get<int>()};
                gs.set_server_port(static_cast<uint_least16_t>(port_number));

                data_line = json_object["rcon_password"];
                strip_leading_and_trailing_quotes(data_line);
                gs.set_rcon_password(std::move(data_line));

                data_line = json_object["private_slot_password"];
                strip_leading_and_trailing_quotes(data_line);
                gs.set_private_slot_password(std::move(data_line));

                main_app.get_game_servers()[parsed_rcon_game_server_index] = std::move(gs);
                ++parsed_rcon_game_server_index;
            }
        }

        main_app.set_rcon_game_servers_count(parsed_rcon_game_server_index);
        main_app.set_game_servers_count(parsed_rcon_game_server_index);
    }

    if (0U == parsed_rcon_game_server_index)
    {
        game_server gs{};
        if (json_resource.contains("game_server_name"))
        {
            data_line = json_resource["game_server_name"];
            strip_leading_and_trailing_quotes(data_line);
            gs.set_server_name(std::move(data_line));
        }
        else
        {
            found_missing_config_setting = true;
            gs.set_server_name("TinyRcon game server");
        }

        if (json_resource.contains("rcon_server_ip_address"))
        {
            data_line = json_resource["rcon_server_ip_address"];
            strip_leading_and_trailing_quotes(data_line);
            gs.set_server_ip_address(std::move(data_line));
        }
        else
        {
            found_missing_config_setting = true;
            gs.set_server_ip_address("185.158.113.146");
        }

        if (json_resource.contains("rcon_port"))
        {
            const int port_number{json_resource["rcon_port"].template get<int>()};
            gs.set_server_port(static_cast<uint_least16_t>(port_number));
        }
        else
        {
            found_missing_config_setting = true;
            gs.set_server_port(28995);
        }

        if (json_resource.contains("rcon_password"))
        {
            data_line = json_resource["rcon_password"];
            strip_leading_and_trailing_quotes(data_line);
            gs.set_rcon_password(std::move(data_line));
        }
        else
        {
            found_missing_config_setting = true;
            gs.set_rcon_password("abc123");
        }

        if (json_resource.contains("private_slot_password"))
        {
            data_line = json_resource["private_slot_password"];
            strip_leading_and_trailing_quotes(data_line);
            gs.set_private_slot_password(std::move(data_line));
        }
        else
        {
            found_missing_config_setting = true;
            gs.set_private_slot_password("abc123");
        }

        main_app.get_game_servers()[0] = std::move(gs);
        ++parsed_rcon_game_server_index;
        main_app.set_rcon_game_servers_count(1);
        main_app.set_game_servers_count(1);
    }

    if (json_resource.contains("check_for_banned_players_time_interval"))
    {
        main_app.set_check_for_banned_players_time_period(
            json_resource["check_for_banned_players_time_interval"].template get<int>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_check_for_banned_players_time_period(5u);
    }

    if (json_resource.contains("game_executable_paths") && json_resource.at("game_executable_paths").is_object())
    {
        auto &game_executable_paths_json_object = json_resource["game_executable_paths"];
        if (game_executable_paths_json_object.contains("codmp_exe_path"))
        {
            data_line = game_executable_paths_json_object["codmp_exe_path"];
            replace_forward_slash_with_backward_slash(data_line);
            strip_leading_and_trailing_quotes(data_line);
            main_app.set_codmp_exe_path(data_line);
            if (!check_if_file_path_exists(main_app.get_codmp_exe_path().c_str()))
            {
                main_app.set_codmp_exe_path("");
            }
        }
        else
        {
            found_missing_config_setting = true;
            main_app.set_codmp_exe_path("");
        }

        if (!check_if_cod1_multiplayer_game_launch_command_is_correct(main_app.get_codmp_exe_path()))
        {
            main_app.set_codmp_exe_path("");
        }

        if (game_executable_paths_json_object.contains("cod2mp_s_exe_path"))
        {
            data_line = game_executable_paths_json_object["cod2mp_s_exe_path"];
            replace_forward_slash_with_backward_slash(data_line);
            strip_leading_and_trailing_quotes(data_line);
            main_app.set_cod2mp_exe_path(data_line);
            if (!check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str()))
            {
                main_app.set_cod2mp_exe_path("");
            }
        }
        else
        {
            found_missing_config_setting = true;
            main_app.set_cod2mp_exe_path("");
        }

        if (!check_if_cod2_multiplayer_game_launch_command_is_correct(main_app.get_cod2mp_exe_path()))
        {
            main_app.set_cod2mp_exe_path("");
        }

        if (game_executable_paths_json_object.contains("iw3mp_exe_path"))
        {
            data_line = game_executable_paths_json_object["iw3mp_exe_path"];
            replace_forward_slash_with_backward_slash(data_line);
            strip_leading_and_trailing_quotes(data_line);
            main_app.set_iw3mp_exe_path(data_line);
            if (!check_if_file_path_exists(main_app.get_iw3mp_exe_path().c_str()))
            {
                main_app.set_iw3mp_exe_path("");
            }
        }
        else
        {
            found_missing_config_setting = true;
            main_app.set_iw3mp_exe_path("");
        }

        if (!check_if_cod4_multiplayer_game_launch_command_is_correct(main_app.get_iw3mp_exe_path()))
        {
            main_app.set_iw3mp_exe_path("");
        }

        if (game_executable_paths_json_object.contains("cod5mp_exe_path"))
        {
            data_line = game_executable_paths_json_object["cod5mp_exe_path"];
            replace_forward_slash_with_backward_slash(data_line);
            strip_leading_and_trailing_quotes(data_line);
            main_app.set_cod5mp_exe_path(data_line);
            if (!check_if_file_path_exists(main_app.get_cod5mp_exe_path().c_str()))
            {
                main_app.set_cod5mp_exe_path("");
            }
        }
        else
        {
            found_missing_config_setting = true;
            main_app.set_cod5mp_exe_path("");
        }

        if (!check_if_cod5_multiplayer_game_launch_command_is_correct(main_app.get_cod5mp_exe_path()))
        {
            main_app.set_cod5mp_exe_path("");
        }
    }

    // if (json_resource.contains("players_stats_feature") &&
    // json_resource.at("players_stats_feature").is_object())
    //{
    //	auto &players_stats_feature_json_object =
    // json_resource["players_stats_feature"]; 	if
    //(players_stats_feature_json_object.contains("enabled"))
    //	{
    //		main_app.set_is_enable_players_stats_feature(players_stats_feature_json_object["enabled"].template
    // get<bool>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_is_enable_players_stats_feature(false);
    //	}

    //	if
    //(players_stats_feature_json_object.contains("enable_tracking_of_player_stats_data_for_day"))
    //	{
    //		main_app.set_is_enable_tracking_of_player_stats_data_for_day(players_stats_feature_json_object["enable_tracking_of_player_stats_data_for_day"].template
    // get<bool>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_is_enable_tracking_of_player_stats_data_for_day(false);
    //	}

    //	if
    //(players_stats_feature_json_object.contains("enable_tracking_of_player_stats_data_for_month"))
    //	{
    //		main_app.set_is_enable_tracking_of_player_stats_data_for_month(players_stats_feature_json_object["enable_tracking_of_player_stats_data_for_month"].template
    // get<bool>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_is_enable_tracking_of_player_stats_data_for_month(false);
    //	}

    //	if
    //(players_stats_feature_json_object.contains("enable_tracking_of_player_stats_data_for_year"))
    //	{
    //		main_app.set_is_enable_tracking_of_player_stats_data_for_year(players_stats_feature_json_object["enable_tracking_of_player_stats_data_for_year"].template
    // get<bool>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_is_enable_tracking_of_player_stats_data_for_year(false);
    //	}

    //	if
    //(players_stats_feature_json_object.contains("enable_tracking_of_player_stats_data_permanently"))
    //	{
    //		main_app.set_is_enable_tracking_of_player_stats_data_permanently(players_stats_feature_json_object["enable_tracking_of_player_stats_data_permanently"].template
    // get<bool>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_is_enable_tracking_of_player_stats_data_permanently(false);
    //	}

    //	// number_of_top_players_to_display_in_game_chat
    //	if
    //(players_stats_feature_json_object.contains("number_of_top_players_to_display_in_game_chat"))
    //	{
    //		main_app.set_number_of_top_players_to_display_in_game_chat(players_stats_feature_json_object["number_of_top_players_to_display_in_game_chat"].template
    // get<int>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_number_of_top_players_to_display_in_game_chat(10u);
    //	}

    //	// number_of_top_players_to_display_in_tinyrcon
    //	if
    //(players_stats_feature_json_object.contains("number_of_top_players_to_display_in_tinyrcon"))
    //	{
    //		main_app.set_number_of_top_players_to_display_in_tinyrcon(players_stats_feature_json_object["number_of_top_players_to_display_in_tinyrcon"].template
    // get<int>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_number_of_top_players_to_display_in_tinyrcon(25u);
    //	}

    //	if
    //(players_stats_feature_json_object.contains("time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat"))
    //	{
    //		main_app.set_time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat(players_stats_feature_json_object["time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat"].template
    // get<int>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat(45u);
    //	}

    //	if
    //(players_stats_feature_json_object.contains("time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon"))
    //	{
    //		main_app.set_time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon(players_stats_feature_json_object["time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon"].template
    // get<int>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon(45u);
    //	}

    //	if
    //(players_stats_feature_json_object.contains("time_period_in_minutes_for_saving_players_stats_data"))
    //	{
    //		main_app.set_time_period_in_minutes_for_saving_players_stats_data(players_stats_feature_json_object["time_period_in_minutes_for_saving_players_stats_data"].template
    // get<int>());
    //	}
    //	else
    //	{
    //		found_missing_config_setting = true;
    //		main_app.set_time_period_in_minutes_for_saving_players_stats_data(30u);
    //	}
    //}

    if (json_resource.contains("tiny_rcon_server_ip"))
    {
        data_line = json_resource["tiny_rcon_server_ip"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_tiny_rcon_server_ip_address(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_tiny_rcon_server_ip_address("85.222.189.119");
    }

    // if (json_resource.contains("tiny_rcon_server_port"))
    // {
    // 	const int port_number{json_resource["tiny_rcon_server_port"].template
    // get<int>()};
    // 	main_app.set_tiny_rcon_server_port(static_cast<uint_least16_t>(port_number));
    // }
    // else
    // {
    // 	found_missing_config_setting = true;
    // 	main_app.set_tiny_rcon_server_port(27015);
    // }

    if (json_resource.contains("tiny_rcon_server_ip_for_players"))
    {
        data_line = json_resource["tiny_rcon_server_ip_for_players"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_tiny_rcon_server_ip_address_for_players(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_tiny_rcon_server_ip_address_for_players("85.222.189.119");
    }

    // if (json_resource.contains("tiny_rcon_server_port_for_players"))
    // {
    // 	const int
    // port_number{json_resource["tiny_rcon_server_port_for_players"].template
    // get<int>()};
    // 	main_app.set_tiny_rcon_server_port_for_players(static_cast<uint_least16_t>(port_number));
    // }
    // else
    // {
    // 	found_missing_config_setting = true;
    // 	main_app.set_tiny_rcon_server_port_for_players(27017);
    // }

    if (json_resource.contains("tiny_rcon_ftp_server_username"))
    {
        data_line = json_resource["tiny_rcon_ftp_server_username"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_tiny_rcon_ftp_server_username(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_tiny_rcon_ftp_server_username("tinyrcon");
    }

    if (json_resource.contains("tiny_rcon_ftp_server_password"))
    {
        data_line = json_resource["tiny_rcon_ftp_server_password"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_tiny_rcon_ftp_server_password(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_tiny_rcon_ftp_server_password("08021980");
    }

    if (json_resource.contains("enable_automatic_connection_flood_ip_ban"))
    {

        main_app.set_is_enable_automatic_connection_flood_ip_ban(
            json_resource["enable_automatic_connection_flood_ip_ban"].template get<bool>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_is_enable_automatic_connection_flood_ip_ban(false);
    }

    if (json_resource.contains("minimum_number_of_connections_from_same_ip_for_automatic_ban"))
    {
        main_app.get_current_game_server().set_minimum_number_of_connections_from_same_ip_for_automatic_ban(
            json_resource["minimum_number_of_connections_from_same_ip_for_automatic_ban"].template get<int>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_current_game_server().set_minimum_number_of_connections_from_same_ip_for_automatic_ban(5);
    }

    if (json_resource.contains("number_of_warnings_for_automatic_kick"))
    {

        main_app.get_current_game_server().set_maximum_number_of_warnings_for_automatic_kick(
            json_resource["number_of_warnings_for_automatic_kick"].template get<int>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_current_game_server().set_maximum_number_of_warnings_for_automatic_kick(2);
    }

    if (json_resource.contains("disable_automatic_kick_messages"))
    {
        main_app.set_is_disable_automatic_kick_messages(
            json_resource["disable_automatic_kick_messages"].template get<bool>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_is_disable_automatic_kick_messages(false);
    }

    if (json_resource.contains("use_original_admin_messages"))
    {
        main_app.set_is_use_original_admin_messages(json_resource["use_original_admin_messages"].template get<bool>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_is_use_original_admin_messages(true);
    }

    if (json_resource.contains("user_defined_warn_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_warn_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_warn_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_warn_msg"] =
                "^7{PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. "
                "^3Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_warn_msg"] =
            "^7{PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. ^3Reason: "
            "^1{REASON}";
    }

    if (json_resource.contains("user_defined_kick_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_kick_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_kick_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_kick_msg"] =
                "^7{PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. "
                "^3Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_kick_msg"] =
            "^7{PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. ^3Reason: "
            "^1{REASON}";
    }

    if (json_resource.contains("user_defined_temp_ban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_temp_ban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_temp_ban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_temp_ban_msg"] =
                "^7{PLAYERNAME} ^7you are being ^1temporarily banned ^7for "
                "^1{TEMPBAN_DURATION} hours ^7by ^1admin {ADMINNAME}.{{br}}^3Reason: "
                "^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_temp_ban_msg"] =
            "^7{PLAYERNAME} ^7you are being ^1temporarily banned ^7for "
            "^1{TEMPBAN_DURATION} hours ^7by ^1admin {ADMINNAME}.{{br}}^3Reason: "
            "^1{REASON}";
    }

    if (json_resource.contains("user_defined_ban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_ban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_ban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_ban_msg"] =
                "^7{PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. "
                "^3Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_ban_msg"] =
            "^7{PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. ^3Reason: "
            "^1{REASON}";
    }

    if (json_resource.contains("user_defined_ip_ban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_ip_ban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_ip_ban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_ip_ban_msg"] =
                "^7{PLAYERNAME} ^1you are being permanently banned by admin "
                "^5{ADMINNAME}. ^3Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_ip_ban_msg"] =
            "^7{PLAYERNAME} ^1you are being permanently banned by admin "
            "^5{ADMINNAME}. ^3Reason: ^1{REASON}";
    }

    if (json_resource.contains("user_defined_ip_address_range_ban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_ip_address_range_ban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_ip_address_range_ban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_ip_address_range_ban_msg"] =
                "^7Admin ^5{ADMINNAME} ^7has ^1banned IP address range "
                "^5{IP_ADDRESS_RANGE}. ^3Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_ip_address_range_ban_msg"] =
            "^7Admin ^5{ADMINNAME} ^7has ^1banned IP address range "
            "^5{IP_ADDRESS_RANGE}. ^3Reason: ^1{REASON}";
    }

    if (json_resource.contains("user_defined_name_ban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_name_ban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_name_ban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_name_ban_msg"] =
                "^7Admin ^5{ADMINNAME} ^7has ^1banned player name: ^7{PLAYERNAME}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_name_ban_msg"] =
            "^7Admin ^5{ADMINNAME} ^7has ^1banned player name: ^7{PLAYERNAME}";
        ;
    }

    if (json_resource.contains("user_defined_name_unban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_name_unban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_name_unban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_name_unban_msg"] =
                "^7Admin ^5{ADMINNAME} ^7has removed previously ^1banned player name: "
                "^7{PLAYERNAME}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_name_unban_msg"] =
            "^7Admin ^5{ADMINNAME} ^7has removed previously ^1banned player name: "
            "^7{PLAYERNAME}";
    }

    if (json_resource.contains("user_defined_city_ban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_city_ban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_city_ban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_city_ban_msg"] =
                "^7Admin ^5{ADMINNAME} ^1has globally banned city: ^5{CITY_NAME}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_city_ban_msg"] =
            "^7Admin ^5{ADMINNAME} ^1has globally banned city: ^5{CITY_NAME}";
        ;
    }

    if (json_resource.contains("user_defined_city_unban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_city_unban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_city_unban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_city_unban_msg"] =
                "^7Admin ^5{ADMINNAME} ^1has removed previously banned city: "
                "^5{CITY_NAME}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_city_unban_msg"] =
            "^7Admin ^5{ADMINNAME} ^1has removed previously banned city: "
            "^5{CITY_NAME}";
    }

    if (json_resource.contains("user_defined_enable_city_ban_feature_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_enable_city_ban_feature_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] =
                "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players "
                "with ^1IP addresses ^7from banned cities.";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_enable_city_ban_feature_msg"] =
            "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with "
            "^1IP addresses ^7from banned cities.";
    }

    if (json_resource.contains("user_defined_disable_city_ban_feature_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_disable_city_ban_feature_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] =
                "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players "
                "with ^1IP addresses ^7from banned cities.";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_disable_city_ban_feature_msg"] =
            "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players "
            "with ^1IP addresses ^7from banned cities.";
    }

    if (json_resource.contains("user_defined_country_ban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_country_ban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_country_ban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_country_ban_msg"] =
                "^7Admin ^5{ADMINNAME} ^1has globally banned country: ^5{COUNTRY_NAME}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_country_ban_msg"] =
            "^7Admin ^5{ADMINNAME} ^1has globally banned country: ^5{COUNTRY_NAME}";
        ;
    }

    if (json_resource.contains("user_defined_country_unban_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_country_unban_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_country_unban_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_country_unban_msg"] =
                "^7Admin ^5{ADMINNAME} ^1has removed previously banned country: "
                "^5{COUNTRY_NAME}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_country_unban_msg"] =
            "^7Admin ^5{ADMINNAME} ^1has removed previously banned country: "
            "^5{COUNTRY_NAME}";
    }

    if (json_resource.contains("user_defined_enable_country_ban_feature_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_enable_country_ban_feature_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] =
                "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players "
                "with ^1IP addresses ^7from banned countries.";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_enable_country_ban_feature_msg"] =
            "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with "
            "^1IP addresses ^7from banned countries.";
    }

    if (json_resource.contains("user_defined_disable_country_ban_feature_msg"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_disable_country_ban_feature_msg"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] =
                "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players "
                "with ^1IP addresses ^7from banned countries.";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_disable_country_ban_feature_msg"] =
            "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players "
            "with ^1IP addresses ^7from banned countries.";
    }

    if (json_resource.contains("user_defined_protect_ip_address_message"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_protect_ip_address_message"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_protect_ip_address_message"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_protect_ip_address_message"] =
                "^1{ADMINNAME} ^7has protected ^1IP address ^7of "
                "{PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_protect_ip_address_message"] =
            "^1{ADMINNAME} ^7has protected ^1IP address ^7of "
            "{PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}";
    }

    if (json_resource.contains("user_defined_unprotect_ip_address_message"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_unprotect_ip_address_message"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_unprotect_ip_address_message"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_unprotect_ip_address_message"] =
                "^1{ADMINNAME} ^7has removed a previously protected ^1IP "
                "address^7.{{br}}^5Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_unprotect_ip_address_message"] =
            "^1{ADMINNAME} ^7has removed a previously protected ^1IP "
            "address^7.{{br}}^5Reason: ^1{REASON}";
    }

    if (json_resource.contains("user_defined_protect_ip_address_range_message"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_protect_ip_address_range_message"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_protect_ip_address_range_message"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_protect_ip_address_range_message"] =
                "^1{ADMINNAME} ^7has protected ^1IP address range ^7of "
                "{PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_protect_ip_address_range_message"] =
            "^1{ADMINNAME} ^7has protected ^1IP address range ^7of "
            "{PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}";
    }

    if (json_resource.contains("user_defined_unprotect_ip_address_range_message"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_unprotect_ip_address_range_message"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_unprotect_ip_address_range_message"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_unprotect_ip_address_range_message"] =
                "^1{ADMINNAME} ^7has removed a previously ^1protected IP address "
                "range^7.{{br}}^5Reason: ^1{REASON}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_unprotect_ip_address_range_message"] =
            "^1{ADMINNAME} ^7has removed a previously ^1protected IP address "
            "range^7.{{br}}^5Reason: ^1{REASON}";
    }

    if (json_resource.contains("user_defined_protect_city_message"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_protect_city_message"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_protect_city_message"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_protect_city_message"] =
                "^1{ADMINNAME} ^7has protected ^1city {CITY_NAME} ^7of {PLAYERNAME}^7.";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_protect_city_message"] =
            "^1{ADMINNAME} ^7has protected ^1city {CITY_NAME} ^7of {PLAYERNAME}^7.";
    }

    if (json_resource.contains("user_defined_unprotect_city_message"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_unprotect_city_message"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_unprotect_city_message"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_unprotect_city_message"] =
                "^1{ADMINNAME} ^7has removed a previously ^1protected city: "
                "{CITY_NAME}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_unprotect_city_message"] =
            "^1{ADMINNAME} ^7has removed a previously ^1protected city: {CITY_NAME}";
    }

    if (json_resource.contains("user_defined_protect_country_message"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_protect_country_message"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_protect_country_message"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_protect_country_message"] =
                "^1{ADMINNAME} ^7has protected ^1country {COUNTRY_NAME} ^7of "
                "{PLAYERNAME}^7.";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_protect_country_message"] =
            "^1{ADMINNAME} ^7has protected ^1country {COUNTRY_NAME} ^7of "
            "{PLAYERNAME}^7.";
    }

    if (json_resource.contains("user_defined_unprotect_country_message"))
    {
        if (!main_app.get_is_use_original_admin_messages())
        {
            data_line = json_resource["user_defined_unprotect_country_message"];
            strip_leading_and_trailing_quotes(data_line);
            main_app.get_admin_messages()["user_defined_unprotect_country_message"] = std::move(data_line);
        }
        else
        {
            main_app.get_admin_messages()["user_defined_unprotect_country_message"] =
                "^1{ADMINNAME} ^7has removed a previously ^1protected country: "
                "{COUNTRY_NAME}";
        }
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["user_defined_unprotect_country_message"] =
            "^1{ADMINNAME} ^7has removed a previously ^1protected country: "
            "{COUNTRY_NAME}";
    }

    if (json_resource.contains("automatic_remove_temp_ban_msg"))
    {

        main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] =
            "^7{PLAYERNAME}'s ^1temporary ban ^7[start date: ^3{TEMP_BAN_START_DATE} "
            "^7expired on ^3{TEMP_BAN_END_DATE}]{{br}}^7has automatically been "
            "removed. ^5Reason of ban: ^1{REASON}";
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["automatic_remove_temp_ban_msg"] =
            "^7{PLAYERNAME}'s ^1temporary ban ^7[start date: ^3{TEMP_BAN_START_DATE} "
            "^7expired on ^3{TEMP_BAN_END_DATE}]{{br}}^7has automatically been "
            "removed. ^5Reason of ban: ^1{REASON}";
    }

    if (json_resource.contains("automatic_kick_temp_ban_msg"))
    {
        main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] =
            "^7Temporarily banned player {PLAYERNAME} ^7is being automatically "
            "^1kicked.{{br}}^7Your temporary ban expires in "
            "^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of "
            "ban: ^1{TEMP_BAN_START_DATE}";
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["automatic_kick_temp_ban_msg"] =
            "^7Temporarily banned player {PLAYERNAME} ^7is being automatically "
            "^1kicked.{{br}}^7Your temporary ban expires in "
            "^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of "
            "ban: ^1{TEMP_BAN_START_DATE}";
    }

    if (json_resource.contains("automatic_kick_ip_ban_msg"))
    {
        main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is "
            "being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| "
            "^5Date of ban: ^1{BAN_DATE}";
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["automatic_kick_ip_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is "
            "being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| "
            "^5Date of ban: ^1{BAN_DATE}";
    }

    if (json_resource.contains("automatic_kick_ip_address_range_ban_msg"))
    {
        main_app.get_admin_messages()["automatic_kick_ip_address_range_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with an ^1IP address ^7from a previously "
            "^1banned IP address range ^7is being automatically "
            "^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: "
            "^1{BAN_DATE}";
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["automatic_kick_ip_address_range_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with an ^1IP address ^7from a previously "
            "^1banned IP address range ^7is being automatically "
            "^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: "
            "^1{BAN_DATE}";
    }

    if (json_resource.contains("automatic_kick_city_ban_msg"))
    {
        main_app.get_admin_messages()["automatic_kick_city_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with an IP address from a ^1banned city: "
            "^5{CITY_NAME} ^7is being automatically ^1kicked.";
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["automatic_kick_city_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with an IP address from a ^1banned city: "
            "^5{CITY_NAME} ^7is being automatically ^1kicked.";
    }

    if (json_resource.contains("automatic_kick_country_ban_msg"))
    {
        main_app.get_admin_messages()["automatic_kick_country_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with an IP address from a ^1banned country:  "
            "^5{COUNTRY_NAME} ^7is being automatically ^1kicked.";
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["automatic_kick_country_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with an IP address from a ^1banned country:  "
            "^5{COUNTRY_NAME} ^7is being automatically ^1kicked.";
    }

    if (json_resource.contains("automatic_kick_name_ban_msg"))
    {
        main_app.get_admin_messages()["automatic_kick_name_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with a previously ^1banned player name ^7is "
            "being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| "
            "^5Date of ban: ^1{BAN_DATE}";
    }
    else
    {
        found_missing_config_setting = true;
        main_app.get_admin_messages()["automatic_kick_name_ban_msg"] =
            "^7Player {PLAYERNAME} ^7with a previously ^1banned player name ^7is "
            "being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| "
            "^5Date of ban: ^1{BAN_DATE}";
    }

    if (json_resource.contains("current_match_info"))
    {
        data_line = json_resource["current_match_info"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_current_match_info(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_current_match_info("^3Map: {MAP_FULL_NAME} ^1({MAP_RCON_NAME}^1) ^3| Gametype: "
                                        "{GAMETYPE_FULL_NAME} ^3| Online/Offline players: "
                                        "{ONLINE_PLAYERS_COUNT}^3|{OFFLINE_PLAYERS_COUNT}");
    }

    if (json_resource.contains("use_different_background_colors_for_even_and_odd_lines"))
    {
        main_app.set_is_use_different_background_colors_for_even_and_odd_lines(
            json_resource["use_different_background_colors_for_even_and_odd_lines"].template get<bool>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_is_use_different_background_colors_for_even_and_odd_lines(true);
    }

    if (json_resource.contains("odd_player_data_lines_bg_color"))
    {
        data_line = json_resource["odd_player_data_lines_bg_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_odd_player_data_lines_bg_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_odd_player_data_lines_bg_color("^0");
    }

    if (json_resource.contains("even_player_data_lines_bg_color"))
    {
        data_line = json_resource["even_player_data_lines_bg_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_even_player_data_lines_bg_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_even_player_data_lines_bg_color("^8");
    }

    if (json_resource.contains("use_different_foreground_colors_for_even_and_odd_lines"))
    {
        main_app.set_is_use_different_foreground_colors_for_even_and_odd_lines(
            json_resource["use_different_foreground_colors_for_even_and_odd_lines"].template get<bool>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_is_use_different_foreground_colors_for_even_and_odd_lines(false);
    }

    if (json_resource.contains("odd_player_data_lines_fg_color"))
    {
        data_line = json_resource["odd_player_data_lines_fg_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_odd_player_data_lines_fg_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_odd_player_data_lines_fg_color("^5");
    }

    if (json_resource.contains("even_player_data_lines_fg_color"))
    {
        data_line = json_resource["even_player_data_lines_fg_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_even_player_data_lines_fg_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_even_player_data_lines_fg_color("^5");
    }

    if (json_resource.contains("full_map_name_color"))
    {

        data_line = json_resource["full_map_name_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_full_map_name_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_full_map_name_color("^2");
    }

    if (json_resource.contains("rcon_map_name_color"))
    {
        data_line = json_resource["rcon_map_name_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_rcon_map_name_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_rcon_map_name_color("^1");
    }

    if (json_resource.contains("full_game_type_color"))
    {
        data_line = json_resource["full_game_type_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_full_gametype_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_full_gametype_color("^2");
    }

    if (json_resource.contains("rcon_game_type_color"))
    {
        data_line = json_resource["rcon_game_type_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_rcon_gametype_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_rcon_gametype_color("^1");
    }

    if (json_resource.contains("online_players_count_color"))
    {
        data_line = json_resource["online_players_count_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_online_players_count_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_online_players_count_color("^2");
    }

    if (json_resource.contains("offline_players_count_color"))
    {
        data_line = json_resource["offline_players_count_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_offline_players_count_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_offline_players_count_color("^1");
    }

    if (json_resource.contains("border_line_color"))
    {
        data_line = json_resource["border_line_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_border_line_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_border_line_color("^5");
    }

    if (json_resource.contains("header_player_pid_color"))
    {
        data_line = json_resource["header_player_pid_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_header_player_pid_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_header_player_pid_color("^1");
    }

    if (json_resource.contains("data_player_pid_color"))
    {
        data_line = json_resource["data_player_pid_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_data_player_pid_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_data_player_pid_color("^1");
    }

    if (json_resource.contains("header_player_score_color"))
    {
        data_line = json_resource["header_player_score_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_header_player_score_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_header_player_score_color("^4");
    }

    if (json_resource.contains("data_player_score_color"))
    {
        data_line = json_resource["data_player_score_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_data_player_score_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_data_player_score_color("^4");
    }

    if (json_resource.contains("header_player_ping_color"))
    {
        data_line = json_resource["header_player_ping_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_header_player_ping_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_header_player_ping_color("^4");
    }

    if (json_resource.contains("data_player_ping_color"))
    {
        data_line = json_resource["data_player_ping_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_data_player_ping_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_data_player_ping_color("^4");
    }

    if (json_resource.contains("header_player_name_color"))
    {
        data_line = json_resource["header_player_name_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_header_player_name_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_header_player_name_color("^4");
    }

    if (json_resource.contains("header_player_ip_color"))
    {
        data_line = json_resource["header_player_ip_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_header_player_ip_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_header_player_ip_color("^4");
    }
    if (json_resource.contains("data_player_ip_color"))
    {
        data_line = json_resource["data_player_ip_color"];
        strip_leading_and_trailing_quotes(data_line);
        main_app.set_data_player_ip_color(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_data_player_ip_color("^4");
    }

    if (json_resource.contains("header_player_geoinfo_color"))
    {
        data_line = json_resource["header_player_geoinfo_color"];
        strip_leading_and_trailing_quotes(data_line);
        // main_app.set_header_player_geoinfo_color(std::move(data_line));
        main_app.set_header_player_geoinfo_color("^4");
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_header_player_geoinfo_color("^4");
    }

    if (json_resource.contains("data_player_geoinfo_color"))
    {
        data_line = json_resource["data_player_geoinfo_color"];
        strip_leading_and_trailing_quotes(data_line);
        // main_app.set_data_player_geoinfo_color(std::move(data_line));
        main_app.set_data_player_geoinfo_color("^4");
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_data_player_geoinfo_color("^4");
    }

    if (json_resource.contains("draw_border_lines"))
    {
        main_app.set_is_draw_border_lines(json_resource["draw_border_lines"].template get<bool>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_is_draw_border_lines(true);
    }

    if (json_resource.contains("ftp_download_site_ip_address"))
    {
        data_line = json_resource["ftp_download_site_ip_address"];
        strip_leading_and_trailing_quotes(data_line);
        trim_in_place(data_line, "/\\ \t\n\f\v");
        if (data_line.starts_with("fttp://") || data_line.starts_with("http://"))
        {
            data_line.erase(0, 7);
        }
        main_app.set_ftp_download_site_ip_address(data_line);
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_ftp_download_site_ip_address("85.222.189.119");
    }

    if (json_resource.contains("ftp_download_folder_path"))
    {
        data_line = json_resource["ftp_download_folder_path"];
        strip_leading_and_trailing_quotes(data_line);
        trim_in_place(data_line, "/\\ \t\n\f\v");
        main_app.set_ftp_download_folder_path(data_line);
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_ftp_download_folder_path("tinyrcon");
    }

    if (json_resource.contains("ftp_download_file_pattern"))
    {
        data_line = json_resource["ftp_download_file_pattern"];
        strip_leading_and_trailing_quotes(data_line);
        trim_in_place(data_line);
        main_app.set_ftp_download_file_pattern(data_line);
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_ftp_download_file_pattern(R"(^_U_TinyRcon[\._-]?v?(\d{1,2}\.\d{1,2}\.\d{1,2}\.\d{1,2})\.exe$)");
    }

    if (json_resource.contains("plugins_geoIP_geo_dat_md5"))
    {
        data_line = json_resource["plugins_geoIP_geo_dat_md5"];
        strip_leading_and_trailing_quotes(data_line);
        trim_in_place(data_line);
        main_app.set_plugins_geoIP_geo_dat_md5(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_plugins_geoIP_geo_dat_md5("");
    }

    if (json_resource.contains("images_data_md5"))
    {
        data_line = json_resource["images_data_md5"];
        strip_leading_and_trailing_quotes(data_line);
        trim_in_place(data_line);
        main_app.set_images_data_md5(std::move(data_line));
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_images_data_md5("");
    }

    if (json_resource.contains("is_automatic_country_kick_enabled"))
    {
        main_app.set_is_automatic_country_kick_enabled(
            json_resource["is_automatic_country_kick_enabled"].template get<bool>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_is_automatic_country_kick_enabled(false);
    }

    auto &banned_countries = main_app.get_current_game_server().get_banned_countries_set();

    ifstream input_file1(main_app.get_banned_countries_file_path(), std::ios::in);
    if (input_file1)
    {
        for (string banned_country; getline(input_file1, banned_country);)
        {
            trim_in_place(banned_country);
            banned_countries.emplace(std::move(banned_country));
        }
    }

    if (json_resource.contains("is_automatic_city_kick_enabled"))
    {
        main_app.set_is_automatic_city_kick_enabled(
            json_resource["is_automatic_city_kick_enabled"].template get<bool>());
    }
    else
    {
        found_missing_config_setting = true;
        main_app.set_is_automatic_city_kick_enabled(false);
    }

    auto &banned_cities = main_app.get_current_game_server().get_banned_cities_set();

    ifstream input_file2(main_app.get_banned_cities_file_path(), std::ios::in);
    if (input_file2)
    {
        for (string banned_city; getline(input_file2, banned_city);)
        {
            trim_in_place(banned_city);
            banned_cities.emplace(std::move(banned_city));
        }
    }

    if (found_missing_config_setting)
    {
        write_tiny_rcon_json_settings_to_file(configFileName);
    }
}

void parse_protected_entries_file(const char *file_path, std::set<std::string> &protected_entries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ifstream input_file{file_path};
    if (!input_file)
    {
        const size_t buffer_size{1024};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"^3Couldn't open file at specified path ^1("s + string{file_path} + ")^3! "s + buffer};
        print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        ofstream bannedIPsFileToWrite{file_path};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
            print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        }
    }
    else
    {
        protected_entries.clear();
        string readData;
        while (getline(input_file, readData))
        {
            stl::helper::trim_in_place(readData);
            string entry;
            for (const auto ch : readData)
            {
                if (isprint(ch) || ' ' == ch)
                    entry.push_back(ch);
            }
            readData = std::move(entry);
            if (!protected_entries.contains(readData))
            {
                protected_entries.emplace(readData);
            }
        }
    }
}

void save_protected_entries_file(const char *file_path, const std::set<std::string> &protected_entries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ofstream output{file_path};
    if (output)
    {

        for (const auto &entry : protected_entries)
        {
            output << entry << '\n';
        }
        output << flush;
        output.close();
    }
}

void parse_tempbans_data_file(const char *file_path, std::vector<player> &temp_banned_players,
                              std::unordered_map<std::string, player> &ip_to_temp_banned_player)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string property_key, property_value;
    ifstream input_file{file_path};
    if (!input_file)
    {
        const size_t buffer_size{1024};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"^3Couldn't open file at specified path ^1("s + string{file_path} + ")^3! "s + buffer};
        print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        ofstream bannedIPsFileToWrite{file_path};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
            print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        }
    }
    else
    {
        ip_to_temp_banned_player.clear();
        temp_banned_players.clear();
        string readData, information_about_protected_player;
        while (getline(input_file, readData))
        {
            stl::helper::trim_in_place(readData, " \t\n");
            vector<string> parts{stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes,
                                                        ignore_empty_string_t::no)};
            for (auto &part : parts)
                stl::helper::trim_in_place(part, " \t\n");
            if (parts.size() < 6)
                continue;
            if (!ip_to_temp_banned_player.contains(parts[0]))
            {
                player temp_banned_player_data{};
                temp_banned_player_data.ip_address = parts[0];
                strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name),
                         parts[1].c_str());
                temp_banned_player_data.banned_start_time = stoll(parts[3]);
                const string converted_ban_date_and_time_info{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", temp_banned_player_data.banned_start_time)};
                strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time),
                         converted_ban_date_and_time_info.c_str());
                temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
                temp_banned_player_data.reason = std::move(parts[5]);
                temp_banned_player_data.banned_by_user_name = (parts.size() >= 7) ? parts[6] : main_app.get_username();
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                                 temp_banned_player_data.ip_address, temp_banned_player_data);
                if (check_if_player_is_protected(temp_banned_player_data, "^7enable ^1tempban ^7for",
                                                 information_about_protected_player))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
                    continue;
                }
                ip_to_temp_banned_player.emplace(temp_banned_player_data.ip_address, temp_banned_player_data);
                temp_banned_players.push_back(std::move(temp_banned_player_data));
            }
        }

        sort(begin(temp_banned_players), end(temp_banned_players),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });
    }
}

void parse_banned_names_file(const char *file_path, std::vector<player> &banned_names_vector,
                             std::unordered_map<std::string, player> &banned_names_map)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string property_key, property_value;
    ifstream input_file{file_path};
    if (!input_file)
    {
        const size_t buffer_size{1024};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"^3Couldn't open file at specified path ^1("s + string{file_path} + ")^3! "s + buffer};
        print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        ofstream bannedIPsFileToWrite{file_path};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
            print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        }
    }
    else
    {
        banned_names_map.clear();
        banned_names_vector.clear();
        string readData, information_about_protected_player;
        while (getline(input_file, readData))
        {
            stl::helper::trim_in_place(readData);
            vector<string> parts = stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes,
                                                          ignore_empty_string_t::no);
            for (auto &part : parts)
                stl::helper::trim_in_place(part);
            if (parts.size() < 5 || parts[2].empty())
                continue;

            if (!banned_names_map.contains(parts[2]))
            {
                player bannedPlayerData{};
                bannedPlayerData.ip_address = parts[0];
                strcpy_s(bannedPlayerData.guid_key, std::size(bannedPlayerData.guid_key), parts[1].c_str());
                strcpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[2].c_str());
                bannedPlayerData.banned_start_time = get_number_of_seconds_from_date_and_time_string(parts[3]);
                const string converted_ban_date_and_time_info{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bannedPlayerData.banned_start_time)};
                strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time),
                         converted_ban_date_and_time_info.c_str());
                bannedPlayerData.reason = std::move(parts[4]);
                bannedPlayerData.banned_by_user_name = (parts.size() >= 6) ? parts[5] : main_app.get_username();
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                                 bannedPlayerData.ip_address, bannedPlayerData);
                if (check_if_player_is_protected(bannedPlayerData, "^7enable ^1player name ban ^7for",
                                                 information_about_protected_player))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
                    continue;
                }
                banned_names_map.emplace(bannedPlayerData.player_name, bannedPlayerData);
                banned_names_vector.push_back(std::move(bannedPlayerData));
            }
        }

        sort(begin(banned_names_vector), end(banned_names_vector),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });
    }
}

void parse_banned_ip_addresses_file(const char *file_path, std::vector<player> &banned_players,
                                    std::unordered_map<std::string, player> &ip_to_banned_player)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string property_key, property_value;
    ifstream input_file{file_path};
    if (!input_file)
    {
        const size_t buffer_size{1024};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"^3Couldn't open file at specified path ^1("s + string{file_path} + ")^3! "s + buffer};
        print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        ofstream bannedIPsFileToWrite{file_path};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
            print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        }
    }
    else
    {
        ip_to_banned_player.clear();
        banned_players.clear();
        string readData, information_about_protected_player;
        while (getline(input_file, readData))
        {
            stl::helper::trim_in_place(readData);
            vector<string> parts = stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes,
                                                          ignore_empty_string_t::no);
            for (auto &part : parts)
                stl::helper::trim_in_place(part);
            if (parts.size() < 5 || parts[0].empty())
                continue;

            if (!ip_to_banned_player.contains(parts[0]))
            {
                player bannedPlayerData{};
                bannedPlayerData.ip_address = parts[0];
                strcpy_s(bannedPlayerData.guid_key, std::size(bannedPlayerData.guid_key), parts[1].c_str());
                strcpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[2].c_str());
                bannedPlayerData.banned_start_time = get_number_of_seconds_from_date_and_time_string(parts[3]);
                const string converted_ban_date_and_time_info{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bannedPlayerData.banned_start_time)};
                strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time),
                         converted_ban_date_and_time_info.c_str());
                bannedPlayerData.reason = std::move(parts[4]);
                bannedPlayerData.banned_by_user_name = (parts.size() >= 6) ? parts[5] : main_app.get_username();
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                                 bannedPlayerData.ip_address, bannedPlayerData);
                if (check_if_player_is_protected(bannedPlayerData, "^7enable ^1IP ban ^7for",
                                                 information_about_protected_player))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
                    continue;
                }
                ip_to_banned_player.emplace(bannedPlayerData.ip_address, bannedPlayerData);
                banned_players.push_back(std::move(bannedPlayerData));
            }
        }

        sort(begin(banned_players), end(banned_players),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });
    }
}

void parse_banned_ip_address_ranges_file(const char *file_path, std::vector<player> &banned_ip_address_ranges,
                                         std::unordered_map<std::string, player> &ip_address_range_to_banned_player)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string property_key, property_value;
    ifstream input_file{file_path};
    if (!input_file)
    {
        const size_t buffer_size{1024};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"^3Couldn't open file at specified path ^1("s + string{file_path} + ")^3! "s + buffer};
        print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        ofstream bannedIPsFileToWrite{file_path};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
            print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        }
    }
    else
    {
        banned_ip_address_ranges.clear();
        ip_address_range_to_banned_player.clear();
        string readData, information_about_protected_player;
        while (getline(input_file, readData))
        {
            stl::helper::trim_in_place(readData);
            vector<string> parts = stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes,
                                                          ignore_empty_string_t::no);
            for (auto &part : parts)
                stl::helper::trim_in_place(part);
            if (parts.size() < 5)
                continue;
            if (!ip_address_range_to_banned_player.contains(parts[0]))
            {
                player bannedPlayerData{};
                strcpy_s(bannedPlayerData.guid_key, std::size(bannedPlayerData.guid_key), parts[1].c_str());
                strcpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[2].c_str());
                bannedPlayerData.banned_start_time = get_number_of_seconds_from_date_and_time_string(parts[3]);
                const string converted_ban_date_and_time_info{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bannedPlayerData.banned_start_time)};
                strcpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time),
                         converted_ban_date_and_time_info.c_str());
                bannedPlayerData.reason = std::move(parts[4]);
                bannedPlayerData.banned_by_user_name = (parts.size() >= 6) ? parts[5] : main_app.get_username();
                get_first_valid_ip_address_from_ip_address_range(parts[0], bannedPlayerData);

                if (check_if_player_is_protected(bannedPlayerData, "^7enable ^1IP address range ban ^7for",
                                                 information_about_protected_player))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, information_about_protected_player.c_str());
                    continue;
                }
                bannedPlayerData.ip_address = parts[0];
                ip_address_range_to_banned_player.emplace(parts[0], bannedPlayerData);
                banned_ip_address_ranges.push_back(std::move(bannedPlayerData));
            }
        }

        sort(begin(banned_ip_address_ranges), end(banned_ip_address_ranges),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });
    }
}

void parse_banned_cities_file(const char *file_path, std::set<std::string> &banned_cities)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string property_key, property_value;
    ifstream input_file{file_path};
    if (!input_file)
    {
        const size_t buffer_size{1024};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"^3Couldn't open file at specified path ^1("s + string{file_path} + ")^3! "s + buffer};
        print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        ofstream bannedIPsFileToWrite{file_path};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
            print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        }
    }
    else
    {
        const auto &protected_cities = main_app.get_current_game_server().get_protected_cities();
        banned_cities.clear();
        for (string readData; getline(input_file, readData);)
        {
            stl::helper::trim_in_place(readData);
            if (readData.empty())
                continue;
            /*string entry;
      for (const auto ch : readData) {
      if (isprint(ch) || ' ' == ch)
      entry.push_back(ch);
      }
      readData = std::move(entry);*/
            if (protected_cities.contains(readData))
            {
                const string information_about_removed_entry{
                    format("^3Banned city ^1{} ^3has been protected by an ^1admin^3!\n^5If you "
                           "want you can ^1unprotect ^5it using the ^1!unprotectcity {} "
                           "^5command.",
                           readData, readData)};
                print_colored_text(app_handles.hwnd_re_messages_data, information_about_removed_entry.c_str());
            }
            else
            {
                banned_cities.emplace(std::move(readData));
            }
        }
    }
}

void parse_banned_countries_file(const char *file_path, std::set<std::string> &banned_countries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string property_key, property_value;
    ifstream input_file{file_path};
    if (!input_file)
    {
        const size_t buffer_size{256U};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"Couldn't open file at specified path ("s + string{file_path} + ")! "s + buffer};
        show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
        ofstream bannedIPsFileToWrite{file_path};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign(format("Couldn't create file at {}!\nError: {}", file_path, buffer));

            show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
        }
    }
    else
    {
        const auto &protected_countries = main_app.get_current_game_server().get_protected_countries();
        banned_countries.clear();
        for (string readData; getline(input_file, readData);)
        {
            stl::helper::trim_in_place(readData);
            if (readData.empty())
                continue;
            /* string entry;
      for (const auto ch : readData) {
      if (isprint(ch) || ' ' == ch)
       entry.push_back(ch);
      }
      readData = std::move(entry);*/
            if (protected_countries.contains(readData))
            {
                const string information_about_removed_entry{
                    format("^3Banned country ^1{} ^3has been protected by an ^1admin^3!\n^5If "
                           "you want you can ^1unprotect ^5it using the ^1!unprotectcountry {} "
                           "^5command.",
                           readData, readData)};
                print_colored_text(app_handles.hwnd_re_messages_data, information_about_removed_entry.c_str());
            }
            else
            {
                banned_countries.emplace(std::move(readData));
            }
        }
    }
}

void save_tempbans_to_file(const char *file_path, const std::vector<player> &temp_banned_players)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ofstream output{file_path};
    if (output)
    {

        for (const auto &tb_player : temp_banned_players)
        {
            output << tb_player.ip_address << '\\' << tb_player.player_name << '\\' << tb_player.banned_date_time
                   << '\\' << tb_player.banned_start_time << '\\' << tb_player.ban_duration_in_hours << '\\'
                   << remove_disallowed_character_in_string(tb_player.reason) << '\\' << tb_player.banned_by_user_name
                   << '\n';
        }

        output << flush;
    }
}

void save_banned_ip_entries_to_file(const char *file_path, const std::vector<player> &banned_ip_entries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ofstream output{file_path};
    if (output)
    {

        for (const auto &banned_player : banned_ip_entries)
        {
            output << banned_player.ip_address << '\\' << banned_player.guid_key << '\\' << banned_player.player_name
                   << '\\' << banned_player.banned_date_time << '\\'
                   << remove_disallowed_character_in_string(banned_player.reason) << '\\'
                   << banned_player.banned_by_user_name << '\n';
        }

        output << flush;
    }
}

void save_banned_ip_address_range_entries_to_file(const char *file_path,
                                                  const std::vector<player> &banned_ip_address_ranges)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ofstream output{file_path};

    if (output)
    {
        for (const auto &banned_player : banned_ip_address_ranges)
        {
            string ip{banned_player.ip_address};
            if (!ip.ends_with(".*"))
            {
                const size_t last_dot_pos{ip.rfind('.')};
                if (last_dot_pos != string::npos)
                {
                    ip.replace(cbegin(ip) + last_dot_pos, cend(ip), ".*");
                }
            }
            output << ip << '\\' << banned_player.guid_key << '\\' << banned_player.player_name << '\\'
                   << banned_player.banned_date_time << '\\'
                   << remove_disallowed_character_in_string(banned_player.reason) << '\\'
                   << banned_player.banned_by_user_name << '\n';
        }

        output << flush;
    }
}

void save_banned_cities_to_file(const char *file_path, const std::set<std::string> &banned_cities)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ofstream output{file_path};

    if (output)
    {
        for (const auto &banned_city : banned_cities)
        {
            output << banned_city << '\n';
        }

        output << flush;
    }
}

void save_banned_countries_to_file(const char *file_path, const std::set<std::string> &banned_countries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ofstream output{file_path};

    if (output)
    {
        for (const auto &banned_country : banned_countries)
        {
            output << banned_country << '\n';
        }

        output << flush;
    }
}

bool temp_ban_player_ip_address(player &pd)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    using namespace std::literals;

    if (main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(pd.ip_address))
        return true;

    unsigned long ip_key{};
    if (!check_ip_address_validity(pd.ip_address, ip_key))
    {
        return false;
    }

    if (pd.country_name == nullptr || len(pd.country_name) == 0)
    {
        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);
    }

    pd.banned_start_time = get_current_time_stamp();
    const string banned_date_time_str{get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time)};
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    pd.banned_by_user_name = main_app.get_username();

    ofstream temp_banned_data_file_to_read{main_app.get_temp_bans_file_path(), ios_base::app};
    if (!temp_banned_data_file_to_read)
    {
        const size_t buffer_size{256U};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"Couldn't open 'tempbans.txt' data file at specified path ("s +
                            string{main_app.get_temp_bans_file_path()} + ")! "s + buffer};
        show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
        ofstream temp_banned_data_file_to_write{main_app.get_temp_bans_file_path()};
        if (!temp_banned_data_file_to_write)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign("Couldn't create 'tempbans.txt' file for holding temporarily banned IP addresses at "s
                                "data\\tempbans.txt!"s +
                                buffer);
            show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
            return false;
        }

        temp_banned_data_file_to_write << pd.ip_address << '\\' << pd.player_name << '\\' << pd.banned_date_time << '\\'
                                       << pd.banned_start_time << '\\' << pd.ban_duration_in_hours << '\\'
                                       << remove_disallowed_character_in_string(pd.reason) << '\\'
                                       << main_app.get_username() << endl;
    }
    else
    {

        temp_banned_data_file_to_read << pd.ip_address << '\\' << pd.player_name << '\\' << pd.banned_date_time << '\\'
                                      << pd.banned_start_time << '\\' << pd.ban_duration_in_hours << '\\'
                                      << remove_disallowed_character_in_string(pd.reason) << '\\'
                                      << main_app.get_username() << endl;
    }

    main_app.get_current_game_server().add_ip_address_to_temp_banned_ip_addresses(pd.ip_address, pd);

    return true;
}

bool global_ban_player_ip_address(player &pd)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    using namespace std::literals;

    if (main_app.get_current_game_server().get_banned_ip_addresses_map().contains(pd.ip_address))
        return true;

    unsigned long ip_key{};
    if (!check_ip_address_validity(pd.ip_address, ip_key))
    {
        return false;
    }

    if (pd.country_name == nullptr || len(pd.country_name) == 0)
    {
        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);
    }

    pd.banned_start_time = get_current_time_stamp();
    const string banned_date_time_str{get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time)};
    strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
    pd.banned_by_user_name = main_app.get_username();

    ofstream bannedIPsFileToRead{main_app.get_ip_bans_file_path(), ios_base::app};
    if (!bannedIPsFileToRead)
    {
        const size_t buffer_size{256U};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"Couldn't open banned IP addresses data file at specified path ("s +
                            string{main_app.get_ip_bans_file_path()} + ")! "s + buffer};
        show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
        ofstream bannedIPsFileToWrite{main_app.get_ip_bans_file_path()};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign("Couldn't create banned IP addresses data file at "s
                                "data\\bans.txt!"s +
                                buffer);
            show_error(app_handles.hwnd_main_window, errorMessage.c_str(), 0);
            return false;
        }

        bannedIPsFileToWrite << pd.ip_address << '\\' << pd.guid_key << '\\' << pd.player_name << '\\'
                             << pd.banned_date_time << '\\' << remove_disallowed_character_in_string(pd.reason) << '\\'
                             << main_app.get_username() << endl;
    }
    else
    {
        bannedIPsFileToRead << pd.ip_address << '\\' << pd.guid_key << '\\' << pd.player_name << '\\'
                            << pd.banned_date_time << '\\' << remove_disallowed_character_in_string(pd.reason) << '\\'
                            << main_app.get_username() << endl;
    }

    main_app.get_current_game_server().add_ip_address_to_banned_ip_addresses(pd.ip_address, pd);

    return true;
}

bool add_temporarily_banned_ip_address(player &pd, vector<player> &temp_banned_players_data,
                                       unordered_map<string, player> &ip_to_temp_banned_player_data)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    unsigned long guid_number{};
    if (!check_ip_address_validity(pd.ip_address, guid_number) || ip_to_temp_banned_player_data.contains(pd.ip_address))
        return false;

    ip_to_temp_banned_player_data.emplace(pd.ip_address, pd);
    temp_banned_players_data.push_back(std::move(pd));

    save_tempbans_to_file(main_app.get_temp_bans_file_path(), temp_banned_players_data);

    return true;
}

std::pair<bool, player> remove_temp_banned_ip_address(const std::string &ip_address, std::string &message,
                                                      const bool is_automatic_temp_ban_remove,
                                                      const bool is_report_public_message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (!main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(ip_address))
    {
        return {false, player{}};
    }

    auto &temp_banned_players = main_app.get_current_game_server().get_temp_banned_ip_addresses_vector();

    const auto found_iter = find_if(std::begin(temp_banned_players), std::end(temp_banned_players),
                                    [&ip_address](const player &p) { return ip_address == p.ip_address; });

    if (found_iter != std::end(temp_banned_players))
    {
        if (is_report_public_message)
        {
            if (!is_automatic_temp_ban_remove)
            {
                const string buffer{format("^7Admin ({}^7) has manually removed ^1temporarily banned IP address "
                                           "^7for player {}. ^7Reason: ^1{}",
                                           main_app.get_username(), found_iter->player_name,
                                           remove_disallowed_character_in_string(found_iter->reason))};
                message.assign(buffer);
            }

            /*const string inform_msg{ format("{}\\{}", main_app.get_username(),
      message) }; main_app.add_message_to_queue(message_t("inform-message",
      inform_msg, true));*/
            // rcon_say(message, true);
        }

        const player pd{
            main_app.get_current_game_server().get_temp_banned_ip_addresses_map().at(found_iter->ip_address)};

        temp_banned_players.erase(remove_if(std::begin(temp_banned_players), std::end(temp_banned_players),
                                            [&ip_address](const player &p) { return ip_address == p.ip_address; }),
                                  std::end(temp_banned_players));

        main_app.get_current_game_server().get_temp_banned_ip_addresses_map().erase(pd.ip_address);

        save_tempbans_to_file(main_app.get_temp_bans_file_path(), temp_banned_players);

        return {true, pd};
    }

    return {true, player{}};
}

bool add_permanently_banned_player_name(player &pd, vector<player> &banned_players_names_vector,
                                        unordered_map<string, player> &banned_players_names_map)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (len(pd.player_name) == 0 || banned_players_names_map.contains(pd.player_name))
        return false;

    banned_players_names_map.emplace(pd.player_name, pd);
    banned_players_names_vector.push_back(std::move(pd));

    save_banned_ip_entries_to_file(main_app.get_banned_names_file_path(), banned_players_names_vector);

    return true;
}

bool add_permanently_banned_ip_address(player &pd, vector<player> &banned_players_data,
                                       unordered_map<string, player> &ip_to_banned_player_data)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    unsigned long guid_number{};
    if (!check_ip_address_validity(pd.ip_address, guid_number) || ip_to_banned_player_data.contains(pd.ip_address))
        return false;

    ip_to_banned_player_data.emplace(pd.ip_address, pd);
    banned_players_data.push_back(std::move(pd));

    save_banned_ip_entries_to_file(main_app.get_ip_bans_file_path(), banned_players_data);

    return true;
}

bool add_permanently_banned_ip_address_range(player &pd, std::vector<player> &banned_ip_address_ranges_vector,
                                             std::unordered_map<std::string, player> &banned_ip_address_ranges_map)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const string banned_ip_range{pd.ip_address};
    if (!banned_ip_range.ends_with(".*.*") && !banned_ip_range.ends_with(".*"))
        return false;
    if (banned_ip_address_ranges_map.contains(banned_ip_range))
        return false;

    banned_ip_address_ranges_map.emplace(banned_ip_range, pd);
    banned_ip_address_ranges_vector.push_back(std::move(pd));

    save_banned_ip_address_range_entries_to_file(main_app.get_ip_range_bans_file_path(),
                                                 banned_ip_address_ranges_vector);

    return true;
}

bool remove_permanently_banned_ip_address_range(player &pd, std::vector<player> &banned_ip_address_ranges_vector,
                                                std::unordered_map<std::string, player> &banned_ip_address_ranges_map)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const string banned_ip_range{pd.ip_address};
    if (!banned_ip_range.ends_with(".*.*") && !banned_ip_range.ends_with(".*"))
        return false;
    if (!banned_ip_address_ranges_map.contains(banned_ip_range))
        return false;

    banned_ip_address_ranges_vector.erase(remove_if(std::begin(banned_ip_address_ranges_vector),
                                                    std::end(banned_ip_address_ranges_vector),
                                                    [&pd](const player &p) { return pd.ip_address == p.ip_address; }),
                                          std::end(banned_ip_address_ranges_vector));

    pd = std::move(banned_ip_address_ranges_map.at(pd.ip_address));
    banned_ip_address_ranges_map.erase(pd.ip_address);

    save_banned_ip_address_range_entries_to_file(main_app.get_ip_range_bans_file_path(),
                                                 banned_ip_address_ranges_vector);

    return true;
}

bool remove_permanently_banned_player_name(player &pd, std::vector<player> &banned_names_vector,
                                           std::unordered_map<std::string, player> &banned_names_map)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (len(pd.player_name) == 0 || !banned_names_map.contains(pd.player_name))
        return false;

    banned_names_vector.erase(remove_if(std::begin(banned_names_vector), std::end(banned_names_vector),
                                        [&pd](const player &p) { return strcmp(pd.player_name, p.player_name) == 0; }),
                              std::end(banned_names_vector));

    pd = std::move(banned_names_map.at(pd.player_name));
    banned_names_map.erase(pd.player_name);

    save_banned_ip_entries_to_file(main_app.get_banned_names_file_path(), banned_names_vector);

    return true;
}

bool add_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (banned_cities.contains(city))
        return false;

    banned_cities.emplace(city);

    save_banned_cities_to_file(main_app.get_banned_cities_file_path(), banned_cities);

    return true;
}

bool remove_permanently_banned_city(const std::string &city, std::set<std::string> &banned_cities)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (!banned_cities.contains(city))
        return false;

    banned_cities.erase(city);

    save_banned_cities_to_file(main_app.get_banned_cities_file_path(), banned_cities);

    return true;
}

bool add_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (banned_countries.contains(country))
        return false;

    banned_countries.emplace(country);

    save_banned_cities_to_file(main_app.get_banned_countries_file_path(), banned_countries);

    return true;
}

bool remove_permanently_banned_country(const std::string &country, std::set<std::string> &banned_countries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (!banned_countries.contains(country))
        return false;

    banned_countries.erase(country);

    save_banned_cities_to_file(main_app.get_banned_countries_file_path(), banned_countries);

    return true;
}

std::pair<bool, player> remove_permanently_banned_ip_address(std::string &ip_address, std::string &message,
                                                             const bool is_report_public_message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    int number{};
    const size_t no{is_valid_decimal_whole_number(ip_address, number) ? static_cast<size_t>(number - 1)
                                                                      : std::string::npos};

    auto &banned_players = main_app.get_current_game_server().get_banned_ip_addresses_vector();

    if (main_app.get_current_game_server().get_banned_ip_addresses_map().find(ip_address) ==
            cend(main_app.get_current_game_server().get_banned_ip_addresses_map()) &&
        no >= banned_players.size())
    {
        return {false, player{}};
    }

    if (no < banned_players.size())
    {
        ip_address = banned_players[no].ip_address;
    }

    const auto found_iter = find_if(std::begin(banned_players), std::end(banned_players),
                                    [&ip_address](const player &p) { return ip_address == p.ip_address; });

    if (found_iter != std::end(banned_players))
    {
        if (is_report_public_message)
        {
            const string buffer{format("^7Admin ({}^7) has manually removed ^1permanently banned IP address "
                                       "^7for player {}. ^7Reason: ^1{}",
                                       main_app.get_username().c_str(), found_iter->player_name,
                                       remove_disallowed_character_in_string(found_iter->reason))};
            message.assign(buffer);
            /*const string inform_msg{ format("{}\\{}", main_app.get_username(),
      message) }; main_app.add_message_to_queue(message_t("inform-message",
      inform_msg, true));*/
            // rcon_say(message);
        }

        const player pd{main_app.get_current_game_server().get_banned_ip_addresses_map().at(found_iter->ip_address)};

        banned_players.erase(remove_if(std::begin(banned_players), std::end(banned_players),
                                       [&ip_address](const player &p) { return ip_address == p.ip_address; }),
                             std::end(banned_players));

        main_app.get_current_game_server().get_banned_ip_addresses_map().erase(pd.ip_address);

        save_banned_ip_entries_to_file(main_app.get_ip_bans_file_path(), banned_players);

        return {true, pd};
    }

    return {true, player{}};
}

bool is_valid_decimal_whole_number(const std::string &str, int &number)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (str.empty())
        return false;

    string ex_msg{format(R"(^1Exception ^3thrown from ^1bool is_valid_decimal_whole_number("{}", "{}"))", str, number)};
    stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

    if (str.length() == 1 && !isdigit(str[0]))
        return false;

    const bool is_negative{'-' == str[0]};
    size_t index{is_negative || '+' == str[0] ? 1U : 0U};
    number = 0;
    for (; index < str.length(); ++index)
    {
        if (!isdigit(str[index]))
            return false;
        number *= 10;
        number += static_cast<int>(str[index] - '0');
    }
    if (is_negative)
        number = -number;

    return true;
}

size_t get_number_of_characters_without_color_codes(const char *text)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t printed_chars_count{};
    const size_t text_len = len(text);
    const char *last = text + text_len;
    for (; *text; ++text)
    {
        if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') &&
            (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3))
        {
            text += 3;
            printed_chars_count += 2;
        }
        else if (text + 2 <= last && *text == '^' && (*(text + 1) >= '0' && *(text + 1) <= '9'))
        {
            ++text;
        }
        else
        {
            ++printed_chars_count;
        }
    }

    return printed_chars_count;
}

bool get_user_input()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static constexpr size_t buffer_size{2048};
    static char user_command[buffer_size];
    GetWindowTextA(app_handles.hwnd_e_user_input, user_command, buffer_size);
    string user_input{user_command};
    process_user_input(user_input);
    SetWindowTextA(app_handles.hwnd_e_user_input, "");
    // ReleaseCapture();
    // SetFocus(app_handles.hwnd_main_window);
    // SetForegroundWindow(app_handles.hwnd_main_window);
    return should_program_terminate(user_input);
}

void print_help_information(const std::vector<std::string> &input_parts)
{
    if (input_parts.size() >= 2 &&
        (input_parts[0] == "!list" || input_parts[0] == "list" || input_parts[0] == "!l" || input_parts[0] == "!help" ||
         input_parts[0] == "help" || input_parts[0] == "!h") &&
        !input_parts[1].empty())
    {
        const string command_name{input_parts[1][0] != '!' ? "!"s + input_parts[1] : input_parts[1]};
        if (str_starts_with(command_name, "!user", true))
        {
            const string help_message{
                R"(
 ^5You can use the following user commands:
 ^1!cls ^7-> ^3clears the screen.
 ^1!colors ^7-> ^5changes colors of various displayed table entries and game information.
 ^1!c [IP:PORT] ^7-> ^3launches your Call of Duty game and connect to currently configured game server
 or optionally specified game server address ^1[IP:PORT]^3.
 ^1!cp [IP:PORT] ^7-> ^5launches your Call of Duty game and connect to currently configured game server
 or optionally specified game server address ^1[IP:PORT] ^5using a private slot.
 ^1!warn player_pid optional_reason ^7-> ^3warns the player whose pid number is equal to specified player_pid.
  A player who's been warned 2 times gets automatically kicked off the server.
 ^1!w player_pid optional_reason ^7-> ^5warns the player whose pid number is equal to specified player_pid.
  A player who's been warned 2 times gets automatically kicked off the server.
 ^1!kick player_pid optional_reason ^7-> ^3kicks the player whose pid number is equal to specified player_pid.
 ^1!k player_pid optional_reason ^7-> ^5kicks the player whose pid number is equal to specified player_pid.
 ^1!tempban player_pid time_duration reason ^7-> ^3temporarily bans (for time_duration hours, default 24 hours) IP address
    of player whose pid = ^112.
 ^1!tb player_pid time_duration reason ^7-> ^5temporarily bans (for time_duration hours, default 24 hours) IP address
    of player whose pid = ^112.
 ^1!ban player_pid optional_reason ^7-> ^3bans the player whose ^1pid number ^3is equal to specified ^1player_pid.
 ^1!b player_pid optional_reason ^7-> ^5bans the player whose ^1pid number ^5is equal to specified ^1player_pid.
 ^1!gb player_pid optional_reason ^7-> ^3permanently bans player's IP address whose ^1pid number ^3is equal to specified ^1player_pid.
 ^1!globalban player_pid optional_reason ^7-> ^5permanently bans player's IP address whose ^1pid number ^5is equal to specified ^1player_pid.
 ^1!status ^7-> ^5retrieves current game state (^1online players' data^5) 
   ^5from the currently viewed game server.
 ^1!t public message ^7-> ^3sends ^1public message ^3to all logged in ^1admins.
 ^1!y username private message ^7-> ^5sends ^1private message ^5to ^1admin ^5whose username is ^1username.
 ^1!sort player_data asc|desc ^7-> ^3sorts players' data (for example: ^1!sort pid asc^3, ^1!sort pid desc^3, ^1!sort score asc^3,
   ^1!sort score desc^3, ^1!sort ping asc^3, ^1!sort ping desc^3, ^1!sort name asc^3, ^1!sort name desc^3, ^1!sort ip asc^3, ^1!sort ip desc^3,
   ^1!sort geo asc^3, ^1!sort geo desc
 ^1!list user|rcon ^7-> ^5shows a list of available ^1user ^5or ^1rcon commands ^5that are available to Tiny^6Rcon ^5users.
 ^1!ranges ^7-> ^3displays permanently banned ^1IP address ranges.
 ^1!bans ^7-> ^5displays permanently banned ^1IP addresses.
 ^1!tempbans ^7-> ^3displays temporarily banned ^1IP addresses.
 ^1!ub valid_ip_address optional_reason ^7-> ^5removes temporarily and|or permanently banned IP address ^1valid_ip_address.
 ^1!unban valid_ip_address optional_reason ^7-> ^3removes temporarily and|or permanently banned IP address ^1valid_ip_address.
 ^1!m map_name game_type ^7-> ^5loads map ^1map_name ^5in specified game type ^1game_type^5, example: ^1!m mp_toujane hq
 ^1!map map_name game_type ^7-> ^3loads map ^1map_name ^3in specified game type ^1game_type^3, example: ^1!map mp_burgundy ctf
 ^1!maps ^7-> ^5displays all available ^1playable maps.
 ^1!config rcon abc123 ^7-> ^3changes currently used ^1rcon_password ^3to ^1abc123
 ^1!config private abc123 ^7-> ^5changes currently used ^1sv_privatepassword ^5to ^1abc123
 ^1!config address 123.101.102.103:28960 ^7-> ^3changes currently used server ^1IP:port ^3to ^1123.101.102.103:28960
 ^1!config name Administrator ^7-> ^5changes currently used ^1username ^5to ^1Administrator
 ^1!messages on|off ^7-> ^3Turns on^5|^3off messages for temporarily and permanently banned players.
 ^1!egb ^7-> ^5Enables Tiny^6Rcon's ^5city ban feature.
 ^1!dgb ^7-> ^3Disables ^5Tiny^6Rcon's ^3city ban feature.
 ^1!bancity city ^7-> ^5Adds player's ^1city ^5to list of ^1banned cities^5.
 ^1!unbancity city ^7-> ^3Removes player's ^1city ^3from list of ^1banned cities^3.
 ^1!banned cities ^7-> ^5Displays list of currently ^1banned cities^5.
 ^1!ecb ^7-> ^3Enables ^5Tiny^6Rcon's ^3country ban feature.
 ^1!dcb ^7-> ^5Disables Tiny^6Rcon's ^5country ban feature.
 ^1!bancountry country ^7-> ^3Adds player's ^1country ^3to list of ^1banned countries.
 ^1!unbancountry country ^7-> ^5Removes player's ^1country ^5from list of ^1banned countries.
 ^1!banned countries ^7-> ^3Displays list of currently ^1banned countries.
 ^1!banrange 12| 111.122.133.* optional_reason ^7-> ^5bans ^1IP address range ^5of player whose ^1pid ^3is equal to ^112.
 ^1!unbanrange 111.122.133.*  optional_reason ^7-> ^3removes previously banned IP address range ^1111.122.133.*.
 ^1!admins ^7-> ^5Displays information about ^1admins.
 ^1!protectip pid|ip_address optional_reason ^7-> ^3protects player's ^1IP address ^4whose ^1pid number ^3or
   ^1IP address ^3is equal to specified ^1pid ^3or ^1ip_address^3, respectively.
 ^1!unprotectip pid|ip_address optional_reason ^7-> ^5removes protected ^1IP address ^5of player whose ^1pid number ^5or
   ^1IP address ^5is equal to specified ^1pid ^5or ^1ip_address^5, respectively.
 ^1!protectiprange pid|ip_address_range optional_reason ^7-> ^3protects player's ^1IP address range ^3whose ^1pid number ^3or
   ^1IP address range ^3is equal to specified ^1pid ^3or ^1ip_address_range^3, respectively.
 ^1!unprotectiprange pid|ip_address_range optional_reason ^7-> ^5removes protected ^1IP address range ^5of player whose ^1pid number
   ^5or ^1IP address range ^5is equal to specified ^1pid5 ^5or ^1ip_address_range^5, respectively.
 ^1!protectcity pid|city_name ^7-> ^3protects player's ^1city ^3whose ^1pid number ^3or ^1city name ^3is equal to
  specified ^1pid ^3or ^1city_name^3, respectively.
 ^1!unprotectcity pid|city_name ^7-> ^5removes protected city of player whose ^1pid number ^5or ^1city name ^5is equal
  to specified ^1pid ^5or ^1city_name^5, respectively.
 ^1!protectcountry pid|country_name ^7-> ^3protects player's country whose ^1pid number ^3or ^1country name ^3is equal to
   specified ^1pid ^3or ^1valid_ip_address^3, respectively.
 ^1!unprotectcountry pid|country_name ^7-> ^5removes protected ^1IP address ^5of player whose ^1pid number ^5or ^1country name
   ^5is equal to specified ^1pid ^5or ^1country_name^5, respectively.
 ^1!banname pid|player_name optional_reason ^7-> ^3bans ^1player name ^3of online player whose ^1pid number ^3is equal to specified ^1pid 
   ^3or bans specified player name ^1player_name.
 ^1!unbanname player_name optional_reason ^7-> ^5removes previously banned ^1player_name.
 ^1!names ^7-> ^3displays all currently banned player names.
 ^1!stats ^7-> ^5displays up-to-date ^5Tiny^6Rcon ^1admins' ^5related stats data.
 ^1!report 12 optional_reason ^7-> ^3reports player whose ^1pid ^3is equal to ^112 ^3with reason ^1optional_reason.
 ^1!unreport 123.123.123.123 optional_reason ^7-> ^5removes previously reported player whose ^1IP address is ^1123.123.123.123.
 ^1!reports 25 ^7-> ^3displays the last ^125 ^3reported players.
 ^1!tp  [optional_number] ^7-> ^5displays top ^125 ^5or ^1optional_number ^5players' stats data.
 ^1!tpd [optional_number] ^7-> ^3displays top ^125 ^3or ^1optional_number ^3players' stats data for current day.
 ^1!tpm [optional_number] ^7-> ^5displays top ^125 ^5or ^1optional_number ^5players' stats data for current month.
 ^1!tpy [optional_number] ^7-> ^3displays top ^125 ^3or ^1optional_number ^3players' stats data for current year.
 ^1!s player_name ^7-> ^5displays player stats data for player whose ^1partially ^5or ^1fully specified name ^5is ^1player_name. 
   ^5You can omit ^1color codes ^5in partially or fully specified ^1player_name^5.
 ^1!addip 123.123.123.123 wh ^7-> ^3bans custom IP address ^1123.123.123.123 ^3with reason ^1wh.
 ^1!addip 123.123.123.123 n:Pro100Nik^2123 r:wh ^7-> ^5bans custom IP address ^1123.123.123.123
   ^5(of player named ^7Pro100Nik^2123^5) with specified reason: ^1wh
 ^1!rc custom_rcon_command [optional_parameters] ^7-> ^3sends ^1custom_rcon_command [optional_parameters] ^3to game server.
   ^7Examples: ^1!rc map_restart ^7| ^1!rc fast_restart ^7| ^1!rc sv_iwdnames)"};

            print_colored_text(app_handles.hwnd_re_messages_data, help_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", is_append_message_to_richedit_control::yes,
                               is_log_message::yes, is_log_datetime::no);
        }
        else if (str_starts_with(command_name, "!rcon", true))
        {
            const string help_message{
                R"(
 ^5You can use the following rcon commands:
 ^1status ^7-> ^3Retrieves current information about online players from the currently viewed game server
   (pid, score, ping, name, guid, ip, qport, rate
 ^1clientkick player_pid optional_reason ^7-> ^5Kicks player whose pid number is equal to specified player_pid.
 ^1kick name optional_reason ^7-> ^3Kicks player whose name is equal to specified name.
 ^1onlykick name_no_color_codes optional_reason ^7-> ^5Kicks player whose name is equal to specified name_no_color_codes.
 ^1banclient player_pid optional_reason ^7-> ^3Bans player whose pid number is equal to specified player_pid.
 ^1banuser name optional_reason ^7-> ^5Bans player whose name is equal to specified name.
 ^1tempbanclient player_pid optional_reason ^7-> ^3Temporarily bans player whose pid number is equal to specified player_pid.
 ^1tempbanuser name optional_reason ^7-> ^5Temporarily bans player whose name is equal to specified name.
 ^1serverinfo ^7-> ^3Retrieves currently active server settings.
 ^1map_rotate ^7-> ^5Loads next map on the server.
 ^1map_restart ^7-> ^3Reloads currently played map on the server.
 ^1map map_name ^7-> ^5Immediately loads map 'map_name' on the server.
 ^1fast_restart ^7-> ^3Quickly restarts currently played map on the server.
 ^1getinfo ^7-> ^5Retrieves basic server information from the server.
 ^1mapname ^7-> ^3Retrieves and displays currently played map's rcon name on the console.
 ^1g_gametype ^7-> ^5Retrieves and displays currently played match's gametype on the console.
 ^1sv_maprotation ^7-> ^3Retrieves and displays server's original map rotation setting.
 ^1sv_maprotationcurrent ^7-> ^5Retrieves and displays server's current map rotation setting.
 ^1say "public message" ^7-> ^3Displays ^1"public message" ^3to all players on the currently viewed game server. 
    The sent public message is seen by all players.
 ^1tell player_pid "private message" ^7-> ^5Sends ^1"private message" ^5to player whose pid number is equal to ^1player_pid.
)"};
            print_colored_text(app_handles.hwnd_re_messages_data, help_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", is_append_message_to_richedit_control::yes,
                               is_log_message::yes, is_log_datetime::no);
        }
        else if (user_commands_help.contains(command_name))
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               format("^5Correct usage for ^3user command ^1{}:\n", command_name).c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(command_name).c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
            print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                               is_log_message::yes, is_log_datetime::no);
        }
        else if (rcon_commands_help.contains(command_name))
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               format("^5Correct usage for ^3rcon command ^1{}:\n", command_name).c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            print_colored_text(app_handles.hwnd_re_messages_data, rcon_commands_help.at(command_name).c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
            print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                               is_log_message::yes, is_log_datetime::no);
        }
    }
    else
    {
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^5********************************************************************"
                           "**"
                           "********************\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "Type '^2!list rcon^5' in the console to get more information about "
                           "the "
                           "available rcon commands\nthat you can run in the console window.\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "Type '^2!list user^5' in the console to get more information about "
                           "the "
                           "available user commands\nthat you can run in the console window.\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "**********************************************************************"
                           "********************\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
    }
}

string prepare_current_match_information()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string current_match_info = main_app.get_current_match_info();

    size_t start = current_match_info.find("{MAP_FULL_NAME}");
    if (string::npos != start)
    {
        current_match_info.replace(start, len("{MAP_FULL_NAME}"),
                                   main_app.get_full_map_name_color() +
                                       get_full_map_name(main_app.get_current_game_server().get_current_map(),
                                                         convert_game_name_to_game_name_t(
                                                             main_app.get_current_game_server().get_game_name())));
    }

    start = current_match_info.find("{MAP_RCON_NAME}");
    if (string::npos != start)
    {
        current_match_info.replace(start, len("{MAP_RCON_NAME}"),
                                   main_app.get_rcon_map_name_color() +
                                       main_app.get_current_game_server().get_current_map());
    }

    start = current_match_info.find("{GAMETYPE_FULL_NAME}");
    if (string::npos != start)
    {
        current_match_info.replace(
            start, len("{GAMETYPE_FULL_NAME}"),
            main_app.get_full_gametype_name_color() +
                get_full_gametype_name(main_app.get_current_game_server().get_current_game_type()));
    }

    start = current_match_info.find("{GAMETYPE_RCON_NAME}");
    if (string::npos != start)
    {
        current_match_info.replace(start, len("{GAMETYPE_RCON_NAME}"),
                                   main_app.get_rcon_gametype_name_color() +
                                       main_app.get_current_game_server().get_current_game_type());
    }

    const int online_players_count = main_app.get_current_game_server().get_number_of_online_players();
    const int offline_players_count = main_app.get_current_game_server().get_number_of_offline_players();

    start = current_match_info.find("{ONLINE_PLAYERS_COUNT}");
    if (string::npos != start)
    {
        current_match_info.replace(start, stl::helper::len("{ONLINE_PLAYERS_COUNT}"),
                                   main_app.get_online_players_count_color() + to_string(online_players_count));
    }

    start = current_match_info.find("{OFFLINE_PLAYERS_COUNT}");
    if (string::npos != start)
    {
        current_match_info.replace(start, stl::helper::len("{OFFLINE_PLAYERS_COUNT}"),
                                   main_app.get_offline_players_count_color() + to_string(offline_players_count));
    }

    return current_match_info;
}

void display_online_admins_information()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ostringstream oss;

    oss << "^2Online admins: ";

    unordered_set<string> already_seen_ip_addresses;

    if (!main_app.get_users().empty())
    {

        for (auto &u : main_app.get_users())
        {
            if (u->is_online && !u->ip_address.empty() && !already_seen_ip_addresses.contains(u->ip_address))
            {
                already_seen_ip_addresses.emplace(u->ip_address);
                oss << format("^7{} ^3(^7{}^3), ", u->user_name, u->player_name);
            }
        }
    }

    online_admins_information = oss.str();
    if (' ' == online_admins_information.back())
        online_admins_information.pop_back();
    if (',' == online_admins_information.back())
        online_admins_information.pop_back();

    SetWindowTextA(app_handles.hwnd_online_admins_information, "");
    print_colored_text(app_handles.hwnd_online_admins_information, online_admins_information.c_str(),
                       is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
    SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);
}

bool check_if_user_provided_argument_is_valid_for_specified_command(const char *cmd, const string &arg)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    int number{};
    if ((str_compare_i(cmd, "!r") == 0) || (str_compare_i(cmd, "!report") == 0) || (str_compare_i(cmd, "!w") == 0) ||
        (str_compare_i(cmd, "!warn") == 0) || (str_compare_i(cmd, "!k") == 0) || (str_compare_i(cmd, "!kick") == 0) ||
        (str_compare_i(cmd, "clientkick") == 0) || (str_compare_i(cmd, "tempbanclient") == 0) ||
        (str_compare_i(cmd, "!b") == 0) || (str_compare_i(cmd, "!ban") == 0) || (str_compare_i(cmd, "banclient") == 0))
    {
        return is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg);
    }

    if ((str_compare_i(cmd, "!tb") == 0) || (str_compare_i(cmd, "!tempban") == 0))
    {
        unsigned long ip_key{};
        return (is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg)) ||
               check_ip_address_validity(arg, ip_key);
    }

    if ((str_compare_i(cmd, "!gb") == 0) || (str_compare_i(cmd, "!globalban") == 0) ||
        (str_compare_i(cmd, "!banip") == 0) || (str_compare_i(cmd, "!addip") == 0) ||
        (str_compare_i(cmd, "!protectip") == 0) || (str_compare_i(cmd, "!unprotectip") == 0))
    {
        unsigned long ip_key{};
        return (is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg)) ||
               check_ip_address_validity(arg, ip_key);
    }

    if ((str_compare_i(cmd, "!br") == 0) || (str_compare_i(cmd, "!banrange") == 0) ||
        (str_compare_i(cmd, "!protectiprange") == 0) || (str_compare_i(cmd, "!unprotectiprange") == 0))
    {
        return (is_valid_decimal_whole_number(arg, number) && check_if_user_provided_pid_is_valid(arg)) ||
               check_ip_address_range_validity(arg);
    }

    if ((str_compare_i(cmd, "!ubr") == 0) || (str_compare_i(cmd, "!unbanrange") == 0))
    {
        return check_ip_address_range_validity(arg);
    }

    if ((str_compare_i(cmd, "!ur") == 0) || (str_compare_i(cmd, "!unreport") == 0))
    {
        unsigned long ip_key{};
        return ((is_valid_decimal_whole_number(arg, number) && number > 0 &&
                 number <= static_cast<int>(main_app.get_reported_players().size())) ||
                check_ip_address_validity(arg, ip_key));
    }

    if (str_compare_i(cmd, "!geoinfo") == 0)
    {
        return arg == "on" || arg == "off";
    }

    return true;
}

bool check_if_user_provided_pid_is_valid(const string &pid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (int number{-1}; is_valid_decimal_whole_number(pid, number))
    {
        const auto &players_data = main_app.get_current_game_server().get_players_data();
        for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
        {
            if (players_data[i].pid == number)
                return true;
        }
    }
    return false;
}

void remove_all_color_codes(char *msg)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const size_t msg_len{stl::helper::len(msg)};
    size_t start{};
    while (start < msg_len)
    {
        start = stl::helper::str_index_of(msg, '^', start);
        if (string::npos == start)
            break;

        if (start + 4 <= msg_len && msg[start + 1] == '^' && (msg[start + 2] >= '0' && msg[start + 2] <= '9') &&
            (msg[start + 3] >= '0' && msg[start + 3] <= '9') /*&& msg[start + 2] == msg[start + 3]*/)
        {
            stl::helper::str_erase_n_chars(msg, start, 4);
        }
        else if (start + 2 <= msg_len && (msg[start + 1] >= '0' && msg[start + 1] <= '9'))
        {
            stl::helper::str_erase_n_chars(msg, start, 2);
        }
        else
        {
            ++start;
        }
    }
}

void remove_all_color_codes(wchar_t *msg)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const size_t msg_len{stl::helper::len(msg)};
    size_t start{};
    while (start < msg_len)
    {
        start = stl::helper::str_index_of(msg, L'^', start);
        if (wstring::npos == start)
            break;

        if (start + 4 <= msg_len && msg[start + 1] == L'^' && (msg[start + 2] >= L'0' && msg[start + 2] <= L'9') &&
            (msg[start + 3] >= L'0' && msg[start + 3] <= L'9') /*&& msg[start + 2] == msg[start + 3]*/)
        {
            stl::helper::str_erase_n_chars(msg, start, 4);
        }
        else if (start + 2 <= msg_len && (msg[start + 1] >= L'0' && msg[start + 1] <= L'9'))
        {
            stl::helper::str_erase_n_chars(msg, start, 2);
        }
        else
        {
            ++start;
        }
    }
}

void remove_all_color_codes(std::string &msg)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t start{};
    while (start < msg.length())
    {

        start = msg.find('^', start);
        if (string::npos == start)
            break;

        if (start + 4 <= msg.length() && msg[start + 1] == '^' && (msg[start + 2] >= '0' && msg[start + 2] <= '9') &&
            (msg[start + 3] >= '0' && msg[start + 3] <= '9') /*&& msg[start + 2] == msg[start + 3]*/)
        {
            msg.erase(start, 4);
        }
        else if (start + 2 <= msg.length() && (msg[start + 1] >= '0' && msg[start + 1] <= '9'))
        {
            msg.erase(start, 2);
        }
        else
        {
            ++start;
        }
    }
}

void remove_all_color_codes(std::wstring &msg)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t start{};
    while (start < msg.length())
    {

        start = msg.find(L'^', start);
        if (wstring::npos == start)
            break;

        if (start + 4 <= msg.length() && msg[start + 1] == L'^' && (msg[start + 2] >= L'0' && msg[start + 2] <= L'9') &&
            (msg[start + 3] >= L'0' && msg[start + 3] <= L'9') /*&& msg[start + 2] == msg[start + 3]*/)
        {
            msg.erase(start, 4);
        }
        else if (start + 2 <= msg.length() && (msg[start + 1] >= L'0' && msg[start + 1] <= L'9'))
        {
            msg.erase(start, 2);
        }
        else
        {
            ++start;
        }
    }
}

std::string remove_ip_addresses(const std::string &src, const char *replacement)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    std::regex ip_address_regex_for_players{R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))"};
    std::string output;
    regex_replace(back_inserter(output), cbegin(src), cend(src), ip_address_regex_for_players, replacement);
    const string needle{"^"s + replacement};
    for (size_t start_pos{}, next_pos; (next_pos = output.find(needle, start_pos)) != string::npos;)
    {
        output.erase(next_pos, 1);
        start_pos = next_pos + needle.length() - 1;
    }
    return output;
}

pair<player, bool> get_online_player_for_specified_pid(const int pid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const auto &players_data = main_app.get_current_game_server().get_players_data();
    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
    {
        if (pid == players_data[i].pid)
            return make_pair(players_data[i], true);
    }
    return make_pair(player{}, false);
}

void check_for_warned_players()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    unordered_map<int, player> &warned_players = main_app.get_current_game_server().get_warned_players_data();
    unordered_map<int, player> refreshed_warned_players_data;
    refreshed_warned_players_data.reserve(warned_players.size());
    const auto &players_data = main_app.get_current_game_server().get_players_data();
    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
    {
        const auto &online_player = players_data[i];
        if (warned_players.contains(online_player.pid))
        {

            if (warned_players.at(online_player.pid).ip_address == online_player.ip_address)
            {
                refreshed_warned_players_data[online_player.pid] = std::move(warned_players.at(online_player.pid));
            }
            else
            {
                warned_players.erase(online_player.pid);
            }
        }
    }
    main_app.get_current_game_server().set_warned_players_data(std::move(refreshed_warned_players_data));
}

void check_for_temp_banned_ip_addresses()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const time_t now_in_seconds = get_current_time_stamp();

    auto &temp_banned_players = main_app.get_current_game_server().get_temp_banned_ip_addresses_vector();
    for (auto &tb : temp_banned_players)
    {
        const time_t ban_expires_time = tb.banned_start_time + (tb.ban_duration_in_hours * 3600);
        if (ban_expires_time <= now_in_seconds)
        {
            string message{main_app.get_automatic_remove_temp_ban_msg()};
            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = tb.player_name;
            main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] =
                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", tb.banned_start_time);
            main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] =
                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", ban_expires_time);
            main_app.get_tinyrcon_dict()["{REASON}"] = remove_disallowed_character_in_string(tb.reason);
            main_app.get_tinyrcon_dict()["{BANNED_BY}"] = remove_disallowed_character_in_string(tb.banned_by_user_name);
            build_tiny_rcon_message(message);
            auto [status, player]{remove_temp_banned_ip_address(tb.ip_address, message, true, true)};
            replace_br_with_new_line(message);
            if (status)
            {
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "remove-tempban",
                    format(R"({}\{}\{}\{}\{}\{}\{})", player.ip_address, player.player_name, player.banned_date_time,
                           player.banned_start_time, player.ban_duration_in_hours, player.reason,
                           player.banned_by_user_name),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                const string private_msg{format(
                    "^2Player ^7{}'s ^1temporary ban ^2has been automatically "
                    "removed.\n^7Info on player ^7{}^7: ^3IP: ^1{} ^5| ^3geoinfo: ^1{}, "
                    "{} ^5| ^3Date of tempban: ^1{}\n ^3Tempban duration: ^1{} hours "
                    "^5|^5Reason: ^1{} ^5| ^3Banned by: ^1{}\n",
                    player.player_name, player.player_name, player.ip_address, player.country_name, player.city,
                    player.banned_date_time, player.ban_duration_in_hours, player.reason, player.banned_by_user_name)};
                const string inform_msg{format("{}\\{}\\{}", main_app.get_username(), message, private_msg)};
                // main_app.add_message_to_queue(message_t("inform-message", inform_msg,
                // true));
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "inform-message", inform_msg, true, main_app.get_tiny_rcon_server_ip_address(),
                    main_app.get_tiny_rcon_server_port(), false);
            }
        }
    }

    const auto &temp_banned_ip_address = main_app.get_current_game_server().get_temp_banned_ip_addresses_map();

    if (!temp_banned_ip_address.empty())
    {

        auto &players_data = main_app.get_current_game_server().get_players_data();
        for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
        {
            auto &online_player = players_data[i];

            if (temp_banned_ip_address.contains(online_player.ip_address))
            {
                player pd{};
                if (main_app.get_current_game_server().get_temp_banned_player_data_for_ip_address(
                        online_player.ip_address, &pd))
                {
                    const time_t ban_expires_time = pd.banned_start_time + (pd.ban_duration_in_hours * 3600);
                    if (ban_expires_time > now_in_seconds)
                    {
                        string message{main_app.get_automatic_kick_temp_ban_msg()};
                        if (main_app.get_is_disable_automatic_kick_messages())
                        {
                            message.clear();
                        }
                        else
                        {
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
                            main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] =
                                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time);
                            main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] =
                                get_time_interval_info_string_for_seconds(ban_expires_time - now_in_seconds);
                            main_app.get_tinyrcon_dict()["{REASON}"] = remove_disallowed_character_in_string(pd.reason);
                            main_app.get_tinyrcon_dict()["{BANNED_BY}"] =
                                remove_disallowed_character_in_string(pd.banned_by_user_name);
                            build_tiny_rcon_message(message);
                            replace_br_with_new_line(message);
                            const string private_msg{
                                format("^3Automatic kick for ^1temporarily banned ^3player ^7{}\n "
                                       "^5[^3IP: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date of tempban: "
                                       "^1{}\n ^3Tempban duration: ^1{} hours ^5|^5Reason: ^1{} ^5| "
                                       "^3Banned by: ^1{}^5]\n",
                                       pd.player_name, pd.ip_address, pd.country_name, pd.city, pd.banned_date_time,
                                       pd.ban_duration_in_hours, pd.reason, pd.banned_by_user_name)};
                            print_colored_text(app_handles.hwnd_re_messages_data, private_msg.c_str());
                            kick_player(online_player.pid, message);
                        }
                    }
                }
            }
        }
    }
}

void check_for_banned_ip_addresses()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const bool is_enable_automatic_connection_flood_ip_ban{main_app.get_is_enable_automatic_connection_flood_ip_ban()};
    const auto &banned_ip_addresses = main_app.get_current_game_server().get_banned_ip_addresses_map();
    const auto &protected_ip_address = main_app.get_current_game_server().get_protected_ip_addresses();
    const auto &banned_ip_address_ranges = main_app.get_current_game_server().get_banned_ip_address_ranges_map();
    const auto &protected_ip_address_ranges = main_app.get_current_game_server().get_protected_ip_address_ranges();
    const auto &banned_countries = main_app.get_current_game_server().get_banned_countries_set();
    const auto &protected_countries = main_app.get_current_game_server().get_protected_countries();
    const auto &banned_cities = main_app.get_current_game_server().get_banned_cities_set();
    const auto &protected_cities = main_app.get_current_game_server().get_protected_cities();
    auto &banned_player_names = main_app.get_current_game_server().get_banned_names_map();
    auto &ip_address_frequency = main_app.get_ip_address_frequency();
    auto &players_data = main_app.get_current_game_server().get_players_data();
    string player_protected_message;
    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
    {
        auto &online_player = players_data[i];
        const string narrow_ip_address_range{
            get_narrow_ip_address_range_for_specified_ip_address(online_player.ip_address)};
        const string wide_ip_address_range{
            get_wide_ip_address_range_for_specified_ip_address(online_player.ip_address)};
        const bool is_ip_protected{protected_ip_address.contains(online_player.ip_address)};
        const bool is_ip_range_protected{protected_ip_address_ranges.contains(narrow_ip_address_range) ||
                                         protected_ip_address_ranges.contains(wide_ip_address_range)};
        const bool is_city_protected{protected_cities.contains(online_player.city)};
        const bool is_country_protected{protected_countries.contains(online_player.country_name)};

        if (is_ip_protected || is_ip_range_protected || is_city_protected || is_country_protected)
            continue;

        if (is_enable_automatic_connection_flood_ip_ban &&
            ip_address_frequency[online_player.ip_address] >=
                main_app.get_current_game_server().get_minimum_number_of_connections_from_same_ip_for_automatic_ban())
        {
            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(online_player.pid);
            string reason{"^1DoS attack"};
            specify_reason_for_player_pid(online_player.pid, reason);
            string public_message{
                "^1{ADMINNAME}: ^5Tiny^6Rcon ^7has successfully automatically banned ^1IP address: "s +
                online_player.ip_address +
                " ^7| ^5Player name: ^7{PLAYERNAME} ^7| ^5Reason: ^1{REASON} ^7| ^5Date of ban: ^1{BAN_DATE}\n"s};
            main_app.get_tinyrcon_dict()["{BAN_DATE}"] = get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}");
            main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);

            build_tiny_rcon_message(public_message);
            // rcon_say(public_message, true);
            const string private_message{
                "^5Tiny^6Rcon ^2has successfully automatically executed command ^1!gb ^2on player ("s +
                get_player_information(online_player.pid) + "^3)\n"s};
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // private_message.c_str(), is_append_message_to_richedit_control::yes,
            // is_log_message::yes, is_log_datetime::yes);
            global_ban_player_ip_address(online_player);
            replace_br_with_new_line(public_message);
            const string inform_message{format("{}\\{}\\{}", main_app.get_username(), public_message, private_message)};
            main_app.add_message_to_queue(message_t("inform-message", inform_message, true));
        }

        if (banned_ip_addresses.contains(online_player.ip_address))
        {
            string message{main_app.get_automatic_kick_ip_ban_msg()};
            player pd{};
            if (main_app.get_is_disable_automatic_kick_messages())
            {
                message.clear();
            }
            else
            {
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
                if (main_app.get_current_game_server().get_banned_player_data_for_ip_address(online_player.ip_address,
                                                                                             &pd))
                {
                    main_app.get_tinyrcon_dict()["{BAN_DATE}"] = pd.banned_date_time;
                    pd.reason = remove_disallowed_character_in_string(pd.reason);
                    main_app.get_tinyrcon_dict()["{REASON}"] = pd.reason;
                }
                build_tiny_rcon_message(message);
            }
            const string banned_player_information{get_player_information(online_player.pid)};
            const string msg{format("^1Automatic kick ^5for previously ^3banned IP: "
                                    "^1{} ^5| ^3Reason: ^1{}\n{}\n",
                                    online_player.ip_address, pd.reason, banned_player_information)};
            replace_br_with_new_line(message);
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            kick_player(online_player.pid, message);
        }

        if (!banned_ip_address_ranges.empty())
        {
            const bool is_short_ip_address_range_ban{banned_ip_address_ranges.contains(narrow_ip_address_range) &&
                                                     !is_ip_range_protected && !is_city_protected &&
                                                     !is_country_protected};
            const bool is_wider_ip_address_range_ban{banned_ip_address_ranges.contains(wide_ip_address_range) &&
                                                     !is_ip_range_protected && !is_city_protected &&
                                                     !is_country_protected};

            if (is_short_ip_address_range_ban || is_wider_ip_address_range_ban)
            {
                string public_message{main_app.get_automatic_kick_ip_address_range_ban_msg()};
                player pd{};
                if (main_app.get_is_disable_automatic_kick_messages())
                {
                    public_message.clear();
                }
                else
                {
                    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
                    if (main_app.get_current_game_server().get_banned_player_data_for_ip_address_range(
                            online_player.ip_address, &pd))
                    {
                        main_app.get_tinyrcon_dict()["{BAN_DATE}"] = pd.banned_date_time;
                        pd.reason = remove_disallowed_character_in_string(pd.reason);
                        main_app.get_tinyrcon_dict()["{REASON}"] = pd.reason;
                    }
                    build_tiny_rcon_message(public_message);
                }
                const string banned_player_information{get_player_information(online_player.pid)};
                const string private_message{
                    format("^1Automatic kick ^5for player with an ^1IP address ^5from a "
                           "previously ^1banned IP address range: {}\n ^5| Reason: ^1{}\n{}\n",
                           (is_short_ip_address_range_ban ? narrow_ip_address_range : wide_ip_address_range), pd.reason,
                           banned_player_information)};
                replace_br_with_new_line(public_message);
                print_colored_text(app_handles.hwnd_re_messages_data, private_message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                kick_player(online_player.pid, public_message);
            }
        }

        if (main_app.get_is_automatic_city_kick_enabled() && !banned_cities.empty())
        {
            if (banned_cities.contains(online_player.city))
            {
                string message{main_app.get_automatic_kick_city_ban_msg()};
                if (main_app.get_is_disable_automatic_kick_messages())
                {
                    message.clear();
                }
                else
                {
                    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
                    main_app.get_tinyrcon_dict()["{CITY_NAME}"] = online_player.city;
                    build_tiny_rcon_message(message);
                }
                const string banned_player_information{get_player_information(online_player.pid)};
                const string msg{format("^1Automatic kick ^5for player ^7{} ^5with a "
                                        "previously ^1banned city name: {}\n{}\n",
                                        online_player.player_name, online_player.city, banned_player_information)};
                replace_br_with_new_line(message);
                print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                kick_player(online_player.pid, message);
            }
        }

        if (main_app.get_is_automatic_country_kick_enabled() && !banned_countries.empty())
        {
            if (banned_countries.contains(online_player.country_name))
            {
                string message{main_app.get_automatic_kick_country_ban_msg()};
                if (main_app.get_is_disable_automatic_kick_messages())
                {
                    message.clear();
                }
                else
                {
                    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
                    main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = online_player.country_name;
                    build_tiny_rcon_message(message);
                }
                const string banned_player_information{get_player_information(online_player.pid)};
                string msg{format("^1Automatic kick ^5for player ^7{} ^5with a "
                                  "previously ^1banned country name: {}\n{}\n",
                                  online_player.player_name, online_player.country_name, banned_player_information)};
                replace_br_with_new_line(message);
                print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                kick_player(online_player.pid, message);
            }
        }

        if (!banned_player_names.empty())
        {
            string player_name{online_player.player_name};
            remove_all_color_codes(player_name);
            trim_in_place(player_name);
            to_lower_case_in_place(player_name);

            if (banned_player_names.contains(player_name))
            {
                string message{main_app.get_automatic_kick_name_ban_msg()};

                if (main_app.get_is_disable_automatic_kick_messages())
                {
                    message.clear();
                }
                else
                {
                    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = online_player.player_name;
                    player &pd = banned_player_names[player_name];
                    main_app.get_tinyrcon_dict()["{BAN_DATE}"] = pd.banned_date_time;
                    pd.reason = remove_disallowed_character_in_string(pd.reason);
                    main_app.get_tinyrcon_dict()["{REASON}"] = pd.reason;
                    build_tiny_rcon_message(message);
                }
                const string banned_player_information{get_player_information(online_player.pid)};
                const string msg{format("^1Automatic kick ^5for previously ^3banned "
                                        "player name: ^1{} ^5| ^3Reason: ^1{}\n{}\n",
                                        player_name, main_app.get_tinyrcon_dict()["{REASON}"],
                                        banned_player_information)};
                replace_br_with_new_line(message);
                print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                kick_player(online_player.pid, message);
            }
        }
    }
}

bool check_if_player_is_protected(const player &online_player, const char *admin_command, string &message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    bool is_ip_protected{};
    bool is_ip_range_protected{};
    unsigned long guid{};
    if (check_ip_address_validity(online_player.ip_address, guid))
    {
        const string narrow_ip_address_range{
            get_narrow_ip_address_range_for_specified_ip_address(online_player.ip_address)};
        const string wide_ip_address_range{
            get_wide_ip_address_range_for_specified_ip_address(online_player.ip_address)};
        is_ip_protected =
            main_app.get_current_game_server().get_protected_ip_addresses().contains(online_player.ip_address);
        is_ip_range_protected =
            main_app.get_current_game_server().get_protected_ip_address_ranges().contains(narrow_ip_address_range) ||
            main_app.get_current_game_server().get_protected_ip_address_ranges().contains(wide_ip_address_range);

        if (is_ip_protected)
        {
            message.assign(format("^3Discarded ^1{} ^3on player ^7{} ^3whose IP address ^1{} ^3is "
                                  "protected!\n^5Please, remove their ^1protected IP address ^5first.",
                                  admin_command, online_player.player_name, online_player.ip_address));
            return true;
        }

        if (is_ip_range_protected)
        {
            message.assign(format(
                "^3Discarded ^1{} ^3on player ^7{} ^3whose IP address range "
                "^1{} ^3is protected!\n^5Please, remove their ^1protected IP "
                "address range ^5first.",
                admin_command, online_player.player_name,
                main_app.get_current_game_server().get_protected_ip_address_ranges().contains(narrow_ip_address_range)
                    ? narrow_ip_address_range
                    : wide_ip_address_range));
            return true;
        }
    }
    const bool is_city_protected{
        len(online_player.city) != 0 &&
        main_app.get_current_game_server().get_protected_cities().contains(online_player.city)};
    const bool is_country_protected{
        len(online_player.country_name) != 0 &&
        main_app.get_current_game_server().get_protected_countries().contains(online_player.country_name)};

    if (is_city_protected)
    {
        message.assign(format("^3Discarded ^1{} ^3on player ^7{} ^3whose city ^1{} ^3is "
                              "protected!\n^5Please, remove their ^1protected city ^5first.",
                              admin_command, online_player.player_name, online_player.city));
        return true;
    }

    if (is_country_protected)
    {

        message.assign(format("^3Discarded ^1{} ^3on player ^7{} ^3whose country ^1{} ^3is "
                              "protected!\n^5Please, remove their ^1protected country ^5first.",
                              admin_command, online_player.player_name, online_player.country_name));
        return true;
    }

    return false;
}

void kick_player(const int pid, const string &message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    // const auto &pd = get_player_data_for_pid(pid);
    // int ping{};
    // const string ping_str{ stl::helper::trim(pd.ping) };
    // const bool is_send_public_message_to_server{
    // is_valid_decimal_whole_number(ping_str, ping) && ping != -1 }; if
    // (!is_send_public_message_to_server) {
    //   const string inform_msg{ format("^5Not displaying ^1public message ^5on
    //   the server because player ^7{}\n ^5has already been ^1kicked.\n",
    //   pd.player_name) }; print_colored_text(app_handles.hwnd_re_messages_data,
    //   inform_msg.c_str());
    // } else {
    // auto &already_displayed_public_messages =
    // main_app.get_already_seen_messages(); if (!custom_message.empty() &&
    // !already_displayed_public_messages.contains(custom_message)) {
    //   already_displayed_public_messages.insert(custom_message);
    //   string message{ custom_message };
    //   rcon_say(message);
    // }
    // }

    const string inform_msg{format("{}\\{}", main_app.get_username(), message)};
    main_app.get_connection_manager_for_messages().process_and_send_message(
        "inform-message", inform_msg, true, main_app.get_tiny_rcon_server_ip_address(),
        main_app.get_tiny_rcon_server_port(), false);

    static char buffer[128];
    (void)snprintf(buffer, std::size(buffer), "clientkick %d", pid);
    string reply_buffer;
    main_app.get_connection_manager().send_and_receive_rcon_data(
        buffer, reply_buffer, main_app.get_current_game_server().get_server_ip_address().c_str(),
        main_app.get_current_game_server().get_server_port(),
        main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(), true);
    auto &warned_players = main_app.get_current_game_server().get_warned_players_data();
    if (warned_players.find(pid) != end(warned_players))
    {
        warned_players.erase(pid);
    }
}

void tempban_player(player &pd, std::string &custom_message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    temp_ban_player_ip_address(pd);
    kick_player(pd.pid, custom_message);
}

void ban_player(const int pid, std::string &custom_message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char buffer[256];
    (void)snprintf(buffer, std::size(buffer), "banclient %d", pid);
    string reply_buffer;
    main_app.get_connection_manager().send_and_receive_rcon_data(
        buffer, reply_buffer, main_app.get_current_game_server().get_server_ip_address().c_str(),
        main_app.get_current_game_server().get_server_port(),
        main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(), true);
    kick_player(pid, custom_message);
}

void say_message(const char *message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char buffer[512];
    (void)snprintf(buffer, std::size(buffer), "say \"%s\"", message);
    // main_app.add_command_to_queue({ "say", format("\"{}\"", message) },
    // command_type::rcon, false);
    string reply_buffer;
    main_app.get_connection_manager().send_and_receive_rcon_data(
        buffer, reply_buffer, main_app.get_current_game_server().get_server_ip_address().c_str(),
        main_app.get_current_game_server().get_server_port(),
        main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(), false,
        false);
}

void rcon_say(string &msg, const bool is_print_to_rich_edit_messages_box)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    using namespace stl::helper;
    str_replace_all(msg, "{{br}}", "\n");
    str_replace_all(msg, "\\", "|");
    str_replace_all(msg, "/", "|");

    msg = word_wrap(msg.c_str(), 140);
    const auto lines = str_split(msg, "\n", nullptr, split_on_whole_needle_t::yes);
    for (const auto &line : lines)
    {
        say_message(line.c_str());
        if (is_print_to_rich_edit_messages_box)
        {
            print_colored_text(app_handles.hwnd_re_messages_data, line.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes,
                               true, true);
        }
    }
}

std::string word_wrap(const char *src, const size_t line_width)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    char buffer[4096];

    const size_t src_len{std::min<size_t>(stl::helper::len(src), std::size(buffer) - 1)};
    size_t i{};

    while (i < src_len)
    {
        // copy src until the end of the line is reached
        for (size_t counter{1}; counter <= line_width; ++counter)
        {
            // check if end of src reached
            if (i == src_len)
            {
                buffer[i] = 0;
                return buffer;
            }
            buffer[i] = src[i];
            // check for newlines embedded in the original input
            // and reset the index
            if (buffer[i] == '\n')
            {
                counter = 1;
            }
            ++i;
        }
        // check for whitespace
        if (isspace(src[i]))
        {
            buffer[i] = '\n';
            ++i;
        }
        else
        {
            // check for nearest whitespace back in src
            for (size_t k{i}; k > 0; k--)
            {
                if (isspace(src[k]))
                {
                    buffer[k] = '\n';
                    // set src index back to character after this one
                    i = k + 1;
                    break;
                }
            }
        }
    }
    buffer[i] = 0;
    string result{buffer};
    return result;
}

void tell_message(const char *message, const int pid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    using namespace stl::helper;
    string msg{message};
    str_replace_all(msg, "{{br}}", "\n");
    str_replace_all(msg, "\\", "|");
    str_replace_all(msg, "/", "|");

    msg = word_wrap(msg.c_str(), 140);
    const auto lines = str_split(msg, "\n", nullptr, split_on_whole_needle_t::yes);

    static char buffer[512]{};
    for (const auto &line : lines)
    {
        // main_app.add_command_to_queue({ "tell", to_string(pid), format("\"{}\"",
        // line) }, command_type::rcon, false);
        (void)snprintf(buffer, std::size(buffer), "tell %d \"%s\"", pid, line.c_str());
        string reply_buffer;
        main_app.get_connection_manager().send_and_receive_rcon_data(
            buffer, reply_buffer, main_app.get_current_game_server().get_server_ip_address().c_str(),
            main_app.get_current_game_server().get_server_port(),
            main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(), false,
            false);
    }
}

void process_user_input(std::string &user_input)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    trim_in_place(user_input);
    if (!user_input.empty())
    {

        auto command_parts = str_split(user_input, " \t\n\f\v", "", split_on_whole_needle_t::no);

        if (!command_parts.empty())
        {
            to_lower_case_in_place(command_parts[0]);
            commands_history.emplace_back(user_input);
            commands_history_index = commands_history.size() - 1;
            if (rcon_status_commands.contains(command_parts[0]))
            {
                initiate_sending_rcon_status_command_now();
            }
            else if (command_parts[0] == "q" || command_parts[0] == "!q" || command_parts[0] == "!quit" ||
                     command_parts[0] == "e" || command_parts[0] == "!e" || command_parts[0] == "exit")
            {
                user_input = "q";
            }
            else if (user_commands_set.find(command_parts[0]) != cend(user_commands_set))
            {
                main_app.add_command_to_queue(std::move(command_parts), command_type::user, false);
            }
            else if (rcon_commands_set.find(command_parts[0]) != cend(rcon_commands_set))
            {
                const bool is_wait_for_reply{rcon_commands_wait_for_reply.find(command_parts[0]) !=
                                             std::cend(rcon_commands_wait_for_reply)};
                main_app.add_command_to_queue(std::move(command_parts), command_type::rcon, is_wait_for_reply);
            }
        }
    }
}

void process_user_command(const std::vector<string> &user_cmd)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (!user_cmd.empty() && main_app.get_command_handlers().contains(user_cmd[0]))
    {
        const string re_msg{format("^2Executing user command: ^5{}\n", str_join(user_cmd, " "))};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_command_handlers().at(user_cmd[0])(user_cmd);
    }
    else
    {
        const string re_msg{format("^3Unknown user command: ^5{}\n", str_join(user_cmd, " "))};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    }
}

void process_rcon_command(const std::vector<string> &rcon_cmd)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (!rcon_cmd.empty() && rcon_commands_set.contains(rcon_cmd[0]))
    {
        const string re_msg{format("^2Executing rcon command: ^5{}\n", str_join(rcon_cmd, " "))};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        if (main_app.get_command_handlers().contains(rcon_cmd[0]))
        {
            main_app.get_command_handlers().at(rcon_cmd[0])(rcon_cmd);
        }
        else if (main_app.get_command_handlers().contains("unknown-rcon"))
        {
            main_app.get_command_handlers().at("unknown-rcon")(rcon_cmd);
        }
    }
    else
    {
        const string re_msg{format("^3Unknown rcon command: ^5{}\n", str_join(rcon_cmd, " "))};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    }
}

volatile bool should_program_terminate(const string &user_input)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (user_input.empty())
        return is_terminate_program.load();
    is_terminate_program.store(user_input == "q" || user_input == "!q" || user_input == "!quit" ||
                               user_input == "quit" || user_input == "e" || user_input == "!e" ||
                               user_input == "exit" || user_input == "!exit");
    return is_terminate_program.load();
}

void sort_players_data(std::vector<player> &players_data, const sort_type sort_method)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const size_t number_of_players{main_app.get_current_game_server().get_number_of_players()};
    /*if (sort_method == sort_type::geo_asc || sort_method == sort_type::geo_desc
  && (players_data[0].country_name == nullptr || len(players_data[0].country_name)
  == 0 || strcmp(players_data[0].country_name, "Unknown") == 0)) { sort_method =
  sort_type::score_desc;
  }*/
    switch (sort_method)
    {
    case sort_type::pid_asc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) { return pl1.pid < pl2.pid; });
        break;

    case sort_type::pid_desc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) { return pl1.pid > pl2.pid; });
        break;

    case sort_type::score_asc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) { return pl1.score < pl2.score; });
        break;

    case sort_type::score_desc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) { return pl1.score > pl2.score; });
        break;

    case sort_type::ping_asc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) { return strcmp(pl1.ping, pl2.ping) < 0; });
        break;

    case sort_type::ping_desc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) { return strcmp(pl1.ping, pl2.ping) > 0; });
        break;

    case sort_type::ip_asc:
        if (me->is_admin)
        {
            std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                      [](const player &pl1, const player &pl2) {
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
        if (me->is_admin)
        {
            std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                      [](const player &pl1, const player &pl2) {
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
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) {
                      string pl1_cleaned_lc_name{pl1.player_name};
                      string pl2_cleaned_lc_name{pl2.player_name};
                      remove_all_color_codes(pl1_cleaned_lc_name);
                      remove_all_color_codes(pl2_cleaned_lc_name);
                      to_lower_case_in_place(pl1_cleaned_lc_name);
                      to_lower_case_in_place(pl2_cleaned_lc_name);
                      return pl1_cleaned_lc_name < pl2_cleaned_lc_name;
                  });
        break;

    case sort_type::name_desc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) {
                      string pl1_cleaned_lc_name{pl1.player_name};
                      string pl2_cleaned_lc_name{pl2.player_name};
                      remove_all_color_codes(pl1_cleaned_lc_name);
                      remove_all_color_codes(pl2_cleaned_lc_name);
                      to_lower_case_in_place(pl1_cleaned_lc_name);
                      to_lower_case_in_place(pl2_cleaned_lc_name);
                      return pl1_cleaned_lc_name > pl2_cleaned_lc_name;
                  });
        break;

    case sort_type::geo_asc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) {
                      char buffer1[256];
                      (void)snprintf(buffer1, std::size(buffer1), "%s, %s",
                                     (len(pl1.country_name) > 0 ? pl1.country_name : pl1.region), pl1.city);
                      char buffer2[256];
                      (void)snprintf(buffer2, std::size(buffer2), "%s, %s",
                                     (len(pl2.country_name) > 0 ? pl2.country_name : pl2.region), pl2.city);
                      string pl1_cleaned_geo{buffer1};
                      string pl2_cleaned_geo{buffer2};
                      to_lower_case_in_place(pl1_cleaned_geo);
                      to_lower_case_in_place(pl2_cleaned_geo);
                      return pl1_cleaned_geo < pl2_cleaned_geo;
                  });
        break;

    case sort_type::geo_desc:
        std::sort(std::begin(players_data), std::begin(players_data) + number_of_players,
                  [](const player &pl1, const player &pl2) {
                      char buffer1[256];
                      (void)snprintf(buffer1, std::size(buffer1), "%s, %s",
                                     (len(pl1.country_name) > 0 ? pl1.country_name : pl1.region), pl1.city);
                      char buffer2[256];
                      (void)snprintf(buffer2, std::size(buffer2), "%s, %s",
                                     (len(pl2.country_name) > 0 ? pl2.country_name : pl2.region), pl2.city);
                      string pl1_cleaned_geo{buffer1};
                      string pl2_cleaned_geo{buffer2};
                      to_lower_case_in_place(pl1_cleaned_geo);
                      to_lower_case_in_place(pl2_cleaned_geo);
                      return pl2_cleaned_geo < pl1_cleaned_geo;
                  });
        break;

    case sort_type::unknown:
        break;
    }
}

void display_temporarily_banned_ip_addresses(const size_t number_of_last_bans_to_display,
                                             const bool is_save_data_to_log_file)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t longest_name_length{12};
    size_t longest_country_length{20};
    auto &temp_banned_players = main_app.get_current_game_server().get_temp_banned_ip_addresses_vector();
    if (!temp_banned_players.empty())
    {
        longest_name_length =
            std::max(longest_name_length,
                     find_longest_player_name_length(temp_banned_players.cbegin(), temp_banned_players.cend(), false));
        longest_country_length =
            std::max(longest_country_length,
                     find_longest_player_country_city_info_length(temp_banned_players, temp_banned_players.size()));
    }

    ostringstream oss;
    ostringstream log;

    const size_t number_of_entries_to_display{number_of_last_bans_to_display != string::npos &&
                                                      number_of_last_bans_to_display < temp_banned_players.size()
                                                  ? number_of_last_bans_to_display
                                                  : temp_banned_players.size()};
    oss << format("\n^5Last ^1{} ^5temporary {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "bans" : "ban");
    log << format("\nLast {} temporary {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "bans" : "ban");

    const string decoration_line(149 + longest_name_length + longest_country_length, '=');
    oss << "^5\n"
        << decoration_line << "\n"
        << "^5| ";
    if (is_save_data_to_log_file)
    {
        log << "\n"
            << decoration_line << "\n"
            << "| ";
    }
    oss << left << setw(5) << "No."
        << " | " << left << setw(15) << "IP address"
        << " | " << left << setw(longest_name_length) << "Player name"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Date/time of tempban"
        << " | " << left << setw(28) << "Tempban expires in"
        << " | " << left << setw(25) << "Reason"
        << " | " << left << setw(32) << "Banned by admin"
        << "|";
    if (is_save_data_to_log_file)
    {
        log << left << setw(5) << "No."
            << " | " << left << setw(15) << "IP address"
            << " | " << left << setw(longest_name_length) << "Player name"
            << " | " << left << setw(longest_country_length) << "Country, city"
            << " | " << left << setw(20) << "Date/time of tempban"
            << " | " << left << setw(28) << "Tempban expires in"
            << " | " << left << setw(25) << "Reason"
            << " | " << left << setw(32) << "Banned by admin"
            << "|";
    }
    oss << "^5\n" << decoration_line << "\n";
    if (is_save_data_to_log_file)
    {
        log << "\n" << decoration_line << "\n";
    }
    if (temp_banned_players.empty())
    {
        const size_t message_len =
            stl::helper::len("| There are no players temporarily banned by their IP addresses yet.");
        oss << "^5| ^3There are no players temporarily banned by their IP "
               "addresses yet.";
        if (is_save_data_to_log_file)
        {
            log << "| There are no players temporarily banned by their IP addresses "
                   "yet.";
        }

        if (message_len + 2 < decoration_line.length())
        {
            oss << string(decoration_line.length() - 2 - message_len, ' ');
            if (is_save_data_to_log_file)
            {
                log << string(decoration_line.length() - 2 - message_len, ' ');
            }
        }
        oss << " ^5|\n";
        if (is_save_data_to_log_file)
        {
            log << " |\n";
        }
    }
    else
    {
        const time_t now_in_seconds = get_current_time_stamp();
        const size_t start_index{number_of_last_bans_to_display != string::npos &&
                                         number_of_last_bans_to_display <= temp_banned_players.size()
                                     ? temp_banned_players.size() - number_of_last_bans_to_display
                                     : 0U};
        size_t no{start_index};
        bool is_first_color{true};
        for (size_t i{start_index}; i < temp_banned_players.size(); ++i)
        {
            auto &bp = temp_banned_players[i];
            ++no;
            const char *next_color{is_first_color ? "^3" : "^5"};
            oss << "^5| " << next_color << left << setw(5) << no << " ^5| " << next_color << left << setw(15)
                << (me->is_admin ? bp.ip_address : "hidden") << " ^5| ^7";
            if (is_save_data_to_log_file)
            {
                log << "| " << left << setw(5) << no << " | " << left << setw(15)
                    << (me->is_admin ? bp.ip_address : "hidden") << " | ";
            }
            stl::helper::trim_in_place(bp.player_name);
            string name{bp.player_name};
            remove_all_color_codes(name);
            const size_t printed_name_char_count1{get_number_of_characters_without_color_codes(bp.player_name)};
            const size_t printed_name_char_count2{name.length()};
            if (printed_name_char_count1 < longest_name_length)
            {
                oss << left << setw(longest_name_length)
                    << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
            }
            else
            {
                oss << left << setw(longest_name_length) << bp.player_name;
            }
            if (is_save_data_to_log_file)
            {
                if (printed_name_char_count2 < longest_name_length)
                {
                    log << left << setw(longest_name_length)
                        << name + string(longest_name_length - printed_name_char_count2, ' ');
                }
                else
                {
                    log << left << setw(longest_name_length) << name;
                }
            }
            oss << " ^5| ";
            if (is_save_data_to_log_file)
            {
                log << " | ";
            }
            char buffer2[256];
            (void)snprintf(buffer2, std::size(buffer2), "%s, %s",
                           (len(bp.country_name) != 0 ? bp.country_name : bp.region), bp.city);
            stl::helper::trim_in_place(bp.reason);
            const time_t ban_expires_time = bp.banned_start_time + (bp.ban_duration_in_hours * 3600);
            const time_t remaining_tempban_duration_in_seconds =
                ban_expires_time >= now_in_seconds ? ban_expires_time - now_in_seconds : 0;
            const string ban_start_date_str =
                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", bp.banned_start_time);
            const string ban_expires_date_str =
                get_time_interval_info_string_for_seconds(remaining_tempban_duration_in_seconds);
            oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left
                << setw(20) << ban_start_date_str << " ^5| " << next_color << left << setw(28) << ban_expires_date_str
                << " ^5| ";
            const size_t printed_reason_char_count1{get_number_of_characters_without_color_codes(bp.reason.c_str())};
            if (printed_reason_char_count1 < 25)
            {
                oss << next_color << left << setw(25) << (bp.reason + string(25 - printed_reason_char_count1, ' '));
            }
            else
            {
                oss << next_color << left << bp.reason;
            }
            oss << " ^5| ";
            stl::helper::trim_in_place(bp.banned_by_user_name);
            string banned_by_admin{bp.banned_by_user_name};
            remove_all_color_codes(banned_by_admin);
            const size_t printed_reason_char_count3{
                get_number_of_characters_without_color_codes(bp.banned_by_user_name.c_str())};
            if (printed_reason_char_count3 < 32)
            {
                oss << "^7" << left << setw(32)
                    << (bp.banned_by_user_name + string(32 - printed_reason_char_count3, ' '));
            }
            else
            {
                oss << "^7" << left << bp.banned_by_user_name;
            }
            oss << "^5|\n";
            if (is_save_data_to_log_file)
            {
                string reason{bp.reason};
                remove_all_color_codes(reason);
                log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20)
                    << ban_start_date_str << " | " << left << setw(28) << ban_expires_date_str << " | ";
                const size_t printed_reason_char_count2{reason.length()};
                if (printed_reason_char_count2 < 25)
                {
                    log << left << setw(25) << (reason + string(25 - printed_reason_char_count2, ' '));
                }
                else
                {
                    log << reason;
                }
                log << " | ";
                const size_t printed_reason_char_count4{banned_by_admin.length()};
                if (printed_reason_char_count4 < 32)
                {
                    log << left << setw(32) << (banned_by_admin + string(32 - printed_reason_char_count4, ' '));
                }
                else
                {
                    log << left << banned_by_admin;
                }
                log << "|\n";
            }
            is_first_color = !is_first_color;
        }
    }
    oss << string{"^5"s + decoration_line + "\n\n"s};
    if (is_save_data_to_log_file)
    {
        log << string{decoration_line + "\n\n"s};
    }
    const string message{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes,
                       is_log_message::no, is_log_datetime::no, true);
    if (is_save_data_to_log_file)
    {
        log_message(log.str(), is_log_datetime::yes);
    }
}

void display_banned_ip_address_ranges(const size_t number_of_last_bans_to_display, const bool is_save_data_to_log_file)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t longest_name_length{12};
    size_t longest_country_length{20};
    auto &banned_players = main_app.get_current_game_server().get_banned_ip_address_ranges_vector();
    if (!banned_players.empty())
    {
        longest_name_length =
            std::max(longest_name_length,
                     find_longest_player_name_length(banned_players.cbegin(), banned_players.cend(), false));
        longest_country_length =
            std::max(longest_country_length,
                     find_longest_player_country_city_info_length(banned_players, banned_players.size()));
    }

    ostringstream oss;
    ostringstream log;

    const size_t number_of_entries_to_display{number_of_last_bans_to_display != string::npos &&
                                                      number_of_last_bans_to_display < banned_players.size()
                                                  ? number_of_last_bans_to_display
                                                  : banned_players.size()};

    oss << format("\n^5Last ^1{} ^5IP address range {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "bans" : "ban");
    log << format("\nLast {} IP address range {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "bans" : "ban");
    const string decoration_line(127 + longest_name_length + longest_country_length, '=');
    oss << "^5\n" << decoration_line << "\n";
    oss << "^5| ";
    if (is_save_data_to_log_file)
    {
        log << "\n"
            << decoration_line << "\n"
            << "| ";
    }
    oss << left << setw(5) << "No."
        << " | " << left << setw(20) << "IP address range"
        << " | " << left << setw(longest_name_length) << "Player name"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Date/time of ban"
        << " | " << left << setw(29) << "Reason"
        << " | " << left << setw(32) << "Banned by admin"
        << "|";
    if (is_save_data_to_log_file)
    {
        log << left << setw(5) << "No."
            << " | " << left << setw(20) << "IP address range"
            << " | " << left << setw(longest_name_length) << "Player name"
            << " | " << left << setw(longest_country_length) << "Country, city"
            << " | " << left << setw(20) << "Date/time of ban"
            << " | " << left << setw(29) << "Reason"
            << " | " << left << setw(32) << "Banned by admin"
            << "|";
    }
    oss << "^5\n" << decoration_line << "\n";
    if (is_save_data_to_log_file)
    {
        log << "\n" << decoration_line << "\n";
    }
    if (banned_players.empty())
    {
        const size_t message_len = stl::helper::len("| You haven't banned any IP address ranges yet.");
        oss << "^5| ^3You haven't banned any IP address ranges yet.";
        if (is_save_data_to_log_file)
        {
            log << "| You haven't banned any IP address ranges yet.";
        }

        if (message_len + 2 < decoration_line.length())
        {
            oss << string(decoration_line.length() - 2 - message_len, ' ');
            if (is_save_data_to_log_file)
            {
                log << string(decoration_line.length() - 2 - message_len, ' ');
            }
        }
        oss << " ^5|\n";
        if (is_save_data_to_log_file)
        {
            log << " |\n";
        }
    }
    else
    {
        bool is_first_color{true};

        const size_t start_index{number_of_last_bans_to_display != string::npos &&
                                         number_of_last_bans_to_display <= banned_players.size()
                                     ? banned_players.size() - number_of_last_bans_to_display
                                     : 0U};
        size_t no{start_index};
        for (size_t i{start_index}; i < banned_players.size(); ++i)
        {
            auto &bp = banned_players[i];
            ++no;
            const char *next_color{is_first_color ? "^3" : "^5"};
            oss << "^5| " << next_color << left << setw(5) << no << " ^5| " << next_color << left << setw(20)
                << bp.ip_address << " ^5| ^7";
            if (is_save_data_to_log_file)
            {
                log << "| " << left << setw(5) << no << " | " << left << setw(20) << bp.ip_address << " | ";
            }
            stl::helper::trim_in_place(bp.player_name);
            string name{bp.player_name};
            remove_all_color_codes(name);
            const size_t printed_name_char_count1{get_number_of_characters_without_color_codes(bp.player_name)};
            const size_t printed_name_char_count2{name.length()};
            if (printed_name_char_count1 < longest_name_length)
            {
                oss << left << setw(longest_name_length)
                    << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
            }
            else
            {
                oss << left << setw(longest_name_length) << bp.player_name;
            }
            if (is_save_data_to_log_file)
            {
                if (printed_name_char_count2 < longest_name_length)
                {
                    log << left << setw(longest_name_length)
                        << name + string(longest_name_length - printed_name_char_count2, ' ');
                }
                else
                {
                    log << left << setw(longest_name_length) << name;
                }
            }

            oss << " ^5| ";

            if (is_save_data_to_log_file)
            {
                log << " | ";
            }
            char buffer2[256];
            snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region),
                     bp.city);
            stl::helper::trim_in_place(bp.reason);
            string reason{bp.reason};
            oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left
                << setw(20) << bp.banned_date_time << " ^5| ";
            const size_t printed_reason_char_count1{get_number_of_characters_without_color_codes(reason.c_str())};
            if (printed_reason_char_count1 < 29)
            {
                oss << next_color << left << setw(29) << (reason + string(29 - printed_reason_char_count1, ' '));
            }
            else
            {
                oss << next_color << left << reason;
            }
            oss << " ^5| ";
            stl::helper::trim_in_place(bp.banned_by_user_name);
            string banned_by_user{bp.banned_by_user_name};
            const size_t printed_reason_char_count3{
                get_number_of_characters_without_color_codes(banned_by_user.c_str())};
            if (printed_reason_char_count3 < 32)
            {
                oss << "^7" << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count3, ' '));
            }
            else
            {
                oss << "^7" << left << banned_by_user;
            }
            oss << "^5|\n";
            if (is_save_data_to_log_file)
            {
                remove_all_color_codes(reason);
                remove_all_color_codes(banned_by_user);
                log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20)
                    << bp.banned_date_time << " | ";
                const size_t printed_reason_char_count2{reason.length()};
                if (printed_reason_char_count2 < 29)
                {
                    log << left << setw(29) << (reason + string(29 - printed_reason_char_count2, ' '));
                }
                else
                {
                    log << reason;
                }
                log << " | ";
                const size_t printed_reason_char_count4{banned_by_user.length()};
                if (printed_reason_char_count4 < 32)
                {
                    log << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count4, ' '));
                }
                else
                {
                    log << banned_by_user;
                }
                log << "|\n";
            }
            is_first_color = !is_first_color;
        }
    }
    oss << string{"^5"s + decoration_line + "\n\n"s};
    if (is_save_data_to_log_file)
    {
        log << string{decoration_line + "\n\n"s};
    }
    const string message{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes,
                       is_log_message::no, is_log_datetime::no, true);
    if (is_save_data_to_log_file)
    {
        log_message(log.str(), is_log_datetime::yes);
    }
}

void display_protected_entries(const char *table_title, const std::set<std::string> &protected_entries,
                               const unordered_map<string, string> &online_player_names,
                               const bool is_save_data_to_log_file)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t longest_protected_entry_length{32};

    auto &banned_players = main_app.get_current_game_server().get_banned_ip_addresses_vector();
    if (!banned_players.empty())
    {
        longest_protected_entry_length =
            std::max(longest_protected_entry_length,
                     find_longest_entry_length(cbegin(protected_entries), cend(protected_entries), false));
    }

    ostringstream oss;
    ostringstream log;
    const string decoration_line(207 + longest_protected_entry_length, '=');
    oss << "^5\n" << decoration_line << "\n";
    oss << "^5| ";
    if (is_save_data_to_log_file)
    {
        log << "\n"
            << decoration_line << "\n"
            << "| ";
    }
    oss << left << setw(longest_protected_entry_length) << table_title << " | " << left << setw(200)
        << " Online player name(s)"
        << " |";
    if (is_save_data_to_log_file)
    {
        log << left << setw(longest_protected_entry_length) << table_title << " | " << left << setw(200)
            << " Online player name(s)"
            << " |";
    }
    oss << "^5\n" << decoration_line << "\n";
    if (is_save_data_to_log_file)
    {
        log << "\n" << decoration_line << "\n";
    }
    if (protected_entries.empty())
    {
        const size_t message_len = stl::helper::len("| There are no protected entries.");
        oss << "^5| ^3There are no protected entries.";
        if (is_save_data_to_log_file)
        {
            log << "| There are no protected entries.";
        }

        if (message_len + 2 < decoration_line.length())
        {
            oss << string(decoration_line.length() - 2 - message_len, ' ');
            if (is_save_data_to_log_file)
            {
                log << string(decoration_line.length() - 2 - message_len, ' ');
            }
        }
        oss << " ^5|\n";
        if (is_save_data_to_log_file)
        {
            log << " |\n";
        }
    }
    else
    {
        bool is_first_color{true};
        for (const auto &protected_entry : protected_entries)
        {
            const char *next_color{is_first_color ? "^3" : "^5"};
            oss << "^5| " << next_color;

            string entry{protected_entry};
            remove_all_color_codes(entry);
            const size_t printed_char_count1{get_number_of_characters_without_color_codes(protected_entry.c_str())};
            const size_t printed_char_count2{entry.length()};
            if (printed_char_count1 < longest_protected_entry_length)
            {
                oss << left << setw(longest_protected_entry_length)
                    << protected_entry + string(longest_protected_entry_length - printed_char_count1, ' ');
            }
            else
            {
                oss << left << setw(longest_protected_entry_length) << protected_entry;
            }
            if (is_save_data_to_log_file)
            {
                if (printed_char_count2 < longest_protected_entry_length)
                {
                    log << left << setw(longest_protected_entry_length)
                        << entry + string(longest_protected_entry_length - printed_char_count2, ' ');
                }
                else
                {
                    log << left << setw(longest_protected_entry_length) << entry;
                }
            }
            if (online_player_names.contains(protected_entry))
            {
                string online_player_name{online_player_names.at(protected_entry)};
                remove_all_color_codes(online_player_name);
                const size_t printed_char_count3{
                    get_number_of_characters_without_color_codes(online_player_names.at(protected_entry).c_str())};
                const size_t printed_char_count4{online_player_name.length()};
                if (printed_char_count3 < 200)
                {
                    oss << " ^5| " << next_color << left << setw(200)
                        << online_player_names.at(protected_entry) + string(200 - printed_char_count3, ' ');
                }
                else
                {
                    oss << " ^5| " << next_color << left << setw(200) << online_player_names.at(protected_entry);
                }
                if (is_save_data_to_log_file)
                {
                    if (printed_char_count4 < 200)
                    {
                        log << " | " << left << setw(200)
                            << online_player_name + string(200 - printed_char_count4, ' ');
                    }
                    else
                    {
                        log << " | " << left << setw(200) << online_player_name;
                    }
                }
            }
            oss << " ^5|\n";
            if (is_save_data_to_log_file)
            {
                log << " |\n";
            }

            is_first_color = !is_first_color;
        }
    }
    oss << string{"^5"s + decoration_line + "\n\n"s};
    if (is_save_data_to_log_file)
    {
        log << string{decoration_line + "\n\n"s};
    }
    const string message{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes,
                       is_log_message::no, is_log_datetime::no, true);
    if (is_save_data_to_log_file)
    {
        log_message(log.str(), is_log_datetime::yes);
    }
}

void display_reported_players(const size_t number_of_last_reports_to_display, const bool is_save_data_to_log_file)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t longest_name_length{12};
    size_t longest_country_length{20};
    auto &reported_players = main_app.get_reported_players();
    if (!reported_players.empty())
    {
        longest_name_length =
            std::max(longest_name_length,
                     find_longest_player_name_length(reported_players.cbegin(), reported_players.cend(), false));
        longest_country_length =
            std::max(longest_country_length,
                     find_longest_player_country_city_info_length(reported_players, reported_players.size()));
    }

    ostringstream oss;
    ostringstream log;

    const size_t number_of_entries_to_display{number_of_last_reports_to_display != string::npos &&
                                                      number_of_last_reports_to_display < reported_players.size()
                                                  ? number_of_last_reports_to_display
                                                  : reported_players.size()};

    oss << format("\n^5Last ^1{} ^5reported {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "players" : "player");
    log << format("\nLast {} reported {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "players" : "player");
    const string decoration_line(122 + longest_name_length + longest_country_length, '=');
    oss << "^5\n" << decoration_line << "\n";
    oss << "^5| ";
    if (is_save_data_to_log_file)
    {
        log << "\n"
            << decoration_line << "\n"
            << "| ";
    }
    oss << left << setw(5) << "No."
        << " | " << left << setw(longest_name_length) << "Player name"
        << " | " << left << setw(15) << "IP address"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Date/time of ban"
        << " | " << left << setw(29) << "Reason"
        << " | " << left << setw(32) << "Reported by"
        << "|";
    if (is_save_data_to_log_file)
    {
        log << left << setw(5) << "No."
            << " | " << left << setw(longest_name_length) << "Player name"
            << " | " << left << setw(15) << "IP address"
            << " | " << left << setw(longest_country_length) << "Country, city"
            << " | " << left << setw(20) << "Date/time of ban"
            << " | " << left << setw(29) << "Reason"
            << " | " << left << setw(32) << "Reported by"
            << "|";
    }
    oss << "^5\n" << decoration_line << "\n";
    if (is_save_data_to_log_file)
    {
        log << "\n" << decoration_line << "\n";
    }
    if (reported_players.empty())
    {
        const size_t message_len = stl::helper::len("| There are no reported players yet.");
        oss << "^5| ^3There are no reported players yet.";
        if (is_save_data_to_log_file)
        {
            log << "| There are no reported players yet.";
        }

        if (message_len + 2 < decoration_line.length())
        {
            oss << string(decoration_line.length() - 2 - message_len, ' ');
            if (is_save_data_to_log_file)
            {
                log << string(decoration_line.length() - 2 - message_len, ' ');
            }
        }
        oss << " ^5|\n";
        if (is_save_data_to_log_file)
        {
            log << " |\n";
        }
    }
    else
    {
        bool is_first_color{true};
        const size_t start_index{number_of_entries_to_display != string::npos &&
                                         number_of_entries_to_display <= reported_players.size()
                                     ? reported_players.size() - number_of_entries_to_display
                                     : 0U};
        size_t no{start_index};
        for (size_t i{start_index}; i < reported_players.size(); ++i)
        {
            auto &bp = reported_players[i];
            ++no;
            const char *next_color{is_first_color ? "^3" : "^5"};
            oss << "^5| " << next_color << left << setw(5) << no << " ^5| ^7";
            log << "| " << left << setw(5) << no << " | ";

            stl::helper::trim_in_place(bp.player_name);
            string name{bp.player_name};
            remove_all_color_codes(name);
            const size_t printed_name_char_count1{get_number_of_characters_without_color_codes(bp.player_name)};
            const size_t printed_name_char_count2{name.length()};
            if (printed_name_char_count1 < longest_name_length)
            {
                oss << left << setw(longest_name_length)
                    << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
            }
            else
            {
                oss << left << setw(longest_name_length) << bp.player_name;
            }
            if (is_save_data_to_log_file)
            {
                if (printed_name_char_count2 < longest_name_length)
                {
                    log << left << setw(longest_name_length)
                        << name + string(longest_name_length - printed_name_char_count2, ' ');
                }
                else
                {
                    log << left << setw(longest_name_length) << name;
                }
            }

            oss << " ^5| " << next_color << left << setw(15) << (me->is_admin ? bp.ip_address : "hidden") << " ^5| ";
            if (is_save_data_to_log_file)
            {
                log << " | " << left << setw(15) << (me->is_admin ? bp.ip_address : "hidden") << " | ";
            }

            char buffer2[256];
            snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region),
                     bp.city);
            stl::helper::trim_in_place(bp.reason);
            string reason{bp.reason};
            oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left
                << setw(20) << bp.banned_date_time << " ^5| ";
            const size_t printed_reason_char_count1{get_number_of_characters_without_color_codes(reason.c_str())};
            if (printed_reason_char_count1 < 29)
            {
                oss << next_color << left << setw(29) << (reason + string(29 - printed_reason_char_count1, ' '));
            }
            else
            {
                oss << next_color << left << reason;
            }
            oss << " ^5| ";
            stl::helper::trim_in_place(bp.banned_by_user_name);
            string banned_by_user{bp.banned_by_user_name};
            const size_t printed_reason_char_count3{
                get_number_of_characters_without_color_codes(banned_by_user.c_str())};
            if (printed_reason_char_count3 < 32)
            {
                oss << "^7" << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count3, ' '));
            }
            else
            {
                oss << "^7" << next_color << left << banned_by_user;
            }
            oss << "^5|\n";
            if (is_save_data_to_log_file)
            {
                remove_all_color_codes(reason);
                remove_all_color_codes(banned_by_user);
                log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20)
                    << bp.banned_date_time << " | ";
                const size_t printed_reason_char_count2{reason.length()};
                if (printed_reason_char_count2 < 29)
                {
                    log << left << setw(29) << (reason + string(29 - printed_reason_char_count2, ' '));
                }
                else
                {
                    log << reason;
                }
                log << " | ";
                const size_t printed_reason_char_count4{banned_by_user.length()};
                if (printed_reason_char_count4 < 32)
                {
                    log << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count4, ' '));
                }
                else
                {
                    log << banned_by_user;
                }
                log << "|\n";
            }
            is_first_color = !is_first_color;
        }
    }
    oss << string{"^5"s + decoration_line + "\n\n"s};
    if (is_save_data_to_log_file)
    {
        log << string{decoration_line + "\n\n"s};
    }
    const string message{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes,
                       is_log_message::no, is_log_datetime::no, true);
    if (is_save_data_to_log_file)
    {
        log_message(log.str(), is_log_datetime::yes);
    }
}

void display_permanently_banned_ip_addresses(const size_t number_of_last_bans_to_display,
                                             const bool is_save_data_to_log_file)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t longest_name_length{12};
    size_t longest_country_length{20};
    auto &banned_players = main_app.get_current_game_server().get_banned_ip_addresses_vector();
    if (!banned_players.empty())
    {
        longest_name_length =
            std::max(longest_name_length,
                     find_longest_player_name_length(banned_players.cbegin(), banned_players.cend(), false));
        longest_country_length =
            std::max(longest_country_length,
                     find_longest_player_country_city_info_length(banned_players, banned_players.size()));
    }

    ostringstream oss;
    ostringstream log;

    const size_t number_of_entries_to_display{number_of_last_bans_to_display != string::npos &&
                                                      number_of_last_bans_to_display < banned_players.size()
                                                  ? number_of_last_bans_to_display
                                                  : banned_players.size()};

    oss << format("\n^5Last ^1{} ^5IP address {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "bans" : "ban");
    log << format("\nLast {} IP address {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "bans" : "ban");
    const string decoration_line(122 + longest_name_length + longest_country_length, '=');
    oss << "^5\n" << decoration_line << "\n";
    oss << "^5| ";
    if (is_save_data_to_log_file)
    {
        log << "\n"
            << decoration_line << "\n"
            << "| ";
    }
    oss << left << setw(5) << "No."
        << " | " << left << setw(15) << "IP address"
        << " | " << left << setw(longest_name_length) << "Player name"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Date/time of ban"
        << " | " << left << setw(29) << "Reason"
        << " | " << left << setw(32) << "Banned by admin"
        << "|";
    if (is_save_data_to_log_file)
    {
        log << left << setw(5) << "No." << left << setw(15) << "IP address"
            << " | " << left << setw(longest_name_length) << "Player name"
            << " | " << left << setw(longest_country_length) << "Country, city"
            << " | " << left << setw(20) << "Date/time of ban"
            << " | " << left << setw(29) << "Reason"
            << " | " << left << setw(32) << "Banned by admin"
            << "|";
    }
    oss << "^5\n" << decoration_line << "\n";
    if (is_save_data_to_log_file)
    {
        log << "\n" << decoration_line << "\n";
    }
    if (banned_players.empty())
    {
        const size_t message_len =
            stl::helper::len("| There are no players permanently banned by their IP addresses yet.");
        oss << "^5| ^3There are no players permanently banned by their IP "
               "addresses yet.";
        if (is_save_data_to_log_file)
        {
            log << "| There are no players permanently banned by their IP addresses "
                   "yet.";
        }

        if (message_len + 2 < decoration_line.length())
        {
            oss << string(decoration_line.length() - 2 - message_len, ' ');
            if (is_save_data_to_log_file)
            {
                log << string(decoration_line.length() - 2 - message_len, ' ');
            }
        }
        oss << " ^5|\n";
        if (is_save_data_to_log_file)
        {
            log << " |\n";
        }
    }
    else
    {
        bool is_first_color{true};
        const size_t start_index{number_of_last_bans_to_display != string::npos &&
                                         number_of_last_bans_to_display <= banned_players.size()
                                     ? banned_players.size() - number_of_last_bans_to_display
                                     : 0U};
        size_t no{start_index};
        for (size_t i{start_index}; i < banned_players.size(); ++i)
        {
            auto &bp = banned_players[i];
            ++no;
            const char *next_color{is_first_color ? "^3" : "^5"};
            oss << "^5| " << next_color << left << setw(5) << no << " ^5| " << next_color << left << setw(15)
                << (me->is_admin ? bp.ip_address : "hidden") << " ^5| ^7";
            if (is_save_data_to_log_file)
            {
                log << "| " << left << setw(5) << no << " | " << left << setw(15)
                    << (me->is_admin ? bp.ip_address : "hidden") << " | ";
            }
            stl::helper::trim_in_place(bp.player_name);
            string name{bp.player_name};
            remove_all_color_codes(name);
            const size_t printed_name_char_count1{get_number_of_characters_without_color_codes(bp.player_name)};
            const size_t printed_name_char_count2{name.length()};
            if (printed_name_char_count1 < longest_name_length)
            {
                oss << left << setw(longest_name_length)
                    << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
            }
            else
            {
                oss << left << setw(longest_name_length) << bp.player_name;
            }
            if (is_save_data_to_log_file)
            {
                if (printed_name_char_count2 < longest_name_length)
                {
                    log << left << setw(longest_name_length)
                        << name + string(longest_name_length - printed_name_char_count2, ' ');
                }
                else
                {
                    log << left << setw(longest_name_length) << name;
                }
            }
            oss << " ^5| ";
            if (is_save_data_to_log_file)
            {
                log << " | ";
            }
            char buffer2[256];
            snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region),
                     bp.city);
            stl::helper::trim_in_place(bp.reason);
            string reason{bp.reason};
            oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left
                << setw(20) << bp.banned_date_time << " ^5| ";
            const size_t printed_reason_char_count1{get_number_of_characters_without_color_codes(reason.c_str())};
            if (printed_reason_char_count1 < 29)
            {
                oss << next_color << left << setw(29) << (reason + string(29 - printed_reason_char_count1, ' '));
            }
            else
            {
                oss << next_color << left << reason;
            }
            oss << " ^5| ";
            stl::helper::trim_in_place(bp.banned_by_user_name);
            string banned_by_user{bp.banned_by_user_name};
            const size_t printed_reason_char_count3{
                get_number_of_characters_without_color_codes(banned_by_user.c_str())};
            if (printed_reason_char_count3 < 32)
            {
                oss << "^7" << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count3, ' '));
            }
            else
            {
                oss << "^7" << next_color << left << banned_by_user;
            }
            oss << "^5|\n";
            if (is_save_data_to_log_file)
            {
                remove_all_color_codes(reason);
                remove_all_color_codes(banned_by_user);
                log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20)
                    << bp.banned_date_time << " | ";
                const size_t printed_reason_char_count2{reason.length()};
                if (printed_reason_char_count2 < 29)
                {
                    log << left << setw(29) << (reason + string(29 - printed_reason_char_count2, ' '));
                }
                else
                {
                    log << reason;
                }
                log << " | ";
                const size_t printed_reason_char_count4{banned_by_user.length()};
                if (printed_reason_char_count4 < 32)
                {
                    log << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count4, ' '));
                }
                else
                {
                    log << banned_by_user;
                }
                log << "|\n";
            }
            is_first_color = !is_first_color;
        }
    }
    oss << string{"^5"s + decoration_line + "\n\n"s};
    if (is_save_data_to_log_file)
    {
        log << string{decoration_line + "\n\n"s};
    }
    const string message{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes,
                       is_log_message::no, is_log_datetime::no, true);
    if (is_save_data_to_log_file)
    {
        log_message(log.str(), is_log_datetime::yes);
    }
}

void display_banned_player_names(const char *title, const size_t number_of_last_bans_to_display,
                                 const bool is_save_data_to_log_file)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t longest_name_length{12};
    size_t longest_country_length{20};
    auto &banned_players = main_app.get_current_game_server().get_banned_names_vector();
    if (!banned_players.empty())
    {
        longest_name_length =
            std::max(longest_name_length,
                     find_longest_player_name_length(banned_players.cbegin(), banned_players.cend(), false));
        longest_country_length =
            std::max(longest_country_length,
                     find_longest_player_country_city_info_length(banned_players, banned_players.size()));
    }

    ostringstream oss;
    ostringstream log;

    const size_t number_of_entries_to_display{number_of_last_bans_to_display != string::npos &&
                                                      number_of_last_bans_to_display < banned_players.size()
                                                  ? number_of_last_bans_to_display
                                                  : banned_players.size()};

    oss << format("\n^5Last ^1{} ^5banned player {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "names" : "name");
    log << format("\nLast {} banned player {}:", number_of_entries_to_display,
                  number_of_entries_to_display != 1 ? "names" : "name");
    const string decoration_line(122 + longest_name_length + longest_country_length, '=');
    oss << "^5\n" << decoration_line << "\n";
    std::string title_str{title};
    stl::helper::trim_in_place(title_str);
    remove_all_color_codes(title_str);
    const size_t printed_name_char_count{get_number_of_characters_without_color_codes(title)};
    const size_t padding_size{(decoration_line.length() - 4u - printed_name_char_count)};
    const string pad_str(padding_size, ' ');
    if (printed_name_char_count + 4u < decoration_line.length())
    {
        oss << "^5| " << left << title << right << pad_str << " ^5|\n";
    }
    else
    {
        oss << "^5| " << left << title << " ^5|\n";
    }
    oss << decoration_line << "\n";
    oss << "^5| ";
    if (is_save_data_to_log_file)
    {
        log << "\n" << decoration_line << "\n";
        if (printed_name_char_count + 4u < decoration_line.length())
        {
            log << "| " << left << title_str << right << pad_str << " |\n";
        }
        else
        {
            log << "| " << left << title_str << " |\n";
        }
        log << decoration_line << "\n"
            << "| ";
    }
    oss << left << setw(5) << "No."
        << " | " << left << setw(15) << "IP address"
        << " | " << left << setw(longest_name_length) << "Player name"
        << " | " << left << setw(longest_country_length) << "Country, city"
        << " | " << left << setw(20) << "Date/time of ban"
        << " | " << left << setw(29) << "Reason"
        << " | " << left << setw(32) << "Banned by admin"
        << "|";
    if (is_save_data_to_log_file)
    {
        oss << left << setw(5) << "No."
            << " | " << left << setw(15) << "IP address"
            << " | " << left << setw(longest_name_length) << "Player name"
            << " | " << left << setw(longest_country_length) << "Country, city"
            << " | " << left << setw(20) << "Date/time of ban"
            << " | " << left << setw(29) << "Reason"
            << " | " << left << setw(32) << "Banned by admin"
            << "|";
    }
    oss << "^5\n" << decoration_line << "\n";
    if (is_save_data_to_log_file)
    {
        log << "\n" << decoration_line << "\n";
    }
    if (banned_players.empty())
    {
        const size_t message_len = stl::helper::len("| There are no players banned by player names yet.");
        oss << "^5| ^3There are no players banned by player names yet.";
        if (is_save_data_to_log_file)
        {
            log << "| There are no players banned by player names yet.";
        }

        if (message_len + 2 < decoration_line.length())
        {
            oss << string(decoration_line.length() - 2 - message_len, ' ');
            if (is_save_data_to_log_file)
            {
                log << string(decoration_line.length() - 2 - message_len, ' ');
            }
        }
        oss << " ^5|\n";
        if (is_save_data_to_log_file)
        {
            log << " |\n";
        }
    }
    else
    {
        bool is_first_color{true};
        const size_t start_index{number_of_last_bans_to_display != string::npos &&
                                         number_of_last_bans_to_display <= banned_players.size()
                                     ? banned_players.size() - number_of_last_bans_to_display
                                     : 0U};
        size_t no{start_index};
        for (size_t i{start_index}; i < banned_players.size(); ++i)
        {
            auto &bp = banned_players[i];
            ++no;
            const char *next_color{is_first_color ? "^3" : "^5"};
            oss << "^5| " << next_color << left << setw(5) << no << " ^5| " << next_color << left << setw(15)
                << bp.ip_address << " ^5| ^7";
            if (is_save_data_to_log_file)
            {
                log << "| " << left << setw(5) << no << " | " << left << setw(15) << bp.ip_address << " | ";
            }
            stl::helper::trim_in_place(bp.player_name);
            string name{bp.player_name};
            remove_all_color_codes(name);
            const size_t printed_name_char_count1{get_number_of_characters_without_color_codes(bp.player_name)};
            const size_t printed_name_char_count2{name.length()};
            if (printed_name_char_count1 < longest_name_length)
            {
                oss << left << setw(longest_name_length)
                    << bp.player_name + string(longest_name_length - printed_name_char_count1, ' ');
            }
            else
            {
                oss << left << setw(longest_name_length) << bp.player_name;
            }
            if (is_save_data_to_log_file)
            {
                if (printed_name_char_count2 < longest_name_length)
                {
                    log << left << setw(longest_name_length)
                        << name + string(longest_name_length - printed_name_char_count2, ' ');
                }
                else
                {
                    log << left << setw(longest_name_length) << name;
                }
            }
            oss << " ^5| ";
            if (is_save_data_to_log_file)
            {
                log << " | ";
            }
            char buffer2[256];
            snprintf(buffer2, std::size(buffer2), "%s, %s", (len(bp.country_name) != 0 ? bp.country_name : bp.region),
                     bp.city);
            stl::helper::trim_in_place(bp.reason);
            string reason{bp.reason};
            oss << next_color << left << setw(longest_country_length) << buffer2 << " ^5| " << next_color << left
                << setw(20) << bp.banned_date_time << " ^5| ";
            const size_t printed_reason_char_count1{get_number_of_characters_without_color_codes(reason.c_str())};
            if (printed_reason_char_count1 < 29)
            {
                oss << next_color << left << setw(29) << (reason + string(29 - printed_reason_char_count1, ' '));
            }
            else
            {
                oss << next_color << left << reason;
            }
            oss << " ^5| ";
            stl::helper::trim_in_place(bp.banned_by_user_name);
            string banned_by_user{bp.banned_by_user_name};
            const size_t printed_reason_char_count3{
                get_number_of_characters_without_color_codes(banned_by_user.c_str())};
            if (printed_reason_char_count3 < 32)
            {
                oss << "^7" << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count3, ' '));
            }
            else
            {
                oss << "^7" << next_color << left << banned_by_user;
            }
            oss << "^5|\n";
            if (is_save_data_to_log_file)
            {
                remove_all_color_codes(reason);
                remove_all_color_codes(banned_by_user);
                log << left << setw(longest_country_length) << buffer2 << " | " << left << setw(20)
                    << bp.banned_date_time << " | ";
                const size_t printed_reason_char_count2{reason.length()};
                if (printed_reason_char_count2 < 29)
                {
                    log << left << setw(29) << (reason + string(29 - printed_reason_char_count2, ' '));
                }
                else
                {
                    log << reason;
                }
                log << " | ";
                const size_t printed_reason_char_count4{banned_by_user.length()};
                if (printed_reason_char_count4 < 32)
                {
                    log << left << setw(32) << (banned_by_user + string(32 - printed_reason_char_count4, ' '));
                }
                else
                {
                    log << banned_by_user;
                }
                log << "|\n";
            }
            is_first_color = !is_first_color;
        }
    }
    oss << string{"^5"s + decoration_line + "\n\n"s};
    if (is_save_data_to_log_file)
    {
        log << string{decoration_line + "\n\n"s};
    }
    const string message{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes,
                       is_log_message::no, is_log_datetime::no, true);
    if (is_save_data_to_log_file)
    {
        log_message(log.str(), is_log_datetime::yes);
    }
}

void display_admins_data(const std::vector<std::shared_ptr<tiny_rcon_client_user>> &users, const char *title)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t longest_name_length{12};
    size_t longest_geoinfo_length{20};
    if (!users.empty())
    {
        longest_name_length = std::max(longest_name_length, find_longest_user_name_length(users, false, users.size()));
        longest_geoinfo_length =
            std::max(longest_geoinfo_length, find_longest_user_country_city_info_length(users, users.size()));
    }

    ostringstream oss;
    const string decoration_line(250 + longest_name_length + longest_geoinfo_length, '=');
    oss << "^5\n" << decoration_line << "\n";
    std::string title_str{title};
    stl::helper::trim_in_place(title_str);
    remove_all_color_codes(title_str);
    const size_t printed_name_char_count{get_number_of_characters_without_color_codes(title)};
    const size_t padding_size{(decoration_line.length() - 4u - printed_name_char_count)};
    const string pad_str(padding_size, ' ');
    if (printed_name_char_count + 4u < decoration_line.length())
    {
        oss << "^5| " << left << title << right << pad_str << " ^5|\n";
    }
    else
    {
        oss << "^5| " << left << title << " ^5|\n";
    }
    oss << decoration_line << "\n";
    oss << "^5| ";
    oss << left << setw(longest_name_length) << "User name"
        << " | " << left << setw(13) << "Is logged in?"
        << " | " << left << setw(11) << "Is online?"
        << " | " << left << setw(16) << "IP address"
        << " | " << left << setw(longest_geoinfo_length) << "Country, city"
        << " | " << left << setw(20) << "Last login"
        << " | " << left << setw(20) << "Last logout"
        << " | ";
    oss << text_element{"Logins", 10, "^1"};
    oss << " ^5| ";
    oss << text_element{"Warnings", 10, "^2"};
    oss << " ^5| ";
    oss << text_element{"Kicks", 10, "^1"};
    oss << " ^5| ";
    oss << text_element{"Tempbans", 10, "^2"};
    oss << " ^5| ";
    oss << text_element{"GUID bans", 10, "^1"};
    oss << " ^5| ";
    oss << text_element{"IP bans", 10, "^2"};
    oss << " ^5| ";
    oss << text_element{"IP range bans", 13, "^1"};
    oss << " ^5| ";
    oss << text_element{"City bans", 10, "^2"};
    oss << " ^5| ";
    oss << text_element{"Country bans", 13, "^1"};
    oss << " ^5| ";
    oss << text_element{"Name bans", 10, "^2"};
    oss << " ^5| ";
    oss << text_element{"Reports", 10, "^1"};
    oss << "^5|";
    oss << "^5\n" << decoration_line << "\n";
    if (users.empty())
    {
        const size_t message_len = stl::helper::len("| There is no received administrator (user) data.");
        oss << "^5| ^3There is no received administrator (user) data.";

        if (message_len + 2 < decoration_line.length())
        {
            oss << string(decoration_line.length() - 2 - message_len, ' ');
        }
        oss << " ^5|\n";
    }
    else
    {
        bool is_first_color{true};
        char buffer[16];
        for (auto &user : users)
        {
            const char *next_color{is_first_color ? "^3" : "^5"};
            oss << "^5| ";
            oss << text_element{user->user_name.c_str(), longest_name_length, "^7"};

            oss << " ^5| ";
            oss << text_element{user->is_logged_in ? "yes" : "no", 13, user->is_logged_in ? "^2" : "^1"};

            oss << " ^5| ";
            oss << text_element{user->is_online ? "yes" : "no", 11, user->is_online ? "^2" : "^1"};

            oss << " ^5| ";
            if (me->is_admin)
            {
                oss << text_element{user->ip_address.c_str(), 16, next_color};
            }
            else
            {
                oss << text_element{"hidden", 16, next_color};
            }

            oss << " ^5| ";
            oss << text_element{user->geo_information.c_str(), longest_geoinfo_length, next_color};

            oss << " ^5| ";
            oss << text_element{
                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", user->last_login_time_stamp).c_str(), 20,
                next_color};

            oss << " ^5| ";
            oss << text_element{
                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", user->last_logout_time_stamp).c_str(), 20,
                next_color};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_logins);
            oss << text_element{buffer, 10, "^1"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_warnings);
            oss << text_element{buffer, 10, "^2"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_kicks);
            oss << text_element{buffer, 10, "^1"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_tempbans);
            oss << text_element{buffer, 10, "^2"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_guidbans);
            oss << text_element{buffer, 10, "^1"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_ipbans);
            oss << text_element{buffer, 10, "^2"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_iprangebans);
            oss << text_element{buffer, 13, "^1"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_citybans);
            oss << text_element{buffer, 10, "^2"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_countrybans);
            oss << text_element{buffer, 13, "^1"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_namebans);
            oss << text_element{buffer, 10, "^2"};

            oss << " ^5| ";
            snprintf(buffer, std::size(buffer), "%lu", user->no_of_reports);
            oss << text_element{buffer, 10, "^1"};

            oss << "^5|\n";

            is_first_color = !is_first_color;
        }
    }
    oss << string{"^5"s + decoration_line + "\n\n"s};

    const string message{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes,
                       is_log_message::no, is_log_datetime::no, true);
}

const std::string &get_full_gametype_name(const std::string &rcon_gametype_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const auto &full_gametype_names =
        get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());
    return full_gametype_names.find(rcon_gametype_name) != cend(full_gametype_names)
               ? full_gametype_names.at(rcon_gametype_name)
               : rcon_gametype_name;
}

const std::string &get_full_map_name(const std::string &rcon_map_name, const game_name_t game_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (game_name == game_name_t::cod2)
    {
        return main_app.get_available_rcon_to_full_map_names().contains(rcon_map_name)
                   ? main_app.get_available_rcon_to_full_map_names().at(rcon_map_name).second
                   : rcon_map_name;
    }
    const auto &rcon_map_names_to_full_map_names =
        get_rcon_map_names_to_full_map_names_for_specified_game_name(game_name);
    return rcon_map_names_to_full_map_names.contains(rcon_map_name) ? rcon_map_names_to_full_map_names.at(rcon_map_name)
                                                                    : rcon_map_name;
}

void display_all_available_maps()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ostringstream oss;
    size_t index{1};
    if (str_compare_i(main_app.get_current_game_server().get_current_game_type().c_str(), "cod2") == 0)
    {
        for (const auto &[rcon_map_name, full_map_name] : main_app.get_available_rcon_to_full_map_names())
        {
            oss << "^2" << rcon_map_name << " ^5-> " << full_map_name.second << '\n';
            ++index;
        }
    }
    else
    {
        const auto &rcon_map_names_to_full_map_names =
            get_rcon_map_names_to_full_map_names_for_specified_game_name(main_app.get_game_name());
        for (const auto &[rcon_map_name, full_map_name] : rcon_map_names_to_full_map_names)
        {
            oss << "^2" << rcon_map_name << " ^5-> " << full_map_name << '\n';
            ++index;
        }
    }
    print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(), is_append_message_to_richedit_control::yes,
                       is_log_message::yes, is_log_datetime::yes, false, true);
}

void import_geoip_data(vector<geoip_data> &geo_data, const char *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const string log_msg{"Importing geological binary data from specified file ("s + string{file_path} + ")."s};
    log_message(log_msg, is_log_datetime::yes);
    geo_data.clear();

    ifstream is{file_path, std::ios::binary};

    if (is)
    {
        const size_t file_size{get_file_size_in_bytes(file_path)};
        const size_t num_of_elements{file_size / sizeof(geoip_data)};
        geo_data.reserve(num_of_elements);
        geo_data.resize(num_of_elements);
        is.read(reinterpret_cast<char *>(geo_data.data()), file_size);
    }
}

void export_geoip_data(const vector<geoip_data> &geo_data, const char *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const string log_msg{"Exporting geological binary data to specified file ("s + string{file_path} + ")."s};
    log_message(log_msg, is_log_datetime::yes);
    ofstream os{file_path, std::ios::binary};

    if (os)
    {

        os.write(reinterpret_cast<const char *>(geo_data.data()),
                 static_cast<std::streamsize>(geo_data.size()) * static_cast<std::streamsize>(sizeof(geoip_data)));
    }
    os << flush;
}

void change_colors()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const std::regex color_code_regex{R"(\^?\d{1})"};
    const auto &fmc = main_app.get_full_map_name_color();
    const auto &rmc = main_app.get_rcon_map_name_color();
    const auto &fgc = main_app.get_full_gametype_name_color();
    const auto &rgc = main_app.get_rcon_gametype_name_color();
    const auto &onp = main_app.get_online_players_count_color();
    const auto &ofp = main_app.get_offline_players_count_color();
    const auto &bc = main_app.get_border_line_color();
    const auto &ph = main_app.get_header_player_pid_color();
    const auto &pd = main_app.get_data_player_pid_color();

    const auto &sh = main_app.get_header_player_score_color();
    const auto &sd = main_app.get_data_player_score_color();

    const auto &pgh = main_app.get_header_player_ping_color();
    const auto &pgd = main_app.get_data_player_ping_color();

    const auto &pnh = main_app.get_header_player_name_color();

    const auto &iph = main_app.get_header_player_ip_color();
    const auto &ipd = main_app.get_data_player_ip_color();

    const auto &gh = main_app.get_header_player_geoinfo_color();
    const auto &gd = main_app.get_data_player_geoinfo_color();

    print_colored_text(app_handles.hwnd_re_messages_data,
                       "^5\nType a number ^1(color code: 0-9) ^5for each color setting\n or press "
                       "^3(Enter) ^5to accept the default value for it.\n",
                       is_append_message_to_richedit_control::yes, is_log_message::no);

    string color_code;

    do
    {
        const string re_msg{fmc + "Full map name color (0-9), press enter to accept default (2): "};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
        color_code = "^2";
    else if (color_code[0] != '^')
        color_code.insert(0, 1, '^');
    main_app.set_full_map_name_color(color_code);

    do
    {
        const string re_msg{rmc + "Rcon map name color (0-9), press enter to accept default (1): "s};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
        color_code = "^1";
    else if (color_code[0] != '^')
        color_code.insert(0, 1, '^');
    main_app.set_rcon_map_name_color(color_code);

    do
    {
        const string re_msg{fgc + "Full gametype name color (0-9), press enter to accept default (2): "};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
        color_code = "^2";
    else if (color_code[0] != '^')
        color_code.insert(0, 1, '^');
    main_app.set_full_gametype_color(color_code);

    do
    {
        const string re_msg{rgc + "Rcon gametype name color (0-9), press enter to accept default (1): "};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
        color_code = "^1";
    else if (color_code[0] != '^')
        color_code.insert(0, 1, '^');
    main_app.set_rcon_gametype_color(color_code);

    do
    {
        const string re_msg{onp + "Online players count color (0-9), press enter to accept default (2): "};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));
    if (color_code.empty())
        color_code = "^2";
    else if (color_code[0] != '^')
        color_code.insert(0, 1, '^');
    main_app.set_online_players_count_color(color_code);

    do
    {
        const string re_msg{ofp + "Offline players count color (0-9), press enter to accept default (1): "};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
        color_code = "^1";
    else if (color_code[0] != '^')
        color_code.insert(0, 1, '^');
    main_app.set_offline_players_count_color(color_code);

    do
    {
        const string re_msg{bc + "Border line color (0-9), press enter to accept default (5): "};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
    } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

    if (color_code.empty())
        color_code = "^5";
    else if (color_code[0] != '^')
        color_code.insert(0, 1, '^');
    main_app.set_border_line_color(color_code);

    do
    {
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^3Use different alternating background colors for even "
                           "and odd lines? (yes|no, enter for default (yes): ",
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
        stl::helper::to_lower_case_in_place(color_code);
    } while (!color_code.empty() && color_code != "yes" && color_code != "no");

    main_app.set_is_use_different_background_colors_for_even_and_odd_lines(color_code.empty() || color_code == "yes");

    if (main_app.get_is_use_different_background_colors_for_even_and_odd_lines())
    {
        do
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               "^5Background color of odd player data lines (0-9), "
                               "press enter to accept default (0 - black): ",
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^0";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_odd_player_data_lines_bg_color(color_code);

        do
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               "^3Background color of even player data lines (0-9), "
                               "press enter to accept default (8 - gray): ",
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^8";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_even_player_data_lines_bg_color(color_code);
    }

    do
    {
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^5Use different alternating foreground colors for even "
                           "and odd lines? (yes|no, enter for default (yes): ",
                           is_append_message_to_richedit_control::yes, is_log_message::no);
        getline(cin, color_code);
        stl::helper::to_lower_case_in_place(color_code);
    } while (!color_code.empty() && color_code != "yes" && color_code != "no");

    main_app.set_is_use_different_foreground_colors_for_even_and_odd_lines(color_code.empty() || color_code == "yes");

    if (main_app.get_is_use_different_foreground_colors_for_even_and_odd_lines())
    {
        do
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               "Foreground color of odd player data lines (0-9), "
                               "press enter to accept default (5): ",
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^5";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_odd_player_data_lines_fg_color(color_code);

        do
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               "Foreground color of even player data lines (0-9), "
                               "press enter to accept default (3): ",
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^4";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_even_player_data_lines_fg_color(color_code);
    }
    else
    {

        do
        {
            const string re_msg{ph + "Player pid header color (0-9), press enter to accept default (1): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^1";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_header_player_pid_color(color_code);

        do
        {
            const string re_msg{pd + "Player pid data color (0-9), press enter to accept default (1): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);

            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^1";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_data_player_pid_color(color_code);

        do
        {
            const string re_msg{sh + "Player score header color (0-9), press enter to accept default (5): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^5";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_header_player_score_color(color_code);

        do
        {
            const string re_msg{sd + "Player score data color (0-9), press enter to accept default (5): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^5";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_data_player_score_color(color_code);

        do
        {
            const string re_msg{pgh + "Player ping header color (0-9), press enter to accept default (3): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^4";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_header_player_ping_color(color_code);

        do
        {
            const string re_msg{pgd + "Player ping data color (0-9), press enter to accept default (3): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^4";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_data_player_ping_color(color_code);

        do
        {
            const string re_msg{pnh + "Player name header color (0-9), press Enter to accept default (5): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^5";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_header_player_name_color(color_code);

        do
        {
            const string re_msg{iph + "Player IP header color (0-9), press Enter to accept default (5): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^5";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_header_player_ip_color(color_code);

        do
        {
            const string re_msg{ipd + "Player IP data color (0-9), press Enter to accept default (5): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^5";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_data_player_ip_color(color_code);

        do
        {
            const string re_msg{gh + "Player country/city header color (0-9), press "
                                     "Enter to accept default (2): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^2";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_header_player_geoinfo_color(color_code);

        do
        {
            const string re_msg{gd + "Player country/city data color (0-9), press "
                                     "Enter to accept default (2): "};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no);
            getline(cin, color_code);
        } while (!color_code.empty() && !regex_match(color_code, color_code_regex));

        if (color_code.empty())
            color_code = "^2";
        else if (color_code[0] != '^')
            color_code.insert(0, 1, '^');
        main_app.set_data_player_geoinfo_color(color_code);
        print_colored_text(app_handles.hwnd_re_messages_data, "^5\n", is_append_message_to_richedit_control::yes,
                           is_log_message::no);
    }

    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
}

void strip_leading_and_trailing_quotes(std::string &data)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (data.length() >= 2U && data.front() == '"' && data.back() == '"')
    {
        data.erase(cbegin(data));
        data.pop_back();
    }
}

void strip_leading_and_trailing_quotes(std::wstring &data)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (data.length() >= 2U && data.front() == L'"' && data.back() == L'"')
    {
        data.erase(cbegin(data));
        data.pop_back();
    }
}

void replace_all_escaped_new_lines_with_new_lines(std::string &data)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    while (true)
    {
        const size_t start{data.find("\\n")};
        if (string::npos == start)
            break;
        data.replace(start, 2, "\n");
    }
}

bool change_server_setting(const std::vector<std::string> &command)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (command.size() <= 2)
    {
        const string correct_cmd_usage{
            R"(
 ^1!config rcon ^5new_rcon_password
 ^1!config private ^5new_private_password
 ^1!config address ^5new_server_address ^3(valid ip:port: ^1123.101.102.53:28960^3)
 ^1!config name ^5new_user_name (for example: ^1config name Admin)
)"};

        print_colored_text(app_handles.hwnd_re_messages_data, correct_cmd_usage.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        return false;
    }

    if (command[0] == "!config")
    {

        if (command[1] == "rcon")
        {
            const string &new_rcon{command[2]};
            if (command[2].empty() || main_app.get_current_game_server().get_rcon_password() == new_rcon)
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3The provided ^1rcon password ^3is too short or it is the same "
                                   "as\n the currently used ^1rcon password^3!\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                return false;
            }
            const string re_msg{"^2You have successfully changed the ^1rcon password ^2to \"^5"s + command[2] +
                                "^2\"\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            main_app.get_current_game_server().set_rcon_password(new_rcon);
            initiate_sending_rcon_status_command_now();
        }
        else if (command[1] == "private")
        {
            const string &new_private_password{command[2]};
            if (command[2].empty() ||
                main_app.get_current_game_server().get_private_slot_password() == new_private_password)
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3The provided ^1private password ^3is too short or it is the "
                                   "same\n as the currently used ^1private password^3!\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                return false;
            }
            const string re_msg{"^2You have successfully changed the ^1private password ^2to \"^5"s + command[2] +
                                "^2\"\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            main_app.get_current_game_server().set_private_slot_password(new_private_password);
        }
        else if (command[1] == "address")
        {
            const size_t colon_pos{command[2].find(':')};
            if (colon_pos == string::npos)
                return false;
            const string ip{command[2].substr(0, colon_pos)};
            const uint_least16_t port{static_cast<uint_least16_t>(stoul(command[2].substr(colon_pos + 1)))};
            unsigned long ip_key{};
            if (!check_ip_address_validity(ip, ip_key) ||
                (ip == main_app.get_current_game_server().get_server_ip_address() &&
                 port == main_app.get_current_game_server().get_server_port()))
                return false;
            const string re_msg{"^2You have successfully changed the ^1game server address ^2to ^5"s + command[2] +
                                "^2\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            main_app.get_current_game_server().set_server_ip_address(ip);
            main_app.get_current_game_server().set_server_port(port);
            initiate_sending_rcon_status_command_now();
        }
        else if (command[1] == "name")
        {
            if (command[2].length() < 3U)
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3The provided ^1user name ^3is too short.\n It "
                                   "has to be at least ^13 characters ^3long!\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                return false;
            }
            const string re_msg{"^2You have successfully changed your ^1user name ^2to ^5"s + command[2] + "^2\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            main_app.set_username(command[2]);
        }
        else
        {
            const string correct_cmd_usage{
                R"(
 ^1!config rcon ^5new_rcon_password
 ^1!config private ^5new_private_password
 ^1!config address ^5new_server_address ^3(valid ip:port: ^1123.101.102.53:28960^3)
 ^1!config name ^5new_user_name (for example: ^1config name Admin)
)"};

            print_colored_text(app_handles.hwnd_re_messages_data, correct_cmd_usage.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            return false;
        }
    }

    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    return true;
}

void log_message(const string &msg, const is_log_datetime is_log_current_date_time)
{
    ostringstream oss;
    if (is_log_current_date_time == is_log_datetime::yes)
    {
        oss << "[" << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}") << "] ";
    }
    oss << msg;
    if (msg.back() != '\n')
    {
        oss << endl;
    }
    // {
    // const string logged_msg{ oss.str() };
    // {
    // lock_guard lg{ log_data_mutex };
    main_app.log_message(oss.str());
    // }
    // }
}

int get_selected_players_pid_number(const int selected_row_in_players_grid, const int selected_col_in_players_grid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_row_in_players_grid,
                                                                  selected_col_in_players_grid))
    {
        string selected_pid_str{GetCellContents(app_handles.hwnd_players_grid, selected_row_in_players_grid, 0)};
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
            selected_pid_str.erase(0, 2);
        if (int pid{}; is_valid_decimal_whole_number(selected_pid_str, pid) && pid >= 0 && pid < max_players_grid_rows)
            return pid;
    }

    return -1;
}

std::string get_player_name_for_pid(const int pid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const auto &players_data = main_app.get_current_game_server().get_players_data();
    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
    {
        if (pid == players_data[i].pid)
            return players_data[i].player_name;
    }

    return {"Unknown player"};
}

player &get_player_data_for_pid(const int pid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static player pd{};
    auto &players_data = main_app.get_current_game_server().get_players_data();
    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
    {
        if (pid == players_data[i].pid)
            return players_data[i];
    }

    pd.pid = -1;
    return pd;
}

std::string get_player_information(const int pid, const bool is_every_property_on_new_line)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    char buffer[512];
    const auto &players_data = main_app.get_current_game_server().get_players_data();
    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
    {
        if (pid == players_data[i].pid)
        {
            const auto &p = players_data[i];
            (void)snprintf(buffer, std::size(buffer),
                           "^3Player name: ^7%s %s ^3PID: ^1%d %s ^3GUID: ^1%s %s ^3Score: ^1%d "
                           "%s ^3Ping: ^1%s\n ^3IP: ^1%s %s ^3Country, city: ^1%s",
                           p.player_name, (is_every_property_on_new_line ? "\n" : "^5|"), p.pid,
                           (is_every_property_on_new_line ? "\n" : "^5|"), (me->is_admin ? p.guid_key : "hidden"),
                           (is_every_property_on_new_line ? "\n" : "^5|"), p.score,
                           (is_every_property_on_new_line ? "\n" : "^5|"), p.ping,
                           (me->is_admin ? p.ip_address.c_str() : "hidden"),
                           (is_every_property_on_new_line ? "\n" : "^5|"),
                           (me->is_admin ? format("{}, {}", p.country_name, p.city).c_str() : p.geo_information));
            string result{buffer};
            if (main_app.get_current_game_server().get_banned_ip_addresses_map().contains(p.ip_address) ||
                main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(p.ip_address) ||
                !p.banned_by_user_name.empty())
            {
                result += is_every_property_on_new_line ? "\n" : " ^5|";
                if (main_app.get_current_game_server().get_banned_ip_addresses_map().contains(p.ip_address))
                {
                    result += format(" ^3Banned by admin: ^7{}", main_app.get_current_game_server()
                                                                     .get_banned_ip_addresses_map()[p.ip_address]
                                                                     .banned_by_user_name);
                }
                else if (main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(p.ip_address))
                {
                    result += format(" ^3Banned by admin: ^7{}", main_app.get_current_game_server()
                                                                     .get_temp_banned_ip_addresses_map()[p.ip_address]
                                                                     .banned_by_user_name);
                }
                else
                {
                    result += format(" ^3Banned by admin: ^7{}", p.banned_by_user_name);
                }
            }
            return result;
        }
    }

    (void)snprintf(buffer, std::size(buffer),
                   "^3Player name: ^5n/a %s ^1PID: ^1%d %s ^3Score: ^5n/a %s "
                   "^3Ping: ^5n/a\n ^3IP: ^5n/a %s ^3Country, city: ^5n/a",
                   (is_every_property_on_new_line ? "\n" : "^3|"), pid, (is_every_property_on_new_line ? "\n" : "^3|"),
                   (is_every_property_on_new_line ? "\n" : "^3|"), (is_every_property_on_new_line ? "\n" : "^3|"));
    return buffer;
}

std::string get_player_information_for_player(player &p)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    char buffer[512];
    if (strlen(p.country_name) == 0 || strcmp(p.country_name, "n/a") == 0)
    {
        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), p.ip_address, p);
    }

    (void)snprintf(buffer, std::size(buffer),
                   "^3Player name: ^7%s ^5| ^3PID: ^1%d ^5| ^3Score: ^1%d ^5| ^3Ping: ^1%s\n "
                   "^3GUID: ^1%s ^5| ^3IP: ^1%s ^5| ^3Country, city: ^1%s, %s",
                   p.player_name, p.pid, p.score, p.ping, (me->is_admin ? p.guid_key : "hidden"),
                   (me->is_admin ? p.ip_address.c_str() : "hidden"), p.country_name, p.city);
    string result{buffer};

    if (main_app.get_current_game_server().get_banned_ip_addresses_map().contains(p.ip_address) ||
        main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(p.ip_address) ||
        !p.banned_by_user_name.empty())
    {
        if (main_app.get_current_game_server().get_banned_ip_addresses_map().contains(p.ip_address))
        {
            result += format(
                " ^5| ^3Banned by admin: ^7{}",
                main_app.get_current_game_server().get_banned_ip_addresses_map()[p.ip_address].banned_by_user_name);
        }
        else if (main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(p.ip_address))
        {
            result += format(" ^5| ^3Banned by admin: ^7{}", main_app.get_current_game_server()
                                                                 .get_temp_banned_ip_addresses_map()[p.ip_address]
                                                                 .banned_by_user_name);
        }
        else
        {
            result += format(" ^5| ^3Banned by admin: ^7{}", p.banned_by_user_name);
        }
    }
    return result;
}

bool specify_reason_for_player_pid(const int pid, const std::string &reason)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    auto &players_data = main_app.get_current_game_server().get_players_data();
    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
    {
        if (pid == players_data[i].pid)
        {
            players_data[i].reason = remove_disallowed_character_in_string(reason);
            return true;
        }
    }

    return false;
}

void build_tiny_rcon_message(std::string &msg)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    for (const auto &[key, value] : main_app.get_tinyrcon_dict())
    {

        for (size_t pos{}; (pos = msg.find(key, pos)) != string::npos; pos += value.length())
        {

            msg.replace(pos, key.length(), value);
        }
    }
}

string get_time_interval_info_string_for_seconds(const time_t seconds)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    constexpr time_t seconds_in_day{3600LL * 24};
    const time_t days = seconds / seconds_in_day;
    const time_t hours = (seconds - days * seconds_in_day) / 3600;
    const time_t minutes = (seconds - (days * seconds_in_day + hours * 3600)) / 60;
    const time_t remaining_seconds = seconds - (days * seconds_in_day + hours * 3600 + minutes * 60);
    ostringstream oss;
    if (days != 0)
    {
        oss << days << (days != 1 ? " days" : " day");
        if (hours != 0 || minutes != 0)
            oss << " : ";
    }

    if (hours != 0)
    {
        oss << hours << (hours != 1 ? " hours" : " hour");
        if (minutes != 0)
            oss << " : ";
    }

    if (minutes != 0)
    {
        oss << minutes << (minutes != 1 ? " minutes" : " minute");
    }

    string result{oss.str()};
    if (!result.empty())
        return result;
    if (remaining_seconds == 0)
        return "just now";
    oss << remaining_seconds << (remaining_seconds != 1 ? " seconds" : " second");
    return oss.str();
}

string get_time_interval_info_string_for_seconds_in_hours_and_minutes(const time_t seconds)
{
    const time_t hours = seconds / 3600;
    const time_t minutes = (seconds - (hours * 3600)) / 60;
    const time_t remaining_seconds = seconds - (hours * 3600 + minutes * 60);
    ostringstream oss;

    if (hours != 0)
    {
        oss << hours << " hrs.";
        if (minutes != 0)
            oss << ' ';
    }

    if (minutes != 0)
    {
        oss << minutes << " min.";
    }

    string result{oss.str()};
    if (!result.empty())
        return result;
    if (remaining_seconds == 0)
        return "just now";
    oss << remaining_seconds << " sec.";
    return oss.str();
}

void change_game_type(const string &game_type, const bool is_reload_map)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const string &full_map_name =
        get_full_map_name(main_app.get_current_game_server().get_current_map(),
                          convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()));
    const auto &full_gametype_names =
        get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());
    if (full_gametype_names.find(game_type) != cend(full_gametype_names))
    {
        char buffer[128]{};
        snprintf(buffer, std::size(buffer), "g_gametype %s", game_type.c_str());
        string reply_buffer;
        main_app.get_connection_manager().send_and_receive_rcon_data(
            buffer, reply_buffer, main_app.get_current_game_server().get_server_ip_address().c_str(),
            main_app.get_current_game_server().get_server_port(),
            main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(), true,
            true);
        main_app.get_current_game_server().set_current_game_type(game_type);
        if (is_reload_map)
        {
            this_thread::sleep_for(std::chrono::milliseconds(50));

            auto &current_user = main_app.get_user_for_name(main_app.get_username());
            current_user->no_of_map_restarts++;
            string public_msg{
                format("^1Admin ^7{} ^7has restarted map ^1{} ^3({}^3)", main_app.get_username(), full_map_name,
                       full_gametype_names.contains(game_type) ? get_full_gametype_name(game_type) : game_type)};
            const string admin_msg{format("{}\\{}", main_app.get_username(), public_msg)};
            main_app.add_message_to_queue(message_t("public-message", admin_msg, true));
            main_app.add_message_to_queue(message_t(
                "add-map_restart",
                format(R"({}\{}\{}\{})", main_app.get_username(), full_map_name,
                       full_gametype_names.contains(game_type) ? get_full_gametype_name(game_type) : game_type,
                       public_msg),
                true));
            rcon_say(public_msg, true);

            main_app.get_connection_manager().send_and_receive_rcon_data(
                "map_restart", reply_buffer, main_app.get_current_game_server().get_server_ip_address().c_str(),
                main_app.get_current_game_server().get_server_port(),
                main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(),
                true, true);
            initiate_sending_rcon_status_command_now();
        }
    }
}

void load_map(const string &rcon_map_name, const string &game_type, const bool is_change_game_type)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const auto &full_gametype_names =
        get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());
    const string &full_map_name = get_full_map_name(
        rcon_map_name, convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()));

    string public_msg{
        is_change_game_type
            ? format("^1Admin ^7{} ^7has changed map to ^1{} ^3({}^3)", main_app.get_username(), full_map_name,
                     full_gametype_names.contains(game_type) ? get_full_gametype_name(game_type) : game_type)
            : format("^7{} ^7has changed map to ^1{}", main_app.get_username(), full_map_name)};
    const string admin_msg{format("{}\\{}", main_app.get_username(), public_msg)};
    main_app.add_message_to_queue(message_t("public-message", admin_msg, true));
    const string &reported_game_type =
        is_change_game_type ? game_type : main_app.get_current_game_server().get_current_game_type();
    main_app.add_message_to_queue(
        message_t("add-map_change",
                  format(R"({}\{}\{}\{})", main_app.get_username(), full_map_name,
                         full_gametype_names.contains(reported_game_type) ? get_full_gametype_name(reported_game_type)
                                                                          : reported_game_type,
                         public_msg),
                  true));
    rcon_say(public_msg, true);

    if (is_change_game_type)
    {
        change_game_type(game_type, false);
    }

    char buffer[128]{};
    snprintf(buffer, std::size(buffer), "map %s", rcon_map_name.c_str());
    string reply;
    main_app.get_connection_manager().send_and_receive_rcon_data(
        buffer, reply, main_app.get_current_game_server().get_server_ip_address().c_str(),
        main_app.get_current_game_server().get_server_port(),
        main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(), true, true);
    main_app.get_current_game_server().set_current_map(rcon_map_name);
    initiate_sending_rcon_status_command_now();
}

bool remove_dir_path_sep_char(char *dir_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const size_t dir_path_len{stl::helper::len(dir_path)};
    if (dir_path_len == 0u)
        return false;
    size_t index{dir_path_len - 1};
    while (index > 0u && (dir_path[index] == '\\' || dir_path[index] == '/'))
    {
        dir_path[index] = '\0';
        --index;
    }

    return index < dir_path_len - 1;
}

bool remove_dir_path_sep_char(wchar_t *dir_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const size_t dir_path_len{stl::helper::len(dir_path)};
    if (dir_path_len == 0u)
        return false;
    size_t index{dir_path_len - 1};
    while (index > 0u && (dir_path[index] == L'\\' || dir_path[index] == L'/'))
    {
        dir_path[index] = L'\0';
        --index;
    }

    return index < dir_path_len - 1;
}

std::string escape_backward_slash_characters_in_place(const std::string &line)
{
    if (line.empty() || line.length() == 1u)
    {
        return !line.empty() && line == "\\" ? "\\\\"s : line;
    }
    string fixed_line;
    fixed_line.reserve(line.length());
    const size_t line_len{line.length()};
    for (size_t i{}; i < line_len; ++i)
    {
        fixed_line.push_back(line[i]);
        if ('\\' == fixed_line.back())
        {
            fixed_line.push_back('\\');
            while (i < line_len && line[i] == '\\')
                ++i;
            --i;
        }
    }

    return fixed_line;
}

void replace_backward_slash_with_forward_slash(std::string &path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    string output;
    output.reserve(path.size());
    size_t start_pos{};

    if (path.starts_with("steam://"))
    {
        start_pos = len("steam://");
        output += "steam://";
    }
    else if (path.starts_with("ftp://"))
    {
        output += "ftp://";
        start_pos = len("ftp://");
    }
    if (path.starts_with("http://"))
    {
        output += "http://";
        start_pos = len("http://");
    }

    char prev{};
    for (size_t i{start_pos}; i < path.length(); ++i)
    {
        if ('\\' == path[i] || '/' == path[i])
        {
            if (prev != '/')
                output.push_back('/');
            prev = '/';
        }
        else
        {
            output.push_back(path[i]);
            prev = path[i];
        }
    }
    path = std::move(output);
}

void replace_backward_slash_with_forward_slash(std::wstring &path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    wstring output;
    output.reserve(path.size());
    size_t start_pos{};

    if (path.starts_with(L"steam://"))
    {
        start_pos = len(L"steam://");
        output += L"steam://";
    }
    else if (path.starts_with(L"ftp://"))
    {
        output += L"ftp://";
        start_pos = len(L"ftp://");
    }
    if (path.starts_with(L"http://"))
    {
        output += L"http://";
        start_pos = len(L"http://");
    }

    wchar_t prev{};
    for (size_t i{start_pos}; i < path.length(); ++i)
    {
        if (L'\\' == path[i] || L'/' == path[i])
        {
            if (prev != L'/')
                output.push_back(L'/');
            prev = L'/';
        }
        else
        {
            output.push_back(path[i]);
            prev = path[i];
        }
    }
    path = std::move(output);
}

void replace_forward_slash_with_backward_slash(std::string &path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (path.starts_with("steam://"))
        return;

    string output;
    output.reserve(path.size());
    char prev{};
    for (size_t i{}; i < path.length(); ++i)
    {
        if ('/' == path[i] || '\\' == path[i])
        {
            if (prev != '\\')
                output.push_back('\\');
            prev = '\\';
        }
        else
        {
            output.push_back(path[i]);
            prev = path[i];
        }
    }
    path = std::move(output);
}

void replace_forward_slash_with_backward_slash(std::wstring &path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (path.starts_with(L"steam://"))
        return;

    wstring output;
    output.reserve(path.size());
    wchar_t prev{};
    for (size_t i{}; i < path.length(); ++i)
    {
        if (L'/' == path[i] || L'\\' == path[i])
        {
            if (prev != L'\\')
                output.push_back(L'\\');
            prev = L'\\';
        }
        else
        {
            output.push_back(path[i]);
            prev = path[i];
        }
    }
    path = std::move(output);
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    unused(lParam);
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    if (uMsg == BFFM_INITIALIZED)
    {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        SendMessage(hwnd, BFFM_SETEXPANDED, TRUE, lpData);
        SendMessage(hwnd, BFFM_SETSTATUSTEXT, NULL, lpData);
        ShowWindow(hwnd, SW_NORMAL);
        SetFocus(hwnd);
        SetForegroundWindow(hwnd);
    }

    return 0;
}

const char *BrowseFolder(const char *saved_path, const char *user_info)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    constexpr size_t max_path_length{32768};
    static char path[max_path_length];

    BROWSEINFO bi{};
    bi.lpszTitle = user_info;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = reinterpret_cast<LPARAM>(saved_path);

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != nullptr)
    {
        // get the name of the folder and put it in path
        SHGetPathFromIDList(pidl, path);

        // free memory used
        IMalloc *imalloc = nullptr;
        if (SUCCEEDED(SHGetMalloc(&imalloc)))
        {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        return path;
    }

    path[0] = '\0';
    return path;
}

const char *find_call_of_duty_1_installation_path(const bool is_show_browse_folder_dialog)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    constexpr size_t max_path_length{32768};
    static char install_path[max_path_length]{};
    static char exe_file_path[max_path_length]{};
    static const char *def_cod1_registry_location_subkeys[] = {
        "SOFTWARE\\Activision\\Call of Duty", R"(SOFTWARE\Wow6432Node\Activision\Call of Duty)",
        "SOFTWARE\\Activision\\Call of Duty(R)", R"(SOFTWARE\WOW6432Node\Activision\Call of Duty(R))", nullptr};
    const char *game_install_path_key = "InstallPath";
    DWORD cch{};
    HKEY game_installation_reg_key{};
    bool found{};

    const char *found_reg_location{};
    const char **def_game_reg_key{def_cod1_registry_location_subkeys};

    const string cod1_steam_appid_string{format("-applaunch {}", cod1_steam_appid)};

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    DWORD is_steam_game_installed{};

    LRESULT status =
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, format(R"(SOFTWARE\Valve\Steam\Apps\{})", cod1_steam_appid).c_str(), 0,
                     KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {
        cch = sizeof(is_steam_game_installed);
        status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

        if (status == ERROR_SUCCESS && is_steam_game_installed == 1)
        {
            HKEY steam_installation_key{};
            status =
                RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
            if (status == ERROR_SUCCESS)
            {
                cch = sizeof(install_path);
                status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr,
                                         reinterpret_cast<LPBYTE>(install_path), &cch);

                if (status == ERROR_SUCCESS)
                {

                    string steam_exe_path{install_path};
                    replace_forward_slash_with_backward_slash(steam_exe_path);
                    if (check_if_file_path_exists(steam_exe_path.c_str()))
                    {
                        string cod_mp_exe_path1{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                "\\steamapps\\common\\Call of Duty\\codmp.exe"s};
                        string cod_mp_exe_path2{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                "\\steamapps\\common\\Call of Duty(R)\\codmp.exe"s};
                        // to_lower_case_in_place(cod_mp_exe_path1);
                        // to_lower_case_in_place(cod_mp_exe_path2);

                        if (check_if_file_path_exists(cod_mp_exe_path1.c_str()) ||
                            check_if_file_path_exists(cod_mp_exe_path2.c_str()))
                        {
                            if (check_if_file_path_exists(cod_mp_exe_path1.c_str()))
                            {
                                // (void)snprintf(exe_file_path, max_path_length, "\"%s\"
                                // -applaunch %ld", steam_exe_path.c_str(), cod1_steam_appid);
                                (void)snprintf(exe_file_path, max_path_length, "%s", cod_mp_exe_path1.c_str());
                            }
                            else if (check_if_file_path_exists(cod_mp_exe_path2.c_str()))
                            {
                                (void)snprintf(exe_file_path, max_path_length, "%s", cod_mp_exe_path2.c_str());
                            }

                            found = true;
                            char buffer[1024];
                            (void)snprintf(buffer, std::size(buffer),
                                           "^2Successfully built your ^3Call of Duty (^4Steam version^3) "
                                           "^2game's launch command: ^5%s\n",
                                           exe_file_path);
                            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                            *def_game_reg_key = nullptr;
                        }
                    }
                }

                RegCloseKey(steam_installation_key);
            }
        }

        RegCloseKey(game_installation_reg_key);
    }

    if (!found)
    {
        ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
        is_steam_game_installed = 0;
        cch = sizeof(is_steam_game_installed);

        status = RegOpenKeyEx(HKEY_CURRENT_USER, format(R"(SOFTWARE\Valve\Steam\Apps\{})", cod1_steam_appid).c_str(), 0,
                              KEY_QUERY_VALUE, &game_installation_reg_key);

        if (status == ERROR_SUCCESS)
        {

            status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

            if (status == ERROR_SUCCESS && is_steam_game_installed == 1)
            {
                HKEY steam_installation_key{};
                status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE,
                                      &steam_installation_key);
                if (status == ERROR_SUCCESS)
                {
                    cch = sizeof(install_path);
                    status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr,
                                             reinterpret_cast<LPBYTE>(install_path), &cch);

                    if (status == ERROR_SUCCESS)
                    {

                        string steam_exe_path{install_path};
                        replace_forward_slash_with_backward_slash(steam_exe_path);
                        if (check_if_file_path_exists(steam_exe_path.c_str()))
                        {
                            string cod_mp_exe_path1{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                    "\\steamapps\\common\\Call of Duty\\codmp.exe"s};
                            string cod_mp_exe_path2{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                    "\\steamapps\\common\\Call of Duty(R)\\codmp.exe"s};
                            // to_lower_case_in_place(cod_mp_exe_path1);
                            // to_lower_case_in_place(cod_mp_exe_path2);

                            if (check_if_file_path_exists(cod_mp_exe_path1.c_str()) ||
                                check_if_file_path_exists(cod_mp_exe_path2.c_str()))
                            {
                                if (check_if_file_path_exists(cod_mp_exe_path1.c_str()))
                                {
                                    // (void)snprintf(exe_file_path, max_path_length, "\"%s\"
                                    // -applaunch %ld", steam_exe_path.c_str(), cod2_steam_appid);
                                    (void)snprintf(exe_file_path, max_path_length, "%s", cod_mp_exe_path1.c_str());
                                }
                                else if (check_if_file_path_exists(cod_mp_exe_path2.c_str()))
                                {
                                    (void)snprintf(exe_file_path, max_path_length, "%s", cod_mp_exe_path2.c_str());
                                }

                                found = true;
                                char buffer[1024];
                                (void)snprintf(buffer, std::size(buffer),
                                               "^2Successfully built your ^3Call of Duty (^4Steam "
                                               "version^3) ^2game's launch command: ^5%s\n",
                                               exe_file_path);
                                print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                                *def_game_reg_key = nullptr;
                            }
                        }
                    }

                    RegCloseKey(steam_installation_key);
                }
            }

            RegCloseKey(game_installation_reg_key);
        }
    }

    while (!found && *def_game_reg_key)
    {

        ZeroMemory(&game_installation_reg_key, sizeof(HKEY));

        cch = sizeof(install_path);

        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

        if (status == ERROR_SUCCESS)
        {

            status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(install_path), &cch);

            if (status == ERROR_SUCCESS)
            {

                WIN32_FIND_DATA find_data{};

                remove_dir_path_sep_char(install_path);

                (void)snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", install_path);

                HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

                if (search_handle != INVALID_HANDLE_VALUE)
                {

                    found = true;
                    found_reg_location = *def_game_reg_key;
                    char buffer[1024];
                    (void)snprintf(buffer, std::size(buffer),
                                   "^2Successfully built your ^3Call of Duty ^2game's "
                                   "launch command: ^5%s\n",
                                   exe_file_path);
                    print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                    *def_game_reg_key = nullptr;
                    break;
                }
            }

            RegCloseKey(game_installation_reg_key);
        }

        def_game_reg_key++;
    }

    if (!found)
    {

        def_game_reg_key = def_cod1_registry_location_subkeys;

        while (!found && *def_game_reg_key)
        {

            ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
            cch = sizeof(install_path);

            status = RegOpenKeyEx(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

            if (status == ERROR_SUCCESS)
            {

                status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr,
                                         reinterpret_cast<LPBYTE>(install_path), &cch);

                if (status == ERROR_SUCCESS)
                {

                    WIN32_FIND_DATA find_data{};

                    remove_dir_path_sep_char(install_path);

                    (void)snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", install_path);

                    HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

                    if (search_handle != INVALID_HANDLE_VALUE)
                    {

                        found = true;
                        found_reg_location = *def_game_reg_key;
                        char buffer[1024];
                        (void)snprintf(buffer, std::size(buffer),
                                       "^2Successfully built your ^3Call of Duty ^2game's "
                                       "launch command: ^5%s\n",
                                       exe_file_path);
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

    if (!found && is_show_browse_folder_dialog)
    {

        str_copy(install_path, "C:\\");
        found_reg_location = nullptr;

        static char msgbuff[1024];
        (void)snprintf(msgbuff, std::size(msgbuff),
                       "Please, select your Call of Duty 1 game installation "
                       "folder and click OK.");

        const char *cod1_game_path = BrowseFolder(install_path, msgbuff);

        if (lstrcmp(cod1_game_path, "") != 0 && lstrcmp(cod1_game_path, "C:\\") != 0)
        {
            (void)snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", cod1_game_path);
        }
    }

    if (!str_contains(exe_file_path, cod1_steam_appid_string.c_str(), 0U, true))
    {
        string exe_path{exe_file_path};
        replace_forward_slash_with_backward_slash(exe_path);
        str_copy(exe_file_path, exe_path.c_str());
    }

    if (!check_if_file_path_exists(exe_file_path) &&
        !str_contains(exe_file_path, cod1_steam_appid_string.c_str(), 0U, true))
    {
        main_app.set_codmp_exe_path("");
    }
    else
    {
        main_app.set_codmp_exe_path(exe_file_path);
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    }

    return exe_file_path;
}

bool check_if_call_of_duty_1_game_is_running(DWORD &pid)
{
    DWORD processes[1024]{}, processes_size{};
    char buffer[512];
    pid = 0;

    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    if (EnumProcesses(processes, sizeof(processes), &processes_size))
        for (size_t i{}; i < processes_size; ++i)
        {
            if (processes[i] == 0)
                continue;
            auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
            const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));
            CloseHandle(ph);
            if (n == 0)
                continue;
            const string module_base_name{buffer};
            if (str_index_of(module_base_name, "codmp.exe", 0, true) != string::npos)
            {
                pid = processes[i];
                return true;
            }
        }

    DWORD cch{};
    HKEY game_installation_reg_key;

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    DWORD is_steam_game_running{};
    cch = sizeof(is_steam_game_running);

    auto status =
        RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {

        status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

        if (status == ERROR_SUCCESS && cod1_steam_appid == is_steam_game_running)
            return true;
    }

    RegCloseKey(game_installation_reg_key);

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    is_steam_game_running = 0;
    cch = sizeof(is_steam_game_running);

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {

        status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

        if (status == ERROR_SUCCESS && cod1_steam_appid == is_steam_game_running)
            return true;
    }

    RegCloseKey(game_installation_reg_key);

    return false;
}

const char *find_call_of_duty_2_installation_path(const bool is_show_browse_folder_dialog)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    constexpr size_t max_path_length{32768};
    static char install_path[max_path_length]{};
    static char exe_file_path[max_path_length]{};
    static const char *def_cod2_registry_location_subkeys[] = {
        "SOFTWARE\\Activision\\Call of Duty 2", R"(SOFTWARE\WOW6432Node\Activision\Call of Duty 2)",
        "SOFTWARE\\Activision\\Call of Duty(R) 2", R"(SOFTWARE\WOW6432Node\Activision\Call of Duty(R) 2)", nullptr};
    const char *game_install_path_key = "InstallPath";
    DWORD cch{};
    HKEY game_installation_reg_key{};
    bool found{};

    const char *found_reg_location{};
    const char **def_game_reg_key{def_cod2_registry_location_subkeys};

    const string cod2_steam_appid_string{format("-applaunch {}", cod2_steam_appid)};

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    DWORD is_steam_game_installed{};

    LRESULT status =
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, format(R"(SOFTWARE\Valve\Steam\Apps\{})", cod2_steam_appid).c_str(), 0,
                     KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {
        cch = sizeof(is_steam_game_installed);
        status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

        if (status == ERROR_SUCCESS && is_steam_game_installed == 1)
        {
            HKEY steam_installation_key{};
            status =
                RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
            if (status == ERROR_SUCCESS)
            {
                cch = sizeof(install_path);
                status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr,
                                         reinterpret_cast<LPBYTE>(install_path), &cch);

                if (status == ERROR_SUCCESS)
                {

                    string steam_exe_path{install_path};
                    replace_forward_slash_with_backward_slash(steam_exe_path);
                    if (check_if_file_path_exists(steam_exe_path.c_str()))
                    {
                        string cod2_mp_exe_path1{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                 "\\steamapps\\common\\Call of Duty 2\\CoD2MP_s.exe"s};
                        string cod2_mp_exe_path2{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                 "\\steamapps\\common\\Call of Duty(R) 2\\CoD2MP_s.exe"s};
                        // to_lower_case_in_place(cod2_mp_exe_path1);
                        // to_lower_case_in_place(cod2_mp_exe_path2);

                        if (check_if_file_path_exists(cod2_mp_exe_path1.c_str()) ||
                            check_if_file_path_exists(cod2_mp_exe_path2.c_str()))
                        {
                            if (check_if_file_path_exists(cod2_mp_exe_path1.c_str()))
                            {
                                // (void)snprintf(exe_file_path, max_path_length, "\"%s\"
                                // -applaunch %ld", steam_exe_path.c_str(), cod2_steam_appid);
                                (void)snprintf(exe_file_path, max_path_length, "%s", cod2_mp_exe_path1.c_str());
                            }
                            else if (check_if_file_path_exists(cod2_mp_exe_path2.c_str()))
                            {
                                (void)snprintf(exe_file_path, max_path_length, "%s", cod2_mp_exe_path1.c_str());
                            }

                            found = true;
                            char buffer[1024];
                            (void)snprintf(buffer, std::size(buffer),
                                           "^2Successfully built your ^3Call of Duty 2 "
                                           "(^4Steam^3) ^2game's launch command:\n '^5%s'\n",
                                           exe_file_path);
                            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                            *def_game_reg_key = nullptr;
                        }
                    }
                }

                RegCloseKey(steam_installation_key);
            }
        }

        RegCloseKey(game_installation_reg_key);
    }

    if (!found)
    {
        ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
        is_steam_game_installed = 0;
        cch = sizeof(is_steam_game_installed);

        status = RegOpenKeyEx(HKEY_CURRENT_USER, format(R"(SOFTWARE\Valve\Steam\Apps\{})", cod2_steam_appid).c_str(), 0,
                              KEY_QUERY_VALUE, &game_installation_reg_key);

        if (status == ERROR_SUCCESS)
        {

            status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

            if (status == ERROR_SUCCESS && is_steam_game_installed == 1)
            {
                HKEY steam_installation_key{};
                status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE,
                                      &steam_installation_key);
                if (status == ERROR_SUCCESS)
                {
                    cch = sizeof(install_path);
                    status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr,
                                             reinterpret_cast<LPBYTE>(install_path), &cch);

                    if (status == ERROR_SUCCESS)
                    {

                        string steam_exe_path{install_path};
                        replace_forward_slash_with_backward_slash(steam_exe_path);
                        if (check_if_file_path_exists(steam_exe_path.c_str()))
                        {
                            string cod2_mp_exe_path1{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                     "\\steamapps\\common\\Call of Duty 2\\CoD2MP_s.exe"s};
                            string cod2_mp_exe_path2{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                     "\\steamapps\\common\\Call of Duty(R) 2\\CoD2MP_s.exe"s};
                            // to_lower_case_in_place(cod2_mp_exe_path1);
                            // to_lower_case_in_place(cod2_mp_exe_path2);

                            if (check_if_file_path_exists(cod2_mp_exe_path1.c_str()) ||
                                check_if_file_path_exists(cod2_mp_exe_path2.c_str()))
                            {
                                if (check_if_file_path_exists(cod2_mp_exe_path1.c_str()))
                                {
                                    // (void)snprintf(exe_file_path, max_path_length, "\"%s\"
                                    // -applaunch %ld", steam_exe_path.c_str(), cod2_steam_appid);
                                    (void)snprintf(exe_file_path, max_path_length, "%s", cod2_mp_exe_path1.c_str());
                                }
                                else if (check_if_file_path_exists(cod2_mp_exe_path2.c_str()))
                                {
                                    (void)snprintf(exe_file_path, max_path_length, "%s", cod2_mp_exe_path1.c_str());
                                }

                                found = true;
                                char buffer[1024];
                                (void)snprintf(buffer, std::size(buffer),
                                               "^2Successfully built your ^3Call of Duty 2 (^4Steam^3) "
                                               "^2game's launch command:\n '^5%s'\n",
                                               exe_file_path);
                                print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                                *def_game_reg_key = nullptr;
                            }
                        }
                    }

                    RegCloseKey(steam_installation_key);
                }
            }

            RegCloseKey(game_installation_reg_key);
        }
    }

    while (!found && *def_game_reg_key)
    {

        ZeroMemory(&game_installation_reg_key, sizeof(HKEY));

        cch = sizeof(install_path);

        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

        if (status == ERROR_SUCCESS)
        {

            status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(install_path), &cch);

            if (status == ERROR_SUCCESS)
            {
                WIN32_FIND_DATA find_data{};
                remove_dir_path_sep_char(install_path);
                (void)snprintf(exe_file_path, max_path_length, "%s\\CoD2MP_s.exe", install_path);

                HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

                if (search_handle != INVALID_HANDLE_VALUE)
                {

                    found = true;
                    found_reg_location = *def_game_reg_key;
                    char buffer[1024];
                    (void)snprintf(buffer, std::size(buffer),
                                   "^2Successfully built your ^3Call of Duty 2 ^2game's "
                                   "launch command: ^5%s\n",
                                   exe_file_path);
                    print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                    *def_game_reg_key = nullptr;
                    break;
                }
            }

            RegCloseKey(game_installation_reg_key);
        }

        ++def_game_reg_key;
    }

    if (!found)
    {

        def_game_reg_key = def_cod2_registry_location_subkeys;

        while (!found && *def_game_reg_key)
        {

            ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
            cch = sizeof(install_path);

            status = RegOpenKeyEx(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

            if (status == ERROR_SUCCESS)
            {

                status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr,
                                         reinterpret_cast<LPBYTE>(install_path), &cch);

                if (status == ERROR_SUCCESS)
                {

                    WIN32_FIND_DATA find_data{};
                    remove_dir_path_sep_char(install_path);
                    (void)snprintf(exe_file_path, max_path_length, "%s\\CoD2MP_s.exe", install_path);
                    HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

                    if (search_handle != INVALID_HANDLE_VALUE)
                    {

                        found = true;
                        found_reg_location = *def_game_reg_key;
                        char buffer[1024];
                        (void)snprintf(buffer, std::size(buffer),
                                       "^2Successfully built your ^3Call of Duty 2 "
                                       "^2game's launch command: ^5%s\n",
                                       exe_file_path);
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

    if (!found && is_show_browse_folder_dialog)
    {

        str_copy(install_path, "C:\\");
        found_reg_location = nullptr;

        static char msgbuff[1024];
        (void)snprintf(msgbuff, std::size(msgbuff),
                       "Please, select your Call of Duty 2 game installation "
                       "folder and click OK.");

        const char *cod2_game_path = BrowseFolder(install_path, msgbuff);

        if (lstrcmp(cod2_game_path, "") != 0 && lstrcmp(cod2_game_path, "C:\\") != 0)
        {
            (void)snprintf(exe_file_path, max_path_length, "%s\\CoD2MP_s.exe", cod2_game_path);
        }
    }

    if (!str_contains(exe_file_path, cod2_steam_appid_string.c_str(), 0U, true))
    {
        string exe_path{exe_file_path};
        replace_forward_slash_with_backward_slash(exe_path);
        str_copy(exe_file_path, exe_path.c_str());
    }

    if (!check_if_file_path_exists(exe_file_path) &&
        !str_contains(exe_file_path, cod2_steam_appid_string.c_str(), 0U, true))
    {
        main_app.set_cod2mp_exe_path("");
    }
    else
    {
        main_app.set_cod2mp_exe_path(exe_file_path);
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    }

    return exe_file_path;
}

bool check_if_call_of_duty_2_game_is_running(DWORD &pid)
{
    DWORD processes[1024]{}, processes_size{};
    char buffer[512];
    pid = 0;
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    if (EnumProcesses(processes, sizeof(processes), &processes_size))
        for (size_t i{}; i < processes_size; ++i)
        {
            if (processes[i] == 0)
                continue;
            auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
            const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));
            CloseHandle(ph);
            if (n == 0)
                continue;
            const string module_base_name{buffer};
            if (str_index_of(module_base_name, "cod2mp_s.exe", 0, true) != string::npos)
            {
                pid = processes[i];
                return true;
            }
        }

    DWORD cch{};
    HKEY game_installation_reg_key;

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    DWORD is_steam_game_running{};
    cch = sizeof(is_steam_game_running);

    auto status =
        RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {

        status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

        if (status == ERROR_SUCCESS && cod2_steam_appid == is_steam_game_running)
            return true;
    }

    RegCloseKey(game_installation_reg_key);

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    is_steam_game_running = 0;
    cch = sizeof(is_steam_game_running);

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {

        status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

        if (status == ERROR_SUCCESS && cod2_steam_appid == is_steam_game_running)
            return true;
    }

    RegCloseKey(game_installation_reg_key);

    return false;
}

const char *find_call_of_duty_4_installation_path(const bool is_show_browse_folder_dialog)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    constexpr size_t max_path_length{32768};
    static char install_path[max_path_length]{};
    static char exe_file_path[max_path_length]{};
    static const char *def_cod4_registry_location_subkeys[] = {
        "SOFTWARE\\Activision\\Call of Duty 4", R"(SOFTWARE\Wow6432Node\Activision\Call of Duty 4)",
        "SOFTWARE\\Activision\\Call of Duty(R) 4", R"(SOFTWARE\WOW6432Node\Activision\Call of Duty(R) 4)", nullptr};
    const char *game_install_path_key = "InstallPath";
    DWORD cch{};
    HKEY game_installation_reg_key{};
    bool found{};

    const char *found_reg_location{};
    const char **def_game_reg_key{def_cod4_registry_location_subkeys};

    const string cod4_steam_appid_string{format("-applaunch {}", cod4_steam_appid)};
    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    DWORD is_steam_game_installed{};

    LRESULT status =
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, format(R"(SOFTWARE\Valve\Steam\Apps\{})", cod4_steam_appid).c_str(), 0,
                     KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {
        cch = sizeof(is_steam_game_installed);
        status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

        if (status == ERROR_SUCCESS && is_steam_game_installed == 1)
        {
            HKEY steam_installation_key{};
            status =
                RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
            if (status == ERROR_SUCCESS)
            {
                cch = sizeof(install_path);
                status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr,
                                         reinterpret_cast<LPBYTE>(install_path), &cch);

                if (status == ERROR_SUCCESS)
                {

                    string steam_exe_path{install_path};
                    replace_forward_slash_with_backward_slash(steam_exe_path);
                    if (check_if_file_path_exists(steam_exe_path.c_str()))
                    {
                        string iw3mp_path1{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                           "\\steamapps\\common\\Call of Duty 4\\iw3mp.exe"s};
                        string iw3mp_path2{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                           "\\steamapps\\common\\Call of Duty(R) 4\\iw3mp.exe"s};
                        // to_lower_case_in_place(iw3mp_path1);
                        // to_lower_case_in_place(iw3mp_path2);

                        if (check_if_file_path_exists(iw3mp_path1.c_str()) ||
                            check_if_file_path_exists(iw3mp_path2.c_str()))
                        {
                            if (check_if_file_path_exists(iw3mp_path1.c_str()))
                            {
                                // (void)snprintf(exe_file_path, max_path_length, "\"%s\"
                                // -applaunch %ld", steam_exe_path.c_str(), cod2_steam_appid);
                                (void)snprintf(exe_file_path, max_path_length, "%s", iw3mp_path1.c_str());
                            }
                            else if (check_if_file_path_exists(iw3mp_path2.c_str()))
                            {
                                (void)snprintf(exe_file_path, max_path_length, "%s", iw3mp_path2.c_str());
                            }

                            found = true;
                            char buffer[1024];
                            (void)snprintf(buffer, std::size(buffer),
                                           "^2Successfully built your ^3Call of Duty 4: Modern Warfare "
                                           "(^4Steam version^3) ^2game's launch command: ^5%s\n",
                                           exe_file_path);
                            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                            *def_game_reg_key = nullptr;
                        }
                    }
                }

                RegCloseKey(steam_installation_key);
            }
        }

        RegCloseKey(game_installation_reg_key);
    }

    if (!found)
    {
        ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
        is_steam_game_installed = 0;
        cch = sizeof(is_steam_game_installed);

        status = RegOpenKeyEx(HKEY_CURRENT_USER, format(R"(SOFTWARE\Valve\Steam\Apps\{})", cod4_steam_appid).c_str(), 0,
                              KEY_QUERY_VALUE, &game_installation_reg_key);

        if (status == ERROR_SUCCESS)
        {

            status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

            if (status == ERROR_SUCCESS && is_steam_game_installed == 1)
            {
                HKEY steam_installation_key{};
                status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE,
                                      &steam_installation_key);
                if (status == ERROR_SUCCESS)
                {
                    cch = sizeof(install_path);
                    status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr,
                                             reinterpret_cast<LPBYTE>(install_path), &cch);

                    if (status == ERROR_SUCCESS)
                    {

                        string steam_exe_path{install_path};
                        replace_forward_slash_with_backward_slash(steam_exe_path);
                        if (check_if_file_path_exists(steam_exe_path.c_str()))
                        {
                            string iw3mp_path1{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                               "\\steamapps\\common\\Call of Duty 4\\iw3mp.exe"s};
                            string iw3mp_path2{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                               "\\steamapps\\common\\Call of Duty(R) 4\\iw3mp.exe"s};
                            // to_lower_case_in_place(iw3mp_path1);
                            // to_lower_case_in_place(iw3mp_path2);

                            if (check_if_file_path_exists(iw3mp_path1.c_str()) ||
                                check_if_file_path_exists(iw3mp_path2.c_str()))
                            {
                                if (check_if_file_path_exists(iw3mp_path1.c_str()))
                                {
                                    // (void)snprintf(exe_file_path, max_path_length, "\"%s\"
                                    // -applaunch %ld", steam_exe_path.c_str(), cod2_steam_appid);
                                    (void)snprintf(exe_file_path, max_path_length, "%s", iw3mp_path1.c_str());
                                }
                                else if (check_if_file_path_exists(iw3mp_path2.c_str()))
                                {
                                    (void)snprintf(exe_file_path, max_path_length, "%s", iw3mp_path2.c_str());
                                }

                                found = true;
                                char buffer[1024];
                                (void)snprintf(buffer, std::size(buffer),
                                               "^2Successfully built your ^3Call of Duty 4: Modern Warfare "
                                               "(^4Steam version^3) ^2game's launch command: ^5%s\n",
                                               exe_file_path);
                                print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                                *def_game_reg_key = nullptr;
                            }
                        }
                    }

                    RegCloseKey(steam_installation_key);
                }
            }

            RegCloseKey(game_installation_reg_key);
        }
    }

    while (!found && *def_game_reg_key)
    {

        ZeroMemory(&game_installation_reg_key, sizeof(HKEY));

        cch = sizeof(install_path);

        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

        if (status == ERROR_SUCCESS)
        {

            status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(install_path), &cch);

            if (status == ERROR_SUCCESS)
            {
                WIN32_FIND_DATA find_data{};
                remove_dir_path_sep_char(install_path);
                (void)snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", install_path);
                HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

                if (search_handle != INVALID_HANDLE_VALUE)
                {

                    found = true;
                    found_reg_location = *def_game_reg_key;
                    char buffer[1024];
                    (void)snprintf(buffer, std::size(buffer),
                                   "^2Successfully built your ^3Call of Duty 4: Modern "
                                   "Warfare ^2game's launch command: ^5%s\n",
                                   exe_file_path);
                    print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                    *def_game_reg_key = nullptr;
                    break;
                }
            }

            RegCloseKey(game_installation_reg_key);
        }

        def_game_reg_key++;
    }

    if (!found)
    {

        def_game_reg_key = def_cod4_registry_location_subkeys;

        while (!found && *def_game_reg_key)
        {

            ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
            cch = sizeof(install_path);

            status = RegOpenKeyEx(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

            if (status == ERROR_SUCCESS)
            {

                status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr,
                                         reinterpret_cast<LPBYTE>(install_path), &cch);

                if (status == ERROR_SUCCESS)
                {
                    WIN32_FIND_DATA find_data{};
                    remove_dir_path_sep_char(install_path);
                    (void)snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", install_path);
                    HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

                    if (search_handle != INVALID_HANDLE_VALUE)
                    {

                        found = true;
                        found_reg_location = *def_game_reg_key;
                        char buffer[1024];
                        (void)snprintf(buffer, std::size(buffer),
                                       "^2Successfully built your ^3Call of Duty 4: Modern "
                                       "Warfare ^2game's launch command: ^5%s\n",
                                       exe_file_path);
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

    if (!found && is_show_browse_folder_dialog)
    {

        str_copy(install_path, "C:\\");
        found_reg_location = nullptr;

        static char msgbuff[1024];
        (void)snprintf(msgbuff, std::size(msgbuff),
                       "Please, select your Call of Duty 4 game installation "
                       "folder and click OK.");

        const char *cod4_game_path = BrowseFolder(install_path, msgbuff);

        if (lstrcmp(cod4_game_path, "") != 0 && lstrcmp(cod4_game_path, "C:\\") != 0)
        {
            (void)snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", cod4_game_path);
        }
    }

    if (!str_contains(exe_file_path, cod4_steam_appid_string.c_str(), 0U, true))
    {
        string exe_path{exe_file_path};
        replace_forward_slash_with_backward_slash(exe_path);
        str_copy(exe_file_path, exe_path.c_str());
    }

    if (!check_if_file_path_exists(exe_file_path) &&
        !str_contains(exe_file_path, cod4_steam_appid_string.c_str(), 0U, true))
    {
        main_app.set_iw3mp_exe_path("");
    }
    else
    {
        main_app.set_iw3mp_exe_path(exe_file_path);
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    }

    return exe_file_path;
}

bool check_if_call_of_duty_4_game_is_running(DWORD &pid)
{
    DWORD processes[1024]{}, processes_size{};
    char buffer[512];
    pid = 0;
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    if (EnumProcesses(processes, sizeof(processes), &processes_size))
        for (size_t i{}; i < processes_size; ++i)
        {
            if (processes[i] == 0)
                continue;
            auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
            const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));
            CloseHandle(ph);
            if (n == 0)
                continue;
            const string module_base_name{buffer};
            if (str_index_of(module_base_name, "iw3mp.exe", 0, true) != string::npos)
            {
                pid = processes[i];
                return true;
            }
        }

    DWORD cch{};
    HKEY game_installation_reg_key;

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    DWORD is_steam_game_running{};
    cch = sizeof(is_steam_game_running);

    auto status =
        RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {

        status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

        if (status == ERROR_SUCCESS && cod4_steam_appid == is_steam_game_running)
            return true;
    }

    RegCloseKey(game_installation_reg_key);

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    is_steam_game_running = 0;
    cch = sizeof(is_steam_game_running);

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {

        status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

        if (status == ERROR_SUCCESS && cod4_steam_appid == is_steam_game_running)
            return true;
    }

    RegCloseKey(game_installation_reg_key);

    return false;
}

const char *find_call_of_duty_5_installation_path(const bool is_show_browse_folder_dialog)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    constexpr size_t max_path_length{32768};
    static char install_path[max_path_length]{};
    static char exe_file_path[max_path_length]{};
    static const char *def_cod5_registry_location_subkeys[] = {
        "SOFTWARE\\Activision\\Call of Duty WAW", R"(SOFTWARE\Wow6432Node\Activision\Call of Duty WAW)",
        "SOFTWARE\\Activision\\Call of Duty(R) WAW", R"(SOFTWARE\WOW6432Node\Activision\Call of Duty(R) WAW)", nullptr};
    const char *game_install_path_key = "InstallPath";
    DWORD cch{};
    HKEY game_installation_reg_key{};
    bool found{};

    const char *found_reg_location{};
    const char **def_game_reg_key{def_cod5_registry_location_subkeys};

    const string cod5_steam_appid_string{format("-applaunch {}", cod5_steam_appid)};
    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    DWORD is_steam_game_installed{};

    LRESULT status =
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, format(R"(SOFTWARE\Valve\Steam\Apps\{})", cod5_steam_appid).c_str(), 0,
                     KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {
        cch = sizeof(is_steam_game_installed);
        status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

        if (status == ERROR_SUCCESS && is_steam_game_installed == 1)
        {
            HKEY steam_installation_key{};
            status =
                RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &steam_installation_key);
            if (status == ERROR_SUCCESS)
            {
                cch = sizeof(install_path);
                status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr,
                                         reinterpret_cast<LPBYTE>(install_path), &cch);

                if (status == ERROR_SUCCESS)
                {

                    string steam_exe_path{install_path};
                    replace_forward_slash_with_backward_slash(steam_exe_path);
                    if (check_if_file_path_exists(steam_exe_path.c_str()))
                    {
                        string cod5mp_path1{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                            "\\steamapps\\common\\Call of Duty World at War\\cod5mp.exe"s};
                        string cod5mp_path2{steam_exe_path.substr(0, steam_exe_path.rfind(L'\\')) +
                                            "\\steamapps\\common\\Call of Duty(R) World at War\\cod5mp.exe"s};
                        // to_lower_case_in_place(cod5mp_path1);
                        // to_lower_case_in_place(cod5mp_path2);

                        if (check_if_file_path_exists(cod5mp_path1.c_str()) ||
                            check_if_file_path_exists(cod5mp_path2.c_str()))
                        {
                            if (check_if_file_path_exists(cod5mp_path1.c_str()))
                            {
                                // (void)snprintf(exe_file_path, max_path_length, "\"%s\"
                                // -applaunch %ld", steam_exe_path.c_str(), cod2_steam_appid);
                                (void)snprintf(exe_file_path, max_path_length, "%s", cod5mp_path1.c_str());
                            }
                            else if (check_if_file_path_exists(cod5mp_path2.c_str()))
                            {
                                (void)snprintf(exe_file_path, max_path_length, "%s", cod5mp_path2.c_str());
                            }

                            found = true;
                            char buffer[1024];
                            (void)snprintf(buffer, std::size(buffer),
                                           "^2Successfully built your ^3Call of Duty 5: World at War "
                                           "(^4Steam version^3) ^2game's launch command: ^5%s\n",
                                           exe_file_path);
                            print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                            *def_game_reg_key = nullptr;
                        }
                    }
                }

                RegCloseKey(steam_installation_key);
            }
        }

        RegCloseKey(game_installation_reg_key);
    }

    if (!found)
    {
        ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
        is_steam_game_installed = 0;
        cch = sizeof(is_steam_game_installed);

        status = RegOpenKeyEx(HKEY_CURRENT_USER, format(R"(SOFTWARE\Valve\Steam\Apps\{})", cod5_steam_appid).c_str(), 0,
                              KEY_QUERY_VALUE, &game_installation_reg_key);

        if (status == ERROR_SUCCESS)
        {

            status = RegQueryValueEx(game_installation_reg_key, "Installed", nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(&is_steam_game_installed), &cch);

            if (status == ERROR_SUCCESS && is_steam_game_installed == 1)
            {
                HKEY steam_installation_key{};
                status = RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE,
                                      &steam_installation_key);
                if (status == ERROR_SUCCESS)
                {
                    cch = sizeof(install_path);
                    status = RegQueryValueEx(steam_installation_key, "SteamExe", nullptr, nullptr,
                                             reinterpret_cast<LPBYTE>(install_path), &cch);

                    if (status == ERROR_SUCCESS)
                    {

                        string steam_exe_path{install_path};
                        replace_forward_slash_with_backward_slash(steam_exe_path);
                        if (check_if_file_path_exists(steam_exe_path.c_str()))
                        {
                            string cod5mp_path1{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                "\\steamapps\\common\\Call of Duty World at War\\cod5mp.exe"s};
                            string cod5mp_path2{steam_exe_path.substr(0, steam_exe_path.rfind('\\')) +
                                                "\\steamapps\\common\\Call of Duty(R) World at War\\cod5mp.exe"s};
                            // to_lower_case_in_place(cod5mp_path1);
                            // to_lower_case_in_place(cod5mp_path2);

                            if (check_if_file_path_exists(cod5mp_path1.c_str()) ||
                                check_if_file_path_exists(cod5mp_path2.c_str()))
                            {
                                if (check_if_file_path_exists(cod5mp_path1.c_str()))
                                {
                                    // (void)snprintf(exe_file_path, max_path_length, "\"%s\"
                                    // -applaunch %ld", steam_exe_path.c_str(), cod2_steam_appid);
                                    (void)snprintf(exe_file_path, max_path_length, "%s", cod5mp_path1.c_str());
                                }
                                else if (check_if_file_path_exists(cod5mp_path2.c_str()))
                                {
                                    (void)snprintf(exe_file_path, max_path_length, "%s", cod5mp_path2.c_str());
                                }

                                found = true;
                                char buffer[1024];
                                (void)snprintf(buffer, std::size(buffer),
                                               "^2Successfully built your ^3Call of Duty 5: World at War "
                                               "(^4Steam version^3) ^2game's launch command: ^5%s\n",
                                               exe_file_path);
                                print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                                *def_game_reg_key = nullptr;
                            }
                        }
                    }

                    RegCloseKey(steam_installation_key);
                }
            }

            RegCloseKey(game_installation_reg_key);
        }
    }

    while (!found && *def_game_reg_key)
    {

        ZeroMemory(&game_installation_reg_key, sizeof(HKEY));

        cch = sizeof(install_path);

        status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

        if (status == ERROR_SUCCESS)
        {

            status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(install_path), &cch);

            if (status == ERROR_SUCCESS)
            {
                WIN32_FIND_DATA find_data{};
                remove_dir_path_sep_char(install_path);
                (void)snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", install_path);
                HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

                if (search_handle != INVALID_HANDLE_VALUE)
                {

                    found = true;
                    found_reg_location = *def_game_reg_key;
                    char buffer[1024];
                    (void)snprintf(buffer, std::size(buffer),
                                   "^2Successfully built your ^3Call of Duty 5: World at "
                                   "War ^2game's launch command: ^5%s\n",
                                   exe_file_path);
                    print_colored_text(app_handles.hwnd_re_messages_data, buffer);
                    *def_game_reg_key = nullptr;
                    break;
                }
            }

            RegCloseKey(game_installation_reg_key);
        }

        def_game_reg_key++;
    }

    if (!found)
    {

        def_game_reg_key = def_cod5_registry_location_subkeys;

        while (!found && *def_game_reg_key)
        {

            ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
            cch = sizeof(install_path);

            status = RegOpenKeyEx(HKEY_CURRENT_USER, *def_game_reg_key, 0, KEY_QUERY_VALUE, &game_installation_reg_key);

            if (status == ERROR_SUCCESS)
            {

                status = RegQueryValueEx(game_installation_reg_key, game_install_path_key, nullptr, nullptr,
                                         reinterpret_cast<LPBYTE>(install_path), &cch);

                if (status == ERROR_SUCCESS)
                {

                    WIN32_FIND_DATA find_data{};
                    remove_dir_path_sep_char(install_path);
                    (void)snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", install_path);
                    HANDLE search_handle = FindFirstFile(exe_file_path, &find_data);

                    if (search_handle != INVALID_HANDLE_VALUE)
                    {

                        found = true;
                        found_reg_location = *def_game_reg_key;
                        char buffer[1024];
                        (void)snprintf(buffer, std::size(buffer),
                                       "^2Successfully built your ^3Call of Duty 5: World "
                                       "at War ^2game's launch command: ^5%s\n",
                                       exe_file_path);
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

    if (!found && is_show_browse_folder_dialog)
    {

        str_copy(install_path, "C:\\");
        found_reg_location = nullptr;

        static char msgbuff[1024];
        (void)snprintf(msgbuff, std::size(msgbuff),
                       "Please, select your Call of Duty 5 game installation "
                       "folder and click OK.");

        const char *cod5_game_path = BrowseFolder(install_path, msgbuff);

        if (lstrcmp(cod5_game_path, "") != 0 && lstrcmp(cod5_game_path, "C:\\") != 0)
        {
            (void)snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", cod5_game_path);
        }
    }

    if (!str_contains(exe_file_path, cod5_steam_appid_string.c_str(), 0U, true))
    {
        string exe_path{exe_file_path};
        replace_forward_slash_with_backward_slash(exe_path);
        str_copy(exe_file_path, exe_path.c_str());
    }

    if (!check_if_file_path_exists(exe_file_path) &&
        !str_contains(exe_file_path, cod5_steam_appid_string.c_str(), 0U, true))
    {
        main_app.set_cod5mp_exe_path("");
    }
    else
    {
        main_app.set_cod5mp_exe_path(exe_file_path);
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    }

    return exe_file_path;
}

bool check_if_call_of_duty_5_game_is_running(DWORD &pid)
{
    DWORD processes[1024]{}, processes_size{};
    char buffer[512];
    pid = 0;
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    if (EnumProcesses(processes, sizeof(processes), &processes_size))
        for (size_t i{}; i < processes_size; ++i)
        {
            if (processes[i] == 0)
                continue;
            auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
            const auto n = GetModuleBaseName(ph, nullptr, buffer, sizeof(buffer));
            CloseHandle(ph);
            if (n == 0)
                continue;
            const string module_base_name{buffer};
            if (str_index_of(module_base_name, "cod5mp.exe", 0, true) != wstring::npos)
            {
                pid = 0;
                return true;
            }
        }

    DWORD cch{};
    HKEY game_installation_reg_key;

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    DWORD is_steam_game_running{};
    cch = sizeof(is_steam_game_running);

    auto status =
        RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {

        status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

        if (status == ERROR_SUCCESS && cod5_steam_appid == is_steam_game_running)
            return true;
    }

    RegCloseKey(game_installation_reg_key);

    ZeroMemory(&game_installation_reg_key, sizeof(HKEY));
    is_steam_game_running = 0;
    cch = sizeof(is_steam_game_running);

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE, &game_installation_reg_key);

    if (status == ERROR_SUCCESS)
    {

        status = RegQueryValueEx(game_installation_reg_key, "RunningAppID", nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(&is_steam_game_running), &cch);

        if (status == ERROR_SUCCESS && cod5_steam_appid == is_steam_game_running)
            return true;
    }

    RegCloseKey(game_installation_reg_key);

    return false;
}

bool connect_to_the_game_server(const std::string &server_ip_port_address, const game_name_t game_name,
                                const bool use_private_slot, const bool minimize_tinyrcon)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    constexpr size_t max_path_length{32768};
    static char command_line[max_path_length]{};

    const char *game_path{};
    DWORD pid{};
    switch (game_name)
    {
    case game_name_t::cod1:
        if (check_if_call_of_duty_1_game_is_running(pid))
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               "Call of Duty Multiplayer game is already running "
                               "in the background.\n",
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            return false;
        }

        game_path = main_app.get_codmp_exe_path().c_str();
        if (!check_if_file_path_exists(game_path) &&
            !stl::helper::str_contains(game_path, format("-applaunch {}", cod1_steam_appid).c_str(), 0U, true))
        {
            game_path = find_call_of_duty_1_installation_path();
        }
        break;

    case game_name_t::cod2:
        if (check_if_call_of_duty_2_game_is_running(pid))
        {
            terminate_running_game_instance(game_name_t::cod2);
            const string temporary_game_file{
                string(main_app.get_cod2mp_exe_path().cbegin(),
                       main_app.get_cod2mp_exe_path().cbegin() + main_app.get_cod2mp_exe_path().rfind('\\') + 1) +
                "__CoD2MP_s"};
            if (check_if_file_path_exists(temporary_game_file.c_str()))
            {
                DeleteFile(temporary_game_file.c_str());
            }
        }

        game_path = main_app.get_cod2mp_exe_path().c_str();
        if (!check_if_file_path_exists(game_path) &&
            !stl::helper::str_contains(game_path, format("-applaunch {}", cod2_steam_appid).c_str(), 0U, true))
        {
            game_path = find_call_of_duty_2_installation_path();
        }
        break;

    case game_name_t::cod4:
        if (check_if_call_of_duty_4_game_is_running(pid))
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               "Call of Duty 4: Modern Warfare Multiplayer game is "
                               "already running in the background.\n",
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            return false;
        }

        game_path = main_app.get_iw3mp_exe_path().c_str();
        if (!check_if_file_path_exists(game_path) &&
            !stl::helper::str_contains(game_path, format("-applaunch {}", cod4_steam_appid).c_str(), 0U, true))
        {
            game_path = find_call_of_duty_4_installation_path();
        }
        break;

    case game_name_t::cod5:
        if (check_if_call_of_duty_5_game_is_running(pid))
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               "Call of Duty 5: World at War Multiplayer game is "
                               "already running in the background.\n",
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            return false;
        }

        game_path = main_app.get_cod5mp_exe_path().c_str();
        if (!check_if_file_path_exists(game_path) &&
            !stl::helper::str_contains(game_path, format("-applaunch {}", cod5_steam_appid).c_str(), 0U, true))
        {
            game_path = find_call_of_duty_5_installation_path();
        }
        break;

    default:
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^1Invalid ^1'game_name_t' enum value has been specified!\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        return false;
    }

    assert(game_path != nullptr && stl::helper::len(game_path) > 0U);

    const bool is_game_installed_via_steam{
        stl::helper::str_contains(game_path, format("-applaunch {}", cod1_steam_appid).c_str(), 0U, true) ||
        stl::helper::str_contains(game_path, format("-applaunch {}", cod2_steam_appid).c_str(), 0U, true) ||
        stl::helper::str_contains(game_path, format("-applaunch {}", cod4_steam_appid).c_str(), 0U, true) ||
        stl::helper::str_contains(game_path, format("-applaunch {}", cod5_steam_appid).c_str(), 0U, true)};

    if (game_path == nullptr || len(game_path) == 0 ||
        (!str_ends_with(game_path, ".exe", true) && !is_game_installed_via_steam))
        return false;

    if (minimize_tinyrcon)
    {
        ShowWindow(app_handles.hwnd_main_window, SW_MINIMIZE);
    }

    if (is_game_installed_via_steam)
    {
        if (use_private_slot)
        {
            (void)snprintf(command_line, max_path_length, "\"%s\" +password %s +connect %s", game_path,
                           main_app.get_current_game_server().get_private_slot_password().c_str(),
                           server_ip_port_address.c_str());
        }
        else
        {
            (void)snprintf(command_line, max_path_length, "\"%s\" +connect %s", game_path,
                           server_ip_port_address.c_str());
        }
    }
    else
    {
        if (use_private_slot)
        {
            (void)snprintf(command_line, max_path_length, "\"%s\" +password %s +connect %s", game_path,
                           main_app.get_current_game_server().get_private_slot_password().c_str(),
                           server_ip_port_address.c_str());
        }
        else
        {
            (void)snprintf(command_line, max_path_length, "\"%s\" +connect %s", game_path,
                           server_ip_port_address.c_str());
        }
    }

    string game_folder{game_path};
    game_folder.erase(cbegin(game_folder) + game_folder.rfind('\\'), cend(game_folder));
    if (!game_folder.empty() && '\\' == game_folder.back())
        game_folder.pop_back();

    if (!is_game_installed_via_steam)
    {

        ostringstream oss;
        oss << "^2Launching ^1" << main_app.get_game_title() << " ^2and connecting to ^1" << server_ip_port_address
            << " ^7" << main_app.get_current_game_server().get_server_name() << " ^2game server...\n";

        const string message{oss.str()};

        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

        STARTUPINFO process_startup_info{};
        process_startup_info.cb = sizeof(STARTUPINFO);
        process_startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK;
        PROCESS_INFORMATION pr_info{};

        if (!CreateProcess(nullptr, command_line,
                           nullptr,                                // Process handle not inheritable
                           nullptr,                                // Thread handle not inheritable
                           FALSE,                                  // Set handle inheritance to FALSE
                           HIGH_PRIORITY_CLASS | DETACHED_PROCESS, // creation flags
                           nullptr,                                // Use parent's environment block
                           game_folder.c_str(),                    // Use parent's starting directory
                           &process_startup_info,                  // Pointer to STARTUPINFO structure
                           &pr_info))
        {
            char buffer[512]{};
            strerror_s(buffer, 512, GetLastError());
            const string error_message{"^1Launching game failed: "s + buffer + "\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            return false;
        }

        if (process_startup_info.hStdError)
            CloseHandle(process_startup_info.hStdError);
        if (process_startup_info.hStdInput)
            CloseHandle(process_startup_info.hStdInput);
        if (process_startup_info.hStdOutput)
            CloseHandle(process_startup_info.hStdOutput);
        CloseHandle(pr_info.hThread);
        CloseHandle(pr_info.hProcess);
    }
    else
    {

        ostringstream oss;
        oss << "^2Launching ^1" << main_app.get_game_title() << " ^2via Steam and connecting to ^1"
            << server_ip_port_address << " ^7" << main_app.get_current_game_server().get_server_name()
            << " ^2game server...\n";
        const string message{oss.str()};
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

        STARTUPINFO process_startup_info{};
        process_startup_info.cb = sizeof(STARTUPINFO);
        process_startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK;
        PROCESS_INFORMATION pr_info{};

        if (!CreateProcess(nullptr, command_line,
                           nullptr,                                // Process handle not inheritable
                           nullptr,                                // Thread handle not inheritable
                           FALSE,                                  // Set handle inheritance to FALSE
                           HIGH_PRIORITY_CLASS | DETACHED_PROCESS, // creation flags
                           nullptr,                                // Use parent's environment block
                           game_folder.c_str(),                    // Use parent's starting directory
                           &process_startup_info,                  // Pointer to STARTUPINFO structure
                           &pr_info))
        {
            char buffer[512]{};
            strerror_s(buffer, std::size(buffer), GetLastError());
            const string error_message{"^1Launching game via Steam failed: "s + buffer + "\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            return false;
        }

        if (process_startup_info.hStdError)
            CloseHandle(process_startup_info.hStdError);
        if (process_startup_info.hStdInput)
            CloseHandle(process_startup_info.hStdInput);
        if (process_startup_info.hStdOutput)
            CloseHandle(process_startup_info.hStdOutput);
        CloseHandle(pr_info.hThread);
        CloseHandle(pr_info.hProcess);
    }

    initiate_sending_rcon_status_command_now();
    return true;
}

bool check_if_file_path_exists(const char *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const directory_entry dir_entry{file_path};
    return dir_entry.exists();
}

bool check_if_file_path_exists(const wchar_t *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const directory_entry dir_entry{file_path};
    return dir_entry.exists();
}

bool check_if_cod1_multiplayer_game_launch_command_is_correct(const std::string &command)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const string cod1_steam_appid_string{std::format("-applaunch {}", cod1_steam_appid)};

    if (command.empty())
        return false;

    const size_t applaunch_start_pos{command.rfind(cod1_steam_appid_string)};

    if (applaunch_start_pos != string::npos)
    {
        string steam_exe_path{command.substr(0, applaunch_start_pos)};
        stl::helper::trim_in_place(steam_exe_path);
        strip_leading_and_trailing_quotes(steam_exe_path);
        return check_if_file_path_exists(steam_exe_path.c_str());
    }

    return check_if_file_path_exists(command.c_str());
}

bool check_if_cod2_multiplayer_game_launch_command_is_correct(const std::string &command)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const string cod2_steam_appid_string{std::format("-applaunch {}", cod2_steam_appid)};

    if (command.empty())
        return false;

    const size_t applaunch_start_pos{command.rfind(cod2_steam_appid_string)};

    if (applaunch_start_pos != string::npos)
    {
        string steam_exe_path{command.substr(0, applaunch_start_pos)};
        stl::helper::trim_in_place(steam_exe_path);
        strip_leading_and_trailing_quotes(steam_exe_path);
        return check_if_file_path_exists(steam_exe_path.c_str());
    }

    return check_if_file_path_exists(command.c_str());
}

bool check_if_cod4_multiplayer_game_launch_command_is_correct(const std::string &command)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const string cod4_steam_appid_string{std::format("-applaunch {}", cod4_steam_appid)};

    if (command.empty())
        return false;

    const size_t applaunch_start_pos{command.rfind(cod4_steam_appid_string)};

    if (applaunch_start_pos != string::npos)
    {
        string steam_exe_path{command.substr(0, applaunch_start_pos)};
        stl::helper::trim_in_place(steam_exe_path);
        strip_leading_and_trailing_quotes(steam_exe_path);
        return check_if_file_path_exists(steam_exe_path.c_str());
    }

    return check_if_file_path_exists(command.c_str());
}

bool check_if_cod5_multiplayer_game_launch_command_is_correct(const std::string &command)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const string cod5_steam_appid_string{std::format("-applaunch {}", cod5_steam_appid)};

    if (command.empty())
        return false;

    const size_t applaunch_start_pos{command.rfind(cod5_steam_appid_string)};

    if (applaunch_start_pos != string::npos)
    {
        string steam_exe_path{command.substr(0, applaunch_start_pos)};
        stl::helper::trim_in_place(steam_exe_path);
        strip_leading_and_trailing_quotes(steam_exe_path);
        return check_if_file_path_exists(steam_exe_path.c_str());
    }

    return check_if_file_path_exists(command.c_str());
}

bool delete_temporary_game_file()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    DWORD processes[1024], processes_size{};
    wchar_t buffer[512];

    memset(processes, 0, 1024 * sizeof(DWORD));

    if (EnumProcesses(processes, sizeof(processes), &processes_size))
        for (size_t i{}; i < processes_size; ++i)
        {
            if (processes[i] == 0)
                continue;
            auto ph = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, processes[i]);
            if (nullptr == ph)
                continue;
            const auto n = GetModuleBaseNameW(ph, nullptr, buffer, sizeof(buffer));

            if (n != 0)
            {
                wstring module_base_name{buffer};
                if (module_base_name.find(L"Call of Duty 2") != wstring::npos ||
                    module_base_name.find(L"Call of Duty(R) 2") != wstring::npos ||
                    stl::helper::str_index_of(module_base_name, L"cod2mp_s.exe", true) != wstring::npos)
                {
                    GetModuleFileNameExW(ph, nullptr, buffer, (DWORD)std::size(buffer));
                    module_base_name.assign(buffer);
                    const auto start =
                        stl::helper::str_last_index_of(module_base_name, L"cod2mp_s.exe", string::npos, true);
                    if (start != wstring::npos)
                    {
                        module_base_name.replace(start, len(L"cod2mp_s.exe"), L"__CoD2MP_s");
                        DeleteFileW(module_base_name.c_str());
                    }
                    return true;
                }
            }

            CloseHandle(ph);
        }

    return false;
}

size_t print_colored_text(HWND re_control, const char *text,
                          const is_append_message_to_richedit_control print_to_richedit_control,
                          const is_log_message log_to_file, is_log_datetime is_log_current_date_time, const bool,
                          const bool is_remove_color_codes_for_log_message)
{
    const char *message{text};
    size_t text_len{stl::helper::len(text)};
    if (nullptr == text || 0U == text_len)
        return 0U;
    const char *last = text + text_len;
    size_t printed_chars_count{};
    bool is_last_char_new_line{text[text_len - 1] == '\n'};

    if (print_to_richedit_control == is_append_message_to_richedit_control::yes)
    {

        if (re_control == app_handles.hwnd_re_messages_data)
        {
            main_app.add_tinyrcon_message_to_queue(
                print_message_t{text, log_to_file, is_log_current_date_time, is_remove_color_codes_for_log_message});
            return 0u;
        }

        ostringstream os;
        string text_str;
        if (is_log_current_date_time == is_log_datetime::yes)
        {
            string current_date_and_time_info{get_date_and_time_for_time_t("^7{DD}.{MM}.{Y} {hh}:{mm}:\n")};
            if (main_app.get_last_date_and_time_information() != current_date_and_time_info)
            {
                main_app.set_last_date_and_time_information(std::move(current_date_and_time_info));
                os << main_app.get_last_date_and_time_information();
            }
            else
            {
                is_log_current_date_time = is_log_datetime::no;
            }
            os << text;
            text_str = os.str();
            text = text_str.c_str();
            last = text + text_str.length();
            text_len = text_str.length();
            is_last_char_new_line = '\n' == text[text_len - 1];
        }

        cursor_to_bottom(re_control);
        // scroll_to_bottom(re_control);

        SETTEXTEX stx{};
        stx.flags = ST_DEFAULT | ST_SELECTION | ST_NEWCHARS;
        stx.codepage = 1251;

        COLORREF bg_color{color::black};
        COLORREF fg_color{color::white};
        set_rich_edit_control_colors(re_control, fg_color, bg_color);

        string msg{};

        for (; *text; ++text)
        {
            if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') &&
                (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3))
            {
                text += 3;
                if (!msg.empty())
                {
                    SendMessage(re_control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx),
                                reinterpret_cast<LPARAM>(msg.c_str()));
                    msg.clear();
                }

                fg_color = rich_edit_colors.at(*text);
                set_rich_edit_control_colors(re_control, fg_color, bg_color);
                msg.push_back('^');
                msg.push_back(*text);
                printed_chars_count += 2;
            }
            else if (text + 2 <= last && *text == '^' && (*(text + 1) >= '0' && *(text + 1) <= '9'))
            {
                ++text;
                if (!msg.empty())
                {
                    SendMessage(re_control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx),
                                reinterpret_cast<LPARAM>(msg.c_str()));
                    msg.clear();
                }
                fg_color = rich_edit_colors.at(*text);
                set_rich_edit_control_colors(re_control, fg_color, bg_color);
            }
            else
            {
                msg.push_back(*text);
                ++printed_chars_count;
            }
        }

        if (!msg.empty())
        {
            SendMessage(re_control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx),
                        reinterpret_cast<LPARAM>(msg.c_str()));
            msg.clear();
        }

        if (!is_last_char_new_line)
        {
            SendMessage(re_control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx), reinterpret_cast<LPARAM>("\n"));
            ++printed_chars_count;
        }

        // cursor_to_bottom(re_control);
        scroll_to_bottom(re_control);
    }

    if (log_to_file == is_log_message::yes)
    {
        string message_without_color_codes{message};
        if (is_remove_color_codes_for_log_message)
        {
            remove_all_color_codes(message_without_color_codes);
        }
        if (!message_without_color_codes.empty() && message_without_color_codes.back() != '\n')
        {
            message_without_color_codes.push_back('\n');
        }
        log_message(message_without_color_codes, is_log_current_date_time);
    }

    return printed_chars_count;
}

size_t print_message(HWND re_control, const std::string &msg, const is_log_message log_to_file,
                     is_log_datetime is_log_current_date_time, const bool is_remove_color_codes_for_log_message)
{
    if (msg.empty())
        return 0U;
    size_t text_len{msg.length()};
    const char *last = msg.c_str() + text_len;
    size_t printed_chars_count{};
    bool is_last_char_new_line{msg.back() == '\n'};

    ostringstream os;
    string text_str;
    const char *text{msg.c_str()};
    if (is_log_current_date_time == is_log_datetime::yes)
    {
        string current_date_and_time_info{get_date_and_time_for_time_t("^7{DD}.{MM}.{Y} {hh}:{mm}:\n")};
        if (main_app.get_last_date_and_time_information() != current_date_and_time_info)
        {
            main_app.set_last_date_and_time_information(std::move(current_date_and_time_info));
            os << main_app.get_last_date_and_time_information();
        }
        else
        {
            is_log_current_date_time = is_log_datetime::no;
        }
        os << msg;
        text_str = os.str();
        text = text_str.c_str();
        last = text + text_str.length();
        // text_len = text_str.length();
        is_last_char_new_line = '\n' == text[text_str.length() - 1];
    }

    cursor_to_bottom(re_control);
    // scroll_to_bottom(re_control);

    SETTEXTEX stx{};
    stx.flags = ST_DEFAULT | ST_SELECTION | ST_NEWCHARS;
    stx.codepage = 1251;

    COLORREF bg_color{color::black};
    COLORREF fg_color{color::white};
    set_rich_edit_control_colors(re_control, fg_color, bg_color);

    string msg2{};

    for (; *text; ++text)
    {
        if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') &&
            (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3))
        {
            text += 3;
            if (!msg2.empty())
            {
                SendMessage(re_control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx),
                            reinterpret_cast<LPARAM>(msg2.c_str()));
                msg2.clear();
            }

            fg_color = rich_edit_colors.at(*text);
            set_rich_edit_control_colors(re_control, fg_color, bg_color);
            msg2.push_back('^');
            msg2.push_back(*text);
            printed_chars_count += 2;
        }
        else if (text + 2 <= last && *text == '^' && (*(text + 1) >= '0' && *(text + 1) <= '9'))
        {
            ++text;
            if (!msg2.empty())
            {
                SendMessage(re_control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx),
                            reinterpret_cast<LPARAM>(msg2.c_str()));
                msg2.clear();
            }
            fg_color = rich_edit_colors.at(*text);
            set_rich_edit_control_colors(re_control, fg_color, bg_color);
        }
        else
        {
            msg2.push_back(*text);
            ++printed_chars_count;
        }
    }

    if (!msg2.empty())
    {
        SendMessage(re_control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx), reinterpret_cast<LPARAM>(msg2.c_str()));
    }

    if (!is_last_char_new_line)
    {
        SendMessage(re_control, EM_SETTEXTEX, reinterpret_cast<WPARAM>(&stx), reinterpret_cast<LPARAM>("\n"));
        ++printed_chars_count;
    }

    // cursor_to_bottom(re_control);
    scroll_to_bottom(re_control);

    if (log_to_file == is_log_message::yes)
    {
        string message_without_color_codes{msg};
        if (is_remove_color_codes_for_log_message)
        {
            remove_all_color_codes(message_without_color_codes);
        }
        if (!message_without_color_codes.empty() && message_without_color_codes.back() != '\n')
        {
            message_without_color_codes.push_back('\n');
        }
        log_message(message_without_color_codes, is_log_current_date_time);
    }

    return printed_chars_count;
}

size_t print_colored_text_to_grid_cell(HDC hdc, RECT &rect, const char *text, DWORD formatting_style)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const size_t text_len{stl::helper::len(text)};
    const char *last = text + text_len;
    size_t printed_chars_count{};

    string msg;
    COLORREF fg_color{color::white};

    for (; *text; ++text)
    {
        if (text + 4 <= last && *text == '^' && *(text + 1) == '^' && (*(text + 2) >= '0' && *(text + 2) <= '9') &&
            (*(text + 3) >= '0' && *(text + 3) <= '9') && *(text + 2) == *(text + 3))
        {
            text += 3;
            if (!msg.empty())
            {
                RECT calculate_rect{rect};
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
        }
        else if (text + 2 <= last && *text == '^' && (*(text + 1) >= '0' && *(text + 1) <= '9'))
        {
            ++text;
            if (!msg.empty())
            {
                RECT calculate_rect{rect};
                DrawTextExA(hdc, msg.data(), msg.length(), &calculate_rect, DT_CALCRECT, nullptr);
                DrawTextExA(hdc, msg.data(), msg.length(), &rect, formatting_style, nullptr);
                rect.left += (calculate_rect.right - calculate_rect.left);
                msg.clear();
            }
            fg_color = colors.at(*text);
            SetTextColor(hdc, fg_color);
        }
        else
        {
            msg.push_back(*text);
            ++printed_chars_count;
        }
    }

    if (!msg.empty())
    {
        RECT calculate_rect{rect};
        DrawTextExA(hdc, msg.data(), msg.length(), &calculate_rect, DT_CALCRECT, nullptr);
        DrawTextExA(hdc, msg.data(), msg.length(), &rect, formatting_style, nullptr);
        rect.left += (calculate_rect.right - calculate_rect.left);
        msg.clear();
    }

    return printed_chars_count;
}

bool get_confirmation_message_from_user(const char *message, const char *caption)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const auto answer =
        MessageBox(app_handles.hwnd_main_window, message, caption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);
    return answer == IDYES;
}

bool check_if_user_wants_to_quit(const char *msg)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    return str_compare(msg, "q") == 0 || str_compare(msg, "!q") == 0 || str_compare(msg, "exit") == 0 ||
           str_compare(msg, "quit") == 0 || str_compare(msg, "!exit") == 0 || str_compare(msg, "!quit") == 0;
}

void set_rich_edit_control_colors(HWND richEditCtrl, const COLORREF fg_color, const COLORREF bg_color,
                                  const char *font_face_name)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    CHARFORMAT2A cf{};
    cf.cbSize = sizeof(CHARFORMAT2A);
    cf.dwMask = CFM_CHARSET | CFM_FACE | CFM_COLOR | CFM_BACKCOLOR | CFM_WEIGHT;
    str_copy(cf.szFaceName, font_face_name);
    cf.wWeight = 800;
    cf.bCharSet = RUSSIAN_CHARSET;
    cf.crTextColor = fg_color;
    cf.crBackColor = bg_color;

    SendMessageA(richEditCtrl, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

CHARFORMATA
get_char_fmt(HWND hwnd, DWORD range)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    CHARFORMATA cf;
    SendMessage(hwnd, EM_GETCHARFORMAT, range, (LPARAM)&cf);
    return cf;
}
void set_char_fmt(HWND hwnd, const CHARFORMAT2A &cf, DWORD range)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SendMessageA(hwnd, EM_SETCHARFORMAT, range, (LPARAM)&cf);
}
void replace_sel(HWND hwnd, const char *str)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SendMessageA(hwnd, EM_REPLACESEL, 0, (LPARAM)str);
}
void cursor_to_bottom(HWND hwnd)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SendMessageA(hwnd, EM_SETSEL, (WPARAM)-2, -1);
}
void scroll_to_beginning(HWND hwnd)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SendMessageA(hwnd, WM_VSCROLL, 0, 0);
    SendMessageA(hwnd, WM_HSCROLL, SB_LEFT, 0);
}
void scroll_to(HWND hwnd, DWORD pos)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SendMessageA(hwnd, WM_VSCROLL, pos, 0);
}
void scroll_to_bottom(HWND hwnd)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    scroll_to(hwnd, SB_BOTTOM);
}

void append_to_title(HWND window, std::string text, const char *animation_sequence_chars)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char window_title[512];
    static size_t anim_char_index{};

    if (len(animation_sequence_chars) == 0u)
        animation_sequence_chars = "-\\|/";
    remove_all_color_codes(text);
    snprintf(window_title, std::size(window_title), "%s %c %s", main_app.get_program_title().c_str(),
             animation_sequence_chars[anim_char_index], text.c_str());
    SetWindowTextA(window, window_title);
    if (++anim_char_index == len(animation_sequence_chars))
        anim_char_index = 0u;
}

void process_key_down_message(const MSG &msg)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (msg.wParam == VK_ESCAPE)
    {
        if (show_user_confirmation_dialog("^3Do you really want to quit?", "Exit program?", "Reason"))
        {
            is_terminate_program.store(true);
            PostQuitMessage(0);
        }
    }
    else if (msg.wParam == VK_UP)
    {
        if (!commands_history.empty())
        {
            if (commands_history_index > 0)
            {
                --commands_history_index;
                if (commands_history_index < commands_history.size())
                {
                    SetWindowTextA(app_handles.hwnd_e_user_input, commands_history.at(commands_history_index).c_str());
                    cursor_to_bottom(app_handles.hwnd_e_user_input);
                }
            }
        }
    }
    else if (msg.wParam == VK_DOWN)
    {
        if (!commands_history.empty())
        {
            if (commands_history_index < commands_history.size() - 1)
            {
                ++commands_history_index;
                if (commands_history_index < commands_history.size())
                {
                    SetWindowTextA(app_handles.hwnd_e_user_input, commands_history.at(commands_history_index).c_str());
                    cursor_to_bottom(app_handles.hwnd_e_user_input);
                }
            }
        }
    }
    else if (app_handles.hwnd_e_user_input == msg.hwnd && msg.wParam == VK_RETURN)
    {
        if (get_user_input())
        {
            is_terminate_program.store(true);
            PostQuitMessage(0);
        }
    }
    else if (msg.wParam == VK_F1)
    {
        is_process_combobox_item_selection_event = false;
        type_of_sort = sort_by_pid_asc ? sort_type::pid_asc : sort_type::pid_desc;
        sort_by_pid_asc = !sort_by_pid_asc;
        process_sort_type_change_request(type_of_sort);
    }
    else if (msg.wParam == VK_F2)
    {
        is_process_combobox_item_selection_event = false;
        type_of_sort = sort_by_score_asc ? sort_type::score_asc : sort_type::score_desc;
        sort_by_score_asc = !sort_by_score_asc;
        process_sort_type_change_request(type_of_sort);
    }
    else if (msg.wParam == VK_F3)
    {
        is_process_combobox_item_selection_event = false;
        type_of_sort = sort_by_ping_asc ? sort_type::ping_asc : sort_type::ping_desc;
        sort_by_ping_asc = !sort_by_ping_asc;
        process_sort_type_change_request(type_of_sort);
    }
    else if (msg.wParam == VK_F4)
    {
        is_process_combobox_item_selection_event = false;
        type_of_sort = sort_by_name_asc ? sort_type::name_asc : sort_type::name_desc;
        sort_by_name_asc = !sort_by_name_asc;
        process_sort_type_change_request(type_of_sort);
    }
    else if (msg.wParam == VK_F5)
    {
        is_process_combobox_item_selection_event = false;
        type_of_sort = sort_by_ip_asc ? sort_type::ip_asc : sort_type::ip_desc;
        sort_by_ip_asc = !sort_by_ip_asc;
        process_sort_type_change_request(type_of_sort);
    }
    else if (msg.wParam == VK_F6)
    {
        is_process_combobox_item_selection_event = false;
        type_of_sort = sort_by_geo_asc ? sort_type::geo_asc : sort_type::geo_desc;
        sort_by_geo_asc = !sort_by_geo_asc;
        process_sort_type_change_request(type_of_sort);
    }
    else if (msg.wParam == VK_F8)
    {
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Manually refreshing players data.\n");
        initiate_sending_rcon_status_command_now();
    }
}

void display_tempbanned_players_remaining_time_period()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    auto &tembanned_players = main_app.get_current_game_server().get_temp_banned_ip_addresses_vector();
    if (tembanned_players.empty())
        return;
    std::sort(std::begin(tembanned_players), std::end(tembanned_players), [](const player &pl1, const player &pl2) {
        return pl1.banned_start_time + (pl1.ban_duration_in_hours * 3600) <
               pl2.banned_start_time + (pl2.ban_duration_in_hours * 3600);
    });

    const auto now_in_seconds{get_current_time_stamp()};

    for (auto &pl : tembanned_players)
    {
        pl.reason = remove_disallowed_character_in_string(pl.reason);
        const auto ban_expires_time = pl.banned_start_time + (pl.ban_duration_in_hours * 3600);
        if (ban_expires_time > now_in_seconds)
        {
            ostringstream oss;
            oss << "^3Player's (^7" << pl.player_name << "^3) temporary ban (banned on ^1"
                << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pl.banned_start_time) << " ^3with reason: ^1"
                << pl.reason << "^3)\n will automatically be removed in ^1"
                << get_time_interval_info_string_for_seconds(ban_expires_time - now_in_seconds) << '\n';
            const string message{oss.str()};
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }
        else
        {
            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = pl.player_name;
            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
            main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] =
                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pl.banned_start_time);
            main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] =
                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", ban_expires_time);
            main_app.get_tinyrcon_dict()["{REASON}"] = pl.reason;
            main_app.get_tinyrcon_dict()["{BANNED_BY}"] = remove_disallowed_character_in_string(pl.banned_by_user_name);
            string message{main_app.get_automatic_remove_temp_ban_msg()};
            main_app.get_connection_manager_for_messages().process_and_send_message(
                "remove-tempban",
                format(R"({}\{}\{}\{}\{}\{}\{})", pl.ip_address, pl.player_name, pl.banned_date_time,
                       pl.banned_start_time, pl.ban_duration_in_hours, pl.reason, pl.banned_by_user_name),
                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            build_tiny_rcon_message(message);
            auto [status, player] = remove_temp_banned_ip_address(pl.ip_address, message, true, true);
            if (status)
            {
                replace_br_with_new_line(message);
                const string private_msg{format(
                    "^2Player ^7{}'s ^1temporary ban ^2has automatically been "
                    "removed.\n^7Info on player ^7{}^7: ^3IP: ^1{} ^5| ^3geoinfo: ^1{}, "
                    "{} ^5| ^3Date of tempban: ^1{}\n ^3Tempban duration: ^1{} hours "
                    "^5|^5Reason: ^1{} ^5| ^3Banned by: ^1{}\n",
                    player.player_name, player.player_name, player.ip_address, player.country_name, player.city,
                    player.banned_date_time, player.ban_duration_in_hours, player.reason, player.banned_by_user_name)};
                const string inform_msg{format("{}\\{}\\{}", main_app.get_username(), message, private_msg)};
                // main_app.add_message_to_queue(message_t("inform-message", inform_msg,
                // true));
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "inform-message", inform_msg, true, main_app.get_tiny_rcon_server_ip_address(),
                    main_app.get_tiny_rcon_server_port(), false);
            }
        }
    }
}

void construct_tinyrcon_gui(HWND hWnd)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (app_handles.hwnd_online_admins_information != nullptr)
    {
        ShowWindow(app_handles.hwnd_online_admins_information, SW_HIDE);
        DestroyWindow(app_handles.hwnd_online_admins_information);
    }

    app_handles.hwnd_online_admins_information =
        CreateWindowExA(0, RICHEDIT_CLASSA, nullptr, WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY, 10, 13,
                        screen_width / 2 + 150, 25, hWnd, nullptr, app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_online_admins_information)
        FatalAppExit(0, "Couldn't create 'app_handles.hwnd_online_admins_information' "
                        "richedit control!");

    assert(app_handles.hwnd_online_admins_information != nullptr);
    SendMessage(app_handles.hwnd_online_admins_information, EM_SETBKGNDCOLOR, NULL, color::black);
    scroll_to_beginning(app_handles.hwnd_online_admins_information);
    set_rich_edit_control_colors(app_handles.hwnd_online_admins_information, color::white, color::black,
                                 "Lucida Console");
    print_colored_text(app_handles.hwnd_online_admins_information,
                       "^2Online admins:", is_append_message_to_richedit_control::yes, is_log_message::no,
                       is_log_datetime::no);
    SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
    SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);

    if (app_handles.hwnd_match_information != nullptr)
    {
        ShowWindow(app_handles.hwnd_match_information, SW_HIDE);
        DestroyWindow(app_handles.hwnd_match_information);
    }

    app_handles.hwnd_match_information =
        CreateWindowExA(0, RICHEDIT_CLASSA, nullptr, WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY, 10, 50,
                        screen_width / 2 + 150, 30, hWnd, nullptr, app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_match_information)
        FatalAppExit(0, "Couldn't create 'app_handles.hwnd_match_information' richedit control!");

    assert(app_handles.hwnd_match_information != nullptr);
    SendMessage(app_handles.hwnd_match_information, EM_SETBKGNDCOLOR, NULL, color::black);
    scroll_to_beginning(app_handles.hwnd_match_information);
    set_rich_edit_control_colors(app_handles.hwnd_match_information, color::white, color::black, "Lucida Console");
    if (g_re_match_information_contents.empty())
        SetWindowTextA(app_handles.hwnd_match_information, "");
    else
        print_colored_text(app_handles.hwnd_match_information, g_re_match_information_contents.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);

    if (app_handles.hwnd_re_messages_data != nullptr)
    {
        ShowWindow(app_handles.hwnd_re_messages_data, SW_HIDE);
        DestroyWindow(app_handles.hwnd_re_messages_data);
    }

    app_handles.hwnd_re_messages_data =
        CreateWindowExA(0, RICHEDIT_CLASSA, nullptr,
                        WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE |
                            ES_LEFT /*| ES_AUTOVSCROLL*/ | ES_NOHIDESEL | ES_READONLY,
                        screen_width / 2 + 170, 80, screen_width / 2 - 190, screen_height / 2 - 120, hWnd,
                        reinterpret_cast<HMENU>(ID_BANNEDEDIT), app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_re_messages_data)
        FatalAppExit(0, "Couldn't create 'g_banned_players_data' richedit control!");

    if (app_handles.hwnd_re_messages_data != nullptr)
    {
        SendMessage(app_handles.hwnd_re_messages_data, EM_SETBKGNDCOLOR, NULL, color::black);
        scroll_to_beginning(app_handles.hwnd_re_messages_data);
        set_rich_edit_control_colors(app_handles.hwnd_re_messages_data, color::white, color::black, "Lucida Console");
        if (g_message_data_contents.empty())
            SetWindowTextA(app_handles.hwnd_re_messages_data, "");
        else
            print_colored_text(app_handles.hwnd_re_messages_data, g_message_data_contents.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    }

    if (app_handles.hwnd_re_help_data != nullptr)
    {
        ShowWindow(app_handles.hwnd_re_help_data, SW_HIDE);
        DestroyWindow(app_handles.hwnd_re_help_data);
    }

    app_handles.hwnd_re_help_data =
        CreateWindowExA(0, RICHEDIT_CLASSA, nullptr,
                        WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_LEFT |
                            ES_AUTOVSCROLL | ES_READONLY,
                        screen_width / 2 + 170, screen_height / 2, screen_width - 20 - (screen_width / 2 + 170),
                        screen_height - 120 - (screen_height / 2 + 100), hWnd, nullptr, app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_re_help_data)
        FatalAppExit(0, "Couldn't create 'g_re_help' richedit control!");
    if (app_handles.hwnd_re_help_data != nullptr)
    {
        SendMessage(app_handles.hwnd_re_help_data, EM_SETBKGNDCOLOR, NULL, color::black);
        scroll_to_beginning(app_handles.hwnd_re_help_data);
        set_rich_edit_control_colors(app_handles.hwnd_re_help_data, color::white, color::black, "Lucida Console");
        print_colored_text(app_handles.hwnd_re_help_data, user_help_message, is_append_message_to_richedit_control::yes,
                           is_log_message::no, is_log_datetime::no, true);
    }

    app_handles.hwnd_e_user_input = CreateWindowExA(
        0, "Edit", nullptr, WS_GROUP | WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 125,
        screen_height - 70, 305, 20, hWnd, reinterpret_cast<HMENU>(ID_USEREDIT), app_handles.hInstance, nullptr);

    app_handles.hwnd_button_warn = CreateWindowExA(
        NULL, "Button", "&Warn", WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
        10, screen_height - 118, 80, 30, hWnd, reinterpret_cast<HMENU>(ID_WARNBUTTON), app_handles.hInstance, nullptr);

    app_handles.hwnd_button_kick = CreateWindowExA(
        NULL, "Button", "&Kick", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 110,
        screen_height - 118, 80, 30, hWnd, reinterpret_cast<HMENU>(ID_KICKBUTTON), app_handles.hInstance, nullptr);

    app_handles.hwnd_button_tempban = CreateWindowExA(
        NULL, "Button", "&Tempban", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 210,
        screen_height - 118, 100, 30, hWnd, reinterpret_cast<HMENU>(ID_TEMPBANBUTTON), app_handles.hInstance, nullptr);

    app_handles.hwnd_button_ipban = CreateWindowExA(
        NULL, "Button", "Ban &IP", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 330,
        screen_height - 118, 100, 30, hWnd, reinterpret_cast<HMENU>(ID_IPBANBUTTON), app_handles.hInstance, nullptr);

    app_handles.hwnd_button_view_tempbans =
        CreateWindowExA(NULL, "Button", "View temporary bans",
                        BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 450, screen_height - 118, 180,
                        30, hWnd, reinterpret_cast<HMENU>(ID_VIEWTEMPBANSBUTTON), app_handles.hInstance, nullptr);

    app_handles.hwnd_button_view_ipbans =
        CreateWindowExA(NULL, "Button", "View IP bans", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
                        650, screen_height - 118, 140, 30, hWnd, reinterpret_cast<HMENU>(ID_VIEWIPBANSBUTTON),
                        app_handles.hInstance, nullptr);

    app_handles.hwnd_button_view_adminsdata = CreateWindowExA(
        NULL, "Button", "View admins' data", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
        screen_width / 2 + 170, screen_height - 175, 150, 25, hWnd, reinterpret_cast<HMENU>(ID_VIEWADMINSDATA),
        app_handles.hInstance, nullptr);

    app_handles.hwnd_connect_button = CreateWindowExA(
        NULL, "Button", "Join server", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 450,
        screen_height - 77, 180, 30, hWnd, reinterpret_cast<HMENU>(ID_CONNECTBUTTON), app_handles.hInstance, nullptr);

    // app_handles.hwnd_connect_private_slot_button = CreateWindowEx(NULL,
    // "Button", "Join server (private slot)", BS_PUSHBUTTON | BS_CENTER |
    // BS_VCENTER | WS_VISIBLE | WS_CHILD, 450, screen_height - 77, 180, 30, hWnd,
    // reinterpret_cast<HMENU>(ID_CONNECTPRIVATESLOTBUTTON),
    // app_handles.hInstance, nullptr);

    app_handles.hwnd_say_button = CreateWindowExA(
        NULL, "Button", "Send public message", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
        screen_width / 2 + 170, screen_height - 140, 150, 25, hWnd, reinterpret_cast<HMENU>(ID_SAY_BUTTON),
        app_handles.hInstance, nullptr);

    app_handles.hwnd_tell_button = CreateWindowExA(
        NULL, "Button", "Send private message", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
        screen_width / 2 + 170, screen_height - 105, 150, 25, hWnd, reinterpret_cast<HMENU>(ID_TELL_BUTTON),
        app_handles.hInstance, nullptr);

    app_handles.hwnd_quit_button = CreateWindowExA(
        NULL, "Button", "E&xit", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width - 90,
        screen_height - 90, 70, 28, hWnd, reinterpret_cast<HMENU>(ID_QUITBUTTON), app_handles.hInstance, nullptr);

    app_handles.hwnd_clear_messages_button =
        CreateWindowExA(NULL, "Button", "C&lear messages",
                        BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 170, 8, 120,
                        28, hWnd, reinterpret_cast<HMENU>(ID_CLEARMESSAGESCREENBUTTON), app_handles.hInstance, nullptr);

    app_handles.hwnd_configure_server_settings_button = CreateWindowExA(
        NULL, "Button", "Configure settings", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
        screen_width / 2 + 310, 8, 130, 28, hWnd, reinterpret_cast<HMENU>(ID_BUTTON_CONFIGURE_SERVER_SETTINGS),
        app_handles.hInstance, nullptr);

    app_handles.hwnd_button_view_rcon =
        CreateWindowExA(NULL, "Button", "&Switch to rcon view",
                        BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, screen_width / 2 + 460, 8, 140,
                        28, hWnd, reinterpret_cast<HMENU>(ID_RCONVIEWBUTTON), app_handles.hInstance, nullptr);

    app_handles.hwnd_button_players_view =
        CreateWindowExA(NULL, "Button", "Show players", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
                        screen_width / 2 + 170, 45, 120, 28, hWnd, reinterpret_cast<HMENU>(ID_SHOWPLAYERSBUTTON),
                        app_handles.hInstance, nullptr);

    app_handles.hwnd_refresh_players_data_button = CreateWindowExA(
        NULL, "Button", "&Refresh players", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
        screen_width / 2 + 310, 45, 130, 28, hWnd, reinterpret_cast<HMENU>(ID_REFRESH_PLAYERS_DATA_BUTTON),
        app_handles.hInstance, nullptr);

    app_handles.hwnd_button_game_servers_list_view =
        CreateWindowExA(NULL, "Button", "Show servers", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
                        screen_width / 2 + 460, 45, 140, 28, hWnd, reinterpret_cast<HMENU>(ID_SHOWSERVERSBUTTON),
                        app_handles.hInstance, nullptr);

    /*app_handles.hwnd_button_refresh_game_servers = CreateWindowExA(NULL,
     * "Button", "Refresh servers", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER |
     * WS_VISIBLE | WS_CHILD, screen_width / 2 + 620, 45, 120, 28, hWnd,
     * reinterpret_cast<HMENU>(ID_REFRESHSERVERSBUTTON), app_handles.hInstance,
     * nullptr);*/

    app_handles.hwnd_combo_box_sortmode = CreateWindowExA(
        NULL, "Combobox", nullptr, WS_GROUP | WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_HSCROLL,
        screen_width - 470, screen_height - 210, 260, 210, hWnd, reinterpret_cast<HMENU>(ID_COMBOBOX_SORTMODE),
        app_handles.hInstance, nullptr);

    SetWindowSubclass(app_handles.hwnd_combo_box_sortmode, ComboProc, 0, 0);

    app_handles.hwnd_combo_box_map = CreateWindowExA(
        NULL, "Combobox", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_HSCROLL,
        screen_width / 2 + 210, screen_height / 2 - 30, 150, 200, hWnd, reinterpret_cast<HMENU>(ID_COMBOBOX_MAP),
        app_handles.hInstance, nullptr);

    SetWindowSubclass(app_handles.hwnd_combo_box_map, ComboProc, 0, 0);

    app_handles.hwnd_combo_box_gametype = CreateWindowExA(
        NULL, "Combobox", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL | WS_HSCROLL,
        screen_width / 2 + 460, screen_height / 2 - 30, 60, 130, hWnd, reinterpret_cast<HMENU>(ID_COMBOBOX_GAMETYPE),
        app_handles.hInstance, nullptr);

    SetWindowSubclass(app_handles.hwnd_combo_box_gametype, ComboProc, 0, 0);

    app_handles.hwnd_button_load =
        CreateWindowExA(NULL, "Button", "Load map", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
                        screen_width / 2 + 540, screen_height / 2 - 33, 80, 25, hWnd,
                        reinterpret_cast<HMENU>(ID_LOADBUTTON), app_handles.hInstance, nullptr);

    if (app_handles.hwnd_upload_speed_info != nullptr)
    {
        ShowWindow(app_handles.hwnd_upload_speed_info, SW_HIDE);
        DestroyWindow(app_handles.hwnd_upload_speed_info);
    }

    app_handles.hwnd_upload_speed_info =
        CreateWindowEx(0, RICHEDIT_CLASS, nullptr, WS_VISIBLE | WS_CHILD | ES_RIGHT | ES_READONLY, screen_width - 640,
                       screen_height - 50, 630, 30, hWnd, nullptr, app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_match_information)
        FatalAppExit(0, "Couldn't create 'app_handles.hwnd_upload_speed_info' richedit control!");

    assert(app_handles.hwnd_upload_speed_info != nullptr);
    SendMessage(app_handles.hwnd_upload_speed_info, EM_SETBKGNDCOLOR, NULL, color::black);
    scroll_to_beginning(app_handles.hwnd_upload_speed_info);
    set_rich_edit_control_colors(app_handles.hwnd_upload_speed_info, color::white, color::black, "Lucida Console");
    print_colored_text(app_handles.hwnd_upload_speed_info, "^3Initializing and configuring ^5Tiny^6Rcon^3...",
                       is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);

    app_handles.hwnd_progress_bar =
        CreateWindowEx(0, PROGRESS_CLASS, nullptr, WS_CHILD | WS_VISIBLE | PBS_MARQUEE | PBS_SMOOTH, screen_width - 190,
                       screen_height - 210, 170, 23, hWnd, nullptr, app_handles.hInstance, nullptr);

    hImageList = ImageList_Create(32, 24, ILC_COLOR32, flag_name_index.size(), 1);
    for (size_t i{}; i < flag_name_index.size(); ++i)
    {
        auto hbmp = static_cast<HBITMAP>(LoadImage(app_handles.hInstance, MAKEINTRESOURCE(151 + i), IMAGE_BITMAP, 32,
                                                   24, LR_CREATEDIBSECTION | LR_DEFAULTSIZE));
        ImageList_Add(hImageList, hbmp, nullptr);
    }

    if (app_handles.hwnd_servers_grid)
    {
        DestroyWindow(app_handles.hwnd_servers_grid);
    }

    app_handles.hwnd_servers_grid =
        CreateWindowExA(WS_EX_CLIENTEDGE, WC_SIMPLEGRIDA, "",
                        /*WS_VISIBLE |*/ WS_CHILD, 10, 85, screen_width / 2 + 135, screen_height - 195, hWnd,
                        reinterpret_cast<HMENU>(502), app_handles.hInstance, nullptr);
    initialize_servers_grid(app_handles.hwnd_servers_grid, 8, max_servers_grid_rows);
    ShowWindow(app_handles.hwnd_servers_grid, SW_HIDE);

    if (app_handles.hwnd_players_grid)
    {
        DestroyWindow(app_handles.hwnd_players_grid);
    }

    app_handles.hwnd_players_grid =
        CreateWindowExA(WS_EX_CLIENTEDGE, WC_SIMPLEGRIDA, "", WS_VISIBLE | WS_CHILD, 10, 85, screen_width / 2 + 135,
                        screen_height - 195, hWnd, reinterpret_cast<HMENU>(501), app_handles.hInstance, nullptr);

    initialize_players_grid(app_handles.hwnd_players_grid, 7, max_players_grid_rows, true);

    for (const auto &[rcon_map_name, full_map_name] : main_app.get_available_rcon_to_full_map_names())
    {
        ComboBox_AddString(app_handles.hwnd_combo_box_map, full_map_name.first.c_str());
    }
    if (main_app.get_available_rcon_to_full_map_names().contains("mp_toujane"))
    {
        SendMessage(
            app_handles.hwnd_combo_box_map, CB_SELECTSTRING, static_cast<WPARAM>(-1),
            reinterpret_cast<LPARAM>(main_app.get_available_rcon_to_full_map_names().at("mp_toujane").first.c_str()));
    }

    const auto &full_gametype_names =
        get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());
    for (const auto &[short_gametype_name, long_gametype_name] : full_gametype_names)
    {
        SendMessage(app_handles.hwnd_combo_box_gametype, CB_ADDSTRING, 0,
                    reinterpret_cast<LPARAM>(short_gametype_name.c_str()));
    }

    set_available_sort_methods(true);

    SendMessage(app_handles.hwnd_combo_box_gametype, CB_SELECTSTRING, static_cast<WPARAM>(-1),
                reinterpret_cast<LPARAM>("ctf"));
}

void PutCell(HWND hgrid, const int row, const int col, const char *text)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SGITEM item{};
    item.row = row;
    item.col = col;
    item.lpCurValue = (LPARAM)text;
    SimpleGrid_SetItemData(hgrid, &item);
}

void display_country_flag(HWND hgrid, const int row, const int col, const char *country_code)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SGITEM item{};
    item.col = col;
    item.row = row;
    const int country_flag_index{flag_name_index.contains(country_code) ? flag_name_index.at(country_code) : 0};
    item.lpCurValue = (LPARAM)country_flag_index;
    SimpleGrid_SetItemData(hgrid, &item);
}

string GetCellContents(HWND hgrid, const int row, const int col)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char cell_buffer[256]{};
    SGITEM item{};
    item.row = row;
    item.col = col;
    item.lpCurValue = (LPARAM)cell_buffer;
    SimpleGrid_GetItemData(hgrid, &item);
    return string{cell_buffer};
}

void initialize_players_grid(HWND hgrid, const size_t cols, const size_t rows, const bool is_for_rcon_status)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    selected_player_row = 0;
    SimpleGrid_ResetContent(hgrid);
    SimpleGrid_SetAllowColResize(app_handles.hwnd_players_grid, TRUE);
    SimpleGrid_Enable(app_handles.hwnd_players_grid, TRUE);
    SimpleGrid_ExtendLastColumn(app_handles.hwnd_players_grid, TRUE);
    SimpleGrid_SetColAutoWidth(app_handles.hwnd_players_grid, TRUE);
    SimpleGrid_SetDoubleBuffer(app_handles.hwnd_players_grid, TRUE);
    SimpleGrid_SetEllipsis(app_handles.hwnd_players_grid, TRUE);
    SimpleGrid_SetGridLineColor(app_handles.hwnd_players_grid, colors.at(main_app.get_border_line_color()[1]));
    SimpleGrid_SetTitleHeight(app_handles.hwnd_players_grid, 0);
    SimpleGrid_SetHilightColor(app_handles.hwnd_players_grid, color::grey);
    SimpleGrid_SetHilightTextColor(app_handles.hwnd_players_grid, color::red);
    SimpleGrid_SetRowHeaderWidth(app_handles.hwnd_players_grid, 0);
    SimpleGrid_SetColsNumbered(app_handles.hwnd_players_grid, FALSE);
    SimpleGrid_SetRowHeight(app_handles.hwnd_players_grid, 28);

    for (size_t col_id{}; col_id < cols; ++col_id)
    {
        SGCOLUMN column{};
        column.dwType = GCT_EDIT;
        if (6 == col_id)
        {
            column.dwType = GCT_IMAGE;
            column.pOptional = hImageList;
        }
        column.lpszHeader = is_for_rcon_status ? rcon_status_grid_column_header_titles.at(col_id).c_str()
                                               : get_status_grid_column_header_titles.at(col_id).c_str();
        SimpleGrid_AddColumn(app_handles.hwnd_players_grid, &column);
    }

    for (size_t i{}; i < rows; ++i)
    {
        SimpleGrid_AddRow(hgrid, "");
    }

    for (size_t i{}; i < rows; ++i)
    {
        for (size_t j{}; j < cols; ++j)
        {
            if (6 == j)
            {
                SGITEM item{};
                item.row = i;
                item.col = j;
                item.lpCurValue = (LPARAM)0;
                SimpleGrid_SetItemData(hgrid, &item);
            }
            else
            {
                PutCell(hgrid, i, j, "");
            }
        }
    }

    Grid_OnSetFont(app_handles.hwnd_players_grid, font_for_players_grid_data, TRUE);
    ShowHscroll(app_handles.hwnd_players_grid);

    const int players_grid_width{screen_width / 2 + 130};

    SimpleGrid_SetColWidth(hgrid, 0, 50);
    SimpleGrid_SetColWidth(hgrid, 1, 70);
    SimpleGrid_SetColWidth(hgrid, 2, 60);
    SimpleGrid_SetColWidth(hgrid, 3, 270);
    if (is_for_rcon_status)
    {
        SimpleGrid_SetColWidth(hgrid, 4, 190);
        SimpleGrid_SetColWidth(hgrid, 5, players_grid_width - 690);
        SimpleGrid_SetColWidth(hgrid, 6, 50);
    }
    SimpleGrid_SetSelectionMode(app_handles.hwnd_players_grid, GSO_FULLROW);
}

void clear_players_data_in_players_grid(HWND hgrid, const size_t start_row, const size_t last_row, const size_t cols)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    for (size_t i{start_row}; i < last_row; ++i)
    {
        for (size_t j{}; j < cols; ++j)
        {
            if (6 == j)
            {
                display_country_flag(hgrid, i, 6, "xy");
            }
            else
            {
                PutCell(hgrid, i, j, "");
            }
        }
        displayed_players_data[i].pid[0] = '\0';
        displayed_players_data[i].score[0] = '\0';
        displayed_players_data[i].ping[0] = '\0';
        displayed_players_data[i].player_name[0] = '\0';
        displayed_players_data[i].ip_address[0] = '\0';
        displayed_players_data[i].geo_info[0] = '\0';
        displayed_players_data[i].country_code = "xy";
    }
}

void clear_servers_data_in_servers_grid(HWND hgrid, const size_t start_row, const size_t last_row, const size_t cols)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    bool stop_check{};
    for (size_t i{start_row}; !stop_check && i < last_row; ++i)
    {
        for (size_t j{}; j < cols; ++j)
        {
            if (7 == j)
            {
                display_country_flag(hgrid, i, 7, "xy");
            }
            else if (GetCellContents(hgrid, i, j)
                         .

                     empty()

            )
            {
                stop_check = true;
            }
            else
            {
                PutCell(hgrid, i, j, "");
            }
        }
    }
}

void display_players_data_in_players_grid(HWND hgrid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    if (me->is_admin)
    {
        display_online_admins_information();
    }
    else
    {
        SetWindowTextA(app_handles.hwnd_online_admins_information, "");
        print_colored_text(app_handles.hwnd_online_admins_information, online_admins_information.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
        SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);
    }

    SetWindowTextA(app_handles.hwnd_match_information, "");
    print_colored_text(app_handles.hwnd_match_information,
                       g_re_match_information_contents.

                       c_str(),
                       is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no

    );
    SendMessage(app_handles.hwnd_match_information, EM_SETSEL, 0, -1);
    SendMessage(app_handles.hwnd_match_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);

    const size_t number_of_players{main_app.get_current_game_server().get_number_of_players()};

    if (number_of_players > 0)
    {
        for (size_t i{}; i < number_of_players; ++i)
        {
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

    const int players_grid_width{screen_width / 2 + 130};
    const int player_name_column_width{findLongestTextWidthInColumn(hgrid, 3, 0, number_of_players)};
    SimpleGrid_SetColWidth(hgrid, 0, 50);
    SimpleGrid_SetColWidth(hgrid, 1, 70);
    SimpleGrid_SetColWidth(hgrid, 2, 60);
    SimpleGrid_SetColWidth(hgrid, 3, player_name_column_width);
    SimpleGrid_SetColWidth(hgrid, 4, 190);
    SimpleGrid_SetColWidth(hgrid, 5, players_grid_width - (player_name_column_width + 420));
    SimpleGrid_SetColWidth(hgrid, 6, 50);
    SimpleGrid_EnableEdit(hgrid, FALSE);
}

void display_game_servers_data_in_servers_grid(HWND hgrid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const size_t number_of_game_servers{main_app.get_game_servers_count()};

    if (number_of_game_servers > 0)
    {
        for (size_t i{}; i < number_of_game_servers; ++i)
        {
            auto &gs = main_app.get_game_servers()[i];
            PutCell(hgrid, i, 0, gs.get_server_pid().c_str());
            PutCell(hgrid, i, 1, gs.get_server_name().c_str());
            PutCell(hgrid, i, 2, gs.get_game_server_address().c_str());
            PutCell(hgrid, i, 3, gs.get_online_and_max_players().c_str());
            PutCell(hgrid, i, 4, gs.get_current_map().c_str());
            PutCell(hgrid, i, 5, gs.get_current_game_type().c_str());
            PutCell(hgrid, i, 6, gs.get_is_voice_enabled() ? "^2Yes" : "^1No");
            display_country_flag(hgrid, i, 7, gs.get_country_code());
        }
    }

    clear_servers_data_in_servers_grid(hgrid, number_of_game_servers, max_servers_grid_rows, 8);

    const int servers_grid_width{screen_width / 2 + 130};
    const int server_name_column_width{findLongestTextWidthInColumn(hgrid, 1, 0, number_of_game_servers, 100, 350)};
    SimpleGrid_SetColWidth(hgrid, 0, 40);
    SimpleGrid_SetColWidth(hgrid, 1, server_name_column_width);
    SimpleGrid_SetColWidth(hgrid, 2, 240);
    SimpleGrid_SetColWidth(hgrid, 3, 80);
    SimpleGrid_SetColWidth(hgrid, 4, servers_grid_width - (server_name_column_width + 510));
    SimpleGrid_SetColWidth(hgrid, 5, 50);
    SimpleGrid_SetColWidth(hgrid, 6, 50);
    SimpleGrid_SetColWidth(hgrid, 7, 50);
    SimpleGrid_EnableEdit(hgrid, FALSE);
}

void display_game_server_data_in_servers_grid(HWND hgrid, const size_t i)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (i < main_app.get_game_servers_count())
    {
        auto &gs = main_app.get_game_servers()[i];
        PutCell(hgrid, i, 0, gs.get_server_pid().c_str());
        PutCell(hgrid, i, 1, gs.get_server_name().c_str());
        PutCell(hgrid, i, 2, gs.get_game_server_address().c_str());
        PutCell(hgrid, i, 3, gs.get_online_and_max_players().c_str());
        PutCell(hgrid, i, 4, gs.get_current_map().c_str());
        PutCell(hgrid, i, 5, gs.get_current_game_type().c_str());
        PutCell(hgrid, i, 6, gs.get_is_voice_enabled() ? "^2Yes" : "^1No");
        display_country_flag(hgrid, i, 7, gs.get_country_code());
    }

    const int servers_grid_width{screen_width / 2 + 130};
    // const int server_name_column_width{ findLongestTextWidthInColumn(hgrid, 1,
    // 0, number_of_game_servers, 100, 350) };
    SimpleGrid_SetColWidth(hgrid, 0, 40);
    SimpleGrid_SetColWidth(hgrid, 1, 330);
    SimpleGrid_SetColWidth(hgrid, 2, 220);
    SimpleGrid_SetColWidth(hgrid, 3, 80);
    SimpleGrid_SetColWidth(hgrid, 4, servers_grid_width - 820);
    SimpleGrid_SetColWidth(hgrid, 5, 50);
    SimpleGrid_SetColWidth(hgrid, 6, 50);
    SimpleGrid_SetColWidth(hgrid, 7, 50);
    SimpleGrid_EnableEdit(hgrid, FALSE);
}

void initialize_servers_grid(HWND hgrid, const size_t cols, const size_t rows)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    selected_server_row = 0;
    main_app.set_game_server_index(0U);
    SimpleGrid_ResetContent(hgrid);
    SimpleGrid_SetAllowColResize(hgrid, TRUE);
    SimpleGrid_Enable(hgrid, TRUE);
    SimpleGrid_ExtendLastColumn(hgrid, TRUE);
    SimpleGrid_SetColAutoWidth(hgrid, TRUE);
    SimpleGrid_SetDoubleBuffer(hgrid, TRUE);
    SimpleGrid_SetEllipsis(hgrid, TRUE);
    SimpleGrid_SetGridLineColor(hgrid, colors.at(main_app.

                                                 get_border_line_color()[1]

                                                 ));
    SimpleGrid_SetTitleHeight(hgrid, 0);
    SimpleGrid_SetHilightColor(hgrid, color::grey);
    SimpleGrid_SetHilightTextColor(hgrid, color::red);
    SimpleGrid_SetRowHeaderWidth(hgrid, 0);
    SimpleGrid_SetColsNumbered(hgrid, FALSE);
    SimpleGrid_SetRowHeight(hgrid, 28);

    for (size_t col_id{}; col_id < cols; ++col_id)
    {
        SGCOLUMN column{};
        column.dwType = GCT_EDIT;
        if (7 == col_id)
        {
            column.dwType = GCT_IMAGE;
            column.pOptional = hImageList;
        }
        column.lpszHeader = servers_grid_column_header_titles.at(col_id).c_str();
        SimpleGrid_AddColumn(hgrid, &column);
    }

    for (size_t i{}; i < rows; ++i)
    {
        SimpleGrid_AddRow(hgrid, "");
    }

    for (size_t i{}; i < rows; ++i)
    {
        for (size_t j{}; j < cols; ++j)
        {
            if (7 == j)
            {
                SGITEM item{};
                item.row = i;
                item.col = j;
                item.lpCurValue = (LPARAM)0;
                SimpleGrid_SetItemData(hgrid, &item);
            }
            else
            {
                PutCell(hgrid, i, j, "");
            }
        }
    }

    Grid_OnSetFont(hgrid, font_for_players_grid_data, TRUE);
    ShowHscroll(hgrid);

    const int servers_grid_width{screen_width / 2 + 130};

    SimpleGrid_SetColWidth(hgrid, 0, 40);
    SimpleGrid_SetColWidth(hgrid, 1, 330);
    SimpleGrid_SetColWidth(hgrid, 2, 220);
    SimpleGrid_SetColWidth(hgrid, 3, 80);
    SimpleGrid_SetColWidth(hgrid, 4, servers_grid_width - 820);
    SimpleGrid_SetColWidth(hgrid, 5, 50);
    SimpleGrid_SetColWidth(hgrid, 6, 50);
    SimpleGrid_SetColWidth(hgrid, 7, 50);
    SimpleGrid_SetSelectionMode(hgrid, GSO_FULLROW);
    SimpleGrid_EnableEdit(hgrid, FALSE);
}

bool is_alpha(const char ch) noexcept
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    return !is_decimal_digit(ch) && !is_ws(ch);
}

bool is_decimal_digit(const char ch) noexcept
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    return ch >= '0' && ch <= '9';
}

bool is_printable(const char ch)
{
    static string printable_characters{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!\"#$%&'()*+"
                                       ",-./:;?@[\\]^_`{|}~ "};
    // static const unordered_set<char>
    // printable_characters_set(printable_characters, printable_characters +
    // strlen(printable_characters));
    return printable_characters.find(ch) != string::npos;
}

bool is_ws(const char ch) noexcept
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\f' || ch == '\v';
}

void change_hdc_fg_color(HDC hdc, COLORREF fg_color)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SetTextColor(hdc, fg_color);
}

bool check_if_selected_cell_indices_are_valid_for_players_grid(const int row_index, const int col_index)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    return row_index >= 0 && row_index < (int)main_app.get_current_game_server().get_number_of_players() &&
           col_index >= 0 && col_index < 7;
}

bool check_if_selected_cell_indices_are_valid_for_game_servers_grid(const int row_index, const int col_index)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    return row_index >= 0 && row_index < (int)main_app.get_game_servers_count() && col_index >= 0 && col_index < 8;
}

void CenterWindow(HWND hwnd)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    RECT rc{};

    GetWindowRect(hwnd, &rc);
    const int win_w{rc.right - rc.left};
    const int win_h{rc.bottom - rc.top};

    const int screen_w{GetSystemMetrics(SM_CXSCREEN)};
    const int screen_h{GetSystemMetrics(SM_CYSCREEN)};

    SetWindowPos(hwnd, HWND_TOP, (screen_w - win_w) / 2, (screen_h - win_h) / 2, 0, 0, SWP_NOSIZE);
}

bool show_user_confirmation_dialog(const char *msg, const char *title, const char *)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char msg_buffer[1024];
    admin_choice.store(0);
    if (app_handles.hwnd_confirmation_dialog)
    {
        DestroyWindow(app_handles.hwnd_confirmation_dialog);
    }
    app_handles.hwnd_confirmation_dialog =
        CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wcex_confirmation_dialog.lpszClassName, title,
                       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN /*& ~WS_MAXIMIZEBOX & ~WS_THICKFRAME &
                                                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
                                                                | WS_THICKFRAME | WS_MINIMIZEBOX*/
                       ,
                       0, 0, 600, 320, app_handles.hwnd_main_window, nullptr, app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_confirmation_dialog)
        return false;

    if (app_handles.hwnd_re_confirmation_message)
    {
        DestroyWindow(app_handles.hwnd_re_confirmation_message);
    }

    app_handles.hwnd_re_confirmation_message = CreateWindowExA(
        0, RICHEDIT_CLASSA, nullptr,
        WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_LEFT | ES_AUTOVSCROLL | ES_READONLY, 10, 10,
        560, 200, app_handles.hwnd_confirmation_dialog, nullptr, app_handles.hInstance, nullptr);
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

    if (app_handles.hwnd_e_reason)
    {
        DestroyWindow(app_handles.hwnd_e_reason);
    }
    app_handles.hwnd_e_reason =
        CreateWindowExA(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 100, 220, 470, 20,
                        app_handles.hwnd_confirmation_dialog, reinterpret_cast<HMENU>(ID_EDIT_ADMIN_REASON),
                        app_handles.hInstance, nullptr);

    if (app_handles.hwnd_yes_button)
    {
        DestroyWindow(app_handles.hwnd_yes_button);
    }
    app_handles.hwnd_yes_button = CreateWindowExA(
        NULL, "Button", "Yes", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 100, 250, 50, 20,
        app_handles.hwnd_confirmation_dialog, reinterpret_cast<HMENU>(ID_YES_BUTTON), app_handles.hInstance, nullptr);

    if (app_handles.hwnd_no_button)
    {
        DestroyWindow(app_handles.hwnd_no_button);
    }
    app_handles.hwnd_no_button = CreateWindowExA(
        NULL, "Button", "No", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 170, 250, 50, 20,
        app_handles.hwnd_confirmation_dialog, reinterpret_cast<HMENU>(ID_NO_BUTTON), app_handles.hInstance, nullptr);

    print_colored_text(app_handles.hwnd_re_confirmation_message, msg, is_append_message_to_richedit_control::yes,
                       is_log_message::yes, is_log_datetime::yes);
    SetWindowTextA(app_handles.hwnd_e_reason, admin_reason.c_str());
    CenterWindow(app_handles.hwnd_confirmation_dialog);
    ShowWindow(app_handles.hwnd_confirmation_dialog, SW_SHOW);
    SetFocus(app_handles.hwnd_no_button);

    MSG wnd_msg{};
    while (GetMessage(&wnd_msg, nullptr, NULL, NULL) != 0)
    {
        TranslateMessage(&wnd_msg);
        if (WM_KEYDOWN == wnd_msg.message && VK_ESCAPE == wnd_msg.wParam)
        {
            admin_choice.store(0);
            admin_reason.assign("not specified");
            EnableWindow(app_handles.hwnd_main_window, TRUE);
            SetFocus(app_handles.hwnd_main_window);
            DestroyWindow(app_handles.hwnd_confirmation_dialog);
        }
        else if (WM_KEYDOWN == wnd_msg.message && VK_RETURN == wnd_msg.wParam)
        {
            if (GetFocus() == app_handles.hwnd_no_button)
            {
                admin_choice.store(0);
                admin_reason.assign("not specified");
                EnableWindow(app_handles.hwnd_main_window, TRUE);
                SetFocus(app_handles.hwnd_main_window);
                DestroyWindow(app_handles.hwnd_confirmation_dialog);
            }
            else if (GetFocus() == app_handles.hwnd_yes_button)
            {
                admin_choice.store(1);
                GetWindowTextA(app_handles.hwnd_e_reason, msg_buffer, std::size(msg_buffer));
                if (stl::helper::len(msg_buffer) > 0)
                {
                    admin_reason.assign(msg_buffer);
                }
                else
                {
                    admin_reason.assign("not specified");
                }
                EnableWindow(app_handles.hwnd_main_window, TRUE);
                SetFocus(app_handles.hwnd_main_window);
                DestroyWindow(app_handles.hwnd_confirmation_dialog);
            }
        }
        else if (WM_KEYDOWN == wnd_msg.message && VK_LEFT == wnd_msg.wParam)
        {
            const auto focused_hwnd = GetFocus();
            if (focused_hwnd == app_handles.hwnd_yes_button)
                SetFocus(app_handles.hwnd_no_button);
            else if (focused_hwnd == app_handles.hwnd_no_button)
                SetFocus(app_handles.hwnd_yes_button);
        }
        else if (WM_KEYDOWN == wnd_msg.message && VK_TAB == wnd_msg.wParam)
        {
            const auto focused_hwnd = GetFocus();
            if (focused_hwnd == app_handles.hwnd_yes_button)
                SetFocus(app_handles.hwnd_no_button);
            else if (focused_hwnd == app_handles.hwnd_no_button)
                SetFocus(app_handles.hwnd_yes_button);
        }
        else if (WM_KEYDOWN == wnd_msg.message && VK_RIGHT == wnd_msg.wParam)
        {
            const auto focused_hwnd = GetFocus();
            if (focused_hwnd == app_handles.hwnd_yes_button)
                SetFocus(app_handles.hwnd_no_button);
            else if (focused_hwnd == app_handles.hwnd_no_button)
                SetFocus(app_handles.hwnd_yes_button);
        }
        else if (WM_LBUTTONDOWN == wnd_msg.message && is_first_left_mouse_button_click_in_reason_edit_control)
        {
            const int x{GET_X_LPARAM(wnd_msg.lParam)};
            const int y{GET_Y_LPARAM(wnd_msg.lParam)};
            RECT rect{};
            GetClientRect(app_handles.hwnd_e_reason, &rect);
            if (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom)
            {
                SetWindowTextA(app_handles.hwnd_e_reason, "");
                admin_reason.assign("not specified");
                is_first_left_mouse_button_click_in_reason_edit_control = false;
            }
        }
        DispatchMessage(&wnd_msg);
    }

    return wnd_msg.wParam != 0;
}

void process_sort_type_change_request(const sort_type new_sort_type)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    type_of_sort = new_sort_type;

    switch (type_of_sort)
    {
    case sort_type::pid_asc:
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^2Executing ^1!sort pid asc ^2command and refreshing players data.\n");
        break;
    case sort_type::pid_desc:
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort pid desc ^2command and "
                                                              "refreshing players data.\n");
        break;
    case sort_type::score_asc:
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort score asc ^2command and "
                                                              "refreshing players data.\n");
        break;
    case sort_type::score_desc:
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort score desc ^2command and "
                                                              "refreshing players data.\n");
        break;
    case sort_type::ping_asc:
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort ping asc ^2command and "
                                                              "refreshing players data.\n");
        break;
    case sort_type::ping_desc:
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort ping desc ^2command and "
                                                              "refreshing players data.\n");
        break;
    case sort_type::name_asc:
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort name asc ^2command and "
                                                              "refreshing players data.\n");
        break;
    case sort_type::name_desc:
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort name desc ^2command and "
                                                              "refreshing players data.\n");
        break;
    case sort_type::ip_asc:
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^2Executing ^1!sort ip asc ^2command and refreshing players data.\n");
        break;
    case sort_type::ip_desc:
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^2Executing ^1!sort ip desc ^2command and refreshing players data.\n");
        break;
    case sort_type::geo_asc:
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^2Executing ^1!sort geo asc ^2command and refreshing players data.\n");
        break;
    case sort_type::geo_desc:
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Executing ^1!sort geo desc ^2command and "
                                                              "refreshing players data.\n");
        break;
    }

    ComboBox_SelectString(app_handles.hwnd_combo_box_sortmode, 0,
                          sort_type_to_sort_names_dict.at(type_of_sort).c_str());
    initiate_sending_rcon_status_command_now();
    is_process_combobox_item_selection_event = true;
}

void update_game_server_setting(game_server &gs, std::string key, std::string value)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string ex_msg{format(R"(^1Exception ^3thrown from ^1void update_game_server_setting("{}", "{}"))", key, value)};
    stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

    if (key == "pswrd")
    {
        gs.set_is_password_protected('0' != value[0]);
    }
    else if (key == "protocol")
    {
        gs.set_protocol_info(stoi(value));
    }
    else if (key == "hostname")
    {
        // if (!is_rcon_game_server(gs))
        gs.set_server_name(value);
    }
    else if (key == "mapname")
    {
        gs.set_current_map(value);
    }
    else if (key == "clients")
    {
        gs.set_number_of_players(stoi(value));
    }
    else if (key == "gametype")
    {
        gs.set_current_game_type(value);
    }
    else if (key == "is_pure")
    {
        gs.set_is_pure('0' != value[0]);
    }
    else if (key == "max_ping")
    {
        gs.set_max_ping(stoi(value));
    }
    else if (key == "game")
    {
        gs.set_game_mod_name(value);
    }
    else if (key == "con_disabled")
    {
        gs.set_is_console_disabled('0' != value[0]);
    }
    else if (key == "kc")
    {
        gs.set_is_kill_cam_enabled('0' != value[0]);
    }
    else if (key == "hw")
    {
        gs.set_hw_info(stoi(value));
    }
    else if (key == "mod")
    {
        gs.set_is_mod_enabled('0' != value[0]);
    }
    else if (key == "voice")
    {
        gs.set_is_voice_enabled('0' != value[0]);
    }
    else if (key == "fs_game")
    {
        gs.set_game_mod_name(value);
        if (!value.empty() && value != "main")
            gs.set_is_mod_enabled(true);
        else
            gs.set_is_mod_enabled(false);
    }
    else if (key == "g_antilag")
    {
        gs.set_is_anti_lag_enabled('1' == value[0]);
    }
    else if (key == "g_gametype")
    {
        gs.set_current_game_type(value);
    }
    else if (key == "gamename")
    {
        gs.set_game_name(value);
    }
    else if (key == "scr_friendlyfire")
    {
        gs.set_is_friendly_fire_enabled('0' != value[0]);
    }
    else if (key == "scr_killcam")
    {
        gs.set_is_kill_cam_enabled('0' != value[0]);
    }
    else if (key == "shortversion")
    {
        gs.set_server_short_version(value);
    }
    else if (key == "sv_allowAnonymous")
    {
        gs.set_is_allow_anonymous_players('0' != value[0]);
    }
    else if (key == "sv_floodProtect")
    {
        gs.set_is_flood_protected('0' != value[0]);
    }
    else if (key == "sv_hostname")
    {
        // if (!is_rcon_game_server(gs))
        gs.set_server_name(value);
    }
    else if (key == "sv_maxclients")
    {
        gs.set_max_number_of_players(stoi(value));
    }
    else if (key == "sv_maxPing")
    {
        gs.set_max_ping(stoi(value));
    }
    else if (key == "sv_maxRate")
    {
        gs.set_max_server_rate(stoi(value));
    }
    else if (key == "sv_minPing")
    {
        gs.set_min_ping(stoi(value));
    }
    else if (key == "sv_privateClients")
    {
        gs.set_max_private_clients(stoi(value));
    }
    else if (key == "sv_pure")
    {
        gs.set_is_pure('0' != value[0]);
    }
    else if (key == "sv_voice")
    {
        gs.set_is_voice_enabled('0' != value[0]);
    }

    main_app.get_server_settings().emplace(move(key), move(value));
}

std::pair<bool, game_name_t> check_if_specified_server_ip_port_and_rcon_password_are_valid(
    const char *ip_address, const uint_least16_t port_number, const char *rcon_password)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    connection_manager cm;
    string reply;
    main_app.get_connection_manager().send_and_receive_rcon_data(
        "version", reply, ip_address, port_number, rcon_password, main_app.get_current_game_server(), true);

    if (str_contains(reply, "Invalid password.", 0u, true) || str_contains(reply, "rconpassword", 0u, true) ||
        str_contains(reply, "Bad rcon", 0u, true))
    {
        main_app.get_connection_manager().send_and_receive_non_rcon_data("getstatus", reply, ip_address, port_number,
                                                                         main_app.get_current_game_server(), true);
        const string &gn{main_app.get_current_game_server().get_game_name()};
        return {false, game_name_to_game_name_t.contains(gn) ? game_name_to_game_name_t.at(gn) : game_name_t::unknown};
    }

    if (stl::helper::str_contains(reply, "CoD1", 0, true))
        return {true, game_name_t::cod1};
    if (stl::helper::str_contains(reply, "CoD2", 0, true))
        return {true, game_name_t::cod2};
    if (stl::helper::str_contains(reply, "CoD4", 0, true))
        return {true, game_name_t::cod4};
    if (stl::helper::str_contains(reply, "CoD5", 0, true))
        return {true, game_name_t::cod5};

    return {true, game_name_t::unknown};
}

bool show_and_process_tinyrcon_configuration_panel(const char *title)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    is_terminate_tinyrcon_settings_configuration_dialog_window.store(false);
    if (app_handles.hwnd_configuration_dialog)
    {
        DestroyWindow(app_handles.hwnd_configuration_dialog);
    }
    app_handles.hwnd_configuration_dialog =
        CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wcex_configuration_dialog.lpszClassName, title,
                       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN /*& ~WS_MAXIMIZEBOX & ~WS_THICKFRAME &
                                                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
                                                                | WS_THICKFRAME | WS_MINIMIZEBOX*/
                       ,
                       0, 0, 920, 730, app_handles.hwnd_main_window, nullptr, app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_configuration_dialog)
        return false;

    (void)CreateWindowExA(0, "Static", "Admin name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 10, 120, 20,
                          app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    if (app_handles.hwnd_user_name)
    {
        DestroyWindow(app_handles.hwnd_user_name);
    }

    app_handles.hwnd_user_name = CreateWindowExA(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220,
                                                 10, 450, 20, app_handles.hwnd_configuration_dialog,
                                                 (HMENU)ID_EDIT_ADMIN_NAME, app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_user_name)
        return false;

    (void)CreateWindowExA(0, "Static", "Server name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 40, 120, 20,
                          app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    if (app_handles.hwnd_server_name)
    {
        DestroyWindow(app_handles.hwnd_server_name);
    }

    app_handles.hwnd_server_name = CreateWindowExA(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220,
                                                   40, 450, 20, app_handles.hwnd_configuration_dialog,
                                                   (HMENU)ID_EDIT_SERVER_NAME, app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_server_name)
        return false;

    if (app_handles.hwnd_server_ip_address)
    {
        DestroyWindow(app_handles.hwnd_server_ip_address);
    }
    (void)CreateWindowExA(0, "Static", "Server IP address:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 70, 130, 20,
                          app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_server_ip_address = CreateWindowExA(
        0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220, 70, 450, 20,
        app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_SERVER_IP, app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_server_ip_address)
        return false;

    if (app_handles.hwnd_server_port)
    {
        DestroyWindow(app_handles.hwnd_server_port);
    }
    (void)CreateWindowExA(0, "Static", "Server port:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 100, 120, 20,
                          app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_server_port = CreateWindowExA(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT, 220,
                                                   100, 450, 20, app_handles.hwnd_configuration_dialog,
                                                   (HMENU)ID_EDIT_SERVER_PORT, app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_server_port)
        return false;

    if (app_handles.hwnd_rcon_password)
    {
        DestroyWindow(app_handles.hwnd_rcon_password);
    }
    (void)CreateWindowExA(0, "Static", "Rcon password:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 130, 140, 20,
                          app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_rcon_password = CreateWindowExA(0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                                                     220, 130, 450, 20, app_handles.hwnd_configuration_dialog,
                                                     (HMENU)ID_EDIT_SERVER_RCON, app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_rcon_password)
        return false;

    if (app_handles.hwnd_enable_city_ban)
    {
        DestroyWindow(app_handles.hwnd_enable_city_ban);
    }

    if (app_handles.hwnd_enable_country_ban)
    {
        DestroyWindow(app_handles.hwnd_enable_country_ban);
    }

    (void)CreateWindowExA(0, "Static", "Enable city ban feature:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 160, 180, 20,
                          app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_enable_city_ban = CreateWindowExA(
        WS_EX_TRANSPARENT, "BUTTON", nullptr, WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 210, 160, 15, 20,
        app_handles.hwnd_configuration_dialog, (HMENU)ID_ENABLE_CITY_BAN_CHECKBOX, app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_enable_city_ban)
        return false;

    (void)CreateWindowExA(0, "Static", "Enable country ban feature:", WS_CHILD | WS_VISIBLE | SS_LEFT, 260, 160, 180,
                          20, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_enable_country_ban = CreateWindowExA(
        WS_EX_TRANSPARENT, "BUTTON", nullptr, WS_VISIBLE | WS_CHILD | BS_CHECKBOX, 480, 160, 15, 20,
        app_handles.hwnd_configuration_dialog, (HMENU)ID_ENABLE_COUNTRY_BAN_CHECKBOX, app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_enable_country_ban)
        return false;

    if (app_handles.hwnd_cod1_path_edit)
    {
        DestroyWindow(app_handles.hwnd_cod1_path_edit);
    }
    (void)CreateWindowExA(0, "Static", "Call of Duty installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 195, 200,
                          25, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_cod1_path_edit = CreateWindowExA(
        0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL,
        220, 190, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD1_PATH,
        app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_cod1_path_edit)
        return false;

    SendMessage(app_handles.hwnd_cod1_path_edit, EM_SETLIMITTEXT, 1024, 0);

    if (app_handles.hwnd_cod1_path_button)
    {
        DestroyWindow(app_handles.hwnd_cod1_path_button);
    }
    app_handles.hwnd_cod1_path_button = CreateWindowExA(
        NULL, "Button", "Browse for codmp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680,
        200, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD1_PATH,
        app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_cod1_path_button)
        return false;

    if (app_handles.hwnd_cod2_path_edit)
    {
        DestroyWindow(app_handles.hwnd_cod2_path_edit);
    }
    (void)CreateWindowExA(0, "Static", "Call of Duty 2 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 245,
                          200, 25, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_cod2_path_edit = CreateWindowExA(
        0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL,
        220, 240, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD2_PATH,
        app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_cod2_path_edit)
        return false;

    if (app_handles.hwnd_cod2_path_button)
    {
        DestroyWindow(app_handles.hwnd_cod2_path_button);
    }
    app_handles.hwnd_cod2_path_button = CreateWindowExA(
        NULL, "Button", "Browse for cod2mp_s.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680,
        250, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD2_PATH,
        app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_cod2_path_button)
        return false;

    if (app_handles.hwnd_cod4_path_edit)
    {
        DestroyWindow(app_handles.hwnd_cod4_path_edit);
    }
    (void)CreateWindowExA(0, "Static", "Call of Duty 4 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 295,
                          200, 25, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_cod4_path_edit = CreateWindowExA(
        0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL,
        220, 290, 450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD4_PATH,
        app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_cod4_path_edit)
        return false;

    if (app_handles.hwnd_cod4_path_button)
    {
        DestroyWindow(app_handles.hwnd_cod4_path_button);
    }
    app_handles.hwnd_cod4_path_button = CreateWindowExA(
        NULL, "Button", "Browse for iw3mp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680,
        300, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD4_PATH,
        app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_cod4_path_button)
        return false;

    if (app_handles.hwnd_cod5_path_edit)
    {
        DestroyWindow(app_handles.hwnd_cod5_path_edit);
    }
    (void)CreateWindowExA(0, "Static", "Call of Duty 5 installation path:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 345,
                          200, 25, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);

    app_handles.hwnd_cod5_path_edit = CreateWindowExA(
        0, "Edit", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOHSCROLL | WS_HSCROLL, 220, 340,
        450, 40, app_handles.hwnd_configuration_dialog, (HMENU)ID_EDIT_CONFIGURATION_COD5_PATH, app_handles.hInstance,
        nullptr);

    if (!app_handles.hwnd_cod5_path_edit)
        return false;

    if (app_handles.hwnd_cod5_path_button)
    {
        DestroyWindow(app_handles.hwnd_cod5_path_button);
    }
    app_handles.hwnd_cod5_path_button = CreateWindowExA(
        NULL, "Button", "Browse for cod5mp.exe", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 680,
        350, 190, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CONFIGURATION_COD5_PATH,
        app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_cod5_path_button)
        return false;

    if (app_handles.hwnd_re_confirmation_message)
    {
        DestroyWindow(app_handles.hwnd_re_confirmation_message);
    }

    app_handles.hwnd_re_confirmation_message = CreateWindowExA(
        0, RICHEDIT_CLASSA, nullptr,
        WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_LEFT | ES_AUTOVSCROLL | ES_READONLY, 220,
        390, 450, 260, app_handles.hwnd_configuration_dialog, nullptr, app_handles.hInstance, nullptr);
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

    if (app_handles.hwnd_save_settings_button)
    {
        DestroyWindow(app_handles.hwnd_save_settings_button);
    }
    app_handles.hwnd_save_settings_button = CreateWindowExA(
        NULL, "Button", "Save changes", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 220, 660, 120,
        25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_SAVE_CHANGES, app_handles.hInstance, nullptr);
    if (!app_handles.hwnd_save_settings_button)
        return false;

    if (app_handles.hwnd_test_connection_button)
    {
        DestroyWindow(app_handles.hwnd_test_connection_button);
    }

    app_handles.hwnd_test_connection_button = CreateWindowExA(
        NULL, "Button", "Test connection", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 360, 660,
        140, 25, app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_TEST_CONNECTION, app_handles.hInstance,
        nullptr);

    if (!app_handles.hwnd_test_connection_button)
        return false;

    if (app_handles.hwnd_close_button)
    {
        DestroyWindow(app_handles.hwnd_close_button);
    }
    app_handles.hwnd_close_button = CreateWindowExA(
        NULL, "Button", "Cancel", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD, 520, 660, 60, 25,
        app_handles.hwnd_configuration_dialog, (HMENU)ID_BUTTON_CANCEL, app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_close_button)
        return false;

    if (app_handles.hwnd_exit_tinyrcon_button)
    {
        DestroyWindow(app_handles.hwnd_exit_tinyrcon_button);
    }
    app_handles.hwnd_exit_tinyrcon_button =
        CreateWindowExA(NULL, "Button", "Exit TinyRcon", BS_PUSHBUTTON | BS_CENTER | BS_VCENTER | WS_VISIBLE | WS_CHILD,
                        600, 660, 120, 25, app_handles.hwnd_configuration_dialog,
                        reinterpret_cast<HMENU>(ID_BUTTON_CONFIGURATION_EXIT_TINYRCON), app_handles.hInstance, nullptr);

    if (!app_handles.hwnd_exit_tinyrcon_button)
        return false;

    SetWindowTextA(app_handles.hwnd_user_name, main_app.get_username().c_str());
    SetWindowTextA(app_handles.hwnd_server_name, main_app.get_game_server_name().c_str());
    SetWindowTextA(app_handles.hwnd_server_ip_address,
                   main_app.get_current_game_server().get_server_ip_address().c_str());
    char buffer_port[8];
    (void)snprintf(buffer_port, std::size(buffer_port), "%d", main_app.get_current_game_server().get_server_port());
    SetWindowTextA(app_handles.hwnd_server_port, buffer_port);
    SetWindowTextA(app_handles.hwnd_rcon_password, main_app.get_current_game_server().get_rcon_password().c_str());

    /*SendMessage(app_handles.hwnd_enable_city_ban, BM_SETCHECK,
  main_app.get_game_server().get_is_automatic_city_kick_enabled() ? BST_CHECKED :
  BST_UNCHECKED, 0); SendMessage(app_handles.hwnd_enable_country_ban, BM_SETCHECK,
  main_app.get_game_server().get_is_automatic_country_kick_enabled() ? BST_CHECKED
  : BST_UNCHECKED, 0);*/

    CheckDlgButton(app_handles.hwnd_configuration_dialog, ID_ENABLE_CITY_BAN_CHECKBOX,
                   main_app.get_is_automatic_city_kick_enabled() ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(app_handles.hwnd_configuration_dialog, ID_ENABLE_COUNTRY_BAN_CHECKBOX,
                   main_app.get_is_automatic_country_kick_enabled() ? BST_CHECKED : BST_UNCHECKED);

    const string cod1_steam_applaunch_id{format("-applaunch {}", cod1_steam_appid)};

    if (!main_app.get_codmp_exe_path().empty() &&
        (main_app.get_codmp_exe_path().find(cod1_steam_applaunch_id) != wstring::npos ||
         check_if_file_path_exists(main_app.get_codmp_exe_path().c_str())))
    {
        SetWindowText(app_handles.hwnd_cod1_path_edit, main_app.get_codmp_exe_path().c_str());
    }
    else
    {
        const string found_cod1_path{find_call_of_duty_1_installation_path(false)};
        if (!found_cod1_path.empty() && (found_cod1_path.find(cod1_steam_applaunch_id) != wstring::npos ||
                                         check_if_file_path_exists(found_cod1_path.c_str())))
        {
            SetWindowText(app_handles.hwnd_cod1_path_edit, found_cod1_path.c_str());
        }
        else
        {
            SetWindowText(app_handles.hwnd_cod1_path_edit, "");
        }
    }

    const string cod2_steam_applaunch_id{format("-applaunch {}", cod2_steam_appid)};

    if (!main_app.get_cod2mp_exe_path().empty() &&
        (main_app.get_cod2mp_exe_path().find(cod2_steam_applaunch_id) != wstring::npos ||
         check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str())))
    {
        SetWindowText(app_handles.hwnd_cod2_path_edit, main_app.get_cod2mp_exe_path().c_str());
    }
    else
    {
        const string found_cod2_path{find_call_of_duty_2_installation_path(false)};
        if (!found_cod2_path.empty() && (found_cod2_path.find(cod2_steam_applaunch_id) != wstring::npos ||
                                         check_if_file_path_exists(found_cod2_path.c_str())))
        {
            SetWindowText(app_handles.hwnd_cod2_path_edit, found_cod2_path.c_str());
        }
        else
        {
            SetWindowText(app_handles.hwnd_cod2_path_edit, "");
        }
    }

    const string cod4_steam_applaunch_id{format("-applaunch {}", cod4_steam_appid)};

    if (!main_app.get_iw3mp_exe_path().empty() &&
        (main_app.get_iw3mp_exe_path().find(cod4_steam_applaunch_id) != wstring::npos ||
         check_if_file_path_exists(main_app.get_iw3mp_exe_path().c_str())))
    {
        SetWindowText(app_handles.hwnd_cod4_path_edit, main_app.get_iw3mp_exe_path().c_str());
    }
    else
    {

        const string found_cod4_path{find_call_of_duty_4_installation_path(false)};
        if (!found_cod4_path.empty() && (found_cod4_path.find(cod4_steam_applaunch_id) != wstring::npos ||
                                         check_if_file_path_exists(found_cod4_path.c_str())))
        {
            SetWindowText(app_handles.hwnd_cod4_path_edit, found_cod4_path.c_str());
        }
        else
        {
            SetWindowText(app_handles.hwnd_cod4_path_edit, "");
        }
    }

    const string cod5_steam_applaunch_id{format("-applaunch {}", cod5_steam_appid)};

    if (!main_app.get_cod5mp_exe_path().empty() &&
        (main_app.get_cod5mp_exe_path().find(cod5_steam_applaunch_id) != wstring::npos ||
         check_if_file_path_exists(main_app.get_cod5mp_exe_path().c_str())))
    {
        SetWindowText(app_handles.hwnd_cod5_path_edit, main_app.get_cod5mp_exe_path().c_str());
    }
    else
    {
        const string found_cod5_path{find_call_of_duty_5_installation_path(false)};
        if (!found_cod5_path.empty() && (found_cod5_path.find(cod5_steam_applaunch_id) != wstring::npos ||
                                         check_if_file_path_exists(found_cod5_path.c_str())))
        {
            SetWindowText(app_handles.hwnd_cod5_path_edit, found_cod5_path.c_str());
        }
        else
        {
            SetWindowText(app_handles.hwnd_cod5_path_edit, "");
        }
    }

    print_colored_text(app_handles.hwnd_re_confirmation_message,
                       "^2Please enter and verify the correctness of your game server's input "
                       "settings by clicking on the ^3Test connection ^2button.\n",
                       is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    CenterWindow(app_handles.hwnd_configuration_dialog);
    ShowWindow(app_handles.hwnd_configuration_dialog, SW_SHOW);
    SetFocus(app_handles.hwnd_close_button);

    MSG wnd_msg{};
    while (GetMessage(&wnd_msg, nullptr, NULL, NULL) != 0)
    {
        if (WM_KEYDOWN == wnd_msg.message && VK_ESCAPE == wnd_msg.wParam)
        {
            EnableWindow(app_handles.hwnd_main_window, TRUE);
            SetFocus(app_handles.hwnd_main_window);
            DestroyWindow(app_handles.hwnd_configuration_dialog);
        }
        else if (WM_KEYDOWN == wnd_msg.message && VK_RETURN == wnd_msg.wParam)
        {
            const auto focused_hwnd = GetFocus();
            if (focused_hwnd == app_handles.hwnd_close_button)
            {
                is_terminate_tinyrcon_settings_configuration_dialog_window.store(true);
                EnableWindow(app_handles.hwnd_main_window, TRUE);
                SetFocus(app_handles.hwnd_main_window);
                DestroyWindow(app_handles.hwnd_configuration_dialog);
            }
            else if (focused_hwnd == app_handles.hwnd_test_connection_button)
            {
                process_button_test_connection_click_event(app_handles.hwnd_configuration_dialog);
            }
            else if (focused_hwnd == app_handles.hwnd_save_settings_button)
            {
                process_button_save_changes_click_event(app_handles.hwnd_configuration_dialog);
            }
            else if (focused_hwnd == app_handles.hwnd_exit_tinyrcon_button)
            {
                EnableWindow(app_handles.hwnd_main_window, TRUE);
                SetFocus(app_handles.hwnd_main_window);
                DestroyWindow(app_handles.hwnd_configuration_dialog);
                is_terminate_program.store(true);
            }
        }
        else if (WM_KEYDOWN == wnd_msg.message && VK_TAB == wnd_msg.wParam)
        {
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
        else if (WM_KEYDOWN == wnd_msg.message && VK_LEFT == wnd_msg.wParam)
        {
            const auto focused_hwnd = GetFocus();
            if (focused_hwnd == app_handles.hwnd_save_settings_button)
                SetFocus(app_handles.hwnd_exit_tinyrcon_button);
            else if (focused_hwnd == app_handles.hwnd_test_connection_button)
                SetFocus(app_handles.hwnd_save_settings_button);
            else if (focused_hwnd == app_handles.hwnd_close_button)
                SetFocus(app_handles.hwnd_test_connection_button);
            else if (focused_hwnd == app_handles.hwnd_exit_tinyrcon_button)
                SetFocus(app_handles.hwnd_close_button);
        }
        else if (WM_KEYDOWN == wnd_msg.message && VK_RIGHT == wnd_msg.wParam)
        {
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
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char msg_buffer[1024];
    string new_user_name;
    string new_server_name;
    string new_server_ip;
    uint_least16_t new_port{};
    string new_rcon_password;
    int new_refresh_time_period{5};
    bool is_invalid_entry{};

    GetWindowText(app_handles.hwnd_user_name, msg_buffer, std::size(msg_buffer));
    if (stl::helper::len(msg_buffer) > 0)
    {
        new_user_name.assign(msg_buffer);
    }
    else
    {
        is_invalid_entry = true;
        print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Admin name cannot be left empty!\n",
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }

    if (!is_invalid_entry)
    {

        GetWindowText(app_handles.hwnd_server_name, msg_buffer, std::size(msg_buffer));
        if (stl::helper::len(msg_buffer) > 0)
        {
            new_server_name.assign(msg_buffer);
        }
        else
        {
            is_invalid_entry = true;
            print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Server name cannot be left empty!\n",
                               is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        }
    }

    if (!is_invalid_entry)
    {
        GetWindowText(app_handles.hwnd_server_ip_address, msg_buffer, std::size(msg_buffer));
        const auto ip_len{stl::helper::len(msg_buffer)};
        unsigned long ip_key{};
        const string server_ip{msg_buffer};
        if (ip_len > 0 && check_ip_address_validity(server_ip, ip_key))
        {
            new_server_ip.assign(server_ip);
        }
        else
        {
            is_invalid_entry = true;
            const string user_message{"^1You have entered an invalid IP address! ^5("s + msg_buffer + ")\n"s};
            print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        }
    }

    if (!is_invalid_entry)
    {

        GetWindowText(app_handles.hwnd_server_port, msg_buffer, std::size(msg_buffer));
        if (int number{}; stl::helper::len(msg_buffer) > 0 && is_valid_decimal_whole_number(msg_buffer, number) &&
                          number >= 1024 && number <= 65535)
        {
            new_port = static_cast<uint_least16_t>(number);
        }
        else
        {
            is_invalid_entry = true;
            const string user_message{"^1You have entered an invalid server port number! ^5("s + msg_buffer +
                                      ")\n^1Port number ^3must be >= ^11024 ^3and <= ^165535\n"s};
            print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        }
    }

    if (!is_invalid_entry)
    {

        GetWindowText(app_handles.hwnd_rcon_password, msg_buffer, std::size(msg_buffer));
        if (stl::helper::len(msg_buffer) > 3)
        {
            new_rcon_password.assign(msg_buffer);
        }
        else
        {
            is_invalid_entry = true;
            const string user_message{"^1You have entered an invalid rcon password! ^5("s + msg_buffer +
                                      ")\n^1Rcon password ^3must be at least ^14 characters ^3long.\n"s};
            print_colored_text(app_handles.hwnd_re_confirmation_message, user_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        }
    }

    if (!is_invalid_entry)
    {
        (void)snprintf(msg_buffer, std::size(msg_buffer),
                       "\n^2Testing connection with the specified game server (^5%s^2) at "
                       "^5%s:%d ^2using the following ^5Tiny^6Rcon ^2settings:\n^1Admin name: "
                       "^5%s\n^1Server name: ^5%s\n^1Server IP address: ^5%s\n^1Server port "
                       "number: ^5%d\n^1Server rcon password: ^5%s\n^1Refresh time period: ^5%d "
                       "second(s)\n",
                       new_server_name.c_str(), new_server_ip.c_str(), new_port, new_user_name.c_str(),
                       new_server_name.c_str(), new_server_ip.c_str(), new_port, new_rcon_password.c_str(),
                       new_refresh_time_period);
        print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer,
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        auto [test_result, game_name] = check_if_specified_server_ip_port_and_rcon_password_are_valid(
            new_server_ip.c_str(), new_port, new_rcon_password.c_str());
        if (test_result)
        {
            main_app.get_current_game_server().set_is_connection_settings_valid(true);
            set_admin_actions_buttons_active(TRUE, false);
            print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Testing connection SUCCEEDED!\n",
                               is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        }
        else
        {
            main_app.get_current_game_server().set_is_connection_settings_valid(false);
            set_admin_actions_buttons_active(FALSE, false);
            print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Testing connection FAILED!\n",
                               is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        }

        (void)snprintf(msg_buffer, std::size(msg_buffer),
                       "\n^2Saving the following ^5Tiny^6Rcon ^2settings:\n^1Admin "
                       "name: ^5%s\n^1Server name: ^5%s\n^1Server IP address: "
                       "^5%s\n^1Server port number: ^5%d\n^1Server rcon password: "
                       "^5%s\n^1Refresh time period: ^5%d\n",
                       new_user_name.c_str(), new_server_name.c_str(), new_server_ip.c_str(), new_port,
                       new_rcon_password.c_str(), new_refresh_time_period);
        print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer,
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);

        main_app.set_username(std::move(new_user_name));
        main_app.set_game_name(game_name);
        main_app.set_game_server_name(std::move(new_server_name));
        main_app.get_current_game_server().set_server_ip_address(std::move(new_server_ip));
        main_app.get_current_game_server().set_server_port(new_port);
        main_app.get_current_game_server().set_rcon_password(std::move(new_rcon_password));
        main_app.set_check_for_banned_players_time_period(new_refresh_time_period);

        GetWindowText(app_handles.hwnd_cod1_path_edit, path_buffer, std::size(path_buffer));
        trim_in_place(path_buffer);
        if (str_contains(path_buffer, format("-applaunch {}", cod1_steam_appid).c_str(), 0U, true) ||
            check_if_file_path_exists(path_buffer))
        {
            main_app.set_codmp_exe_path(path_buffer);
        }

        GetWindowText(app_handles.hwnd_cod2_path_edit, path_buffer, std::size(path_buffer));
        trim_in_place(path_buffer);
        if (str_contains(path_buffer, format("-applaunch {}", cod2_steam_appid).c_str(), 0U, true) ||
            check_if_file_path_exists(path_buffer))
        {
            main_app.set_cod2mp_exe_path(path_buffer);
        }

        GetWindowText(app_handles.hwnd_cod4_path_edit, path_buffer, std::size(path_buffer));
        trim_in_place(path_buffer);
        if (str_contains(path_buffer, format("-applaunch {}", cod4_steam_appid).c_str(), 0U, true) ||
            check_if_file_path_exists(path_buffer))
        {
            main_app.set_iw3mp_exe_path(path_buffer);
        }

        GetWindowText(app_handles.hwnd_cod5_path_edit, path_buffer, std::size(path_buffer));
        trim_in_place(path_buffer);
        if (str_contains(path_buffer, format("-applaunch {}", cod5_steam_appid).c_str(), 0u, true) ||
            check_if_file_path_exists(path_buffer))
        {
            main_app.set_cod5mp_exe_path(path_buffer);
        }

        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
        print_colored_text(app_handles.hwnd_re_confirmation_message,
                           "^2Displayed server configuration settings have been "
                           "successfully saved!\n",
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        MessageBox(hwnd, "Displayed server configuration settings have been successfully saved!",
                   "Successfully saved server settings", MB_ICONINFORMATION | MB_OK);
        EnableWindow(app_handles.hwnd_main_window, TRUE);
        SetFocus(app_handles.hwnd_main_window);
        DestroyWindow(hwnd);
    }
    else
    {
        print_colored_text(app_handles.hwnd_re_confirmation_message,
                           "^1Failed to save displayed server configuration settings!\n",
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        MessageBox(hwnd, "Failed to save displayed server configuration settings!", "Failed to save server settings!",
                   MB_ICONWARNING | MB_OK);
    }
}

void process_button_test_connection_click_event(HWND)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char msg_buffer[1024];
    string new_user_name;
    string new_server_name;
    string new_server_ip;
    uint_least16_t new_port{};
    string new_rcon_password;
    int new_refresh_time_period{5};
    bool is_invalid_entry{};

    GetWindowTextA(app_handles.hwnd_user_name, msg_buffer, std::size(msg_buffer));
    if (stl::helper::len(msg_buffer) > 0)
    {
        new_user_name.assign(msg_buffer);
    }
    else
    {
        is_invalid_entry = true;
        print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Admin name text field cannot be empty!\n",
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }

    GetWindowTextA(app_handles.hwnd_server_name, msg_buffer, std::size(msg_buffer));
    if (stl::helper::len(msg_buffer) > 0)
    {
        new_server_name.assign(msg_buffer);
    }
    else
    {
        is_invalid_entry = true;
        print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Server name text field cannot be empty!\n",
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }

    GetWindowTextA(app_handles.hwnd_server_ip_address, msg_buffer, std::size(msg_buffer));
    const auto ip_len{stl::helper::len(msg_buffer)};
    unsigned long ip_key{};
    const string server_ip{msg_buffer};
    if (ip_len > 0 && check_ip_address_validity(server_ip, ip_key))
    {
        new_server_ip.assign(msg_buffer);
    }
    else
    {
        is_invalid_entry = true;
        const string user_message1{"^1You have entered an invalid IP address! ^5("s + msg_buffer + ")\n"s};
        print_colored_text(app_handles.hwnd_re_confirmation_message, user_message1.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }

    GetWindowTextA(app_handles.hwnd_server_port, msg_buffer, std::size(msg_buffer));
    int number{};
    if (stl::helper::len(msg_buffer) > 0 && is_valid_decimal_whole_number(msg_buffer, number))
    {
        new_port = static_cast<uint_least16_t>(number);
    }
    else
    {
        is_invalid_entry = true;
        const string user_message2{"^1You have entered an invalid server port number! ^5("s + msg_buffer + ")\n"s};
        print_colored_text(app_handles.hwnd_re_confirmation_message, user_message2.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }

    GetWindowTextA(app_handles.hwnd_rcon_password, msg_buffer, std::size(msg_buffer));
    if (stl::helper::len(msg_buffer) > 0)
    {
        new_rcon_password.assign(msg_buffer);
    }
    else
    {
        is_invalid_entry = true;
        const string user_message3{"^1You have entered an invalid rcon password! ^5("s + msg_buffer + ")\n"s};
        print_colored_text(app_handles.hwnd_re_confirmation_message, user_message3.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    }

    main_app.set_is_automatic_city_kick_enabled(Button_GetCheck(app_handles.hwnd_enable_city_ban) != BST_UNCHECKED);
    main_app.set_is_automatic_country_kick_enabled(Button_GetCheck(app_handles.hwnd_enable_country_ban) !=
                                                   BST_UNCHECKED);

    if (!is_invalid_entry)
    {
        (void)snprintf(msg_buffer, std::size(msg_buffer),
                       "\n^2Testing connection with the specified game server (^5%s^2) at "
                       "^5%s:%d ^2using the following ^5Tiny^6Rcon ^2settings:\n^1Admin name: "
                       "^5%s\n^1Server name: ^5%s\n^1Server IP address: ^5%s\n^1Server port "
                       "number: ^5%d\n^1Server rcon password: ^5%s\n^1Refresh time period: ^5%d "
                       "second(s)\n",
                       new_server_name.c_str(), new_server_ip.c_str(), new_port, new_user_name.c_str(),
                       new_server_name.c_str(), new_server_ip.c_str(), new_port, new_rcon_password.c_str(),
                       new_refresh_time_period);
        print_colored_text(app_handles.hwnd_re_confirmation_message, msg_buffer,
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        auto [test_result, game_name] = check_if_specified_server_ip_port_and_rcon_password_are_valid(
            new_server_ip.c_str(), new_port, new_rcon_password.c_str());
        if (test_result)
        {
            print_colored_text(app_handles.hwnd_re_confirmation_message, "^2Testing connection SUCCEEDED!\n",
                               is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        }
        else
        {
            print_colored_text(app_handles.hwnd_re_confirmation_message, "^1Testing connection FAILED!\n",
                               is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
        }
    }
}

const std::map<std::string, std::string> &get_rcon_map_names_to_full_map_names_for_specified_game_name(
    const game_name_t game_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);

    static const map<string, string> cod1_rcon_map_name_full_map_names{
        {"mp_brecourt", "Brecourt, France"}, {"mp_carentan", "Carentan, France"},
        {"mp_chateau", "Chateau, Germany"},  {"mp_dawnville", "St. Mere Eglise, France"},
        {"mp_depot", "Depot, Poland"},       {"mp_harbor", "Harbor, Germany"},
        {"mp_hurtgen", "Hurtgen, Germany"},  {"mp_pavlov", "Pavlov, Russia"},
        {"mp_powcamp", "Powcamp, Poland"},   {"mp_railyard", "Stalingrad, Russia"},
        {"mp_rocket", "Rocket, Germany"},    {"mp_ship", "Ship, England"}};

    static const map<string, string> cod2_rcon_map_name_full_map_names{
        {"mp_breakout", "Villers-Bocage, France"},
        {"mp_brecourt", "Brecourt, France"},
        {"mp_burgundy", "Burgundy, France"},
        {"mp_carentan", "Carentan, France"},
        {"mp_dawnville", "St. Mere Eglise, France"},
        {"mp_decoy", "El Alamein, Egypt"},
        {"mp_downtown", "Moscow, Russia"},
        {"mp_farmhouse", "Beltot, France"},
        {"mp_leningrad", "Leningrad, Russia"},
        {"mp_matmata", "Matmata, Tunisia"},
        {"mp_railyard", "Stalingrad, Russia"},
        {"mp_toujane", "Toujane, Tunisia"},
        {"mp_trainstation", "Caen, France"},
        {"mp_rhine", "Wallendar, Germany"},
        {"mp_harbor", "Rostov, Russia"},
        {"mp_firestation", "Firestation"},
        {"mp_industry", "Industry, Germany"},
        {"gob_rats", "Gob Rats"},
        {"mp_gob_rats", "Gob Rats v2"},
        {"mp_breach", "Currahee! Breach"},
        {"mp_nuked", "Nuketown, Arizona"},
        {"mp_zaafrane", "Zaafrane, Tunisia"},
        {"mp_tripoli", "Tripoli, Libya"},
        {"mp_farao", "Farao, Egypt"},
        {"mp_gob_subzero", "Gob Subzero"},
        {"gob_italy2", "Gob Italy EXTRA!"},
        {"mp_devilscreek", "Devils Creek"},
        {"mp_naout", "North Africa, Outpost"},
        {"mp_rouen", "Currahee! Rouen"},
        {"mp_verimatmatav1", "Matmata, Verindra"},
        {"mp_verimatmatav2", "MatmataII, Verindra"},
        {"mp_veribank", "Verindra Bank"},
        {"mp_gob_eulogy", "Gob Eulogy"},
        {"mp_wuesten", "WuestenSturm"},
        {"mp_tuscany", "Tuscany, Italy"},
        {"mp_malta", "Malta, Italy"},
        {"mp_jojo", "JoJo"},
        {"mp_dicky2", "Dicky v2"},
        {"mp_lcftown", "LCF Town v1.0"},
        {"mp_houn", "Houn, Libya"},
        {"mp_omaha_v2a", "Omaha Beach v2a by Backdraft"},
        {"rocket", "Rocket"},
        {"mp_africorps", "Africorps vs. Desertrats"},
        {"mp_warsaw", "Warsaw, Poland"},
        {"mp_valley_summer_2", "Valley Summer v2"},
        {"mp_bzt", "Bazna, Tunisia"},
        {"jm_kuwehr2", "Kuwehr2 by Rez|l and Pnin"},
        {"gob_icestation", "Gob Icestation"},
        {"mp_coalminev2", "Coal Mine v2"},
        {"mp_warzonev1", "Warzone"},
        {"mp_2011", "The Short Line 2011"},
        {"mp_worldcup", "Worldcup by Kams"},
        {"mp_factory", "Factory, Germany"},
        {"mp_gob_mice", "Gob Mice"},
        {"mp_killers_city", "KILLERS city"},
        {"mp_cbxmap", "CBX-map"},
        {"mp_fog", "Fog"},
        {"mp_libya", "Libya (VehiclesMod)"},
        {"mp_rock_island", "Rock Island"},
        {"mp_canal_final", "Canal, Belgium"},
        {"canal2", "Canal"},
        {"mp_bridge", "The Bridge"},
        {"mtl_the_rock", "MTL The Rock"},
        {"mp_battlefield", "Battlefield, France"},
        {"mp_rhine", "Rhine"},
        {"mp_harbor", "Harbor"},
        {"mp_el_milh", "El Milh, Africa"},
        {"mp_sfrance_final", "MoHSouthern France(Final)"},
        {"mp_panodra", "PanodraWPC"},
        {"mp_clanfight_r", "MtO Clanfight Revisited!"},
        {"mp_airfield2", "Airfield v2"},
        {"mp_curra_tuscany2", "Currahee Tuscany 2"},
        {"mp_reckoning_day", "Reckoning day"},
        {"mp_coddm2", "K6 Destroyed Village France"},
        {"mp_delta_mission", "Delta Mission, The day after..."},
        {"gob_iced", "Gob Iced"},
        {"mp_mroa_rathouse", "MRoA Rathouse"},
        {"mp_gp2", "Gaperon2"},
        {"warehouse", "WareHouse"},
        {"mp_bathroom", "Da BathRoom"},
        {"mp_winter_toujane", "Currahee Winter Toujane"},
        {"mp_izmir", "Izmir by (PHk) Schenk"},
        {"mp_complex_v2", "The Complex v2"},
        {"mp_port", "Port, Tunisia"},
        {"mp_hill24", "Hill24"},
        {"mp_powcamp", "Prisoners of War Camp, Germany"},
        {"mp_v2", "V2"},
        {"mp_valence_v2", "Valence, France"},
        {"mp_vallente", "Vallente, France"},
        {"mp_depot", "Depot, Germany"},
        {"mp_dome", "Dome"},
        {"mp_ode_de_chateau", "Ode de Chateau"},
        {"mp_sl", "The Short Line v1.2"},
        {"mp_killhouse", "Killhouse"},
        {"mp_mersin", "Mersin Turkey v1.0"},
        {"fru_assault", "Fru Assault"},
        {"mp_el_mechili", "El Mechili, Lybia"},
        {"mp_rts2", "Road to Stalingrad"},
        {"mp_stalingrad_2", "Stalingrad v2 - Summer"},
        {"mp_neuville", "Neuville, France"},
        {"mp_street", "K6 Street"},
        {"mp_dustville", "Dustville"},
        {"mp_ctf", "Morte Ville"},
        {"mp_maaloy_s", "Maaloy, Summer"},
        {"mp_new_dawnville", "New Dawnville - eXtended v2"},
        {"mp_kemptown", "Currahee! Kemptown"},
        {"mp_foucarville", "{=EM=}Foucarville"},
        {"mp_nuenen_v1", "Nuenen, Netherlands"},
        {"mp_rushtown_lib_finale", "Rushtown Finale"},
        {"mp_bretot", "Bretot, France"},
        {"mp_rouxeville", "Rouxeville, France"},
        {"mp_snipearena", "Sniper Arena, Australia"},
        {"mp_courtyard", "CoD2 Courtyard"},
        {"mp_colditz", "Colditz, Germany"},
        {"mp_cunisia", "Cunisia"},
        {"mp_cologne", "Cologne, Germany"},
        {"mp_cairo", "11th ACR Kairo"},
        {"mp_cr_kasserine", "CR Kasserine, Tunisia"},
        {"mp_mareth", "Mareth, Tunisia"},
        {"mp_scrapyard", "Abandoned Scrapyard"},
        {"mp_nfactorys", "New Factory"},
        {"mp_codcastle2", "cod2 Castle"},
        {"mp_orionzen", "Currahee! Orion Zen"},
        {"mp_woods", "Woods"},
        {"mp_grandville", "Granville Depot"},
        {"mp_burgundy_v2", "Burgundy v2, France"},
        {"mp_hungbulung_v2", "Hungbulung v2"},
        {"mp_harbor_v2", "Harbor v2"},
        {"mp_gob_eulogy_v2", "Gob Eulogy v2"},
        {"mp_containers_park", "Container Park"},
        {"mp_mitres", "Mitres"},
        {"mp_oc", "Octagon"},
        {"mp_bayeux", "Bayeux, France"},
        {"mp_CoD_Dust", "cod2 Dust"},
        {"mp_tower", "The tower"},
        {"mp_dbt_rats", ":DBT: Rats"},
        {"mp_crazymaze", "CrazyMaze"},
        {"mp_gamerpistol", "Gamer pistol"},
        {"mp_hawkeyeaim", "Hawkey's Aim map"},
        {"mp_destiny", "Destiny"},
        {"mp_verla", "Verladenpanzer"},
        {"mp_dust", "Dust"},
        {"mp_minecraft", "Minecraft by Rychie"},
        {"mp_maginotline", "The Maginotline"},
        {"mp_tunis", "Tunis, Tunisia"},
        {"mp_fobdors_arena", "Fobdor's Arena"},
        {"mp_glossi8", "Lyon, France"},
        {"mp_chateau", "Chateau v1.1"},
        {"mp_poland", "Polonya,Europe"},
        {"mp_fontenay", "Fontenay, France"},
        {"mp_shipment_v2", "Shipment v2"},
        {"mp_temporal", "TFC's Port"},
        {"v_gob_aim", "Gob Aim"},
        {"mp_industryv2", "Industry, Germany"},
        {"mp_oasis", "TFC Oasis"},
        {"mp_alburjundi", "Currahee Al Burjundi"},
        {"mp_stlo", "Saint Lo"},
        {"mp_emt", "Egyptian Market Town (Day)"},
        {"mp_prison", "TFC's Prison"},
        {"gob_scopeshot2", "Scopeshot v2.0"},
        {"mp_hybrid", "Hybrid's Game"},
        {"mp_survival_1", "EMS - Survival"},
        {"mp_bataan", "Bataan"},
        {"mp_blockarena", "Blockarena"},
        {"mp_vovel", "Vovel2, Russia"},
        {"mp_subzero_plus", "Gob Subzero Plus"},
        {"mp_assink", "Het Assink Lyceum"},
        {"mp_theros_park", "Theros Park"},
        {"mp_bumbarsky", "Bumbarsky, Pudgura"},
        {"mp_bridge", "The Bridge"},
        {"mp_pavlov", "Pavlov's House, Russia"},
        {"mp_badarena", "BAD Arena"},
        {"mp_deadend", "Dead End"},
        {"mp_kraut_hammer", "Kraut Hammer beta v2"},
        {"mp_winter_assault", "Winter Assault"},
        {"mp_tobruk", "Tobruk, Libya"},
        {"mp_farao_eye", "Farao Eye"},
        {"mp_omaze_v3", "Outlaw Maze"},
        {"mp_labyrinth", "Labyrinth"},
        {"mp_ctflaby", "Labyrinth"},
        {"dc_degeast", "DC-De Geast"},
        {"mp_training", "The devil's training map"},
        {"mp_harmata", "Harmata, Egypt"},
        {"mp_tuscany_ext_beta", "Currahee! Tuscany (Extended), Italy"},
        {"mp_africa_v2", "Somewhere in Africa v2"},
        {"mp_marketsquare_b1", "Market Square"},
        {"mp_Argentan_France", "Argentan, France"},
        {"mp_belltown", "Belltown"},
        {"mp_13tunisia", "Tunisia, 1942"},
        {"mp_heraan", "Heraan"},
        {"mp_townville", "Townville"},
        {"mp_tank_depot", "Tank Depot, Germany"},
        {"mp_boneville", "Boneville, France"},
        {"mp_toujentan", "Toujentan, Frenchisia"},
        {"mp_remagen2", "MoHRemagen (Final)"},
        {"mp_sand_dune", "Sand dune"},
        {"mp_chelm", "Chelm, Poland"},
        {"mp_giantroom", "Rat Room v3"},
        {"mp_carentan_ville", "Carentan Ville, France"},
        {"mp_oradour", "Oradour-Sur-Glane"},
        {"mtl_purgatory", "MTL Purgatory"},
        {"mp_rouen_hq", "Rouen, France"},
        {"mp_alger", "Alger by Kams"},
        {"mp_draguignan", "Draguignan by Kams"},
        {"gob_tozeur", "Gob Tozeur"},
        {"mp_ship2", "Ship v2"},
        {"mp_gob_arena", "Gob Arena"},
        {"mp_nijmegen_bridge", "Nijmegen Bridge"},
        {"mp_bigred", "Big Red"},
        {"mp_bobo", "Bobo"},
        {"mp_argentan_france", "Argentan,France"},
        {"mp_comproom_v1", "39th||Comp. Room"},
        {"mp_craville", "Craville Belgium"},
        {"mp_accona_desert", "Accona Desert"},
        {"mp_utca", "Hardened street"},
        {"mp_mythicals_carentan", "Carentan v2, France"},
        {"mp_normandy_farm", "Normandy Farm"},
        {"mp_natnarak", "Natnarak, Libya"},
        {"mp_newburgundy", "TeamC New Burgundy"},
        {"btw_n10", "Btw. N10"},
        {"mp_zeppelinfeld", "Zeppelinfeld, Germany"},
        {"mp_waldcamp_day", "Waldcamp day, Germany"},
        {"mp_sg_marketgarden", "Market Garden"},
        {"mp_survival_2", "EMS - Survival v2"},
        {"mp_oase", "Oasis, Africa"},
        {"mp_rusty", "Rusty"},
        {"mp_stuttgart", "Stuttgart, Germany"},
        {"mp_pecs", "Pecs, Hungary"},
        {"mp_st", "Small town"},
        {"mp_aimgobII", "Gob Aim by (PHk) Schenk"},
        {"fr_mouseb", "FR MouseB"},
        {"mp_gob_arena_v2", "Gob Arena v2"},
        {"mp_island_fortress", "Island Fortress"},
        {"mp_grimms_depot", "GR1MMs Depot"},
        {"mp_bridge2", "The Bridge v2"},
        {"mp_wioska_v2", "Wioska v2"},
        {"mp_de", "Dead End"},
        {"mp_sd2", "Sadia v2"},
        {"mp_ut2", "Road to Salvation"},
        {"mp_fh2", "Fohadiszallas -=T.P.K.=- v2"},
        {"mp_sa", "Sniper Arena, Australia"},
        {"r2", "Rostov, Russia"},
        {"mp_hw", "UDW High Wire"},
        {"mp_b2", "Bandit"},
        {"mp_p2", "Port, Algeria"},
        {"mp_vs2", "Verschneit v2"},
        {"mp_rb", "River base"},
        {"mp_cm", "Cat and Mouse"},
        {"mp_depot_v2", "Depot v2 by Lonsofore"},
        {"mp_cr", "39th||Comp. Room"},
        {"mp_gr", "Rat Room v3"},
        {"mp_wr", "Winter"},
        {"mp_fb", "{BPU} Facility Beta"},
        {"mp_lu", "TnTPlayground"},
        {"mp_su", "Subone, Siberia"},
        {"mp_st2", "MoH Stalingrad, Russia"},
        {"mp_wp", "TFL Winter Place"},
        {"mp_wz", "Warzone v1"},
        {"mp_dt", "Deathtrap"},
        {"mp_bv2", "Bathroom v2"},
        {"mp_ma", "Mansion, Russia"}};

    static const map<string, string> cod4_rcon_map_name_full_map_names{{"mp_backlot", "Backlot"},
                                                                       {"mp_bloc", "Bloc"},
                                                                       {"mp_bog", "Bog"},
                                                                       {"mp_broadcast", "Broadcast"},
                                                                       {"mp_carentan", "Chinatown"},
                                                                       {"mp_cargoship", "Wet Work"},
                                                                       {"mp_citystreets", "District"},
                                                                       {"mp_convoy", "Ambush"},
                                                                       {"mp_countdown", "Countdown"},
                                                                       {"mp_crash", "Crash"},
                                                                       {"mp_crash_snow", "Crash Snow"},
                                                                       {"mp_creek", "Creek"},
                                                                       {"mp_crossfire", "Crossfire"},
                                                                       {"mp_farm", "Downpour"},
                                                                       {"mp_killhouse", "Killhouse"},
                                                                       {"mp_overgrown", "Overgrown"},
                                                                       {"mp_pipeline", "Pipeline"},
                                                                       {"mp_shipment", "Shipment"},
                                                                       {"mp_showdown", "Showdown"},
                                                                       {"mp_strike", "Strike"},
                                                                       {"mp_vacant", "Vacant"}};

    static const map<string, string> cod5_rcon_map_name_full_map_names{
        {"mp_airfield", "Airfield"},   {"mp_asylum", "Asylum"},         {"mp_castle", "Castle"},
        {"mp_shrine", "Cliffside"},    {"mp_courtyard", "Courtyard"},   {"mp_dome", "Dome"},
        {"mp_downfal", "Downfal"},     {"mp_hangar", "Hangar"},         {"mp_makin", "Makin"},
        {"mp_outskirts", "Outskirts"}, {"mp_roundhouse", "Roundhouse"}, {"mp_seelow", "Seelow"},
        {"mp_suburban", "Upheava"}};

    switch (game_name)
    {
    case game_name_t::cod1:
        return cod1_rcon_map_name_full_map_names;
    case game_name_t::cod2:
        return cod2_rcon_map_name_full_map_names;
        // return main_app.get_available_rcon_to_full_map_names();
    case game_name_t::cod4:
        return cod4_rcon_map_name_full_map_names;
    case game_name_t::cod5:
        return cod5_rcon_map_name_full_map_names;
    default:
        return cod2_rcon_map_name_full_map_names;
    }
}

const std::map<std::string, std::string> &get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(
    const game_name_t game_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const map<string, string> cod1_full_gametype_names{{"dm", "Deathmatch"},         {"tdm", "Team Deathmatch"},
                                                              {"sd", "Search and Destroy"}, {"re", "Retrieva"},
                                                              {"be", "Behind Enemy Lines"}, {"hq", "Headquarters"}};

    static const map<string, string> cod2_full_gametype_names{{"dm", "Deathmatch"},        {"tdm", "Team-Deathmatch"},
                                                              {"ctf", "Capture The Flag"}, {"hq", "Headquarters"},
                                                              {"sd", "Search & Destroy"},  {"dom", "Domination"}};

    static const map<string, string> cod4_full_gametype_names{{"dm", "Free for al"}, {"war", "Team Deathmatch"},
                                                              {"sab", "Sabotage"},   {"sd", "Search and Destroy"},
                                                              {"dom", "Domination"}, {"koth", "Headquarters"}};

    static const map<string, string> cod5_full_gametype_names{
        {"dm", "Free for al"}, {"tdm", "Team Deathmatch"},  {"sab", "Sabotage"}, {"sd", "Search and Destroy"},
        {"dom", "Domination"}, {"ctf", "Capture the Flag"}, {"twar", "War"},     {"koth", "Headquarters"}};

    switch (game_name)
    {
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

const std::map<std::string, std::string> &get_full_map_names_to_rcon_map_names_for_specified_game_name(
    const game_name_t game_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const map<string, string> cod1_full_map_name_rcon_map_names{
        {"Brecourt, France", "mp_brecourt"}, {"Carentan, France", "mp_carentan"},
        {"Chateau, Germany", "mp_chateau"},  {"St. Mere Eglise, France", "mp_dawnville"},
        {"Depot, Poland", "mp_depot"},       {"Harbor, Germany", "mp_harbor"},
        {"Hurtgen, Germany", "mp_hurtgen"},  {"Pavlov, Russia", "mp_pavlov"},
        {"Powcamp, Poland", "mp_powcamp"},   {"Stalingrad, Russia", "mp_railyard"},
        {"Rocket, Germany", "mp_rocket"},    {"Ship, England", "mp_ship"}};

    static const map<string, string> cod2_full_map_name_rcon_map_names{
        {"Villers-Bocage, France", "mp_breakout"},
        {"Brecourt, France", "mp_brecourt"},
        {"Burgundy, France", "mp_burgundy"},
        {"Carentan, France", "mp_carentan"},
        {"St. Mere Eglise, France", "mp_dawnville"},
        {"El Alamein, Egypt", "mp_decoy"},
        {"Moscow, Russia", "mp_downtown"},
        {"Beltot, France", "mp_farmhouse"},
        {"Leningrad, Russia", "mp_leningrad"},
        {"Matmata, Tunisia", "mp_matmata"},
        {"Stalingrad, Russia", "mp_railyard"},
        {"Toujane, Tunisia", "mp_toujane"},
        {"Caen, France", "mp_trainstation"},
        {"Wallendar, Germany", "mp_rhine"},
        {"Rostov, Russia", "mp_harbor"},
        {"Firestation", "mp_firestation"},
        {"Industry, Germany", "mp_industry"},
        {"Gob Rats", "gob_rats"},
        {"Gob Rats v2", "mp_gob_rats"},
        {"Currahee! Breach", "mp_breach"},
        {"Nuketown, Arizona", "mp_nuked"},
        {"Zaafrane, Tunisia", "mp_zaafrane"},
        {"Tripoli, Libya", "mp_tripoli"},
        {"Farao, Egypt", "mp_farao"},
        {"Gob Subzero", "mp_gob_subzero"},
        {"Gob Italy EXTRA!", "gob_italy2"},
        {"Devils Creek", "mp_devilscreek"},
        {"North Africa, Outpost", "mp_naout"},
        {"Currahee! Rouen", "mp_rouen"},
        {"Matmata, Verindra", "mp_verimatmatav1"},
        {"MatmataII, Verindra", "mp_verimatmatav2"},
        {"Verindra Bank", "mp_veribank"},
        {"Gob Eulogy", "mp_gob_eulogy"},
        {"WuestenSturm", "mp_wuesten"},
        {"Tuscany, Italy", "mp_tuscany"},
        {"Malta, Italy", "mp_malta"},
        {"JoJo", "mp_jojo"},
        {"Dicky v2", "mp_dicky2"},
        {"LCF Town v1.0", "mp_lcftown"},
        {"Houn, Libya", "mp_houn"},
        {"Omaha Beach v2a by Backdraft", "mp_omaha_v2a"},
        {"Rocket", "rocket"},
        {"Africorps vs. Desertrats", "mp_africorps"},
        {"Warsaw, Poland", "mp_warsaw"},
        {"Valley Summer v2", "mp_valley_summer_2"},
        {"Bazna, Tunisia", "mp_bzt"},
        {"Kuwehr2 by Rez|l and Pnin", "jm_kuwehr2"},
        {"Gob Icestation", "gob_icestation"},
        {"Coal Mine v2", "mp_coalminev2"},
        {"Warzone", "mp_warzonev1"},
        {"The Short Line 2011", "mp_2011"},
        {"Worldcup by Kams", "mp_worldcup"},
        {"Factory, Germany", "mp_factory"},
        {"Gob Mice", "mp_gob_mice"},
        {"KILLERS city", "mp_killers_city"},
        {"CBX-map", "mp_cbxmap"},
        {"Fog", "mp_fog"},
        {"Libya (VehiclesMod)", "mp_libya"},
        {"Rock Island", "mp_rock_island"},
        {"Canal, Belgium", "mp_canal_final"},
        {"Canal", "canal2"},
        {"The Bridge", "mp_bridge"},
        {"MTL The Rock", "mtl_the_rock"},
        {"Battlefield, France", "mp_battlefield"},
        {"Rhine", "mp_rhine"},
        {"Harbor", "mp_harbor"},
        {"El Milh, Africa", "mp_el_milh"},
        {"MoHSouthern France(Final)", "mp_sfrance_final"},
        {"PanodraWPC", "mp_panodra"},
        {"MtO Clanfight Revisited!", "mp_clanfight_r"},
        {"Airfield v2", "mp_airfield2"},
        {"Currahee Tuscany 2", "mp_curra_tuscany2"},
        {"Reckoning day", "mp_reckoning_day"},
        {"K6 Destroyed Village France", "mp_coddm2"},
        {"Delta Mission, The day after...", "mp_delta_mission"},
        {"Gob Iced", "gob_iced"},
        {"MRoA Rathouse", "mp_mroa_rathouse"},
        {"Gaperon2", "mp_gp2"},
        {"WareHouse", "warehouse"},
        {"Da BathRoom", "mp_bathroom"},
        {"Currahee Winter Toujane", "mp_winter_toujane"},
        {"Izmir by (PHk) Schenk", "mp_izmir"},
        {"The Complex v2", "mp_complex_v2"},
        {"Port, Tunisia", "mp_port"},
        {"Hill24", "mp_hill24"},
        {"Prisoners of War Camp, Germany", "mp_powcamp"},
        {"V2", "mp_v2"},
        {"Valence, France", "mp_valence_v2"},
        {"Vallente, France", "mp_vallente"},
        {"Depot, Germany", "mp_depot"},
        {"Dome", "mp_dome"},
        {"Ode de Chateau", "mp_ode_de_chateau"},
        {"The Short Line v1.2", "mp_sl"},
        {"Killhouse", "mp_killhouse"},
        {"Mersin Turkey v1.0", "mp_mersin"},
        {"Fru Assault", "fru_assault"},
        {"El Mechili, Lybia", "mp_el_mechili"},
        {"Road to Stalingrad", "mp_rts2"},
        {"Stalingrad v2 - Summer", "mp_stalingrad_2"},
        {"Neuville, France", "mp_neuville"},
        {"K6 Street", "mp_street"},
        {"Dustville", "mp_dustville"},
        {"Morte Ville", "mp_ctf"},
        {"Maaloy, Summer", "mp_maaloy_s"},
        {"New Dawnville - eXtended v2", "mp_new_dawnville"},
        {"Currahee! Kemptown", "mp_kemptown"},
        {"{=EM=}Foucarville", "mp_foucarville"},
        {"Nuenen, Netherlands", "mp_nuenen_v1"},
        {"Rushtown Finale", "mp_rushtown_lib_finale"},
        {"Bretot, France", "mp_bretot"},
        {"Rouxeville, France", "mp_rouxeville"},
        {"Sniper Arena, Australia", "mp_snipearena"},
        {"CoD2 Courtyard", "mp_courtyard"},
        {"Colditz, Germany", "mp_colditz"},
        {"Cunisia", "mp_cunisia"},
        {"Cologne, Germany", "mp_cologne"},
        {"11th ACR Kairo", "mp_cairo"},
        {"CR Kasserine, Tunisia", "mp_cr_kasserine"},
        {"Mareth, Tunisia", "mp_mareth"},
        {"Abandoned Scrapyard", "mp_scrapyard"},
        {"New Factory", "mp_nfactorys"},
        {"cod2 Castle", "mp_codcastle2"},
        {"Currahee! Orion Zen", "mp_orionzen"},
        {"Woods", "mp_woods"},
        {"Granville Depot", "mp_grandville"},
        {"Burgundy v2, France", "mp_burgundy_v2"},
        {"Hungbulung v2", "mp_hungbulung_v2"},
        {"Harbor v2", "mp_harbor_v2"},
        {"Gob Eulogy v2", "mp_gob_eulogy_v2"},
        {"Container Park", "mp_containers_park"},
        {"Mitres", "mp_mitres"},
        {"Octagon", "mp_oc"},
        {"Bayeux, France", "mp_bayeux"},
        {"cod2 Dust", "mp_CoD_Dust"},
        {"The tower", "mp_tower"},
        {":DBT: Rats", "mp_dbt_rats"},
        {"CrazyMaze", "mp_crazymaze"},
        {"Gamer pistol", "mp_gamerpistol"},
        {"Hawkey's Aim map", "mp_hawkeyeaim"},
        {"Destiny", "mp_destiny"},
        {"Verladenpanzer", "mp_verla"},
        {"Dust", "mp_dust"},
        {"Minecraft by Rychie", "mp_minecraft"},
        {"The Maginotline", "mp_maginotline"},
        {"Tunis, Tunisia", "mp_tunis"},
        {"Fobdor's Arena", "mp_fobdors_arena"},
        {"Lyon, France", "mp_glossi8"},
        {"Chateau v1.1", "mp_chateau"},
        {"Polonya,Europe", "mp_poland"},
        {"Fontenay, France", "mp_fontenay"},
        {"Shipment v2", "mp_shipment_v2"},
        {"TFC's Port", "mp_temporal"},
        {"Gob Aim", "v_gob_aim"},
        {"Industry, Germany", "mp_industryv2"},
        {"TFC Oasis", "mp_oasis"},
        {"Currahee Al Burjundi", "mp_alburjundi"},
        {"Saint Lo", "mp_stlo"},
        {"Egyptian Market Town (Day)", "mp_emt"},
        {"TFC's Prison", "mp_prison"},
        {"Scopeshot v2.0", "gob_scopeshot2"},
        {"Hybrid´s Game", "mp_hybrid"},
        {"EMS - Survival", "mp_survival_1"},
        {"Bataan", "mp_bataan"},
        {"Blockarena", "mp_blockarena"},
        {"Vovel2, Russia", "mp_vovel"},
        {"Gob Subzero Plus", "mp_subzero_plus"},
        {"Het Assink Lyceum", "mp_assink"},
        {"Theros Park", "mp_theros_park"},
        {"Bumbarsky, Pudgura", "mp_bumbarsky"},
        {"The Bridge", "mp_bridge"},
        {"Pavlov's House, Russia", "mp_pavlov"},
        {"BAD Arena", "mp_badarena"},
        {"Dead End", "mp_deadend"},
        {"Kraut Hammer beta v2", "mp_kraut_hammer"},
        {"Winter Assault", "mp_winter_assault"},
        {"Tobruk, Libya", "mp_tobruk"},
        {"Farao Eye", "mp_farao_eye"},
        {"Outlaw Maze", "mp_omaze_v3"},
        {"Labyrinth", "mp_labyrinth"},
        {"Labyrinth", "mp_ctflaby"},
        {"DC-De Geast", "dc_degeast"},
        {"The devil's training map", "mp_training"},
        {"Harmata, Egypt", "mp_harmata"},
        {"Currahee! Tuscany (Extended), Italy", "mp_tuscany_ext_beta"},
        {"Somewhere in Africa v2", "mp_africa_v2"},
        {"Market Square", "mp_marketsquare_b1"},
        {"Argentan, France", "mp_Argentan_France"},
        {"Belltown", "mp_belltown"},
        {"Tunisia, 1942", "mp_13tunisia"},
        {"Heraan", "mp_heraan"},
        {"Townville", "mp_townville"},
        {"Tank Depot, Germany", "mp_tank_depot"},
        {"Boneville, France", "mp_boneville"},
        {"Toujentan, Frenchisia", "mp_toujentan"},
        {"MoHRemagen (Final)", "mp_remagen2"},
        {"Sand dune", "mp_sand_dune"},
        {"Chelm, Poland", "mp_chelm"},
        {"Rat Room v3", "mp_giantroom"},
        {"Carentan Ville, France", "mp_carentan_ville"},
        {"Oradour-Sur-Glane", "mp_oradour"},
        {"MTL Purgatory", "mtl_purgatory"},
        {"Rouen, France", "mp_rouen_hq"},
        {"Alger by Kams", "mp_alger"},
        {"Draguignan by Kams", "mp_draguignan"},
        {"Gob Tozeur", "gob_tozeur"},
        {"Ship v2", "mp_ship2"},
        {"Gob Arena", "mp_gob_arena"},
        {"Nijmegen Bridge", "mp_nijmegen_bridge"},
        {"Big Red", "mp_bigred"},
        {"Bobo", "mp_bobo"},
        {"Argentan,France", "mp_argentan_france"},
        {"39th||Comp. Room", "mp_comproom_v1"},
        {"Craville Belgium", "mp_craville"},
        {"Accona Desert", "mp_accona_desert"},
        {"Hardened street", "mp_utca"},
        {"Carentan v2, France", "mp_mythicals_carentan"},
        {"Normandy Farm", "mp_normandy_farm"},
        {"Natnarak, Libya", "mp_natnarak"},
        {"TeamC New Burgundy", "mp_newburgundy"},
        {"Btw. N10", "btw_n10"},
        {"Zeppelinfeld, Germany", "mp_zeppelinfeld"},
        {"Waldcamp day, Germany", "mp_waldcamp_day"},
        {"Market Garden", "mp_sg_marketgarden"},
        {"EMS - Survival v2", "mp_survival_2"},
        {"Oasis, Africa", "mp_oase"},
        {"Rusty", "mp_rusty"},
        {"Stuttgart, Germany", "mp_stuttgart"},
        {"Pecs, Hungary", "mp_pecs"},
        {"Small town", "mp_st"},
        {"Gob Aim by (PHk) Schenk", "mp_aimgobII"},
        {"FR MouseB", "fr_mouseb"},
        {"Gob Arena v2", "mp_gob_arena_v2"},
        {"Island Fortress", "mp_island_fortress"},
        {"GR1MMs Depot", "mp_grimms_depot"},
        {"The Bridge v2", "mp_bridge2"},
        {"Wioska v2", "mp_wioska_v2"},
        {"Dead End", "mp_de"},
        {"Sadia v2", "mp_sd2"},
        {"Road to Salvation", "mp_ut2"},
        {"Fohadiszallas -=T.P.K.=- v2", "mp_fh2"},
        {"Sniper Arena, Australia", "mp_sa"},
        {"Rostov, Russia", "r2"},
        {"UDW High Wire", "mp_hw"},
        {"Bandit", "mp_b2"},
        {"Port, Algeria", "mp_p2"},
        {"Verschneit v2", "mp_vs2"},
        {"River base", "mp_rb"},
        {"Cat and Mouse", "mp_cm"},
        {"Depot v2 by Lonsofore", "mp_depot_v2"},
        {"39th||Comp. Room", "mp_cr"},
        {"Rat Room v3", "mp_gr"},
        {"Winter", "mp_wr"},
        {"{BPU} Facility Beta", "mp_fb"},
        {"TnTPlayground", "mp_lu"},
        {"Subone, Siberia", "mp_su"},
        {"MoH Stalingrad, Russia", "mp_st2"},
        {"TFL Winter Place", "mp_wp"},
        {"Warzone v1", "mp_wz"},
        {"Deathtrap", "mp_dt"},
        {"Bathroom v2", "mp_bv2"},
        {"Mansion, Russia", "mp_ma"}};

    static const map<string, string> cod4_full_map_name_rcon_map_names{{"Backlot", "mp_backlot"},
                                                                       {"Bloc", "mp_bloc"},
                                                                       {"Bog", "mp_bog"},
                                                                       {"Broadcast", "mp_broadcast"},
                                                                       {"Chinatown", "mp_carentan"},
                                                                       {"Wet Work", "mp_cargoship"},
                                                                       {"District", "mp_citystreets"},
                                                                       {"Ambush", "mp_convoy"},
                                                                       {"Countdown", "mp_countdown"},
                                                                       {"Crash", "mp_crash"},
                                                                       {"Crash Snow", "mp_crash_snow"},
                                                                       {"Creek", "mp_creek"},
                                                                       {"Crossfire", "mp_crossfire"},
                                                                       {"Downpour", "mp_farm"},
                                                                       {"Killhouse", "mp_killhouse"},
                                                                       {"Overgrown", "mp_overgrown"},
                                                                       {"Pipeline", "mp_pipeline"},
                                                                       {"Shipment", "mp_shipment"},
                                                                       {"Showdown", "mp_showdown"},
                                                                       {"Strike", "mp_strike"},
                                                                       {"Vacant", "mp_vacant"}};

    static const map<string, string> cod5_full_map_name_rcon_map_names{
        {"Airfield", "mp_airfield"},   {"Asylum", "mp_asylum"},         {"Castle", "mp_castle"},
        {"Cliffside", "mp_shrine"},    {"Courtyard", "mp_courtyard"},   {"Dome", "mp_dome"},
        {"Downfal", "mp_downfal"},     {"Hangar", "mp_hangar"},         {"Makin", "mp_makin"},
        {"Outskirts", "mp_outskirts"}, {"Roundhouse", "mp_roundhouse"}, {"Seelow", "mp_seelow"},
        {"Upheava", "mp_suburban"}};

    switch (game_name)
    {
    case game_name_t::cod1:
        return cod1_full_map_name_rcon_map_names;
    case game_name_t::cod2:
        // return cod2_full_map_name_rcon_map_names;
        return main_app.get_available_full_map_to_rcon_map_names();
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
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const string ip{main_app.get_current_game_server().get_server_ip_address()};
    const uint_least16_t port{main_app.get_current_game_server().get_server_port()};
    const string rcon{main_app.get_current_game_server().get_rcon_password()};
    const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(ip.c_str(), port, rcon.c_str());
    main_app.set_game_name(result.second);
    if (result.first)
    {
        main_app.get_current_game_server().set_is_connection_settings_valid(true);
        set_admin_actions_buttons_active(TRUE);
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^2Initialization of ^1network settings ^2has successfully completed.\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    }
    else
    {
        main_app.get_current_game_server().set_is_connection_settings_valid(false);
        set_admin_actions_buttons_active(FALSE);
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^3Initialization of ^1network settings ^3has failed.\n^3The provided "
                           "^1rcon password ^3is incorrect.\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    }

    return true;
}

void initiate_sending_rcon_status_command_now()
{
    static const size_t rcon_status_time_period{main_app.get_check_for_banned_players_time_period()};
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    main_app.set_game_server_index(selected_server_row);
    atomic_counter.store(rcon_status_time_period);
    is_refresh_players_data_event.store(true);
}

void prepare_players_data_for_display(game_server &gs, const bool is_log_status_table)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    char buffer[256];
    size_t longest_name_length{32};
    size_t longest_country_length{32};

    auto &players = gs.get_players_data();
    const size_t number_of_players{gs.get_number_of_players()};
    correct_truncated_player_names(gs, gs.get_server_ip_address().c_str(), gs.get_server_port());
    check_for_temp_banned_ip_addresses();
    check_for_banned_ip_addresses();
    check_for_warned_players();

    // const auto &game_servers = main_app.get_game_servers();
    const auto &current_game_server = main_app.get_current_game_server();

    if (current_game_server.get_server_ip_address() == gs.get_server_ip_address() &&
        current_game_server.get_server_port() == gs.get_server_port())
    {

        check_if_admins_are_online_and_get_admins_player_names(players, number_of_players);

        if (number_of_players > 0)
        {
            sort_players_data(players, type_of_sort);
            if (!players.empty())
            {
                longest_name_length = std::max(
                    longest_name_length, find_longest_player_name_length(players.cbegin(), players.cend(), false));
                longest_country_length = std::max(
                    longest_country_length, find_longest_player_country_city_info_length(players, number_of_players));
            }
        }

        ostringstream log;
        g_re_match_information_contents = prepare_current_match_information();
        string match_information{g_re_match_information_contents};

        if (is_log_status_table)
        {
            string admins_information{online_admins_information};
            remove_all_color_codes(admins_information);
            remove_all_color_codes(match_information);
            log << admins_information << '\n';
            log << match_information;
        }

        const auto &pd = main_app.get_data_player_pid_color();
        const auto &sd = main_app.get_data_player_score_color();
        const auto &pgd = main_app.get_data_player_ping_color();
        const auto &ipd = main_app.get_data_player_ip_color();
        const auto &gd = main_app.get_data_player_geoinfo_color();

        const string decoration_line(46 + longest_name_length + longest_country_length, '=');
        if (is_log_status_table)
        {
            log << string{"\n"s + decoration_line + "\n"s};
            log << "|";
            log << right << setw(3) << "Pid"
                << " | " << setw(6) << "Score"
                << " | " << setw(4) << "Ping"
                << " | " << left << setw(longest_name_length) << "Player name"
                << " | " << left << setw(16) << "IP address"
                << " | " << left << setw(longest_country_length) << "Country, region, city"
                << "|";
            log << string{"\n"s + decoration_line + "\n"s};
        }
        if (0 == number_of_players)
        {
            if (is_log_status_table)
            {
                string server_message{main_app.get_server_message()};
                remove_all_color_codes(server_message);
                log << server_message;
                const size_t printed_chars_count =
                    get_number_of_characters_without_color_codes(main_app.get_server_message().c_str());
                const string filler{string(decoration_line.length() - 1 - printed_chars_count, ' ') + "|\n"s};
                log << filler;
            }
        }
        else
        {

            for (size_t i{}; i < number_of_players; ++i)
            {

                const player &p{players[i]};

                if (main_app.get_game_server_index() < main_app.get_rcon_game_servers_count())
                {

                    snprintf(buffer, std::size(buffer), "%s%d", pd.c_str(), p.pid);
                    strcpy_s(displayed_players_data[i].pid, std::size(displayed_players_data[i].pid), buffer);

                    snprintf(buffer, std::size(buffer), "%s%d", sd.c_str(), p.score);
                    strcpy_s(displayed_players_data[i].score, std::size(displayed_players_data[i].score), buffer);

                    snprintf(buffer, std::size(buffer), "%s%s", pgd.c_str(), p.ping);
                    strcpy_s(displayed_players_data[i].ping, std::size(displayed_players_data[i].ping), buffer);

                    strcpy_s(displayed_players_data[i].player_name, std::size(displayed_players_data[i].player_name),
                             p.player_name);

                    snprintf(buffer, std::size(buffer), "%s%s", ipd.c_str(), p.ip_address.c_str());
                    const size_t no_of_chars_to_copy{std::min<size_t>(19, len(buffer))};
                    strncpy_s(displayed_players_data[i].ip_address, std::size(displayed_players_data[i].ip_address),
                              buffer, no_of_chars_to_copy);

                    snprintf(buffer, std::size(buffer), "%s%s, %s", gd.c_str(),
                             (strlen(p.country_name) != 0 ? p.country_name : p.region), p.city);
                    strcpy_s(displayed_players_data[i].geo_info, std::size(displayed_players_data[i].geo_info), buffer);
                    displayed_players_data[i].country_code = p.country_code;
                }

                if (is_log_status_table)
                {
                    log << "|";
                    log << right << setw(3) << p.pid << " | " << setw(6) << p.score << " | " << setw(4) << p.ping
                        << " | ";
                    string name{p.player_name};
                    remove_all_color_codes(name);
                    const size_t printed_name_char_count{name.length()};
                    log << name;
                    if (printed_name_char_count < longest_name_length)
                    {
                        log << string(longest_name_length - printed_name_char_count, ' ');
                    }
                    log << " | ";
                    snprintf(buffer, std::size(buffer), "%s, %s",
                             (strlen(p.country_name) != 0 ? p.country_name : p.region), p.city);
                    log << left << setw(16) << p.ip_address << " | " << left << setw(longest_country_length) << buffer
                        << "|" << '\n';
                }
            }
        }

        if (main_app.get_game_server_index() < main_app.get_rcon_game_servers_count())
        {
            is_refreshed_players_data_ready_event.store(true);
        }

        if (is_log_status_table)
        {
            log << string{decoration_line + "\n"s};
            log_message(log.str(), is_log_datetime::yes);
        }
    }
}

void prepare_players_data_for_display_for_regular_users(game_server &gs, const bool is_log_status_table)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    char buffer[256];
    size_t longest_name_length{32};
    size_t longest_country_length{32};

    auto &players = gs.get_players_data();
    const size_t number_of_players{gs.get_number_of_players()};

    if (number_of_players > 0)
    {
        if (!is_display_geoinformation_data_for_players.load())
        {
            sort_players_data(players, sort_type::score_desc);
        }
        else
        {
            if (type_of_sort != sort_type::geo_asc)
            {
                type_of_sort = sort_type::geo_asc;
                ComboBox_SelectString(app_handles.hwnd_combo_box_sortmode, 0,
                                      sort_type_to_sort_names_dict.at(type_of_sort).c_str());
                // process_sort_type_change_request(sort_type::geo_asc);
            }

            std::sort(std::begin(players), std::begin(players) + number_of_players,
                      [](const player &pl1, const player &pl2) {
                          string pl1_cleaned_geo{pl1.geo_information};
                          string pl2_cleaned_geo{pl2.geo_information};
                          to_lower_case_in_place(pl1_cleaned_geo);
                          to_lower_case_in_place(pl2_cleaned_geo);
                          return pl1_cleaned_geo < pl2_cleaned_geo;
                      });

        } /*else if (type_of_sort == sort_type::geo_desc) {
    std::sort(std::begin(players), std::begin(players) + number_of_players, [](const
    player &pl1, const player &pl2) { string pl1_cleaned_geo{ pl1.geo_information };
        string pl2_cleaned_geo{ pl2.geo_information };
        to_lower_case_in_place(pl1_cleaned_geo);
        to_lower_case_in_place(pl2_cleaned_geo);
        return pl1_cleaned_geo > pl2_cleaned_geo;
    });
    }*/

        if (!players.empty())
        {
            longest_name_length =
                std::max(longest_name_length, find_longest_player_name_length(players.cbegin(), players.cend(), false));
            longest_country_length = std::max(longest_country_length,
                                              find_longest_player_country_city_info_length(players, number_of_players));
        }
    }

    ostringstream log;
    g_re_match_information_contents = prepare_current_match_information();
    string match_information{g_re_match_information_contents};

    if (is_log_status_table)
    {
        string admins_information{online_admins_information};
        remove_all_color_codes(admins_information);
        remove_all_color_codes(match_information);
        log << admins_information << '\n';
        log << match_information;
    }

    const auto &pd = main_app.get_data_player_pid_color();
    const auto &sd = main_app.get_data_player_score_color();
    const auto &pgd = main_app.get_data_player_ping_color();
    const auto &ipd = main_app.get_data_player_ip_color();
    const auto &gd = main_app.get_data_player_geoinfo_color();

    const string decoration_line(46 + longest_name_length + longest_country_length, '=');
    if (is_log_status_table)
    {
        log << string{"\n"s + decoration_line + "\n"s};
        log << "|";
        log << right << setw(3) << "Pid"
            << " | " << setw(6) << "Score"
            << " | " << setw(4) << "Ping"
            << " | " << left << setw(longest_name_length) << "Player name"
            << " | " << left << setw(16) << "IP address"
            << " | " << left << setw(longest_country_length) << "Country, region, city"
            << "|";
        log << string{"\n"s + decoration_line + "\n"s};
    }
    if (0 == number_of_players)
    {
        if (is_log_status_table)
        {
            string server_message{main_app.get_server_message()};
            remove_all_color_codes(server_message);
            log << server_message;
            const size_t printed_chars_count =
                get_number_of_characters_without_color_codes(main_app.get_server_message().c_str());
            const string filler{string(decoration_line.length() - 1 - printed_chars_count, ' ') + "|\n"s};
            log << filler;
        }
    }
    else
    {

        for (size_t i{}; i < number_of_players; ++i)
        {

            const player &p{players[i]};

            if (main_app.get_game_server_index() < main_app.get_rcon_game_servers_count())
            {

                snprintf(buffer, std::size(buffer), "%s%d", pd.c_str(), p.pid);
                strcpy_s(displayed_players_data[i].pid, std::size(displayed_players_data[i].pid), buffer);

                snprintf(buffer, std::size(buffer), "%s%d", sd.c_str(), p.score);
                strcpy_s(displayed_players_data[i].score, std::size(displayed_players_data[i].score), buffer);

                snprintf(buffer, std::size(buffer), "%s%s", pgd.c_str(), p.ping);
                strcpy_s(displayed_players_data[i].ping, std::size(displayed_players_data[i].ping), buffer);

                strcpy_s(displayed_players_data[i].player_name, std::size(displayed_players_data[i].player_name),
                         p.player_name);

                snprintf(buffer, std::size(buffer), "%s%s", ipd.c_str(), p.ip_address.c_str());
                const size_t no_of_chars_to_copy{std::min<size_t>(19, len(buffer))};
                strncpy_s(displayed_players_data[i].ip_address, std::size(displayed_players_data[i].ip_address), buffer,
                          no_of_chars_to_copy);

                snprintf(buffer, std::size(buffer), "%s%s", gd.c_str(), p.geo_information);
                strcpy_s(displayed_players_data[i].geo_info, std::size(displayed_players_data[i].geo_info), buffer);
                displayed_players_data[i].country_code = p.geo_country_code;
            }

            if (is_log_status_table)
            {
                log << "|";
                log << right << setw(3) << p.pid << " | " << setw(6) << p.score << " | " << setw(4) << p.ping << " | ";
                string name{p.player_name};
                remove_all_color_codes(name);
                const size_t printed_name_char_count{name.length()};
                log << name;
                if (printed_name_char_count < longest_name_length)
                {
                    log << string(longest_name_length - printed_name_char_count, ' ');
                }
                log << " | ";
                snprintf(buffer, std::size(buffer), "%s, %s", (strlen(p.country_name) != 0 ? p.country_name : p.region),
                         p.city);
                log << left << setw(16) << p.ip_address << " | " << left << setw(longest_country_length) << buffer
                    << "|" << '\n';
            }
        }
    }

    if (main_app.get_game_server_index() < main_app.get_rcon_game_servers_count())
    {
        is_refreshed_players_data_ready_event.store(true);
    }

    if (is_log_status_table)
    {
        log << string{decoration_line + "\n"s};
        log_message(log.str(), is_log_datetime::yes);
    }
}

void prepare_players_data_for_display_of_getstatus_response(game_server &gs, const bool is_log_status_table)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    char buffer[256];
    size_t longest_name_length{32};

    auto &players = gs.get_players_data();
    const size_t number_of_players{gs.get_number_of_players()};

    if (number_of_players > 0)
    {
        sort_players_data(players, sort_type::score_desc);
        if (!players.empty())
        {
            longest_name_length =
                std::max(longest_name_length, find_longest_player_name_length(players.cbegin(), players.cend(), false));
        }
    }

    ostringstream log;

    g_re_match_information_contents = prepare_current_match_information();
    string match_information{g_re_match_information_contents};

    if (is_log_status_table)
    {
        string admins_information{online_admins_information};
        remove_all_color_codes(admins_information);
        remove_all_color_codes(match_information);
        log << admins_information << '\n';
        log << match_information;
    }

    const auto &pd = main_app.get_data_player_pid_color();
    const auto &sd = main_app.get_data_player_score_color();
    const auto &pgd = main_app.get_data_player_ping_color();

    const string decoration_line(24 + longest_name_length, '=');
    if (is_log_status_table)
    {
        log << string{"\n"s + decoration_line + "\n"s};
        log << "|" << setw(6) << " score"
            << " | " << setw(4) << "ping"
            << " | " << setw(longest_name_length) << left << "Player name" << '|'
            << string{"\n"s + decoration_line + "\n"s};
        if (number_of_players == 0)
        {
            log << string{"| Server is empty."};
            const string filler{string(decoration_line.length() - 1 - stl::helper::len("| Server is empty."), ' ') +
                                "|\n"s};
            log << filler;
        }
    }

    if (number_of_players > 0)
    {

        for (size_t i{}; i < number_of_players; ++i)
        {

            const player &p{players[i]};

            snprintf(buffer, std::size(buffer), "%s%d", pd.c_str(), p.pid);
            strcpy_s(displayed_players_data[i].pid, std::size(displayed_players_data[i].pid), buffer);

            snprintf(buffer, std::size(buffer), "%s%d", sd.c_str(), p.score);
            strcpy_s(displayed_players_data[i].score, std::size(displayed_players_data[i].score), buffer);

            snprintf(buffer, std::size(buffer), "%s%s", pgd.c_str(), p.ping);
            strcpy_s(displayed_players_data[i].ping, std::size(displayed_players_data[i].ping), buffer);

            strcpy_s(displayed_players_data[i].player_name, std::size(displayed_players_data[i].player_name),
                     p.player_name);

            displayed_players_data[i].ip_address[0] = 0;
            displayed_players_data[i].geo_info[0] = 0;
            displayed_players_data[i].country_code = "xy";

            if (is_log_status_table)
            {
                log << "|" << right << setw(3) << p.pid << " | " << setw(6) << p.score << " | " << setw(4) << p.ping
                    << " | ";
                string name{p.player_name};
                remove_all_color_codes(name);
                log << name;
                const size_t printed_name_char_count{name.length()};
                if (printed_name_char_count < longest_name_length)
                {
                    log << string(longest_name_length - printed_name_char_count, ' ');
                }
                log << "|\n";
            }
        }
    }

    if (is_log_status_table)
    {
        log << string{decoration_line + "\n"s};
        log_message(log.str(), is_log_datetime::yes);
    }

    is_refreshed_players_data_ready_event.store(true);
}

size_t get_file_size_in_bytes(const char *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ifstream input{file_path, std::ios::binary | std::ios::in | std::ios::ate};
    if (!input)
    {
        return 0;
    }
    const std::fstream::pos_type file_size = input.tellg();
    return static_cast<size_t>(file_size);
}

size_t get_file_size_in_bytes(const wchar_t *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ifstream input{file_path, std::ios::binary | std::ios::in | std::ios::ate};
    if (!input)
    {
        return 0;
    }
    const std::fstream::pos_type file_size = input.tellg();
    return static_cast<size_t>(file_size);
}

std::string get_date_and_time_for_time_t(const char *date_time_format_str, time_t t_c)
{
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static constexpr const char *time_formatters[]{"{Y}", "{MMM}", "{MM}", "{M}",  "{D}", "{DD}",
                                                   "{h}", "{hh}",  "{m}",  "{mm}", "{s}", "{ss}"};

    if (t_c <= 0)
    {
        const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        t_c = std::chrono::system_clock::to_time_t(now);
    }
    tm time_info{};
    localtime_s(&time_info, &t_c);
    string time_format_str{date_time_format_str};
    char element[16];
    for (const auto &fmt : time_formatters)
    {
        const auto key_start = time_format_str.find(fmt);
        if (key_start != string::npos)
        {
            if (strcmp(fmt, "{Y}") == 0)
            {
                time_format_str.replace(key_start, 3, to_string(time_info.tm_year + 1900));
            }
            else if (strcmp(fmt, "{MMM}") == 0)
            {
                time_format_str.replace(key_start, 5, get_current_short_month_name(time_info.tm_mon + 1));
            }
            else if (strcmp(fmt, "{MM}") == 0)
            {
                snprintf(element, 16, "%02d", time_info.tm_mon + 1);
                time_format_str.replace(key_start, 4, element);
            }
            else if (strcmp(fmt, "{M}") == 0)
            {
                snprintf(element, 16, "%d", time_info.tm_mon + 1);
                time_format_str.replace(key_start, 3, element);
            }
            else if (strcmp(fmt, "{D}") == 0)
            {
                time_format_str.replace(key_start, 3, to_string(time_info.tm_mday));
            }
            else if (strcmp(fmt, "{DD}") == 0)
            {
                snprintf(element, 16, "%02d", time_info.tm_mday);
                time_format_str.replace(key_start, 4, element);
            }
            else if (strcmp(fmt, "{h}") == 0)
            {
                time_format_str.replace(key_start, 3, to_string(time_info.tm_hour));
            }
            else if (strcmp(fmt, "{hh}") == 0)
            {
                snprintf(element, 16, "%02d", time_info.tm_hour);
                time_format_str.replace(key_start, 4, element);
            }
            else if (strcmp(fmt, "{m}") == 0)
            {
                time_format_str.replace(key_start, 3, to_string(time_info.tm_min));
            }
            else if (strcmp(fmt, "{mm}") == 0)
            {
                snprintf(element, 16, "%02d", time_info.tm_min);
                time_format_str.replace(key_start, 4, element);
            }
            else if (strcmp(fmt, "{s}") == 0)
            {
                time_format_str.replace(key_start, 3, to_string(time_info.tm_sec));
            }
            else if (strcmp(fmt, "{ss}") == 0)
            {
                snprintf(element, 16, "%02d", time_info.tm_sec);
                time_format_str.replace(key_start, 4, element);
            }
        }
    }

    return time_format_str;
}

const char *get_current_short_month_name(const size_t month_index)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    switch (month_index)
    {
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

bool parse_getstatus_response_for_specified_game_server(game_server &gs)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static connection_manager cm;
    string received_reply;
    cm.send_and_receive_non_rcon_data("getstatus", received_reply, gs.get_server_ip_address().c_str(),
                                      gs.get_server_port(), gs, true, false);

    if (str_starts_with(received_reply, "statusresponse", true))
    {

        const size_t first_sep_pos{received_reply.find('\\')};
        if (first_sep_pos == string::npos)
            return false;

        received_reply.erase(cbegin(received_reply), cbegin(received_reply) + first_sep_pos + 1);
        size_t new_line_pos{received_reply.find('\n')};
        if (string::npos == new_line_pos)
            new_line_pos = received_reply.length();
        const string server_info{cbegin(received_reply), cbegin(received_reply) + new_line_pos};
        vector<string> parsedData{
            str_split(server_info, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no)};
        for (size_t i{}; i + 1 < parsedData.size(); i += 2)
        {
            update_game_server_setting(gs, std::move(parsedData[i]), std::move(parsedData[i + 1]));
        }

        int player_num{};
        int number_of_online_players{};
        int number_of_offline_players{};
        const char *start{received_reply.c_str() + new_line_pos + 1}, *last{start};
        auto &players_data = gs.get_players_data();
        const char *current{start};

        while (current < received_reply.c_str() + received_reply.length() && *current)
        {
            while (*last != '\n')
                ++last;
            current = last + 1;
            const string playerDataLine(start, last);

            start = last = playerDataLine.c_str();

            // Extracting player's score value from received UDP data packet for
            // getstatus command
            while (' ' == *start)
                ++start;
            last = start;
            while (*last != ' ')
                ++last;
            string temp_score{start, last};
            stl::helper::trim_in_place(temp_score);
            int player_score;
            if (!is_valid_decimal_whole_number(temp_score, player_score))
                player_score = 0;

            // Extracting player's ping value from received UDP data packet for
            // getstatus command
            start = last;
            while (' ' == *start)
                ++start;
            last = start;
            while (*last != ' ')
                ++last;

            string player_ping{start, last};
            stl::helper::trim_in_place(player_ping);
            if ("999" == player_ping || "-1" == player_ping || "CNCT" == player_ping || "ZMBI" == player_ping)
            {
                ++number_of_offline_players;
            }
            else
            {
                ++number_of_online_players;
            }

            // Extracting player_name information from received UDP data packet
            // for getstatus command
            start = last;
            while (*start != '"')
                ++start;
            last = ++start;
            while (*last != '"')
                ++last;

            players_data[player_num].pid = player_num;
            players_data[player_num].score = player_score;
            strcpy_s(players_data[player_num].ping, std::size(players_data[player_num].ping), player_ping.c_str());
            const auto no_of_chars_to_copy = static_cast<size_t>(last - start);
            strncpy_s(players_data[player_num].player_name, std::size(players_data[player_num].player_name), start,
                      no_of_chars_to_copy);
            players_data[player_num].player_name[no_of_chars_to_copy] = '\0';
            stl::helper::trim_in_place(players_data[player_num].player_name);
            players_data[player_num].country_name = "Unknown";
            players_data[player_num].region = "Unknown";
            players_data[player_num].city = "Unknown";
            players_data[player_num].country_code = "xy";
            ++player_num;
            start = last = current;
        }

        gs.set_number_of_players(player_num);
        gs.set_number_of_online_players(number_of_online_players);
        gs.set_number_of_offline_players(number_of_offline_players);
        // prepare_players_data_for_display_of_getstatus_response(false);

        return true;
    }

    return false;
}

bool parse_getinfo_response_for_specified_game_server(game_server &gs, std::string &number_of_online_players,
                                                      std::string &number_of_max_players)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static connection_manager cm;
    string received_reply;
    cm.send_and_receive_non_rcon_data("getinfo", received_reply, gs.get_server_ip_address().c_str(),
                                      gs.get_server_port(), gs, true, false);
    number_of_online_players.clear();
    number_of_max_players.clear();

    if (str_starts_with(received_reply, "inforesponse", true))
    {

        trim_in_place(received_reply);
        const size_t start{received_reply.find('\\')};
        if (start == string::npos)
            return false;

        received_reply.erase(cbegin(received_reply), cbegin(received_reply) + start + 1);
        vector<string> parsedData{
            str_split(received_reply, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no)};
        for (size_t i{}; i + 1 < parsedData.size(); i += 2)
        {
            if (parsedData[i] == "clients")
            {
                number_of_online_players = parsedData[i + 1];
            }
            else if (parsedData[i] == "sv_maxclients")
            {
                number_of_max_players = parsedData[i + 1];
            }
            update_game_server_setting(gs, std::move(parsedData[i]), std::move(parsedData[i + 1]));
        }

        return !number_of_online_players.empty() && !number_of_max_players.empty();
    }

    return false;
}

void correct_truncated_player_names(game_server &gs, const char *ip_address, const uint_least16_t port_number)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static connection_manager cm;
    string reply;
    cm.send_and_receive_non_rcon_data("getstatus", reply, ip_address, port_number, gs, true, false);

    if (str_starts_with(reply, "statusresponse", true))
    {

        const size_t off{reply.find('\\')};
        if (string::npos == off)
            return;
        const char *current = reply.c_str() + off + 1;
        const char *lastIndex = strchr(current, '\n');
        string_view server_info(current, lastIndex);
        std::vector<string> parsedData{
            str_split(server_info, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no)};
        for (size_t i{}; i + 1 < parsedData.size(); i += 2)
        {
            update_game_server_setting(gs, std::move(parsedData[i]), std::move(parsedData[i + 1]));
        }

        const auto new_line_pos = static_cast<size_t>(lastIndex - reply.c_str());

        unordered_map<string, int> unique_player_names;

        auto &players_data = gs.get_players_data();

        size_t start{new_line_pos + 1}, pl_index{};

        while (start < reply.length())
        {

            // Extracting player's score value from received UDP data packet for
            // getstatus command
            while (' ' == reply[start])
                ++start;
            size_t last{start};
            while (reply[last] != ' ')
                ++last;
            string temp_score{reply.substr(start, last - start)};
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

            string unique_player_name{reply.substr(start, last - start)};
            trim_in_place(unique_player_name);
            if (unique_player_name == players_data[pl_index].player_name)
            {
                players_data[pl_index].score = player_score;
            }
            unique_player_names[std::move(unique_player_name)]++;
            ++pl_index;
            start = ++last;
            if (reply[start] == '\n')
                ++start;
        }

        vector<pair<size_t, string>> truncated_player_names;

        for (size_t i{}; i < gs.get_number_of_players(); ++i)
        {

            auto &p = players_data[i];

            string key{p.player_name};
            const auto found_iter = unique_player_names.find(key);
            if (found_iter != end(unique_player_names))
            {
                --found_iter->second;
                if (found_iter->second <= 0)
                    unique_player_names.erase(found_iter);
            }
            else
            {
                truncated_player_names.emplace_back(i, std::move(key));
            }
        }

        if (!truncated_player_names.empty())
        {

            for (const auto &pd : truncated_player_names)
            {
                for (auto &unique_player_name : unique_player_names)
                {
                    if (unique_player_name.second > 0)
                    {

                        if ((unique_player_name.first.length() - pd.second.length() <= 2) &&
                            (unique_player_name.first.ends_with(pd.second) ||
                             unique_player_name.first.starts_with(pd.second)))
                        {
                            // print_message_about_corrected_player_name(app_handles.hwnd_re_messages_data,
                            // pd.second.c_str(), unique_player_name.first.c_str());
                            strcpy_s(players_data[pd.first].player_name, std::size(players_data[pd.first].player_name),
                                     unique_player_name.first.c_str());
                            --unique_player_name.second;
                            break;
                        }

                        if (unique_player_name.first.length() > pd.second.length() && !pd.second.empty())
                        {
                            size_t i{}, k{unique_player_name.first.length()}, j{pd.second.length()};

                            for (; i < pd.second.length(); ++i)
                            {
                                if (pd.second[i] != unique_player_name.first[i])
                                    break;
                            }

                            for (; j >= i; --j, --k)
                            {
                                if (pd.second[j - 1] != unique_player_name.first[k - 1])
                                    break;
                            }

                            if (j - i <= 1)
                            {
                                // print_message_about_corrected_player_name(app_handles.hwnd_re_messages_data,
                                // pd.second.c_str(), unique_player_name.first.c_str());
                                strcpy_s(players_data[pd.first].player_name,
                                         std::size(players_data[pd.first].player_name),
                                         unique_player_name.first.c_str());
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

void print_message_about_corrected_player_name(HWND re_hwnd, const char *truncated_name, const char *corrected_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (truncated_name && corrected_name)
    {
        char buffer[128]{};
        (void)snprintf(buffer, std::size(buffer), "^2Corrected truncated player name from ^7%s ^2to ^7%s\n",
                       truncated_name, corrected_name);
        print_colored_text(re_hwnd, buffer, is_append_message_to_richedit_control::yes, is_log_message::yes,
                           is_log_datetime::yes);
    }
}

void set_admin_actions_buttons_active(const BOOL is_enable, const bool is_reset_to_default_sort_mode)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    // EnableWindow(app_handles.hwnd_button_warn, is_enable);
    // EnableWindow(app_handles.hwnd_button_kick, is_enable);
    // EnableWindow(app_handles.hwnd_button_tempban, is_enable);
    // EnableWindow(app_handles.hwnd_button_ipban, is_enable);
    // EnableWindow(app_handles.hwnd_say_button, is_enable);
    // EnableWindow(app_handles.hwnd_tell_button, is_enable);
    // EnableWindow(app_handles.hwnd_button_load, is_enable);
    set_available_sort_methods(is_enable, is_reset_to_default_sort_mode);
}

void set_available_sort_methods(const bool is_admin, const bool is_reset_to_default_sort_mode)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_RESETCONTENT, 0, 0);
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by pid in asc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by pid in desc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by score in asc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by score in desc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by ping in asc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by ping in desc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by name in asc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by name in desc. order"));
    if (is_admin)
    {
        SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                    reinterpret_cast<LPARAM>("Sort by IP in asc. order"));
        SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                    reinterpret_cast<LPARAM>("Sort by IP in desc. order"));
        if (is_reset_to_default_sort_mode)
        {
            type_of_sort = sort_type::geo_asc;
        }
    }
    else if (is_reset_to_default_sort_mode)
    {
        type_of_sort = sort_type::score_desc;
    }
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by geoinfo in asc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>("Sort by geoinfo in desc. order"));
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_SELECTSTRING, static_cast<WPARAM>(-1),
                reinterpret_cast<LPARAM>(sort_type_to_sort_names_dict.at(type_of_sort).c_str()));
}

std::pair<bool, std::string> extract_7z_file_to_specified_path(const char *compressed_7z_file_path,
                                                               const char *destination_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    try
    {
        const string compressed_7z_file_path_utf8{compressed_7z_file_path};
        const string destination_path_utf8{destination_path};
        using namespace bit7z;
        Bit7zLibrary lib{"7za.dll"};
        BitFileExtractor extractor{lib, BitFormat::SevenZip};
        extractor.setOverwriteMode(bit7z::OverwriteMode::Overwrite);
        extractor.extract(compressed_7z_file_path_utf8, destination_path_utf8);
        return make_pair(true, string{});
    }
    catch (const std::exception &ex)
    {
        return make_pair(false, ex.what());
    }
}

std::pair<bool, std::string> create_7z_file_file_at_specified_path(const std::vector<std::string> &files_to_add,
                                                                   const std::string &local_file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    try
    {

        DeleteFile(local_file_path.c_str());
        using namespace bit7z;
        Bit7zLibrary lib{"7za.dll"};
        BitFileCompressor compressor{lib, BitFormat::SevenZip};
        compressor.setOverwriteMode(bit7z::OverwriteMode::Overwrite);
        compressor.compress(files_to_add, local_file_path);
        return make_pair(true, string{"SUCCESS"});
    }
    catch (const std::exception &ex)
    {
        return make_pair(false, ex.what());
    }
}

void save_banned_entries_to_file(const char *file_path, const set<string> &banned_entries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ofstream output_file(file_path, std::ios::out);

    for (const auto &banned_entry : banned_entries)
    {
        output_file << banned_entry << '\n';
    }

    output_file << flush;
    output_file.close();
}

void display_banned_cities(const std::set<std::string> &banned_cities)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ostringstream oss;
    if (banned_cities.empty())
    {
        oss << "\n^3You haven't banned any ^1cities ^3yet.\n";
    }
    else
    {
        oss << format("\n^5There {} ^1{} ^5banned {}:\n", banned_cities.size() != 1 ? "are" : "is",
                      banned_cities.size(), banned_cities.size() != 1 ? "cities" : "city");

        for (const auto &banned_city : banned_cities)
        {
            oss << "^1" << banned_city << '\n';
        }
    }

    oss << (main_app.get_is_automatic_city_kick_enabled() ? "\n^5The ^1automatic city ban ^5feature is ^2currently "
                                                            "enabled^5.\n\n"
                                                          : "\n^5The ^1automatic city ban ^5feature is ^1currently "
                                                            "disabled^5.\n\n");
    const string information{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, information.c_str(),
                       is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no, true);
}

void display_banned_countries(const std::set<std::string> &banned_countries)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ostringstream oss;
    if (banned_countries.empty())
    {
        oss << "\n^3You haven't banned any ^1countries ^3yet.\n";
    }
    else
    {
        oss << format("\n^5There {} ^1{} ^5banned {}:\n", banned_countries.size() != 1 ? "are" : "is",
                      banned_countries.size(), banned_countries.size() != 1 ? "countries" : "country");

        for (const auto &banned_country : banned_countries)
        {
            oss << "^1" << banned_country << '\n';
        }
    }

    oss << (main_app.get_is_automatic_country_kick_enabled()
                ? "\n^5The ^1automatic country ban ^5feature is ^2currently "
                  "enabled^5.\n\n"
                : "\n^5The ^1automatic country ban ^5feature is ^1currently "
                  "disabled^5.\n\n");

    const string information{oss.str()};
    print_colored_text(app_handles.hwnd_re_messages_data, information.c_str(),
                       is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no, true);
}

time_t get_current_time_stamp()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(now);
}

time_t get_number_of_seconds_from_date_and_time_string(const std::string &date_and_time)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const regex date_format1{R"((\d{1,2})/(\d{1,2})/(\d{4})\s+(\d{1,2}):(\d{1,2}))"};
    static const regex date_format2{R"((\d{1,2})\.(\d{1,2})\.(\d{4})\s+(\d{1,2}):(\d{1,2}))"};
    static const regex date_format3{R"((\d{4})-(\w+)-(\d{1,2})\s+(\d{1,2}):(\d{1,2}))"};
    static const unordered_map<string, int> short_month_name_index{
        {"Jan", 0}, {"Feb", 1}, {"Mar", 2}, {"Apr", 3}, {"May", 4},  {"Jun", 5},
        {"Jul", 6}, {"Aug", 7}, {"Sep", 8}, {"Oct", 9}, {"Nov", 10}, {"Dec", 11},
    };

    string ex_msg{format(R"(^1Exception ^3thrown from ^1time_t get_number_of_seconds_from_date_and_time_string("{}"))",
                         date_and_time)};
    stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

    smatch matches1{}, matches2{}, matches3{};
    string day, month, hour, minute;
    std::tm t{};

    if (regex_match(date_and_time, matches1, date_format1))
    {
        day = matches1[1].str();
        if (!day.empty() && '0' == day[0])
            day.erase(0, 1);
        t.tm_mday = stoi(day);
        month = matches1[2].str();
        if (!month.empty() && '0' == month[0])
            month.erase(0, 1);
        t.tm_mon = stoi(month) - 1;
        t.tm_year = stoi(matches1[3].str()) - 1900;

        hour = matches1[4].str();
        if (!hour.empty() && '0' == hour[0])
            hour.erase(0, 1);
        t.tm_hour = stoi(hour);

        minute = matches1[5].str();
        if (!minute.empty() && '0' == minute[0])
            minute.erase(0, 1);
        t.tm_min = stoi(minute);
    }
    else if (regex_match(date_and_time, matches2, date_format2))
    {
        day = matches2[1].str();
        if (!day.empty() && '0' == day[0])
            day.erase(0, 1);
        t.tm_mday = stoi(day);
        month = matches2[2].str();
        if (!month.empty() && '0' == month[0])
            month.erase(0, 1);
        t.tm_mon = stoi(month) - 1;
        t.tm_year = stoi(matches2[3].str()) - 1900;

        hour = matches2[4].str();
        if (!hour.empty() && '0' == hour[0])
            hour.erase(0, 1);
        t.tm_hour = stoi(hour);

        minute = matches2[5].str();
        if (!minute.empty() && '0' == minute[0])
            minute.erase(0, 1);
        t.tm_min = stoi(minute);
    }
    else if (regex_match(date_and_time, matches3, date_format3))
    {
        t.tm_year = stoi(matches3[1].str()) - 1900;
        month = matches3[2].str();
        if (short_month_name_index.contains(month))
            t.tm_mon = short_month_name_index.at(month);
        else
            t.tm_mon = 0;
        day = matches3[3].str();
        if (!day.empty() && '0' == day[0])
            day.erase(0, 1);
        t.tm_mday = stoi(day);

        hour = matches3[4].str();
        if (!hour.empty() && '0' == hour[0])
            hour.erase(0, 1);
        t.tm_hour = stoi(hour);

        minute = matches3[5].str();
        if (!minute.empty() && '0' == minute[0])
            minute.erase(0, 1);
        t.tm_min = stoi(minute);
    }

    return mktime(&t);
}

std::string get_narrow_ip_address_range_for_specified_ip_address(const std::string &ip_address)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const regex ip_range_regex{R"(^\d{1,3}\.\d{1,3}\.\d{1,3}\.\*$)"};
    smatch matches;
    if (regex_match(ip_address, matches, ip_range_regex))
        return ip_address;
    unsigned long guid{};
    if (!check_ip_address_validity(ip_address, guid))
        return {"n/a"};
    size_t last{ip_address.length() - 1};
    while (ip_address[last] != '.')
        --last;
    return {string(ip_address.cbegin(), ip_address.cbegin() + last) + ".*"};
}

std::string get_wide_ip_address_range_for_specified_ip_address(const std::string &ip_address)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const regex ip_range_regex{R"(^\d{1,3}\.\d{1,3}\.\*\.\*$)"};
    smatch matches;
    if (regex_match(ip_address, matches, ip_range_regex))
        return ip_address;
    unsigned long guid{};
    if (!check_ip_address_validity(ip_address, guid))
        return {"n/a"};
    size_t last{ip_address.length() - 1};

    while (ip_address[last] != '.')
        --last;
    --last;
    while (ip_address[last] != '.')
        --last;
    return {string(ip_address.cbegin(), ip_address.cbegin() + last) + ".*.*"};
}

void check_if_admins_are_online_and_get_admins_player_names(const std::vector<player> &players,
                                                            size_t no_of_online_players)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    unsigned long guid_key{};
    no_of_online_players = std::min(no_of_online_players, players.size());
    for (auto &u : main_app.get_users())
    {
        if (!check_ip_address_validity(u->ip_address, guid_key))
            continue;
        u->is_online = false;
        for (size_t i{}; i < no_of_online_players; ++i)
        {
            if (u->ip_address == players[i].ip_address)
            {
                u->is_online = true;
                u->player_name = players[i].player_name;
                break;
            }
        }
    }
}

bool save_current_user_data_to_json_file(const char *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    std::ofstream config_file{file_path};

    if (!config_file)
    {
        const string re_msg{format("^1Error ^3saving ^5Tiny^6Rcon ^1user's data ^3to specified "
                                   "file:\n\t^5{}^3Please make sure that the ^5Tiny^6Rcon ^3folder is not "
                                   "set to ^1read-only ^3mode.",
                                   file_path)};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str());
        return false;
    }

    /*auto me = shared_ptr<tiny_rcon_client_user>{};
    if (me->is_admin) {
      me = main_app.get_user_for_name(main_app.get_username());
    } else {
      me = main_app.get_player_for_name(main_app.get_username(),
    main_app.get_user_ip_address());
    }*/

    const string user_name{remove_disallowed_character_in_string(me->user_name)};
    config_file << format(
        R"({}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{})", user_name, (me->is_admin ? "true" : "false"),
        (me->is_logged_in ? "true" : "false"), (me->is_online ? "true" : "false"), me->ip_address, me->geo_information,
        me->last_login_time_stamp, me->last_logout_time_stamp, me->no_of_logins, me->no_of_warnings, me->no_of_kicks,
        me->no_of_tempbans, me->no_of_guidbans, me->no_of_ipbans, me->no_of_iprangebans, me->no_of_citybans,
        me->no_of_countrybans, me->no_of_namebans, me->no_of_reports);

    config_file << flush;

    return true;
}

void load_tinyrcon_client_user_data(const char *file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ifstream configFile{file_path};

    if (!configFile)
    {
        save_current_user_data_to_json_file(file_path);
        configFile.open(file_path, std::ios_base::in);
        if (!configFile)
        {
            const auto f_path{fs::path(file_path)};
            const string re_msg{format("^1Error ^3opening ^5Tiny^6Rcon ^1user's data "
                                       "^3file:\n\t^5{}\n^3Please make sure that the ^5{} ^3file\n\texists "
                                       "and that it is not set to ^1read-only ^3mode.",
                                       file_path, (f_path.has_filename() ? f_path.filename().string() : file_path))};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str());
            return;
        }
    }

    string data{};
    if (!getline(configFile, data))
    {
        data = "^3Player\\false\\false\\false\\n/a\\Unknown, "
               "Unknown\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0";
    }

    trim_in_place(data);
    auto parts = str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts)
    {
        trim_in_place(part);
    }

    string username{parts.size() >= 1 ? parts[0] : "^3Player"};
    // const bool is_admin{ parts.size() >= 2 && parts[1] == "true" ? true : false
    // };
    const bool is_logged_in{parts.size() >= 3 && parts[2] == "true" ? true : false};
    const bool is_online{parts.size() >= 4 && parts[3] == "true" ? true : false};
    unsigned long guid{};
    string ip_address = check_ip_address_validity(main_app.get_user_ip_address(), guid)
                            ? main_app.get_user_ip_address()
                            : (parts.size() >= 5 ? parts[4] : "n/a");
    shared_ptr<tiny_rcon_client_user> u{main_app.get_current_game_server().get_is_connection_settings_valid()
                                            ? main_app.get_user_for_name(username)
                                            : main_app.get_player_for_name(username, main_app.get_user_ip_address())};
    u->is_admin = main_app.get_current_game_server().get_is_connection_settings_valid();
    u->user_name = std::move(username);
    u->is_logged_in = is_logged_in;
    u->is_online = is_online;

    if (check_ip_address_validity(ip_address, guid))
    {
        player admin{};
        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), ip_address, admin);
        u->ip_address = std::move(ip_address);
        u->country_code = admin.country_code;
        u->geo_information = format("{}, {}", admin.country_name, admin.city);
    }
    else
    {
        u->ip_address = "n/a";
        u->geo_information = parts.size() >= 6 ? parts[5] : "Unknown, Unknown";
        u->country_code = "xy";
    }

    int number{};
    u->last_login_time_stamp = parts.size() >= 7 && is_valid_decimal_whole_number(parts[6], number) ? number : 0;
    u->last_logout_time_stamp = parts.size() >= 8 && is_valid_decimal_whole_number(parts[7], number) ? number : 0;
    u->no_of_logins =
        static_cast<size_t>(parts.size() >= 9 && is_valid_decimal_whole_number(parts[8], number) ? number : 0);
    u->no_of_warnings =
        static_cast<size_t>(parts.size() >= 10 && is_valid_decimal_whole_number(parts[9], number) ? number : 0);
    u->no_of_kicks =
        static_cast<size_t>(parts.size() >= 11 && is_valid_decimal_whole_number(parts[10], number) ? number : 0);
    u->no_of_tempbans =
        static_cast<size_t>(parts.size() >= 12 && is_valid_decimal_whole_number(parts[11], number) ? number : 0);
    u->no_of_guidbans =
        static_cast<size_t>(parts.size() >= 13 && is_valid_decimal_whole_number(parts[12], number) ? number : 0);
    u->no_of_ipbans =
        static_cast<size_t>(parts.size() >= 14 && is_valid_decimal_whole_number(parts[13], number) ? number : 0);
    u->no_of_iprangebans =
        static_cast<size_t>(parts.size() >= 15 && is_valid_decimal_whole_number(parts[14], number) ? number : 0);
    u->no_of_citybans =
        static_cast<size_t>(parts.size() >= 16 && is_valid_decimal_whole_number(parts[15], number) ? number : 0);
    u->no_of_countrybans =
        static_cast<size_t>(parts.size() >= 17 && is_valid_decimal_whole_number(parts[16], number) ? number : 0);
    u->no_of_namebans =
        static_cast<size_t>(parts.size() >= 18 && is_valid_decimal_whole_number(parts[17], number) ? number : 0);
    u->no_of_reports =
        static_cast<size_t>(parts.size() >= 19 && is_valid_decimal_whole_number(parts[18], number) ? number : 0);
}

bool validate_admin_and_show_missing_admin_privileges_message(const bool is_show_message_box,
                                                              const is_log_message log_message,
                                                              const is_log_datetime log_date_time)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (!me->is_admin)
    {
        string warning_msg{format("^7{}^3, you need to have the correct ^1rcon password ^3to be able to "
                                  "execute ^1admin-level ^3commands.\n",
                                  main_app.get_username())};
        print_colored_text(app_handles.hwnd_re_messages_data, warning_msg.c_str(),
                           is_append_message_to_richedit_control::yes, log_message, log_date_time);
        if (is_show_message_box)
        {
            remove_all_color_codes(warning_msg);
            MessageBoxA(app_handles.hwnd_main_window, "Warning", warning_msg.c_str(), MB_ICONWARNING | MB_OK);
        }
        return false;
    }

    return true;
}

void removed_disallowed_character_in_string(std::string &input)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    // const string disallowed_chars{ "\\/-[]\"'{|}%:;=+\b\f\n\r\t " };
    string_view disallowed_chars{"\\", len("\\")};
    const std::unordered_set<char> disallowed_characters(cbegin(disallowed_chars), cend(disallowed_chars));
    string cleaned_input;
    for (const char ch : input)
    {
        if (!disallowed_characters.contains(ch))
            cleaned_input.push_back(ch);
    }
    input = std::move(cleaned_input);
}

std::string remove_disallowed_character_in_string(const std::string &input)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    // const string disallowed_chars{ "\\/-[]\"'{|}%:;=+\b\f\n\r\t " };
    string_view disallowed_chars{"\\", len("\\")};
    const std::unordered_set<char> disallowed_characters(cbegin(disallowed_chars), cend(disallowed_chars));
    string cleaned_input;
    for (const char ch : input)
    {
        if (!disallowed_characters.contains(ch))
            cleaned_input.push_back(ch);
        else
            cleaned_input.push_back('_');
    }

    return cleaned_input;
}

std::string remove_disallowed_characters_in_ip_address(const std::string &ip_address)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    // const string disallowed_chars{ "\\/-[]\"'{|}%:;=+\b\f\n\r\t " };
    string_view disallowed_chars{".\\[]:", len(".\\[]:")};
    const std::unordered_set<char> disallowed_characters(cbegin(disallowed_chars), cend(disallowed_chars));
    string cleaned_input;
    for (const char ch : ip_address)
    {
        if (!disallowed_characters.contains(ch))
            cleaned_input.push_back(ch);
    }

    return cleaned_input;
}

size_t ltrim_specified_characters(char *buffer, const size_t buffer_len, const char *needle_chars)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const std::unordered_set<char> disallowed_characters(needle_chars, needle_chars + len(needle_chars));
    size_t i{};
    while (i < buffer_len)
    {
        if (!disallowed_characters.contains(buffer[i]))
            break;
        ++i;
    }
    if (i > 0)
    {
        std::copy(buffer + i, buffer + buffer_len, buffer);
    }
    return i;
}

std::string get_cleaned_user_name(const std::string &name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string admin_name{remove_disallowed_character_in_string(name)};
    remove_all_color_codes(admin_name);
    trim_in_place(admin_name);
    to_lower_case_in_place(admin_name);
    return admin_name;
}

void replace_br_with_new_line(std::string &message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    for (size_t next{string::npos}; (next = message.find("{{br}}")) != string::npos;)
    {
        message.replace(next, 6, "\n");
    }
}

void get_first_valid_ip_address_from_ip_address_range(std::string ip_range, player &pd)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (ip_range.ends_with("*.*"))
    {
        ip_range.replace(ip_range.length() - 3, 3, "1.1");
    }
    else if (ip_range.ends_with(".*"))
    {
        ip_range.back() = '1';
    }

    pd.ip_address = ip_range;
    convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), ip_range, pd);
}

bool run_executable(const char *file_path_for_executable)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    PROCESS_INFORMATION ProcessInfo{};
    STARTUPINFO StartupInfo{};
    StartupInfo.cb = sizeof(StartupInfo);

    if (CreateProcess(file_path_for_executable, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &StartupInfo,
                      &ProcessInfo))
    {
        if (StartupInfo.hStdError)
            CloseHandle(StartupInfo.hStdError);
        if (StartupInfo.hStdInput)
            CloseHandle(StartupInfo.hStdInput);
        if (StartupInfo.hStdOutput)
            CloseHandle(StartupInfo.hStdOutput);
        CloseHandle(ProcessInfo.hThread);
        CloseHandle(ProcessInfo.hProcess);
        return true;
    }

    return false;
}

void restart_tinyrcon_client(const char *file_path_to_tinyrcon_exe, const string &file_path_to_temporary_tinyrcon_exe,
                             const string &file_path_to_old_tinyrcon_exe)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    // if
    // (run_executable(main_app.get_auto_update_manager().get_self_full_path().c_str()))
    // {
    if (run_executable(file_path_to_tinyrcon_exe))
    {
        if (!file_path_to_temporary_tinyrcon_exe.empty() && !file_path_to_old_tinyrcon_exe.empty() &&
            file_path_to_temporary_tinyrcon_exe != file_path_to_old_tinyrcon_exe)
        {
            MoveFile(file_path_to_temporary_tinyrcon_exe.c_str(), file_path_to_old_tinyrcon_exe.c_str());
        }
        is_terminate_program.store(true);
        PostQuitMessage(0);
        CloseWindow(app_handles.hwnd_main_window);
        DestroyWindow(app_handles.hwnd_main_window);
        ExitProcess(0);
    }
}

size_t get_random_number()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static random_device rd{};
    static std::mt19937_64 rand_engine(rd());
    static std::uniform_int_distribution<size_t> number_range(1000ULL, std::numeric_limits<size_t>::max());
    return number_range(rand_engine);
}

bool parse_game_type_information_from_rcon_reply(const string &incoming_data_buffer, game_server &gs)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (size_t first_pos{}; (first_pos = incoming_data_buffer.find(R"("g_gametype" is: ")")) != string::npos &&
                            incoming_data_buffer.find("default: \"") != string::npos)
    {
        string ex_msg3{format(
            R"(^1Exception ^3thrown from 'if (strstr(incoming_data_buffer, "g_gametype" is: ") != nullptr){{...}}'\nincoming_data_buffer="{}")",
            incoming_data_buffer)};
        stack_trace_element ste3{app_handles.hwnd_re_messages_data, std::move(ex_msg3)};
        main_app.get_current_game_server().set_is_connection_settings_valid(true);
        first_pos += strlen(R"("g_gametype" is: ")");
        const size_t last_pos{incoming_data_buffer.find_first_of("^7\" ", first_pos)};
        if (string::npos != last_pos && last_pos < incoming_data_buffer.length() && last_pos - first_pos <= 3)
        {
            gs.set_current_game_type(incoming_data_buffer.substr(first_pos, last_pos - first_pos));
        }
        else
        {
            gs.set_current_game_type("ctf");
        }
        return true;
    }

    return false;
}

std::string find_users_player_name_for_installed_cod2_game(const std::shared_ptr<tiny_rcon_client_user> &user,
                                                           const std::string &mod_folder_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const string player_name{"^7Unknown Soldier"};

    if (user->is_admin)
    {
        unsigned long guid_key{};
        const auto &user_ip_address = main_app.get_user_ip_address();
        if (!user_ip_address.empty() && check_ip_address_validity(user_ip_address, guid_key))
        {
            const auto &players_data = main_app.get_current_game_server().get_players_data();
            for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
            {
                if (players_data[i].ip_address == user_ip_address)
                    return players_data[i].player_name;
            }
        }
    }

    if (!main_app.get_cod2mp_exe_path().empty())
    {
        string cod2_game_folder{main_app.get_cod2mp_exe_path()};
        cod2_game_folder.erase(cbegin(cod2_game_folder) + cod2_game_folder.find_last_of("\\/"), cend(cod2_game_folder));
        string current_path{cod2_game_folder + "\\main\\players\\active.txt"};
        if (check_if_file_path_exists(current_path.c_str()))
        {
            ifstream input_file1{current_path};
            if (string line1; input_file1 && getline(input_file1, line1))
            {
                const string user_name_folder{stl::helper::trim(line1)};
                input_file1.close();

                const string player_config_mp_file_path{
                    format("{}\\{}\\players\\{}\\config_mp.cfg", cod2_game_folder, mod_folder_name, user_name_folder)};

                if (check_if_file_path_exists(player_config_mp_file_path.c_str()))
                {

                    ifstream input_file2(player_config_mp_file_path, std::ios::in);
                    if (input_file2)
                    {

                        for (string line2; getline(input_file2, line2);)
                        {
                            trim_in_place(line2);
                            const size_t start_pos1{line2.find("seta name")};
                            if (string::npos != start_pos1)
                            {
                                input_file2.close();
                                string name{trim(line2.substr(start_pos1 + len("seta name") + 1))};
                                strip_leading_and_trailing_quotes(name);
                                return name;
                            }

                            const size_t start_pos2{line2.find("set name")};
                            if (string::npos != start_pos2)
                            {
                                input_file2.close();
                                string name{trim(line2.substr(start_pos1 + len("set name") + 1))};
                                strip_leading_and_trailing_quotes(name);
                                return name;
                            }
                        }
                    }

                    const string player_config_mp_file_path_in_main{
                        format("{}\\main\\players\\{}\\config_mp.cfg", cod2_game_folder, user_name_folder)};

                    if (check_if_file_path_exists(player_config_mp_file_path_in_main.c_str()))
                    {
                        ifstream input_file3(player_config_mp_file_path_in_main, std::ios::in);
                        if (input_file3)
                        {

                            for (string line3; getline(input_file3, line3);)
                            {
                                trim_in_place(line3);
                                const size_t start_pos1{line3.find("seta name")};
                                if (string::npos != start_pos1)
                                {
                                    input_file3.close();
                                    string name{trim(line3.substr(start_pos1 + len("seta name") + 1))};
                                    strip_leading_and_trailing_quotes(name);
                                    return name;
                                }

                                const size_t start_pos2{line3.find("set name")};
                                if (string::npos != start_pos2)
                                {
                                    input_file3.close();
                                    string name{trim(line3.substr(start_pos1 + len("set name") + 1))};
                                    strip_leading_and_trailing_quotes(name);
                                    return name;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return player_name;
}

std::string find_version_of_installed_cod2_game()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static const vector<char> cod2_1_0_version_info{'C', 'o', 'D', '2', ' ', 'M', 'P', 0, '1', '.', '0', 0};
    static const vector<char> cod2_1_01_version_info{'C', 'o', 'D', '2', ' ', 'M', 'P', 0, '1', '.', '0', '1', 0};
    static const vector<char> cod2_1_2_version_info{'C', 'o', 'D', '2', ' ', 'M', 'P', 0, '1', '.', '2', 0};
    static const vector<char> cod2_1_3_version_info{'C', 'o', 'D', '2', ' ', 'M', 'P', 0, '1', '.', '3', 0};

    string game_version;

    const size_t file_size{get_file_size_in_bytes(main_app.get_cod2mp_exe_path().c_str())};
    ifstream input{main_app.get_cod2mp_exe_path().c_str(), std::ios::binary};
    vector<char> file_buffer(file_size, 0);
    input.read(file_buffer.data(), file_size);
    auto found_iter =
        find_end(cbegin(file_buffer), cend(file_buffer), cbegin(cod2_1_0_version_info), cend(cod2_1_0_version_info));
    if (found_iter != cend(file_buffer))
        return "1.0";

    found_iter =
        find_end(cbegin(file_buffer), cend(file_buffer), cbegin(cod2_1_01_version_info), cend(cod2_1_01_version_info));
    if (found_iter != cend(file_buffer))
        return "1.01";

    found_iter =
        find_end(cbegin(file_buffer), cend(file_buffer), cbegin(cod2_1_2_version_info), cend(cod2_1_2_version_info));
    if (found_iter != cend(file_buffer))
        return "1.2";

    found_iter =
        find_end(cbegin(file_buffer), cend(file_buffer), cbegin(cod2_1_3_version_info), cend(cod2_1_3_version_info));
    if (found_iter != cend(file_buffer))
        return "1.3";

    return "1.0";
}

// bool download_missing_cod2_game_patch_files()
// {
//   if (!check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str())) {
//     find_call_of_duty_2_installation_path(false);
//   }
//   const char *cod2mp_s_file_path = main_app.get_cod2mp_exe_path().c_str();
//   if (!cod2mp_s_file_path || len(cod2mp_s_file_path) == 0 ||
//   strcmp(cod2mp_s_file_path, "C:\\") == 0 ||
//   !check_if_file_path_exists(cod2mp_s_file_path))
//     return false;
//
//   bool is_downloaded_patch_file{};
//
//   path cod2mp_s_path{ cod2mp_s_file_path };
//   const string cod2_patches_folder_path{ cod2mp_s_path.parent_path().string()
//   + "\\cod2_patches" };
//
//   static const vector<string> necessary_folders{
//     cod2_patches_folder_path,
//     format("{}\\1_0", cod2_patches_folder_path),
//     format("{}\\1_01", cod2_patches_folder_path),
//     format("{}\\1_2", cod2_patches_folder_path),
//     format("{}\\1_2\\main", cod2_patches_folder_path),
//     format("{}\\1_3", cod2_patches_folder_path),
//     format("{}\\1_3\\main", cod2_patches_folder_path),
//     format("{}\\cod_temp_file_remover", cod2_patches_folder_path)
//   };
//
//   if (!create_necessary_folders_and_files(necessary_folders))
//     return false;
//
//
//   const string ftp_download_site_info{ "ftp://"s +
//   main_app.get_ftp_download_site_ip_address() +
//   (!main_app.get_ftp_download_folder_path().empty() ? "/"s +
//   main_app.get_ftp_download_folder_path() + "/"s : "/"s) };
//
//
//   static const unordered_map<string, string>
//     cod2_1_0_patch_files{
//       { format("{}\\1_0\\CoD2MP_s.exe", cod2_patches_folder_path),
//       "cod2_patches/1_0/CoD2MP_s.exe" }, {
//       format("{}\\1_0\\gfx_d3d_mp_x86_s.dll", cod2_patches_folder_path),
//         "cod2_patches/1_0/gfx_d3d_mp_x86_s.dll" },
//       { format("{}\\1_0\\gfx_d3d_x86_s.dll", cod2_patches_folder_path),
//         "cod2_patches/1_0/gfx_d3d_x86_s.dll" },
//       { format("{}\\1_0\\mss32.dll", cod2_patches_folder_path),
//       "cod2_patches/1_0/mss32.dll" }
//     };
//
//   for (const auto &file_path : cod2_1_0_patch_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading missing CoD2 v1.0 game
//       file ^5{}^3...\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded missing
//         CoD2 v1.0 game file ^5{}^2.\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   static const unordered_map<string, string>
//     cod2_1_01_patch_files{
//       { format("{}\\1_01\\CoD2MP_s.exe", cod2_patches_folder_path),
//       "cod2_patches/1_01/CoD2MP_s.exe" }, {
//       format("{}\\1_01\\gfx_d3d_mp_x86_s.dll", cod2_patches_folder_path),
//       "cod2_patches/1_01/gfx_d3d_mp_x86_s.dll" }, {
//       format("{}\\1_01\\gfx_d3d_x86_s.dll", cod2_patches_folder_path),
//       "cod2_patches/1_01/gfx_d3d_x86_s.dll" }, {
//       format("{}\\1_01\\mss32.dll", cod2_patches_folder_path),
//       "cod2_patches/1_01/mss32.dll" },
//       // { format("{}\\1_01\\version.inf", cod2_patches_folder_path),
//       "cod2_patches/1_01/version.inf" }
//     };
//
//   for (const auto &file_path : cod2_1_01_patch_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading missing CoD2 v1.01 game
//       file ^5{}^3...\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded missing
//         CoD2 v1.01 game file ^5{}^2.\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   static const unordered_map<string, string>
//     cod2_1_2_patch_files{
//       { format("{}\\1_2\\CoD2MP_s.exe", cod2_patches_folder_path),
//         "cod2_patches/1_2/CoD2MP_s.exe" },
//       { format("{}\\1_2\\gfx_d3d_mp_x86_s.dll", cod2_patches_folder_path),
//         "cod2_patches/1_2/gfx_d3d_mp_x86_s.dll" },
//       { format("{}\\1_2\\gfx_d3d_x86_s.dll", cod2_patches_folder_path),
//         "cod2_patches/1_2/gfx_d3d_x86_s.dll" },
//       { format(R"({}\1_2\main\iw_15.iwd)", cod2_patches_folder_path),
//       "cod2_patches/1_2/main/iw_15.iwd" }
//     };
//
//   for (const auto &file_path : cod2_1_2_patch_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading missing CoD2 v1.2 game
//       file ^5{}^3...\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded missing
//         CoD2 v1.2 game file ^5{}^2.\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   static const unordered_map<string, string>
//     cod2_1_3_patch_files{
//       { format("{}\\1_3\\CoD2MP_s.exe", cod2_patches_folder_path),
//       "cod2_patches/1_3/CoD2MP_s.exe" }, {
//       format("{}\\1_3\\gfx_d3d_mp_x86_s.dll", cod2_patches_folder_path),
//       "cod2_patches/1_3/gfx_d3d_mp_x86_s.dll" }, {
//       format("{}\\1_3\\gfx_d3d_x86_s.dll", cod2_patches_folder_path),
//       "cod2_patches/1_3/gfx_d3d_x86_s.dll" }, {
//       format(R"({}\1_3\main\iw_15.iwd)", cod2_patches_folder_path),
//       "cod2_patches/1_3/main/iw_15.iwd" }, {
//       format(R"({}\1_3\main\localized_english_iw11.iwd)",
//       cod2_patches_folder_path),
//       "cod2_patches/1_3/main/localized_english_iw11.iwd" }
//     };
//
//   for (const auto &file_path : cod2_1_3_patch_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading missing CoD2 v1.3 game
//       file ^5{}^3...\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded missing
//         CoD2 v1.3 game file ^5{}^2.\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   static const unordered_map<string, string>
//     cod2_patches_script_files{
//       { format("{}\\install_cod2_1.0_eng_patch.bat",
//       cod2_patches_folder_path),
//       "cod2_patches/install_cod2_1.0_eng_patch.bat" }, {
//       format("{}\\install_cod2_1.01_eng_patch.bat",
//       cod2_patches_folder_path),
//       "cod2_patches/install_cod2_1.01_eng_patch.bat" }, {
//       format("{}\\install_cod2_1.2_eng_patch.bat", cod2_patches_folder_path),
//       "cod2_patches/install_cod2_1.2_eng_patch.bat" }, {
//       format("{}\\install_cod2_1.3_eng_patch.bat", cod2_patches_folder_path),
//       "cod2_patches/install_cod2_1.3_eng_patch.bat" }, {
//       format("{}\\cod_temp_file_remover\\cod_temp_file_remover.exe",
//       cod2_patches_folder_path),
//       "cod2_patches/cod_temp_file_remover/cod_temp_file_remover.exe" }
//     };
//
//   for (const auto &file_path : cod2_patches_script_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading a missing CoD2 patch
//       related script file: ^5{}\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded a
//         missing CoD2 patch related script file: ^5{}\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   return is_downloaded_patch_file;
// }
//
// bool check_if_cod2_v1_0_game_patch_files_are_missing_and_download_them()
//{
//   if (!check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str())) {
//     find_call_of_duty_2_installation_path(true);
//   }
//   const char *cod2mp_s_file_path = main_app.get_cod2mp_exe_path().c_str();
//   if (!cod2mp_s_file_path || len(cod2mp_s_file_path) == 0 ||
//   strcmp(cod2mp_s_file_path, "C:\\") == 0 ||
//   !check_if_file_path_exists(cod2mp_s_file_path))
//     return false;
//
//   bool is_downloaded_patch_file{};
//
//   path cod2mp_s_path{ cod2mp_s_file_path };
//   const string cod2_patches_folder_path{ cod2mp_s_path.parent_path().string()
//   + "\\cod2_patches" };
//
//   static const vector<string> necessary_folders{
//     cod2_patches_folder_path,
//     format("{}\\1_0", cod2_patches_folder_path)
//   };
//
//   if (!create_necessary_folders_and_files(necessary_folders))
//     return false;
//
//
//   const string ftp_download_site_info{ "ftp://"s +
//   main_app.get_ftp_download_site_ip_address() +
//   (!main_app.get_ftp_download_folder_path().empty() ? "/"s +
//   main_app.get_ftp_download_folder_path() + "/"s : "/"s) };
//
//
//   static const unordered_map<string, string>
//     cod2_1_0_patch_files{
//       { format("{}\\1_0\\CoD2MP_s.exe", cod2_patches_folder_path),
//       "cod2_patches/1_0/CoD2MP_s.exe" }, {
//       format("{}\\1_0\\gfx_d3d_mp_x86_s.dll", cod2_patches_folder_path),
//         "cod2_patches/1_0/gfx_d3d_mp_x86_s.dll" },
//       { format("{}\\1_0\\gfx_d3d_x86_s.dll", cod2_patches_folder_path),
//         "cod2_patches/1_0/gfx_d3d_x86_s.dll" },
//       { format("{}\\1_0\\mss32.dll", cod2_patches_folder_path),
//       "cod2_patches/1_0/mss32.dll" }
//     };
//
//   for (const auto &file_path : cod2_1_0_patch_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading missing CoD2 v1.0 game
//       file ^5{}^3...\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded missing
//         CoD2 v1.0 game file ^5{}^2.\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   return is_downloaded_patch_file;
// }
//
// bool check_if_cod2_v1_01_game_patch_files_are_missing_and_download_them()
//{
//   if (!check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str())) {
//     find_call_of_duty_2_installation_path(true);
//   }
//   const char *cod2mp_s_file_path = main_app.get_cod2mp_exe_path().c_str();
//   if (!cod2mp_s_file_path || len(cod2mp_s_file_path) == 0 ||
//   strcmp(cod2mp_s_file_path, "C:\\") == 0 ||
//   !check_if_file_path_exists(cod2mp_s_file_path))
//     return false;
//
//   bool is_downloaded_patch_file{};
//
//   path cod2mp_s_path{ cod2mp_s_file_path };
//   const string cod2_patches_folder_path{ cod2mp_s_path.parent_path().string()
//   + "\\cod2_patches" };
//
//   static const vector<string> necessary_folders{
//     cod2_patches_folder_path,
//     format("{}\\1_01", cod2_patches_folder_path)
//   };
//
//   if (!create_necessary_folders_and_files(necessary_folders))
//     return false;
//
//
//   const string ftp_download_site_info{ "ftp://"s +
//   main_app.get_ftp_download_site_ip_address() +
//   (!main_app.get_ftp_download_folder_path().empty() ? "/"s +
//   main_app.get_ftp_download_folder_path() + "/"s : "/"s) };
//
//   static const unordered_map<string, string>
//     cod2_1_01_patch_files{
//       { format("{}\\1_01\\CoD2MP_s.exe", cod2_patches_folder_path),
//       "cod2_patches/1_01/CoD2MP_s.exe" }, {
//       format("{}\\1_01\\gfx_d3d_mp_x86_s.dll", cod2_patches_folder_path),
//       "cod2_patches/1_01/gfx_d3d_mp_x86_s.dll" }, {
//       format("{}\\1_01\\gfx_d3d_x86_s.dll", cod2_patches_folder_path),
//       "cod2_patches/1_01/gfx_d3d_x86_s.dll" }, {
//       format("{}\\1_01\\mss32.dll", cod2_patches_folder_path),
//       "cod2_patches/1_01/mss32.dll" },
//       // { format("{}\\1_01\\version.inf", cod2_patches_folder_path),
//       "cod2_patches/1_01/version.inf" }
//     };
//
//   for (const auto &file_path : cod2_1_01_patch_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading missing CoD2 v1.01 game
//       file ^5{}^3...\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded missing
//         CoD2 v1.01 game file ^5{}^2.\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   return is_downloaded_patch_file;
// }
//
// bool check_if_cod2_v1_2_game_patch_files_are_missing_and_download_them()
// {
//   if (!check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str())) {
//     find_call_of_duty_2_installation_path(true);
//   }
//   const char *cod2mp_s_file_path = main_app.get_cod2mp_exe_path().c_str();
//   if (!cod2mp_s_file_path || len(cod2mp_s_file_path) == 0 ||
//   strcmp(cod2mp_s_file_path, "C:\\") == 0 ||
//   !check_if_file_path_exists(cod2mp_s_file_path))
//     return false;
//
//   bool is_downloaded_patch_file{};
//
//   path cod2mp_s_path{ cod2mp_s_file_path };
//   const string cod2_patches_folder_path{ cod2mp_s_path.parent_path().string()
//   + "\\cod2_patches" };
//
//   static const vector<string> necessary_folders{
//     cod2_patches_folder_path,
//     format("{}\\1_2", cod2_patches_folder_path),
//     format("{}\\1_2\\main", cod2_patches_folder_path)
//   };
//
//   if (!create_necessary_folders_and_files(necessary_folders))
//     return false;
//
//
//   const string ftp_download_site_info{ "ftp://"s +
//   main_app.get_ftp_download_site_ip_address() +
//   (!main_app.get_ftp_download_folder_path().empty() ? "/"s +
//   main_app.get_ftp_download_folder_path() + "/"s : "/"s) };
//
//   static const unordered_map<string, string>
//     cod2_1_2_patch_files{
//       { format("{}\\1_2\\CoD2MP_s.exe", cod2_patches_folder_path),
//         "cod2_patches/1_2/CoD2MP_s.exe" },
//       { format("{}\\1_2\\gfx_d3d_mp_x86_s.dll", cod2_patches_folder_path),
//         "cod2_patches/1_2/gfx_d3d_mp_x86_s.dll" },
//       { format("{}\\1_2\\gfx_d3d_x86_s.dll", cod2_patches_folder_path),
//         "cod2_patches/1_2/gfx_d3d_x86_s.dll" },
//       { format(R"({}\1_2\main\iw_15.iwd)", cod2_patches_folder_path),
//       "cod2_patches/1_2/main/iw_15.iwd" }
//     };
//
//   for (const auto &file_path : cod2_1_2_patch_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading missing CoD2 v1.2 game
//       file ^5{}^3...\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded missing
//         CoD2 v1.2 game file ^5{}^2.\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   return is_downloaded_patch_file;
// }
//
// bool check_if_cod2_v1_3_game_patch_files_are_missing_and_download_them()
//{
//   if (!check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str())) {
//     find_call_of_duty_2_installation_path(true);
//   }
//   const char *cod2mp_s_file_path = main_app.get_cod2mp_exe_path().c_str();
//   if (!cod2mp_s_file_path || len(cod2mp_s_file_path) == 0 ||
//   strcmp(cod2mp_s_file_path, "C:\\") == 0 ||
//   !check_if_file_path_exists(cod2mp_s_file_path))
//     return false;
//
//   bool is_downloaded_patch_file{};
//
//   path cod2mp_s_path{ cod2mp_s_file_path };
//   const string cod2_patches_folder_path{ cod2mp_s_path.parent_path().string()
//   + "\\cod2_patches" };
//
//   static const vector<string> necessary_folders{
//     cod2_patches_folder_path,
//     format("{}\\1_3", cod2_patches_folder_path),
//     format("{}\\1_3\\main", cod2_patches_folder_path)
//   };
//
//   if (!create_necessary_folders_and_files(necessary_folders))
//     return false;
//
//
//   const string ftp_download_site_info{ "ftp://"s +
//   main_app.get_ftp_download_site_ip_address() +
//   (!main_app.get_ftp_download_folder_path().empty() ? "/"s +
//   main_app.get_ftp_download_folder_path() + "/"s : "/"s) };
//
//   static const unordered_map<string, string>
//     cod2_1_3_patch_files{
//       { format("{}\\1_3\\CoD2MP_s.exe", cod2_patches_folder_path),
//       "cod2_patches/1_3/CoD2MP_s.exe" }, {
//       format("{}\\1_3\\gfx_d3d_mp_x86_s.dll", cod2_patches_folder_path),
//       "cod2_patches/1_3/gfx_d3d_mp_x86_s.dll" }, {
//       format("{}\\1_3\\gfx_d3d_x86_s.dll", cod2_patches_folder_path),
//       "cod2_patches/1_3/gfx_d3d_x86_s.dll" }, {
//       format(R"({}\1_3\main\iw_15.iwd)", cod2_patches_folder_path),
//       "cod2_patches/1_3/main/iw_15.iwd" }, {
//       format(R"({}\1_3\main\localized_english_iw11.iwd)",
//       cod2_patches_folder_path),
//       "cod2_patches/1_3/main/localized_english_iw11.iwd" }
//     };
//
//   for (const auto &file_path : cod2_1_3_patch_files) {
//     if (!check_if_file_path_exists(file_path.first.c_str())) {
//       const string file_name{ string(file_path.second.cbegin() +
//       file_path.second.rfind('/') + 1, file_path.second.cend()) }; const
//       string download_url_buffer{ format("ftp://{}/{}/{}",
//       main_app.get_ftp_download_site_ip_address(),
//       main_app.get_ftp_download_folder_path(), file_path.second) }; const
//       string message_before{ format("^3Downloading missing CoD2 v1.3 game
//       file ^5{}^3...\n", file_name) };
//       print_colored_text(app_handles.hwnd_re_messages_data,
//       message_before.c_str(), is_append_message_to_richedit_control::yes,
//       is_log_message::yes, is_log_datetime::yes); if
//       (main_app.get_auto_update_manager().download_file(download_url_buffer.c_str(),
//       file_path.first.c_str())) {
//         const string message_after{ format("^2Successfully downloaded missing
//         CoD2 v1.3 game file ^5{}^2.\n", file_name) };
//         print_colored_text(app_handles.hwnd_re_messages_data,
//         message_after.c_str(), is_append_message_to_richedit_control::yes,
//         is_log_message::yes, is_log_datetime::yes); is_downloaded_patch_file
//         = true;
//       }
//     }
//   }
//
//   return is_downloaded_patch_file;
// }

void view_game_servers(HWND grid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    display_game_servers_data_in_servers_grid(grid);
}

void refresh_game_servers_data(HWND hgrid)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (is_refreshing_game_servers_data_event.load())
        return;
    is_refreshing_game_servers_data_event.store(true);

    char buffer[32];

    string number_of_online_players, number_of_max_players;
    for (size_t i{}; i < main_app.get_rcon_game_servers_count(); ++i)
    {
        auto &gs = main_app.get_game_servers()[i];
        if (parse_getinfo_response_for_specified_game_server(gs, number_of_online_players, number_of_max_players) ||
            parse_getstatus_response_for_specified_game_server(gs))
        {
            snprintf(buffer, std::size(buffer), "^2%d/^1%d", gs.get_number_of_players(),
                     gs.get_max_number_of_players());
            player p{};
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                             gs.get_server_ip_address(), p);
            gs.set_server_pid(format("^4{}", i + 1));
            gs.set_game_server_address(format("{}:{}", gs.get_server_ip_address(), gs.get_server_port()));
            gs.set_online_and_max_players(buffer);
            const auto &rcon_map_names_to_full_map_names = get_rcon_map_names_to_full_map_names_for_specified_game_name(
                convert_game_name_to_game_name_t(gs.get_game_name()));
            gs.set_current_full_map_name(rcon_map_names_to_full_map_names.contains(gs.get_current_map())
                                             ? rcon_map_names_to_full_map_names.at(gs.get_current_map())
                                             : gs.get_current_map());
            gs.set_country_code(p.country_code);
            display_game_server_data_in_servers_grid(hgrid, i);
        }
    }

    main_app.set_game_servers_count(main_app.get_rcon_game_servers_count());
    clear_servers_data_in_servers_grid(hgrid, main_app.get_rcon_game_servers_count(), max_servers_grid_rows, 8u);

    const string game_version_number{find_version_of_installed_cod2_game()};
    const int protocol = main_app.get_cod2_game_version_to_protocol().contains(game_version_number)
                             ? main_app.get_cod2_game_version_to_protocol().at(game_version_number)
                             : 115;

    const string getservers_command_to_send{format("getservers {} full empty", protocol)};
    string received_reply;
    main_app.get_connection_manager().send_and_receive_non_rcon_data(
        getservers_command_to_send.c_str(), received_reply, main_app.get_cod2_master_server_ip_address().c_str(),
        main_app.get_cod2_master_server_port(), main_app.get_current_game_server(), true, false);
    parse_and_display_downloaded_game_servers_data(received_reply, game_version_number.c_str(), true);
    auto &game_servers = main_app.get_game_servers();

    std::sort(begin(game_servers) + main_app.get_rcon_game_servers_count(),
              begin(game_servers) + main_app.get_game_servers_count(),
              [](const game_server &gs1, const game_server &gs2) {
                  return gs1.get_number_of_players() > gs2.get_number_of_players();
              });

    for (size_t sid{main_app.get_rcon_game_servers_count()}; sid < main_app.get_game_servers_count(); ++sid)
    {
        snprintf(buffer, std::size(buffer), "^2%d/^1%d", game_servers[sid].get_number_of_players(),
                 game_servers[sid].get_max_number_of_players());
        player p{};
        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                         game_servers[sid].get_server_ip_address(), p);
        game_servers[sid].set_server_pid(format("^4{}", sid + 1));
        game_servers[sid].set_game_server_address(
            format("{}:{}", game_servers[sid].get_server_ip_address(), game_servers[sid].get_server_port()));
        game_servers[sid].set_online_and_max_players(buffer);
        game_servers[sid].set_current_full_map_name(get_full_map_name(
            game_servers[sid].get_current_map(), convert_game_name_to_game_name_t(game_servers[sid].get_game_name())));
        game_servers[sid].set_country_code(p.country_code);
        ++sid;
    }

    view_game_servers(hgrid);

    is_refreshing_game_servers_data_event.store(false);
}

bool parse_and_display_downloaded_game_servers_data(std::string &game_servers_data, const char *version_number,
                                                    const bool is_display_parsed_game_servers_data)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    size_t start{game_servers_data.find('\\')};
    if (string::npos == start)
        return false;
    ++start;

    const size_t last{game_servers_data.rfind('\\')};
    if (string::npos == last)
        return false;

    auto &game_servers = main_app.get_game_servers();

    const char *get_servers_response_data{game_servers_data.c_str()};
    char buffer[32];

    set<string> seen_game_server_addresses;

    while (start < last)
    {
        const size_t next{game_servers_data.find('\\', start)};
        if (next != string::npos && next - start != 6)
        {
            start = next + 1;
            continue;
        }
        uint_least16_t port_number{static_cast<uint_least16_t>(get_servers_response_data[start + 4])};
        port_number *= 256;
        port_number += static_cast<uint_least16_t>(get_servers_response_data[start + 5]);
        snprintf(buffer, std::size(buffer), "%d.%d.%d.%d", (unsigned char)get_servers_response_data[start],
                 (unsigned char)get_servers_response_data[start + 1],
                 (unsigned char)get_servers_response_data[start + 2],
                 (unsigned char)get_servers_response_data[start + 3]);
        const string ip_address{buffer};
        bool is_parse_game_server_data{true};
        for (size_t i{}; i < main_app.get_rcon_game_servers_count(); ++i)
        {
            if (game_servers[i].get_server_ip_address() == ip_address &&
                game_servers[i].get_server_port() == port_number)
            {
                is_parse_game_server_data = false;
                break;
            }
        }

        const string game_server_address{format("{}:{}", ip_address, port_number)};

        if (is_parse_game_server_data && !seen_game_server_addresses.contains(game_server_address))
            seen_game_server_addresses.emplace(game_server_address);

        start += 7;
    }

    size_t sid{main_app.get_rcon_game_servers_count()},
        parsed_game_servers_count{main_app.get_rcon_game_servers_count()};

    const string started_parsing_message{
        format("^5Started processing ^3Call of duty 2 ^5game server related ^1getstatus "
               "^5responses.\n^5Number of retrieved ^3Call of duty 2 v{} ^5game server "
               "addresses: ^1{}\n",
               version_number, seen_game_server_addresses.size() + main_app.get_rcon_game_servers_count())};
    print_colored_text(app_handles.hwnd_re_messages_data, started_parsing_message.c_str());

    for (auto &game_server_address : seen_game_server_addresses)
    {
        const size_t colon_sep_pos{game_server_address.find(':')};
        if (colon_sep_pos == string::npos)
            continue;
        const string ip_address{game_server_address.substr(0, colon_sep_pos)};
        const uint_least16_t port_number =
            static_cast<uint_least16_t>(stoi(game_server_address.substr(colon_sep_pos + 1)));
        game_server current_game_server{};
        current_game_server.set_server_ip_address(ip_address);
        current_game_server.set_server_port(port_number);
        string number_of_online_players, number_of_max_players;
        if (parse_getinfo_response_for_specified_game_server(current_game_server, number_of_online_players,
                                                             number_of_max_players) ||
            parse_getstatus_response_for_specified_game_server(current_game_server))
        {

            snprintf(buffer, std::size(buffer), "^2%d/^1%d", current_game_server.get_number_of_players(),
                     current_game_server.get_max_number_of_players());
            player p{};
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                             current_game_server.get_server_ip_address(), p);
            current_game_server.set_server_pid(format("^4{}", sid + 1));
            current_game_server.set_game_server_address(
                format("{}:{}", current_game_server.get_server_ip_address(), current_game_server.get_server_port()));
            current_game_server.set_online_and_max_players(buffer);
            current_game_server.set_current_full_map_name(
                get_full_map_name(current_game_server.get_current_map(),
                                  convert_game_name_to_game_name_t(current_game_server.get_game_name())));
            current_game_server.set_country_code(p.country_code);
            const size_t server_index{main_app.get_game_servers_count()};
            game_servers[server_index] = std::move(current_game_server);
            main_app.set_game_servers_count(server_index + 1);
            ++parsed_game_servers_count;
            if (is_display_parsed_game_servers_data)
            {
                display_game_server_data_in_servers_grid(app_handles.hwnd_servers_grid, sid);
            }
            string status_message{format("Processed and displayed game server related data for {} out of {} {} "
                                         "game servers.",
                                         parsed_game_servers_count,
                                         seen_game_server_addresses.size() + main_app.get_rcon_game_servers_count(),
                                         version_number)};
            append_to_title(app_handles.hwnd_main_window, std::move(status_message));
            ++sid;
        }
    }
    const string finished_parsing_message{
        format("^5Finished parsing ^1getstatus ^5responses for ^3game servers' ^5related "
               "data.\n^5Number of online ^3Call of duty 2 v{} ^5game servers: ^2{}\n",
               version_number, parsed_game_servers_count)};
    print_colored_text(app_handles.hwnd_re_messages_data, finished_parsing_message.c_str());

    return true;
}

bool terminate_running_game_instance(const game_name_t game_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    DWORD pid{};

    switch (game_name)
    {

    case game_name_t::cod1:
        if (!check_if_call_of_duty_1_game_is_running(pid))
            return false;
        break;

    case game_name_t::cod2:
        if (!check_if_call_of_duty_2_game_is_running(pid))
            return false;
        break;

    case game_name_t::cod4:
        if (!check_if_call_of_duty_4_game_is_running(pid))
            return false;
        break;

    case game_name_t::cod5:
        if (!check_if_call_of_duty_5_game_is_running(pid))
            return false;
        break;

    default:
        return false;
    }

    if (pid != 0)
    {
        HANDLE process_handle{OpenProcess(PROCESS_ALL_ACCESS | PROCESS_TERMINATE, FALSE, pid)};
        TerminateProcess(process_handle, 0);
        CloseHandle(process_handle);
        return true;
    }

    return false;
}

game_name_t convert_game_name_to_game_name_t(const std::string &game_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    for (const auto &p : game_name_to_game_name_t)
    {
        if (str_contains(p.first, game_name, 0U, true))
            return p.second;
    }

    return game_name_t::cod2;
}

std::string wstring_to_string(const wchar_t *s, const char dfault, const std::locale &loc)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    std::ostringstream stm;

    while (*s != L'\0')
    {
        stm << std::use_facet<std::ctype<wchar_t>>(loc).narrow(*s++, dfault);
    }
    return stm.str();
}

std::string get_server_address_for_connect_command(const int selected_row_index)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    if (check_if_selected_cell_indices_are_valid_for_game_servers_grid(selected_row_index, 2))
    {
        string selected_server_address{GetCellContents(app_handles.hwnd_servers_grid, selected_row_index, 2)};
        if (selected_server_address.length() >= 2 && '^' == selected_server_address[0] &&
            is_decimal_digit(selected_server_address[1]))
            selected_server_address.erase(0, 2);
        auto parts = stl::helper::str_split(selected_server_address, ":", nullptr, split_on_whole_needle_t::yes,
                                            ignore_empty_string_t::yes);
        for (auto &&part : parts)
            stl::helper::trim_in_place(part);
        unsigned long guid{};
        int port_number{};
        if (parts.size() == 2 && check_ip_address_validity(parts[0], guid) &&
            is_valid_decimal_whole_number(parts[1], port_number))
            return selected_server_address;
        return {};
    }
    return {};
}

// std::shared_ptr<tiny_rcon_client_user>
// &get_user_for_specified_username_and_ip_address()
//{
//   return me->is_admin ? main_app.get_user_for_name(
//            main_app.get_username())
//                       : main_app.get_player_for_name(main_app.get_username(),
//                         main_app.get_user_ip_address());
// }

void load_reported_players_to_file(const char *file_path, std::vector<player> &reported_players)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    string property_key, property_value;
    ifstream input_file{file_path};
    if (!input_file)
    {
        const size_t buffer_size{1024};
        char buffer[buffer_size];
        strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
        string errorMessage{"^3Couldn't open file at specified path ^1("s + string{file_path} + ")^3! "s + buffer};
        print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        ofstream bannedIPsFileToWrite{file_path};
        if (!bannedIPsFileToWrite)
        {
            strerror_s(buffer, buffer_size, static_cast<int>(GetLastError()));
            errorMessage.assign(format("^3Couldn't create file at ^1{}^3!\nError: ^1{}", file_path, buffer));
            print_colored_text(app_handles.hwnd_re_messages_data, errorMessage.c_str());
        }
    }
    else
    {
        reported_players.clear();
        string readData, information_about_protected_player;
        while (getline(input_file, readData))
        {
            stl::helper::trim_in_place(readData);
            vector<string> parts = stl::helper::str_split(readData, "\\", nullptr, split_on_whole_needle_t::yes,
                                                          ignore_empty_string_t::no);
            for (auto &part : parts)
                stl::helper::trim_in_place(part);
            if (parts.size() < 5)
                continue;

            player bannedPlayerData{};
            size_t no_of_chars_to_copy{
                std::min<size_t>(std::size(bannedPlayerData.player_name) - 1, parts[0].length())};
            strncpy_s(bannedPlayerData.player_name, std::size(bannedPlayerData.player_name), parts[0].c_str(),
                      no_of_chars_to_copy);
            bannedPlayerData.ip_address = parts[1];
            bannedPlayerData.banned_start_time = get_number_of_seconds_from_date_and_time_string(parts[2]);
            // const string converted_ban_date_and_time_info{
            // get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}",
            // bannedPlayerData.banned_start_time) };
            no_of_chars_to_copy = std::min<size_t>(std::size(bannedPlayerData.banned_date_time) - 1, parts[2].length());
            strncpy_s(bannedPlayerData.banned_date_time, std::size(bannedPlayerData.banned_date_time), parts[2].c_str(),
                      no_of_chars_to_copy);
            bannedPlayerData.reason = std::move(parts[3]);
            bannedPlayerData.banned_by_user_name = parts.size() >= 5 ? parts[4] : "^3Player";
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                             bannedPlayerData.ip_address, bannedPlayerData);
            reported_players.push_back(std::move(bannedPlayerData));
        }
    }

    std::sort(begin(reported_players), end(reported_players),
              [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });
}

void save_reported_players_to_file(const char *file_path, const std::vector<player> &reported_players)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    ofstream output{file_path};
    if (output)
    {

        for (const auto &reported_player : reported_players)
        {
            output << reported_player.player_name << '\\' << reported_player.ip_address << '\\'
                   << get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", reported_player.banned_start_time) << '\\'
                   << remove_disallowed_character_in_string(reported_player.reason) << '\\'
                   << reported_player.banned_by_user_name << '\n';
        }

        output << flush;
    }
}

std::pair<bool, player> remove_reported_player(std::string &ip_address, std::string &message,
                                               const bool is_report_public_message)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    unsigned long ip_key{};
    auto &reported_players = main_app.get_reported_players();
    auto found_iter = std::cend(reported_players);
    size_t no{string::npos};
    if (check_ip_address_validity(ip_address, ip_key))
    {
        found_iter = find_if(std::cbegin(reported_players), std::cend(reported_players),
                             [&ip_address](const player &p) { return ip_address == p.ip_address; });
        no = found_iter != cend(reported_players) ? static_cast<size_t>(found_iter - std::cbegin(reported_players))
                                                  : string::npos;
    }
    else if (int number{}; is_valid_decimal_whole_number(ip_address, number))
    {
        no = static_cast<size_t>(number - 1);
    }

    if (no >= reported_players.size())
    {
        return {false, player{}};
    }

    if (no < reported_players.size())
    {

        found_iter = std::cbegin(reported_players) + static_cast<std::ptrdiff_t>(no);

        if (is_report_public_message)
        {

            const string buffer{format("^7Admin ({}^7) has successfully removed previously ^1reported player "
                                       "^7{}. ^7Reason: ^1{}",
                                       main_app.get_username(), found_iter->player_name,
                                       remove_disallowed_character_in_string(found_iter->reason))};
            message.assign(buffer);
            // rcon_say(message, true);
        }

        const player pd{*found_iter};
        reported_players.erase(found_iter);

        save_reported_players_to_file(main_app.get_reported_players_file_path(), reported_players);

        return {true, pd};
    }

    return {true, player{}};
}

bool copy_tinyrcon_files_from_source_to_destination_path(const std::string &dest_path,
                                                         const std::string &src_exe_file_name)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const vector<std::string> files_to_copy{"config\\tinyrcon.json",
                                            "data\\banned_cities.txt",
                                            "data\\banned_countries.txt",
                                            "data\\banned_names.txt",
                                            "data\\bans.txt",
                                            "data\\ip_range_bans.txt",
                                            "data\\protected_cities.txt",
                                            "data\\protected_countries.txt",
                                            "data\\protected_ip_address_ranges.txt",
                                            "data\\protected_ip_addresses.txt",
                                            "data\\reported_players.txt",
                                            "data\\tempbans.txt",
                                            "data\\user.txt",
                                            "log\\commands_history.log",
                                            "plugins\\geoIP\\geo.dat",
                                            "7za.dll",
                                            "TinyRcon.exe"};

    for (auto &&file_path : files_to_copy)
    {
        const string src_file_path{format("{}{}", main_app.get_current_working_directory(), file_path)};
        const string dest_file_path{format("{}{}", dest_path, file_path)};
        // if (check_if_file_path_exists(src_file_path.c_str()))
        if (CopyFile(src_file_path.c_str(), dest_file_path.c_str(), FALSE) == 0)
        {
            CopyFile(file_path.c_str(), dest_file_path.c_str(), FALSE);
        }
    }

    const string src_exe_file_path{format("{}{}", main_app.get_current_working_directory(), src_exe_file_name)};
    const string dest_exe_file_path{format("{}TinyRcon.exe", dest_path)};

    if (CopyFile(src_exe_file_path.c_str(), dest_exe_file_path.c_str(), FALSE) == 0)
    {
        CopyFile(src_exe_file_name.c_str(), dest_exe_file_path.c_str(), FALSE);
    }

    return true;
}

void check_version_number_and_file_path_information(version_data &dest_version)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    char exe_file_path[MAX_PATH]{};

    string cwd{main_app.get_current_working_directory()};
    replace_backward_slash_with_forward_slash(cwd);
    main_app.get_connection_manager_for_messages().process_and_send_message(
        "tinyrcon-info", format("{}\\{}\\{}", main_app.get_username(), main_app.get_user_ip_address(), cwd), true,
        main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);

    GetModuleFileName(nullptr, exe_file_path, MAX_PATH);

    const string src_exe_file_path{exe_file_path};
    version_data src_version{};
    unsigned long src_version_number{};
    main_app.get_auto_update_manager().get_file_version(src_exe_file_path, src_version, src_version_number);

    const string dest_path{"C:\\Games\\TinyRcon\\"};

    const string dest_exe_file_path{format("{}TinyRcon.exe", dest_path)};
    unsigned long dest_version_number{};
    main_app.get_auto_update_manager().get_file_version(dest_exe_file_path, dest_version, dest_version_number);

    string dest_path_lc{main_app.get_current_working_directory()};
    while (!dest_path_lc.empty() && '\\' == dest_path_lc.back())
    {
        dest_path_lc.pop_back();
    }
    to_lower_case_in_place(dest_path_lc);
    if (dest_path_lc == "c:\\games\\tinyrcon")
        return;

    const string dest_config_file_path{format("{}config\\tinyrcon.json", dest_path)};

    const vector<string> source_file_paths{
        "config",
        "data",
        "log",
        "temp",
        "tools",
        "plugins\\geoIP",
        "config\\tinyrcon.json",
        main_app.get_user_data_file_path(),
        main_app.get_temp_bans_file_path(),
        main_app.get_ip_bans_file_path(),
        main_app.get_ip_range_bans_file_path(),
        main_app.get_banned_countries_file_path(),
        main_app.get_banned_cities_file_path(),
        main_app.get_banned_names_file_path(),
        main_app.get_protected_ip_addresses_file_path(),
        main_app.get_protected_ip_address_ranges_file_path(),
        main_app.get_protected_cities_file_path(),
        main_app.get_protected_countries_file_path(),
        main_app.get_reported_players_file_path(),
        // "plugins\\geoIP\\geo.dat"
    };

    const vector<string> dest_file_paths{
        format("{}config", dest_path),
        format("{}data", dest_path),
        format("{}log", dest_path),
        format("{}temp", dest_path),
        format("{}tools", dest_path),
        format("{}plugins\\geoIP", dest_path),
        dest_config_file_path,
        format("{}{}", dest_path, main_app.get_user_data_file_path()),
        format("{}{}", dest_path, main_app.get_temp_bans_file_path()),
        format("{}{}", dest_path, main_app.get_ip_bans_file_path()),
        format("{}{}", dest_path, main_app.get_ip_range_bans_file_path()),
        format("{}{}", dest_path, main_app.get_banned_countries_file_path()),
        format("{}{}", dest_path, main_app.get_banned_cities_file_path()),
        format("{}{}", dest_path, main_app.get_banned_names_file_path()),
        format("{}{}", dest_path, main_app.get_protected_ip_addresses_file_path()),
        format("{}{}", dest_path, main_app.get_protected_ip_address_ranges_file_path()),
        format("{}{}", dest_path, main_app.get_protected_cities_file_path()),
        format("{}{}", dest_path, main_app.get_protected_countries_file_path()),
        format("{}{}", dest_path, main_app.get_reported_players_file_path()),
        // format("{}plugins\\geoIP\\geo.dat", dest_path)
    };
    for (size_t i{}; i < source_file_paths.size(); ++i)
    {
        if (!check_if_file_path_exists(source_file_paths[i].c_str()))
        {
            create_necessary_file_path(source_file_paths[i]);
        }

        if (!check_if_file_path_exists(dest_file_paths[i].c_str()))
        {
            if (source_file_paths[i].ends_with(".txt") || source_file_paths[i].ends_with(".json"))
            {
                if (CopyFile(source_file_paths[i].c_str(), dest_file_paths[i].c_str(), FALSE) == 0)
                {
                    create_necessary_file_path(dest_file_paths[i]);
                }
            }
            else
            {
                create_necessary_file_path(dest_file_paths[i]);
            }
        }
    }

    if (src_version_number == dest_version_number)
    {
        const string src_config_file_path{format("{}config\\tinyrcon.json", main_app.get_current_working_directory())};
        const size_t src_config_file_size{check_if_file_path_exists(src_config_file_path.c_str())
                                              ? get_file_size_in_bytes(src_config_file_path.c_str())
                                              : 0U};
        const size_t dest_config_file_size{check_if_file_path_exists(dest_config_file_path.c_str())
                                               ? get_file_size_in_bytes(dest_config_file_path.c_str())
                                               : 0U};
        if (src_config_file_size != 0U && src_config_file_size != dest_config_file_size)
        {
            if (check_if_file_path_exists(src_config_file_path.c_str()))
            {
                CopyFile(src_config_file_path.c_str(), dest_config_file_path.c_str(), FALSE);
            }
            else
            {
                CopyFile("config\\tinyrcon.json", dest_config_file_path.c_str(), FALSE);
            }
        }
    }
    else
    {
        const string different_version_information{
            format("^2Current version of ^5Tiny^6Rcon ^2before ^3copying files\n and "
                   "restarting ^2is ^5{}.{}.{}.{}\n",
                   src_version.major, src_version.minor, src_version.revision, src_version.sub_revision)};
        main_app.get_connection_manager_for_messages().process_and_send_message(
            "tinyrcon-info",
            format("{}\\{}\\{}", main_app.get_username(), main_app.get_user_ip_address(),
                   different_version_information),
            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        copy_tinyrcon_files_from_source_to_destination_path("C:\\Games\\TinyRcon\\",
                                                            get_file_name_from_path(exe_file_path));
    }

    if (src_exe_file_path != dest_exe_file_path)
    {
        const string different_version_information{format(
            "{}\\{}\\^3Restarting ^7{}^3's ^5Tiny^6Rcon ^3from new location: "
            "^1'{}'\n",
            main_app.get_username(), main_app.get_user_ip_address(), main_app.get_username(), dest_exe_file_path)};
        main_app.get_connection_manager_for_messages().process_and_send_message(
            "tinyrcon-info",
            format("{}\\{}\\{}", main_app.get_username(), main_app.get_user_ip_address(),
                   different_version_information),
            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        restart_tinyrcon_client(dest_exe_file_path.c_str(), src_exe_file_path,
                                format("{}TinyRcon.exe", main_app.get_current_working_directory()));
    }
}

std::string get_file_name_from_path(const std::string &file_path)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const auto slash_pos{file_path.rfind('\\')};
    return file_path.substr(slash_pos + 1);
}

void execute_at_exit()
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const string player_name{find_users_player_name_for_installed_cod2_game(
        me, !main_app.get_current_game_server().get_game_mod_name().empty()
                ? main_app.get_current_game_server().get_game_mod_name()
                : "main"s)};

    const auto current_ts{get_current_time_stamp()};
    if (me->is_admin)
    {
        main_app.get_connection_manager_for_messages().process_and_send_message(
            "request-logout",
            format("{}\\{}\\{}\\{}\\{}", me->user_name, me->ip_address, current_ts, player_name,
                   main_app.get_game_version_number()),
            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
    else
    {
        main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
            "request-logout-player",
            format("{}\\{}\\{}\\{}\\{}", me->user_name, me->ip_address, current_ts, player_name,
                   main_app.get_game_version_number()),
            true, main_app.get_tiny_rcon_server_ip_address_for_players(),
            main_app.get_tiny_rcon_server_port_for_players(), false);
    }
    me->is_logged_in = false;
    me->last_logout_time_stamp = current_ts;
    save_current_user_data_to_json_file(main_app.get_user_data_file_path());

    save_reported_players_to_file(main_app.get_reported_players_file_path(), main_app.get_reported_players());

    save_protected_entries_file(main_app.get_protected_ip_addresses_file_path(),
                                main_app.get_current_game_server().get_protected_ip_addresses());
    save_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(),
                                main_app.get_current_game_server().get_protected_ip_address_ranges());
    save_protected_entries_file(main_app.get_protected_cities_file_path(),
                                main_app.get_current_game_server().get_protected_cities());
    save_protected_entries_file(main_app.get_protected_countries_file_path(),
                                main_app.get_current_game_server().get_protected_countries());

    /*save_players_stats_data("data\\player_stats.dat",
    main_app.get_stats_data().get_scores_vector(),
    main_app.get_stats_data().get_scores_map());

    const string
    file_name_path_for_stats_data_for_year{format("data\\player_stats_for_year-{}.dat",
    main_app.get_stats_data().get_stop_time_in_seconds_for_year())};
    save_players_stats_data(file_name_path_for_stats_data_for_year.c_str(),
    main_app.get_stats_data().get_scores_for_year_vector(),
    main_app.get_stats_data().get_scores_for_year_map());

    const string
    file_name_path_for_stats_data_for_month{format("data\\player_stats_for_month-{}.dat",
    main_app.get_stats_data().get_stop_time_in_seconds_for_month())};
    save_players_stats_data(file_name_path_for_stats_data_for_month.c_str(),
    main_app.get_stats_data().get_scores_for_month_vector(),
    main_app.get_stats_data().get_scores_for_month_map());

    const string
    file_name_path_for_stats_data_for_day{format("data\\player_stats_for_day-{}.dat",
    main_app.get_stats_data().get_stop_time_in_seconds_for_day())};
    save_players_stats_data(file_name_path_for_stats_data_for_day.c_str(),
    main_app.get_stats_data().get_scores_for_day_vector(),
    main_app.get_stats_data().get_scores_for_day_map());*/
}

bool check_if_exists_and_download_missing_custom_map_files_downloader(
    /*const char* downloader_program_file_path*/)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static constexpr size_t buffer_length{1024u};
    static wchar_t user_profile_folder[buffer_length]{};
    static wchar_t command_line[buffer_length]{};

    if (!SHGetSpecialFolderPathW(NULL, user_profile_folder, CSIDL_PROFILE, FALSE))
    {
        wchar_t *dir_path{};
        size_t buffer_count{};
        const auto error_number = _wdupenv_s(&dir_path, &buffer_count, L"USERPROFILE");
        if (error_number)
            return false;
        // char *user_folder_path = getenv("USERPROFILE");
        if (!dir_path || len(dir_path) == 0)
            return false;
        wcscpy_s(user_profile_folder, std::size(user_profile_folder), dir_path);
        free(dir_path);
    }

    wstring desktop_dir_path{user_profile_folder};
    wstring tinyrcon_dir_path{str_to_wstr(main_app.get_current_working_directory())};

    replace_forward_slash_with_backward_slash(desktop_dir_path);
    if (!desktop_dir_path.empty() && (L'\\' == desktop_dir_path.back() || L'/' == desktop_dir_path.back()))
        desktop_dir_path.pop_back();
    desktop_dir_path.push_back(L'\\');
    desktop_dir_path.append(L"Desktop\\CTF_custom_maps_auto_downloader.lnk");

    const wstring downloader_program_file_path{format(L"{}CTF_custom_maps_auto_downloader.exe", tinyrcon_dir_path)};

    if (!check_if_file_path_exists(downloader_program_file_path.c_str()))
    {

        if (!main_app.get_auto_update_manager().download_file(
                L"ftp://85.222.189.119/tinyrcon/CTF_custom_maps_auto_downloader.exe",
                downloader_program_file_path.c_str()))
        {
            const string warning_message{
                "^3Could not download CTF game server's custom maps auto downloader from\n ^5ftp://85.222.189.119/tinyrcon/CTF_custom_maps_auto_downloader.exe\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, warning_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            return false;
        }

        (void)swprintf_s(command_line, std::size(command_line), L"\"%s\" /NP", downloader_program_file_path.c_str());
        STARTUPINFOW process_startup_info{};
        process_startup_info.cb = sizeof(STARTUPINFOW);
        process_startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK;
        PROCESS_INFORMATION pr_info{};

        if (!CreateProcessW(nullptr, command_line,
                            nullptr,                                    // Process handle not inheritable
                            nullptr,                                    // Thread handle not inheritable
                            FALSE,                                      // Set handle inheritance to FALSE
                            NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE, // creation flags
                            nullptr,                                    // Use parent's environment block
                            tinyrcon_dir_path.c_str(),                  // Use parent's starting directory
                            &process_startup_info,                      // Pointer to STARTUPINFO structure
                            &pr_info))
        {
            char buffer[512]{};
            strerror_s(buffer, 512, GetLastError());
            const string error_message{"^3Launching CTF game server's custom maps auto downloader failed: ^1"s +
                                       buffer + "\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            return false;
        }

        if (process_startup_info.hStdError)
            CloseHandle(process_startup_info.hStdError);
        if (process_startup_info.hStdInput)
            CloseHandle(process_startup_info.hStdInput);
        if (process_startup_info.hStdOutput)
            CloseHandle(process_startup_info.hStdOutput);
        CloseHandle(pr_info.hThread);
        CloseHandle(pr_info.hProcess);
    }

    if (!check_if_file_path_exists(desktop_dir_path.c_str()))
    {
        CreateLink(downloader_program_file_path.c_str(), desktop_dir_path.c_str(),
                   L"CTF game server's custom map auto-downloader");
        const wchar_t value[]{L"~ RUNASADMIN"};
        RegSetKeyValueW(HKEY_CURRENT_USER, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers)",
                        downloader_program_file_path.c_str(), REG_SZ, L"~ RUNASADMIN", sizeof(value));
    }

    desktop_dir_path.assign(user_profile_folder);

    replace_forward_slash_with_backward_slash(desktop_dir_path);
    if (!desktop_dir_path.empty() && (L'\\' == desktop_dir_path.back() || L'/' == desktop_dir_path.back()))
        desktop_dir_path.pop_back();
    desktop_dir_path.push_back(L'\\');
    desktop_dir_path.append(L"Desktop\\TinyRcon.lnk");

    if (!check_if_file_path_exists(desktop_dir_path.c_str()))
    {
        wchar_t tiny_rcon_exe_path[512]{};
        GetModuleFileNameW(nullptr, tiny_rcon_exe_path, std::size(tiny_rcon_exe_path));
        // const wstring tiny_rcon_exe_path{ format(L"{}TinyRcon.exe",
        // main_app.get_current_working_directory()) };
        const wstring version_information{format(L"TinyRcon v{}", str_to_wstr(program_version))};
        CreateLink(tiny_rcon_exe_path, desktop_dir_path.c_str(), version_information.c_str());
        const wchar_t value[]{L"~ RUNASADMIN"};
        RegSetKeyValueW(HKEY_CURRENT_USER, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers)",
                        tiny_rcon_exe_path, REG_SZ, L"~ RUNASADMIN", sizeof(value));
    }

    return true;
}

// CreateLink - Uses the Shell's IShellLink and IPersistFile interfaces
//              to create and store a shortcut to the specified object.
//
// Returns the result of calling the member functions of the interfaces.
//
// Parameters:
// lpszPathObj  - Address of a buffer that contains the path of the object,
//                including the file name.
// lpszPathLink - Address of a buffer that contains the path where the
//                Shell link is to be stored, including the file name.
// lpszDesc     - Address of a buffer that contains a description of the
//                Shell link, stored in the Comment field of the link
//                properties.

HRESULT
CreateLink(const wchar_t *lpszPathObj, const wchar_t *lpszPathLink, const wchar_t *lpszDesc)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    HRESULT hres;
    IShellLinkW *psl;

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID *)&psl);
    if (SUCCEEDED(hres))
    {
        IPersistFile *ppf{};

        // Set the path to the shortcut target and add the description.
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);

        // Query IShellLink for the IPersistFile interface, used for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);

        if (SUCCEEDED(hres))
        {
            // WCHAR wsz[MAX_PATH];
            // Ensure that the string is Unicode.
            // MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);

            // Add code here to check return value from MultiByteWideChar
            // for success.

            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(lpszPathLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

const std::string &get_current_map_image_name(const std::string &current_rcon_map_map)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    const static string unknown_rcon_map_name_image{"IDR_NOMAP"};
    const string rcon_map_name_index{stl::helper::to_lower_case(stl::helper::trim(current_rcon_map_map))};

    static const std::unordered_map<std::string, std::string> rcon_map_name_to_image_name{
        {"mp_burgundy", "IDR_MP_BURGUNDY"},
        {"mp_carentan", "IDR_MP_CARENTAN"},
        {"mp_dawnville", "IDR_MP_DAWNVILLE"},
        {"mp_breakout", "IDR_MP_BREAKOUT"},
        {"mp_brecourt", "IDR_MP_BRECOURT"},
        {"mp_burgundy", "IDR_MP_BURGUNDY"},
        {"mp_carentan", "IDR_MP_CARENTAN"},
        {"mp_dawnville", "IDR_MP_DAWNVILLE"},
        {"mp_decoy", "IDR_MP_DECOY"},
        {"mp_downtown", "IDR_MP_DOWNTOWN"},
        {"mp_farmhouse", "IDR_MP_FARMHOUSE"},
        {"mp_leningrad", "IDR_MP_LENINGRAD"},
        {"mp_matmata", "IDR_MP_MATMATA"},
        {"mp_railyard", "IDR_MP_RAILYARD"},
        {"mp_toujane", "IDR_MP_TOUJANE"},
        {"mp_trainstation", "IDR_MP_TRAINSTATION"},
        {"mp_depot", "IDR_MP_DEPOT"},
        {"german_town", "IDR_GERMAN_TOWN"},
        {"mp_harbor", "IDR_MP_HARBOR"},
        {"mp_naout", "IDR_MP_NAOUT"},
        {"mp_rhine", "IDR_MP_RHINE"},
        {"mp_rouen", "IDR_MP_ROUEN"},
        {"mp_saint_lo", "IDR_MP_SAINT_LO"},
        {"mp_shipment", "IDR_MP_SHIPMENT"},
        {"mp_tigertown", "IDR_MP_TIGERTOWN"},
        {"mp_tobruk", "IDR_MP_TOBRUK"},
        {"mp_tripoli", "IDR_MP_TRIPOLI"},
        {"mp_farao", "IDR_MP_FARAO"},
        {"canal2", "IDR_CANAL2"},
        {"mp_bridge", "IDR_MP_BRIDGE"}};

    if (rcon_map_name_to_image_name.contains(rcon_map_name_index))
        return rcon_map_name_to_image_name.at(rcon_map_name_index);
    return unknown_rcon_map_name_image;
}

void load_current_map_image(const std::string &rcon_map_name)
{
    static string current_rcon_map_name{"n/a"};

    if (rcon_map_name != current_rcon_map_name)
    {
        current_rcon_map_name = rcon_map_name;
        // const string& current_map_image_name =
        // get_current_map_image_name(current_rcon_map_name); if (g_hBitMap != NULL)
        // DeleteBitmap(g_hBitMap);
        g_hBitMap = main_app.get_bitmap_image_handler().get_bitmap_image(rcon_map_name);
        /*if (NULL == g_hBitMap) {
                g_hBitMap = LoadBitmapA(GetModuleHandle(NULL),
        current_map_image_name.c_str());
        }*/
        InvalidateRect(app_handles.hwnd_main_window, nullptr, FALSE);
    }
}

std::wstring str_to_wstr(const std::string &src)
{
    return std::wstring(std::cbegin(src), std::cend(src));
}

std::string wstr_to_str(const std::wstring &src)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converterX;
    return converterX.to_bytes(src);
}

vector<string> get_file_name_matches_for_specified_file_path_pattern(const char *dir_path, const char *file_pattern)
{
    char download_file_path_pattern[MAX_PATH];
    vector<string> found_file_names;

    snprintf(download_file_path_pattern, std::size(download_file_path_pattern), "%s\\%s", dir_path, file_pattern);
    WIN32_FIND_DATA file_data{};
    HANDLE read_file_data_handle{FindFirstFile(download_file_path_pattern, &file_data)};

    if (read_file_data_handle != INVALID_HANDLE_VALUE)
    {
        while (stl::helper::len(file_data.cFileName) > 0)
        {
            found_file_names.emplace_back(file_data.cFileName);
            ZeroMemory(&file_data, sizeof(WIN32_FIND_DATA));
            FindNextFile(read_file_data_handle, &file_data);
        }

        FindClose(read_file_data_handle);
    }

    return found_file_names;
}

std::string calculate_md5_checksum_of_file(const char *file_path)
{

    if (!check_if_file_path_exists(file_path))
        return {};

    const size_t length(get_file_size_in_bytes(file_path));
    if (length == 0)
        return {};

    ifstream input_file(file_path, std::ios::binary | std::ios::in);
    if (!input_file)
        return {};

    unique_ptr<char[]> file_data{make_unique<char[]>(length)};
    input_file.read(file_data.get(), length);

    return md5(file_data.get(), length);
}

// void print_trace_message(const char* file_name, const size_t line_number,
// const char* function_name) {
//	ostringstream oss;
//	stl::helper::tracer{ oss, file_name, line_number }("%s\n",
// function_name); 	print_colored_text(app_handles.hwnd_re_messages_data,
// oss.str().c_str());
// }

bool fix_path_strings_in_json_config_file(const std::string &config_file_path)
{

    ifstream input_file{config_file_path};
    if (!input_file)
        return false;

    const size_t append_pos{config_file_path.rfind(".json")};
    if (append_pos == string::npos)
        return false;

    const string corrected_config_file_path{string(config_file_path.cbegin(), config_file_path.cbegin() + append_pos) +
                                            "_fix.json"s};
    ofstream output_file{corrected_config_file_path};

    if (!output_file)
        return false;

    for (string line; getline(input_file, line);)
    {
        trim_in_place(line);
        output_file << escape_backward_slash_characters_in_place(line) << '\n';
    }

    input_file.close();
    output_file.flush();
    output_file.close();
    DeleteFileA(config_file_path.c_str());
    MoveFileExA(corrected_config_file_path.c_str(), config_file_path.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    return true;
}

// void process_topplayers_request(const std::string &d)
//{
//	std::thread process_topplayers_request_task{[data = d]()
//												{
//													//
// IsGUIThread(TRUE); 													auto parts = stl::helper::str_split(data, "\\",
// nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
//													//
// for (auto &part : parts) stl::helper::trim_in_place(part); 													if
// (parts.size() >= 2)
//													{
//														int
// n{25}; 														if (parts[0] == "!s")
//														{
//															const
// string title{format("^5Stats data of ^1player(s) ^5matching specified player
// name ^1({}):", parts[1])}; 															string public_message;
// const string players_stats_information{get_top_players_stats_data(main_app.get_stats_data().get_scores_vector(),
// main_app.get_stats_data().get_scores_map(), n, public_message, title.c_str(),
// parts[1])}; print_colored_text(app_handles.hwnd_re_messages_data, players_stats_information.c_str(),
// is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true, true);
//														}
//														else
// if (parts[0] == "tp" || parts[0] == "!tp")
//														{
//															if
//(!is_valid_decimal_whole_number(parts[1], n)) 																n = 25;
// const string title{format("Top {} {}", n, n != 1 ? "players" : "player")};
// string public_message; 															const string
// players_stats_information{get_top_players_stats_data(main_app.get_stats_data().get_scores_vector(),
// main_app.get_stats_data().get_scores_map(), n, public_message,
// title.c_str())}; print_colored_text(app_handles.hwnd_re_messages_data, players_stats_information.c_str(),
// is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true, true);
//														}
//														else
// if (parts[0] == "tpy" || parts[0] == "!tpy")
//														{
//															if
//(!is_valid_decimal_whole_number(parts[1], n)) 																n = 25;
// tm time_info{}; 															auto t_c = get_current_time_stamp();
// localtime_s(&time_info, &t_c); 															const string
// title{format("Top {} {} for year {}:", n, n != 1 ? "players" : "player",
// time_info.tm_year + 1900)}; 															string public_message;
// const string
// players_stats_information{get_top_players_stats_data(main_app.get_stats_data().get_scores_for_year_vector(),
// main_app.get_stats_data().get_scores_for_year_map(), n, public_message,
// title.c_str())}; print_colored_text(app_handles.hwnd_re_messages_data, players_stats_information.c_str(),
// is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true, true);
//														}
//														else
// if (parts[0] == "tpm" || parts[0] == "!tpm")
//														{
//															if
//(!is_valid_decimal_whole_number(parts[1], n)) 																n = 25;
// tm time_info{}; 															auto t_c = get_current_time_stamp();
// localtime_s(&time_info, &t_c); 															const string
// title{format("Top {} {} for month {}:", n, n != 1 ? "players" : "player",
// get_current_short_month_name(time_info.tm_mon + 1), time_info.tm_year +
// 1900)}; 															string public_message;
// const string
// players_stats_information{get_top_players_stats_data(main_app.get_stats_data().get_scores_for_month_vector(),
// main_app.get_stats_data().get_scores_for_month_map(), n, public_message,
// title.c_str())}; print_colored_text(app_handles.hwnd_re_messages_data, players_stats_information.c_str(),
// is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true, true);
//														}
//														else
// if (parts[0] == "tpd" || parts[0] == "!tpd")
//														{
//															if
//(!is_valid_decimal_whole_number(parts[1], n)) 																n = 25;
// tm time_info{}; 															auto t_c = get_current_time_stamp();
// localtime_s(&time_info, &t_c); 															const string
// title{format("Top {} {} for {}.{}.{}:", n, n != 1 ? "players" : "player",
// time_info.tm_mday, get_current_short_month_name(time_info.tm_mon + 1),
// time_info.tm_year + 1900)}; 															string public_message;
// const string
// players_stats_information{get_top_players_stats_data(main_app.get_stats_data().get_scores_for_day_vector(),
// main_app.get_stats_data().get_scores_for_day_map(), n, public_message,
// title.c_str())}; print_colored_text(app_handles.hwnd_re_messages_data, players_stats_information.c_str(),
// is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true, true);
//														}
//													}
//												}};
//	process_topplayers_request_task.detach();
// }
//
//[[maybe_unused]] bool tell_player_their_stats_data_in_a_private_message(const
// char *, const player &pd)
//{
//	auto &stats_data_vector = main_app.get_stats_data().get_scores_vector();
//	auto &stats_data_map = main_app.get_stats_data().get_scores_map();
//	if (stats_data_map.contains(pd.player_name_index))
//	{
//		const auto &user = stats_data_map[pd.player_name_index];
//		for (size_t i{}; i < stats_data_vector.size(); ++i)
//		{
//			if (pd.player_name_index ==
// stats_data_vector[i].index_name)
//			{
//				char buffer[32]{};
//				ostringstream rcon;
//				rcon << "Your stats data: ^5Rank: ";
//				snprintf(buffer, std::size(buffer), "%lu", i +
// 1); 				rcon << "^1" << buffer << " ^5Score: "; 				snprintf(buffer,
// std::size(buffer), "%lld", user.score); 				rcon << "^1" << buffer << " ^5SPM: ";
//				const auto time_interval_in_minutes =
//(user.time_spent_on_server_in_seconds + 5) / 60; 				snprintf(buffer,
// std::size(buffer), "%.2lf spm", time_interval_in_minutes != 0 ?
// static_cast<double>(user.score) / time_interval_in_minutes : 0.0); 				rcon <<
//"^1" << buffer << " ^5Time played: "; 				const string
// time_period_information{get_time_interval_info_string_for_seconds_in_hours_and_minutes(user.time_spent_on_server_in_seconds
//+ 5)}; 				rcon << "^1" << time_period_information; 				const string
// private_msg{rcon.str()}; 				tell_message(private_msg.c_str(), pd.pid); 				return
// true;
//			}
//		}
//	}
//
//	return false;
// }
//
// bool remove_stats_for_player_name(const std::string &player_name_index)
//{
//
//	auto &sd = main_app.get_stats_data();
//	bool is_found_player_name{};
//
//	{
//		lock_guard lg{main_app.get_stats_data().get_stats_data_mutex()};
//		if (sd.get_scores_map().contains(player_name_index))
//		{
//			sd.get_scores_map().erase(player_name_index);
//			is_found_player_name = true;
//		}
//
//		if (sd.get_scores_for_year_map().contains(player_name_index))
//		{
//			sd.get_scores_for_year_map().erase(player_name_index);
//			is_found_player_name = true;
//		}
//
//		if (sd.get_scores_for_month_map().contains(player_name_index))
//		{
//			sd.get_scores_for_month_map().erase(player_name_index);
//			is_found_player_name = true;
//		}
//
//		if (sd.get_scores_for_day_map().contains(player_name_index))
//		{
//			sd.get_scores_for_day_map().erase(player_name_index);
//			is_found_player_name = true;
//		}
//	}
//
//	if (is_found_player_name)
//	{
//		sort_players_stats_data(sd.get_scores_vector(),
// sd.get_scores_map()); 		sort_players_stats_data(sd.get_scores_for_year_vector(),
// sd.get_scores_for_year_map());
//		sort_players_stats_data(sd.get_scores_for_month_vector(),
// sd.get_scores_for_month_map());
//		sort_players_stats_data(sd.get_scores_for_day_vector(),
// sd.get_scores_for_day_map());
//	}
//
//	return is_found_player_name;
// }
//
// void rcon_say_top_players(std::string &&title)
//{
//	string public_message;
//	get_top_players_stats_data(main_app.get_stats_data().get_scores_vector(),
// main_app.get_stats_data().get_scores_map(),
// main_app.get_number_of_top_players_to_display_in_game_chat(), public_message,
//"Top players:");
//	// print_colored_text(app_handles.hwnd_re_messages_data,
// top_players_stats.c_str(), is_append_message_to_richedit_control::yes,
// is_log_message::no, is_log_datetime::yes, true, true, true); 	if
//(main_app.get_current_game_server().get_number_of_players() >= 2u)
//	{
//		const string
// online_players_stats{get_online_players_stats_data_report(main_app.get_stats_data().get_scores_vector(),
// main_app.get_stats_data().get_scores_map(), "Online players' stats data:")};
//		print_colored_text(app_handles.hwnd_re_messages_data,
// online_players_stats.c_str(), is_append_message_to_richedit_control::yes,
// is_log_message::no, is_log_datetime::yes, true, true);
//	}
//
//	rcon_say(title, false);
//	Sleep(3000);
//	auto lines = stl::helper::str_split(public_message, '\n', nullptr,
// split_on_whole_needle_t::yes, ignore_empty_string_t::yes); 	for (size_t i{}; i
//< std::min<size_t>(10u, lines.size()); ++i)
//	{
//		stl::helper::trim_in_place(lines[i]);
//		if (lines[i].empty())
//			continue;
//		rcon_say(lines[i], false);
//		Sleep(3000);
//	}
//
//	const auto &stats_data_map = main_app.get_stats_data().get_scores_map();
//	const auto &players_data =
// main_app.get_current_game_server().get_players_data(); 	for (size_t i{}; i <
// main_app.get_current_game_server().get_number_of_players(); ++i)
//	{
//		const auto &pd = players_data[i];
//		if (stats_data_map.contains(pd.player_name_index))
//		{
//			tell_player_their_stats_data_in_a_private_message("^5Your
// stats data: ", pd); 			Sleep(3000);
//		}
//	}
// }
//
// std::string get_top_players_stats_data(
//	std::vector<player_stats> &stats_data, std::unordered_map<std::string,
// player_stats> &stats_data_map, 	const size_t number_of_top_players, std::string
//&public_message, const char *title, 	std::string partial_or_full_player_name,
// const bool find_exact_player_name_match)
//{
//	sort_players_stats_data(stats_data, stats_data_map);
//
//	size_t number_of_entries_to_display{
//		number_of_top_players <= 1000U && number_of_top_players <
// stats_data.size() 			? number_of_top_players 			: std::min<size_t>(1000U,
// stats_data.size())};
//
//	if (!partial_or_full_player_name.empty())
//	{
//		number_of_entries_to_display = 25u;
//		partial_or_full_player_name =
// get_cleaned_user_name(partial_or_full_player_name); 		vector<player_stats>
// found_player_stats;
//
//		unordered_set<string> seen_index_names;
//		unordered_set<string> seen_player_names;
//		size_t j{};
//		for (size_t i{}; i < stats_data.size() && j <
// number_of_entries_to_display; ++i)
//		{
//			if (seen_index_names.contains(stats_data[i].index_name)
//|| seen_player_names.contains(stats_data[i].player_name)) 				continue;
//			seen_index_names.emplace(stats_data[i].index_name);
//			seen_player_names.emplace(stats_data[i].player_name);
//			if ((!find_exact_player_name_match &&
// stl::helper::str_contains(stats_data[i].index_name,
// partial_or_full_player_name.c_str(), 0U)) || partial_or_full_player_name ==
// stats_data[i].index_name)
//			{
//				found_player_stats.emplace_back(stats_data[i]);
//				found_player_stats.back().first_seen = i + 1;
//				++j;
//				if (find_exact_player_name_match)
//					break;
//			}
//		}
//
//		size_t longest_name_length{12};
//		if (!found_player_stats.empty())
//		{
//			longest_name_length = std::max(longest_name_length,
// find_longest_player_name_length(found_player_stats.cbegin(),
// found_player_stats.cend(), false));
//		}
//
//		ostringstream oss;
//		const string decoration_line(68 + longest_name_length, '=');
//		oss << "^5\n"
//			<< decoration_line << "\n";
//		std::string title_str{title};
//		stl::helper::trim_in_place(title_str);
//		remove_all_color_codes(title_str);
//		const size_t
// printed_name_char_count{get_number_of_characters_without_color_codes(title)};
//		const size_t padding_size{(decoration_line.length() - 4u -
// printed_name_char_count)}; 		const string pad_str(padding_size, ' '); 		if
//(printed_name_char_count + 4u <= decoration_line.length())
//		{
//			oss << "^5| " << left << title << right << pad_str << "
//^5|\n";
//		}
//		else
//		{
//			oss << "^5| " << left << title << " ^5|\n";
//		}
//		oss << decoration_line << "\n";
//		oss << "^5| ";
//		oss << left << setw(6) << "Rank "
//			<< " | " << left << setw(longest_name_length) << "Player
// name"
//			<< " | " << left << setw(10) << "Score"
//			<< " | " << left << setw(17) << "Score/minute"
//			<< " | " << left << setw(20) << "Time played"
//			<< "^5|\n";
//		oss << decoration_line << "\n";
//		bool is_first_color{true};
//		char buffer[32]{};
//		for (size_t i{}; i < found_player_stats.size(); ++i)
//		{
//			const auto &user = found_player_stats[i];
//			const char *next_color{is_first_color ? "^3" : "^5"};
//			snprintf(buffer, std::size(buffer), "%lld.",
// user.first_seen); 			oss << "^5| " << text_element{buffer, 6, next_color}; 			oss <<
//" ^5| " << text_element{user.player_name, longest_name_length, "^7"} << " ^5|
//"; 			snprintf(buffer, std::size(buffer), "%lld", user.score); 			oss <<
// text_element{buffer, 10, next_color} << " ^5| "; 			const auto
// time_interval_in_minutes = (user.time_spent_on_server_in_seconds + 5) / 60;
//			snprintf(buffer, std::size(buffer), "%.2lf spm",
// time_interval_in_minutes != 0 ? static_cast<double>(user.score) /
// time_interval_in_minutes : 0.0); 			oss << text_element{buffer, 17, next_color}
//<< " ^5| "; 			const string
// time_period_information{get_time_interval_info_string_for_seconds_in_hours_and_minutes(user.time_spent_on_server_in_seconds
//+ 5)}; 			oss << text_element{time_period_information.c_str(), 20, next_color} <<
//"^5|\n"; 			is_first_color = !is_first_color;
//		}
//
//		if (0u == j)
//		{
//			const size_t message_len = stl::helper::len("| No
// matches found!"); 			oss << "^5| ^3No matches found!";
//
//			if (message_len + 2 < decoration_line.length())
//			{
//				oss << string(decoration_line.length() - 2 -
// message_len, ' ');
//			}
//			oss << " ^5|\n";
//		}
//
//		oss << string{"^5"s + decoration_line + "\n\n"s};
//		return oss.str();
//	}
//
//	size_t longest_name_length{12};
//	if (!stats_data.empty())
//	{
//		longest_name_length = std::max(longest_name_length,
// find_longest_player_name_length(stats_data.cbegin(), stats_data.cbegin() +
// number_of_entries_to_display, false));
//	}
//
//	ostringstream oss;
//	ostringstream rcon;
//	const string decoration_line(68 + longest_name_length, '=');
//	oss << "^5\n"
//		<< decoration_line << "\n";
//	std::string title_str{title};
//	stl::helper::trim_in_place(title_str);
//	remove_all_color_codes(title_str);
//	const size_t
// printed_name_char_count{get_number_of_characters_without_color_codes(title)};
//	const size_t padding_size{(decoration_line.length() - 4u -
// printed_name_char_count)}; 	const string pad_str(padding_size, ' '); 	if
//(printed_name_char_count + 4u < decoration_line.length())
//	{
//		oss << "^5| " << left << title << right << pad_str << " ^5|\n";
//	}
//	else
//	{
//		oss << "^5| " << left << title << " ^5|\n";
//	}
//	oss << decoration_line << "\n";
//	oss << "^5| ";
//	oss << left << setw(6) << "Rank "
//		<< " | " << left << setw(longest_name_length) << "Player name"
//		<< " | " << left << setw(10) << "Score"
//		<< " | " << left << setw(17) << "Score/minute"
//		<< " | " << left << setw(20) << "Time played"
//		<< "^5|\n";
//	oss << decoration_line << "\n";
//	if (stats_data.empty())
//	{
//		const size_t message_len = stl::helper::len("| Players' stats
// data hasn't been tracked yet."); 		oss << "^5| ^3Players' stats data hasn't been
// tracked yet.";
//
//		if (message_len + 2 < decoration_line.length())
//		{
//			oss << string(decoration_line.length() - 2 -
// message_len, ' ');
//		}
//		oss << " ^5|\n";
//	}
//	else
//	{
//		bool is_first_color{true};
//		char buffer[32]{};
//		char buffer2[32]{};
//		unordered_set<string> seen_index_names;
//		unordered_set<string> seen_player_names;
//		size_t j{};
//		for (size_t i{}; i < stats_data.size() && j <
// number_of_entries_to_display; ++i)
//		{
//			if (seen_index_names.contains(stats_data[i].index_name)
//|| seen_player_names.contains(stats_data[i].player_name)) 				continue;
//			seen_index_names.emplace(stats_data[i].index_name);
//			seen_player_names.emplace(stats_data[i].player_name);
//			const auto &user = stats_data[i];
//			++j;
//			const char *next_color{is_first_color ? "^3" : "^5"};
//			snprintf(buffer, std::size(buffer), "%lu.", i + 1);
//			oss << "^5| " << text_element{buffer, 6, next_color};
//			// rcon << text_element{ buffer, 6, "^1" } << ' ';
//			rcon << "^1" << buffer << ' ';
//			oss << " ^5| " << text_element{user.player_name,
// longest_name_length, "^7"} << " ^5| "; 			rcon << "^7" << user.player_name << '
//'; 			snprintf(buffer, std::size(buffer), "%lld", user.score); 			oss <<
// text_element{buffer, 10, next_color} << " ^5| "; 			rcon << "^7Score: ^4" <<
// buffer << ' '; 			const auto time_interval_in_minutes =
//(user.time_spent_on_server_in_seconds + 5) / 60; 			snprintf(buffer,
// std::size(buffer), "%.2lf spm", time_interval_in_minutes != 0 ?
// static_cast<double>(user.score) / time_interval_in_minutes : 0.0);
//			snprintf(buffer2, std::size(buffer2), "%.2lf",
// time_interval_in_minutes != 0 ? static_cast<double>(user.score) /
// time_interval_in_minutes : 0.0); 			oss << text_element{buffer, 17, next_color}
//<< " ^5| "; 			rcon << "^7Score/min: ^4" << buffer2 << ' '; 			const string
// time_period_information{get_time_interval_info_string_for_seconds_in_hours_and_minutes(user.time_spent_on_server_in_seconds
//+ 5)}; 			oss << text_element{time_period_information.c_str(), 20, next_color} <<
//"^5|\n"; 			rcon << "^7Time: ^2" << time_period_information << '\n'; 			if
//(find_exact_player_name_match) 				break; 			is_first_color = !is_first_color;
//		}
//
//		if (0u == j)
//		{
//			const size_t message_len = stl::helper::len("| No
// matches found!"); 			oss << "^5| ^3No matches found!";
//
//			if (message_len + 2 < decoration_line.length())
//			{
//				oss << string(decoration_line.length() - 2 -
// message_len, ' ');
//			}
//			oss << " ^5|\n";
//		}
//	}
//	oss << string{"^5"s + decoration_line + "\n\n"s};
//	public_message = rcon.str();
//	return oss.str();
// }
//
// void sort_players_stats_data(std::vector<player_stats> &stats_data_vec,
// std::unordered_map<std::string, player_stats> &stats_data_map)
//{
//	std::lock_guard lg{main_app.get_stats_data().get_stats_data_mutex()};
//	stats_data_vec.clear();
//	stats_data_vec.reserve(stats_data_map.size());
//
//	for (const auto &[name_index, stats_data] : stats_data_map)
//	{
//		stats_data_vec.emplace_back(stats_data);
//	}
//
//	std::sort(std::execution::par, std::begin(stats_data_vec),
// std::end(stats_data_vec), [](const player_stats &ps1, const player_stats &ps2)
//			  { return ps1.score > ps2.score; });
//
//	for (size_t i{}; i < stats_data_vec.size(); ++i)
//	{
//		const auto current_max_score{stats_data_vec[i].score};
//		size_t j{i};
//		for (; j < stats_data_vec.size() && current_max_score ==
// stats_data_vec[j].score; ++j)
//			;
//		if (j - i > 1u)
//		{
//			std::sort(stats_data_vec.begin() + i,
// stats_data_vec.begin() + j, [](const player_stats &ps1, const player_stats
//&ps2) 					  { return ps1.time_spent_on_server_in_seconds <
// ps2.time_spent_on_server_in_seconds; });
//		}
//
//		i = j - 1;
//	}
// }
//
// void save_players_stats_data(const char *file_path, std::vector<player_stats>
// &stats_data_vec, std::unordered_map<std::string, player_stats>
// &stats_data_map)
//{
//	sort_players_stats_data(stats_data_vec, stats_data_map);
//	ofstream output_file{file_path, std::ios::binary};
//	output_file.write(reinterpret_cast<const char *>(stats_data_vec.data()),
// stats_data_vec.size() * sizeof(player_stats)); 	output_file.flush();
//	output_file.close();
// }
//
// void load_players_stats_data(const char *file_path, std::vector<player_stats>
// &stats_data_vec, std::unordered_map<std::string, player_stats>
// &stats_data_map)
//{
//	std::lock_guard lg{main_app.get_stats_data().get_stats_data_mutex()};
//
//	stats_data_vec.clear();
//	stats_data_map.clear();
//
//	if (!check_if_file_path_exists(file_path))
//	{
//		ofstream output_file{file_path, std::ios::binary};
//		output_file.flush();
//		output_file.close();
//		return;
//	}
//
//	const size_t input_file_size = get_file_size_in_bytes(file_path);
//	if (input_file_size >= sizeof(player_stats))
//	{
//		const size_t number_of_player_stats = input_file_size /
// sizeof(player_stats); 		stats_data_vec.reserve(number_of_player_stats);
//		stats_data_vec.resize(number_of_player_stats, player_stats{});
//		ifstream input_file{file_path, std::ios::binary};
//		input_file.read(reinterpret_cast<char *>(stats_data_vec.data()),
// number_of_player_stats * sizeof(player_stats)); 		input_file.close();
//
//		for (const auto &ps : stats_data_vec)
//		{
//			if (len(ps.index_name) > 0u)
//			{
//				if (!stats_data_map.contains(ps.index_name))
//				{
//					stats_data_map.emplace(ps.index_name,
// ps);
//				}
//			}
//		}
//
//		sort_players_stats_data(stats_data_vec, stats_data_map);
//	}
// }
//
// std::string get_online_players_stats_data_report(std::vector<player_stats>
// &stats_data, std::unordered_map<std::string, player_stats> &stats_data_map,
// const char *title)
//{
//	sort_players_stats_data(stats_data, stats_data_map);
//	const size_t
// number_of_players{main_app.get_current_game_server().get_number_of_players()};
//	vector<player_stats> found_player_stats;
//	unordered_map<string, string> online_player_name_index_to_player_name;
//	for (const auto &pd :
// main_app.get_current_game_server().get_players_data())
//	{
//		online_player_name_index_to_player_name[pd.player_name_index] =
// pd.player_name;
//	}
//
//	size_t j{};
//	for (size_t i{}; i < stats_data.size() && j < number_of_players; ++i)
//	{
//		if
//(online_player_name_index_to_player_name.contains(stats_data[i].index_name))
//		{
//			found_player_stats.emplace_back(stats_data[i]);
//			found_player_stats.back().first_seen = i + 1;
//			++j;
//			online_player_name_index_to_player_name.erase(stats_data[i].index_name);
//		}
//	}
//
//	size_t longest_name_length{12};
//	if (!found_player_stats.empty())
//	{
//		longest_name_length = std::max(longest_name_length,
// find_longest_player_name_length(found_player_stats.cbegin(),
// found_player_stats.cend(), false));
//	}
//
//	ostringstream oss;
//	const string decoration_line(68 + longest_name_length, '=');
//	oss << "^5\n"
//		<< decoration_line << "\n";
//	std::string title_str{title};
//	stl::helper::trim_in_place(title_str);
//	remove_all_color_codes(title_str);
//	const size_t
// printed_name_char_count{get_number_of_characters_without_color_codes(title)};
//	const size_t padding_size{(decoration_line.length() - 4u -
// printed_name_char_count)}; 	const string pad_str(padding_size, ' '); 	if
//(printed_name_char_count + 4u <= decoration_line.length())
//	{
//		oss << "^5| " << left << title << right << pad_str << " ^5|\n";
//	}
//	else
//	{
//		oss << "^5| " << left << title << " ^5|\n";
//	}
//	oss << decoration_line << "\n";
//	oss << "^5| ";
//	oss << left << setw(6) << "Rank "
//		<< " | " << left << setw(longest_name_length) << "Player name"
//		<< " | " << left << setw(10) << "Score"
//		<< " | " << left << setw(17) << "Score/minute"
//		<< " | " << left << setw(20) << "Time played"
//		<< "^5|\n";
//	oss << decoration_line << "\n";
//	bool is_first_color{true};
//	char buffer[32]{};
//	for (size_t i{}; i < found_player_stats.size(); ++i)
//	{
//		const auto &user = found_player_stats[i];
//		const char *next_color{is_first_color ? "^3" : "^5"};
//		snprintf(buffer, std::size(buffer), "%lld.", user.first_seen);
//		oss << "^5| " << text_element{buffer, 6, next_color};
//		oss << " ^5| " << text_element{user.player_name,
// longest_name_length, "^7"} << " ^5| "; 		snprintf(buffer, std::size(buffer),
//"%lld", user.score); 		oss << text_element{buffer, 10, next_color} << " ^5| ";
//		const auto time_interval_in_minutes =
//(user.time_spent_on_server_in_seconds + 5) / 60; 		snprintf(buffer,
// std::size(buffer), "%.2lf spm", time_interval_in_minutes != 0 ?
// static_cast<double>(user.score) / time_interval_in_minutes : 0.0); 		oss <<
// text_element{buffer, 17, next_color} << " ^5| "; 		const string
// time_period_information{get_time_interval_info_string_for_seconds_in_hours_and_minutes(user.time_spent_on_server_in_seconds
//+ 5)}; 		oss << text_element{time_period_information.c_str(), 20, next_color} <<
//"^5|\n"; 		is_first_color = !is_first_color;
//	}
//
//	if (0u == j)
//	{
//		const size_t message_len = stl::helper::len("| No online
// players' stats data found!"); 		oss << "^5| ^3No online players' stats data
// found!";
//
//		if (message_len + 2 < decoration_line.length())
//		{
//			oss << string(decoration_line.length() - 2 -
// message_len, ' ');
//		}
//		oss << " ^5|\n";
//	}
//
//	oss << string{"^5"s + decoration_line + "\n\n"s};
//	return oss.str();
// }
//
// void update_player_scores(stats &tinyrcon_stats)
//{
//	static time_t last_timestamp{get_current_time_stamp()};
//
//	const size_t
// number_of_players{main_app.get_current_game_server().get_number_of_players()};
//	auto &current_players_data =
// main_app.get_current_game_server().get_players_data(); 	auto
//&previous_player_score_and_timestamp =
// main_app.get_stats_data().get_previous_player_score_and_timestamp();
//
//	const auto current_timestamp = get_current_time_stamp();
//
//	for (size_t i{}; i < std::min<size_t>(number_of_players, 64u); ++i)
//	{
//
//		const auto
//&current_index_name{current_players_data[i].player_name_index};
//
//		if (current_index_name.empty())
//			continue;
//
//		if
//(!previous_player_score_and_timestamp.contains(current_index_name))
//		{
//			previous_player_score_and_timestamp.emplace(current_index_name,
// make_pair(0, current_timestamp));
//		}
//
//		tinyrcon_stats.update_player_score(current_players_data[i],
// current_players_data[i].score -
// previous_player_score_and_timestamp[current_index_name].first,
// current_timestamp - last_timestamp, current_timestamp);
//		previous_player_score_and_timestamp[current_index_name].first =
// current_players_data[i].score;
//		previous_player_score_and_timestamp[current_index_name].second =
// current_timestamp;
//	}
//
//	last_timestamp = current_timestamp;
// }

bool download_bitmap_image_file(const char *bitmap_image_name, const char *destination_file_path)
{
    const std::string download_bitmap_image_url{format("ftp://{}/{}/data/images/maps/{}.bmp",
                                                       main_app.get_ftp_download_site_ip_address(),
                                                       main_app.get_ftp_download_folder_path(), bitmap_image_name)};
    return main_app.get_auto_update_manager().download_file(download_bitmap_image_url.c_str(), destination_file_path);
}

bool is_rcon_game_server(const game_server &gs)
{
    const auto &game_servers = main_app.get_game_servers();
    for (size_t i{}; i < std::min<size_t>(main_app.get_rcon_game_servers_count(), main_app.get_game_servers_count());
         ++i)
    {
        if (gs.get_server_ip_address() == game_servers[i].get_server_ip_address() &&
            gs.get_server_port() == game_servers[i].get_server_port())
        {
            return true;
        }
    }

    return false;
}
