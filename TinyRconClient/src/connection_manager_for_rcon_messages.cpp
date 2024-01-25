#include "tiny_rcon_client_application.h"
#include "connection_manager_for_rcon_messages.h"

using namespace std;
using namespace stl::helper;

extern tiny_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;

using namespace asio;

connection_manager_for_rcon_messages::connection_manager_for_rcon_messages() : udp_socket_for_messages{ udp_service_for_messages }
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

size_t connection_manager_for_rcon_messages::process_and_send_message(const std::string &command_name, const std::string &data, const bool is_show_in_messages, const std::string &remote_ip, const uint_least16_t remote_port, const bool is_call_msg_handler) const
{
  char send_buffer[512];
  const auto t_c = get_current_time_stamp();

  if (is_call_msg_handler) {
    const auto &msg_handler = main_app.get_message_handler(command_name);
    msg_handler(command_name, t_c, data, is_show_in_messages);
  }

  const string sender{ remove_disallowed_character_in_string(main_app.get_username()) };
  snprintf(send_buffer, std::size(send_buffer), R"(%s\%s\%s\%lld\%s\%s)", command_name.c_str(), sender.c_str(), main_app.get_current_game_server().get_rcon_password().c_str(), t_c, is_show_in_messages ? "true" : "false", data.c_str());
  const string outgoing_data{ send_buffer };
  const ip::udp::endpoint dst{ ip::address::from_string(remote_ip), remote_port };
  const size_t sent_bytes = udp_socket_for_messages.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), dst);
  main_app.add_to_next_uploaded_data_in_bytes(sent_bytes);
  return sent_bytes;
}


bool connection_manager_for_rcon_messages::wait_for_and_process_response_message(const std::string &remote_ip, const uint_least16_t remote_port) const
{
  static unordered_map<size_t, std::vector<std::pair<size_t, std::string>>> received_message_sequences;

  char incoming_data_buffer[receive_buffer_size];

  ip::udp::endpoint destination{ ip::address::from_string(remote_ip), remote_port };
  asio::error_code erc{};
  const size_t noOfReceivedBytes = udp_socket_for_messages.receive_from(
    buffer(incoming_data_buffer, receive_buffer_size), destination, 0, erc);
  if (erc)
    return false;

  if (noOfReceivedBytes > 0) {

    string message(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
    if (!message.starts_with("[[")) return false;
    const size_t header_start{ message.find("[[") };
    if (header_start == string::npos) return false;
    const size_t header_last{ message.find("]]", header_start + 2) };
    if (header_last == string::npos) return false;
    const string message_header{ message.substr(header_start + 2, header_last - (header_start + 2)) };
    message.erase(0, header_last + 2);
    auto msg_numbers = str_split(message_header, ":", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
    if (msg_numbers.size() != 3) return false;
    const size_t message_id = stoul(msg_numbers[0]);
    const size_t number_of_message_parts = stoul(msg_numbers[1]);
    const size_t current_message_number = stoul(msg_numbers[2]);
    if (received_message_sequences.contains(message_id)) {
      received_message_sequences[message_id].emplace_back(make_pair(current_message_number, message));
      std::sort(begin(received_message_sequences[message_id]), end(received_message_sequences[message_id]), [](const pair<size_t, string> &lp, const pair<size_t, string> &rp) {
        return lp.first < rp.first;
      });
    } else {
      received_message_sequences[message_id] = vector<std::pair<size_t, string>>{ make_pair(current_message_number, message) };
    }

    main_app.add_to_next_downloaded_data_in_bytes(noOfReceivedBytes);

    if (number_of_message_parts == received_message_sequences[message_id].size()) {

      message.clear();

      for (auto &&message_part : received_message_sequences[message_id]) {
        message.append(message_part.second);
      }

      received_message_sequences.erase(message_id);

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
      const auto &message_handler = main_app.get_remote_message_handler(message_handler_name);
      message_handler(sender, timestamp, data, is_show_in_messages);
      return true;
    }
  }

  return false;
}
