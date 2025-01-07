#pragma once
// #define ASIO_STANDALONE
#include <asio.hpp>
#include <string>
#include "tiny_rcon_utility_functions.h"

using namespace asio;

class connection_manager_for_messages
{
  using rcv_timeout_option =
    asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>;
  inline static constexpr size_t receive_buffer_size{ 4096 };

public:
  connection_manager_for_messages();
  inline ~connection_manager_for_messages()
  {
    if (udp_socket_for_messages.is_open()) {
      udp_socket_for_messages.close();
    }
  };

  time_t &get_last_rcon_status_received() noexcept { return last_rcon_status_received;  }
  size_t process_and_send_message(const std::string &command_name, const std::string &data, const bool is_show_in_messages, const std::string &remote_ip, const uint_least16_t remote_port, const bool is_call_msg_handler = true) const;
  bool wait_for_and_process_response_message(const std::string &remote_ip, const uint_least16_t remote_port) const;


private:
  time_t last_rcon_status_received{};
  inline static std::size_t number_of_sent_messages{};
  inline static std::size_t number_of_receive_messages{};
  io_service udp_service_for_messages;
  mutable ip::udp::socket udp_socket_for_messages;
  
};
