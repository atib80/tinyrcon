#pragma once
#define ASIO_STANDALONE
#include <asio.hpp>
#include <string>
#include "tiny_rcon_utility_functions.h"

using namespace asio;

class connection_manager
{
  using rcv_timeout_option =
    asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>;
  inline static constexpr size_t receive_buffer_size{ 2048 };

public:
  connection_manager();
  inline ~connection_manager()
  {
    if (udp_socket.is_open()) {
      udp_socket.close();
    }
  };

  void prepare_rcon_command(char *send_buffer, const std::size_t send_buffer_capacity, const char *command_to_send, const char *rcon_password) const noexcept;

  size_t send_rcon_command(const std::string &send_buffer, const char *remote_ip, const uint_least16_t remote_port) const;

  size_t receive_data_from_server(const char *remote_ip, const uint_least16_t remote_port, std::string &reply_buffer, const bool is_process_reply = true) const;

  void send_and_receive_rcon_data(const char *command_to_send, std::string &reply_buffer, const char *remote_ip, const uint_least16_t remote_port, const char *rcon_password, const bool is_wait_for_reply = true, const bool is_process_reply = true) const;

  inline std::vector<geoip_data> &get_geoip_data() noexcept { return geoip_db; }

private:
  inline static std::size_t number_of_sent_rcon_commands{};
  std::vector<geoip_data> geoip_db;
  io_service udp_service;
  mutable ip::udp::socket udp_socket;
  mutable std::recursive_mutex rcon_mutex;
};
