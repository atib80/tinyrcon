#include "connection_manager_for_messages.h"
#include "tiny_rcon_utility_functions.h"
#include "tiny_rcon_server_application.h"
#include "stl_helper_functions.hpp"
#include <utility>

using namespace std;
using namespace stl::helper;

extern tiny_rcon_server_application main_app;
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
    asio::ip::udp::endpoint local_endpoint{ asio::ip::address::from_string(main_app.get_tiny_rcon_server_ip_address()), main_app.get_tiny_rcon_server_port() };
    udp_socket_for_messages.bind(local_endpoint);

  } catch (std::exception &ex) {
    show_error(app_handles.hwnd_main_window, ex.what(), 0);
  }
}

size_t connection_manager_for_messages::process_and_send_message(const std::string &command_name, const std::string &data, const bool is_show_in_messages, const std::shared_ptr<tiny_rcon_client_user> &user) const
{
  if (user->remote_endpoint != asio::ip::udp::endpoint{}) {
    const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();
    const auto t_c = std::chrono::system_clock::to_time_t(now);

    const string outgoing_data{ format("{}\\{}\\{}\\{}\\{}", command_name, user->user_name, t_c, is_show_in_messages ? "true" : "false", data) };

    const size_t sent_bytes = udp_socket_for_messages.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), user->remote_endpoint);
    return sent_bytes;
  }

  return 0;
}


bool connection_manager_for_messages::wait_for_and_process_response_message()
{
  char incoming_data_buffer[receive_buffer_size]{};
  size_t noOfReceivedBytes{};

  ip::udp::endpoint remote_endpoint;
  asio::error_code erc1{};
  noOfReceivedBytes = udp_socket_for_messages.receive_from(
    buffer(incoming_data_buffer, receive_buffer_size), remote_endpoint, 0, erc1);
  if (erc1) {
    return false;
  }


  string message(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
  trim_in_place(message);

  string sender_ip{ remote_endpoint.address().to_v4().to_string() };

  if (noOfReceivedBytes > 0 && '{' != message.front() && '}' != message.back()) {
    auto parts = stl::helper::str_split(message, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    if (parts.size() < 6) {
      const string incorrectly_formatted_message_received{ format("^3Received an incorrectly formatted message from ^1IP address ({})^3!\n^3Contents of the message:'^1{}^3'\n", sender_ip, message) };
      print_colored_text(app_handles.hwnd_re_messages_data, incorrectly_formatted_message_received.c_str());
      return false;
    }

    const string message_contents{ str_join(cbegin(parts) + 5, cend(parts), '\\') };

    int number;
    if (!is_valid_decimal_whole_number(parts[3], number) || (parts[4] != "true" && parts[4] != "false")) {
      const string incorrectly_formatted_message_received{ format("^3Received an incorrectly formatted message!\n^7{} ^3(IP address: '^1{}^3' and rcon_password: '^1{}^3') sent the following command: '^1{}^3'\nContents of message:\n'^1{}^3'\n", parts[1], sender_ip, parts[2], parts[0], message_contents) };
      print_colored_text(app_handles.hwnd_re_messages_data, incorrectly_formatted_message_received.c_str());
      return false;
    }

    if (main_app.get_game_server().get_rcon_password() == parts[2]) {
      const string message_handler_name{ std::move(parts[0]) };
      const string sender{ std::move(parts[1]) };
      const time_t timestamp{ stoll(parts[3]) };
      const bool is_show_in_messages{ parts[4] == "true" };
      const auto &user = main_app.get_user_for_name(sender, sender_ip);
      user->ip_address = std::move(sender_ip);
      user->remote_endpoint = std::move(remote_endpoint);
      unsigned long guid{};
      if (check_ip_address_validity(user->ip_address, guid)) {
        player pd{};
        strcpy_s(pd.player_name, std::size(pd.player_name), sender.c_str());
        strcpy_s(pd.ip_address, std::size(pd.ip_address), user->ip_address.c_str());
        convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), user->ip_address, pd);
        user->geo_information = format("{}, {}", pd.country_name, pd.city);
        user->country_code = pd.country_code;
      } else {
        user->ip_address = "n/a";
        user->geo_information = "Unknown, Unknown";
        user->country_code = "xy";
      }
      const auto &message_handler = main_app.get_message_handler(message_handler_name);
      message_handler(sender, timestamp, message_contents, is_show_in_messages, user->ip_address);
      return true;
    } else {
      const string unathorized_message_received{ format("^3Received an unauthorized message!\n^7{} ^3(IP address: '^1{}^3' and rcon_password: '^1{}^3') sent the following command: '^1{}^3'\nContents of message:\n'^1{}^3'\n", parts[1], sender_ip, parts[2], parts[0], message_contents) };
      print_colored_text(app_handles.hwnd_re_messages_data, unathorized_message_received.c_str());
    }
  } else {
    const string incorrectly_formatted_message_received{ format("^3Received an incorrectly formatted message from ^1IP address ({})^3!\n^3Contents of the message:'^1{}^3'\n", sender_ip, message) };
    print_colored_text(app_handles.hwnd_re_messages_data, incorrectly_formatted_message_received.c_str());
  }

  return false;
}
