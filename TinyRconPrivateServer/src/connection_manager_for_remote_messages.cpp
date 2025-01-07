#include "connection_manager_for_remote_messages.h"
#include "tiny_rcon_utility_functions.h"
#include "tiny_rcon_client_application.h"
#include "stl_helper_functions.hpp"
#include <utility>

using namespace std;
using namespace stl::helper;

extern tiny_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;

using namespace asio;

connection_manager_for_remote_messages::connection_manager_for_remote_messages() : udp_socket_for_messages{ udp_service_for_messages }
{
  try {
    if (udp_socket_for_messages.is_open()) {
      udp_socket_for_messages.close();
    }

    udp_socket_for_messages.open(ip::udp::v4());
    udp_socket_for_messages.set_option(rcv_timeout_option{ 700 });
    asio::ip::udp::endpoint local_endpoint{ asio::ip::address::from_string("192.168.1.15"), 27017 };
    udp_socket_for_messages.bind(local_endpoint);

  } catch (std::exception &ex) {
    show_error(app_handles.hwnd_main_window, ex.what(), 0);
  }
}

size_t connection_manager_for_remote_messages::process_and_send_message(const std::string &command_name, const std::string &data, const bool is_show_in_messages, const std::shared_ptr<tiny_rcon_client_user> &user) const
{
  size_t no_of_all_bytes_sent{};

  if (user->remote_endpoint != asio::ip::udp::endpoint{}) {
    const std::chrono::time_point<std::chrono::system_clock> now =
      std::chrono::system_clock::now();
    const auto t_c = std::chrono::system_clock::to_time_t(now);
    const string outgoing_data{ format(R"({}\{}\{}\{}\{})", command_name, user->user_name, t_c, is_show_in_messages ? "true" : "false", data) };
    const size_t message_len{ outgoing_data.length() };
    const size_t number_of_message_parts = message_len / 1350 + (message_len % 1350 > 0 ? 1 : 0);
    const size_t random_message_id{ get_random_number() };

    for (size_t start{}, msg_index{ 1 }; start < message_len; ++msg_index) {
      const size_t sent_len{ std::min<size_t>(message_len - start, 1350) };
      const string output{ format("[[{}:{}:{}]]{}", random_message_id, number_of_message_parts, msg_index, outgoing_data.substr(start, sent_len)) };
      start += sent_len;
      const size_t sent_bytes = udp_socket_for_messages.send_to(buffer(output.c_str(), output.length()), user->remote_endpoint);
      main_app.add_to_next_uploaded_data_in_bytes(sent_bytes);
      no_of_all_bytes_sent += sent_bytes;
    }
    return no_of_all_bytes_sent;
  }

  return 0;
}

