#pragma once
// #define ASIO_STANDALONE
#include <asio.hpp>
#include <string>
#include "tiny_rcon_client_user.h"
#include "stl_helper_functions.hpp"

using namespace asio;

struct tiny_rcon_client_user;

class connection_manager_for_remote_messages
{
  using rcv_timeout_option =
    asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>;
  inline static constexpr size_t receive_buffer_size{ 2048 };

public:
  connection_manager_for_remote_messages();

  inline ~connection_manager_for_remote_messages()
  {
    if (udp_socket_for_messages.is_open()) {
      udp_socket_for_messages.close();
    }
  };

  size_t process_and_send_message(const std::string &command_name, const std::string &data, const bool is_show_in_messages, const std::shared_ptr<tiny_rcon_client_user> &user) const;
  bool wait_for_and_process_response_message();

private:
  inline static std::size_t number_of_sent_messages{};
  inline static std::size_t number_of_receive_messages{};
  io_service udp_service_for_messages;
  mutable ip::udp::socket udp_socket_for_messages;
};