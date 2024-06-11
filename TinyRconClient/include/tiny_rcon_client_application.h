#pragma once
// clang-format off
#include "autoupdate.h"
#include "bitmap_image_handler.h"
#include "connection_manager.h"
#include "connection_manager_for_messages.h"
#include "connection_manager_for_rcon_messages.h"
#include "game_server.h"
#include "internet_handle.h"
#include "tiny_rcon_client_user.h"
#include "tiny_rcon_utility_classes.h"
#include "tiny_rcon_utility_functions.h"
#include <atomic>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
// clang-format on

#undef max

using std::string;

extern tiny_rcon_handles app_handles;

class tiny_rcon_client_application : public disabled_copy_operations, public disabled_move_operations
{
    bool is_log_file_open{};
    bool is_custom_map_names_message_received{};
    bool is_draw_border_lines{true};
    bool is_disable_automatic_kick_messages{false};
    bool is_use_original_admin_messages{true};
    bool is_ftp_server_online{true};
    bool is_installed_cod2_game_steam_version{};
    bool is_use_different_background_colors_for_even_and_odd_lines{true};
    bool is_use_different_foreground_colors_for_even_and_odd_lines{false};
    bool is_automatic_city_kick_enabled{false};
    bool is_automatic_country_kick_enabled{false};
    bool is_enable_automatic_connection_flood_ip_ban{true};
    bool is_bans_synchronized{false};
    bool is_enable_automatic_program_updates{true};
    bool is_enable_automatic_missing_map_image_download{true};
    /*bool is_enable_players_stats_feature{false};
    bool is_enable_tracking_of_player_stats_data_for_day{false};
    bool is_enable_tracking_of_player_stats_data_for_month{false};
    bool is_enable_tracking_of_player_stats_data_for_year{false};
    bool is_enable_tracking_of_player_stats_data_permanently{false};
    size_t number_of_top_players_to_display_in_game_chat{10u};
    size_t number_of_top_players_to_display_in_tinyrcon{50u};
    size_t
    time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat{45u};
    size_t
    time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon{45u};
    size_t time_period_in_minutes_for_saving_players_stats_data{30u};*/
    size_t game_server_index{};
    size_t game_servers_count{};
    size_t rcon_game_servers_count{};
    static constexpr size_t max_game_servers_size{4096};
    std::atomic<uint64_t> previous_downloaded_data_in_bytes{0ULL};
    std::atomic<uint64_t> previous_uploaded_data_in_bytes{0ULL};
    std::atomic<uint64_t> next_downloaded_data_in_bytes{0ULL};
    std::atomic<uint64_t> next_uploaded_data_in_bytes{0ULL};
    const uint_least16_t tiny_rcon_server_port{27015};
    const uint_least16_t tiny_rcon_server_port_for_players{27017};
    game_name_t game_name{game_name_t::unknown};
    std::condition_variable command_queue_cv{};
    std::mutex command_mutex{};
    std::recursive_mutex command_queue_mutex{};
    std::recursive_mutex message_queue_mutex{};
    std::recursive_mutex remote_message_queue_mutex{};
    std::recursive_mutex tinyrcon_messages_queue_mutex{};
    string username{"^3Player"};
    string player_name{"^7Unknown Soldier"};
    std::string game_server_name{"185.158.113.146:28995 CoD2 CTF"};
    std::string codmp_exe_path;
    std::string cod2mp_s_exe_path;
    std::string iw3mp_exe_path;
    std::string cod5mp_exe_path;
    std::string command_line_info;
    std::string program_title{"Welcome to TinyRcon"};
    std::string game_version_number{"1.0"};
    std::string game_version_information;
    std::string user_ip_address;
    std::string current_working_directory;
    std::string tinyrcon_config_file_path{"config\\tinyrcon.json"};
    std::string user_data_file_path{"data\\user.txt"};
    std::string reported_players_file_path{"data\\reported_players.txt"};
    std::string temp_bans_file_path{"data\\tempbans.txt"};
    std::string ip_bans_file_path{"data\\bans.txt"};
    std::string ip_range_bans_file_path{"data\\ip_range_bans.txt"};
    std::string banned_countries_file_path{"data\\banned_countries.txt"};
    std::string banned_cities_file_path{"data\\banned_cities.txt"};
    std::string banned_names_file_path{"data\\banned_names.txt"};
    std::string protected_ip_addresses_file_path{"data\\protected_ip_addresses.txt"};
    std::string protected_ip_address_ranges_file_path{"data\\protected_ip_address_ranges.txt"};
    std::string protected_cities_file_path{"data\\protected_cities.txt"};
    std::string protected_countries_file_path{"data\\protected_countries.txt"};
    std::string ftp_download_site_ip_address{"85.222.189.119"};
    std::string ftp_download_folder_path{"tinyrcon"};
    std::string ftp_download_file_pattern{R"(^_U_TinyRcon[\._-]?v?(\d{1,2}\.\d{1,2}\.\d{1,2}\.\d{1,2})\.exe$)"};
    // std::string images_data_file_pattern{
    // LR"(^images_data-[0-9a-fA-F]{32}\.7z$)" };
    std::string plugins_geoIP_geo_dat_md5;
    std::string images_data_md5;
    std::string image_files_path_to_md5;
    std::ofstream log_file;
    std::string tiny_rcon_ftp_server_username{"tinyrcon"};
    std::string tiny_rcon_ftp_server_password{"08021980"};
    std::string tiny_rcon_server_ip_address{"85.222.189.119"};
    std::string tiny_rcon_server_ip_address_for_players{"85.222.189.119"};
    std::string cod2_master_server_ip_address{"185.34.107.159"};

    string current_match_info{"^3Map: {MAP_FULL_NAME} ^1({MAP_RCON_NAME}^1) ^3| Gametype: "
                              "{GAMETYPE_FULL_NAME} ^3| Online/Offline players: "
                              "{ONLINE_PLAYERS_COUNT}^3|{OFFLINE_PLAYERS_COUNT}"};
    string odd_player_data_lines_fg_color{"^5"};
    string even_player_data_lines_fg_color{"^5"};
    string odd_player_data_lines_bg_color{"^0"};
    string even_player_data_lines_bg_color{"^8"};
    string full_map_name_color{"^2"};
    string rcon_map_name_color{"^1"};
    string full_game_type_color{"^2"};
    string rcon_game_type_color{"^1"};
    string online_players_count_color{"^2"};
    string offline_players_count_color{"^1"};
    string border_line_color{"^5"};
    string header_player_pid_color{"^1"};
    string data_player_pid_color{"^1"};
    string header_player_score_color{"^4"};
    string data_player_score_color{"^4"};
    string header_player_ping_color{"^4"};
    string data_player_ping_color{"^4"};
    string header_player_name_color{"^4"};
    string header_player_ip_color{"^4"};
    string data_player_ip_color{"^4"};
    string header_player_geoinfo_color{"^4"};
    string data_player_geoinfo_color{"^4"};
    string server_message{"^5| ^3Server is empty!^5"};
    string last_date_and_time_information;
    size_t check_for_banned_players_time_period{5u};

    uint_least16_t cod2_master_server_port{20710};
    std::queue<command_t> command_queue{};
    std::queue<message_t> message_queue{};
    std::queue<message_t> remote_message_queue{};
    std::queue<print_message_t> tinyrcon_messages_to_print;

    std::vector<std::shared_ptr<tiny_rcon_client_user>> users;
    std::vector<std::shared_ptr<tiny_rcon_client_user>> players;
    std::vector<player> reported_players;

