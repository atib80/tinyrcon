#include "framework.h"
#include "resource.h"
#include "stack_trace_element.h"

#undef min

#pragma comment(linker, \
    "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace std;
using namespace stl::helper;
using namespace std::string_literals;
using namespace std::chrono;
using namespace std::filesystem;

extern const string program_version{ "1.2.0.7" };

extern std::atomic<bool> is_terminate_program;
extern volatile std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window;
extern string g_message_data_contents;

tiny_rcon_server_application main_app;

PROCESS_INFORMATION pr_info{};
condition_variable exit_flag{};
mutex mu{};
// mutex inform_message_mutex{};

volatile std::atomic<bool> is_display_temporarily_banned_players_data_event{ false };
volatile std::atomic<bool> is_display_permanently_banned_players_data_event{ false };
volatile std::atomic<bool> is_display_banned_ip_address_ranges_data_event{ false };
volatile std::atomic<bool> is_display_banned_cities_data_event{ false };
volatile std::atomic<bool> is_display_banned_countries_data_event{ false };
volatile std::atomic<bool> is_display_protected_ip_addresses_data_event{ false };
volatile std::atomic<bool> is_display_protected_ip_address_ranges_data_event{ false };
volatile std::atomic<bool> is_display_protected_cities_data_event{ false };
volatile std::atomic<bool> is_display_protected_countries_data_event{ false };
volatile std::atomic<bool> is_display_admins_data_event{ false };

extern const int screen_width{ GetSystemMetrics(SM_CXSCREEN) };
extern const int screen_height{ GetSystemMetrics(SM_CYSCREEN) - 30 };
// RECT client_rect{ 0, 0, screen_width, screen_height };
// extern const char *user_help_message =
//   R"(^5For a list of possible commands type ^1help ^5or ^1list user ^5or ^1list rcon ^5in the console.
//^3Type ^1!say "Public message" to all players" ^3[Enter] to send "Public message" to all online players.
//^5Type ^1!tell 12 "Private message" ^5[Enter] to send "Private message" to only player whose pid = ^112
//^3Type ^1s ^3[Enter] in the console to refresh current status of players data.
//^5Type ^1!w 12 optional_reason ^5[Enter] to warn player whose pid = ^112
//        ^5(Player with ^12 warnings ^5is automatically kicked.)
//^3Type ^1!k 12 optional_reason ^3[Enter] to kick player whose pid = ^112
//^5Type ^1!tb 12 24 optional_reason ^5[Enter] to temporarily ban (for 24 hours) IP address of player whose pid = ^112
//^3Type ^1!gb 12 optional_reason ^3[Enter] to ban IP address of player whose pid = ^112
//^5Type ^1!addip 123.123.123.123 optional_reason ^5[Enter] to ban custom IP address (^1123.123.123.123^5)
//^3Type ^1!ub 123.123.123.123 ^3[Enter] to remove temporarily and/or permanently banned IP address.
//^5Type ^1bans ^5[Enter] to see all permanently banned IP addresses.
//^3Type ^1tempbans ^3[Enter] to see all temporarily banned IP addresses.
//^5Type ^1!m mapname gametype ^5[Enter] to load map 'mapname' in 'gametype' mode (^1!m mp_toujane ctf^5)
//^3Type ^1!c [IP:PORT] ^3[Enter] to launch your Call of Duty game and connect to currently configured
//  game server or optionally specified game server address ^1[IP:PORT]
//^5Type ^1!cp [IP:PORT] ^5[Enter] to launch your Call of Duty game and connect to currently configured
//  game server or optionally specified game server address ^1[IP:PORT] using a private slot.
//^3Type ^1q ^3[Enter] to quit the program.
//^5>> Press ^1F1 ^5to sort players data by their 'pid' values in ascending/descending order.
//^3>> Press ^1F2 ^3to sort players data by their 'score' values in ascending/descending order.
//^5>> Press ^1F3 ^5to sort players data by their 'ping' values in ascending/descending order.
//^3>> Press ^1F4 ^3to sort players data by their 'name' values in ascending/descending order.
//^5>> Press ^1F5 ^5to sort players data by their 'IP address' values in ascending/descending order.
//^3>> Press ^1F6 ^3to sort players data by their 'country - city' values in ascending/descending order.
//^5>> Press ^1F8 ^5to refresh current status of players data.
//^3>> Press ^1Ctrl + W ^3to warn player.
//^5>> Press ^1Ctrl + K ^5to kick player.
//^3>> Press ^1Ctrl + T ^3to temp-ban player.
//^5>> Press ^1Ctrl + B ^5to ban IP address of player.
//^3>> Press ^1Ctrl + S ^3to refresh players' data.
//^5>> Press ^1Ctrl + J ^5to connect to game server.
//^3>> Press ^1Ctrl + X ^3to exit TinyRcon.
//^5Type ^1!egb ^5[Enter] to enable city ban (automatic kick for banned cities).
//^3Type ^1!dgb ^3[Enter] to disable city ban (automatic kick for banned cities).
//^5Type ^1!bancity city_name ^5to enable automatic kick for players from city ^1city_name
//^3Type ^1!unbancity city_name ^3to disable automatic kick for players from city ^1city_name
//^5Type ^1!banned cities ^5[Enter] to see all currently ^1banned cities^5.
//^3Type ^1!ecb ^3[Enter] to enable country ban (automatic kick for banned countries).
//^5Type ^1!dcb ^5[Enter] to disable country ban (automatic kick for banned countries).
//^3Type ^1!bancountry country_name ^3to enable automatic kick for players from country ^1country_name
//^5Type ^1!unbancountry country_name ^5to disable automatic kick for players from country ^1country_name
//^3Type ^1!banned countries ^3[Enter] to see all currently ^1banned countries^3.
//)";

extern const std::unordered_map<string, sort_type> sort_mode_names_dict;

extern const std::unordered_map<int, const char *> button_id_to_label_text{
  { ID_WARNBUTTON, "&Warn" },
  { ID_KICKBUTTON, "&Kick" },
  { ID_TEMPBANBUTTON, "&Tempban" },
  { ID_IPBANBUTTON, "&Ban IP" },
  { ID_VIEWTEMPBANSBUTTON, "View temporary bans" },
  { ID_VIEWIPBANSBUTTON, "View &IP bans" },
  { ID_REFRESHDATABUTTON, "Refre&sh data" },
  { ID_SAY_BUTTON, "Send &public message" },
  { ID_TELL_BUTTON, "Send p&rivate message" },
  { ID_QUITBUTTON, "E&xit" },
  { ID_LOADBUTTON, "&Load map" },
  { ID_YES_BUTTON, "Yes" },
  { ID_NO_BUTTON, "No" },
  { ID_BUTTON_SAVE_CHANGES, "Save changes" },
  { ID_BUTTON_TEST_CONNECTION, "Test connection" },
  { ID_BUTTON_CANCEL, "Cancel" },
  { ID_BUTTON_CONFIGURE_SERVER_SETTINGS, "Confi&gure settings" },
  { ID_CLEARMESSAGESCREENBUTTON, "Clear messages" },
  { ID_BUTTON_CONFIGURATION_EXIT_TINYRCON, "Exit TinyRcon" },
  { ID_BUTTON_CONFIGURATION_COD1_PATH, "Browse for codmp.exe" },
  { ID_BUTTON_CONFIGURATION_COD2_PATH, "Browse for cod2mp_s.exe" },
  { ID_BUTTON_CONFIGURATION_COD4_PATH, "Browse for iw3mp.exe" },
  { ID_BUTTON_CONFIGURATION_COD5_PATH, "Browse for cod5mp.exe" }
};


unordered_map<size_t, string> users_table_column_header_titles;
unordered_map<size_t, string> get_status_grid_column_header_titles;

extern const char *prompt_message{ "Administrator >>" };
extern const char *refresh_players_data_fmt_str{ "Refreshing players data in %zu %s." };
extern const size_t max_users_grid_rows;

tiny_rcon_handles app_handles{};

WNDCLASSEX wcex, wcex_confirmation_dialog /*, wcex_configuration_dialog*/;

int selected_row{};
int selected_col{};

extern bool sort_by_pid_asc;
extern bool sort_by_score_asc;
extern bool sort_by_ping_asc;
extern bool sort_by_name_asc;
extern bool sort_by_ip_asc;
extern bool sort_by_geo_asc;

bool is_main_window_constructed{};
bool is_tinyrcon_initialized{};
bool is_process_combobox_item_selection_event{ true };
bool is_first_left_mouse_button_click_in_reason_edit_control{ true };
const HBRUSH RED_BRUSH{ CreateSolidBrush(color::red) };
atomic<int> admin_choice{ 0 };
string admin_reason{ "not specified" };
extern HIMAGELIST hImageList;
HFONT font_for_players_grid_data{};

extern const map<string, string> user_commands_help;

