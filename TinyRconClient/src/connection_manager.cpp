// #define WINVER 0x0502

#include "connection_manager.h"
#include <utility>
#include <regex>
#include "tiny_rcon_client_application.h"
#include "simple_grid.h"


using namespace std;
using namespace stl::helper;

extern tiny_rcon_client_application main_app;
extern tiny_rcon_handles app_handles;
extern const size_t max_players_grid_rows;
extern string previous_map;

static const std::regex player_ip_address_regex{ R"((\d+\.\d+\.\d+\.\d+:(-?\d+)\s*(-?\d+)\s*(\d+)))" };
static const std::regex bot_ip_address_regex{ R"((\d{5,}\.\d{5,}):(-?\d+)\s*(-?\d+)\s*(\d+))" };
static const string player_name_needle_chars{ "^7" };

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

void connection_manager::prepare_non_rcon_command(char *buffer, const std::size_t buffer_size, const char *command_to_send) const
{
  (void)snprintf(buffer, buffer_size, "\xFF\xFF\xFF\xFF%s", command_to_send);
}

size_t
  connection_manager::send_non_rcon_command(
    const string &outgoing_data,
    const char *remote_ip,
    const uint_least16_t remote_port) const
{

  const ip::udp::endpoint dst{ ip::address::from_string(remote_ip), remote_port };
  const size_t sent_bytes = udp_socket_for_non_rcon_commands.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), dst);
  if (sent_bytes > 0) {
    main_app.add_to_next_uploaded_data_in_bytes(sent_bytes);
    ++number_of_sent_non_rcon_commands;
  }
  return sent_bytes;
}

size_t connection_manager::receive_non_rcon_reply_from_server(const char *remote_ip, const uint_least16_t remote_port, game_server &gs, std::string &received_reply, const bool is_process_reply) const
{
  char incoming_data_buffer[receive_buffer_size];
  size_t noOfReceivedBytes{}, noOfAllReceivedBytes{};

  received_reply.clear();

  while (true) {
    const ip::udp::endpoint expected_remote_endpoint{ ip::address::from_string(remote_ip), remote_port };
    ip::udp::endpoint remote_endpoint{};
    asio::error_code erc{};

    noOfReceivedBytes = udp_socket_for_non_rcon_commands.receive_from(
      buffer(incoming_data_buffer, receive_buffer_size), remote_endpoint, 0, erc);

    if (erc) break;

    incoming_data_buffer[noOfReceivedBytes] = '\0';

    main_app.add_to_next_downloaded_data_in_bytes(noOfReceivedBytes);


    if (remote_endpoint != ip::udp::endpoint{} && expected_remote_endpoint != remote_endpoint) return 0;

    if (noOfReceivedBytes > 0U) {
      noOfAllReceivedBytes += noOfReceivedBytes;

      string udp_packet(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
      ltrim_in_place(udp_packet, " \t\n\xFF");
      if (str_starts_with(udp_packet, string_view{ "getServersResponse\n", len("getServersResponse\n") }, true)) {

        if (udp_packet.ends_with("\\EOT"))
          udp_packet.erase(udp_packet.length() - 4, 4);

        if (str_starts_with(received_reply, string_view{ "getServersResponse\n", len("getServersResponse\n") }, true)) {
          udp_packet.erase(0, len("getServersResponse\n") + 1);
        }

        received_reply.append(udp_packet);

        if (received_reply.ends_with("\\EOF")) {
          break;
        }

      } else {
        received_reply.append(udp_packet);
        if (str_starts_with(received_reply, string_view{ "infoResponse\n", len("infoResponse\n") }, true)) break;
      }
    }
  }

  if (!is_process_reply)
    return noOfAllReceivedBytes;

  if (noOfAllReceivedBytes > 0) {

    if (str_starts_with(received_reply, "inforesponse", true)) {

      trim_in_place(received_reply);
      const size_t start{ received_reply.find('\\') };
      if (start == string::npos)
        return noOfAllReceivedBytes;

      received_reply.erase(cbegin(received_reply), cbegin(received_reply) + start + 1);
      print_colored_text(app_handles.hwnd_re_messages_data, received_reply.c_str());
      vector<string> parsedData{ str_split(received_reply, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no) };
      for (size_t i{}; i + 1 < parsedData.size(); i += 2) {
        update_game_server_setting(gs, parsedData[i], parsedData[i + 1]);
      }
    } else if (str_starts_with(received_reply, "statusresponse", true)) {

      const size_t first_sep_pos{ received_reply.find('\\') };
      if (first_sep_pos == string::npos)
        return noOfAllReceivedBytes;

      received_reply.erase(cbegin(received_reply), cbegin(received_reply) + first_sep_pos + 1);
      size_t new_line_pos{ received_reply.find('\n') };
      if (string::npos == new_line_pos) new_line_pos = received_reply.length();
      const string server_info{ cbegin(received_reply), cbegin(received_reply) + new_line_pos };
      vector<string> parsedData{ str_split(server_info, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no) };
      for (size_t i{}; i + 1 < parsedData.size(); i += 2) {
        update_game_server_setting(gs, std::move(parsedData[i]), std::move(parsedData[i + 1]));
      }

      int player_num{};
      int number_of_online_players{};
      int number_of_offline_players{};
      const char *start{ received_reply.c_str() + new_line_pos + 1 }, *last{ start };
      auto &players_data = gs.get_players_data();
      const char *current{ start };

      while (current < received_reply.c_str() + received_reply.length() && *current) {
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

        players_data[player_num].pid = player_num;
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

      gs.set_number_of_players(player_num);
      gs.set_number_of_online_players(number_of_online_players);
      gs.set_number_of_offline_players(number_of_offline_players);
      prepare_players_data_for_display_of_getstatus_response(gs, false);
    }
  }

  return noOfAllReceivedBytes;
}

void connection_manager::prepare_rcon_command(
  char *buffer,
  const size_t buffer_size,
  const char *rcon_command_to_send,
  const char *rcon_password) const
{
  (void)snprintf(buffer, buffer_size, "\xFF\xFF\xFF\xFFrcon %s %s", rcon_password, rcon_command_to_send);
}

size_t connection_manager::send_rcon_command(
  const string &outgoing_data,
  const char *remote_ip,
  const uint_least16_t remote_port) const
{

  const ip::udp::endpoint dst{ ip::address::from_string(remote_ip), remote_port };
  const size_t sent_bytes = udp_socket_for_rcon_commands.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()), dst);
  if (sent_bytes > 0) {
    main_app.add_to_next_uploaded_data_in_bytes(sent_bytes);
    ++number_of_sent_rcon_commands;
  }
  return sent_bytes;
}