    map<string, string> server_settings;
    std::unordered_map<string, size_t> ip_address_frequency;
    std::unordered_map<std::string, std::string> admin_messages{
        {"user_defined_warn_msg", "^7{PLAYERNAME} ^1you have been warned by admin ^5{ADMINNAME}. ^3Reason: "
                                  "^1{REASON}"},
        {"user_defined_kick_msg", "^7{PLAYERNAME} ^1you are being kicked by admin ^5{ADMINNAME}. ^3Reason: "
                                  "^1{REASON}"},
        {"user_defined_temp_ban_msg", "^7{PLAYERNAME} ^7you are being ^1temporarily banned ^7for "
                                      "^1{TEMPBAN_DURATION} hours ^7by ^1admin {ADMINNAME}.{{br}}^3Reason: "
                                      "^1{REASON}"},
        {"user_defined_ban_msg", "^7{PLAYERNAME} ^1you are being banned by admin ^5{ADMINNAME}. ^3Reason: "
                                 "^1{REASON}"},
        {"user_defined_ip_ban_msg", "^7{PLAYERNAME} ^1you are being permanently banned by admin "
                                    "^5{ADMINNAME}. ^3Reason: ^1{REASON}"},
        {"user_defined_ip_address_range_ban_msg", "^7Admin ^5{ADMINNAME} ^7has ^1banned IP address range "
                                                  "^5{IP_ADDRESS_RANGE}. ^3Reason: ^1{REASON}"},
        {
            "user_defined_name_ban_msg",
            "^7Admin ^5{ADMINNAME} ^7has ^1banned player name: ^7{PLAYERNAME}",
        },
        {"user_defined_name_unban_msg", "^7Admin ^5{ADMINNAME} ^7has removed previously ^1banned player name: "
                                        "^7{PLAYERNAME}"},
        {"user_defined_city_ban_msg", "^7Admin ^5{ADMINNAME} ^1has globally banned city: ^5{CITY_NAME}"},
        {"user_defined_city_unban_msg", "^7Admin ^5{ADMINNAME} ^1has removed previously banned city: "
                                        "^5{CITY_NAME}"},
        {"user_defined_enable_city_ban_feature_msg",
         "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with "
         "^1IP addresses ^7from banned cities."},
        {
            "user_defined_disable_city_ban_feature_msg",
            "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players "
            "with ^1IP addresses ^7from banned cities.",
        },
        {
            "user_defined_country_ban_msg",
            "^7Admin ^5{ADMINNAME} ^1has banned country: ^5{COUNTRY_NAME}",
        },
        {"user_defined_country_unban_msg", "^7Admin ^5{ADMINNAME} ^1has removed previously banned country: "
                                           "^5{COUNTRY_NAME}"},
        {"user_defined_enable_country_ban_feature_msg",
         "^7Admin ^5{ADMINNAME} ^7has enabled ^1automatic kick ^7for players with "
         "^1IP addresses ^7from banned countries."},
        {"user_defined_disable_country_ban_feature_msg",
         "^7Admin ^5{ADMINNAME} ^7has disabled ^1automatic kick ^7for players "
         "with ^1IP addresses ^7from banned countries."},
        {"automatic_remove_temp_ban_msg", "^1{BANNED_BY}: ^7{PLAYERNAME}'s ^1temporary ban ^7[start date: "
                                          "^3{TEMP_BAN_START_DATE} ^7expired on ^3{TEMP_BAN_END_DATE}]{{br}}^7has "
                                          "automatically been removed. ^5Reason of ban: ^1{REASON}"},
        {"automatic_kick_temp_ban_msg", "^1{BANNED_BY}: ^7Temporarily banned player {PLAYERNAME} ^7is being "
                                        "automatically ^1kicked.{{br}}^7Your temporary ban expires on "
                                        "^1{TEMP_BAN_END_DATE}.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of "
                                        "ban: ^1{TEMP_BAN_START_DATE}"},
        {"automatic_kick_ip_ban_msg", "^7Player {PLAYERNAME} ^7with a previously ^1banned IP address ^7is "
                                      "being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| "
                                      "^5Date of ban: ^1{BAN_DATE}"},
        {"automatic_kick_ip_address_range_ban_msg", "^7Player {PLAYERNAME} ^7with an ^1IP address ^7from a previously "
                                                    "^1banned IP address range ^7is being automatically "
                                                    "^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| ^5Date of ban: "
                                                    "^1{BAN_DATE}"},
        {
            "automatic_kick_city_ban_msg",
            "^7Player {PLAYERNAME} ^7with an IP address from a ^1banned city: "
            "^5{CITY_NAME} ^7is being automatically ^1kicked.",
        },
        {"automatic_kick_country_ban_msg", "^7Player {PLAYERNAME} ^7with an IP address from a ^1banned country:  "
                                           "^5{COUNTRY_NAME} ^7is being automatically ^1kicked."},
        {"automatic_kick_name_ban_msg", "^7Player {PLAYERNAME} ^7with a previously ^1banned player name ^7is "
                                        "being automatically ^1kicked.{{br}}^5Reason of ban: ^1{REASON} ^7| "
                                        "^5Date of ban: ^1{BAN_DATE}"},
        {"user_defined_protect_ip_address_message", "^1{ADMINNAME} ^7has protected ^1IP address ^7of "
                                                    "{PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}"},
        {"user_defined_unprotect_ip_address_message", "^1{ADMINNAME} ^7has removed a previously protected ^1IP "
                                                      "address^7.{{br}}^5Reason: ^1{REASON}"},
        {"user_defined_protect_ip_address_range_message", "^1{ADMINNAME} ^7has protected ^1IP address range ^7of "
                                                          "{PLAYERNAME}^7.{{br}}^5Reason: ^1{REASON}"},
        {"user_defined_unprotect_ip_address_range_message",
         "^1{ADMINNAME} ^7has removed a previously ^1protected IP address "
         "range^7.{{br}}^5Reason: ^1{REASON}"},
        {"user_defined_protect_city_message", "^1{ADMINNAME} ^7has protected ^1city ({CITY_NAME}) ^7of "
                                              "{PLAYERNAME}^7."},
        {"user_defined_unprotect_city_message", "^1{ADMINNAME} ^7has removed a previously ^1protected city: "
                                                "{CITY_NAME}"},
        {"user_defined_protect_country_message", "^1{ADMINNAME} ^7has protected ^1country ({COUNTRY_NAME}) ^7of "
                                                 "{PLAYERNAME}^7."},
        {"user_defined_unprotect_country_message", "^1{ADMINNAME} ^7has removed a previously ^1protected country: "
                                                   "{COUNTRY_NAME}"}};

    const std::unordered_map<game_name_t, std::string> game_names{{game_name_t::unknown, "Unknown game!"},
                                                                  {game_name_t::cod1, "Call of Duty Multiplayer"},
                                                                  {game_name_t::cod2, "Call of Duty 2 Multiplayer"},
                                                                  {game_name_t::cod4, "Call of Duty 4: Modern Warfare"},
                                                                  {game_name_t::cod5, "Call of Duty 5: World at War"}};

    const std::unordered_map<std::string, int> cod2_game_version_to_protocol{
        {"1.0", 115}, {"1.01", 115}, {"1.2", 117}, {"1.3", 118}};

    std::map<std::string, std::pair<std::string, std::string>> available_rcon_to_full_map_names{
        {"mp_breakout", {"Villers-Bocage, France", "Villers-Bocage, France"}},
        {"mp_brecourt", {"Brecourt, France", "Brecourt, France"}},
        {"mp_burgundy", {"Burgundy, France", "Burgundy, France"}},
        {"mp_carentan", {"Carentan, France", "Carentan, France"}},
        {"mp_dawnville", {"St. Mere Eglise, France", "St. Mere Eglise, France"}},
        {"mp_decoy", {"El Alamein, Egypt", "El Alamein, Egypt"}},
        {"mp_downtown", {"Moscow, Russia", "Moscow, Russia"}},
        {"mp_farmhouse", {"Beltot, France", "Beltot, France"}},
        {"mp_leningrad", {"Leningrad, Russia", "Leningrad, Russia"}},
        {"mp_matmata", {"Matmata, Tunisia", "Matmata, Tunisia"}},
        {"mp_railyard", {"Stalingrad, Russia", "Stalingrad, Russia"}},
        {"mp_toujane", {"Toujane, Tunisia", "Toujane, Tunisia"}},
        {"mp_trainstation", {"Caen, France", "Caen, France"}}};
    std::map<std::string, std::string> available_full_map_to_rcon_map_names{
        {"Villers-Bocage, France", "mp_breakout"},   {"Brecourt, France", "mp_brecourt"},
        {"Burgundy, France", "mp_burgundy"},         {"Carentan, France", "mp_carentan"},
        {"St. Mere Eglise, France", "mp_dawnville"}, {"El Alamein, Egypt", "mp_decoy"},
        {"Moscow, Russia", "mp_downtown"},           {"Beltot, France", "mp_farmhouse"},
        {"Leningrad, Russia", "mp_leningrad"},       {"Matmata, Tunisia", "mp_matmata"},
        {"Stalingrad, Russia", "mp_railyard"},       {"Toujane, Tunisia", "mp_toujane"},
        {"Caen, France", "mp_trainstation"},
    };
    std::map<std::string, std::shared_ptr<tiny_rcon_client_user>> name_to_user;
    std::map<std::string, std::shared_ptr<tiny_rcon_client_user>> player_to_user;
    // std::unordered_map<std::string, bool> is_user_data_received;

