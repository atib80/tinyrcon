#pragma once

#include <string>
#include <asio.hpp>

struct tiny_rcon_client_user
{
  bool is_admin{true};
  bool is_logged_in{};
  bool is_online{};
  uint_least16_t user_port{};
  size_t no_of_logins{};
  size_t no_of_warnings{};
  size_t no_of_kicks{};
  size_t no_of_tempbans{};
  size_t no_of_guidbans{};
  size_t no_of_ipbans{};
  size_t no_of_iprangebans{};
  size_t no_of_citybans{};
  size_t no_of_countrybans{};
  time_t last_login_time_stamp{};
  time_t last_logout_time_stamp{};
  const char *country_code{ "xy" };
  std::string user_name{"^1Admin"};
  std::string player_name;
  std::string ip_address;
  std::string geo_information;
  asio::ip::udp::endpoint remote_endpoint;
};
