#pragma once

#include <atomic>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using std::map;
using std::unordered_map;
using std::string;
using std::unordered_set;
using std::vector;

struct player_data
{
  char player_name[33];
  char guid_key[33];
  char banned_date_time[33];
  char ip_address[16];
  char ping[5];
  size_t warned_times{};
  time_t banned_start_time{};
  time_t ban_duration_in_hours{ 24 };
  const char *country_name{};
  const char *region{};
  const char *city{};
  const char *country_code{};
  int pid;
  int score{};
  std::string reason;
};

class alignas(size_t) game_server
{
public:
  game_server() : players_data(64, player_data{})
  {
  }

  [[nodiscard]] const std::string &get_server_ip_address() const noexcept
  {
    return ip_address;
  }

  void set_server_ip_address(string new_server_ip_address) noexcept
  {
    ip_address = std::move(new_server_ip_address);
  }

  [[nodiscard]] uint_least16_t get_server_port() const noexcept { return port; }

  void set_server_port(const uint_least16_t newServerPort) noexcept
  {
    port = newServerPort;
  }

  const std::string &get_server_name() const noexcept
  {
    return sv_hostname;
  }

  void set_server_name(string new_server_name) noexcept
  {
    sv_hostname = std::move(new_server_name);
  }

  void set_game_name(string new_game_name) noexcept
  {
    game_name = std::move(new_game_name);
  }

  [[nodiscard]] const string &get_game_name() const noexcept
  {
    return game_name;
  }

  [[nodiscard]] const string &get_current_game_type() const noexcept
  {
    return current_game_type;
  }

  void set_current_game_type(string new_current_game_type) noexcept
  {
    current_game_type = std::move(new_current_game_type);
  }

  [[nodiscard]] const string &get_current_map() const noexcept
  {
    return current_map;
  }

  void set_current_map(string new_current_map) noexcept
  {
    current_map = std::move(new_current_map);
  }

  void set_next_map(string new_next_map) noexcept
  {
    next_map = std::move(new_next_map);
  }

  void set_map_rotation(string new_map_rotation) noexcept
  {
    map_rotation = std::move(new_map_rotation);
  }

  void
    set_map_rotation_current(string new_map_rotation_current) noexcept
  {
    map_rotation_current = std::move(new_map_rotation_current);
  }

  void set_game_mod_name(string new_game_mod_name) noexcept
  {
    game_mod_name = std::move(new_game_mod_name);
  }

  void set_rcon_password(string new_rcon_password) noexcept
  {
    rcon_password = std::move(new_rcon_password);
  }

  [[nodiscard]] const string &get_rcon_password() const noexcept
  {
    return rcon_password;
  }

  [[nodiscard]] const string &get_private_slot_password() const noexcept
  {
    return private_slot_password;
  }

  void set_private_slot_password(string new_private_slot_password) noexcept
  {
    private_slot_password = std::move(new_private_slot_password);
  }

  void set_check_for_banned_players_time_period(size_t new_value) noexcept
  {
    if (new_value < 1 || new_value > 30) new_value = 5;

    this->check_for_banned_players_time_period = new_value;
  }

  size_t get_check_for_banned_players_time_period() const noexcept
  {
    return this->check_for_banned_players_time_period;
  }

  void set_max_number_of_players(const int new_max_number_of_players) noexcept
  {
    max_number_of_players = new_max_number_of_players;
  }

  void set_current_number_of_players(
    const int new_current_number_of_players) noexcept
  {
    current_number_of_players = new_current_number_of_players;
  }

  void set_min_ping(const int new_min_ping) noexcept
  {
    min_ping = new_min_ping;
  }

  void set_max_ping(const int new_max_ping) noexcept
  {
    max_ping = new_max_ping;
  }

  void set_protocol_info(const int new_protocol) noexcept
  {
    protocol = new_protocol;
  }

  void set_hw_info(const int new_hw) noexcept { hw = new_hw; }

  [[nodiscard]] bool get_rcon_specified() const noexcept
  {
    return is_rcon_specified;
  }

  void set_is_rcon_valid(const bool new_rcon_valid) noexcept
  {
    is_rcon_valid = new_rcon_valid;
  }

  void set_is_pure(const bool new_pure) noexcept { is_pure = new_pure; }

  void set_is_kill_cam_enabled(const bool new_kill_cam_enabled) noexcept
  {
    is_kill_cam_enabled = new_kill_cam_enabled;
  }