ATOM register_window_classes(HINSTANCE hInstance);
bool initialize_main_app(HINSTANCE, const int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR);
LRESULT CALLBACK WndProcForConfirmationDialog(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE,
  _In_ LPSTR,
  _In_ int nCmdShow)
{
  InitCommonControls();
  LoadLibrary("Riched20.dll");

  HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

  register_window_classes(hInstance);

  if (!initialize_main_app(hInstance, nCmdShow))
    return 0;

  main_app.set_current_working_directory();

  const string config_folder_path{ format("{}{}", main_app.get_current_working_directory(), "config") };
  const string data_folder_path{ format("{}{}", main_app.get_current_working_directory(), "data") };
  const string log_folder_path{ format("{}{}", main_app.get_current_working_directory(), "log") };
  const string plugins_geoIP_folder_path{ format("{}{}", main_app.get_current_working_directory(), "plugins\\geoIP") };
  const string config_file_path{ format("{}{}", main_app.get_current_working_directory(), "config\\tinyrcon.json") };
  const string custom_map_names_file_path{ format("{}{}", main_app.get_current_working_directory(), "data\\custom_map_names.txt") };

  if (!create_necessary_folders_and_files({
        config_folder_path,
        data_folder_path,
        log_folder_path,
        plugins_geoIP_folder_path,
        main_app.get_tinyrcon_config_file_path(),
        custom_map_names_file_path.c_str(),
        main_app.get_temp_bans_file_path(),
        main_app.get_ip_bans_file_path(),
        main_app.get_ip_range_bans_file_path(),
        main_app.get_banned_countries_file_path(),
        main_app.get_banned_cities_file_path(),
        main_app.get_banned_names_file_path(),
        main_app.get_removed_temp_bans_file_path(),
        main_app.get_removed_ip_bans_file_path(),
        main_app.get_removed_ip_range_bans_file_path(),
        main_app.get_removed_banned_cities_file_path(),
        main_app.get_removed_banned_countries_file_path(),
        main_app.get_removed_banned_names_file_path(),
        main_app.get_protected_ip_addresses_file_path(),
        main_app.get_protected_ip_address_ranges_file_path(),
        main_app.get_protected_cities_file_path(),
        main_app.get_protected_countries_file_path(),
      })) {
    show_error(app_handles.hwnd_main_window, "Error creating necessary program folders and files!", 0);
  }

  parse_tinyrcon_tool_config_file(main_app.get_tinyrcon_config_file_path());
  load_available_map_names(custom_map_names_file_path.c_str());

  main_app.add_command_handler({ "cls", "!cls" }, [](const vector<string> &) {
    Edit_SetText(app_handles.hwnd_re_messages_data, "");
    g_message_data_contents.clear();
  });

  main_app.add_command_handler({ "!ranges" }, [](const vector<string> &) {
    is_display_banned_ip_address_ranges_data_event.store(true);
  });

  main_app.add_command_handler({ "!admins" }, [](const vector<string> &) {
    is_display_admins_data_event.store(true);
  });

  main_app.add_command_handler({ "bans", "!bans" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() == 3U && user_cmd[1] == "clear" && user_cmd[2] == "all") {
      for (const auto &ip : main_app.get_game_server().get_banned_ip_addresses_map()) {
        string message;
        string ip_address{ ip.first };
        remove_permanently_banned_ip_address(ip_address);
      }
    }
    is_display_permanently_banned_players_data_event.store(true);
  });

  main_app.add_command_handler({ "tempbans", "!tempbans" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() == 3U && user_cmd[1] == "clear" && user_cmd[2] == "all") {
      for (const auto &ip : main_app.get_game_server().get_temp_banned_ip_addresses_map()) {
        string message;
        remove_temp_banned_ip_address(ip.first);
      }
    }

    is_display_temporarily_banned_players_data_event.store(true);
  });

  main_app.add_command_handler({ "!restart" }, [](const vector<string> &user_cmd) {
    for (const auto &u : main_app.get_users()) {
      unsigned long ip_key{};
      // const string admin_name{ get_cleaned_user_name(u->user_name) };
      if (/*u->user_name.find("^4W^1W") == string::npos && */ check_ip_address_validity(u->ip_address, ip_key)) {
        const string info_message{ format("^5Sent a request to ^1admin ^7{} ^5with IP address ^1{} ^5from ^1{} ^5to restart their ^5Tiny^6Rcon ^5program.\n", u->user_name, u->ip_address, u->geo_information) };
        print_colored_text(app_handles.hwnd_re_messages_data, info_message.c_str());
        main_app.get_connection_manager_for_messages().process_and_send_message("restart-tinyrcon", "^3Restarting your ^5Tiny^6Rcon ^3client program.", true, u);
      }
    }
  });

  main_app.add_message_handler("request-welcome-message", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {

      const auto &current_user = main_app.get_user_for_name(parts[0], sender_ip);

      main_app.get_connection_manager_for_messages().process_and_send_message("receive-welcome-message", main_app.get_welcome_message(parts[0]), true, current_user);
    } });

  main_app.add_message_handler("inform-message", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    string cleaned_data{ data };
    replace_br_with_new_line(cleaned_data);
    print_colored_text(app_handles.hwnd_re_messages_data, cleaned_data.c_str());
    auto parts = stl::helper::str_split(cleaned_data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 2 && !parts[0].empty() && !parts[1].empty()) {
      const auto &sent_by{ main_app.get_user_for_name(parts[0], sender_ip) };
      string cleaned_message{ parts[1] };
      remove_all_color_codes(cleaned_message);
      auto &sent_rcon_public_messages{ main_app.get_sent_rcon_public_messages() };
      const auto current_ts{ get_current_time_stamp() };
      if (size_t start, next; (start = cleaned_message.find("expires in ")) != string::npos && (next = cleaned_message.rfind("Date of ban:")) != string::npos) {
        const size_t dot_pos{ cleaned_message.find('.', start + len("expires in ")) };
        if (dot_pos != string::npos) {
          cleaned_message.erase(begin(cleaned_message) + start + len("expires in "), begin(cleaned_message) + dot_pos);
        }

        next = cleaned_message.rfind("Date of ban:");
        if (next != string::npos) {
          cleaned_message.erase(begin(cleaned_message) + next + len("Date of ban:"), end(cleaned_message));
        }
      } else if (size_t next{}; (start = cleaned_message.find("[start date:")) != string::npos && (next = cleaned_message.find("expired on")) != string::npos) {
        const size_t last{ cleaned_message.find(']', next + len("expired on")) };
        if (last != string::npos) {
          cleaned_message.erase(begin(cleaned_message) + start, begin(cleaned_message) + last + 1);
        }
      }

      if (!sent_rcon_public_messages.contains(cleaned_message)) {
        sent_rcon_public_messages.emplace(cleaned_message, 0);
      }

      if (current_ts - sent_rcon_public_messages[cleaned_message] >= 5) {
        if (cleaned_data.find("automatically") != string::npos && cleaned_data.find("kicked") != string::npos) {
          ++main_app.get_tinyrcon_stats_data().get_no_of_autokicks();
        } else if (cleaned_data.find("reported") != string::npos) {
          ++main_app.get_tinyrcon_stats_data().get_no_of_reports();
        }
        sent_rcon_public_messages[cleaned_message] = current_ts;
        main_app.get_connection_manager_for_messages().process_and_send_message("inform-message", std::format("accept\\{}", cleaned_data), true, sent_by);
      } else {
        main_app.get_connection_manager_for_messages().process_and_send_message("inform-message", std::format("deny\\{}", cleaned_data), true, sent_by);
      }
    }
  });

  main_app.add_message_handler("public-message", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    print_colored_text(app_handles.hwnd_re_messages_data, data.c_str());
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 2) {
      const string sent_by{ get_cleaned_user_name(parts[0]) };
      for (const auto &u : main_app.get_users()) {
        const string admin_name{ get_cleaned_user_name(u->user_name) };
        unsigned long ip_key{};
        if (sent_by != admin_name && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("public-message", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("private-message", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    print_colored_text(app_handles.hwnd_re_messages_data, data.c_str());
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {
      const string sent_to{ get_cleaned_user_name(parts[1]) };
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        const string admin_name{ get_cleaned_user_name(u->user_name) };
        if (admin_name == sent_to && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("private-message", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("tinyrcon-info", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    print_colored_text(app_handles.hwnd_re_messages_data, format("^5Received message from ^7{} (^5IP address: ^1{}^5):\n", user, sender_ip).c_str());
    print_colored_text(app_handles.hwnd_re_messages_data, data.c_str());
  });

  main_app.add_message_handler("heartbeat", [](const string &user, const time_t timestamp, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 2) {
      auto &current_user = main_app.get_user_for_name(parts[0], sender_ip);
      player pd{};
      convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), current_user->ip_address, pd);
      current_user->geo_information = format("{}, {}", pd.country_name, pd.city);
      current_user->country_code = pd.country_code;
      current_user->is_logged_in = true;
    }
  });

  main_app.add_message_handler("request-login", [](const string &user, const time_t timestamp, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {
      auto &current_user = main_app.get_user_for_name(parts[0], sender_ip);
      for (const auto& u : main_app.get_users()) {
        if (sender_ip == u->ip_address) {
          u->is_logged_in = false;
          u->is_online = false;
        }        
      }
      // if (!current_user->is_logged_in) {
      player pd{};
      convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), current_user->ip_address, pd);
      current_user->geo_information = format("{}, {}", pd.country_name, pd.city);
      current_user->country_code = pd.country_code;
      current_user->no_of_logins++;
      current_user->is_logged_in = true;
      current_user->last_login_time_stamp = get_current_time_stamp();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      main_app.add_message_to_queue(message_t("confirm-login", format("{}\\{}\\{}", current_user->user_name, current_user->ip_address, current_user->no_of_logins), current_user, true));
      main_app.get_connection_manager_for_messages().process_and_send_message("receive-welcome-message", main_app.get_welcome_message(current_user->user_name), true, current_user);
      string message{ format("^7{} ^2has logged in to ^5Tiny^6Rcon ^5server.\n^2Number of logins: ^1{}\n", current_user->user_name, current_user->no_of_logins) };
      if (parts.size() >= 4) {
        message += format("^2Player name: ^7{}\n", parts[3]);
      }
      if (parts.size() >= 5) {
        message += format("^2Game's version number: ^1{}\n", parts[4]);
      }
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != current_user.get() && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("inform-login", message, true, u);
        }
      }
      const string message1{ format("^7{} ^3has sent a ^1login request ^3to ^5Tiny^6Rcon ^5server.", user) };
      print_colored_text(app_handles.hwnd_re_messages_data, message1.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
      // }
    }
  });

  main_app.add_message_handler("request-logout", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 2) {
      const auto &current_user = main_app.get_user_for_name(parts[0], sender_ip);

      for (const auto &u : main_app.get_users()) {
        if (sender_ip == u->ip_address) {
          u->is_logged_in = false;
          u->is_online = false;
        }
      }

      // if (current_user->is_logged_in) {
      const string message1{ format("^7{} ^3has sent a ^1logout request ^3to ^5Tiny^6Rcon ^5server.", user) };
      print_colored_text(app_handles.hwnd_re_messages_data, message1.c_str());
      player pd{};
      convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), current_user->ip_address, pd);
      current_user->geo_information = format("{}, {}", pd.country_name, pd.city);
      current_user->country_code = pd.country_code;
      current_user->is_logged_in = false;
      current_user->last_logout_time_stamp = get_current_time_stamp();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      main_app.get_connection_manager_for_messages().process_and_send_message("confirm-logout", format("{}\\{}\\{}", current_user->user_name, current_user->ip_address, current_user->no_of_logins), true, current_user);
      const string message2{ format("^7{} ^2has logged out of ^5Tiny^6Rcon ^5server.\n", current_user->user_name) };
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != current_user.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("inform-logout", message2, true, u);
        }
      }
      print_colored_text(app_handles.hwnd_re_messages_data, message2.c_str());
      // }
    }
  });

  // request-tempbans
  main_app.add_message_handler("request-tempbans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    const auto &temp_banned_players =
      main_app.get_game_server().get_temp_banned_players_data();
    const size_t start_index{ 50U <= temp_banned_players.size() ? temp_banned_players.size() - 50U : 0U };
    auto &admin = main_app.get_user_for_name(user, sender_ip);
    admin->is_logged_in = true;
    print_colored_text(app_handles.hwnd_re_messages_data, format("^5Received request from ^7{} ^5to send them the ^1last {} temporary ban ^5entries.", admin->user_name, temp_banned_players.size() < 50U ? temp_banned_players.size() : 50U).c_str());
    size_t message_length{};
    ostringstream oss;
    for (size_t i{ start_index }; i < temp_banned_players.size(); ++i) {
      const auto &tb_player = temp_banned_players[i];
      const string data_line{ format("{}\\{}\\{}\\{}\\{}\\{}\\{}\n", tb_player.ip_address, tb_player.player_name, tb_player.banned_date_time, tb_player.banned_start_time, tb_player.ban_duration_in_hours, remove_disallowed_character_in_string(tb_player.reason), tb_player.banned_by_user_name) };
      oss << data_line;
      message_length += data_line.length();
      if (message_length > 1300) {
        main_app.add_message_to_queue(message_t("receive-tempbans", oss.str(), admin));
        oss.str(string{});
        oss.clear();
        message_length = 0U;
      }
    }

    if (message_length > 0U) {
      const string last_data_line{ oss.str() };
      main_app.add_message_to_queue(message_t("receive-tempbans", last_data_line, admin));
    }
  });

  // request-ipaddressbans
  main_app.add_message_handler("request-ipaddressbans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    const auto &banned_ip_addresses =
      main_app.get_game_server().get_banned_ip_addresses_vector();
    const size_t start_index{ 50U <= banned_ip_addresses.size() ? banned_ip_addresses.size() - 50U : 0U };
    auto &admin = main_app.get_user_for_name(user, sender_ip);
    admin->is_logged_in = true;
    print_colored_text(app_handles.hwnd_re_messages_data, format("^5Received request from ^7{} ^5to send them the ^1last {} IP address ban ^5entries.", admin->user_name, banned_ip_addresses.size() < 50U ? banned_ip_addresses.size() : 50U).c_str());
    size_t message_length{};
    ostringstream oss;
    for (size_t i{ start_index }; i < banned_ip_addresses.size(); ++i) {
      const auto &banned_player = banned_ip_addresses[i];
      const string data_line{ format("{}\\{}\\{}\\{}\\{}\\{}\n", banned_player.ip_address, banned_player.guid_key, banned_player.player_name, banned_player.banned_date_time, remove_disallowed_character_in_string(banned_player.reason), banned_player.banned_by_user_name) };
      oss << data_line;
      message_length += data_line.length();
      if (message_length > 1300) {
        main_app.add_message_to_queue(message_t("receive-ipaddressbans", oss.str(), admin));
        oss.str(string{});
        oss.clear();
        message_length = 0U;
      }
    }

    if (message_length > 0U) {
      const string last_data_line{ oss.str() };
      main_app.add_message_to_queue(message_t("receive-ipaddressbans", last_data_line, admin));
    }
  });

  // request-ipaddressrangebans
  main_app.add_message_handler("request-ipaddressrangebans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    const auto &banned_ip_address_ranges =
      main_app.get_game_server().get_banned_ip_address_ranges_vector();
    const size_t start_index{ 50U <= banned_ip_address_ranges.size() ? banned_ip_address_ranges.size() - 50U : 0U };
    auto &admin = main_app.get_user_for_name(user, sender_ip);
    admin->is_logged_in = true;
    print_colored_text(app_handles.hwnd_re_messages_data, format("^5Received request from ^7{} ^5to send them the ^1last {} IP address range ban ^5entries.", admin->user_name, banned_ip_address_ranges.size() < 50U ? banned_ip_address_ranges.size() : 50U).c_str());
    size_t message_length{};
    ostringstream oss;
    for (size_t i{ start_index }; i < banned_ip_address_ranges.size(); ++i) {
      const auto &banned_player = banned_ip_address_ranges[i];
      string ip{ banned_player.ip_address };
      if (!ip.ends_with(".*")) {
        const size_t last_dot_pos{ ip.rfind('.') };
        if (last_dot_pos != string::npos) {
          ip.replace(cbegin(ip) + last_dot_pos, cend(ip), ".*");
        }
      }
      const string data_line{ format("{}\\{}\\{}\\{}\\{}\\{}\n", ip, banned_player.guid_key, banned_player.player_name, banned_player.banned_date_time, remove_disallowed_character_in_string(banned_player.reason), banned_player.banned_by_user_name) };
      oss << data_line;
      message_length += data_line.length();
      if (message_length > 1300) {
        main_app.add_message_to_queue(message_t("receive-ipaddressrangebans", oss.str(), admin));
        oss.str(string{});
        oss.clear();
        message_length = 0U;
      }
    }

    if (message_length > 0U) {
      const string last_data_line{ oss.str() };
      main_app.add_message_to_queue(message_t("receive-ipaddressrangebans", last_data_line, admin));
    }
  });

  // request-namebans
  main_app.add_message_handler("request-namebans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    const auto &banned_names =
      main_app.get_game_server().get_banned_names_vector();
    const size_t start_index{ 50U <= banned_names.size() ? banned_names.size() - 50U : 0U };
    auto &admin = main_app.get_user_for_name(user, sender_ip);
    admin->is_logged_in = true;
    print_colored_text(app_handles.hwnd_re_messages_data, format("^5Received request from ^7{} ^5to send them the ^1last {} banned name ^5entries.", admin->user_name, banned_names.size() < 50U ? banned_names.size() : 50U).c_str());
    size_t message_length{};
    ostringstream oss;
    for (size_t i{ start_index }; i < banned_names.size(); ++i) {
      const auto &banned_player = banned_names[i];
      const string data_line{ format("{}\\{}\\{}\\{}\\{}\\{}\n", banned_player.ip_address, banned_player.guid_key, banned_player.player_name, banned_player.banned_date_time, remove_disallowed_character_in_string(banned_player.reason), banned_player.banned_by_user_name) };
      oss << data_line;
      message_length += data_line.length();
      if (message_length > 1300) {
        main_app.add_message_to_queue(message_t("receive-namebans", oss.str(), admin));
        oss.str(string{});
        oss.clear();
        message_length = 0U;
      }
    }

    if (message_length > 0U) {
      const string last_data_line{ oss.str() };
      main_app.add_message_to_queue(message_t("receive-namebans", last_data_line, admin));
    }
  });

  // request-citybans
  main_app.add_message_handler("request-citybans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    const auto &banned_cities =
      main_app.get_game_server().get_banned_cities_set();
    auto &admin = main_app.get_user_for_name(user, sender_ip);
    admin->is_logged_in = true;
    print_colored_text(app_handles.hwnd_re_messages_data, format("^5Received request from ^7{} ^5to send them ^1all of the banned city ^5entries.", admin->user_name).c_str());
    size_t message_length{};
    ostringstream oss;
    for (const auto &banned_city : banned_cities) {
      const string data_line{ format("{}\n", banned_city) };
      oss << data_line;
      message_length += data_line.length();
      if (message_length > 1300) {
        main_app.add_message_to_queue(message_t("receive-citybans", oss.str(), admin));
        oss.str(string{});
        oss.clear();
        message_length = 0U;
      }
    }

    if (message_length > 0U) {
      const string last_data_line{ oss.str() };
      main_app.add_message_to_queue(message_t("receive-citybans", last_data_line, admin));
    }
  });

  // request-countrybans
  main_app.add_message_handler("request-countrybans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    const auto &banned_countries =
      main_app.get_game_server().get_banned_countries_set();
    auto &admin = main_app.get_user_for_name(user, sender_ip);
    admin->is_logged_in = true;
    print_colored_text(app_handles.hwnd_re_messages_data, format("^5Received request from ^7{} ^5to send them all ^1all of the banned country ^5entries.", admin->user_name).c_str());
    size_t message_length{};
    ostringstream oss;
    for (const auto &banned_country : banned_countries) {
      const string data_line{ format("{}\n", banned_country) };
      oss << data_line;
      message_length += data_line.length();
      if (message_length > 1300) {
        main_app.add_message_to_queue(message_t("receive-countrybans", oss.str(), admin));
        oss.str(string{});
        oss.clear();
        message_length = 0U;
      }
    }

    if (message_length > 0U) {
      const string last_data_line{ oss.str() };
      main_app.add_message_to_queue(message_t("receive-countrybans", last_data_line, admin));
    }
  });

  main_app.add_message_handler("query-response", [](const string &, const time_t, const string &, bool, const string &) {
    /*print_colored_text(app_handles.hwnd_re_messages_data, format("Received query request from ^5Tiny^6Rcon ^7user {} (IP: {})\nMessage contents: '^5{}^7'\n", user, sender_ip, data).c_str());*/
  });

  main_app.add_message_handler("request-admindata", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    auto &user_data = main_app.get_user_for_name(user, sender_ip);
    user_data->is_logged_in = true;
    const auto &users = main_app.get_users();
    if (!users.empty()) {
      for (size_t i{}; i < users.size(); ++i) {
        const string admin_data{ format(R"({}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{})", users[i]->user_name, (users[i]->is_admin ? "true" : "false"), (users[i]->is_logged_in ? "true" : "false"), (users[i]->is_online ? "true" : "false"), users[i]->ip_address, users[i]->geo_information, users[i]->last_login_time_stamp, users[i]->last_logout_time_stamp, users[i]->no_of_logins, users[i]->no_of_warnings, users[i]->no_of_kicks, users[i]->no_of_tempbans, users[i]->no_of_guidbans, users[i]->no_of_ipbans, users[i]->no_of_iprangebans, users[i]->no_of_citybans, users[i]->no_of_countrybans, users[i]->no_of_namebans) };
        main_app.add_message_to_queue(message_t("receive-admindata", admin_data, user_data));
      }
    }
  });


  main_app.add_message_handler("upload-bans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages, const string &sender_ip) {
    const string admins_bans_compressed_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
    if (!check_if_file_path_exists(admins_bans_compressed_file_path.c_str())) {
      print_colored_text(app_handles.hwnd_re_messages_data, format("^3Admin's ^1{} ^3file is not found at ^1{}", admins_bans_compressed_file_path, main_app.get_ftp_bans_folder_path()).c_str());
      return;
    }

    const unordered_map<string, string> data_files{
      { "tempbans", format("{}data\\tempbans.txt", main_app.get_current_working_directory()) }, { "bans", format("{}data\\bans.txt", main_app.get_current_working_directory()) }, { "ip_range_bans", format("{}data\\ip_range_bans.txt", main_app.get_current_working_directory()) }, { "banned_cities", format("{}data\\banned_cities.txt", main_app.get_current_working_directory()) }, { "banned_countries", format("{}data\\banned_countries.txt", main_app.get_current_working_directory()) }, { "banned_names", format("{}data\\banned_names.txt", main_app.get_current_working_directory()) }
    };

    const size_t first_digit_pos{ data.find_first_of("0123456789") };
    if (first_digit_pos == string::npos) return;
    const string file_name_key{ data.substr(0, first_digit_pos) };

    if (data_files.contains(file_name_key)) {

      if (file_name_key == "tempbans") {
        const string admins_tempbans_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
        vector<player> admins_tempbanned_ip_addresses_data;
        unordered_map<string, player> admins_tempbanned_ip_to_player_data;
        parse_tempbans_data_file(admins_tempbans_file_path.c_str(), admins_tempbanned_ip_addresses_data, admins_tempbanned_ip_to_player_data);
        auto &tiny_rcon_server_tempbanned_players_vector = main_app.get_game_server().get_temp_banned_players_data();
        auto &tiny_rcon_server_tempbanned_players_map = main_app.get_game_server().get_temp_banned_ip_addresses_map();
        for (auto &pd : admins_tempbanned_ip_addresses_data) {
          if (!tiny_rcon_server_tempbanned_players_map.contains(pd.ip_address)) {
            tiny_rcon_server_tempbanned_players_map.emplace(pd.ip_address, pd);
            tiny_rcon_server_tempbanned_players_vector.push_back(std::move(pd));
          }
        }

        std::sort(std::begin(tiny_rcon_server_tempbanned_players_vector), std::end(tiny_rcon_server_tempbanned_players_vector), [](const player &pd1, const player &pd2) {
          return pd1.banned_start_time < pd2.banned_start_time;
        });

        save_tempbans_to_file(admins_tempbans_file_path.c_str(), tiny_rcon_server_tempbanned_players_vector);
        save_tempbans_to_file(main_app.get_temp_bans_file_path(), tiny_rcon_server_tempbanned_players_vector);

      } else if (file_name_key == "bans") {
        const string admins_ip_bans_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
        vector<player> admins_banned_ip_addresses_data;
        unordered_map<string, player> admins_banned_ip_to_player_data;
        parse_banned_ip_addresses_file(admins_ip_bans_file_path.c_str(), admins_banned_ip_addresses_data, admins_banned_ip_to_player_data);
        auto &tiny_rcon_server_banned_ip_addresses_vector = main_app.get_game_server().get_banned_ip_addresses_vector();
        auto &tiny_rcon_server_banned_ip_addresses_map = main_app.get_game_server().get_banned_ip_addresses_map();
        for (auto &pd : admins_banned_ip_addresses_data) {
          if (!tiny_rcon_server_banned_ip_addresses_map.contains(pd.ip_address)) {
            tiny_rcon_server_banned_ip_addresses_map.emplace(pd.ip_address, pd);
            tiny_rcon_server_banned_ip_addresses_vector.push_back(std::move(pd));
          }
        }

        std::sort(std::begin(tiny_rcon_server_banned_ip_addresses_vector), std::end(tiny_rcon_server_banned_ip_addresses_vector), [](const player &pd1, const player &pd2) {
          return pd1.banned_start_time < pd2.banned_start_time;
        });

        save_banned_ip_entries_to_file(admins_ip_bans_file_path.c_str(), tiny_rcon_server_banned_ip_addresses_vector);
        save_banned_ip_entries_to_file(main_app.get_ip_bans_file_path(), tiny_rcon_server_banned_ip_addresses_vector);
      } else if (file_name_key == "ip_range_bans") {
        const string admins_ip_range_bans_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
        vector<player> admins_banned_ip_address_ranges_data;
        unordered_map<string, player> admins_banned_ip_range_to_player_data;
        parse_banned_ip_address_ranges_file(admins_ip_range_bans_file_path.c_str(), admins_banned_ip_address_ranges_data, admins_banned_ip_range_to_player_data);
        auto &tiny_rcon_server_banned_ip_address_ranges_vector = main_app.get_game_server().get_banned_ip_address_ranges_vector();
        auto &tiny_rcon_server_banned_ip_address_ranges_map = main_app.get_game_server().get_banned_ip_address_ranges_map();
        for (auto &pd : admins_banned_ip_address_ranges_data) {
          if (!tiny_rcon_server_banned_ip_address_ranges_map.contains(pd.ip_address)) {
            tiny_rcon_server_banned_ip_address_ranges_map.emplace(pd.ip_address, pd);
            tiny_rcon_server_banned_ip_address_ranges_vector.push_back(std::move(pd));
          }
        }

        std::sort(std::begin(tiny_rcon_server_banned_ip_address_ranges_vector), std::end(tiny_rcon_server_banned_ip_address_ranges_vector), [](const player &pd1, const player &pd2) {
          return pd1.banned_start_time < pd2.banned_start_time;
        });

        save_banned_ip_address_range_entries_to_file(admins_ip_range_bans_file_path.c_str(), tiny_rcon_server_banned_ip_address_ranges_vector);
        save_banned_ip_address_range_entries_to_file(main_app.get_ip_range_bans_file_path(), tiny_rcon_server_banned_ip_address_ranges_vector);
      } else if (file_name_key == "banned_cities") {
        const string admins_banned_cities_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
        set<string> admins_banned_cities_set;
        parse_banned_cities_file(admins_banned_cities_file_path.c_str(), admins_banned_cities_set);
        auto &tiny_rcon_server_banned_cities_set = main_app.get_game_server().get_banned_cities_set();
        for (const auto &banned_city : admins_banned_cities_set) {
          if (!tiny_rcon_server_banned_cities_set.contains(banned_city)) {
            tiny_rcon_server_banned_cities_set.emplace(banned_city);
          }
        }

        save_banned_cities_to_file(admins_banned_cities_file_path.c_str(), tiny_rcon_server_banned_cities_set);
        save_banned_cities_to_file(main_app.get_banned_cities_file_path(), tiny_rcon_server_banned_cities_set);

      } else if (file_name_key == "banned_countries") {
        const string admins_banned_countries_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
        set<string> admins_banned_countries_set;
        parse_banned_countries_file(admins_banned_countries_file_path.c_str(), admins_banned_countries_set);
        auto &tiny_rcon_server_banned_countries_set = main_app.get_game_server().get_banned_countries_set();
        for (const auto &banned_country : admins_banned_countries_set) {
          if (!tiny_rcon_server_banned_countries_set.contains(banned_country)) {
            tiny_rcon_server_banned_countries_set.emplace(banned_country);
          }
        }

        save_banned_countries_to_file(admins_banned_countries_file_path.c_str(), tiny_rcon_server_banned_countries_set);
        save_banned_countries_to_file(main_app.get_banned_countries_file_path(), tiny_rcon_server_banned_countries_set);
      } else if (file_name_key == "banned_names") {
        const string admins_name_bans_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
        vector<player> admins_banned_names_vector;
        unordered_map<string, player> admins_banned_names_map;
        parse_banned_names_file(admins_name_bans_file_path.c_str(), admins_banned_names_vector, admins_banned_names_map);
        auto &tiny_rcon_server_banned_names_vector = main_app.get_game_server().get_banned_names_vector();
        auto &tiny_rcon_server_banned_names_map = main_app.get_game_server().get_banned_names_map();
        for (auto &pd : admins_banned_names_vector) {
          if (!tiny_rcon_server_banned_names_map.contains(pd.player_name)) {
            tiny_rcon_server_banned_names_map.emplace(pd.player_name, pd);
            tiny_rcon_server_banned_names_vector.push_back(std::move(pd));
          }
        }

        std::sort(std::begin(tiny_rcon_server_banned_names_vector), std::end(tiny_rcon_server_banned_names_vector), [](const player &pd1, const player &pd2) {
          return pd1.banned_start_time < pd2.banned_start_time;
        });

        save_banned_ip_entries_to_file(admins_name_bans_file_path.c_str(), tiny_rcon_server_banned_names_vector);
        save_banned_ip_entries_to_file(main_app.get_banned_names_file_path(), tiny_rcon_server_banned_names_vector);
      }

      // 3. send "receive-bans" message to tinyrcon client
      const auto &sender_user = main_app.get_user_for_name(user, sender_ip);
      main_app.add_message_to_queue(message_t("receive-bans", data, sender_user, true));
    }
  });

  main_app.add_message_handler("upload-bans-compressed", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    const string admins_bans_compressed_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
    if (!check_if_file_path_exists(admins_bans_compressed_file_path.c_str())) {
      print_colored_text(app_handles.hwnd_re_messages_data, format("^3Admin ^7{}'s ^1{} ^3file is not found at ^1{}", user, admins_bans_compressed_file_path, main_app.get_ftp_bans_folder_path()).c_str());
      return;
    }
    const string temp_folder_path{ format("{}temp\\", main_app.get_current_working_directory()) };

    const string temp_tempbans_file_path{ format("{}tempbans.txt", temp_folder_path) };
    if (check_if_file_path_exists(temp_tempbans_file_path.c_str())) {
      DeleteFileA(temp_tempbans_file_path.c_str());
    }

    const string temp_bans_file_path{ format("{}bans.txt", temp_folder_path) };
    if (check_if_file_path_exists(temp_bans_file_path.c_str())) {
      DeleteFileA(temp_bans_file_path.c_str());
    }

    const string temp_ip_range_bans_file_path{ format("{}ip_range_bans.txt", temp_folder_path) };
    if (check_if_file_path_exists(temp_ip_range_bans_file_path.c_str())) {
      DeleteFileA(temp_ip_range_bans_file_path.c_str());
    }

    const string temp_banned_cities_file_path{ format("{}banned_cities.txt", temp_folder_path) };
    if (check_if_file_path_exists(temp_banned_cities_file_path.c_str())) {
      DeleteFileA(temp_banned_cities_file_path.c_str());
    }

    const string temp_banned_countries_file_path{ format("{}banned_countries.txt", temp_folder_path) };
    if (check_if_file_path_exists(temp_banned_countries_file_path.c_str())) {
      DeleteFileA(temp_banned_countries_file_path.c_str());
    }

    const string temp_banned_names_file_path{ format("{}banned_names.txt", temp_folder_path) };
    if (check_if_file_path_exists(temp_banned_names_file_path.c_str())) {
      DeleteFileA(temp_banned_names_file_path.c_str());
    }

    auto [status, error_msg] = extract_7z_file_to_specified_path(admins_bans_compressed_file_path.c_str(), temp_folder_path.c_str());
    if (!status) {
      const string message{ format("^3Error extracting ^1admin's {} ^3file to ^1'{}'\n^3Error message: ^1{}", data, temp_folder_path, error_msg) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
      return;
    }

    const string admins_tempbans_file_path{ format("{}temp\\tempbans.txt", main_app.get_current_working_directory()) };
    if (check_if_file_path_exists(admins_tempbans_file_path.c_str())) {
      vector<player> admins_tempbanned_ip_addresses_data;
      unordered_map<string, player> admins_tempbanned_ip_to_player_data;
      parse_tempbans_data_file(admins_tempbans_file_path.c_str(), admins_tempbanned_ip_addresses_data, admins_tempbanned_ip_to_player_data);
      auto &tiny_rcon_server_tempbanned_players_vector = main_app.get_game_server().get_temp_banned_players_data();
      auto &tiny_rcon_server_tempbanned_players_map = main_app.get_game_server().get_temp_banned_ip_addresses_map();
      for (auto &pd : admins_tempbanned_ip_addresses_data) {
        if (!tiny_rcon_server_tempbanned_players_map.contains(pd.ip_address)) {
          tiny_rcon_server_tempbanned_players_map.emplace(pd.ip_address, pd);
          tiny_rcon_server_tempbanned_players_vector.push_back(std::move(pd));
        }
      }

      std::sort(std::begin(tiny_rcon_server_tempbanned_players_vector), std::end(tiny_rcon_server_tempbanned_players_vector), [](const player &pd1, const player &pd2) {
        return pd1.banned_start_time < pd2.banned_start_time;
      });

      save_tempbans_to_file(admins_tempbans_file_path.c_str(), tiny_rcon_server_tempbanned_players_vector);
      save_tempbans_to_file(main_app.get_temp_bans_file_path(), tiny_rcon_server_tempbanned_players_vector);
    }


    const string admins_ip_bans_file_path{ format("{}temp\\bans.txt", main_app.get_current_working_directory()) };
    if (check_if_file_path_exists(admins_ip_bans_file_path.c_str())) {
      vector<player> admins_banned_ip_addresses_data;
      unordered_map<string, player> admins_banned_ip_to_player_data;
      parse_banned_ip_addresses_file(admins_ip_bans_file_path.c_str(), admins_banned_ip_addresses_data, admins_banned_ip_to_player_data);
      auto &tiny_rcon_server_banned_ip_addresses_vector = main_app.get_game_server().get_banned_ip_addresses_vector();
      auto &tiny_rcon_server_banned_ip_addresses_map = main_app.get_game_server().get_banned_ip_addresses_map();
      for (auto &pd : admins_banned_ip_addresses_data) {
        if (!tiny_rcon_server_banned_ip_addresses_map.contains(pd.ip_address)) {
          tiny_rcon_server_banned_ip_addresses_map.emplace(pd.ip_address, pd);
          tiny_rcon_server_banned_ip_addresses_vector.push_back(std::move(pd));
        }
      }

      std::sort(std::begin(tiny_rcon_server_banned_ip_addresses_vector), std::end(tiny_rcon_server_banned_ip_addresses_vector), [](const player &pd1, const player &pd2) {
        return pd1.banned_start_time < pd2.banned_start_time;
      });

      save_banned_ip_entries_to_file(admins_ip_bans_file_path.c_str(), tiny_rcon_server_banned_ip_addresses_vector);
      save_banned_ip_entries_to_file(main_app.get_ip_bans_file_path(), tiny_rcon_server_banned_ip_addresses_vector);
    }

    // process admin's banned IP address range entries
    const string admins_ip_range_bans_file_path{ format("{}temp\\ip_range_bans.txt", main_app.get_current_working_directory()) };
    if (check_if_file_path_exists(admins_ip_range_bans_file_path.c_str())) {
      vector<player> admins_banned_ip_address_ranges_data;
      unordered_map<string, player> admins_banned_ip_range_to_player_data;
      parse_banned_ip_address_ranges_file(admins_ip_range_bans_file_path.c_str(), admins_banned_ip_address_ranges_data, admins_banned_ip_range_to_player_data);
      auto &tiny_rcon_server_banned_ip_address_ranges_vector = main_app.get_game_server().get_banned_ip_address_ranges_vector();
      auto &tiny_rcon_server_banned_ip_address_ranges_map = main_app.get_game_server().get_banned_ip_address_ranges_map();
      for (auto &pd : admins_banned_ip_address_ranges_data) {
        if (!tiny_rcon_server_banned_ip_address_ranges_map.contains(pd.ip_address)) {
          tiny_rcon_server_banned_ip_address_ranges_map.emplace(pd.ip_address, pd);
          tiny_rcon_server_banned_ip_address_ranges_vector.push_back(std::move(pd));
        }
      }

      std::sort(std::begin(tiny_rcon_server_banned_ip_address_ranges_vector), std::end(tiny_rcon_server_banned_ip_address_ranges_vector), [](const player &pd1, const player &pd2) {
        return pd1.banned_start_time < pd2.banned_start_time;
      });

      save_banned_ip_address_range_entries_to_file(admins_ip_range_bans_file_path.c_str(), tiny_rcon_server_banned_ip_address_ranges_vector);
      save_banned_ip_address_range_entries_to_file(main_app.get_ip_range_bans_file_path(), tiny_rcon_server_banned_ip_address_ranges_vector);
    }

    // process admin's banned city entries
    const string admins_banned_cities_file_path{ format("{}temp\\banned_cities.txt", main_app.get_current_working_directory()) };
    if (check_if_file_path_exists(admins_banned_cities_file_path.c_str())) {
      set<string> admins_banned_cities_set;
      parse_banned_cities_file(admins_banned_cities_file_path.c_str(), admins_banned_cities_set);
      auto &tiny_rcon_server_banned_cities_set = main_app.get_game_server().get_banned_cities_set();
      for (const auto &banned_city : admins_banned_cities_set) {
        if (!tiny_rcon_server_banned_cities_set.contains(banned_city)) {
          tiny_rcon_server_banned_cities_set.emplace(banned_city);
        }
      }

      save_banned_cities_to_file(admins_banned_cities_file_path.c_str(), tiny_rcon_server_banned_cities_set);
      save_banned_cities_to_file(main_app.get_banned_cities_file_path(), tiny_rcon_server_banned_cities_set);
    }

    // process admin's banned country entries
    const string admins_banned_countries_file_path{ format("{}temp\\banned_countries.txt", main_app.get_current_working_directory()) };
    if (check_if_file_path_exists(admins_banned_countries_file_path.c_str())) {
      set<string> admins_banned_countries_set;
      parse_banned_countries_file(admins_banned_countries_file_path.c_str(), admins_banned_countries_set);
      auto &tiny_rcon_server_banned_countries_set = main_app.get_game_server().get_banned_countries_set();
      for (const auto &banned_country : admins_banned_countries_set) {
        if (!tiny_rcon_server_banned_countries_set.contains(banned_country)) {
          tiny_rcon_server_banned_countries_set.emplace(banned_country);
        }
      }

      save_banned_countries_to_file(admins_banned_countries_file_path.c_str(), tiny_rcon_server_banned_countries_set);
      save_banned_countries_to_file(main_app.get_banned_countries_file_path(), tiny_rcon_server_banned_countries_set);
    }

    const string admins_banned_names_file_path{ format("{}temp\\banned_names.txt", main_app.get_current_working_directory()) };
    if (check_if_file_path_exists(admins_banned_names_file_path.c_str())) {
      vector<player> admins_banned_names_vector;
      unordered_map<string, player> admins_banned_names_map;
      parse_banned_names_file(admins_banned_names_file_path.c_str(), admins_banned_names_vector, admins_banned_names_map);
      auto &tiny_rcon_server_banned_names_vector = main_app.get_game_server().get_banned_names_vector();
      auto &tiny_rcon_server_banned_names_map = main_app.get_game_server().get_banned_names_map();
      for (auto &pd : admins_banned_names_vector) {
        if (!tiny_rcon_server_banned_names_map.contains(pd.player_name)) {
          tiny_rcon_server_banned_names_map.emplace(pd.player_name, pd);
          tiny_rcon_server_banned_names_vector.push_back(std::move(pd));
        }
      }

      std::sort(std::begin(tiny_rcon_server_banned_names_vector), std::end(tiny_rcon_server_banned_names_vector), [](const player &pd1, const player &pd2) {
        return pd1.banned_start_time < pd2.banned_start_time;
      });

      save_banned_ip_entries_to_file(admins_banned_names_file_path.c_str(), tiny_rcon_server_banned_names_vector);
      save_banned_ip_entries_to_file(main_app.get_banned_names_file_path(), tiny_rcon_server_banned_names_vector);
    }

    // 1. delete existing bans_12321312312_132132131231.7z file on the ftp server
    DeleteFileA(admins_bans_compressed_file_path.c_str());

    // 2. create a new compressed 7z file (bans_12321312312_132132131231.7z) from temp\\tempbans.txt, temp\\bans.txt, temp\\ip_range_bans.txt, temp\\banned_cities, temp\\banned_countries.txt on the ftp server.
    const vector<string> files_to_compress{
      format("{}temp\\tempbans.txt", main_app.get_current_working_directory()), format("{}temp\\bans.txt", main_app.get_current_working_directory()), format("{}temp\\ip_range_bans.txt", main_app.get_current_working_directory()), format("{}temp\\banned_cities.txt", main_app.get_current_working_directory()), format("{}temp\\banned_countries.txt", main_app.get_current_working_directory()), format("{}temp\\banned_names.txt", main_app.get_current_working_directory())
    };
    create_7z_file_file_at_specified_path(files_to_compress, admins_bans_compressed_file_path);

    // 3. send "receive-bans" message to tinyrcon client
    const auto &sender_user = main_app.get_user_for_name(user, sender_ip);
    main_app.add_message_to_queue(message_t("receive-bans-compressed", data, sender_user, true));
  });

  main_app.add_message_handler("request-protectedipaddresses", [](const string &user, const time_t, const string &, bool, const string &sender_ip) {
    const auto &protected_ip_addresses = main_app.get_game_server().get_protected_ip_addresses();
    ostringstream oss;
    for (const auto &entry : protected_ip_addresses) {
      unsigned long guid_key{};
      if (check_ip_address_validity(entry, guid_key)) {
        oss << entry << '\\';
      }
    }
    oss << flush;
    const auto &sender_user = main_app.get_user_for_name(user, sender_ip);
    main_app.add_message_to_queue(message_t("receive-protectedipaddresses", oss.str(), sender_user, true));
  });

  main_app.add_message_handler("request-protectedipaddressranges", [](const string &user, const time_t, const string &, bool, const string &sender_ip) {
    const auto &protected_ip_address_ranges = main_app.get_game_server().get_protected_ip_address_ranges();
    ostringstream oss;
    for (const auto &entry : protected_ip_address_ranges) {
      oss << entry << '\\';
    }
    oss << flush;
    const auto &sender_user = main_app.get_user_for_name(user, sender_ip);
    main_app.add_message_to_queue(message_t("receive-protectedipaddressranges", oss.str(), sender_user, true));
  });

  main_app.add_message_handler("request-protectedcities", [](const string &user, const time_t, const string &, bool, const string &sender_ip) {
    const auto &protected_ip_cities = main_app.get_game_server().get_protected_cities();
    ostringstream oss;
    for (const auto &entry : protected_ip_cities) {
      oss << entry << '\\';
    }
    oss << flush;
    const auto &sender_user = main_app.get_user_for_name(user, sender_ip);
    main_app.add_message_to_queue(message_t("receive-protectedcities", oss.str(), sender_user, true));
  });

  main_app.add_message_handler("request-protectedcountries", [](const string &user, const time_t, const string &, bool, const string &sender_ip) {
    const auto &protected_ip_countries = main_app.get_game_server().get_protected_countries();
    ostringstream oss;
    for (const auto &entry : protected_ip_countries) {
      oss << entry << '\\';
    }
    oss << flush;
    const auto &sender_user = main_app.get_user_for_name(user, sender_ip);
    main_app.add_message_to_queue(message_t("receive-protectedcountries", oss.str(), sender_user, true));
  });

  main_app.add_message_handler("protect-ipaddress", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3) {
      auto &protected_ip_addresses = main_app.get_game_server().get_protected_ip_addresses();
      if (!protected_ip_addresses.contains(parts[1])) {
        protected_ip_addresses.emplace(parts[1]);
        ++main_app.get_tinyrcon_stats_data().get_no_of_protected_ip_addresses();
        save_banned_entries_to_file(main_app.get_protected_ip_addresses_file_path(), protected_ip_addresses);
        player protected_player{};
        protected_player.ip_address = parts[1];
        convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), parts[1], protected_player);
        const string message{ format("^7{} ^2added IP address ^1{} ^5[{}, {}] ^2to the list of protected ^1IP addresses^2.\n^5Reason: ^1{}", parts[0], parts[1], protected_player.country_name, protected_player.city, parts[2]) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        auto &user_data = main_app.get_user_for_name(parts[0], sender_ip);
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("protect-ipaddress", data, true, u);
          }
        }
      }
    }
  });

  main_app.add_message_handler("unprotect-ipaddress", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3) {
      auto &protected_ip_addresses = main_app.get_game_server().get_protected_ip_addresses();
      if (protected_ip_addresses.contains(parts[1])) {
        protected_ip_addresses.erase(parts[1]);
        save_banned_entries_to_file(main_app.get_protected_ip_addresses_file_path(), protected_ip_addresses);
        player protected_player{};
        protected_player.ip_address = parts[1];
        convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), parts[1], protected_player);
        const string message{ format("^7{} ^3has removed protected IP address ^1{} ^5[{}, {}] ^3from the list of protected ^1IP addresses^3.\n^5Reason: ^1{}", parts[0], parts[1], protected_player.country_name, protected_player.city, parts[2]) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        auto &user_data = main_app.get_user_for_name(parts[0], sender_ip);
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("unprotect-ipaddress", data, true, u);
          }
        }
      }
    }
  });


  main_app.add_message_handler("protect-ipaddressrange", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3) {
      auto &protected_ip_address_ranges = main_app.get_game_server().get_protected_ip_address_ranges();
      if (!protected_ip_address_ranges.contains(parts[1])) {
        protected_ip_address_ranges.emplace(parts[1]);
        ++main_app.get_tinyrcon_stats_data().get_no_of_protected_ip_address_ranges();
        save_banned_entries_to_file(main_app.get_protected_ip_address_ranges_file_path(), protected_ip_address_ranges);
        const string message{ format("^7{} ^2protected IP address range ^1{} ^2with reason: ^1{}", parts[0], parts[1], parts[2]) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        auto &user_data = main_app.get_user_for_name(parts[0], sender_ip);
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("unprotect-ipaddressrange", data, true, u);
          }
        }
      }
    }
  });

  main_app.add_message_handler("unprotect-ipaddressrange", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3) {
      auto &protected_ip_address_ranges = main_app.get_game_server().get_protected_ip_address_ranges();
      if (protected_ip_address_ranges.contains(parts[1])) {
        protected_ip_address_ranges.erase(parts[1]);
        save_banned_entries_to_file(main_app.get_protected_ip_address_ranges_file_path(), protected_ip_address_ranges);
        const string message{ format("^7{} ^3removed protected IP address range ^1{} ^3with reason: ^1{}", parts[0], parts[1], parts[2]) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        auto &user_data = main_app.get_user_for_name(parts[0], sender_ip);
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("unprotect-ipaddressrange", data, true, u);
          }
        }
      }
    }
  });

  main_app.add_message_handler("protect-city", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3) {
      auto &protected_cities = main_app.get_game_server().get_protected_cities();
      if (!protected_cities.contains(parts[1])) {
        protected_cities.emplace(parts[1]);
        ++main_app.get_tinyrcon_stats_data().get_no_of_protected_cities();
        save_banned_entries_to_file(main_app.get_protected_cities_file_path(), protected_cities);
        const string message{ format("^7{} ^2protected city ^1{} ^2with reason: ^1{}", parts[0], parts[1], parts[2]) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        auto &user_data = main_app.get_user_for_name(parts[0], sender_ip);
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("protect-city", data, true, u);
          }
        }
      }
    }
  });

  main_app.add_message_handler("unprotect-city", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3) {
      auto &protected_cities = main_app.get_game_server().get_protected_cities();
      if (protected_cities.contains(parts[1])) {
        protected_cities.erase(parts[1]);
        save_banned_entries_to_file(main_app.get_protected_cities_file_path(), protected_cities);
        const string message{ format("^7{} ^3removed protected city ^1{} ^3with reason: ^1{}", parts[0], parts[1], parts[2]) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        auto &user_data = main_app.get_user_for_name(parts[0], sender_ip);
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("unprotect-city", data, true, u);
          }
        }
      }
    }
  });

  main_app.add_message_handler("protect-country", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3) {
      auto &protected_countries = main_app.get_game_server().get_protected_countries();
      if (!protected_countries.contains(parts[1])) {
        protected_countries.emplace(parts[1]);
        ++main_app.get_tinyrcon_stats_data().get_no_of_protected_countries();
        save_banned_entries_to_file(main_app.get_protected_countries_file_path(), protected_countries);
        const string message{ format("^7{} ^2protected country ^1{} ^2with reason: ^1{}", parts[0], parts[1], parts[2]) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        auto &user_data = main_app.get_user_for_name(parts[0], sender_ip);
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("protect-country", data, true, u);
          }
        }
      }
    }
  });

  main_app.add_message_handler("unprotect-country", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3) {
      auto &protected_countries = main_app.get_game_server().get_protected_countries();
      if (protected_countries.contains(parts[1])) {
        protected_countries.erase(parts[1]);
        save_banned_entries_to_file(main_app.get_protected_countries_file_path(), protected_countries);
        const string message{ format("^7{} ^3removed protected country ^1{} ^3with reason: ^1{}", parts[0], parts[1], parts[2]) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        auto &user_data = main_app.get_user_for_name(parts[0], sender_ip);
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("unprotect-country", data, true, u);
          }
        }
      }
    }
  });

  main_app.add_message_handler("add-map_restart", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 4) {
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_map_restarts++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_map_restarts();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string inform_message{ format("^1Admin ^7{} ^5has restarted map ^1{} ^5in ^3{} ^5game type.\n", parts[0], parts[1], parts[2]) };
      print_colored_text(app_handles.hwnd_re_messages_data, inform_message.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, parts[3].c_str());
    }
  });


  main_app.add_message_handler("add-map_change", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 4) {
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_map_changes++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_map_changes();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string inform_message{ format("^1Admin ^7{} ^5has loaded map ^1{} ^5in ^3{} ^5game type.\n", parts[0], parts[1], parts[2]) };
      print_colored_text(app_handles.hwnd_re_messages_data, inform_message.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, parts[3].c_str());
    }
  });


  main_app.add_message_handler("add-warning", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5) {
      player warned_player{};
      warned_player.ip_address = parts[0];
      strcpy_s(warned_player.guid_key, std::size(warned_player.guid_key), parts[1].c_str());
      strcpy_s(warned_player.player_name, std::size(warned_player.player_name), parts[2].c_str());
      strcpy_s(warned_player.banned_date_time, std::size(warned_player.banned_date_time), parts[3].c_str());
      warned_player.banned_start_time = get_number_of_seconds_from_date_and_time_string(warned_player.banned_date_time);
      warned_player.reason = std::move(parts[4]);
      warned_player.banned_by_user_name = parts.size() >= 6 ? parts[5] : user;
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        warned_player.ip_address,
        warned_player);
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_warnings++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_warnings();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has ^1warned ^3player ^7{} ^3[^5IP address ^1{} ^3| ^5geoinfo: ^1{}, {}\n^5Date/time of warning: ^1{} ^3| ^5Reason of warning: ^1{} ^3| ^5Warned by: ^7{}\n", warned_player.banned_by_user_name, warned_player.player_name, warned_player.ip_address, warned_player.country_name, warned_player.city, warned_player.banned_date_time, warned_player.reason, warned_player.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-warning", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-kick", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5) {
      player kicked_player{};
      kicked_player.ip_address = parts[0];
      strcpy_s(kicked_player.guid_key, std::size(kicked_player.guid_key), parts[1].c_str());
      strcpy_s(kicked_player.player_name, std::size(kicked_player.player_name), parts[2].c_str());
      strcpy_s(kicked_player.banned_date_time, std::size(kicked_player.banned_date_time), parts[3].c_str());
      kicked_player.banned_start_time = get_number_of_seconds_from_date_and_time_string(kicked_player.banned_date_time);
      kicked_player.reason = std::move(parts[4]);
      kicked_player.banned_by_user_name = parts.size() >= 6 ? parts[5] : user;
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        kicked_player.ip_address,
        kicked_player);
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_kicks++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_kicks();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has ^1kicked ^3player ^7{} ^3[^5IP address ^1{} ^3| ^5geoinfo: ^1{}, {}\n^5Date/time of kick: ^1{} ^3| ^5Reason of kick: ^1{} ^3| ^5Kicked by: ^7{}\n", kicked_player.banned_by_user_name, kicked_player.player_name, kicked_player.ip_address, kicked_player.country_name, kicked_player.city, kicked_player.banned_date_time, kicked_player.reason, kicked_player.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-kick", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-tempban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 6 && !main_app.get_game_server().get_temp_banned_ip_addresses_map().contains(parts[0])) {
      player temp_banned_player_data{};
      temp_banned_player_data.ip_address = parts[0];
      strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name), parts[1].c_str());
      strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time), parts[2].c_str());
      temp_banned_player_data.banned_start_time = stoll(parts[3]);
      temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
      temp_banned_player_data.reason = std::move(parts[5]);
      temp_banned_player_data.banned_by_user_name = parts.size() >= 7 ? parts[6] : user;
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        temp_banned_player_data.ip_address,
        temp_banned_player_data);
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_tempbans++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_tempbans();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^5has temporarily banned ^1IP address {}\n ^5for ^3player name: ^7{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Ban duration: ^1{} hours ^5| ^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", temp_banned_player_data.banned_by_user_name, temp_banned_player_data.ip_address, temp_banned_player_data.player_name, temp_banned_player_data.country_name, temp_banned_player_data.city, temp_banned_player_data.banned_date_time, temp_banned_player_data.ban_duration_in_hours, temp_banned_player_data.reason, temp_banned_player_data.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_temporarily_banned_ip_address(temp_banned_player_data, main_app.get_game_server().get_temp_banned_players_data(), main_app.get_game_server().get_temp_banned_ip_addresses_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-tempban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("remove-tempban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 6 && main_app.get_game_server().get_temp_banned_ip_addresses_map().contains(parts[0])) {
      player temp_banned_player_data{};
      temp_banned_player_data.ip_address = parts[0];
      strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name), parts[1].c_str());
      strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time), parts[2].c_str());
      temp_banned_player_data.banned_start_time = stoll(parts[3]);
      temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
      temp_banned_player_data.reason = std::move(parts[5]);
      temp_banned_player_data.banned_by_user_name = parts.size() >= 7 ? parts[6] : user;
      const string removed_by{ parts.size() >= 8 ? parts[7] : user };
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        temp_banned_player_data.ip_address,
        temp_banned_player_data);
      const string msg{ format("^7{} ^5has removed temporarily banned ^1IP address {}\n ^5for ^3player name: ^7{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Ban duration: ^1{} hours ^5| ^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", removed_by, temp_banned_player_data.ip_address, temp_banned_player_data.player_name, temp_banned_player_data.country_name, temp_banned_player_data.city, temp_banned_player_data.banned_date_time, temp_banned_player_data.ban_duration_in_hours, temp_banned_player_data.reason, temp_banned_player_data.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      string message_about_removal;
      remove_temp_banned_ip_address(temp_banned_player_data.ip_address);
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-tempban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-guidban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && !main_app.get_game_server().get_banned_ip_addresses_map().contains(parts[0])) {
      player pd{};
      pd.ip_address = parts[0];
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : user;
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_guidbans++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_guid_bans();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^5has banned the ^1GUID key ^5of player:\n^3Name: ^7{} ^5| ^3IP address: ^1{} ^5| ^3GUID: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.guid_key, pd.country_name, pd.city, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-guidban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-nameban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && !main_app.get_game_server().get_banned_names_map().contains(parts[2])) {
      player pd{};
      pd.ip_address = parts[0];
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
      pd.reason = remove_disallowed_character_in_string(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : user;
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);

      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_namebans++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_name_bans();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);

      const string msg{ format("^7{} ^5has banned ^1player name: ^7{}\n^3IP address: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.country_name, pd.city, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_player_name(pd, main_app.get_game_server().get_banned_names_vector(), main_app.get_game_server().get_banned_names_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-nameban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("remove-nameban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && main_app.get_game_server().get_banned_names_map().contains(parts[2])) {
      player pd{};
      pd.ip_address = parts[0];
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
      pd.reason = remove_disallowed_character_in_string(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : user;
      const string removed_by{ parts.size() >= 7 ? parts[6] : user };
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);

      auto &user_data = main_app.get_user_for_name(user, sender_ip);

      const string msg{ format("^7{} ^5has removed ^1banned player name ^7{}\n^3IP address: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", removed_by, pd.player_name, pd.ip_address, pd.country_name, pd.city, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      remove_permanently_banned_player_name(pd, main_app.get_game_server().get_banned_names_vector(), main_app.get_game_server().get_banned_names_map());

      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-nameban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-ipban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && !main_app.get_game_server().get_banned_ip_addresses_map().contains(parts[0])) {
      player pd{};
      pd.ip_address = parts[0];
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : user;
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_ipbans++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_ip_bans();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has permanently banned ^1IP address ^3of player ^7{}.\n^3IP address: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of GUID ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.country_name, pd.city, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_ip_address(pd, main_app.get_game_server().get_banned_ip_addresses_vector(), main_app.get_game_server().get_banned_ip_addresses_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-ipban", data, true, u);
        }
      }
    }
  });


  main_app.add_message_handler("remove-ipban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && main_app.get_game_server().get_banned_ip_addresses_map().contains(parts[0])) {
      player pd{};
      pd.ip_address = parts[0];
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : user;
      const string removed_by{ parts.size() >= 7 ? parts[6] : user };
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has removed ^1banned IP address {}\n ^5for ^3player name: ^7{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", removed_by, pd.ip_address, pd.player_name, pd.country_name, pd.city, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      string ip_address{ pd.ip_address };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      remove_permanently_banned_ip_address(ip_address);
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-ipban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-iprangeban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && !main_app.get_game_server().get_banned_ip_address_ranges_map().contains(parts[0])) {
      player pd{};
      pd.ip_address = parts[0];
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : user;
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      user_data->no_of_iprangebans++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_ip_address_range_bans();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^5has banned ^1IP address range:\n^5[^3Player name: ^7{} ^5| ^3IP address range: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}^5]\n", pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.country_name, pd.city, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_ip_address_range(pd, main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-iprangeban", data, true, u);
        }
      }
    } });

  main_app.add_message_handler("remove-iprangeban", [](const string &user, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (main_app.get_game_server().get_banned_ip_address_ranges_map().contains(parts[0])) {
      player pd{};
      pd.ip_address = parts[0];
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : user;
      const string removed_by{ parts.size() >= 7 ? parts[6] : user };
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has removed previously ^1banned IP address range:\n^5[^3Player name: ^7{} ^5| ^3IP range: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}^5]\n", removed_by, pd.player_name, pd.ip_address, pd.country_name, pd.city, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      string ip_address{ pd.ip_address };
      auto &user_data = main_app.get_user_for_name(user, sender_ip);
      string message_about_removal;
      remove_permanently_banned_ip_address_range(pd, main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-iprangeban", data, true, u);
        }
      }
    }
  });
  main_app.add_message_handler("add-cityban", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string city{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };
      auto &user_data = main_app.get_user_for_name(admin, sender_ip);
      user_data->no_of_citybans++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_city_bans();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has banned city ^1{} ^3at ^1{}\n", admin, city, get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_city(city, main_app.get_game_server().get_banned_cities_set());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-cityban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("remove-cityban", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string city{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };

      const string msg{ format("^7{} ^2has removed banned city ^1{} ^2at ^1{}\n", admin, city, get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      remove_permanently_banned_city(city, main_app.get_game_server().get_banned_cities_set());
      auto &user_data = main_app.get_user_for_name(admin, sender_ip);
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-cityban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-countryban", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string country{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };
      auto &user_data = main_app.get_user_for_name(admin, sender_ip);
      user_data->no_of_countrybans++;
      ++main_app.get_tinyrcon_stats_data().get_no_of_country_bans();
      save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has banned country ^1{} ^3at ^1{}\n", admin, country, get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_country(country, main_app.get_game_server().get_banned_countries_set());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-countryban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("remove-countryban", [](const string &, const time_t, const string &data, bool, const string &sender_ip) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string city{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };
      auto &user_data = main_app.get_user_for_name(admin, sender_ip);

      const string msg{ format("^7{} ^2has removed banned country ^1{} ^2at ^1{}\n", admin, city, get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      remove_permanently_banned_country(city, main_app.get_game_server().get_banned_countries_set());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u.get() != user_data.get() && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-countryban", data, true, u);
        }
      }
    }
  });

  // main_app.set_command_line_info(user_help_message);

  users_table_column_header_titles[0] = main_app.get_game_server().get_header_player_pid_color() + "User name"s;
  users_table_column_header_titles[1] = main_app.get_game_server().get_header_player_score_color() + "Is admin?"s;
  users_table_column_header_titles[2] = main_app.get_game_server().get_header_player_score_color() + "Logged in?"s;
  users_table_column_header_titles[3] = main_app.get_game_server().get_header_player_score_color() + "Online?"s;
  users_table_column_header_titles[4] = main_app.get_game_server().get_header_player_ping_color() + "IP address"s;
  users_table_column_header_titles[5] = main_app.get_game_server().get_header_player_name_color() + "Geological information"s;
  users_table_column_header_titles[6] = main_app.get_game_server().get_header_player_ip_color() + "Flag"s;
  users_table_column_header_titles[7] = main_app.get_game_server().get_header_player_geoinfo_color() + "Date of last login"s;
  users_table_column_header_titles[8] = main_app.get_game_server().get_header_player_geoinfo_color() + "Date of last logout"s;
  users_table_column_header_titles[9] = main_app.get_game_server().get_header_player_pid_color() + "Logins"s;
  users_table_column_header_titles[10] = main_app.get_game_server().get_header_player_score_color() + "Warnings"s;
  users_table_column_header_titles[11] = main_app.get_game_server().get_header_player_ping_color() + "Kicks"s;
  users_table_column_header_titles[12] = main_app.get_game_server().get_header_player_name_color() + "Tempbans"s;
  users_table_column_header_titles[13] = main_app.get_game_server().get_header_player_ip_color() + "GUID bans"s;
  users_table_column_header_titles[14] = main_app.get_game_server().get_header_player_geoinfo_color() + "IP bans"s;
  users_table_column_header_titles[15] = main_app.get_game_server().get_header_player_geoinfo_color() + "IP range bans"s;
  users_table_column_header_titles[16] = main_app.get_game_server().get_header_player_geoinfo_color() + "City bans"s;
  users_table_column_header_titles[17] = main_app.get_game_server().get_header_player_geoinfo_color() + "Country bans"s;
  users_table_column_header_titles[18] = main_app.get_game_server().get_header_player_geoinfo_color() + "Name bans"s;

  construct_tinyrcon_gui(app_handles.hwnd_main_window);
  main_app.open_log_file("log/commands_history.log");

  const string program_title{ main_app.get_program_title() + " | "s + main_app.get_game_server_name() + " | "s + "version: "s + program_version };
  SetWindowText(app_handles.hwnd_main_window, program_title.c_str());

  CenterWindow(app_handles.hwnd_main_window);

  SetFocus(app_handles.hwnd_e_user_input);

  std::thread task_thread{
    [&]() {
      IsGUIThread(TRUE);
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started parsing ^1tinyrcon.json ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished parsing ^1tinyrcon.json ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started importing serialized binary geological data from\n ^1'plugins/geoIP/geo.dat' ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      // const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\IP2LOCATION-LITE-DB3.csv" };
      // parse_geodata_lite_csv_file(geo_dat_file_path.c_str());
      const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\geo.dat" };
      import_geoip_data(main_app.get_connection_manager_for_messages().get_geoip_data(), geo_dat_file_path.c_str());

      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished importing serialized binary geological data from\n ^1'plugins/geoIP/geo.dat' ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_tinyrcon_server_users_data(main_app.get_users_data_file_path());

      parse_protected_entries_file(main_app.get_protected_ip_addresses_file_path(), main_app.get_game_server().get_protected_ip_addresses());
      parse_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(), main_app.get_game_server().get_protected_ip_address_ranges());
      parse_protected_entries_file(main_app.get_protected_cities_file_path(), main_app.get_game_server().get_protected_cities());
      parse_protected_entries_file(main_app.get_protected_countries_file_path(), main_app.get_game_server().get_protected_countries());

      parse_tempbans_data_file(main_app.get_temp_bans_file_path(), main_app.get_game_server().get_temp_banned_players_data(), main_app.get_game_server().get_temp_banned_ip_addresses_map());
      parse_tempbans_data_file(main_app.get_removed_temp_bans_file_path(), main_app.get_game_server().get_removed_temp_banned_players_vector(), main_app.get_game_server().get_removed_temp_banned_players_map(), true);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1temporarily banned IP addresses ^2from ^5data\\tempbans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_ip_addresses_file(main_app.get_ip_bans_file_path(), main_app.get_game_server().get_banned_ip_addresses_vector(), main_app.get_game_server().get_banned_ip_addresses_map());
      parse_banned_ip_addresses_file(main_app.get_removed_ip_bans_file_path(), main_app.get_game_server().get_removed_banned_ip_addresses_vector(), main_app.get_game_server().get_removed_banned_ip_addresses_map(), true);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1permanently banned IP addresses ^2from ^5data\\bans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_ip_address_ranges_file(main_app.get_ip_range_bans_file_path(), main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
      parse_banned_ip_address_ranges_file(main_app.get_removed_ip_range_bans_file_path(), main_app.get_game_server().get_removed_banned_ip_address_ranges_vector(), main_app.get_game_server().get_removed_banned_ip_address_ranges_map(), true);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned IP address ranges ^2from ^5data\\ip_range_bans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_cities_file(main_app.get_banned_cities_file_path(), main_app.get_game_server().get_banned_cities_set());
      parse_banned_cities_file(main_app.get_removed_banned_cities_file_path(), main_app.get_game_server().get_removed_banned_cities_set(), true);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned cities ^2from ^5data\\banned_cities.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_countries_file(main_app.get_banned_countries_file_path(), main_app.get_game_server().get_banned_countries_set());
      parse_banned_countries_file(main_app.get_removed_banned_countries_file_path(), main_app.get_game_server().get_removed_banned_countries_set(), true);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned countries ^2from ^5data\\banned_countries.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_names_file(main_app.get_banned_names_file_path(), main_app.get_game_server().get_banned_names_vector(), main_app.get_game_server().get_banned_names_map());
      parse_banned_names_file(main_app.get_removed_banned_names_file_path(), main_app.get_game_server().get_removed_banned_names_vector(), main_app.get_game_server().get_removed_banned_names_map(), true);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned names ^2from ^5data\\banned_names.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      load_tinyrcon_statistics_data("data\\data_stats.json");

      is_main_window_constructed = true;

      while (true) {
        try {

          {
            unique_lock ul{ mu };
            exit_flag.wait_for(ul, 20ms, [&]() {
              return is_terminate_program.load();
            });
          }

          if (is_terminate_program.load()) break;

          while (!main_app.is_command_queue_empty()) {
            auto cmd = main_app.get_command_from_queue();
            main_app.process_queue_command(std::move(cmd));
          }
        } catch (std::exception &ex) {
          const string error_message{ format("^3A specific exception was caught in command queue's thread!\n^1Exception: {}", ex.what()) };
          print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
        } catch (...) {
          char buffer[512];
          strerror_s(buffer, GetLastError());
          const string error_message{ format("^3A generic error was caught in command queue's thread!\n^1Exception: {}", buffer) };
          print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
        }
      }
    }
  };

  task_thread.detach();

  std::thread messaging_thread{
    [&]() {
      while (true) {
        try {

          main_app.get_connection_manager_for_messages().wait_for_and_process_response_message();

          while (!main_app.is_message_queue_empty()) {
            message_t message{ main_app.get_message_from_queue() };
            main_app.get_connection_manager_for_messages().process_and_send_message(message.command, message.data, message.is_show_in_messages, message.sender);
          }
        } catch (std::exception &ex) {
          const string error_message{ format("^3A specific exception was caught in message queue's thread!\n^1Exception: {}", ex.what()) };
          print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
        } catch (...) {
          char buffer[512];
          strerror_s(buffer, GetLastError());
          const string error_message{ format("^3A generic error was caught in message queue's thread!\n^1Exception: {}", buffer) };
          print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
        }
      }
    }
  };

  messaging_thread.detach();

  MSG msg{};
  try {

    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
      if (TranslateAccelerator(app_handles.hwnd_main_window, hAccel, &msg) != 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        SetWindowText(app_handles.hwnd_e_user_input, "");
      } else if (IsDialogMessage(app_handles.hwnd_main_window, &msg) == 0) {
        TranslateMessage(&msg);
        if (msg.message == WM_KEYDOWN) {
          process_key_down_message(msg);
        }
        DispatchMessage(&msg);
      } else if (msg.message == WM_KEYDOWN) {
        process_key_down_message(msg);
      } else if (msg.message == WM_RBUTTONDOWN && app_handles.hwnd_users_table == GetFocus()) {
        const int x{ GET_X_LPARAM(msg.lParam) };
        const int y{ GET_Y_LPARAM(msg.lParam) };
        display_context_menu_over_grid(app_handles.hwnd_users_table, x, y);
      }

      if (is_main_window_constructed && !is_tinyrcon_initialized) {

        ifstream temp_messages_file{ "log/temporary_message.log" };
        if (temp_messages_file) {
          for (string line; getline(temp_messages_file, line);) {
            print_colored_text(app_handles.hwnd_re_messages_data, line.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          }
          temp_messages_file.close();
        }

        is_tinyrcon_initialized = true;

        display_users_data_in_users_table(app_handles.hwnd_users_table);
        display_online_admins_information();
        SetTimer(app_handles.hwnd_main_window, ID_TIMER, 1000, nullptr);
      }
    }

    save_tiny_rcon_users_data_to_json_file(main_app.get_users_data_file_path());
    save_protected_entries_file(main_app.get_protected_ip_addresses_file_path(), main_app.get_game_server().get_protected_ip_addresses());
    save_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(), main_app.get_game_server().get_protected_ip_address_ranges());
    save_protected_entries_file(main_app.get_protected_cities_file_path(), main_app.get_game_server().get_protected_cities());
    save_protected_entries_file(main_app.get_protected_countries_file_path(), main_app.get_game_server().get_protected_countries());

    save_tempbans_to_file(main_app.get_removed_temp_bans_file_path(), main_app.get_game_server().get_removed_temp_banned_players_vector());

    save_banned_ip_entries_to_file(main_app.get_removed_ip_bans_file_path(), main_app.get_game_server().get_removed_banned_ip_addresses_vector());
    save_banned_ip_address_range_entries_to_file(main_app.get_removed_ip_range_bans_file_path(), main_app.get_game_server().get_removed_banned_ip_address_ranges_vector());
    save_banned_cities_to_file(main_app.get_removed_banned_cities_file_path(), main_app.get_game_server().get_removed_banned_cities_set());
    save_banned_countries_to_file(main_app.get_removed_banned_countries_file_path(), main_app.get_game_server().get_removed_banned_countries_set());

    save_tinyrcon_statistics_data("data\\data_stats.json");

    is_terminate_program.store(true);
    exit_flag.notify_all();

    if (pr_info.hProcess != NULL)
      CloseHandle(pr_info.hProcess);
    if (pr_info.hThread != NULL)
      CloseHandle(pr_info.hThread);


    log_message("Exiting TinyRcon server...", is_log_datetime::yes);

    DestroyAcceleratorTable(hAccel);
    const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\geo.dat" };
    // export_geoip_data(main_app.get_connection_manager().get_geoip_data(), geo_dat_file_path.c_str());

    if (wcex.hbrBackground != nullptr)
      DeleteObject((HGDIOBJ)wcex.hbrBackground);
    if (RED_BRUSH != NULL)
      DeleteObject((HGDIOBJ)RED_BRUSH);
    if (font_for_players_grid_data != NULL)
      DeleteFont(font_for_players_grid_data);
    if (hImageList)
      ImageList_Destroy(hImageList);
    UnregisterClass(wcex.lpszClassName, app_handles.hInstance);
    UnregisterClass(wcex_confirmation_dialog.lpszClassName, app_handles.hInstance);
  } catch (std::exception &ex) {
    const string error_message{ format("^3A specific exception was caught in WinMain's thread of execution!\n^1Exception: {}", ex.what()) };
    print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
  } catch (...) {
    char buffer[512];
    strerror_s(buffer, GetLastError());
    const string error_message{ format("^3A generic error was caught in WinMain's thread of execution\n^1Exception: {}", buffer) };
    print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
  }
  return static_cast<int>(msg.wParam);
}

