#define WINVER 0x0502

#include "connection_manager.h"
#include "tiny_rcon_client_application.h"
#include "stl_helper_functions.hpp"
#include <utility>
#include <regex>
#include "stack_trace_element.h"

using namespace std;
using namespace stl::helper;

extern tiny_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;
extern const size_t max_players_grid_rows;
extern string previous_map;
extern int selected_row;

static const std::regex ip_address_regex{ R"((\d+\.\d+\.\d+\.\d+:(-?\d+)\s*(-?\d+)\s*(\d+)))" };

using namespace asio;

using stl::helper::strstr;

connection_manager::connection_manager() : udp_socket_for_rcon_commands{ udp_service_for_rcon_commands }, udp_socket_for_non_rcon_commands{ udp_service_for_non_rcon_commands }
{
  try {
    if (udp_socket_for_rcon_commands.is_open()) {
      udp_socket_for_rcon_commands.close();
    }

    udp_socket_for_rcon_commands.open(ip::udp::v4());
    udp_socket_for_rcon_commands.set_option(rcv_timeout_option{ 700 });

    if (udp_socket_for_non_rcon_commands.is_open()) {
      udp_socket_for_non_rcon_commands.close();
    }

    udp_socket_for_non_rcon_commands.open(ip::udp::v4());
    udp_socket_for_non_rcon_commands.set_option(rcv_timeout_option{ 700 });

  } catch (std::exception &ex) {
    show_error(app_handles.hwnd_main_window, ex.what(), 0);
  }
}

void connection_manager::prepare_non_rcon_command(char *buffer, const std::size_t buffer_size, const char *command_to_send) const noexcept
{
  (void)snprintf(buffer, buffer_size, "\xFF\xFF\xFF\xFF%s", command_to_send);

  ++number_of_sent_non_rcon_commands;
}

size_t
  connection_manager::send_non_rcon_command(
    const string &outgoing_data,
    const char *remote_ip,
    const uint_least16_t remote_port) const
{
  string ex_msg{ format(R"(^1Exception ^3thrown from ^1size_t connection_manager::send_non_rcon_command("{}", "{}", {}))", outgoing_data, remote_ip, remote_port) };
  stack_trace_element ste{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };

  const ip::udp::endpoint dst{ ip::address::from_string(remote_ip), remote_port };
  const size_t sent_bytes = udp_socket_for_non_rcon_commands.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), dst);
  return sent_bytes;
}

size_t connection_manager::receive_non_rcon_reply_from_server(const char *remote_ip, const uint_least16_t remote_port, std::string &received_reply, const bool is_process_reply) const
{
  string ex_msg{ format(R"(^1Exception ^3thrown from ^1size_t connection_manager::receive_non_rcon_reply_from_server("{}", {}, "{}", {}))", remote_ip, remote_port, received_reply, is_process_reply ? "true" : "false") };
  stack_trace_element ste1{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };

  char incoming_data_buffer[receive_buffer_size];
  size_t noOfReceivedBytes{}, noOfAllReceivedBytes{};

  received_reply.clear();

  while (true) {
    ZeroMemory(incoming_data_buffer, receive_buffer_size);
    ip::udp::endpoint destination{ ip::address::from_string(remote_ip), remote_port };
    asio::error_code err1{};

    noOfReceivedBytes = udp_socket_for_non_rcon_commands.receive_from(
      buffer(incoming_data_buffer, receive_buffer_size), destination, 0, err1);

    if (err1)
      break;

    if (destination.address().to_v4().to_string() != remote_ip) continue;

    if (noOfReceivedBytes > 0U) {
      ltrim_in_place(incoming_data_buffer, " \t\n\xFF");
      received_reply.append(incoming_data_buffer);
      noOfAllReceivedBytes += noOfReceivedBytes;
    }
  }

  if (noOfAllReceivedBytes > 0) {

    const char *start{ received_reply.c_str() }, *current{}, *last{};
    if (received_reply.starts_with("infoResponse")) {
      current = received_reply.c_str() + 13;
      auto parsedData = str_split(current, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
      for (size_t i{}; i + 1 < parsedData.size(); i += 2) {
        update_game_server_setting(std::move(parsedData[i]),
          std::move(parsedData[i + 1]));
      }
    } else if (received_reply.starts_with("statusResponse")) {

      if (!is_process_reply)
        return noOfAllReceivedBytes;

      current = received_reply.c_str() + 15;

      const char *lastIndex = strchr(current, '\n');
      string_view server_info(current, lastIndex);
      vector<string> parsedData{ str_split(server_info, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no) };
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

      while (*current && current < received_reply.c_str() + received_reply.length()) {
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
      ++rcon_status_sent_counter;
      const bool log_players_data{ rcon_status_sent_counter % 60 == 0 };
      if (60 == rcon_status_sent_counter)
        rcon_status_sent_counter = 0;
      prepare_players_data_for_display_of_getstatus_response(log_players_data);
    }
  }

  return noOfAllReceivedBytes;
}

void connection_manager::prepare_rcon_command(
  char *buffer,
  const size_t buffer_size,
  const char *rconCommandToSend,
  const char *rcon_password) const noexcept
{
  (void)snprintf(buffer, buffer_size, "\xFF\xFF\xFF\xFFrcon %s %s", rcon_password, rconCommandToSend);
  ++number_of_sent_rcon_commands;
}

size_t connection_manager::send_rcon_command(
  const string &outgoing_data,
  const char *remote_ip,
  const uint_least16_t remote_port) const
{
  string ex_msg{ format(R"(^1Exception ^3thrown from ^1size_t connection_manager::send_rcon_command("{}", "{}", {}))", outgoing_data, remote_ip, remote_port) };
  stack_trace_element ste{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };

  const ip::udp::endpoint dst{ ip::address::from_string(remote_ip), remote_port };
  const size_t sent_bytes = udp_socket_for_rcon_commands.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), dst);
  return sent_bytes;
}

