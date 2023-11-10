#include "json_parser.hpp"
#include "tiny_rcon_client_application.h"
#include "connection_manager_for_messages.h"

using namespace std;
using namespace stl::helper;

extern tiny_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;

using namespace asio;

connection_manager_for_messages::connection_manager_for_messages() : udp_socket_for_messages{ udp_service_for_messages }
{
  try {
    if (udp_socket_for_messages.is_open()) {
      udp_socket_for_messages.close();
    }

    udp_socket_for_messages.open(ip::udp::v4());
    udp_socket_for_messages.set_option(rcv_timeout_option{ 700 });

  } catch (std::exception &ex) {
    show_error(app_handles.hwnd_main_window, ex.what(), 0);
  }
}

size_t connection_manager_for_messages::process_and_send_message(const std::string &command_name, const std::string &data, const bool is_show_in_messages, const std::string &remote_ip, const uint_least16_t remote_port, const bool is_call_msg_handler) const
{
  const auto t_c = get_current_time_stamp();

  if (is_call_msg_handler) {
    const auto &msg_handler = main_app.get_message_handler(command_name);
    msg_handler(command_name, t_c, data, is_show_in_messages);
  }

  const string sender{ remove_disallowed_character_in_string(main_app.get_username()) };
  const string outgoing_data{ format(R"({}\{}\{}\{}\{}\{})", command_name, sender, main_app.get_game_server().get_rcon_password(), t_c, is_show_in_messages ? "true" : "false", data) };

  const ip::udp::endpoint dst{ ip::address::from_string(remote_ip), remote_port };
  const size_t sent_bytes = udp_socket_for_messages.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), dst);
  return sent_bytes;
}


bool connection_manager_for_messages::wait_for_and_process_response_message(const std::string &remote_ip, const uint_least16_t remote_port) const
{
  char incoming_data_buffer[2048]{};
  size_t noOfReceivedBytes{};

  ip::udp::endpoint destination{ ip::address::from_string(remote_ip), remote_port };
  asio::error_code erc{};
  noOfReceivedBytes = udp_socket_for_messages.receive_from(
    buffer(incoming_data_buffer, receive_buffer_size), destination, 0, erc);
  if (erc)
    return false;

  const string sender_ip{ destination.address().to_v4().to_string() };  
  if (sender_ip != remote_ip) 
      return false;

  string message(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
  trim_in_place(message);

  if (noOfReceivedBytes > 0 && '{' != message.front() && '}' != message.back()) {

    std::vector<string> parts;
    size_t count_of_key_values{}, start{};
    for (size_t next{ string::npos }; (next = message.find('\\', start)) != string::npos; start = next + 1) {
      parts.emplace_back(cbegin(message) + start, cbegin(message) + next);
      ++count_of_key_values;
      if (4 == count_of_key_values) break;
    }

    if (count_of_key_values < 4)
      return false;

    start = message.find('\\', start);
    if (string::npos == start)
      return false;

    const string data{ message.substr(start + 1) };
    for (auto &part : parts)
      trim_in_place(part);

    int number;
    if (!is_valid_decimal_whole_number(parts[2], number) || (parts[3] != "true" && parts[3] != "false"))
      return false;

    const string message_handler_name{ std::move(parts[0]) };
    const string sender{ std::move(parts[1]) };
    const time_t timestamp{ stoll(parts[2]) };
    const bool is_show_in_messages{ parts[3] == "true" };
    const auto &message_handler = main_app.get_message_handler(message_handler_name);
    message_handler(sender, timestamp, data, is_show_in_messages);
    return true;
  }

  return false;
}