size_t connection_manager::receive_rcon_reply_from_server(
  const char *remote_ip,
  const uint_least16_t remote_port,
  game_server &gs,
  std::string &received_reply,
  const bool) const
{
  char incoming_data_buffer[receive_buffer_size];
  size_t noOfReceivedBytes{}, noOfAllReceivedBytes{};

  received_reply.clear();
  const char *start_needle_to_search_for{ "print\n" };
  const auto start_needle_to_search_for_len{ len("print\n") };

  while (true) {
    const ip::udp::endpoint expected_remote_endpoint{ ip::address::from_string(remote_ip), remote_port };
    ip::udp::endpoint remote_endpoint{};
    asio::error_code erc{};

    noOfReceivedBytes = udp_socket_for_rcon_commands.receive_from(
      buffer(incoming_data_buffer, receive_buffer_size), remote_endpoint, 0, erc);

    if (erc) break;

    incoming_data_buffer[noOfReceivedBytes] = '\0';

    main_app.add_to_next_downloaded_data_in_bytes(noOfReceivedBytes);

    if (remote_endpoint != ip::udp::endpoint{} && expected_remote_endpoint != remote_endpoint) continue;

    if (noOfReceivedBytes > 0U) {

      const size_t start_needle_pos = stl::helper::str_index_of(incoming_data_buffer, start_needle_to_search_for);
      if (start_needle_pos != string::npos) {
        received_reply.append(incoming_data_buffer + start_needle_pos + start_needle_to_search_for_len, incoming_data_buffer + noOfReceivedBytes);
      } else {
        received_reply.append(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);
      }
      const bool is_check_for_two_new_line_characters{ received_reply.find("map_rotate...\n\n") == string::npos && received_reply.find("==== ShutdownGame ====") == string::npos };

      noOfAllReceivedBytes += noOfReceivedBytes;

      if ((is_check_for_two_new_line_characters && received_reply.ends_with("\n\n")) || received_reply.ends_with("-----------------------------------\n-----------------------------------\n")) break;
    }
  }

  if (noOfAllReceivedBytes > 0) {

    const char *start{};
    if (received_reply.starts_with("Invalid password.") || received_reply.starts_with("Bad rcon")) {
      main_app.set_is_connection_settings_valid(false);
      set_admin_actions_buttons_active(FALSE);
    } else {
      const auto gn = main_app.get_game_name();
      const char *current{}, *last{};

      auto [rcon_status_response_needle1, rcon_status_response_needle2] = get_appropriate_rcon_status_response_header(main_app.get_game_name());

      if (received_reply.find(rcon_status_response_needle1) != string::npos
          || (rcon_status_response_needle2 != nullptr && received_reply.find(rcon_status_response_needle2) != string::npos)) {
        if (!main_app.get_is_connection_settings_valid()) {
          main_app.set_is_connection_settings_valid(true);
          set_admin_actions_buttons_active(TRUE);
        }

        start = received_reply.c_str() + 5;
        last = start;
        while (*last != 0x0A && *last != '=')
          ++last;
        string current_map(start, last);
        trim_in_place(current_map);
        if (previous_map.empty() || current_map != previous_map) {
          previous_map = current_map;
          initialize_elements_of_container_to_specified_value(gs.get_players_data(), player{}, 0);
          clear_players_data_in_players_grid(app_handles.hwnd_players_grid, 0, max_players_grid_rows, 7);
        }
        gs.set_current_map(current_map);

        bool is_server_empty_or_error_receiving_udp_datagrams{};

        const auto digit_pos = received_reply.find_first_of(
          "0123456789", (last - received_reply.c_str()) + 1);
        if (digit_pos == string::npos) {
          is_server_empty_or_error_receiving_udp_datagrams = true;
          received_reply.clear();
        } else {
          received_reply.erase(begin(received_reply), begin(received_reply) + digit_pos);
        }

        auto &ip_address_frequency = main_app.get_ip_address_frequency();
        ip_address_frequency.clear();

        int number_of_online_players{};
        int number_of_offline_players{};
        size_t pl_index{};

        if (!is_server_empty_or_error_receiving_udp_datagrams) {

          vector<string> lines{ stl::helper::str_split(received_reply, "\n", nullptr) };
          for (size_t i{}; i < lines.size(); ++i) {
            auto first = lines[i].cbegin();
            auto last2 = lines[i].cend();
            std::smatch ip_matches;
            size_t found_count{};
            vector<std::string::const_iterator> new_line_positions;
            while (regex_search(first, last2, ip_matches, player_ip_address_regex)) {
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

          auto &players_data = gs.get_players_data();

          for (const string &player_line : lines) {
            if (!regex_search(player_line, player_ip_address_regex) && !regex_search(player_line, bot_ip_address_regex)) continue;
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
            int player_score{};

            while (j < i && !isdigit(player_line[j]) && player_line[j] != '-') ++j;

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
            string player_name;

            smatch bot_ip_regex_smatch;
            if (regex_search(player_line, bot_ip_regex_smatch, bot_ip_address_regex)) {
              ip_address = bot_ip_regex_smatch[1];
              size_t last_pos{ player_line.find("^7", j) };
              if (string::npos == last_pos) {
                last_pos = player_line.find_first_of("^7");
              }
              player_name.assign(player_line.cbegin() + j, player_line.cbegin() + last_pos);
              if (!player_name.empty() && ('^' == player_name.back() || '7' == player_name.back())) player_name.pop_back();
            } else {

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

              if (!ip_address.empty() && '0' == ip_address[0]) {
                ip_address.erase(0, 1);
                ++i;
              }

              while (is_ws(player_line[i])) --i;
              while (is_decimal_digit(player_line[i])) --i;
              while (is_ws(player_line[i])) --i;

              player_name.assign(player_line.c_str() + pn_start, player_line.c_str() + i);
              auto pn_end = player_name.find_last_of("^7");
              if (string::npos != pn_end)
                player_name.erase(cbegin(player_name) + pn_end, cend(player_name));
              if (const size_t pn_len{ player_name.length() }; (pn_len >= 1) && (player_name[pn_len - 1] == '^' || player_name[pn_len - 1] == '7')) {
                player_name.pop_back();
              }
            }

            players_data[pl_index].pid = player_pid;
            players_data[pl_index].score = player_score;
            strcpy_s(players_data[pl_index].ping, std::size(players_data[pl_index].ping), player_ping.c_str());
            strcpy_s(players_data[pl_index].guid_key, std::size(players_data[pl_index].guid_key), player_guid.c_str());
            const size_t no_of_chars_to_copy{ std::min<size_t>(32, player_name.length()) };
            strncpy_s(players_data[pl_index].player_name, std::size(players_data[pl_index].player_name), player_name.c_str(), no_of_chars_to_copy);
            players_data[pl_index].player_name[no_of_chars_to_copy] = '\0';

            players_data[pl_index].ip_address = ip_address;
            convert_guid_key_to_country_name(geoip_db, players_data[pl_index].ip_address, players_data[pl_index]);

            unsigned long guid_key{};

            if (check_ip_address_validity(players_data[pl_index].ip_address, guid_key)) {
              ++ip_address_frequency[players_data[pl_index].ip_address];
            }
            ++pl_index;
          }
        }

        gs.set_number_of_players(pl_index);
        gs.set_number_of_online_players(number_of_online_players);
        gs.set_number_of_offline_players(number_of_offline_players);
        ++rcon_status_sent_counter;
        const bool log_players_data{ rcon_status_sent_counter % 60 == 0 };
        if (60 == rcon_status_sent_counter)
          rcon_status_sent_counter = 0;
        prepare_players_data_for_display(gs, log_players_data);


      } else if (received_reply.find("Server info") != string::npos) {
        main_app.get_server_settings().clear();
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
          update_game_server_setting(gs, std::move(property), std::move(value));
        }
      } else if (received_reply.find("map_rotate...\n\n") != string::npos) {
        main_app.set_is_connection_settings_valid(true);
        print_colored_text(app_handles.hwnd_re_messages_data, received_reply.c_str());
      } else if (size_t start_pos{}; (start_pos = received_reply.find(R"("sv_mapRotationCurrent" is: ")")) != string::npos && received_reply.find(R"(default: ")") != string::npos) {
        main_app.set_is_connection_settings_valid(true);
        start_pos += strlen(R"("sv_mapRotationCurrent" is: ")");
        const size_t last_pos{ received_reply.find_first_of("^7\" ", start_pos) };
        if (last_pos != string::npos) {
          gs.set_map_rotation_current(received_reply.substr(start_pos, last_pos - start_pos));
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
              gs.set_next_map(string(start, last));
            }
          }
        }
      } else if (size_t first_pos1{}; (first_pos1 = received_reply.find(R"("sv_mapRotation" is: ")")) != string::npos && received_reply.find(R"(default: ")") != string::npos) {
        main_app.set_is_connection_settings_valid(true);
        first_pos1 += strlen(R"("sv_mapRotation" is: ")");
        const size_t last_pos{ received_reply.find_first_of("^7\" ", first_pos1) };
        if (last_pos != string::npos) {
          gs.set_map_rotation(received_reply.substr(first_pos1, last_pos - first_pos1));
        }
      } else if (received_reply.find("sv_hostname") != string::npos) {
        main_app.set_is_connection_settings_valid(true);
        current = received_reply.c_str() + 29;
        start = last = current;
        while (*last != '^' && *(last + 1) != '7' && *(last + 2) != '"')
          ++last;
        gs.set_server_name(string(start, last));
      } else if (size_t first_pos2{}; (first_pos2 = received_reply.find(R"("mapname" is: ")")) != string::npos && received_reply.find(R"(default: ")") != string::npos) {
        main_app.set_is_connection_settings_valid(true);
        first_pos2 += strlen(R"("mapname" is: ")");
        const size_t last_pos{ received_reply.find_first_of("^7\" ", first_pos2) };
        if (last_pos != string::npos) {
          string current_map(received_reply.substr(first_pos2, last_pos - first_pos2));
          if (previous_map.empty() || current_map != previous_map) {
            previous_map = current_map;
            initialize_elements_of_container_to_specified_value(
              gs.get_players_data(), player{}, 0);
            clear_players_data_in_players_grid(app_handles.hwnd_players_grid, 0, max_players_grid_rows, 7);
          }
          gs.set_current_map(std::move(current_map));
        }
      } else {
        parse_game_type_information_from_rcon_reply(received_reply, gs);
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
  game_server &gs,
  const bool is_wait_for_reply,
  const bool is_process_reply) const
{
  constexpr size_t buffer_size{ 1024 };
  char outgoing_data_buffer[buffer_size]{};

  if (main_app.get_game_server_index() >= main_app.get_rcon_game_servers_count())
    rcon_password = unknown_rcon_password.c_str();
  prepare_rcon_command(outgoing_data_buffer, buffer_size, command_to_send, rcon_password);
  (void)send_rcon_command(outgoing_data_buffer, remote_ip, remote_port);
  if (is_wait_for_reply) {
    receive_rcon_reply_from_server(remote_ip, remote_port, gs, received_reply, is_process_reply);
  }
}

void connection_manager::send_and_receive_non_rcon_data(const char *command_to_send, std::string &reply_buffer, const char *remote_ip, const uint_least16_t remote_port, game_server &gs, const bool is_wait_for_reply, const bool is_process_reply) const
{
  constexpr size_t buffer_size{ 1024 };
  char outgoing_data_buffer[buffer_size]{};

  prepare_non_rcon_command(outgoing_data_buffer, buffer_size, command_to_send);
  (void)send_non_rcon_command(outgoing_data_buffer, remote_ip, remote_port);
  if (is_wait_for_reply) {
    receive_non_rcon_reply_from_server(remote_ip, remote_port, gs, reply_buffer, is_process_reply);
  }
}
