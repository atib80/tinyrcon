#include "connection_manager.h"
#include "tiny_cod2_rcon_client_application.h"
#include "stl_helper_functions.hpp"
#include <utility>
#include <regex>

using namespace std;

extern tiny_cod2_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;
extern string previous_map;
extern int selected_row;

static const std::regex ip_address_regex{ R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}:-?\d+\s+-?\d+\s+\d+))" };

using namespace asio;

using stl::helper::strstr;

connection_manager::connection_manager() : udp_socket{ udp_service }
{
  try {
    if (udp_socket.is_open()) {
      udp_socket.close();
    }

    udp_socket.open(ip::udp::v4());
    udp_socket.set_option(rcv_timeout_option{ 700 });

  } catch (std::exception &ex) {
    show_error(app_handles.hwnd_main_window, ex.what(), 0);
  }
}

void connection_manager::prepare_rcon_command(
  char *buffer,
  const size_t buffer_size,
  const char *rconCommandToSend,
  const char *rcon_password) const noexcept
{
  // ZeroMemory(buffer, buffer_size);
  if (_strcmpi(rconCommandToSend, "getstatus") == 0 || _strcmpi(rconCommandToSend, "getinfo") == 0) {
    (void)snprintf(buffer, buffer_size, "\xFF\xFF\xFF\xFF%s", rconCommandToSend);
  } else {
    (void)snprintf(buffer, buffer_size, "\xFF\xFF\xFF\xFFrcon %s %s", rcon_password, rconCommandToSend);
    ++number_of_sent_rcon_commands;
  }
}

size_t connection_manager::send_rcon_command(
  const string &outgoing_data,
  const char *remote_ip,
  const uint_least16_t remote_port) const
{
  const ip::udp::endpoint dst{ ip::address::from_string(remote_ip), remote_port };
  const size_t sent_bytes = udp_socket.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), dst);
  return sent_bytes;
}