ATOM register_window_classes(HINSTANCE hInstance)
{
  wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONSERVER));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = CreateSolidBrush(color::black);
  wcex.lpszClassName = "tinyrconservergui";
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  auto status = RegisterClassEx(&wcex);
  if (!status) {
    char error_msg[256]{};
    (void)snprintf(error_msg, std::size(error_msg), "Windows class creation failed with error code: %d", GetLastError());
    MessageBox(nullptr, error_msg, "Window Class Failed", MB_ICONERROR);
  }

  wcex_confirmation_dialog = {};
  wcex_confirmation_dialog.cbSize = sizeof(WNDCLASSEX);
  wcex_confirmation_dialog.style = CS_HREDRAW | CS_VREDRAW;
  wcex_confirmation_dialog.lpfnWndProc = WndProcForConfirmationDialog;
  wcex_confirmation_dialog.hInstance = hInstance;
  wcex_confirmation_dialog.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONSERVER));
  wcex_confirmation_dialog.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex_confirmation_dialog.hbrBackground = CreateSolidBrush(color::black);
  wcex_confirmation_dialog.lpszClassName = "ConfirmationDialog";
  wcex_confirmation_dialog.hIconSm = LoadIcon(wcex_confirmation_dialog.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  status = RegisterClassEx(&wcex_confirmation_dialog);
  if (!status) {
    char error_msg[256]{};
    (void)snprintf(error_msg, std::size(error_msg), "Windows class creation failed with error code: %d", GetLastError());
    MessageBox(nullptr, error_msg, "Window Class Failed", MB_ICONERROR);
  }

  return status;
}