    std::unordered_map<std::string, std::string> tinyrcon_dict{
        {"{ADMINNAME}", username}, {"{PLAYERNAME}", ""}, {"{REASON}", "not specified"}};

    std::unordered_map<std::string, std::function<void(const std::vector<std::string> &)>> command_handlers;
    std::unordered_map<std::string,
                       std::function<void(const std::string &, const time_t, const std::string &, const bool)>>
        message_handlers;
    std::unordered_map<std::string,
                       std::function<void(const std::string &, const time_t, const std::string &, const bool)>>
        remote_message_handlers;

    auto_update_manager au;
    bitmap_image_handler bih;
    // stats stats_data;
    connection_manager rcon_connection_manager;
    connection_manager_for_messages cm_for_messages;
    connection_manager_for_rcon_messages cm_for_rcon_messages;
    std::array<game_server, max_game_servers_size> game_servers;

  public:
    tiny_rcon_client_application() = default;

    ~tiny_rcon_client_application() = default;

    bool get_is_disable_automatic_kick_messages() const noexcept
    {
        return is_disable_automatic_kick_messages;
    }

    void set_is_disable_automatic_kick_messages(const bool new_value) noexcept
    {
        is_disable_automatic_kick_messages = new_value;
    }

    bool get_is_use_original_admin_messages() const noexcept
    {
        return is_use_original_admin_messages;
    }

    void set_is_use_original_admin_messages(const bool new_value) noexcept
    {
        is_use_original_admin_messages = new_value;
    }

    bool get_is_ftp_server_online() const noexcept
    {
        return is_ftp_server_online;
    }

    void set_is_ftp_server_online(const bool new_value) noexcept
    {
        is_ftp_server_online = new_value;
    }

    bool get_is_installed_cod2_game_steam_version() const noexcept
    {
        return is_installed_cod2_game_steam_version;
    }

    void set_is_installed_cod2_game_steam_version(const bool new_value) noexcept
    {
        is_installed_cod2_game_steam_version = new_value;
    }

    void set_is_bans_synchronized(const bool new_value) noexcept
    {
        is_bans_synchronized = new_value;
    }

    bool get_is_bans_synchronized() const noexcept
    {
        return is_bans_synchronized;
    }

    bool get_is_enable_automatic_connection_flood_ip_ban() const noexcept
    {
        return is_enable_automatic_connection_flood_ip_ban;
    }

    void set_is_enable_automatic_connection_flood_ip_ban(const bool new_value) noexcept
    {
        is_enable_automatic_connection_flood_ip_ban = new_value;
    }

    bool get_is_automatic_city_kick_enabled() const noexcept
    {
        return is_automatic_city_kick_enabled;
    }

    void set_is_automatic_city_kick_enabled(const bool new_value) noexcept
    {
        is_automatic_city_kick_enabled = new_value;
    }

    bool get_is_automatic_country_kick_enabled() const noexcept
    {
        return is_automatic_country_kick_enabled;
    }

    void set_is_automatic_country_kick_enabled(const bool new_value) noexcept
    {
        is_automatic_country_kick_enabled = new_value;
    }

    bool get_is_use_different_background_colors_for_even_and_odd_lines() const noexcept
    {
        return is_use_different_background_colors_for_even_and_odd_lines;
    }

    void set_is_use_different_background_colors_for_even_and_odd_lines(const bool new_value) noexcept
    {
        is_use_different_background_colors_for_even_and_odd_lines = new_value;
    }

    bool get_is_use_different_foreground_colors_for_even_and_odd_lines() const noexcept
    {
        return is_use_different_foreground_colors_for_even_and_odd_lines;
    }

    void set_is_use_different_foreground_colors_for_even_and_odd_lines(const bool new_value) noexcept
    {
        is_use_different_foreground_colors_for_even_and_odd_lines = new_value;
    }

    bool get_is_enable_automatic_program_updates() const noexcept
    {
        return is_enable_automatic_program_updates;
    }

    void set_is_enable_automatic_program_updates(const bool new_value) noexcept
    {
        is_enable_automatic_program_updates = new_value;
    }

    bool get_is_enable_automatic_missing_map_image_download() const noexcept
    {
        return is_enable_automatic_missing_map_image_download;
    }

    void set_is_enable_automatic_missing_map_image_download(const bool new_value) noexcept
    {
        is_enable_automatic_missing_map_image_download = new_value;
    }

    /*bool get_is_enable_players_stats_feature() const noexcept
    {
            return is_enable_players_stats_feature;
    }

    void set_is_enable_players_stats_feature(const bool new_value) noexcept
    {
            is_enable_players_stats_feature = new_value;
    }

    bool get_is_enable_tracking_of_player_stats_data_for_day() const noexcept
    {
            return is_enable_tracking_of_player_stats_data_for_day;
    }

    void set_is_enable_tracking_of_player_stats_data_for_day(const bool new_value)
    noexcept
    {
            is_enable_tracking_of_player_stats_data_for_day = new_value;
    }

    bool get_is_enable_tracking_of_player_stats_data_for_month() const noexcept
    {
            return is_enable_tracking_of_player_stats_data_for_month;
    }

    void set_is_enable_tracking_of_player_stats_data_for_month(const bool
    new_value) noexcept
    {
            is_enable_tracking_of_player_stats_data_for_month = new_value;
    }

    bool get_is_enable_tracking_of_player_stats_data_for_year() const noexcept
    {
            return is_enable_tracking_of_player_stats_data_for_year;
    }

    void set_is_enable_tracking_of_player_stats_data_for_year(const bool
    new_value) noexcept
    {
            is_enable_tracking_of_player_stats_data_for_year = new_value;
    }

    bool get_is_enable_tracking_of_player_stats_data_permanently() const noexcept
    {
            return is_enable_tracking_of_player_stats_data_permanently;
    }

    void set_is_enable_tracking_of_player_stats_data_permanently(const bool
    new_value) noexcept
    {
            is_enable_tracking_of_player_stats_data_permanently = new_value;
    }

    size_t get_number_of_top_players_to_display_in_game_chat() const noexcept
    {
            return number_of_top_players_to_display_in_game_chat;
    }

    void set_number_of_top_players_to_display_in_game_chat(const size_t new_value)
    noexcept
    {
            number_of_top_players_to_display_in_game_chat = new_value;
    }

    size_t get_number_of_top_players_to_display_in_tinyrcon() const noexcept
    {
            return number_of_top_players_to_display_in_tinyrcon;
    }

    void set_number_of_top_players_to_display_in_tinyrcon(const size_t new_value)
    noexcept
    {
            number_of_top_players_to_display_in_tinyrcon = new_value;
    }

    size_t
    get_time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat()
    const noexcept
    {
            return
    time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat;
    }

    void
    set_time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat(const
    size_t new_value) noexcept
    {
            time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat
    = new_value;
    }

    size_t
    get_time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon()
    const noexcept
    {
            return
    time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon;
    }

    void
    set_time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon(const
    size_t new_value) noexcept
    {
            time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon
    = new_value;
    }

    size_t get_time_period_in_minutes_for_saving_players_stats_data() const
    noexcept
    {
            return time_period_in_minutes_for_saving_players_stats_data;
    }

    void set_time_period_in_minutes_for_saving_players_stats_data(const size_t
    new_value) noexcept
    {
            time_period_in_minutes_for_saving_players_stats_data = new_value;
    }*/

