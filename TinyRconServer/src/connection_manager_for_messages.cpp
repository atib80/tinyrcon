#include "connection_manager_for_messages.h"
#include "stl_helper_functions.hpp"
#include "tiny_rcon_server_application.h"
#include "tiny_rcon_utility_functions.h"
#include <utility>

using namespace std;
using namespace stl::helper;

extern tiny_rcon_server_application main_app;
extern tiny_rcon_handles app_handles;

using namespace asio;

connection_manager_for_messages::connection_manager_for_messages() : udp_socket_for_messages{udp_service_for_messages}
{
    try
    {
        if (udp_socket_for_messages.is_open())
        {
            udp_socket_for_messages.close();
        }

        udp_socket_for_messages.open(ip::udp::v4());
        udp_socket_for_messages.set_option(rcv_timeout_option{700});
        asio::ip::udp::endpoint local_endpoint{
            asio::ip::address::from_string(main_app.get_tiny_rcon_server_ip_address()),
            static_cast<asio::ip::port_type>(main_app.get_tiny_rcon_server_port())};
        udp_socket_for_messages.bind(local_endpoint);
    }
    catch (std::exception &ex)
    {
        show_error(app_handles.hwnd_main_window, ex.what(), 0);
    }
}

size_t connection_manager_for_messages::process_and_send_message(
    const std::string &command_name, const std::string &data, const bool is_show_in_messages,
    const std::shared_ptr<tiny_rcon_client_user> &user) const
{
    if (user->remote_endpoint != asio::ip::udp::endpoint{})
    {
        const std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
        const auto t_c = std::chrono::system_clock::to_time_t(now);

        const string outgoing_data{format(R"({}\{}\{}\{}\{})", command_name, user->user_name, t_c,
                                          is_show_in_messages ? "true" : "false", data)};

        const size_t sent_bytes = udp_socket_for_messages.send_to(buffer(outgoing_data.c_str(), outgoing_data.length()),
                                                                  user->remote_endpoint);
        if (sent_bytes > 0u)
            main_app.add_to_next_uploaded_data_in_bytes(sent_bytes);

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
    noOfReceivedBytes = udp_socket_for_messages.receive_from(buffer(incoming_data_buffer, receive_buffer_size),
                                                             remote_endpoint, 0, erc1);
    if (erc1)
    {
        return false;
    }

    if (noOfReceivedBytes > 0u)
        main_app.add_to_next_downloaded_data_in_bytes(noOfReceivedBytes);

    string message(incoming_data_buffer, incoming_data_buffer + noOfReceivedBytes);

    string sender_ip{remote_endpoint.address().to_v4().to_string()};
    string geo_information;
    player pd{};
    unsigned long guid{};
    const bool is_sender_ip_address_valid{check_ip_address_validity(sender_ip, guid)};
    if (is_sender_ip_address_valid)
    {
        pd.ip_address = sender_ip;
        convert_guid_key_to_country_name(main_app.get_geoip_data(), pd.ip_address, pd);
        geo_information = format("{}, {}", pd.country_name, pd.city);
    }

    if (noOfReceivedBytes > 0 && '{' != message.front() && '}' != message.back())
    {
        auto parts =
            stl::helper::str_split(message, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() < 6)
        {
            const string incorrectly_formatted_message_received{
                format("^3Received an incorrectly formatted message from IP address: ^1{} ^5| "
                       "^3geoinfo: ^1{}\n^3Contents of message: ^1'{}'\n",
                       sender_ip, geo_information, message)};
            print_colored_text(app_handles.hwnd_re_messages_data, incorrectly_formatted_message_received.c_str());
            return false;
        }

        const string message_contents{str_join(cbegin(parts) + 5, cend(parts), '\\')};

        int64_t number{};
        if (!is_valid_decimal_whole_number(parts[3], number) || (parts[4] != "true" && parts[4] != "false"))
        {
            const string incorrectly_formatted_message_received{
                format("^3Received an incorrectly formatted message!\n^7{} ^3(IP address: "
                       "^1{} ^5| ^3geoinfo: ^1{} ^5| ^3rcon_password: ^1{}^3) sent the "
                       "following command: ^1'{}'\nContents of message: ^1'{}'\n",
                       parts[1], sender_ip, geo_information, parts[2], parts[0], message_contents)};
            print_colored_text(app_handles.hwnd_re_messages_data, incorrectly_formatted_message_received.c_str());
            return false;
        }

        const string message_handler_name{parts[0]};
        const string sender{parts[1]};
        const time_t timestamp{stoll(parts[3])};
        const bool is_show_in_messages{parts[4] == "true"};

        if (message_handler_name == "query-request")
        {
            // const string information{ format("Received query-request from user {}
            // (IP: {} geoinfo: {})\nMessage contents: {}\n", sender, sender_ip,
            // geo_information, message_contents) };
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // information.c_str());

            if (size_t start{}; (start = message_contents.find("is_user_admin?")) != string::npos)
            {
                const size_t sep_pos{message_contents.rfind('\\')};
                const size_t user_name_start{start + strlen("is_user_admin?")};
                if (sep_pos != string::npos)
                {
                    const string username{trim(message_contents.substr(user_name_start, sep_pos - user_name_start))};
                    const string password{trim(message_contents.substr(sep_pos + 1))};
                    const string response{
                        format("{}={}", message_contents, main_app.is_user_admin(username, password) ? "yes" : "no")};
                    auto user = make_shared<tiny_rcon_client_user>();
                    user->user_name = sender;
                    user->ip_address = sender_ip;
                    user->remote_endpoint = remote_endpoint;
                    process_and_send_message("query-response", response, true, user);
                    // const string information2{ format("Sent query-response to user {}
                    // (IP: {} geoinfo: {})\nMessage contents: {}\n", sender, sender_ip,
                    // geo_information, response) };
                    // print_colored_text(app_handles.hwnd_re_messages_data,
                    // information2.c_str());
                }
            }
            return true;
        }

        if (message_handler_name == "request-welcome-message")
        {
            const string information{format("^5Received ^1request-welcome-message ^5from user ^7{} ^5(^3IP: ^1{} "
                                            "^3geoinfo: ^1{}^5)\n^3Message contents: ^5{}\n",
                                            sender, sender_ip, geo_information, message_contents)};
            // log_message(information, is_log_datetime::yes);
            print_colored_text(app_handles.hwnd_re_messages_data, information.c_str());
            auto parts = stl::helper::str_split(message_contents, "\\", nullptr, split_on_whole_needle_t::yes,
                                                ignore_empty_string_t::no);
            for (auto &part : parts)
                stl::helper::trim_in_place(part);
            if (parts.size() >= 3)
            {
                auto user = make_shared<tiny_rcon_client_user>();
                user->user_name = parts[0];
                user->ip_address = sender_ip;
                user->remote_endpoint = remote_endpoint;
                process_and_send_message("receive-welcome-message", main_app.get_welcome_message(parts[0]), true, user);
            }
            return true;
        }

        if (message_handler_name == "inc-number-of-reports")
        {
            const string information{format("^5Received ^1inc-number-of-reports ^5from user ^7{} ^5(^3IP: ^1{} "
                                            "^3geoinfo: ^1{}^5)\n^3Message contents: ^5{}\n",
                                            sender, sender_ip, geo_information, message_contents)};
            print_colored_text(app_handles.hwnd_re_messages_data, information.c_str());
            // log_message(information, is_log_datetime::yes);
            ++main_app.get_tinyrcon_stats_data().get_no_of_reports();
            print_colored_text(
                app_handles.hwnd_re_messages_data,
                format("^5Number of received reports: ^1{}\n", main_app.get_tinyrcon_stats_data().get_no_of_reports())
                    .c_str());
            return true;
        }

        if (message_handler_name == "request-mapnames")
        {
            const string information{format("^5Received ^1request-mapnames ^5from user ^7{} ^5(^3IP: ^1{} "
                                            "^3geoinfo: ^1{}^5)\n^3Message contents: ^5{}\n",
                                            sender, sender_ip, geo_information, message_contents)};
            print_colored_text(app_handles.hwnd_re_messages_data, information.c_str());
            // log_message(information, is_log_datetime::yes);
            auto user = make_shared<tiny_rcon_client_user>();
            user->user_name = sender;
            user->ip_address = sender_ip;
            user->remote_endpoint = remote_endpoint;
            send_user_available_map_names(user);
            return true;
        }

        if (main_app.get_game_server().get_rcon_password() == parts[2])
        {
            const auto &user = main_app.get_user_for_name(sender, sender_ip);
            user->ip_address = sender_ip;
            user->remote_endpoint = remote_endpoint;
            strcpy_s(pd.player_name, std::size(pd.player_name), sender.c_str());
            // pd.ip_address = user->ip_address;
            user->geo_information = geo_information;
            user->country_code = pd.country_code;
            const auto &message_handler = main_app.get_message_handler(message_handler_name);
            message_handler(sender, timestamp, message_contents, is_show_in_messages, user->ip_address);
            const string information{format("^5Received ^1'{}' ^5from authorized user ^7{} ^5(^3IP: ^1{} ^5| "
                                            "^3geoinfo: ^1{}^5)\n^5Contents of message: ^1'{}'\n",
                                            message_handler_name, sender, sender_ip, geo_information,
                                            message_contents)};
            log_message(information, is_log_datetime::yes);
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // information.c_str());
            return true;
        }
        else
        {
            const string unathorized_message_received{
                format("^3Received an unauthorized message!\n^7{} ^5(^3IP: ^1{} ^5| ^3rcon: "
                       "^1{} ^5| ^3geoinfo: ^1{}^5) sent the following command: "
                       "^1'{}'\n^5Contents of message: ^1'{}'\n",
                       sender, sender_ip, parts[2], geo_information, message_handler_name, message_contents)};
            print_colored_text(app_handles.hwnd_re_messages_data, unathorized_message_received.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, false, true);
        }
    }
    else
    {
        const string incorrectly_formatted_message_received{
            format("^3Received an incorrectly formatted message from ^3IP: ^1{} ^5| "
                   "^3geoinfo: ^1{}\n^5Contents of message: ^1'{}'\n",
                   sender_ip, geo_information, message)};
        print_colored_text(app_handles.hwnd_re_messages_data, incorrectly_formatted_message_received.c_str());
    }

    return false;
}