  void set_is_allow_anonymous_players(
    const bool new_is_allow_anonymous_players) noexcept
  {
    is_allow_anonymous_players = new_is_allow_anonymous_players;
  }

  void set_is_mod_enabled(const bool new_is_mod_enabled) noexcept
  {
    is_mod_enabled = new_is_mod_enabled;
  }

  void set_is_voice_enabled(const bool new_is_voice_enabled) noexcept
  {
    is_voice_enabled = new_is_voice_enabled;
  }

  void set_is_anti_lag_enabled(const bool new_is_anti_lag_enabled) noexcept
  {
    is_anti_lag_enabled = new_is_anti_lag_enabled;
  }

  void set_is_friendly_fire_enabled(
    const bool new_is_friendly_fire_enabled) noexcept
  {
    is_friendly_fire_enabled = new_is_friendly_fire_enabled;
  }

  void set_server_short_version(string new_short_version) noexcept
  {
    short_version = std::move(new_short_version);
  }

  void set_is_flood_protected(const bool new_is_flood_protected) noexcept
  {
    is_flood_protected = new_is_flood_protected;
  }

  void set_is_console_disabled(const bool new_is_console_disabled) noexcept
  {
    is_console_disabled = new_is_console_disabled;
  }

  void set_is_password_protected(
    const bool new_is_password_protected) noexcept
  {
    is_password_protected = new_is_password_protected;
  }

  void set_max_server_rate(const int new_server_max_rate) noexcept
  {
    max_server_rate = new_server_max_rate;
  }

  void set_max_private_clients(const int new_max_private_clients) noexcept
  {
    max_private_clients = new_max_private_clients;
  }

  //std::vector<player_data> &get_tempbanned_players_to_unban() noexcept
  //{
  //  return tempbanned_players_to_unban;
  //}

  std::vector<player_data> &get_players_data() noexcept
  {
    return players_data;
  }

  player_data &get_player_data(const int pid) noexcept
  {
    static player_data default_player_data{};

    for (auto &pd : this->players_data) {
      if (pid == pd.pid)
        return pd;
    }

    return default_player_data;
  }

  [[nodiscard]] map<string, string> &get_server_settings() noexcept
  {
    return server_settings;
  }

  vector<player_data> &get_temp_banned_players_data() noexcept
  {
    return tempbanned_players_data;
  }

  bool get_temp_banned_player_data_for_ip_address(const std::string &ip, player_data *tb_player)
  {
    if (nullptr == tb_player)
      return false;

    for (auto &tbp : tempbanned_players_data) {
      if (ip == tbp.ip_address) {
        *tb_player = tbp;
        return true;
      }
    }

    return false;
  }

  vector<player_data> &get_banned_players_data() noexcept
  {
    return banned_players_data;
  }

  bool get_banned_player_data_for_ip_address(const std::string &ip, player_data *banned_player)
  {
    if (nullptr == banned_player)
      return false;

    for (auto &bp : banned_players_data) {
      if (ip == bp.ip_address) {
        *banned_player = bp;
        return true;
      }
    }

    return false;
  }
  unordered_map<int, player_data> &get_warned_players_data() noexcept
  {
    return warned_players_data;
  }

  void set_warned_players_data(unordered_map<int, player_data> new_warned_players_data) noexcept
  {
    warned_players_data = std::move(new_warned_players_data);
  }

  const unordered_set<string> &get_set_of_temp_banned_ip_addresses() const noexcept
  {
    return temp_banned_ip_addresses;
  }

  bool add_ip_address_to_set_of_temp_banned_ip_addresses(
    const string &new_ip_address)
  {
    if (temp_banned_ip_addresses.count(new_ip_address) == 1U)
      return false;
    temp_banned_ip_addresses.emplace(new_ip_address);
    return true;
  }

  bool remove_ip_address_from_set_of_temp_banned_ip_addresses(
    const string &new_ip_address)
  {
    if (temp_banned_ip_addresses.count(new_ip_address) == 0)
      return false;
    temp_banned_ip_addresses.erase(new_ip_address);
    return true;
  }

  const unordered_set<string> &get_set_of_banned_ip_addresses() const noexcept
  {
    return banned_ip_addresses;
  }

  bool add_ip_address_to_set_of_banned_ip_addresses(
    const string &new_ip_address)
  {
    if (banned_ip_addresses.count(new_ip_address) == 1U)
      return false;
    banned_ip_addresses.emplace(new_ip_address);
    return true;
  }