    inline game_name_t get_game_name() const noexcept
    {
        return game_name;
    }

    inline void set_game_name(const game_name_t new_game_name) noexcept
    {
        game_name = new_game_name;
    }

    inline std::condition_variable &get_command_queue_cv() noexcept
    {
        return command_queue_cv;
    }

    inline std::mutex &get_command_queue_mutex() noexcept
    {
        return command_mutex;
    }

    inline const string &get_username() const noexcept
    {
        return username;
    }

    inline void set_username(string new_value)
    {
        remove_disallowed_character_in_string(new_value);
        username = std::move(new_value);
    }

    inline void set_player_name(string new_value)
    {
        remove_disallowed_character_in_string(new_value);
        player_name = std::move(new_value);
    }

    inline const string &get_player_name() const noexcept
    {
        return player_name;
    }

    inline const string &get_game_server_name() const noexcept
    {
        return game_server_name;
    }

    inline void set_game_server_name(string new_value) noexcept
    {
        game_server_name = std::move(new_value);
    }

    inline const std::string &get_codmp_exe_path() const noexcept
    {
        return codmp_exe_path;
    }

    inline void set_codmp_exe_path(string new_value) noexcept
    {
        codmp_exe_path = std::move(new_value);
    }

    inline const string &get_cod2mp_exe_path() const noexcept
    {
        return cod2mp_s_exe_path;
    }

    inline void set_cod2mp_exe_path(string new_value) noexcept
    {
        cod2mp_s_exe_path = std::move(new_value);
    }

    inline const string &get_iw3mp_exe_path() const noexcept
    {
        return iw3mp_exe_path;
    }

    inline void set_iw3mp_exe_path(string new_value) noexcept
    {
        iw3mp_exe_path = std::move(new_value);
    }

    inline const string &get_cod5mp_exe_path() const noexcept
    {
        return cod5mp_exe_path;
    }

    inline void set_cod5mp_exe_path(string new_value) noexcept
    {
        cod5mp_exe_path = std::move(new_value);
    }

    inline void set_command_line_info(string new_value) noexcept
    {
        command_line_info = std::move(new_value);
    }

    inline connection_manager &get_connection_manager() noexcept
    {
        return rcon_connection_manager;
    }

    inline connection_manager_for_messages &get_connection_manager_for_messages() noexcept
    {
        return cm_for_messages;
    }

    inline connection_manager_for_rcon_messages &get_connection_manager_for_rcon_messages() noexcept
    {
        return cm_for_rcon_messages;
    }

    inline auto_update_manager &get_auto_update_manager() noexcept
    {
        return au;
    }

    inline bitmap_image_handler &get_bitmap_image_handler() noexcept
    {

        return bih;
    }

    inline size_t get_game_servers_count() const noexcept
    {
        return game_servers_count;
    }

    inline void set_game_servers_count(const size_t new_value) noexcept
    {
        if (new_value < max_game_servers_size)
            game_servers_count = new_value;
    }

    inline size_t get_rcon_game_servers_count() const noexcept
    {
        return rcon_game_servers_count;
    }

    inline void set_rcon_game_servers_count(const size_t new_value) noexcept
    {
        rcon_game_servers_count = new_value <= game_servers.size() ? new_value : 0;
    }

    inline size_t get_game_server_index() const noexcept
    {
        return game_server_index;
    }

    void set_game_server_index(const size_t new_server_index) noexcept
    {
        if (new_server_index < game_servers_count)
            game_server_index = new_server_index;
    }

    inline uint64_t get_previous_downloaded_data_in_bytes() const noexcept
    {
        return previous_downloaded_data_in_bytes.load();
    }

    void set_previous_downloaded_data_in_bytes(const size_t new_value) noexcept
    {
        previous_downloaded_data_in_bytes.store(new_value);
    }

    inline uint64_t get_previous_uploaded_data_in_bytes() const noexcept
    {
        return previous_uploaded_data_in_bytes.load();
    }

    void set_previous_uploaded_data_in_bytes(const size_t new_value) noexcept
    {
        previous_uploaded_data_in_bytes.store(new_value);
    }

    inline uint64_t get_next_downloaded_data_in_bytes() const noexcept
    {
        return next_downloaded_data_in_bytes.load();
    }

    void add_to_next_downloaded_data_in_bytes(const size_t value) noexcept
    {
        next_downloaded_data_in_bytes.fetch_add(value);
    }

    inline uint64_t get_next_uploaded_data_in_bytes() const noexcept
    {
        return next_uploaded_data_in_bytes.load();
    }

    void add_to_next_uploaded_data_in_bytes(const size_t value) noexcept
    {
        next_uploaded_data_in_bytes.fetch_add(value);
    }

    void update_download_and_upload_speed_statistics()
    {
        static constexpr uint64_t one_gigabyte{1024U * 1024U * 1024U};
        static constexpr uint64_t one_megabyte{1024U * 1024U};
        static constexpr uint64_t one_kilobyte{1024U};
        static auto last_time_stamp = std::chrono::high_resolution_clock::now();

        const auto now_time_stamp = std::chrono::high_resolution_clock::now();
        const double elapsed_time_interval_in_seconds =
            static_cast<double>(std::chrono::duration<double, std::milli>{now_time_stamp - last_time_stamp}.count()) /
            1000.0;

        uint64_t uploaded_gigabytes{}, uploaded_megabytes{}, uploaded_kilobytes{};
        const uint64_t uploaded_bytes = next_uploaded_data_in_bytes.load();
        std::ostringstream oss_ua;

        if (uploaded_bytes >= one_gigabyte)
        {
            oss_ua << std::format("^1{:.2f} GB", static_cast<double>(uploaded_bytes) / one_gigabyte);
        }
        else if (uploaded_bytes >= one_megabyte)
        {
            oss_ua << std::format("^1{:.2f} MB", static_cast<double>(uploaded_bytes) / one_megabyte);
        }
        else if (uploaded_bytes >= one_kilobyte)
        {
            oss_ua << std::format("^1{:.2f} kB", static_cast<double>(uploaded_bytes) / one_kilobyte);
        }
        else
        {
            oss_ua << std::format("^1{} bytes", uploaded_bytes);
        }

        uploaded_gigabytes = uploaded_megabytes = uploaded_kilobytes = 0ULL;
        const uint64_t uploaded_bytes_diff =
            static_cast<uint64_t>((next_uploaded_data_in_bytes.load() - previous_uploaded_data_in_bytes.load()) /
                                  elapsed_time_interval_in_seconds);
        std::ostringstream oss_us;
        oss_us << "^3Upload speed: ";

        if (uploaded_bytes_diff >= one_gigabyte)
        {
            oss_us << std::format("^1{:.2f} GB", static_cast<double>(uploaded_bytes_diff) / one_gigabyte);
        }
        else if (uploaded_bytes_diff >= one_megabyte)
        {
            oss_us << std::format("^1{:.2f} MB", static_cast<double>(uploaded_bytes_diff) / one_megabyte);
        }
        else if (uploaded_bytes_diff >= one_kilobyte)
        {
            oss_us << std::format("^1{:.2f} kB", static_cast<double>(uploaded_bytes_diff) / one_kilobyte);
        }
        else
        {
            oss_us << std::format("^1{} bytes", uploaded_bytes_diff);
        }

        oss_us << " ^3Total: " << oss_ua.str();
        const string upload_speed_message{oss_us.str()};

        uint64_t downloaded_gigabytes{}, downloaded_megabytes{}, downloaded_kilobytes{};
        const uint64_t downloaded_bytes = next_downloaded_data_in_bytes.load();
        std::ostringstream oss_da;

        if (downloaded_bytes >= one_gigabyte)
        {
            oss_da << std::format("^1{:.2f} GB", static_cast<double>(downloaded_bytes) / one_gigabyte);
        }
        else if (downloaded_bytes >= one_megabyte)
        {
            oss_da << std::format("^1{:.2f} MB", static_cast<double>(downloaded_bytes) / one_megabyte);
        }
        else if (downloaded_bytes >= one_kilobyte)
        {
            oss_da << std::format("^1{:.2f} kB", static_cast<double>(downloaded_bytes) / one_kilobyte);
        }
        else
        {
            oss_da << std::format("^1{} bytes", downloaded_bytes);
        }

        downloaded_gigabytes = downloaded_megabytes = downloaded_kilobytes = 0ULL;
        const uint64_t downloaded_bytes_diff =
            static_cast<uint64_t>((next_downloaded_data_in_bytes.load() - previous_downloaded_data_in_bytes.load()) /
                                  elapsed_time_interval_in_seconds);
        std::ostringstream oss_ds;

        oss_ds << "^2Download speed: ";

        if (downloaded_bytes_diff >= one_gigabyte)
        {
            oss_ds << std::format("^1{:.2f} GB", static_cast<double>(downloaded_bytes_diff) / one_gigabyte);
        }
        else if (downloaded_bytes_diff >= one_megabyte)
        {
            oss_ds << std::format("^1{:.2f} MB", static_cast<double>(downloaded_bytes_diff) / one_megabyte);
        }
        else if (downloaded_bytes_diff >= one_kilobyte)
        {
            oss_ds << std::format("^1{:.2f} kB", static_cast<double>(downloaded_bytes_diff) / one_kilobyte);
        }
        else
        {
            oss_ds << std::format("^1{} bytes", downloaded_bytes_diff);
        }

        oss_ds << " ^2Total: " << oss_da.str();
        const string download_speed_message{oss_ds.str()};
        const string speed_message{format("{} ^5| {}", oss_ds.str(), oss_us.str())};
        SetWindowTextA(app_handles.hwnd_upload_speed_info, "");
        print_colored_text(app_handles.hwnd_upload_speed_info, speed_message.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no, true);
        previous_uploaded_data_in_bytes.store(next_uploaded_data_in_bytes.load());
        previous_downloaded_data_in_bytes.store(next_downloaded_data_in_bytes.load());
        last_time_stamp = now_time_stamp;
    }