bool connection_manager_for_remote_messages::wait_for_and_process_response_message()
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

  const string message(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);

  string sender_ip{ remote_endpoint.address().to_v4().to_string() };
  string geo_information;
  player pd{};
  unsigned long guid{};
  const bool is_sender_ip_address_valid{ check_ip_address_validity(sender_ip, guid) };
  if (is_sender_ip_address_valid) {
    pd.ip_address = sender_ip;
    convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);
    geo_information = format("{}, {}", pd.country_name, pd.city);
  }

  if (noOfReceivedBytes > 0 && '{' != message.front() && '}' != message.back()) {
    auto parts = stl::helper::str_split(message, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    if (parts.size() < 6) {
      const string incorrectly_formatted_message_received{ format("Received an incorrectly formatted message from IP address: {} | geoinfo: {}\nMessage contents: '{}'\n", sender_ip, geo_information, message) };
      log_message(incorrectly_formatted_message_received, is_log_datetime::yes);

      return false;
    }

    const string message_contents{ str_join(cbegin(parts) + 5, cend(parts), '\\') };

    int number;
    if (!is_valid_decimal_whole_number(parts[3], number) || (parts[4] != "true" && parts[4] != "false")) {
      const string incorrectly_formatted_message_received{ format("Received an incorrectly formatted message!\n{} (IP address: {} | geoinfo: {} | rcon_password: {}) sent the following command: '{}'\nMessage contents: '{}'\n", parts[1], sender_ip, geo_information, parts[2], parts[0], message_contents) };
      log_message(incorrectly_formatted_message_received, is_log_datetime::yes);
      return false;
    }

    const string message_handler_name{ std::move(parts[0]) };
    const string sender{ std::move(parts[1]) };
    const time_t timestamp{ stoll(parts[3]) };
    const bool is_show_in_messages{ parts[4] == "true" };
    if (is_sender_ip_address_valid) {

      if (message_handler_name == "request-mapnames-player") {
        auto user = make_shared<tiny_rcon_client_user>();
        user->is_logged_in = true;
        user->user_name = sender;
        user->ip_address = sender_ip;
        user->remote_endpoint = remote_endpoint;
        send_user_available_map_names(user);
        const string information{ format("Received 'request-mapnames' from user {} (IP: {} geoinfo: {})\nMessage contents: '{}'\n", sender, sender_ip, geo_information, message_contents) };
        // print_colored_text(app_handles.hwnd_re_messages_data, information.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, true, false, false);
        log_message(information, is_log_datetime::yes);
        return true;
      }

      // request-imagesdata
      if (message_handler_name == "request-imagesdata") {
        auto user = make_shared<tiny_rcon_client_user>();
        user->is_logged_in = true;
        user->user_name = sender;
        user->ip_address = sender_ip;
        user->remote_endpoint = remote_endpoint;
        const string information{ format("Received 'request-imagesdata' from user {} (IP: {} geoinfo: {})\nMessage contents: '{}'\n", sender, sender_ip, geo_information, message_contents) };
        log_message(information, is_log_datetime::yes);
        if (!main_app.get_image_files_path_to_md5().empty()) {
          main_app.add_remote_message_to_queue(server_message_t("receive-imagesdata", main_app.get_image_files_path_to_md5(), user, true));
        }
        return true;
      }

      if (message_handler_name.starts_with("add-report") || message_handler_name.starts_with("remove-report") || message_handler_name.starts_with("request-reports") || message_handler_name.ends_with("-player")) {
        auto &player = get_user_for_specified_username_and_ip_address(sender, sender_ip);
        player->is_logged_in = true;
        player->ip_address = sender_ip;
        player->remote_endpoint = remote_endpoint;
        player->geo_information = geo_information;
        player->country_code = pd.country_code;
        if (message_handler_name.find("rcon-heartbeat") == string::npos) {
          const string information_message{ format("Received command '{}' from user {} [IP: {} geoinfo: {}]\nMessage contents: '{}'\n", message_handler_name, sender, sender_ip, player->geo_information, message_contents) };
          log_message(information_message, is_log_datetime::yes);
        }
        const auto &message_handler = main_app.get_remote_message_handler(message_handler_name);
        message_handler(sender, timestamp, message_contents, is_show_in_messages, sender_ip);
        return true;
      }

      // request-topplayers
      if (message_handler_name == "request-topplayers") {
        auto &player = get_user_for_specified_username_and_ip_address(sender, sender_ip);
        player->is_logged_in = true;
        player->ip_address = sender_ip;
        player->remote_endpoint = remote_endpoint;
        player->geo_information = geo_information;
        player->country_code = pd.country_code;
        const string information_message{ format("Received command '{}' from user {} [IP: {} geoinfo: {}]\nMessage contents: '{}'\n", message_handler_name, sender, sender_ip, player->geo_information, message_contents) };
        log_message(information_message, is_log_datetime::yes);
        const auto &message_handler = main_app.get_remote_message_handler(message_handler_name);
        message_handler(sender, timestamp, message_contents, is_show_in_messages, sender_ip);
        return true;
      }

      strcpy_s(pd.player_name, std::size(pd.player_name), sender.c_str());

      if (!is_authorized_remote_user(sender, parts[2] /*, pd.country_name*/)) {
        const string message_from_unauthorized_user{ format("Received a message from an unauthorized user!\n{} (IP address: {} | Country, city: {} | rcon_password: {}) sent the following command: '{}'\nMessage contents: '{}'\n", sender, sender_ip, geo_information, parts[2], message_handler_name, message_contents) };
        log_message(message_from_unauthorized_user, is_log_datetime::yes);
        return false;
      }
    } else {
      const string message_from_invalid_ip_address{ format("Received a message from an invalid IP address!\nUser '{}' (IP address: {} | rcon_password: {}) sent the following command: '{}'\nMessage contents: '{}'\n", sender, sender_ip, parts[2], message_handler_name, message_contents) };
      log_message(message_from_invalid_ip_address, is_log_datetime::yes);
      return false;
    }

    const auto &user = main_app.get_remote_user_for_name(sender, sender_ip);
    user->ip_address = sender_ip;
    user->remote_endpoint = remote_endpoint;
    user->geo_information = format("{}, {}", pd.country_name, pd.city);
    user->country_code = pd.country_code;
    const auto &message_handler = main_app.get_remote_message_handler(message_handler_name);
    message_handler(sender, timestamp, message_contents, is_show_in_messages, user->ip_address);
    if (message_handler_name.find("rcon-heartbeat") == string::npos) {
      const string information{ format("Received command '{}' from authorized user {} (IP: {} geoinfo: {})\nMessage contents: {}\n", message_handler_name, sender, sender_ip, geo_information, message_contents) };
      log_message(information, is_log_datetime::yes);
    }
    return true;
  }

  const string incorrectly_formatted_message_received{ format("Received an incorrectly formatted message from IP address: {} | geoinfo: {}\nContents of message: '{}'\n", sender_ip, geo_information, message) };
  log_message(incorrectly_formatted_message_received, is_log_datetime::yes);

  return false;
}