size_t connection_manager::receive_data_from_server(
  const char *remote_ip,
  const uint_least16_t remote_port,
  std::string &received_reply) const
{
  char incoming_data_buffer[receive_buffer_size];
  size_t noOfReceivedBytes{}, noOfAllReceivedBytes{};

  ip::udp::endpoint destination{ ip::address::from_string(remote_ip), remote_port };
  asio::error_code err1{};
  noOfReceivedBytes = udp_socket.receive_from(
    buffer(incoming_data_buffer, receive_buffer_size), destination, 0, err1);
  if (err1) {
    main_app.set_server_message("^5| ^1Server is down.^5");
    return noOfReceivedBytes;
  }

  if (main_app.get_server_message().find("down.") != string::npos) {
    main_app.set_server_message("^5| ^3Server is empty.^5");
  }
  received_reply.clear();
  if (noOfReceivedBytes > 0) {
    noOfAllReceivedBytes += noOfReceivedBytes;
    received_reply.assign(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);

    const char *start =
      stl::helper::strstr(incoming_data_buffer, "\xFF\xFF\xFF\xFFprint\nInvalid password.");
    if (start != nullptr) {
      main_app.set_is_connection_settings_valid(false);
      main_app.add_command_to_queue({ "getstatus" }, command_type::rcon, true);
    } else {
      incoming_data_buffer[noOfAllReceivedBytes] = '\0';
      const char *current{}, *last{};

      const auto [rcon_status_response_needle1, rcon_status_response_needle2] = get_appropriate_rcon_status_response_header(main_app.get_game_name());

      if ((strstr(incoming_data_buffer, rcon_status_response_needle1)
            != nullptr)
          || (rcon_status_response_needle2 != nullptr && strstr(incoming_data_buffer, rcon_status_response_needle2) != nullptr)) {
        main_app.set_is_connection_settings_valid(true);
        start = received_reply.c_str() + 15;
        last = start;
        while (*last != 0x0A)
          ++last;
        string current_map(start, last);
        if (previous_map.empty() || current_map != previous_map) {
          selected_row = 0;
          previous_map = current_map;
        }
        main_app.get_game_server().set_current_map(std::move(current_map));

        bool is_server_empty_or_error_receiving_udp_datagrams{};

        const auto digit_pos = received_reply.find_first_of(
          "0123456789", (last - received_reply.c_str()) + 1);
        if (digit_pos == string::npos) {
          is_server_empty_or_error_receiving_udp_datagrams = true;
          received_reply.clear();
        } else {
          received_reply.erase(0, digit_pos);
        }

        while (strstr(incoming_data_buffer, "\n\n") == nullptr) {
          asio::error_code err2{};
          ZeroMemory(incoming_data_buffer, receive_buffer_size);
          noOfReceivedBytes = udp_socket.receive_from(
            buffer(incoming_data_buffer, receive_buffer_size), destination, 0, err2);
          noOfAllReceivedBytes += noOfReceivedBytes;
          if (err2) {
            is_server_empty_or_error_receiving_udp_datagrams = true;
            break;
          }

          const size_t new_line_pos = stl::helper::str_index_of(incoming_data_buffer, "\n");
          if (new_line_pos != string::npos) {
            received_reply.append(incoming_data_buffer + new_line_pos + 1, incoming_data_buffer + noOfReceivedBytes);
          } else {
            received_reply.append(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
          }
        }

        auto &ip_address_frequency = main_app.get_game_server().get_ip_address_frequency();
        ip_address_frequency.clear();

        int number_of_online_players{};
        int number_of_offline_players{};
        size_t pl_index{};

        if (!is_server_empty_or_error_receiving_udp_datagrams) {

          vector<string> lines{ stl::helper::str_split(received_reply, "\n", nullptr, true) };

          for (size_t i{}; i < lines.size(); ++i) {
            auto first = lines[i].cbegin();
            auto last2 = lines[i].cend();
            std::smatch ip_matches;
            size_t found_count{};
            vector<std::string::const_iterator> new_line_positions;
            while (regex_search(first, last2, ip_matches, ip_address_regex)) {
              ++found_count;
              if (found_count == 2) {
                std::string next_player_line(new_line_positions[0], last2);
                lines[i].erase(new_line_positions[0], last2);
                lines.insert(cbegin(lines) + static_cast<std::ptrdiff_t>(i + 1), std::move(next_player_line));
                break;
              }

              first = ip_matches[1].second;
              new_line_positions.emplace_back(first);
            }
          }

          auto &players_data = main_app.get_game_server().get_players_data();
          const auto &status_regex = get_appropriate_status_regex_for_specified_game_name(main_app.get_game_name());

          for (const string &player_line : lines) {

            std::smatch matches{};

            if (std::regex_search(player_line, matches, status_regex)) {
              const int player_pid{ stoi(matches[1].str()) };

              const string score{ matches[2].str() };
              int player_score{};
              if (!is_valid_decimal_whole_number(score, player_score))
                player_score = 0;

              string player_ping{ matches[3].str() };
              stl::helper::trim_in_place(player_ping);
              int ping_value{};
              if ("999" == player_ping || "-1" == player_ping || !is_valid_decimal_whole_number(player_ping, ping_value)) {
                ++number_of_offline_players;
              } else {
                ++number_of_online_players;
              }

              string guid_key{ matches[4].str() };

              string player_name{ matches[5].str() };
              const size_t pn_len{ player_name.length() };
              if (pn_len >= 2 && player_name[pn_len - 2] == '^' && player_name[pn_len - 1] == '7') {
                player_name.pop_back();
                player_name.pop_back();
              }
              stl::helper::trim_in_place(player_name, " \t\n\f\v\r");

              const string ip_address{ matches[7].str() };

              players_data[pl_index].pid = player_pid;
              players_data[pl_index].score = player_score;
              strcpy_s(players_data[pl_index].ping, 5, player_ping.c_str());
              if (strcmp(player_name.c_str(), players_data[pl_index].player_name) != 0) {
                const size_t no_of_chars_to_copy{ std::min<size_t>(32, player_name.length()) };
                strncpy_s(players_data[pl_index].player_name, 33, player_name.c_str(), no_of_chars_to_copy);
                players_data[pl_index].player_name[no_of_chars_to_copy] = '\0';
                // stl::helper::trim_in_place(players_data[pl_index].player_name);
              }

              if (strcmp(ip_address.c_str(), players_data[pl_index].ip_address) != 0) {

                strcpy_s(players_data[pl_index].ip_address, 16, ip_address.c_str());
                convert_guid_key_to_country_name(
                  geoip_db, players_data[pl_index].ip_address, players_data[pl_index]);
              }


              if (stl::helper::len(players_data[pl_index].ip_address) > 0) {
                ++ip_address_frequency[players_data[pl_index].ip_address];
              }
              ++pl_index;
            }
          }
        }

        main_app.get_game_server().set_number_of_players(pl_index);
        main_app.get_game_server().set_number_of_online_players(number_of_online_players);
        main_app.get_game_server().set_number_of_offline_players(number_of_offline_players);
        check_for_temp_banned_ip_addresses();
        check_for_banned_ip_addresses();
        check_for_warned_players();

      } else if (strstr(incoming_data_buffer, "infoResponse") != nullptr && incoming_data_buffer[4] == 'i') {
        current = incoming_data_buffer + 18;
        auto parsedData = stl::helper::str_split(current, "\\", "", true);
        for (size_t i{}; i + 1 < parsedData.size(); i += 2) {
          update_game_server_setting(std::move(parsedData[i]),
            std::move(parsedData[i + 1]));
        }
      } else if (strstr(incoming_data_buffer, "statusResponse") != nullptr && incoming_data_buffer[4] == 's') {
        current = incoming_data_buffer + 20;

        const char *lastIndex = strchr(current, '\n');
        const string &server_info{ current, lastIndex };
        auto parsedData = stl::helper::str_split(server_info, "\\", "", true);
        for (size_t i{}; i + 1 < parsedData.size(); i += 2) {
          update_game_server_setting(std::move(parsedData[i]),
            std::move(parsedData[i + 1]));
        }

        int player_num{};
        int number_of_online_players{};
        int number_of_offline_players{};
        start = last = lastIndex + 1;
        auto &players_data = main_app.get_game_server().get_players_data();
        current = start;

        while (*current && current < incoming_data_buffer + noOfAllReceivedBytes) {
          while (*last != '\n')
            ++last;
          current = last + 1;
          const string playerDataLine{ start, last };

          start = last = playerDataLine.c_str();

          // Extracting player's score value from received UDP data packet for
          // getstatus command
          while (' ' == *start)
            ++start;
          last = start;
          while (*last != ' ')
            ++last;
          string temp_score{ start, last };
          stl::helper::trim_in_place(temp_score);
          int player_score;
          if (!is_valid_decimal_whole_number(temp_score, player_score))
            player_score = 0;

          // Extracting player's ping value from received UDP data packet for
          // getstatus command
          start = last;
          while (' ' == *start)
            ++start;
          last = start;
          while (*last != ' ')
            ++last;

          string player_ping{ start, last };
          stl::helper::trim_in_place(player_ping);
          if ("999" == player_ping || "-1" == player_ping || "CNCT" == player_ping || "ZMBI" == player_ping) {
            ++number_of_offline_players;
          } else {
            ++number_of_online_players;
          }

          // Extracting player_name information from received UDP data packet
          // for getstatus command
          start = last;
          while (*start != '"')
            ++start;
          last = ++start;
          while (*last != '"')
            ++last;

          players_data[player_num].pid = player_num + 1;
          players_data[player_num].score = player_score;
          strcpy_s(players_data[player_num].ping, std::size(players_data[player_num].ping), player_ping.c_str());
          const size_t no_of_chars_to_copy = static_cast<size_t>(last - start);
          strncpy_s(players_data[player_num].player_name, std::size(players_data[player_num].player_name), start, no_of_chars_to_copy);
          players_data[player_num].player_name[no_of_chars_to_copy] = '\0';
          stl::helper::trim_in_place(players_data[player_num].player_name);
          players_data[player_num].country_name = "Unknown";
          players_data[player_num].region = "Unknown";
          players_data[player_num].city = "Unknown";
          players_data[player_num].country_code = "xy";
          ++player_num;
          start = last = current;
        }

        main_app.get_game_server().set_number_of_players(player_num);
        main_app.get_game_server().set_number_of_online_players(number_of_online_players);
        main_app.get_game_server().set_number_of_offline_players(number_of_offline_players);

      } else if (strstr(incoming_data_buffer, "Server info") != nullptr) {
        main_app.get_game_server().get_server_settings().clear();
        current = incoming_data_buffer + 32;
        while (*current && current < incoming_data_buffer + noOfAllReceivedBytes) {
          start = last = current;
          while (*last != ' ')
            ++last;
          string property{ start, last };
          start = last;
          while (*start == ' ')
            ++start;
          last = start;
          while (*last != 0x0A)
            ++last;
          string value{ start, last };
          current = ++last;
          update_game_server_setting(std::move(property), std::move(value));
        }
      } else if (strstr(incoming_data_buffer, "map_rotate...\n\n") != nullptr) {
        main_app.set_is_connection_settings_valid(true);
        received_reply.assign(incoming_data_buffer,
          incoming_data_buffer + noOfReceivedBytes);
        while (true) {
          asio::error_code err3{};
          ZeroMemory(incoming_data_buffer, receive_buffer_size);
          noOfReceivedBytes = udp_socket.receive_from(
            buffer(incoming_data_buffer, receive_buffer_size), destination, 0, err3);
          if (err3) break;
          received_reply.append(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
          noOfAllReceivedBytes += noOfReceivedBytes;
        }

      } else if (strstr(incoming_data_buffer, "sv_mapRotationCurrent") != nullptr) {
        main_app.set_is_connection_settings_valid(true);
        current = &incoming_data_buffer[0] + 39;
        start = current;
        last = stl::helper::strstr(current, "^7\"");
        if (start == last)
          main_app.get_game_server().set_map_rotation_current(" ");
        else {
          main_app.get_game_server().set_map_rotation_current(string(start, last));
          if (strstr(current, "gametype") != nullptr) {
            current = strstr(current, "map");

            if (current != nullptr) {
              current += 4;
              while (*current == ' ')
                ++current;
              start = last = current;
              while (*last != ' ')
                ++last;
              main_app.get_game_server().set_next_map(string(start, last));
            }
          }
        }
      } else if (strstr(incoming_data_buffer, "sv_mapRotation") != nullptr) {
        main_app.set_is_connection_settings_valid(true);
        current = &incoming_data_buffer[0] + 32;
        start = current;
        last = stl::helper::strstr(current, "^7\"");
        main_app.get_game_server().set_map_rotation(string(start, last));
      } else if (strstr(incoming_data_buffer, "sv_hostname") != nullptr) {
        main_app.set_is_connection_settings_valid(true);
        current = &incoming_data_buffer[0] + 29;
        start = last = current;
        while (*last != '^' && *(last + 1) != '7' && *(last + 2) != '"')
          ++last;
        main_app.get_game_server().set_server_name(string(start, last));
      } else if (strstr(incoming_data_buffer, "mapname") != nullptr) {
        main_app.set_is_connection_settings_valid(true);
        start = current = &incoming_data_buffer[0] + 25;
        last = stl::helper::strstr(current, "^7");
        if (last != nullptr) {
          string current_map(start, last);
          if (previous_map.empty() || current_map != previous_map) {
            selected_row = 0;
            previous_map = current_map;
          }
          main_app.get_game_server().set_current_map(std::move(current_map));
        }
      } else if (strstr(incoming_data_buffer, "\"g_gametype\" is: \"") != nullptr && strstr(incoming_data_buffer, "default: \"") != nullptr) {
        main_app.set_is_connection_settings_valid(true);
        start =
          stl::helper::strstr(incoming_data_buffer, "\"g_gametype\" is: \"") + 18;
        last = stl::helper::strstr(start, "^7");
        if (last != nullptr) {
          main_app.get_game_server().set_current_game_type(string(start, last));
        }

      } else {
        while (true) {
          asio::error_code err3{};
          noOfReceivedBytes = udp_socket.receive_from(
            buffer(incoming_data_buffer, receive_buffer_size), destination, 0, err3);
          if (err3) break;
          received_reply.append(incoming_data_buffer,
            incoming_data_buffer + noOfReceivedBytes);
          noOfAllReceivedBytes += noOfReceivedBytes;
        }
      }
    }
  }

  return noOfAllReceivedBytes;
}


void connection_manager::send_and_receive_rcon_data(
  const char *rcon_command_to_send,
  std::string &received_reply,
  const char *remote_ip,
  const uint_least16_t remote_port,
  const char *rcon_password,
  const bool is_wait_for_reply) const
{
  constexpr size_t buffer_size{ 1536 };
  static char outgoing_data_buffer[buffer_size];
  std::lock_guard lg{ rcon_mutex };
  prepare_rcon_command(outgoing_data_buffer, buffer_size, rcon_command_to_send, rcon_password);
  (void)send_rcon_command(outgoing_data_buffer, remote_ip, remote_port);
  if (is_wait_for_reply) {
    receive_data_from_server(remote_ip, remote_port, received_reply);
  }
}