    inline game_server &get_current_game_server() noexcept
    {
        if (game_server_index >= game_servers_count)
            game_server_index = 0U;

        return game_servers[game_server_index];
    }

    inline std::array<game_server, max_game_servers_size> &get_game_servers() noexcept
    {
        return game_servers;
    }

    inline std::unordered_map<std::string, std::string> &get_admin_messages() noexcept
    {
        return admin_messages;
    }

    inline const std::string &get_program_title() const noexcept
    {
        return program_title;
    }

    inline void set_program_title(std::string new_program_title) noexcept
    {
        program_title = std::move(new_program_title);
    }

    inline const std::string &get_user_ip_address() const noexcept
    {
        return user_ip_address;
    }

    inline void set_user_ip_address(std::string new_user_ip_address) noexcept
    {
        user_ip_address = std::move(new_user_ip_address);
    }

    const std::unordered_map<std::string, int> &get_cod2_game_version_to_protocol() noexcept
    {
        return cod2_game_version_to_protocol;
    }

    std::map<std::string, std::pair<std::string, std::string>> &get_available_rcon_to_full_map_names() noexcept
    {
        return available_rcon_to_full_map_names;
    }

    std::map<std::string, std::string> &get_available_full_map_to_rcon_map_names() noexcept
    {
        return available_full_map_to_rcon_map_names;
    }

    std::vector<std::shared_ptr<tiny_rcon_client_user>> &get_users() noexcept
    {
        return users;
    }

    std::vector<std::shared_ptr<tiny_rcon_client_user>> &get_players() noexcept
    {
        return players;
    }

    // reported players
    std::vector<player> &get_reported_players() noexcept
    {
        return reported_players;
    }

    std::shared_ptr<tiny_rcon_client_user> &get_user_for_name(const std::string &name)
    {
        string cleaned_name{get_cleaned_user_name(name)};

        if (!name_to_user.contains(cleaned_name))
        {
            users.emplace_back(std::make_shared<tiny_rcon_client_user>());
            users.back()->user_name = name;
            name_to_user.emplace(cleaned_name, users.back());
        }

        return name_to_user.at(cleaned_name);
    }

    // bool is_user_admin(const std::string &name) const noexcept
    //{
    //   string cleaned_name{ get_cleaned_user_name(name) };
    //   // unsigned long guid_key{};
    //   // if (check_ip_address_validity(ip_address, guid_key)) {
    //   // player pd{};
    //   //
    //   convert_guid_key_to_country_name(rcon_connection_manager.get_geoip_data(),
    //   ip_address, pd);
    //   // if (cleaned_name == "admin") {
    //   // cleaned_name += std::format("_{}", pd.country_name);
    //   // }
    //   // }
    //   return name_to_user.contains(cleaned_name);
    // }

    std::shared_ptr<tiny_rcon_client_user> &get_player_for_name(const std::string &name, const string &ip_address)
    {
        string cleaned_name{get_cleaned_user_name(name)};
        unsigned long guid_key{};
        if (check_ip_address_validity(ip_address, guid_key))
            cleaned_name += std::format("_{}", ip_address);

        if (!player_to_user.contains(cleaned_name))
        {
            players.emplace_back(std::make_shared<tiny_rcon_client_user>());
            players.back()->user_name = name;
            player_to_user.emplace(cleaned_name, players.back());
        }

        return player_to_user.at(cleaned_name);
    }

    std::unordered_map<std::string, std::string> &get_tinyrcon_dict() noexcept
    {
        return tinyrcon_dict;
    }

    std::string get_user_defined_warn_message()
    {
        return admin_messages["user_defined_warn_msg"];
    }

    std::string get_user_defined_kick_message()
    {
        return admin_messages["user_defined_kick_msg"];
    }

    std::string get_user_defined_tempban_message()
    {
        return admin_messages["user_defined_temp_ban_msg"];
    }

    std::string get_user_defined_ban_message()
    {
        return admin_messages["user_defined_ban_msg"];
    }

    std::string get_user_defined_ipban_message()
    {
        return admin_messages["user_defined_ip_ban_msg"];
    }

    std::string get_user_defined_ip_address_range_ban_message()
    {
        return admin_messages["user_defined_ip_address_range_ban_msg"];
    }

    std::string get_user_defined_name_ban_message()
    {
        return admin_messages["user_defined_name_ban_msg"];
    }

    std::string get_user_defined_name_unban_message()
    {
        return admin_messages["user_defined_name_unban_msg"];
    }

    std::string get_user_defined_city_ban_msg()
    {
        return admin_messages["user_defined_city_ban_msg"];
    }

    std::string get_user_defined_city_unban_msg()
    {
        return admin_messages["user_defined_city_unban_msg"];
    }

    std::string get_user_defined_enable_city_ban_feature_msg()
    {
        return admin_messages["user_defined_enable_city_ban_feature_msg"];
    }

    std::string get_user_defined_disable_city_ban_feature_msg()
    {
        return admin_messages["user_defined_disable_city_ban_feature_msg"];
    }

    std::string get_user_defined_country_ban_msg()
    {
        return admin_messages["user_defined_country_ban_msg"];
    }

    std::string get_user_defined_country_unban_msg()
    {
        return admin_messages["user_defined_country_unban_msg"];
    }

    std::string get_user_defined_enable_country_ban_feature_msg()
    {
        return admin_messages["user_defined_enable_country_ban_feature_msg"];
    }

    std::string get_user_defined_disable_country_ban_feature_msg()
    {
        return admin_messages["user_defined_disable_country_ban_feature_msg"];
    }

    std::string get_automatic_remove_temp_ban_msg()
    {
        return admin_messages["automatic_remove_temp_ban_msg"];
    }

