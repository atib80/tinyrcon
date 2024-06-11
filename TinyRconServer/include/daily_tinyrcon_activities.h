#pragma once

#include <format>
#include <sstream>
#include <string>

class tiny_rcon_server_application;

class tinyrcon_activities_stats
{
    size_t no_of_warnings{};
    size_t no_of_autokicks{};
    size_t no_of_kicks{};
    size_t no_of_tempbans{};
    size_t no_of_guid_bans{};
    size_t no_of_ip_bans{};
    size_t no_of_ip_address_range_bans{};
    size_t no_of_city_bans{};
    size_t no_of_country_bans{};
    size_t no_of_protected_ip_addresses{};
    size_t no_of_protected_ip_address_ranges{};
    size_t no_of_protected_cities{};
    size_t no_of_protected_countries{};
    size_t no_of_map_restarts{};
    size_t no_of_map_changes{};
    time_t start_time{};
    // std::string last_warn_message;
    // std::string last_kick_message;
    // std::string last_tempban_message;
    // std::string last_guid_ban_message;
    // std::string last_ip_ban_message;
    // std::string last_ip_address_range_ban_message;
    // std::string last_city_ban_message;
    // std::string last_country_ban_message;
    // std::string last_protected_ip_address_message;
    // std::string last_protected_ip_address_range_message;
    // std::string last_protected_city_message;
    // std::string last_protected_country_message;
    // std::string last_map_restart_message;
    // std::string last_map_change_message;

  public:
    time_t get_start_time() const noexcept
    {
        return start_time;
    }

    void set_start_time(const time_t new_start_time) noexcept
    {
        start_time = new_start_time;
    }
    size_t &get_no_of_warnings() noexcept
    {
        return no_of_warnings;
    }

    size_t &get_no_of_kicks() noexcept
    {
        return no_of_kicks;
    }

    size_t &get_no_of_autokicks() noexcept
    {
        return no_of_autokicks;
    }

    size_t &get_no_of_tempbans() noexcept
    {
        return no_of_tempbans;
    }

    size_t &get_no_of_guid_bans() noexcept
    {
        return no_of_guid_bans;
    }

    size_t &get_no_of_ip_bans() noexcept
    {
        return no_of_ip_bans;
    }

    size_t &get_no_of_ip_address_range_bans() noexcept
    {
        return no_of_ip_address_range_bans;
    }

    size_t &get_no_of_city_bans() noexcept
    {
        return no_of_city_bans;
    }

    size_t &get_no_of_country_bans() noexcept
    {
        return no_of_country_bans;
    }

    size_t &get_no_of_protected_ip_addresses() noexcept
    {
        return no_of_protected_ip_addresses;
    }

    size_t &get_no_of_protected_ip_address_ranges() noexcept
    {
        return no_of_protected_ip_address_ranges;
    }

    size_t &get_no_of_protected_cities() noexcept
    {
        return no_of_protected_cities;
    }

    size_t &get_no_of_protected_countries() noexcept
    {
        return no_of_protected_countries;
    }

    size_t &get_no_of_map_restarts() noexcept
    {
        return no_of_map_restarts;
    }

    size_t &get_no_of_map_changes() noexcept
    {
        return no_of_map_changes;
    }

    /*std::string get_welcome_message(tiny_rcon_server_application& main_app,
    const std::string &user_name) const
    {
      const size_t no_of_ip_bans =
    main_app.get_game_server().get_banned_ip_addresses_map().size(); const size_t
    no_of_ip_address_range_bans =
    main_app.get_game_server().get_banned_ip_address_ranges_map().size(); const
    size_t no_of_city_bans =
    main_app.get_game_server().get_banned_cities_set().size(); const size_t
    no_of_country_bans =
    main_app.get_game_server().get_banned_countries_set().size(); const size_t
    no_of_protected_ip_addresses =
    main_app.get_game_server().get_protected_ip_addresses().size(); const size_t
    no_of_protected_ip_address_ranges =
    main_app.get_game_server().get_protected_ip_address_ranges().size(); const
    size_t no_of_protected_cities =
    main_app.get_game_server().get_protected_cities().size(); const size_t
    no_of_protected_countries =
    main_app.get_game_server().get_protected_countries().size();

      std::ostringstream oss;
      oss << std::format("^5Hi, welcome back, ^1admin ^7{}\n", user_name);
      oss << std::format("^5Number of logged ^1auto-kicks ^5so far is ^3{}.\n",
    no_of_autokicks); oss << std::format("^5Admins have so far...\n   ^1warned
    ^3{} ^5{},\n   ^1kicked ^3{} ^5{},\n   ^1temporarily banned ^3{} ^5{},\n",
    no_of_warnings, no_of_warnings != 1 ? "players" : "player", no_of_kicks,
    no_of_kicks != 1 ? "players" : "player", no_of_tempbans, no_of_tempbans != 1 ?
    "players" : "player"); oss << std::format("   ^1permanently banned GUID {}
    ^5of ^3{} ^5{},\n   ^1banned IP {} ^5of ^3{} ^5{},\n   ^1banned IP address
    range(s) ^5of ^3{} ^5{},\n", no_of_guid_bans != 1 ? "keys" : "key",
    no_of_guid_bans, no_of_guid_bans != 1 ? "players" : "player", no_of_ip_bans !=
    1 ? "addresses" : "address", no_of_ip_bans, no_of_ip_bans != 1 ? "players" :
    "player", no_of_ip_address_range_bans, no_of_ip_address_range_bans != 1 ?
    "players" : "player"); oss << std::format("   ^1banned {} ^5of ^3{} ^5{},\n
    ^1banned {} ^5of ^3{} ^5{},\n   ^1protected IP {} ^5of ^3{} ^5{},\n",
    no_of_city_bans != 1 ? "cities" : "city", no_of_city_bans, no_of_city_bans !=
    1 ? "players" : "player", no_of_country_bans != 1 ? "countries" : "country",
    no_of_country_bans, no_of_country_bans != 1 ? "players" : "player",
    no_of_protected_ip_addresses != 1 ? "addresses" : "address",
    no_of_protected_ip_addresses, no_of_protected_ip_addresses != 1 ? "players" :
    "player"); oss << std::format("   ^1protected IP address {} ^5of ^3{} ^5{},\n
    ^1protected {} ^5of ^3{} ^5{},\n^1protected {} ^5of ^3{} ^5{},\n",
    no_of_protected_ip_address_ranges != 1 ? "ranges" : "range",
    no_of_protected_ip_address_ranges, no_of_protected_ip_address_ranges != 1 ?
    "players" : "player", no_of_protected_cities != 1 ? "cities" : "city",
    no_of_protected_cities, no_of_protected_cities != 1 ? "players" : "player",
    no_of_protected_countries != 1 ? "countries" : "country",
    no_of_protected_countries, no_of_protected_countries != 1 ? "players" :
    "player"); oss << std::format("^5Admins executed ^3{} ^1map restarts ^5and
    ^3{} ^1map changes^5.\n", no_of_map_restarts, no_of_map_changes); return
    oss.str();
    }*/
};
