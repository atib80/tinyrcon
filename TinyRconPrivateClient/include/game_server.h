#pragma once

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "player.h"

using std::map;
using std::unordered_map;
using std::string;
using std::unordered_set;
using std::vector;

class game_server
{
public:
  game_server() : players_data(64, player{})
  {
  }

  [[nodiscard]] const std::string &get_game_server_address() const noexcept
  {
    return game_server_address;
  }

  void set_game_server_address(string new_game_server_address) noexcept
  {
    game_server_address = std::move(new_game_server_address);
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

  void set_server_port(const uint_least16_t newServerPort)
  {
    port = newServerPort;
  }

  const std::string &get_server_name() const noexcept
  {
    return sv_hostname;
  }

  void set_server_name(string new_server_name)
  {
    string cleaned_server_name;
    cleaned_server_name.reserve(new_server_name.length());
    for (const auto ch : new_server_name) {
      if (isprint(ch))
        cleaned_server_name.push_back(ch);
    }
    sv_hostname = std::move(cleaned_server_name);
  }

  void
    set_game_name(string new_game_name) noexcept
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

  [[nodiscard]] const string &get_current_full_map_name() const noexcept
  {
    return current_full_map_name;
  }

  void set_current_full_map_name(string new_current_full_map_name) noexcept
  {
    current_full_map_name = std::move(new_current_full_map_name);
  }

  void set_next_map(string new_next_map) noexcept
  {
    next_map = std::move(new_next_map);
  }

  void set_game_version_number(string new_game_version_number) noexcept
  {
    game_version_number = std::move(new_game_version_number);
  }

  const std::string &get_game_version_number() const noexcept
  {
    return game_version_number;
  }

  void set_map_rotation(string new_map_rotation) noexcept
  {
    map_rotation = std::move(new_map_rotation);
  }

  void set_map_rotation_current(string new_map_rotation_current) noexcept
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

  void set_server_pid(string new_server_pid) noexcept
  {
    server_pid = std::move(new_server_pid);
  }

  const std::string &get_server_pid() const noexcept
  {
    return server_pid;
  }

  void set_online_and_max_players(string new_online_and_max_players) noexcept
  {
    online_and_max_players = std::move(new_online_and_max_players);
  }

  const std::string &get_online_and_max_players() const noexcept
  {
    return online_and_max_players;
  }

  void set_country_code(const char *new_country_code) noexcept
  {
    country_code = new_country_code;
  }

  const char *get_country_code() const noexcept
  {
    return country_code;
  }

  void set_max_number_of_players(const int new_max_number_of_players) noexcept
  {
    max_number_of_players = new_max_number_of_players;
  }

  int get_max_number_of_players() const noexcept
  {
    return max_number_of_players;
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

  /* [[nodiscard]] bool get_rcon_specified() const noexcept
   {
     return is_rcon_specified;
   }*/

  // void set_is_rcon_valid(const bool new_rcon_valid) noexcept
  //{
  //   is_rcon_valid = new_rcon_valid;
  // }

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

  bool get_is_voice_enabled() const noexcept
  {
    return is_voice_enabled;
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

  int get_max_private_clients() const noexcept
  {
    return max_private_clients;
  }

  void set_max_private_clients(const int new_max_private_clients) noexcept
  {
    max_private_clients = new_max_private_clients;
  }


  size_t get_minimum_number_of_connections_from_same_ip_for_automatic_ban() const noexcept
  {
    return minimum_number_of_connections_from_same_ip_for_automatic_ban;
  }

  void set_minimum_number_of_connections_from_same_ip_for_automatic_ban(size_t new_value) noexcept
  {
    new_value = std::max<size_t>(5, new_value);
    minimum_number_of_connections_from_same_ip_for_automatic_ban = new_value;
  }

  size_t get_maximum_number_of_warnings_for_automatic_kick() const noexcept
  {
    return maximum_number_of_warnings_for_automatic_kick;
  }

  void set_maximum_number_of_warnings_for_automatic_kick(size_t new_value) noexcept
  {
    if (!(new_value >= 1 && new_value <= 5))
      new_value = 3;
    maximum_number_of_warnings_for_automatic_kick = new_value;
  }

  const std::vector<player> &get_players_data() const noexcept
  {
    return players_data;
  }

  std::vector<player> &get_players_data() noexcept
  {
    return players_data;
  }

  player &get_player_data(const int pid) noexcept
  {
    static player default_player_data{};

    for (auto &pd : this->players_data) {
      if (pid == pd.pid)
        return pd;
    }

    return default_player_data;
  }

  // vector<player> &get_temp_banned_players_data()
  //{
  //   return temp_banned_ip_addresses_vector;
  // }

  // bool get_temp_banned_player_data_for_ip_address(const std::string &ip, player *tb_player)
  //{
  //   if (nullptr == tb_player)
  //     return false;

  //  for (auto &tbp : temp_banned_ip_addresses_vector) {
  //    if (ip == tbp.ip_address) {
  //      *tb_player = tbp;
  //      return true;
  //    }
  //  }

  //  return false;
  //}

  // vector<player> &get_banned_ip_addresses_vector()
  //{
  //   return banned_ip_addresses_vector;
  // }

  // bool get_banned_player_data_for_ip_address(const std::string &ip, player *banned_player)
  //{
  //   if (nullptr == banned_player)
  //     return false;

  //  for (auto &bp : banned_ip_addresses_vector) {
  //    if (ip == bp.ip_address) {
  //      *banned_player = bp;
  //      return true;
  //    }
  //  }

  //  return false;
  //}

  // bool get_banned_player_data_for_ip_address_range(const std::string &ip, player *banned_player)
  //{
  //   if (nullptr == banned_player)
  //     return false;

  //  const string narrow_ip_address_range{ get_narrow_ip_address_range_for_specified_ip_address(ip) };
  //  const string wide_ip_address_range{ get_wide_ip_address_range_for_specified_ip_address(ip) };

  //  for (auto &bp : banned_ip_address_ranges_vector) {
  //    if (narrow_ip_address_range == bp.ip_address || wide_ip_address_range == bp.ip_address) {
  //      *banned_player = bp;
  //      return true;
  //    }
  //  }

  //  return false;
  //}

  // unordered_map<int, player> &get_warned_players_data()
  //{
  //   return warned_players_data;
  // }

  // void set_warned_players_data(unordered_map<int, player> new_warned_players_data)
  //{
  //   warned_players_data = std::move(new_warned_players_data);
  // }

  // unordered_map<string, player> &get_temp_banned_ip_addresses_map()
  //{
  //   return temp_banned_ip_addresses_map;
  // }

  // bool add_ip_address_to_temp_banned_ip_addresses(
  //   const string &new_ip_address,
  //   const player &temp_banned_player_data)
  //{

  //  // std::lock_guard lg{ protect_player_data };
  //  if (temp_banned_ip_addresses_map.contains(new_ip_address))
  //    return false;
  //  temp_banned_ip_addresses_map.emplace(new_ip_address, temp_banned_player_data);
  //  temp_banned_ip_addresses_vector.emplace_back(temp_banned_player_data);
  //  return true;
  //}

  // bool remove_ip_address_from_temp_banned_ip_addresses(
  //   const string &new_ip_address)
  //{
  //   // std::lock_guard lg{ protect_player_data };
  //   if (!temp_banned_ip_addresses_map.contains(new_ip_address))
  //     return false;
  //   temp_banned_ip_addresses_map.erase(new_ip_address);
  //   temp_banned_ip_addresses_vector.erase(find_if(cbegin(temp_banned_ip_addresses_vector), cend(temp_banned_ip_addresses_vector), [&new_ip_address](const player &pd) { return new_ip_address == pd.ip_address; }), cend(temp_banned_ip_addresses_vector));
  //   return true;
  // }

  // unordered_map<string, player> &get_banned_ip_addresses_map()
  //{
  //   return banned_ip_addresses_map;
  // }

  // bool add_ip_address_to_banned_ip_addresses(
  //   const string &new_ip_address,
  //   const player &banned_player_data)
  //{
  //   // std::lock_guard lg{ protect_player_data };
  //   if (banned_ip_addresses_map.contains(new_ip_address))
  //     return false;
  //   banned_ip_addresses_map.emplace(new_ip_address, banned_player_data);
  //   banned_ip_addresses_vector.emplace_back(banned_player_data);
  //   return true;
  // }

  // bool remove_ip_address_from_banned_ip_addresses(
  //   const string &new_ip_address)
  //{
  //   // std::lock_guard lg{ protect_player_data };
  //   if (!banned_ip_addresses_map.contains(new_ip_address))
  //     return false;
  //   banned_ip_addresses_map.erase(new_ip_address);
  //   banned_ip_addresses_vector.erase(find_if(cbegin(banned_ip_addresses_vector), cend(banned_ip_addresses_vector), [&new_ip_address](const player &pd) { return new_ip_address == pd.ip_address; }), cend(banned_ip_addresses_vector));
  //   return true;
  // }

  // vector<player> &get_banned_ip_address_ranges_vector()
  //{
  //   return banned_ip_address_ranges_vector;
  // }

  // unordered_map<string, player> &get_banned_ip_address_ranges_map()
  //{
  //   return banned_ip_address_ranges_map;
  // }

  // bool add_ip_address_range_to_banned_ip_address_ranges(
  //   const string &new_ip_address_range,
  //   const player &banned_player_data)
  //{
  //   // std::lock_guard lg{ protect_player_data };
  //   if (banned_ip_address_ranges_map.contains(new_ip_address_range))
  //     return false;
  //   banned_ip_address_ranges_map.emplace(new_ip_address_range, banned_player_data);
  //   banned_ip_address_ranges_vector.emplace_back(banned_player_data);
  //   return true;
  // }

  // bool remove_ip_address_range_from_banned_ip_address_ranges(
  //   const string &new_ip_address_range)
  //{
  //   // std::lock_guard lg{ protect_player_data };
  //   if (!banned_ip_address_ranges_map.contains(new_ip_address_range))
  //     return false;
  //   banned_ip_address_ranges_map.erase(new_ip_address_range);
  //   banned_ip_address_ranges_vector.erase(find_if(cbegin(banned_ip_address_ranges_vector), cend(banned_ip_address_ranges_vector), [&new_ip_address_range](const player &pd) { return new_ip_address_range == pd.ip_address; }), cend(banned_ip_address_ranges_vector));
  //   return true;
  // }

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

  // std::set<std::string> &get_banned_cities_set()
  //{
  //   return banned_cities;
  // }

  // std::set<std::string> &get_banned_countries_set()
  //{
  //   return banned_countries;
  // }

  // std::set<string> &get_protected_ip_addresses()
  //{
  //   return protected_ip_addresses_set;
  // }

  // std::set<string> &get_protected_ip_address_ranges()
  //{
  //   return protected_ip_address_ranges_set;
  // }

  // std::set<string> &get_protected_cities()
  //{
  //   return protected_cities_set;
  // }

  // std::set<string> &get_protected_countries()
  //{
  //   return protected_countries_set;
  // }

  // vector<player> &get_removed_temp_banned_players_vector()
  //{
  //   return removed_temp_banned_players_data;
  // }

  // std::unordered_map<std::string, player> &get_removed_temp_banned_players_map()
  //{
  //   return removed_temp_banned_ip_addresses_map;
  // }

  // vector<player> &get_removed_banned_ip_addresses_vector()
  //{
  //   return removed_banned_ip_addresses_vector;
  // }

  // std::unordered_map<std::string, player> &get_removed_banned_ip_addresses_map()
  //{
  //   return removed_banned_ip_addresses_map;
  // }


  // vector<player> &get_removed_banned_ip_address_ranges_vector()
  //{
  //   return removed_banned_ip_address_ranges_vector;
  // }

  // unordered_map<string, player> &get_removed_banned_ip_address_ranges_map()
  //{
  //   return removed_banned_ip_address_ranges_map;
  // }

  // std::set<std::string> &get_removed_banned_cities_set()
  //{
  //   return removed_banned_cities;
  // }

  // std::set<std::string> &get_removed_banned_countries_set()
  //{
  //   return removed_banned_countries;
  // }


private:
  // unordered_map<string, player> temp_banned_ip_addresses_map;
  // unordered_map<string, player> banned_ip_addresses_map;
  // unordered_map<string, player> banned_ip_address_ranges_map;
  // unordered_map<int, player> warned_players_data;
  // std::unordered_map<string, player> removed_temp_banned_ip_addresses_map;
  // std::unordered_map<string, player> removed_banned_ip_addresses_map;
  // std::unordered_map<string, player> removed_banned_ip_address_ranges_map;
  // std::set<std::string> banned_cities;
  // std::set<std::string> banned_countries;
  // std::set<std::string> removed_banned_cities;
  // std::set<std::string> removed_banned_countries;
  // std::set<string> protected_ip_addresses_set;
  // std::set<string> protected_ip_address_ranges_set;
  // std::set<std::string> protected_cities_set;
  // std::set<std::string> protected_countries_set;
  // std::vector<player> removed_temp_banned_players_data;
  // std::vector<player> removed_banned_ip_addresses_vector;
  // std::vector<player> removed_banned_ip_address_ranges_vector;
  vector<player> players_data;
  // vector<player> temp_banned_ip_addresses_vector;
  // vector<player> banned_ip_addresses_vector;
  // vector<player> banned_ip_address_ranges_vector;
  string short_version{ "1.0" };
  string game_server_address;
  string ip_address{ "185.158.113.146" };
  string sv_hostname{ "CoD2 CTF" };
  string game_name{ "Call of Duty 2" };
  string rcon_password{
    "abc123"
  };
  string private_slot_password{
    "abc123"
  };
  string game_version_number;
  string map_rotation;
  string map_rotation_current;
  string next_map;
  string current_map;
  string current_full_map_name;
  string current_game_type;
  string game_mod_name;
  // string serverInfo;
  string server_pid;
  string online_and_max_players;

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
  size_t minimum_number_of_connections_from_same_ip_for_automatic_ban{ 12 };
  size_t maximum_number_of_warnings_for_automatic_kick{ 2 };
  uint_least16_t port{ 28995 };
  const char *country_code{};
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
};