    std::string get_automatic_kick_temp_ban_msg()
    {
        return admin_messages["automatic_kick_temp_ban_msg"];
    }

    std::string get_automatic_kick_ip_ban_msg()
    {
        return admin_messages["automatic_kick_ip_ban_msg"];
    }

    std::string get_automatic_kick_ip_address_range_ban_msg()
    {
        return admin_messages["automatic_kick_ip_address_range_ban_msg"];
    }

    std::string get_automatic_kick_city_ban_msg()
    {
        return admin_messages["automatic_kick_city_ban_msg"];
    }

    std::string get_automatic_kick_country_ban_msg()
    {
        return admin_messages["automatic_kick_country_ban_msg"];
    }

    std::string get_automatic_kick_name_ban_msg()
    {
        return admin_messages["automatic_kick_name_ban_msg"];
    }

    std::string get_user_defined_protect_ip_address_message()
    {
        return admin_messages["user_defined_protect_ip_address_message"];
    }

    std::string get_user_defined_unprotect_ip_address_message()
    {
        return admin_messages["user_defined_unprotect_ip_address_message"];
    }

    std::string get_user_defined_protect_ip_address_range_message()
    {
        return admin_messages["user_defined_protect_ip_address_range_message"];
    }

    std::string get_user_defined_unprotect_ip_address_range_message()
    {
        return admin_messages["user_defined_unprotect_ip_address_range_message"];
    }

    std::string get_user_defined_protect_city_message()
    {
        return admin_messages["user_defined_protect_city_message"];
    }

    std::string get_user_defined_unprotect_city_message()
    {
        return admin_messages["user_defined_unprotect_city_message"];
    }

    std::string get_user_defined_protect_country_message()
    {
        return admin_messages["user_defined_protect_country_message"];
    }

    std::string get_user_defined_unprotect_country_message()
    {
        return admin_messages["user_defined_unprotect_country_message"];
    }

    inline const char *get_game_title() noexcept
    {
        if (game_names.contains(game_name))
            return game_names.at(game_name).c_str();
        return "Unknown game";
    }

    /*bool get_is_config_file_copied() const noexcept
    {
      return is_config_file_copied;
    }

    void set_is_config_file_copied(const bool new_value) noexcept
    {
      is_config_file_copied = new_value;
    }*/

    bool get_is_custom_map_names_message_received() const noexcept
    {
        return is_custom_map_names_message_received;
    }

    void set_is_custom_map_names_message_received(const bool new_value) noexcept
    {
        is_custom_map_names_message_received = new_value;
    }

    bool get_is_draw_border_lines() const noexcept
    {
        return is_draw_border_lines;
    }

    void set_is_draw_border_lines(const bool new_value) noexcept
    {
        is_draw_border_lines = new_value;
    }

    const std::string &get_current_working_directory() const noexcept
    {
        return current_working_directory;
    }

    void set_current_working_directory()
    {
        constexpr size_t max_path_limit{32768};
        std::unique_ptr<char[]> exe_file_path{std::make_unique<char[]>(max_path_limit)};
        if (const auto path_len = GetModuleFileName(nullptr, exe_file_path.get(), max_path_limit); path_len != 0)
        {
            std::string_view exe_file_path_sv(exe_file_path.get(), path_len);
            current_working_directory.assign(
                std::string(cbegin(exe_file_path_sv), cbegin(exe_file_path_sv) + exe_file_path_sv.rfind('\\') + 1));
        }
        else
        {
            std::filesystem::path entry("TinyRcon.exe");
            current_working_directory.assign(entry.parent_path().string());
            if (!current_working_directory.empty() && current_working_directory.back() != '\\')
            {
                if ('/' == current_working_directory.back())
                    current_working_directory.back() = '\\';
                else
                    current_working_directory.push_back('\\');
            }
        }

        /*tinyrcon_config_file_path.assign(format("\"{}{}\"",
        current_working_directory, tinyrcon_config_file_path));
        user_data_file_path.assign(format("\"{}{}\"", current_working_directory,
        user_data_file_path)); reported_players_file_path.assign(format("\"{}{}\"",
        current_working_directory, reported_players_file_path));
        temp_bans_file_path.assign(format("\"{}{}\"", current_working_directory,
        temp_bans_file_path)); ip_bans_file_path.assign(format("\"{}{}\"",
        current_working_directory, ip_bans_file_path));
        ip_range_bans_file_path.assign(format("\"{}{}\"", current_working_directory,
        ip_range_bans_file_path));
        banned_countries_file_path.assign(format("\"{}{}\"",
        current_working_directory, banned_countries_file_path));
        banned_cities_file_path.assign(format("\"{}{}\"", current_working_directory,
        banned_cities_file_path)); banned_names_file_path.assign(format("\"{}{}\"",
        current_working_directory, banned_names_file_path));
        protected_ip_addresses_file_path.assign(format("\"{}{}\"",
        current_working_directory, protected_ip_addresses_file_path));
        protected_ip_address_ranges_file_path.assign(format("\"{}{}\"",
        current_working_directory, protected_ip_address_ranges_file_path));
        protected_cities_file_path.assign(format("\"{}{}\"",
        current_working_directory, protected_cities_file_path));
        protected_countries_file_path.assign(format("\"{}{}\"",
        current_working_directory, protected_countries_file_path));*/
    }

    const char *get_tinyrcon_config_file_path() const noexcept
    {
        return tinyrcon_config_file_path.c_str();
    }

    const char *get_user_data_file_path() const noexcept
    {
        return user_data_file_path.c_str();
    }

    // reported_players_file_path
    const char *get_reported_players_file_path() const noexcept
    {
        return reported_players_file_path.c_str();
    }

    const char *get_temp_bans_file_path() const noexcept
    {
        return temp_bans_file_path.c_str();
    }

    const char *get_ip_bans_file_path() const noexcept
    {
        return ip_bans_file_path.c_str();
    }

    const char *get_ip_range_bans_file_path() const noexcept
    {
        return ip_range_bans_file_path.c_str();
    }

    const char *get_banned_countries_file_path() const noexcept
    {
        return banned_countries_file_path.c_str();
    }

    const char *get_banned_cities_file_path() const noexcept
    {
        return banned_cities_file_path.c_str();
    }

    const char *get_banned_names_file_path() const noexcept
    {
        return banned_names_file_path.c_str();
    }

    const char *get_protected_ip_addresses_file_path() const noexcept
    {
        return protected_ip_addresses_file_path.c_str();
    }

    const char *get_protected_ip_address_ranges_file_path() const noexcept
    {
        return protected_ip_address_ranges_file_path.c_str();
    }

    const char *get_protected_cities_file_path() const noexcept
    {
        return protected_cities_file_path.c_str();
    }

    const char *get_protected_countries_file_path() const noexcept
    {
        return protected_countries_file_path.c_str();
    }

    const std::string &get_tiny_rcon_ftp_server_username() const noexcept
    {
        return tiny_rcon_ftp_server_username;
    }

    void set_tiny_rcon_ftp_server_username(string new_tiny_rcon_ftp_server_username) noexcept
    {
        tiny_rcon_ftp_server_username = std::move(new_tiny_rcon_ftp_server_username);
    }

    const std::string &get_tiny_rcon_ftp_server_password() const noexcept
    {
        return tiny_rcon_ftp_server_password;
    }

    void set_tiny_rcon_ftp_server_password(string new_tiny_rcon_ftp_server_password) noexcept
    {
        tiny_rcon_ftp_server_password = std::move(new_tiny_rcon_ftp_server_password);
    }

    const std::string &get_tiny_rcon_server_ip_address() const noexcept
    {
        return tiny_rcon_server_ip_address;
    }

    void set_tiny_rcon_server_ip_address(string new_tiny_rcon_server_ip_address) noexcept
    {
        tiny_rcon_server_ip_address = std::move(new_tiny_rcon_server_ip_address);
    }

    const std::string &get_tiny_rcon_server_ip_address_for_players() const noexcept
    {
        return tiny_rcon_server_ip_address_for_players;
    }

