#include "connection_manager_for_messages.h"
#include "tiny_rcon_utility_functions.h"
#include "tiny_rcon_server_application.h"
#include "stl_helper_functions.hpp"
#include "json_parser.hpp"
#include "tiny_rcon_server_application.h"
#include "connection_manager_for_messages.h"
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
    asio::ip::udp::endpoint local_endpoint{ asio::ip::address::from_string("192.168.1.15"), 27015 };
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

    const string outgoing_data{ format("{}\\^5Tiny^6Rcon ^5server\\{}\\{}\\{}", command_name, t_c, is_show_in_messages ? "true" : "false", data) };

    const size_t sent_bytes = udp_socket_for_messages.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), user->remote_endpoint);
    return sent_bytes;
  }

  return 0;
}


bool connection_manager_for_messages::wait_for_and_process_response_message()
{
  char incoming_data_buffer[2048]{};
  size_t noOfReceivedBytes{};

  ip::udp::endpoint remote_endpoint;
  asio::error_code erc{};
  noOfReceivedBytes = udp_socket_for_messages.receive_from(
    buffer(incoming_data_buffer, receive_buffer_size), remote_endpoint, 0, erc);
  if (erc) {
    /*print_colored_text(app_handles.hwnd_re_messages_data, "^1Error reading data from socket!^5\n");
    const string error_message{ format("^1Error: ^3{}\n", erc.message()) };
    print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());*/
    return false;
  }

  string message(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
  trim_in_place(message);

  if (noOfReceivedBytes > 0 && '{' != message.front() && '}' != message.back()) {
    std::vector<string> parts;
    size_t count_of_key_values{}, start{};
    for (size_t next{ string::npos }; (next = message.find('\\', start)) != string::npos; start = next + 1) {
      parts.emplace_back(cbegin(message) + start, cbegin(message) + next);
      ++count_of_key_values;
      if (5 == count_of_key_values) break;
    }

    if (count_of_key_values < 5)
      return false;

    start = message.find('\\', start);
    if (string::npos == start)
      return false;

    const string data{ message.substr(start + 1) };
    for (auto &part : parts)
      trim_in_place(part);

    int number;
    if (!is_valid_decimal_whole_number(parts[3], number) || (parts[4] != "true" && parts[4] != "false"))
      return false;

    if (main_app.get_game_server().get_rcon_password() == parts[2]) {
      const string message_handler_name{ std::move(parts[0]) };
      const string sender{ std::move(parts[1]) };
      const time_t timestamp{ stoll(parts[3]) };
      const bool is_show_in_messages{ parts[4] == "true" };
      const auto &user = main_app.get_user_for_name(sender);
      error_code erc2{};
      user->ip_address = remote_endpoint.address().to_v4().to_string(erc2);
      if (erc2) {
        const string error_msg{ format("^3Error getting admin's IP address information from their remote endpoint!\n^1Error: {}", erc2.message()) };
        print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str());
      }
      user->remote_endpoint = std::move(remote_endpoint);
      unsigned long guid{};
      if (check_ip_address_validity(user->ip_address, guid)) {
        player_data pd{};
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
      message_handler(sender, timestamp, data, is_show_in_messages);
    }

    return true;
  }

  return false;
}