  bool remove_ip_address_from_set_of_banned_ip_addresses(
    const string &new_ip_address)
  {
    if (banned_ip_addresses.count(new_ip_address) == 0)
      return false;
    banned_ip_addresses.erase(new_ip_address);
    return true;
  }

  unordered_map<string, size_t> &get_ip_address_frequency() noexcept
  {
    return ip_address_frequency;
  }

  size_t get_number_of_players() const noexcept
  {
    return number_of_players;
  }

  void set_number_of_players(const size_t new_value) noexcept
  {
    number_of_players = new_value;
  }

  size_t get_number_of_online_players() const noexcept
  {
    return number_of_players_online;
  }

  void set_number_of_online_players(const size_t new_value) noexcept
  {
    number_of_players_online = new_value;
  }

  size_t get_number_of_offline_players() const noexcept
  {
    return number_of_players_offline;
  }

  void set_number_of_offline_players(const size_t new_value) noexcept
  {
    number_of_players_offline = new_value;
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

private:
  map<string, string> server_settings;
  unordered_map<string, size_t> ip_address_frequency;
  unordered_set<string> temp_banned_ip_addresses;
  unordered_set<string> banned_ip_addresses;
  vector<player_data> players_data;
  vector<player_data> tempbanned_players_data;
  // vector<player_data> tempbanned_players_to_unban;
  vector<player_data> banned_players_data;
  unordered_map<int, player_data> warned_players_data;
  string short_version{ "1.0" };
  string ip_address{ "127.0.0.1" };
  string sv_hostname{ "CoDHost" };
  string game_name{ "unknown" };
  string rcon_password{
    "abc123"
  };
  string private_slot_password{
    "abc123"
  };
  string map_rotation;
  string map_rotation_current;
  string next_map;
  string current_map;
  string current_game_type;
  string game_mod_name;
  string serverInfo;
  string current_match_info{ "^3Map: {MAP_FULL_NAME} ^1({MAP_RCON_NAME}^1) ^3| Gametype: {GAMETYPE_FULL_NAME} ^3| Online/Offline players: {ONLINE_PLAYERS_COUNT}^3|{OFFLINE_PLAYERS_COUNT}" };
  string odd_player_data_lines_fg_color{ "^5" };
  string even_player_data_lines_fg_color{ "^3" };
  string odd_player_data_lines_bg_color{ "^0" };
  string even_player_data_lines_bg_color{ "^8" };
  string full_map_name_color{ "^2" };
  string rcon_map_name_color{ "^1" };
  string full_game_type_color{ "^2" };
  string rcon_game_type_color{ "^1" };
  string online_players_count_color{ "^2" };
  string offline_players_count_color{ "^1" };
  string border_line_color{ "^5" };
  string header_player_pid_color{ "^1" };
  string data_player_pid_color{ "^1" };
  string header_player_score_color{ "^3" };
  string data_player_score_color{ "^3" };
  string header_player_ping_color{ "^3" };
  string data_player_ping_color{ "^3" };
  string header_player_name_color{ "^3" };
  string header_player_ip_color{ "^3" };
  string data_player_ip_color{ "^3" };
  string header_player_geoinfo_color{ "^3" };
  string data_player_geoinfo_color{ "^3" };
  std::atomic<size_t> check_for_banned_players_time_period{ 5 };
  int max_number_of_players{ 64 };
  int max_private_clients{};
  int current_number_of_players{};
  int protocol{};
  int hw{};
  int max_ping{ 999 };
  int min_ping{};
  int max_server_rate{ 25000 };
  size_t number_of_players_online{};
  size_t number_of_players_offline{};
  size_t number_of_players{};
  uint_least16_t port{ 28960 };
  bool is_display_server{ true };
  bool is_rcon_specified{ true };
  bool is_rcon_valid{ true };
  bool is_pure{ true };
  bool is_kill_cam_enabled{ true };
  bool is_allow_anonymous_players{ true };
  bool is_mod_enabled{};
  bool is_voice_enabled{};
  bool is_anti_lag_enabled{};
  bool is_friendly_fire_enabled{};
  bool is_flood_protected{};
  bool is_console_disabled{};
  bool is_password_protected{};
  bool is_use_different_background_colors_for_even_and_odd_lines{ true };
  bool is_use_different_foreground_colors_for_even_and_odd_lines{ false };
};
