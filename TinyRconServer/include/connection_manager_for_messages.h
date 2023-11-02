#pragma once
// #define ASIO_STANDALONE
#include <asio.hpp>
#include <string>
#include "tiny_rcon_client_user.h"
#include "stl_helper_functions.hpp"

using namespace asio;
using ip::tcp;

struct geoip_data
{
  unsigned long lower_ip_bound;
  unsigned long upper_ip_bound;
  char country_code[4];
  char country_name[35];
  char region[35];
  char city[35];

  geoip_data() = default;

  geoip_data(const unsigned long lib, const unsigned long uib, const char *code, const char *country, const char *reg, const char *ci) : lower_ip_bound{ lib }, upper_ip_bound{ uib }
  {

    const size_t no_of_chars_for_country_code = std::min<size_t>(3U, stl::helper::len(code));
    memcpy(country_code, code, no_of_chars_for_country_code);
    country_code[no_of_chars_for_country_code] = 0;

    const size_t no_of_chars_for_country_name = std::min<size_t>(34U, stl::helper::len(country));
    memcpy(country_name, country, no_of_chars_for_country_name);
    country_name[no_of_chars_for_country_name] = 0;

    const size_t no_of_chars_for_region = std::min<size_t>(34U, stl::helper::len(reg));
    memcpy(region, reg, no_of_chars_for_region);
    region[no_of_chars_for_region] = 0;

    const size_t no_of_chars_for_city = std::min<size_t>(34U, stl::helper::len(ci));
    memcpy(city, ci, no_of_chars_for_city);
    city[no_of_chars_for_city] = 0;
  }

  constexpr const char *get_country_code() const
  {
    return country_code;
  }

  constexpr const char *get_country_name() const
  {
    return country_name;
  }
  constexpr const char *get_region() const
  {
    return region;
  }

  constexpr const char *get_city() const
  {
    return city;
  }
};

struct tiny_rcon_client_user;

class connection_manager_for_messages
{
  using rcv_timeout_option =
    asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>;
  inline static constexpr size_t receive_buffer_size{ 2048 };

public:
  connection_manager_for_messages();

  inline ~connection_manager_for_messages()
  {
    if (udp_socket_for_messages.is_open()) {
      udp_socket_for_messages.close();
    }
  };

  size_t process_and_send_message(const std::string &command_name, const std::string &data, const bool is_show_in_messages, const std::shared_ptr<tiny_rcon_client_user> &user) const;
  bool wait_for_and_process_response_message();

  inline std::vector<geoip_data> &get_geoip_data() noexcept { return geoip_db; }


private:
  inline static std::size_t number_of_sent_messages{};
  inline static std::size_t number_of_receive_messages{};
  io_service udp_service_for_messages;
  mutable ip::udp::socket udp_socket_for_messages;
  std::vector<geoip_data> geoip_db;
};