size_t connection_manager::receive_rcon_reply_from_server(
  const char *remote_ip,
  const uint_least16_t remote_port,
  std::string &received_reply,
  const bool is_process_reply) const
{
  char incoming_data_buffer[receive_buffer_size]{};
  size_t noOfReceivedBytes{}, noOfAllReceivedBytes{};

  string ex_msg{ format(R"(^1Exception ^3thrown from ^1size_t connection_manager::receive_rcon_reply_from_server("{}", {}, "{}", {}))", remote_ip, remote_port, received_reply, is_process_reply ? "true" : "false") };
  stack_trace_element ste1{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };

  received_reply.clear();

  while (true) {
    ZeroMemory(incoming_data_buffer, receive_buffer_size);
    ip::udp::endpoint destination{ ip::address::from_string(remote_ip), remote_port };
    asio::error_code erc{};

    noOfReceivedBytes = udp_socket_for_rcon_commands.receive_from(
      buffer(incoming_data_buffer, receive_buffer_size), destination, 0, erc);

    if (erc) break;

    if (noOfReceivedBytes > 0U) {
      const char *start_needle_to_search_for{ "print\n" };
      const size_t new_line_pos = stl::helper::str_index_of(incoming_data_buffer, start_needle_to_search_for);
      if (new_line_pos != string::npos) {
        received_reply.append(incoming_data_buffer + new_line_pos + len(start_needle_to_search_for), incoming_data_buffer + noOfReceivedBytes);
      } else {
        received_reply.append(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
      }
      noOfAllReceivedBytes += noOfReceivedBytes;
    }

    /*if (received_reply.ends_with("\n\n"))
        break;*/
  }

  if (noOfAllReceivedBytes > 0) {

    const char *start{};
    if (received_reply.starts_with("\xFF\xFF\xFF\xFFprint\nInvalid password.")) {
      main_app.get_game_server().set_is_connection_settings_valid(false);
      set_admin_actions_buttons_active(FALSE);
    } else {
      const auto gn = main_app.get_game_name();
      const char *current{}, *last{};

      auto [rcon_status_response_needle1, rcon_status_response_needle2] = get_appropriate_rcon_status_response_header(main_app.get_game_name());

      if (received_reply.find(rcon_status_response_needle1) != string::npos
          || (rcon_status_response_needle2 != nullptr && received_reply.find(rcon_status_response_needle2) != string::npos)) {
        if (!main_app.get_game_server().get_is_connection_settings_valid()) {
          main_app.get_game_server().set_is_connection_settings_valid(true);
          set_admin_actions_buttons_active(TRUE);
        }

        start = received_reply.c_str() + 5;
        last = start;
        while (*last != 0x0A && *last != '=')
          ++last;
        string current_map(start, last);
        trim_in_place(current_map);
        if (previous_map.empty() || current_map != previous_map) {
          selected_row = 0;
          previous_map = current_map;
          initialize_elements_of_container_to_specified_value(main_app.get_game_server().get_players_data(), player{}, 0);
          clear_players_data_in_players_grid(app_handles.hwnd_players_grid, 0, max_players_grid_rows, 7);
        }
        main_app.get_game_server().set_current_map(std::move(current_map));

        bool is_server_empty_or_error_receiving_udp_datagrams{};

        const auto digit_pos = received_reply.find_first_of(
          "0123456789", (last - received_reply.c_str()) + 1);
        if (digit_pos == string::npos) {
          is_server_empty_or_error_receiving_udp_datagrams = true;
          received_reply.clear();
        } else {
          received_reply.erase(begin(received_reply), begin(received_reply) + digit_pos);
        }

        auto &ip_address_frequency = main_app.get_game_server().get_ip_address_frequency();
        ip_address_frequency.clear();

        int number_of_online_players{};
        int number_of_offline_players{};
        size_t pl_index{};

        if (!is_server_empty_or_error_receiving_udp_datagrams) {
          string ex_msg2{ R"(^1Exception ^3thrown from 'if (!is_server_empty_or_error_receiving_UDP_datagrams){...}')" };
          stack_trace_element ste2{
            app_handles.hwnd_re_messages_data,
            std::move(ex_msg2)
          };

          vector<string> lines{ stl::helper::str_split(received_reply, "\n", nullptr) };
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

          for (const string &player_line : lines) {
            int j{}, i{ 3 };
            int digit_count{};
            int player_pid{};
            while (is_ws(player_line[j]) && j < i) ++j;
            while (isdigit(player_line[j]) && j < i && digit_count < 2) {
              player_pid *= 10;
              player_pid += static_cast<int>(player_line[j] - '0');
              ++digit_count;
              ++j;
            }

            if (player_pid >= 64) {
              player_pid /= 10;
              --j;
            }

            i = j + 6;

            while (j < i && !isdigit(player_line[j]) && player_line[j] != '-') ++j;

            int player_score{};

            bool is_player_score_negative{};
            if (player_line[j] == '-') {
              is_player_score_negative = true;
              ++j;
            }

            while (isdigit(player_line[j]) && j < i) {
              player_score *= 10;
              player_score += static_cast<int>(player_line[j] - '0');
              ++j;
            }

            if (is_player_score_negative)
              player_score = -player_score;


            i = j + 5;

            while (is_ws(player_line[j]) && j < i) ++j;
            i = j;
            int letter_count{};
            digit_count = 0;
            while (!is_ws(player_line[j])) {
              if ((isalnum(player_line[j]) || player_line[j] == '-') && digit_count < 3 && letter_count < 4) {
                if (isdigit(player_line[j]))
                  ++digit_count;
                ++letter_count;
                ++j;

                if (2 == letter_count && 1 == digit_count && '-' == player_line[i] && '1' == player_line[i + 1]) {
                  j = i + 2;
                  break;
                }
              } else
                break;
            }

            const string player_ping{ player_line.substr(i, letter_count) };


            int ping_value{};
            if (player_ping == "999" || player_ping == "-1" || !is_valid_decimal_whole_number(player_ping, ping_value)) {
              ++number_of_offline_players;
            } else {
              ++number_of_online_players;
            }

            i = j + ((gn == game_name_t::cod1 || gn == game_name_t::cod2) ? 8 : 33);
            while (is_ws(player_line[j]) && j < i) ++j;
            const auto guid_start{ j };
            if (gn == game_name_t::cod1 || gn == game_name_t::cod2) {
              while (isdigit(player_line[j]) && j < i) ++j;
            } else {
              while (isxdigit(player_line[j]) && j < i) ++j;
            }

            const string player_guid{ trim(player_line.substr(guid_start, j - guid_start)) };

            while (is_ws(player_line[j])) ++j;
            const int pn_start{ j };

            string ip_address;
            ip_address.reserve(16);

            j = player_line.rfind(':');
            if (string::npos == j) {
              j = player_line.rfind('.');

              if (string::npos != j) {
                i = j - 1;
                ++j;
                int octet{};
                while (isdigit(player_line[j])) {
                  ++j;
                  octet *= 10;
                  octet += static_cast<int>(player_line[j] - '0');
                  if (octet > 255) {
                    break;
                  }
                  ip_address.push_back(player_line[j]);
                }

                ip_address.insert(0, 1, '.');

                int dot_count{};
                octet = 0;
                int factor{ 1 };
                while (isdigit(player_line[i]) || player_line[i] == '.') {
                  if (player_line[i] == '.') {
                    ++dot_count;
                    if (dot_count > 2)
                      break;
                    factor = 1;
                    octet = 0;
                  } else {

                    octet += factor * static_cast<int>(player_line[i] - '0');
                    factor *= 10;
                    if (octet > 255) {
                      ++dot_count;
                      if (dot_count > 2)
                        break;
                      octet = 0;
                      factor = 1;
                      ip_address.insert(0, 1, '.');
                      continue;
                    }
                  }

                  ip_address.insert(0, 1, player_line[i]);
                  --i;
                }
              }
            } else {

              i = j - 1;
              int dot_count{};
              int octet{};
              int factor{ 1 };

              while (isdigit(player_line[i]) || player_line[i] == '.') {
                if (player_line[i] == '.') {
                  ++dot_count;
                  if (dot_count > 3)
                    break;
                  factor = 1;
                  octet = 0;
                } else {
                  octet += factor * static_cast<int>(player_line[i] - '0');
                  factor *= 10;
                  if (octet > 255) {
                    ++dot_count;
                    if (dot_count > 3)
                      break;
                    octet = 0;
                    factor = 1;
                    ip_address.insert(0, 1, '.');
                    continue;
                  }
                }

                ip_address.insert(0, 1, player_line[i]);
                --i;
              }
            }

            while (is_ws(player_line[i])) --i;
            while (is_decimal_digit(player_line[i])) --i;
            while (is_ws(player_line[i])) --i;

            string player_name(player_line.c_str() + pn_start, player_line.c_str() + i);
            auto pn_end = player_name.find_last_of("^7");
            if (string::npos != pn_end)
              player_name.erase(cbegin(player_name) + pn_end, cend(player_name));
            if (const size_t pn_len{ player_name.length() }; (pn_len >= 1) && (player_name[pn_len - 1] == '^' || player_name[pn_len - 1] == '7')) {
              player_name.pop_back();
            }

            players_data[pl_index].pid = player_pid;
            players_data[pl_index].score = player_score;
            strcpy_s(players_data[pl_index].ping, std::size(players_data[pl_index].ping), player_ping.c_str());
            strcpy_s(players_data[pl_index].guid_key, std::size(players_data[pl_index].guid_key), player_guid.c_str());
            if (strcmp(player_name.c_str(), players_data[pl_index].player_name) != 0) {
              const size_t no_of_chars_to_copy{ std::min<size_t>(32, player_name.length()) };
              strncpy_s(players_data[pl_index].player_name, std::size(players_data[pl_index].player_name), player_name.c_str(), no_of_chars_to_copy);
              players_data[pl_index].player_name[no_of_chars_to_copy] = '\0';
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

        main_app.get_game_server().set_number_of_players(pl_index);
        main_app.get_game_server().set_number_of_online_players(number_of_online_players);
        main_app.get_game_server().set_number_of_offline_players(number_of_offline_players);
        ++rcon_status_sent_counter;
        const bool log_players_data{ rcon_status_sent_counter % 60 == 0 };
        if (60 == rcon_status_sent_counter)
          rcon_status_sent_counter = 0;
        prepare_players_data_for_display(log_players_data);

      } else if (received_reply.find("Server info") != string::npos) {
        main_app.get_game_server().get_server_settings().clear();
        current = received_reply.c_str() + 32;
        while (*current && current < received_reply.c_str() + noOfAllReceivedBytes) {
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
      } else if (received_reply.find("map_rotate...\n\n") != string::npos) {
        main_app.get_game_server().set_is_connection_settings_valid(true);
        print_colored_text(app_handles.hwnd_re_messages_data, received_reply.c_str());
      } else if (size_t start_pos{}; (start_pos = received_reply.find(R"("sv_mapRotationCurrent" is: ")")) != string::npos && received_reply.find(R"(default: ")") != string::npos) {
        main_app.get_game_server().set_is_connection_settings_valid(true);
        start_pos += strlen(R"("sv_mapRotationCurrent" is: ")");
        const size_t last_pos{ received_reply.find_first_of("^7\" ", start_pos) };
        if (last_pos != string::npos) {
          main_app.get_game_server().set_map_rotation_current(received_reply.substr(start_pos, last_pos - start_pos));
          current = received_reply.c_str() + start_pos;
          if (strstr(current, "gametype") != nullptr) {
            current = strstr(current, "map");
            if (current != nullptr) {
              current += 4;
              while (*current == ' ')
                ++current;
              start = last = current;
              while (*last != ' ' && *last != '^' && *last != '7' && *last != '"')
                ++last;
              main_app.get_game_server().set_next_map(string(start, last));
            }
          }
        }
      } else if (size_t first_pos1{}; (first_pos1 = received_reply.find(R"("sv_mapRotation" is: ")")) != string::npos && received_reply.find(R"(default: ")") != string::npos) {
        main_app.get_game_server().set_is_connection_settings_valid(true);
        first_pos1 += strlen(R"("sv_mapRotation" is: ")");
        const size_t last_pos{ received_reply.find_first_of("^7\" ", first_pos1) };
        if (last_pos != string::npos) {
          main_app.get_game_server().set_map_rotation(received_reply.substr(first_pos1, last_pos - first_pos1));
        }
      } else if (received_reply.find("sv_hostname") != string::npos) {
        main_app.get_game_server().set_is_connection_settings_valid(true);
        current = received_reply.c_str() + 29;
        start = last = current;
        while (*last != '^' && *(last + 1) != '7' && *(last + 2) != '"')
          ++last;
        main_app.get_game_server().set_server_name(string(start, last));
      } else if (size_t first_pos2{}; (first_pos2 = received_reply.find(R"("mapname" is: ")")) != string::npos && received_reply.find(R"(default: ")") != string::npos) {
        main_app.get_game_server().set_is_connection_settings_valid(true);
        first_pos2 += strlen(R"("mapname" is: ")");
        const size_t last_pos{ received_reply.find_first_of("^7\" ", first_pos2) };
        if (last_pos != string::npos) {
          string current_map(received_reply.substr(first_pos2, last_pos - first_pos2));
          if (previous_map.empty() || current_map != previous_map) {
            selected_row = 0;
            previous_map = current_map;
            initialize_elements_of_container_to_specified_value(
              main_app.get_game_server().get_players_data(), player{}, 0);
            clear_players_data_in_players_grid(app_handles.hwnd_players_grid, 0, max_players_grid_rows, 7);
          }
          main_app.get_game_server().set_current_map(std::move(current_map));
        }
      } else {
        parse_game_type_information_from_rcon_reply(received_reply);
      }
    }
  }
  return noOfAllReceivedBytes;
}

void connection_manager::send_and_receive_rcon_data(
  const char *command_to_send,
  std::string &received_reply,
  const char *remote_ip,
  const uint_least16_t remote_port,
  const char *rcon_password,
  const bool is_wait_for_reply,
  const bool is_process_reply) const
{
  constexpr size_t buffer_size{ 1536 };
  static char outgoing_data_buffer[buffer_size];

  string ex_msg{ format(R"(^1Exception ^3thrown from ^1size_t connection_manager::send_and_receive_rcon_data("{}", "{}", "{}", {}, "{}", {}, {}))", command_to_send, received_reply, remote_ip, remote_port, rcon_password, is_wait_for_reply ? "true" : "false", is_process_reply ? "true" : "false") };
  stack_trace_element ste{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };
  std::lock_guard lg{ rcon_mutex };
  prepare_rcon_command(outgoing_data_buffer, buffer_size, command_to_send, rcon_password);
  (void)send_rcon_command(outgoing_data_buffer, remote_ip, remote_port);
  if (is_wait_for_reply) {
    receive_rcon_reply_from_server(remote_ip, remote_port, received_reply, is_process_reply);
  }
}

void connection_manager::send_and_receive_non_rcon_data(const char *command_to_send, std::string &reply_buffer, const char *remote_ip, const uint_least16_t remote_port, const bool is_wait_for_reply, const bool is_process_reply) const
{
  constexpr size_t buffer_size{ 1536 };
  static char outgoing_data_buffer[buffer_size];

  string ex_msg{ format(R"(^1Exception ^3thrown from ^1size_t connection_manager::send_and_receive_non_rcon_data("{}", "{}", "{}", {}, {}, {}))", command_to_send, reply_buffer, remote_ip, remote_port, is_wait_for_reply ? "true" : "false", is_process_reply ? "true" : "false") };
  stack_trace_element ste{
    app_handles.hwnd_re_messages_data,
    std::move(ex_msg)
  };
  std::lock_guard lg{ non_rcon_mutex };
  prepare_non_rcon_command(outgoing_data_buffer, buffer_size, command_to_send);
  (void)send_non_rcon_command(outgoing_data_buffer, remote_ip, remote_port);
  if (is_wait_for_reply) {
    receive_non_rcon_reply_from_server(remote_ip, remote_port, reply_buffer, is_process_reply);
  }
}