    void set_tiny_rcon_server_ip_address_for_players(string new_tiny_rcon_server_ip_address_for_players) noexcept
    {
        tiny_rcon_server_ip_address_for_players = std::move(new_tiny_rcon_server_ip_address_for_players);
    }

    uint_least16_t get_tiny_rcon_server_port() const noexcept
    {
        return tiny_rcon_server_port;
    }

    // void set_tiny_rcon_server_port(const int new_tiny_rcon_server_port)
    // noexcept
    // {
    // 	tiny_rcon_server_port =
    // static_cast<uint_least16_t>(new_tiny_rcon_server_port);
    // }

    uint_least16_t get_tiny_rcon_server_port_for_players() const noexcept
    {
        return tiny_rcon_server_port_for_players;
    }

    // void set_tiny_rcon_server_port_for_players(const int
    // new_tiny_rcon_server_port_for_players) noexcept
    // {
    // 	tiny_rcon_server_port_for_players =
    // static_cast<uint_least16_t>(new_tiny_rcon_server_port_for_players);
    // }

    const std::string &get_game_version_number() const noexcept
    {
        return game_version_number;
    }

    void set_game_version_number(string new_game_version_number) noexcept
    {
        game_version_number = std::move(new_game_version_number);
    }

    const std::string &get_game_version_information() const noexcept
    {
        return game_version_information;
    }

    void set_game_version_information(string new_game_version_information) noexcept
    {
        game_version_information = std::move(new_game_version_information);
    }

    const std::string &get_cod2_master_server_ip_address() const noexcept
    {
        return cod2_master_server_ip_address;
    }

    void set_cod2_master_server_ip_address(string new_cod2_master_server_ip_address) noexcept
    {
        cod2_master_server_ip_address = std::move(new_cod2_master_server_ip_address);
    }

    uint_least16_t get_cod2_master_server_port() const noexcept
    {
        return cod2_master_server_port;
    }

    void set_cod2_master_server_port(const int new_cod2_master_server_port) noexcept
    {
        cod2_master_server_port = static_cast<uint_least16_t>(new_cod2_master_server_port);
    }

    const std::string &get_ftp_download_site_ip_address() const noexcept
    {
        return ftp_download_site_ip_address;
    }

    void set_ftp_download_site_ip_address(std::string new_ftp_download_site) noexcept
    {
        ftp_download_site_ip_address = std::move(new_ftp_download_site);
    }
    const std::string &get_ftp_download_folder_path() const noexcept
    {
        return ftp_download_folder_path;
    }

    void set_ftp_download_folder_path(std::string new_ftp_download_folder_path) noexcept
    {
        ftp_download_folder_path = std::move(new_ftp_download_folder_path);
    }

    const std::string &get_ftp_download_file_pattern() const noexcept
    {
        return ftp_download_file_pattern;
    }

    void set_ftp_download_file_pattern(std::string new_ftp_download_file_pattern) noexcept
    {
        ftp_download_file_pattern = std::move(new_ftp_download_file_pattern);
    }

    const std::string &get_image_files_path_to_md5() const noexcept
    {
        return image_files_path_to_md5;
    }

    void set_image_files_path_to_md5(std::string value) noexcept
    {
        image_files_path_to_md5 = std::move(value);
    }

    const std::string &get_plugins_geoIP_geo_dat_md5() const noexcept
    {
        return plugins_geoIP_geo_dat_md5;
    }

    void set_plugins_geoIP_geo_dat_md5(std::string new_plugins_geoIP_geo_dat_md5) noexcept
    {
        plugins_geoIP_geo_dat_md5 = std::move(new_plugins_geoIP_geo_dat_md5);
    }

    // images_data_md5
    const std::string &get_images_data_md5() const noexcept
    {
        return images_data_md5;
    }

    void set_images_data_md5(std::string new_images_data_md5) noexcept
    {
        images_data_md5 = std::move(new_images_data_md5);
    }

    const string &get_current_match_info() const noexcept
    {
        return current_match_info;
    }

    void set_current_match_info(string new_value) noexcept
    {
        current_match_info = std::move(new_value);
    }

    const string &get_full_map_name_color() const noexcept
    {
        return full_map_name_color;
    }

    void set_full_map_name_color(string new_value) noexcept
    {
        full_map_name_color = std::move(new_value);
    }

    const string &get_rcon_map_name_color() const noexcept
    {
        return rcon_map_name_color;
    }

    void set_rcon_map_name_color(string new_value) noexcept
    {
        rcon_map_name_color = std::move(new_value);
    }

    const string &get_full_gametype_name_color() const noexcept
    {
        return full_game_type_color;
    }

    void set_full_gametype_color(string new_value) noexcept
    {
        full_game_type_color = std::move(new_value);
    }

    const string &get_rcon_gametype_name_color() const noexcept
    {
        return rcon_game_type_color;
    }

    void set_rcon_gametype_color(string new_value) noexcept
    {
        rcon_game_type_color = std::move(new_value);
    }

    const string &get_online_players_count_color() const noexcept
    {
        return online_players_count_color;
    }

    void set_online_players_count_color(string new_value) noexcept
    {
        online_players_count_color = std::move(new_value);
    }

    const string &get_offline_players_count_color() const noexcept
    {
        return offline_players_count_color;
    }

    void set_offline_players_count_color(string new_value) noexcept
    {
        offline_players_count_color = std::move(new_value);
    }

    const string &get_border_line_color() const noexcept
    {
        return border_line_color;
    }

    void set_border_line_color(string new_value) noexcept
    {
        border_line_color = std::move(new_value);
    }

    const string &get_header_player_pid_color() const noexcept
    {
        return header_player_pid_color;
    }

    void set_header_player_pid_color(string new_value) noexcept
    {
        header_player_pid_color = std::move(new_value);
    }

    const string &get_data_player_pid_color() const noexcept
    {
        return data_player_pid_color;
    }

    void set_data_player_pid_color(string new_value) noexcept
    {
        data_player_pid_color = std::move(new_value);
    }

    const string &get_header_player_score_color() const noexcept
    {
        return header_player_score_color;
    }

    void set_header_player_score_color(string new_value) noexcept
    {
        header_player_score_color = std::move(new_value);
    }

    const string &get_data_player_score_color() const noexcept
    {
        return data_player_score_color;
    }

    void set_data_player_score_color(string new_value) noexcept
    {
        data_player_score_color = std::move(new_value);
    }

    const string &get_header_player_ping_color() const noexcept
    {
        return header_player_ping_color;
    }

    void set_header_player_ping_color(string new_value) noexcept
    {
        header_player_ping_color = std::move(new_value);
    }

    const string &get_data_player_ping_color() const noexcept
    {
        return data_player_ping_color;
    }

    void set_data_player_ping_color(string new_value) noexcept
    {
        data_player_ping_color = std::move(new_value);
    }

    const string &get_header_player_name_color() const noexcept
    {
        return header_player_name_color;
    }

    void set_header_player_name_color(string new_value) noexcept
    {
        header_player_name_color = std::move(new_value);
    }

    const string &get_header_player_ip_color() const noexcept
    {
        return header_player_ip_color;
    }

    void set_header_player_ip_color(string new_value) noexcept
    {
        header_player_ip_color = std::move(new_value);
    }

    const string &get_data_player_ip_color() const noexcept
    {
        return data_player_ip_color;
    }

    void set_data_player_ip_color(string new_value) noexcept
    {
        data_player_ip_color = std::move(new_value);
    }

    const string &get_header_player_geoinfo_color() const noexcept
    {
        return header_player_geoinfo_color;
    }

    void set_header_player_geoinfo_color(string new_value) noexcept
    {
        header_player_geoinfo_color = std::move(new_value);
    }

    const string &get_data_player_geoinfo_color() const noexcept
    {
        return data_player_geoinfo_color;
    }

    void set_data_player_geoinfo_color(string new_value) noexcept
    {
        data_player_geoinfo_color = std::move(new_value);
    }