bool initialize_main_app(HINSTANCE hInstance, const int nCmdShow)
{
  app_handles.hInstance = hInstance;// Store instance handle in our global variable

  InitSimpleGrid(app_handles.hInstance);

  INITCOMMONCONTROLSEX icex{};

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_PROGRESS_CLASS;
  InitCommonControlsEx(&icex);

  font_for_players_grid_data = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_DECORATIVE, "Lucida Console");

  // AdjustWindowRectEx(&client_rect, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, FALSE, 0);

  RECT desktop_work_area{};
  SystemParametersInfoA(SPI_GETWORKAREA, 0, &desktop_work_area, 0);
  // AdjustWindowRectEx(&desktop_work_area, WS_OVERLAPPEDWINDOW /*| WS_HSCROLL | WS_VSCROLL*/, FALSE, WS_EX_OVERLAPPEDWINDOW);

  app_handles.hwnd_main_window = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wcex.lpszClassName, "TinyRcon server", WS_OVERLAPPEDWINDOW /*| WS_HSCROLL | WS_VSCROLL*/, 0, 0, desktop_work_area.right - desktop_work_area.left, desktop_work_area.bottom - desktop_work_area.top, nullptr, nullptr, hInstance, nullptr);

  // app_handles.hwnd_main_window = CreateWindowEx(0, wcex.lpszClassName, "Welcome to TinyRcon | version: 2.4", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME /*WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL*/, 0, 0, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, nullptr, nullptr, hInstance, nullptr);

  if (!app_handles.hwnd_main_window)
    return false;

  ShowWindow(app_handles.hwnd_main_window, nCmdShow);
  UpdateWindow(app_handles.hwnd_main_window);
  Sleep(50);
  MoveWindow(app_handles.hwnd_main_window, 0, 0, desktop_work_area.right - desktop_work_area.left, desktop_work_area.bottom - desktop_work_area.top, TRUE);
  SendMessageA(app_handles.hwnd_main_window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
  return true;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static char msg_buffer[512];
  static char message_buffer[512];
  static const size_t time_period{ main_app.get_game_server().get_check_for_banned_players_time_period() };
  static size_t counter{};
  HBRUSH orig_textEditBrush{};
  HBRUSH comboBrush1{}, comboBrush2{}, comboBrush3{}, comboBrush4{}, comboBrush5{};
  HBRUSH defaultbrush{};
  HBRUSH hotbrush{};
  HBRUSH selectbrush{};
  HMENU hPopupMenu;
  PAINTSTRUCT ps;

  static HPEN red_pen{};
  static HPEN light_blue_pen{};

  switch (message) {

  case WM_CONTEXTMENU: {
    if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_re_messages_data) {
      hPopupMenu = CreatePopupMenu();
      InsertMenu(hPopupMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDC_COPY, "&Copy");
      InsertMenu(hPopupMenu, 1, MF_BYPOSITION | MF_STRING, ID_VIEWTEMPBANSBUTTON, "View temporarily banned IP addresses");
      InsertMenu(hPopupMenu, 2, MF_BYPOSITION | MF_STRING, ID_VIEWIPBANSBUTTON, "View permanently banned IP addresses");
      InsertMenu(hPopupMenu, 3, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCITIES, "View banned cities");
      InsertMenu(hPopupMenu, 4, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCOUNTRIES, "View banned countries");
      InsertMenu(hPopupMenu, 5, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSES, "View protected IP addresses");
      InsertMenu(hPopupMenu, 6, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSRANGES, "View protected IP address ranges");
      InsertMenu(hPopupMenu, 7, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCITIES, "View protected cities");
      InsertMenu(hPopupMenu, 8, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCOUNTRIES, "View protected countries");
      InsertMenu(hPopupMenu, 9, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, 10, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "Clear messages");
      InsertMenu(hPopupMenu, 11, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, 12, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    } else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_e_user_input) {
      hPopupMenu = CreatePopupMenu();
      InsertMenu(hPopupMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDC_PASTE, "&Paste");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    }
  } break;

  case WM_TIMER: {

    ++counter;

    if (counter % 5 == 0) {
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      display_online_admins_information();
    }

    if (counter % 30 == 0) {
      counter = 0;
      const auto current_ts{ get_current_time_stamp() };
      unsigned long guid_key{};
      for (auto &u : main_app.get_users()) {
        if (u->is_logged_in && check_ip_address_validity(u->ip_address, guid_key)) {
          u->is_logged_in = false;
          main_app.get_connection_manager_for_messages().process_and_send_message("heartbeat", format("{}\\{}", u->user_name, current_ts), true, u);
        }
      }
      save_tinyrcon_statistics_data("data\\data_stats.json");
    }

    HDC hdc = BeginPaint(hWnd, &ps);

    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, color::black);
    SetTextColor(hdc, color::red);

    RECT bounding_rectangle = {
      10,
      screen_height - 75,
      130,
      screen_height - 55
    };

    InvalidateRect(hWnd, &bounding_rectangle, TRUE);
    EndPaint(hWnd, &ps);

    if (is_display_temporarily_banned_players_data_event.load()) {
      std::thread display_large_data_set{ display_temporarily_banned_ip_addresses, std::string::npos, false };
      display_large_data_set.detach();
      is_display_temporarily_banned_players_data_event.store(false);
    } else if (is_display_permanently_banned_players_data_event.load()) {
      std::thread display_large_data_set{ display_permanently_banned_ip_addresses, std::string::npos, false };
      display_large_data_set.detach();
      is_display_permanently_banned_players_data_event.store(false);
    } else if (is_display_banned_ip_address_ranges_data_event.load()) {
      std::thread display_large_data_set{ display_banned_ip_address_ranges, std::string::npos, false };
      display_large_data_set.detach();
      is_display_banned_ip_address_ranges_data_event.store(false);
    } else if (is_display_banned_cities_data_event.load()) {
      std::thread display_large_data_set{ []() { display_banned_cities(main_app.get_game_server().get_banned_cities_set()); } };
      display_large_data_set.detach();
      is_display_banned_cities_data_event.store(false);
    } else if (is_display_banned_countries_data_event.load()) {
      std::thread display_large_data_set{ []() { display_banned_countries(main_app.get_game_server().get_banned_countries_set()); } };
      display_large_data_set.detach();
      is_display_banned_countries_data_event.store(false);
    } else if (is_display_admins_data_event.load()) {
      std::thread display_large_data_set{ display_admins_data };
      display_large_data_set.detach();
      is_display_admins_data_event.store(false);
    } else if (is_display_protected_ip_addresses_data_event.load()) {
      std::thread display_large_data_set{ []() {
        display_protected_entries("Protected IP addresses:", main_app.get_game_server().get_protected_ip_addresses());
      } };
      display_large_data_set.detach();
      is_display_protected_ip_addresses_data_event.store(false);
    } else if (is_display_protected_ip_address_ranges_data_event.load()) {
      std::thread display_large_data_set{ []() { display_protected_entries("Protected IP address ranges:", main_app.get_game_server().get_protected_ip_address_ranges()); } };
      display_large_data_set.detach();
      is_display_protected_ip_address_ranges_data_event.store(false);
    } else if (is_display_protected_cities_data_event.load()) {
      std::thread display_large_data_set{ []() {
        display_protected_entries("Protected cities:", main_app.get_game_server().get_protected_cities());
      } };
      display_large_data_set.detach();
      is_display_protected_cities_data_event.store(false);
    } else if (is_display_protected_countries_data_event.load()) {
      std::thread display_large_data_set{ []() {
        display_protected_entries("Protected countries:", main_app.get_game_server().get_protected_countries());
      } };
      display_large_data_set.detach();
      is_display_protected_countries_data_event.store(false);
    }


  }

  break;

  case WM_NOTIFY: {
    if (wParam == 501) {
      auto nmhdr = (NMGRID *)lParam;
      const int row_index = nmhdr->row;
      const int col_index = nmhdr->col;

      if (row_index < (int)max_users_grid_rows) {
        selected_row = row_index;
      }

      if (col_index >= 0 && col_index < 6) {
        selected_col = col_index;
      }
    }

    LPNMHDR some_item = (LPNMHDR)lParam;

    if ((some_item->idFrom == ID_VIEWTEMPBANSBUTTON || some_item->idFrom == ID_VIEWIPBANSBUTTON || some_item->idFrom == ID_QUITBUTTON || some_item->idFrom == ID_CLEARMESSAGESCREENBUTTON)
        && (some_item->code == NM_CUSTOMDRAW)) {
      LPNMCUSTOMDRAW item = (LPNMCUSTOMDRAW)some_item;

      if ((item->uItemState & CDIS_FOCUS) || (item->uItemState & CDIS_SELECTED)) {
        if (!selectbrush)
          selectbrush = CreateSolidBrush(color::yellow);

        if (!red_pen)
          red_pen = CreatePen(PS_SOLID, 2, color::red);

        HGDIOBJ old_pen = SelectObject(item->hdc, red_pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, selectbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawTextEx(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
        return CDRF_SKIPDEFAULT;
      } else {
        if (item->uItemState & CDIS_HOT) {
          if (!hotbrush)
            hotbrush = CreateSolidBrush(color::yellow);

          if (!red_pen)
            red_pen = CreatePen(PS_SOLID, 2, color::red);

          HGDIOBJ old_pen = SelectObject(item->hdc, red_pen);
          HGDIOBJ old_brush = SelectObject(item->hdc, hotbrush);

          Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

          SelectObject(item->hdc, old_pen);
          SelectObject(item->hdc, old_brush);

          SetTextColor(item->hdc, color::red);
          SetBkMode(item->hdc, TRANSPARENT);
          DrawTextEx(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
          return CDRF_SKIPDEFAULT;
        }

        if (defaultbrush == NULL)
          defaultbrush = CreateSolidBrush(color::light_blue);

        if (!light_blue_pen)
          light_blue_pen = CreatePen(PS_SOLID, 2, color::light_blue);

        HGDIOBJ old_pen = SelectObject(item->hdc, light_blue_pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, defaultbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawTextEx(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
        return CDRF_SKIPDEFAULT;
      }
    }
    return CDRF_DODEFAULT;

  } break;


  case WM_COMMAND: {

    const int wmId = LOWORD(wParam);
    const int wparam_high_word = HIWORD(wParam);

    switch (wmId) {

    case IDC_COPY: {

      SetForegroundWindow(app_handles.hwnd_re_messages_data);
      INPUT ip;
      ip.type = INPUT_KEYBOARD;
      ip.ki.wScan = 0;
      ip.ki.time = 0;
      ip.ki.dwExtraInfo = 0;
      // Press the "Ctrl" key
      ip.ki.wVk = VK_CONTROL;
      ip.ki.dwFlags = 0;// 0 for key press
      SendInput(1, &ip, sizeof(INPUT));

      // Press the "C" key
      ip.ki.wVk = 'C';
      ip.ki.dwFlags = 0;// 0 for key press
      SendInput(1, &ip, sizeof(INPUT));

      // Release the "C" key
      ip.ki.wVk = 'C';
      ip.ki.dwFlags = KEYEVENTF_KEYUP;
      SendInput(1, &ip, sizeof(INPUT));

      // Release the "Ctrl" key
      ip.ki.wVk = VK_CONTROL;
      ip.ki.dwFlags = KEYEVENTF_KEYUP;
      SendInput(1, &ip, sizeof(INPUT));
    } break;

    case IDC_PASTE: {
      SetForegroundWindow(app_handles.hwnd_e_user_input);
      INPUT ip;
      ip.type = INPUT_KEYBOARD;
      ip.ki.wScan = 0;
      ip.ki.time = 0;
      ip.ki.dwExtraInfo = 0;
      // Press the "Ctrl" key
      ip.ki.wVk = VK_CONTROL;
      ip.ki.dwFlags = 0;// 0 for key press
      SendInput(1, &ip, sizeof(INPUT));

      // Press the "V" key
      ip.ki.wVk = 'V';
      ip.ki.dwFlags = 0;// 0 for key press
      SendInput(1, &ip, sizeof(INPUT));

      // Release the "V" key
      ip.ki.wVk = 'V';
      ip.ki.dwFlags = KEYEVENTF_KEYUP;
      SendInput(1, &ip, sizeof(INPUT));

      // Release the "Ctrl" key
      ip.ki.wVk = VK_CONTROL;
      ip.ki.dwFlags = KEYEVENTF_KEYUP;
      SendInput(1, &ip, sizeof(INPUT));

    } break;

    case ID_QUITBUTTON:
      // if (show_user_confirmation_dialog("^3Do you really want to exit?\n", "Exit TinyRcon server?")) {
      is_terminate_program.store(true);
      exit_flag.notify_all();
      PostQuitMessage(0);
      // }
      break;

    case ID_CLEARMESSAGESCREENBUTTON: {
      Edit_SetText(app_handles.hwnd_re_messages_data, "");
      g_message_data_contents.clear();
    } break;


    case ID_VIEWTEMPBANSBUTTON:
      is_display_temporarily_banned_players_data_event.store(true);
      break;

    case ID_VIEWIPBANSBUTTON:
      is_display_permanently_banned_players_data_event.store(true);
      break;

    case ID_VIEWIPRANGEBANSBUTTON:
      is_display_banned_ip_address_ranges_data_event.store(true);
      break;

    case ID_VIEWBANNEDCITIES:
      is_display_banned_cities_data_event.store(true);
      break;

    case ID_VIEWBANNEDCOUNTRIES:
      is_display_banned_countries_data_event.store(true);
      break;

    case ID_VIEWADMINSDATA: {
      is_display_admins_data_event.store(true);
    } break;

    case ID_VIEWPROTECTEDIPADDRESSES:
      is_display_protected_ip_addresses_data_event.store(true);
      break;
    case ID_VIEWPROTECTEDIPADDRESSRANGES:
      is_display_protected_ip_address_ranges_data_event.store(true);
      break;
    case ID_VIEWPROTECTEDCITIES:
      is_display_protected_cities_data_event.store(true);
      break;
    case ID_VIEWPROTECTEDCOUNTRIES:
      is_display_protected_countries_data_event.store(true);
      break;
    }

  } break;

  case WM_PAINT: {

    HDC hdc = BeginPaint(hWnd, &ps);

    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, color::black);
    SetTextColor(hdc, color::red);

    RECT bounding_rectangle = {
      10,
      screen_height - 75,
      130,
      screen_height - 55
    };

    DrawText(hdc, prompt_message, -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT | DT_END_ELLIPSIS);


    EndPaint(hWnd, &ps);

  } break;

  case WM_KEYDOWN:
    if (wParam == VK_DOWN || wParam == VK_UP)
      return 0;
    break;
  case WM_CLOSE:
    is_terminate_program.store(true);
    exit_flag.notify_all();
    DestroyWindow(app_handles.hwnd_main_window);
    return 0;


  case WM_DESTROY:
    KillTimer(hWnd, ID_TIMER);
    if (orig_textEditBrush) DeleteBrush(orig_textEditBrush);
    if (comboBrush1) DeleteBrush(comboBrush1);
    if (comboBrush2) DeleteBrush(comboBrush2);
    if (comboBrush3) DeleteBrush(comboBrush3);
    if (comboBrush4) DeleteBrush(comboBrush4);
    if (comboBrush5) DeleteBrush(comboBrush5);
    if (defaultbrush) DeleteBrush(defaultbrush);
    if (hotbrush) DeleteBrush(hotbrush);
    if (selectbrush) DeleteBrush(selectbrush);
    if (red_pen) DeletePen(red_pen);
    if (light_blue_pen) DeletePen(light_blue_pen);
    PostQuitMessage(0);
    return 0;

  case WM_SIZE:
    if (hWnd == app_handles.hwnd_main_window && is_main_window_constructed)
      return 0;
    return DefWindowProc(hWnd, message, wParam, lParam);

  case WM_CTLCOLOREDIT: {
    HDC dc = (HDC)wParam;
    SetTextColor(dc, color::blue);
    SetBkColor(dc, color::yellow);
    SetBkMode(dc, OPAQUE);
    orig_textEditBrush = CreateSolidBrush(color::yellow);
    return (INT_PTR)orig_textEditBrush;
  }
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR)
{
  static char buf[512];
  static HPEN red_pen{};
  static HPEN black_pen{};
  HBRUSH selectbrush{};

  switch (msg) {

  case WM_PAINT: {
    DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if (!(style & CBS_DROPDOWNLIST)) break;

    PAINTSTRUCT ps{};
    auto hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);

    // Select our color when the button is selected
    if (GetFocus() == hwnd) {
      if (!selectbrush)
        selectbrush = CreateSolidBrush(color::yellow);

      // Create red_pen for button border
      if (!red_pen) red_pen = CreatePen(PS_SOLID, 2, color::red);

      // Select our brush into hDC
      HGDIOBJ old_pen = SelectObject(hdc, red_pen);
      HGDIOBJ old_brush = SelectObject(hdc, selectbrush);
      SetBkMode(hdc, OPAQUE);
      SetBkColor(hdc, color::yellow);
      SetTextColor(hdc, color::red);
      FillRect(hdc, &rc, selectbrush);
      Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

      RECT temp = rc;
      InflateRect(&temp, -2, -2);
      DrawFocusRect(hdc, &temp);

      // Clean up
      SelectObject(hdc, old_pen);
      SelectObject(hdc, old_brush);

    } else {

      auto brush = CreateSolidBrush(color::light_blue);
      if (!black_pen) black_pen = CreatePen(PS_SOLID, 2, color::black);
      auto oldbrush = SelectObject(hdc, brush);
      auto oldpen = SelectObject(hdc, black_pen);
      SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
      SetBkMode(hdc, OPAQUE);
      SetBkColor(hdc, color::light_blue);
      SetTextColor(hdc, color::red);

      Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

      SelectObject(hdc, oldpen);
      SelectObject(hdc, oldbrush);
      DeleteObject(brush);
    }

    if (GetFocus() == hwnd) {
      RECT temp = rc;
      InflateRect(&temp, -2, -2);
      DrawFocusRect(hdc, &temp);
    }

    int index = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (index >= 0) {
      SendMessage(hwnd, CB_GETLBTEXT, index, (LPARAM)buf);
      rc.left += 5;
      DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      rc.left = rc.right - 15;
      DrawText(hdc, "v", -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    EndPaint(hwnd, &ps);
    return 0;
  }


  case WM_NCDESTROY: {
    if (selectbrush) DeleteBrush(selectbrush);
    if (red_pen) DeletePen(red_pen);
    if (black_pen) DeletePen(black_pen);
    RemoveWindowSubclass(hwnd, ComboProc, uIdSubClass);
    break;
  }
  }

  return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProcForConfirmationDialog(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static char msg_buffer[1024];
  HBRUSH orig_textEditBrush{}, comboBrush1{}, comboBrush2{}, comboBrush3{}, comboBrush4{}, black_brush{};
  HBRUSH defaultbrush{};
  HBRUSH hotbrush{};
  HBRUSH selectbrush{};
  static HPEN red_pen{};
  static HPEN light_blue_pen{};

  switch (message) {

  case WM_CREATE:
    is_first_left_mouse_button_click_in_reason_edit_control = true;
    EnableWindow(app_handles.hwnd_main_window, FALSE);
    break;

  case WM_NOTIFY: {

    LPNMHDR some_item = (LPNMHDR)lParam;

    if ((some_item->idFrom == ID_YES_BUTTON || some_item->idFrom == ID_NO_BUTTON)
        && (some_item->code == NM_CUSTOMDRAW)) {
      LPNMCUSTOMDRAW item = (LPNMCUSTOMDRAW)some_item;

      if ((item->uItemState & CDIS_FOCUS) || (item->uItemState & CDIS_SELECTED)) {
        if (!selectbrush)
          selectbrush = CreateSolidBrush(color::yellow);

        if (!red_pen)
          red_pen = CreatePen(PS_SOLID, 2, color::red);

        HGDIOBJ old_pen = SelectObject(item->hdc, red_pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, selectbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        return CDRF_SKIPDEFAULT;
      } else {
        if (item->uItemState & CDIS_HOT) {
          if (!hotbrush)
            hotbrush = CreateSolidBrush(color::yellow);

          if (!red_pen)
            red_pen = CreatePen(PS_SOLID, 2, color::red);

          HGDIOBJ old_pen = SelectObject(item->hdc, red_pen);
          HGDIOBJ old_brush = SelectObject(item->hdc, hotbrush);

          Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

          SelectObject(item->hdc, old_pen);
          SelectObject(item->hdc, old_brush);

          SetTextColor(item->hdc, color::red);
          SetBkMode(item->hdc, TRANSPARENT);
          DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
          return CDRF_SKIPDEFAULT;
        }

        if (defaultbrush == NULL)
          defaultbrush = CreateSolidBrush(color::light_blue);

        if (!light_blue_pen)
          light_blue_pen = CreatePen(PS_SOLID, 2, color::light_blue);

        HGDIOBJ old_pen = SelectObject(item->hdc, light_blue_pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, defaultbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        return CDRF_SKIPDEFAULT;
      }
    }
    return CDRF_DODEFAULT;

  } break;


  case WM_COMMAND: {
    const int wmId = LOWORD(wParam);
    switch (wmId) {

    case ID_YES_BUTTON:
      admin_choice.store(1);
      Edit_GetText(app_handles.hwnd_e_reason, msg_buffer, std::size(msg_buffer));
      if (stl::helper::len(msg_buffer) > 0) {
        admin_reason.assign(msg_buffer);
      } else {
        admin_reason.assign("not specified");
      }
      EnableWindow(app_handles.hwnd_main_window, TRUE);
      SetFocus(app_handles.hwnd_e_user_input);
      DestroyWindow(hWnd);
      break;

    case ID_NO_BUTTON:
      admin_choice.store(0);
      admin_reason.assign("not specified");
      EnableWindow(app_handles.hwnd_main_window, TRUE);
      SetFocus(app_handles.hwnd_e_user_input);
      DestroyWindow(hWnd);
      break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }

  } break;

  case WM_PAINT: {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hWnd, &ps);
    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, color::black);
    SetTextColor(hdc, color::red);
    RECT bounding_rectangle{ 10, 220, 90, 240 };
    DrawText(hdc, "Reason:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT /*| DT_END_ELLIPSIS*/);
    EndPaint(hWnd, &ps);
  } break;


  case WM_CLOSE:
    admin_choice.store(0);
    admin_reason = "not specified";
    EnableWindow(app_handles.hwnd_main_window, TRUE);
    SetFocus(app_handles.hwnd_e_user_input);
    DestroyWindow(hWnd);
    break;

  case WM_DESTROY:
    if (orig_textEditBrush) DeleteBrush(orig_textEditBrush);
    if (comboBrush1) DeleteBrush(comboBrush1);
    if (comboBrush2) DeleteBrush(comboBrush2);
    if (comboBrush3) DeleteBrush(comboBrush3);
    if (comboBrush4) DeleteBrush(comboBrush4);
    if (black_brush) DeleteBrush(black_brush);
    if (defaultbrush) DeleteBrush(defaultbrush);
    if (hotbrush) DeleteBrush(hotbrush);
    if (selectbrush) DeleteBrush(selectbrush);
    if (red_pen) DeletePen(red_pen);
    if (light_blue_pen) DeletePen(light_blue_pen);
    PostQuitMessage(admin_choice.load());
    return 0;

  case WM_CTLCOLOREDIT: {
    HDC dc = (HDC)wParam;
    SetTextColor(dc, color::red);
    SetBkColor(dc, color::yellow);
    SetBkMode(dc, OPAQUE);
    if (!orig_textEditBrush)
      orig_textEditBrush = CreateSolidBrush(color::yellow);
    return (INT_PTR)orig_textEditBrush;
  }

  case WM_CTLCOLORSTATIC: {
    HDC hEdit = (HDC)wParam;
    SetTextColor(hEdit, color::red);
    SetBkColor(hEdit, color::black);
    SetBkMode(hEdit, OPAQUE);
    if (!comboBrush4)
      comboBrush4 = CreateSolidBrush(color::black);
    return (INT_PTR)comboBrush4;
  }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}
