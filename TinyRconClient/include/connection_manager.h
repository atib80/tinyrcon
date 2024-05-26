#pragma once

#include <asio.hpp>
#include <string>

class game_server;
struct geoip_data;

using namespace asio;

class connection_manager
{
  using rcv_timeout_option =
    asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>;
  inline static constexpr size_t receive_buffer_size{ 4096 };

public:
  connection_manager();
  inline ~connection_manager()
  {
    if (udp_socket_for_rcon_commands.is_open()) {
      udp_socket_for_rcon_commands.close();
    }

    if (udp_socket_for_non_rcon_commands.is_open()) {
      udp_socket_for_non_rcon_commands.close();
    }
  };

  void prepare_rcon_command(char *send_buffer, const std::size_t send_buffer_capacity, const char *command_to_send, const char *rcon_password) const;

  void prepare_non_rcon_command(char *buffer, const std::size_t buffer_size, const char *command_to_send) const;

  size_t send_rcon_command(const std::string &send_buffer, const char *remote_ip, const uint_least16_t remote_port) const;

  size_t send_non_rcon_command(const std::string &send_buffer, const char *remote_ip, const uint_least16_t remote_port) const;

  size_t receive_rcon_reply_from_server(const char *remote_ip, const uint_least16_t remote_port, game_server &gs, std::string &reply_buffer, const bool is_process_reply = true) const;

  size_t process_rcon_status_messages(game_server &gs, const std::string &rcon_status_data) const;

  size_t receive_non_rcon_reply_from_server(const char *remote_ip, const uint_least16_t remote_port, game_server &gs, std::string &reply_buffer, const bool is_process_reply = true) const;

  void send_and_receive_rcon_data(const char *command_to_send, std::string &reply_buffer, const char *remote_ip, const uint_least16_t remote_port, const char *rcon_password, game_server &gs, const bool is_wait_for_reply = true, const bool is_process_reply = true) const;
  void send_and_receive_non_rcon_data(const char *command_to_send, std::string &reply_buffer, const char *remote_ip, const uint_least16_t remote_port, game_server &gs, const bool is_wait_for_reply = true, const bool is_process_reply = true) const;

  inline std::vector<geoip_data> &get_geoip_data() { return geoip_db; }

  inline time_t &get_last_rcon_status_received() { return last_rcon_status_received; }

private:
  time_t last_rcon_status_received{};
  inline static std::size_t number_of_sent_non_rcon_commands{};
  inline static std::size_t number_of_sent_rcon_commands{};
  inline static std::size_t rcon_status_sent_counter{};
  inline static const std::string unknown_rcon_password{ "abc123" };
  std::vector<geoip_data> geoip_db;
  io_service udp_service_for_rcon_commands;
  io_service udp_service_for_non_rcon_commands;
  mutable ip::udp::socket udp_socket_for_rcon_commands;
  mutable ip::udp::socket udp_socket_for_non_rcon_commands;
  mutable std::recursive_mutex rcon_mutex;
  mutable std::recursive_mutex non_rcon_mutex;
};