    const string &get_odd_player_data_lines_bg_color() const noexcept
    {
        return odd_player_data_lines_bg_color;
    }

    void set_odd_player_data_lines_bg_color(string new_value) noexcept
    {
        odd_player_data_lines_bg_color = std::move(new_value);
    }

    const string &get_even_player_data_lines_bg_color() const noexcept
    {
        return even_player_data_lines_bg_color;
    }

    void set_even_player_data_lines_bg_color(string new_value) noexcept
    {
        even_player_data_lines_bg_color = std::move(new_value);
    }

    const string &get_odd_player_data_lines_fg_color() const noexcept
    {
        return odd_player_data_lines_fg_color;
    }

    void set_odd_player_data_lines_fg_color(string new_value) noexcept
    {
        odd_player_data_lines_fg_color = std::move(new_value);
    }

    const string &get_even_player_data_lines_fg_color() const noexcept
    {
        return even_player_data_lines_fg_color;
    }

    void set_even_player_data_lines_fg_color(string new_value) noexcept
    {
        even_player_data_lines_fg_color = std::move(new_value);
    }

    inline const std::string &get_server_message() const noexcept
    {
        return server_message;
    }

    inline void set_server_message(std::string new_value) noexcept
    {
        server_message = std::move(new_value);
    }

    inline const std::string &get_last_date_and_time_information() const noexcept
    {
        return last_date_and_time_information;
    }

    inline void set_last_date_and_time_information(std::string new_value) noexcept
    {
        last_date_and_time_information = std::move(new_value);
    }

    void set_check_for_banned_players_time_period(size_t new_value) noexcept
    {
        if (new_value < 5u || new_value > 60u)
            new_value = 5u;

        check_for_banned_players_time_period = new_value;
    }

    size_t get_check_for_banned_players_time_period() const noexcept
    {
        return check_for_banned_players_time_period;
    }

    unordered_map<string, size_t> &get_ip_address_frequency() noexcept
    {
        return ip_address_frequency;
    }

    bool open_log_file(const char *file_path)
    {
        if (log_file.is_open())
        {
            log_file.flush();
            log_file.close();
        }

        log_file.open(file_path, std::ios::app);
        is_log_file_open = !log_file.fail();
        return is_log_file_open;
    }

    void log_message(const std::string &message)
    {
        if (is_log_file_open)
        {
            log_file << message << std::flush;
        }
    }

    bool is_command_queue_empty() noexcept
    {
        std::lock_guard lg{command_queue_mutex};
        return command_queue.empty();
    }

    inline void add_command_to_queue(std::vector<std::string> cmd, const command_type cmd_type,
                                     const bool wait_for_reply)
    {
        {
            std::lock_guard lg{command_queue_mutex};
            command_queue.emplace(std::move(cmd), cmd_type, wait_for_reply);
        }
        // std::unique_lock ul{ command_mutex };
        command_queue_cv.notify_one();
    }

    inline command_t get_command_from_queue()
    {
        std::lock_guard lg{command_queue_mutex};
        command_t next_command = command_queue.front();
        command_queue.pop();
        return next_command;
    }

    inline void process_queue_command(command_t cmd)
    {
        std::lock_guard lg{command_queue_mutex};
        if (cmd.type == command_type::rcon)
        {
            process_rcon_command(cmd.command);
        }
        else if (cmd.type == command_type::user)
        {
            process_user_command(cmd.command);
        }
    }

    bool is_message_queue_empty() noexcept
    {
        std::lock_guard lg{message_queue_mutex};
        return message_queue.empty();
    }

    inline void add_message_to_queue(message_t message)
    {
        std::lock_guard lg{message_queue_mutex};
        message_queue.emplace(std::move(message));
    }

    inline message_t get_message_from_queue()
    {
        std::lock_guard lg{message_queue_mutex};
        auto next_message = std::move(message_queue.front());
        message_queue.pop();
        return next_message;
    }

    bool is_remote_message_queue_empty() noexcept
    {
        std::lock_guard lg{remote_message_queue_mutex};
        return remote_message_queue.empty();
    }

    inline void add_remote_message_to_queue(message_t message)
    {
        std::lock_guard lg{remote_message_queue_mutex};
        remote_message_queue.emplace(std::move(message));
    }

    inline message_t get_remote_message_from_queue()
    {
        std::lock_guard lg{remote_message_queue_mutex};
        auto next_message = std::move(remote_message_queue.front());
        remote_message_queue.pop();
        return next_message;
    }

    bool is_tinyrcon_message_queue_empty() noexcept
    {
        std::lock_guard lg{tinyrcon_messages_queue_mutex};
        return tinyrcon_messages_to_print.empty();
    }

    inline void add_tinyrcon_message_to_queue(print_message_t message)
    {
        std::lock_guard lg{tinyrcon_messages_queue_mutex};
        tinyrcon_messages_to_print.emplace(std::move(message));
    }

    inline print_message_t get_tinyrcon_message_from_queue()
    {
        std::lock_guard lg{tinyrcon_messages_queue_mutex};
        auto next_message = std::move(tinyrcon_messages_to_print.front());
        tinyrcon_messages_to_print.pop();
        return next_message;
    }

    const std::unordered_map<std::string, std::function<void(const std::vector<std::string> &)>> &get_command_handlers()
        const noexcept
    {
        return command_handlers;
    }

    void add_command_handler(std::vector<std::string> command_names,
                             std::function<void(const std::vector<std::string> &)> command_handler)
    {
        if (command_names.size() > 1)
        {
            for (size_t i{}; i < command_names.size() - 1; ++i)
            {
                command_handlers.emplace(std::move(command_names[i]), command_handler);
            }
        }
        command_handlers.emplace(std::move(command_names.back()), std::move(command_handler));
    }

    std::pair<bool, const std::function<void(const std::vector<std::string> &)> &> get_command_handler(
        const std::string &command_name) const noexcept
    {
        static std::function<void(const std::vector<std::string> &)> unknown_command_handler{
            [](const std::vector<std::string> &) {}};

        if (command_handlers.contains(command_name))
            return {true, command_handlers.at(command_name)};
        return {false, unknown_command_handler};
    }

    const std::unordered_map<std::string,
                             std::function<void(const std::string &, const time_t, const std::string &, const bool)>> &
    get_message_handlers() const noexcept
    {
        return message_handlers;
    }

    void add_message_handler(
        std::string message_name,
        std::function<void(const std::string &, const time_t, const std::string &, const bool)> message_handler)
    {
        message_handlers.emplace(std::move(message_name), std::move(message_handler));
    }

    const std::function<void(const std::string &, const time_t, const std::string &, const bool)> &get_message_handler(
        const std::string &message_name) const noexcept
    {
        static std::function<void(const std::string &, const time_t, const std::string &, const bool)>
            unknown_message_handler{[](const std::string &, const time_t, const std::string &, const bool) {}};

        if (message_handlers.contains(message_name))
            return message_handlers.at(message_name);
        return unknown_message_handler;
    }

    const std::unordered_map<std::string,
                             std::function<void(const std::string &, const time_t, const std::string &, const bool)>> &
    get_remote_message_handlers() const noexcept
    {
        return remote_message_handlers;
    }

    void add_remote_message_handler(
        std::string remote_message_name,
        std::function<void(const std::string &, const time_t, const std::string &, const bool)> remote_message_handler)
    {
        remote_message_handlers.emplace(std::move(remote_message_name), std::move(remote_message_handler));
    }

    const std::function<void(const std::string &, const time_t, const std::string &, const bool)> &
    get_remote_message_handler(const std::string &remote_message_name) const noexcept
    {
        static std::function<void(const std::string &, const time_t, const std::string &, const bool)>
            unknown_remote_message_handler{[](const std::string &, const time_t, const std::string &, const bool) {}};

        if (remote_message_handlers.contains(remote_message_name))
            return remote_message_handlers.at(remote_message_name);
        return unknown_remote_message_handler;
    }

    [[nodiscard]] map<string, string> &get_server_settings() noexcept
    {
        return server_settings;
    }
};
