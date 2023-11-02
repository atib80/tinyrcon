#include "framework.h"
#include "resource.h"

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

extern const string program_version{ "1.0.0.7" };

extern char const *const tinyrcon_config_file_path = "config\\tinyrcon.json";

extern char const *const tempbans_file_path =
  "data\\tempbans.txt";

extern char const *const banned_ip_addresses_file_path =
  "data\\bans.txt";

extern char const *const ip_range_bans_file_path =
  "data\\ip_range_bans.txt";

extern char const *const banned_countries_list_file_path =
  "data\\banned_countries.txt";

extern char const *const banned_cities_list_file_path =
  "data\\banned_cities.txt";

extern char const *const users_data_file_path =
  "data\\users.txt";

extern std::atomic<bool> is_terminate_program;
extern volatile std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window;
extern string g_message_data_contents;

tiny_rcon_server_application main_app;

PROCESS_INFORMATION pr_info{};

condition_variable exit_flag{};
mutex mu{};

std::atomic<bool> is_display_permanently_banned_players_data_event{ false };
std::atomic<bool> is_display_temporarily_banned_players_data_event{ false };
std::atomic<bool> is_display_banned_cities_data_event{ false };
std::atomic<bool> is_display_banned_countries_data_event{ false };

extern const int screen_width{ GetSystemMetrics(SM_CXSCREEN) };
extern const int screen_height{ GetSystemMetrics(SM_CYSCREEN) - 30 };
RECT client_rect{ 0, 0, screen_width, screen_height };
extern const char *user_help_message =
  R"(^5For a list of possible commands type ^1help ^5or ^1list user ^5or ^1list rcon ^5in the console.
^3Type ^1!say "Public message" to all players" ^3[Enter] to send "Public message" to all online players.
^5Type ^1!tell 12 "Private message" ^5[Enter] to send "Private message" to only player whose pid = ^112
^3Type ^1s ^3[Enter] in the console to refresh current status of players data.
^5Type ^1!w 12 optional_reason ^5[Enter] to warn player whose pid = ^112 
       ^5(Player with ^12 warnings ^5is automatically kicked.)
^3Type ^1!k 12 optional_reason ^3[Enter] to kick player whose pid = ^112
^5Type ^1!tb 12 24 optional_reason ^5[Enter] to temporarily ban (for 24 hours) IP address of player whose pid = ^112
^3Type ^1!gb 12 optional_reason ^3[Enter] to ban IP address of player whose pid = ^112
^5Type ^1!addip 123.123.123.123 optional_reason ^5[Enter] to ban custom IP address (^1123.123.123.123^5)
^3Type ^1!ub 123.123.123.123 ^3[Enter] to remove temporarily and/or permanently banned IP address.
^5Type ^1bans ^5[Enter] to see all permanently banned IP addresses.
^3Type ^1tempbans ^3[Enter] to see all temporarily banned IP addresses.
^5Type ^1!m mapname gametype ^5[Enter] to load map 'mapname' in 'gametype' mode (^1!m mp_toujane ctf^5)
^3Type ^1!c [IP:PORT] ^3[Enter] to launch your Call of Duty game and connect to currently configured 
 game server or optionally specified game server address ^1[IP:PORT]
^5Type ^1!cp [IP:PORT] ^5[Enter] to launch your Call of Duty game and connect to currently configured 
 game server or optionally specified game server address ^1[IP:PORT] using a private slot.
^3Type ^1q ^3[Enter] to quit the program.
^5>> Press ^1F1 ^5to sort players data by their 'pid' values in ascending/descending order.
^3>> Press ^1F2 ^3to sort players data by their 'score' values in ascending/descending order.
^5>> Press ^1F3 ^5to sort players data by their 'ping' values in ascending/descending order.
^3>> Press ^1F4 ^3to sort players data by their 'name' values in ascending/descending order.
^5>> Press ^1F5 ^5to sort players data by their 'IP address' values in ascending/descending order.
^3>> Press ^1F6 ^3to sort players data by their 'country - city' values in ascending/descending order.
^5>> Press ^1F8 ^5to refresh current status of players data.
^3>> Press ^1Ctrl + W ^3to warn player.
^5>> Press ^1Ctrl + K ^5to kick player.
^3>> Press ^1Ctrl + T ^3to temp-ban player.
^5>> Press ^1Ctrl + B ^5to ban IP address of player.
^3>> Press ^1Ctrl + S ^3to refresh players' data.
^5>> Press ^1Ctrl + J ^5to connect to game server.
^3>> Press ^1Ctrl + X ^3to exit TinyRcon.
^5Type ^1!egb ^5[Enter] to enable city ban (automatic kick for banned cities).
^3Type ^1!dgb ^3[Enter] to disable city ban (automatic kick for banned cities).
^5Type ^1!bancity city_name ^5to enable automatic kick for players from city ^1city_name
^3Type ^1!unbancity city_name ^3to disable automatic kick for players from city ^1city_name
^5Type ^1!banned cities ^5[Enter] to see all currently ^1banned cities^5.
^3Type ^1!ecb ^3[Enter] to enable country ban (automatic kick for banned countries).
^5Type ^1!dcb ^5[Enter] to disable country ban (automatic kick for banned countries).
^3Type ^1!bancountry country_name ^3to enable automatic kick for players from country ^1country_name
^5Type ^1!unbancountry country_name ^5to disable automatic kick for players from country ^1country_name
^3Type ^1!banned countries ^3[Enter] to see all currently ^1banned countries^3.
)";


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

  if (!create_necessary_folders_and_files({ tinyrcon_config_file_path, tempbans_file_path, banned_ip_addresses_file_path, ip_range_bans_file_path, banned_countries_list_file_path, banned_cities_list_file_path, "log", "plugins\\geoIP" })) {
    show_error(app_handles.hwnd_main_window, "Error creating necessary program folders and files!", 0);
  }

  parse_tinyrcon_tool_config_file(tinyrcon_config_file_path);

  main_app.add_command_handler({ "cls", "!cls" }, [](const vector<string> &) {
    Edit_SetText(app_handles.hwnd_re_messages_data, "");
    g_message_data_contents.clear();
  });

  main_app.add_command_handler({ "bans", "!bans" }, [](const vector<string> &user_cmd) {
    if (!main_app.get_is_connection_settings_valid()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "^3You need to have the correct ^1rcon password ^3to be able to execute ^1admin-level ^3commands!\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return;
    }
    if (user_cmd.size() == 3U && user_cmd[1] == "clear" && user_cmd[2] == "all") {
      for (const auto &ip : main_app.get_game_server().get_banned_ip_addresses_map()) {
        string message;
        string ip_address{ ip.first };
        remove_permanently_banned_ip_address(ip_address, message, false);
      }
    }
    is_display_permanently_banned_players_data_event.store(true);
  });

  main_app.add_command_handler({ "tempbans", "!tempbans" }, [](const vector<string> &user_cmd) {
    if (!main_app.get_is_connection_settings_valid()) {
      print_colored_text(app_handles.hwnd_re_messages_data, "^3You need to have the correct ^1rcon password ^3to be able to execute ^1admin-level ^3commands!\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      return;
    }
    if (user_cmd.size() == 3U && user_cmd[1] == "clear" && user_cmd[2] == "all") {
      for (const auto &ip : main_app.get_game_server().get_temp_banned_ip_addresses_map()) {
        string message;
        remove_temp_banned_ip_address(ip.first, message, false);
      }
    }

    is_display_temporarily_banned_players_data_event.store(true);
  });


  main_app.add_message_handler("inform-message", [](const string &, const time_t, const string &data, bool) {
    print_colored_text(app_handles.hwnd_re_messages_data, data.c_str());
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 2) {
      const string sent_by{ get_cleaned_user_name(parts[0]) };
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        const string admin_name{ get_cleaned_user_name(u->user_name) };
        if (admin_name != sent_by && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("inform-message", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("public-message", [](const string &, const time_t, const string &data, bool) {
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

  main_app.add_message_handler("private-message", [](const string &, const time_t, const string &data, bool) {
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

  main_app.add_message_handler("request-login", [](const string &user, const time_t timestamp, const string &data, bool) {
    const string message{ format("^7{} ^3has sent a ^1login request ^3to ^5Tiny^6Rcon ^5server.", user) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {
      auto &current_user = main_app.get_user_for_name(parts[0]);
      current_user->ip_address = std::move(parts[1]);
      player_data pd{};
      convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), current_user->ip_address, pd);
      current_user->geo_information = format("{}, {}", pd.country_name, pd.city);
      current_user->country_code = pd.country_code;
      current_user->no_of_logins++;
      current_user->is_logged_in = true;
      current_user->last_login_time_stamp = get_current_time_stamp();
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string message{ format("^7{} ^2has logged in to ^5Tiny^6Rcon ^5server.\n^2Number of logins: ^1{}", current_user->user_name, current_user->no_of_logins) };
      main_app.add_message_to_queue(message_t("confirm-login", format("{}\\{}\\{}", current_user->user_name, current_user->ip_address, current_user->no_of_logins), current_user, true));
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != current_user->user_name && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("inform-login", message, true, u);
        }
      }
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    }
  });

  main_app.add_message_handler("request-logout", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages) {
    if (is_print_in_messages) {
      const string message{ format("^7{} ^3has sent a ^1logout request ^3to ^5Tiny^6Rcon ^5server.", user) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
      auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
      for (auto &part : parts) stl::helper::trim_in_place(part);
      if (parts.size() >= 3) {
        const auto &current_user = main_app.get_user_for_name(parts[0]);
        current_user->ip_address = std::move(parts[1]);
        player_data pd{};
        convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), current_user->ip_address, pd);
        current_user->geo_information = format("{}, {}", pd.country_name, pd.city);
        current_user->country_code = pd.country_code;
        current_user->is_logged_in = false;
        current_user->last_logout_time_stamp = get_current_time_stamp();
        save_tiny_rcon_users_data_to_json_file(users_data_file_path);
        display_users_data_in_users_table(app_handles.hwnd_users_table);
        const string message{ format("^7{} ^2has logged out of ^5Tiny^6Rcon ^5server.\n", current_user->user_name) };
        main_app.add_message_to_queue(message_t("confirm-logout", format("{}\\{}\\{}", current_user->user_name, current_user->ip_address, current_user->no_of_logins), current_user, true));
        for (const auto &u : main_app.get_users()) {
          unsigned long ip_key{};
          if (u->user_name != current_user->user_name && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
            main_app.get_connection_manager_for_messages().process_and_send_message("inform-logout", message, true, u);
          }
        }
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
      }
    }
  });

  main_app.add_message_handler("upload-admindata", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 17) {
      auto &u = main_app.get_user_for_name(parts[0]);
      u->user_name = std::move(parts[0]);
      u->is_admin = parts[1] == "true";
      u->is_logged_in = parts[2] == "true";
      u->is_online = parts[3] == "true";

      unsigned long guid{};
      u->ip_address = parts[4];
      if (check_ip_address_validity(u->ip_address, guid)) {
        player_data admin{};
        convert_guid_key_to_country_name(main_app.get_connection_manager_for_messages().get_geoip_data(), parts[4], admin);
        u->country_code = admin.country_code;
        u->geo_information = format("{}, {}", admin.country_name, admin.city);
      } else {
        u->geo_information = "n/a";
        u->country_code = "xy";
      }

      u->last_login_time_stamp = stoll(parts[6]);
      u->last_logout_time_stamp = stoll(parts[7]);
      u->no_of_logins = stoul(parts[8]);
      u->no_of_warnings = stoul(parts[9]);
      u->no_of_kicks = stoul(parts[10]);
      u->no_of_tempbans = stoul(parts[11]);
      u->no_of_guidbans = stoul(parts[12]);
      u->no_of_ipbans = stoul(parts[13]);
      u->no_of_iprangebans = stoul(parts[14]);
      u->no_of_citybans = stoul(parts[15]);
      u->no_of_countrybans = stoul(parts[16]);

      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
    }
  });

  main_app.add_message_handler("request-admindata", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages) {
    /*const string message{ format("^3Received ^1'request-admindata' ^3message from ^1admin ^7{}.\n", user) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());*/
    // auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    // for (auto &part : parts) stl::helper::trim_in_place(part);
    // if (parts.size() >= 3) {
    const auto &users = main_app.get_users();
    if (!users.empty()) {
      for (size_t i{}; i < users.size(); ++i) {
        // if (users[i]->user_name != parts[0]) {
        const string admin_data{ format(R"({}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{})", users[i]->user_name, (users[i]->is_admin ? "true" : "false"), (users[i]->is_logged_in ? "true" : "false"), (users[i]->is_online ? "true" : "false"), users[i]->ip_address, users[i]->geo_information, users[i]->last_login_time_stamp, users[i]->last_logout_time_stamp, users[i]->no_of_logins, users[i]->no_of_warnings, users[i]->no_of_kicks, users[i]->no_of_tempbans, users[i]->no_of_guidbans, users[i]->no_of_ipbans, users[i]->no_of_iprangebans, users[i]->no_of_citybans, users[i]->no_of_countrybans) };
        main_app.add_message_to_queue(message_t("receive-admindata", admin_data, main_app.get_user_for_name(user)));
        // }
      }
    }
    // }
  });


  main_app.add_message_handler("upload-ipbans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages) {
    // const string message{ format("^3Received ^1'upload-ipbans' ^3request from ^7{}\n^3data->\"^1{}^3\"", user, data) };
    // print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string admins_ipbans_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
    vector<player_data> admins_banned_ip_addresses_data;
    unordered_map<string, player_data> admins_banned_ip_to_player_data;
    parse_banned_ip_addresses_file(admins_ipbans_file_path.c_str(), admins_banned_ip_addresses_data, admins_banned_ip_to_player_data);
    auto &tiny_rcon_server_banned_ip_addresses_vector = main_app.get_game_server().get_banned_ip_addresses_vector();
    auto &tiny_rcon_server_banned_ip_addresses_map = main_app.get_game_server().get_banned_ip_addresses_map();
    for (auto &pd : admins_banned_ip_addresses_data) {
      if (!tiny_rcon_server_banned_ip_addresses_map.contains(pd.ip_address)) {
        tiny_rcon_server_banned_ip_addresses_map.emplace(pd.ip_address, pd);
        tiny_rcon_server_banned_ip_addresses_vector.push_back(std::move(pd));
      }
    }

    std::sort(std::begin(tiny_rcon_server_banned_ip_addresses_vector), std::end(tiny_rcon_server_banned_ip_addresses_vector), [](const player_data &pd1, const player_data &pd2) {
      return pd1.banned_start_time < pd2.banned_start_time;
    });

    save_banned_ip_entries_to_file(admins_ipbans_file_path.c_str(), tiny_rcon_server_banned_ip_addresses_vector);
    const string file_path{ main_app.get_current_working_directory() + banned_ip_addresses_file_path };
    save_banned_ip_entries_to_file(file_path.c_str(), tiny_rcon_server_banned_ip_addresses_vector);
    const auto &sender_user = main_app.get_user_for_name(user);
    main_app.add_message_to_queue(message_t("receive-ipbans", data, sender_user, true));
  });

  main_app.add_message_handler("upload-iprangebans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages) {
    // const string message{ format("^3Received ^1'upload-ipbanranges' ^3request from ^7{}\n^3data->\"^1{}^3\"", user, data) };
    // print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string admins_ip_address_range_bans_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
    vector<player_data> admins_banned_ip_address_ranges_data;
    unordered_map<string, player_data> admins_banned_ip_address_range_to_player_data;
    parse_banned_ip_address_ranges_file(admins_ip_address_range_bans_file_path.c_str(), admins_banned_ip_address_ranges_data, admins_banned_ip_address_range_to_player_data);
    auto &tiny_rcon_server_banned_ip_address_ranges_vector = main_app.get_game_server().get_banned_ip_address_ranges_vector();
    auto &tiny_rcon_server_banned_ip_address_ranges_map = main_app.get_game_server().get_banned_ip_address_ranges_map();
    for (auto &pd : admins_banned_ip_address_ranges_data) {
      if (!tiny_rcon_server_banned_ip_address_ranges_map.contains(pd.ip_address)) {
        tiny_rcon_server_banned_ip_address_ranges_map.emplace(pd.ip_address, pd);
        tiny_rcon_server_banned_ip_address_ranges_vector.push_back(std::move(pd));
      }
    }

    std::sort(std::begin(tiny_rcon_server_banned_ip_address_ranges_vector), std::end(tiny_rcon_server_banned_ip_address_ranges_vector), [](const player_data &pd1, const player_data &pd2) {
      return pd1.banned_start_time < pd2.banned_start_time;
    });

    save_banned_ip_address_range_entries_to_file(admins_ip_address_range_bans_file_path.c_str(), tiny_rcon_server_banned_ip_address_ranges_vector);
    const string banned_ip_address_ranges_file_path{ main_app.get_current_working_directory() + "data\\ip_range_bans.txt" };
    save_banned_ip_address_range_entries_to_file(banned_ip_address_ranges_file_path.c_str(), tiny_rcon_server_banned_ip_address_ranges_vector);
    const auto &sender_user = main_app.get_user_for_name(user);
    main_app.add_message_to_queue(message_t("receive-iprangebans", data, sender_user, true));
  });

  main_app.add_message_handler("upload-citybans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages) {
    // const string message{ format("^3Received ^1'upload-citybans' ^3request from ^7{}\n^3data->\"^1{}^3\"", user, data) };
    // print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
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
    const string banned_cities_file_path{ main_app.get_current_working_directory() + "data\\banned_cities.txt" };
    save_banned_cities_to_file(banned_cities_file_path.c_str(), tiny_rcon_server_banned_cities_set);
    const auto &sender_user = main_app.get_user_for_name(user);
    main_app.add_message_to_queue(message_t("receive-citybans", data, sender_user, true));
  });

  main_app.add_message_handler("upload-countrybans", [](const string &user, const time_t timestamp, const string &data, bool is_print_in_messages) {
    // const string message{ format("^3Received ^1'upload-countrybans' ^3request from ^7{}\n^3data->\"^1{}^3\"", user, data) };
    // print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string admins_banned_countries_file_path{ format("{}\\{}", main_app.get_ftp_bans_folder_path(), data) };
    set<string> admins_banned_countries_set;
    parse_banned_cities_file(admins_banned_countries_file_path.c_str(), admins_banned_countries_set);
    auto &tiny_rcon_server_banned_countries_set = main_app.get_game_server().get_banned_countries_set();
    for (const auto &banned_country : admins_banned_countries_set) {
      if (!tiny_rcon_server_banned_countries_set.contains(banned_country)) {
        tiny_rcon_server_banned_countries_set.emplace(banned_country);
      }
    }

    save_banned_countries_to_file(admins_banned_countries_file_path.c_str(), tiny_rcon_server_banned_countries_set);
    const string banned_countries_file_path{ main_app.get_current_working_directory() + "data\\banned_countries.txt" };
    save_banned_countries_to_file(banned_countries_file_path.c_str(), tiny_rcon_server_banned_countries_set);
    const auto &sender_user = main_app.get_user_for_name(user);
    main_app.add_message_to_queue(message_t("receive-countrybans", data, sender_user, true));
  });


  main_app.add_message_handler("add-warning", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5) {
      player_data warned_player{};
      strcpy_s(warned_player.ip_address, std::size(warned_player.ip_address), parts[0].c_str());
      strcpy_s(warned_player.guid_key, std::size(warned_player.guid_key), parts[1].c_str());
      strcpy_s(warned_player.player_name, std::size(warned_player.player_name), parts[2].c_str());
      strcpy_s(warned_player.banned_date_time, std::size(warned_player.banned_date_time), parts[3].c_str());
      warned_player.reason = std::move(parts[4]);
      warned_player.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        warned_player.ip_address,
        warned_player);
      main_app.get_user_for_name(user)->no_of_warnings++;
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has ^1warned ^3player ^7{} ^3[^5IP address ^1{} ^3| ^5GUID: ^1{}\n^5Date/time of warning: ^1{} ^3| ^5Reason of warning: ^1{} ^3| ^5Warned by: ^7{}\n", user, warned_player.player_name, warned_player.ip_address, warned_player.guid_key, warned_player.banned_date_time, warned_player.reason, warned_player.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-warning", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-kick", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5) {
      player_data kicked_player{};
      strcpy_s(kicked_player.ip_address, std::size(kicked_player.ip_address), parts[0].c_str());
      strcpy_s(kicked_player.guid_key, std::size(kicked_player.guid_key), parts[1].c_str());
      strcpy_s(kicked_player.player_name, std::size(kicked_player.player_name), parts[2].c_str());
      strcpy_s(kicked_player.banned_date_time, std::size(kicked_player.banned_date_time), parts[3].c_str());
      kicked_player.reason = std::move(parts[4]);
      kicked_player.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        kicked_player.ip_address,
        kicked_player);
      main_app.get_user_for_name(user)->no_of_kicks++;
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has ^1kicked ^3player ^7{} ^3[^5IP address ^1{} ^3| ^5GUID: ^1{}\n^5Date/time of kick: ^1{} ^3| ^5Reason of kick: ^1{} ^3| ^5Kicked by: ^7{}\n", user, kicked_player.player_name, kicked_player.ip_address, kicked_player.guid_key, kicked_player.banned_date_time, kicked_player.reason, kicked_player.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-kick", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-tempban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 6 && !main_app.get_game_server().get_temp_banned_ip_addresses_map().contains(parts[0])) {
      player_data temp_banned_player_data{};
      strcpy_s(temp_banned_player_data.ip_address, std::size(temp_banned_player_data.ip_address), parts[0].c_str());
      strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name), parts[1].c_str());
      strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time), parts[2].c_str());
      temp_banned_player_data.banned_start_time = stoll(parts[3]);
      temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
      temp_banned_player_data.reason = std::move(parts[5]);
      temp_banned_player_data.banned_by_user_name = parts.size() >= 7 ? std::move(parts[6]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        temp_banned_player_data.ip_address,
        temp_banned_player_data);
      main_app.get_user_for_name(user)->no_of_tempbans++;
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^5has temporarily banned ^1IP address {}\n ^5for ^3player name: ^7{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Ban duration: ^1{} hours ^5| ^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, temp_banned_player_data.ip_address, temp_banned_player_data.player_name, temp_banned_player_data.guid_key, temp_banned_player_data.banned_date_time, temp_banned_player_data.ban_duration_in_hours, temp_banned_player_data.reason, temp_banned_player_data.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_temporarily_banned_ip_address(temp_banned_player_data, main_app.get_game_server().get_temp_banned_players_data(), main_app.get_game_server().get_temp_banned_ip_addresses_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-tempban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("remove-tempban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 6 && main_app.get_game_server().get_temp_banned_ip_addresses_map().contains(parts[0])) {
      player_data temp_banned_player_data{};
      strcpy_s(temp_banned_player_data.ip_address, std::size(temp_banned_player_data.ip_address), parts[0].c_str());
      strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name), parts[1].c_str());
      strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time), parts[2].c_str());
      temp_banned_player_data.banned_start_time = stoll(parts[3]);
      temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
      temp_banned_player_data.reason = std::move(parts[5]);
      temp_banned_player_data.banned_by_user_name = parts.size() >= 7 ? std::move(parts[6]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        temp_banned_player_data.ip_address,
        temp_banned_player_data);
      const string msg{ format("^7{} ^5has removed temporarily banned ^1IP address {}\n ^5for ^3player name: ^7{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Ban duration: ^1{} hours ^5| ^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, temp_banned_player_data.ip_address, temp_banned_player_data.player_name, temp_banned_player_data.guid_key, temp_banned_player_data.banned_date_time, temp_banned_player_data.ban_duration_in_hours, temp_banned_player_data.reason, temp_banned_player_data.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      string message_about_removal;
      remove_temp_banned_ip_address(temp_banned_player_data.ip_address, message_about_removal, false, false);
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-tempban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-guidban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && !main_app.get_game_server().get_banned_ip_addresses_map().contains(parts[0])) {
      player_data pd{};
      strcpy_s(pd.ip_address, std::size(pd.ip_address), parts[0].c_str());
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);
      main_app.get_user_for_name(user)->no_of_guidbans++;
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^5has banned the ^1GUID key ^5of player:\n^3Name: ^7{} ^5| ^3IP address: ^1{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, pd.player_name, pd.ip_address, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-guidban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-ipban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && !main_app.get_game_server().get_banned_ip_addresses_map().contains(parts[0])) {
      player_data pd{};
      strcpy_s(pd.ip_address, std::size(pd.ip_address), parts[0].c_str());
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);
      main_app.get_user_for_name(user)->no_of_ipbans++;
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has permanently banned ^1IP address ^3of player ^7{}.\n^3IP address: ^1{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of GUID ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, pd.player_name, pd.ip_address, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_ip_address(pd, main_app.get_game_server().get_banned_ip_addresses_vector(), main_app.get_game_server().get_banned_ip_addresses_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-ipban", data, true, u);
        }
      }
    }
  });


  main_app.add_message_handler("remove-ipban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && main_app.get_game_server().get_banned_ip_addresses_map().contains(parts[0])) {
      player_data pd{};
      strcpy_s(pd.ip_address, std::size(pd.ip_address), parts[0].c_str());
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has removed ^1banned IP address {}\n ^5for ^3player name: ^7{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, pd.ip_address, pd.player_name, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      string ip_address{ pd.ip_address };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      string message_about_removal;
      remove_permanently_banned_ip_address(ip_address, message_about_removal, false);
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-ipban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-iprangeban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 5 && !main_app.get_game_server().get_banned_ip_address_ranges_map().contains(parts[0])) {
      player_data pd{};
      strcpy_s(pd.ip_address, std::size(pd.ip_address), parts[0].c_str());
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);
      main_app.get_user_for_name(user)->no_of_iprangebans++;     
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^5has banned ^1IP address range:\n^5[^3Player name: ^7{} ^5| ^3IP address range: ^1{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}^5]\n", pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_ip_address_range(pd, main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-iprangeban", data, true, u);
        }
      }

  } });

  main_app.add_message_handler("remove-iprangeban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (main_app.get_game_server().get_banned_ip_address_ranges_map().contains(parts[0])) {
      player_data pd{};
      strcpy_s(pd.ip_address, std::size(pd.ip_address), parts[0].c_str());
      strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
      strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
      strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
      pd.reason = std::move(parts[4]);
      pd.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager_for_messages().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has removed previously ^1banned IP address range:\n^5[^3Player name: ^7{} ^5| ^3IP range: ^1{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}^5]\n", user, pd.player_name, pd.ip_address, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      string ip_address{ pd.ip_address };
      string message_about_removal;
      remove_permanently_banned_ip_address_range(pd, main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != user && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-iprangeban", data, true, u);
        }
      }
    }
  });
  main_app.add_message_handler("add-cityban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string city{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };
      main_app.get_user_for_name(user)->no_of_citybans++;
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has banned city ^1{} ^3at ^1{}\n", admin, city, get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_city(city, main_app.get_game_server().get_banned_cities_set());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != admin && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-cityban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("remove-cityban", [](const string &user, const time_t, const string &data, bool) {
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
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != admin && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-cityban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("add-countryban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string country{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };
      main_app.get_user_for_name(user)->no_of_countrybans++;
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      const string msg{ format("^7{} ^3has banned country ^1{} ^3at ^1{}\n", admin, country, get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_country(country, main_app.get_game_server().get_banned_countries_set());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != admin && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("add-countryban", data, true, u);
        }
      }
    }
  });

  main_app.add_message_handler("remove-countryban", [](const string &user, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string city{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };

      const string msg{ format("^7{} ^2has removed banned country ^1{} ^2at ^1{}\n", admin, city, get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      remove_permanently_banned_country(city, main_app.get_game_server().get_banned_countries_set());
      for (const auto &u : main_app.get_users()) {
        unsigned long ip_key{};
        if (u->user_name != admin && u->is_logged_in && check_ip_address_validity(u->ip_address, ip_key)) {
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-countryban", data, true, u);
        }
      }
    }
  });

  main_app.set_command_line_info(user_help_message);

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


  construct_tinyrcon_gui(app_handles.hwnd_main_window);
  main_app.open_log_file("log/commands_history.log");
  main_app.set_current_working_directory();

  const string program_title{ main_app.get_program_title() + " | "s + main_app.get_game_server_name() + " | "s + "version: "s + program_version };
  SetWindowText(app_handles.hwnd_main_window, program_title.c_str());

  CenterWindow(app_handles.hwnd_main_window);

  MSG msg{};

  SetFocus(app_handles.hwnd_e_user_input);
  // PostMessage(app_handles.hwnd_progress_bar, PBM_SETMARQUEE, (WPARAM)TRUE, (LPARAM)5);

  std::thread task_thread{
    [&]() {
      IsGUIThread(TRUE);
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started parsing ^1tinyrcon.json ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished parsing ^1tinyrcon.json ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      /*{
        auto_update_manager au{};
        main_app.set_current_working_directory(au.get_self_current_working_directory());
      }*/
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started importing serialized binary geological data from\n ^1'plugins/geoIP/geo.dat' ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      // const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\IP2LOCATION-LITE-DB3.csv" };
      // parse_geodata_lite_csv_file(geo_dat_file_path.c_str());
      const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\geo.dat" };
      import_geoip_data(main_app.get_connection_manager_for_messages().get_geoip_data(), geo_dat_file_path.c_str());

      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished importing serialized binary geological data from\n ^1'plugins/geoIP/geo.dat' ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_tinyrcon_server_users_data(users_data_file_path);

      parse_tempbans_data_file(tempbans_file_path, main_app.get_game_server().get_temp_banned_players_data(), main_app.get_game_server().get_temp_banned_ip_addresses_map());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1temporarily banned IP addresses ^2from ^5data\\tempbans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_ip_addresses_file(banned_ip_addresses_file_path, main_app.get_game_server().get_banned_ip_addresses_vector(), main_app.get_game_server().get_banned_ip_addresses_map());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1permanently banned IP addresses ^2from ^5data\\bans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_ip_address_ranges_file(ip_range_bans_file_path, main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned IP address ranges ^2from ^5data\\ip_range_bans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_cities_file(banned_cities_list_file_path, main_app.get_game_server().get_banned_cities_set());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned cities ^2from ^5data\\banned_cities.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_countries_file(banned_countries_list_file_path, main_app.get_game_server().get_banned_countries_set());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned countries ^2from ^5data\\banned_countries.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      is_main_window_constructed = true;

      while (true) {
        {
          unique_lock ul{ mu };
          exit_flag.wait_for(ul, 100ms, [&]() {
            return is_terminate_program.load();
          });
        }

        if (is_terminate_program.load()) break;

        while (!main_app.is_command_queue_empty()) {
          auto cmd = main_app.get_command_from_queue();
          main_app.process_queue_command(std::move(cmd));
        }
      }
    }
  };

  std::thread messaging_thread{
    [&]() {
      while (true) {
        /*{
          unique_lock ul{ mu };
          exit_flag.wait_for(ul, 10ms, [&]() {
            return is_terminate_program.load();
          });
        }

        if (is_terminate_program.load()) {
          break;
        }*/

        main_app.get_connection_manager_for_messages().wait_for_and_process_response_message();

        while (!main_app.is_message_queue_empty()) {
          message_t message{ main_app.get_message_from_queue() };
          main_app.get_connection_manager_for_messages().process_and_send_message(message.command, message.data, message.is_show_in_messages, message.sender);
        }
      }
    }
  };

  messaging_thread.detach();

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

  save_tiny_rcon_users_data_to_json_file(users_data_file_path);

  is_terminate_program.store(true);
  {
    lock_guard ul{ mu };
    exit_flag.notify_all();
  }

  task_thread.join();

  if (pr_info.hProcess != NULL)
    CloseHandle(pr_info.hProcess);
  if (pr_info.hThread != NULL)
    CloseHandle(pr_info.hThread);


  log_message("Exiting TinyRcon program.", is_log_datetime::yes);

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
  // UnregisterClass(wcex_configuration_dialog.lpszClassName, app_handles.hInstance);

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

  app_handles.hwnd_main_window = CreateWindowEx(0, wcex.lpszClassName, "Welcome to TinyRcon | version: 2.4", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME /*WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL*/, 0, 0, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, nullptr, nullptr, hInstance, nullptr);

  if (!app_handles.hwnd_main_window)
    return false;

  ShowWindow(app_handles.hwnd_main_window, nCmdShow);
  UpdateWindow(app_handles.hwnd_main_window);
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
      InsertMenu(hPopupMenu, 5, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, 6, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "&Clear TinyRcon messages");
      InsertMenu(hPopupMenu, 7, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, 8, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit TinyRcon");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    } else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_e_user_input) {
      hPopupMenu = CreatePopupMenu();
      InsertMenu(hPopupMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDC_PASTE, "&Paste");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    }
  } break;

  case WM_TIMER: {

    ++counter;
    if (counter % 10 == 0) {
      display_users_data_in_users_table(app_handles.hwnd_users_table);
      display_online_admins_information();      
    }

    if (counter % 30 == 0) {
      counter = 0;
      save_tiny_rcon_users_data_to_json_file(users_data_file_path);
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
      display_temporarily_banned_ip_addresses();
      is_display_temporarily_banned_players_data_event.store(false);
    } else if (is_display_permanently_banned_players_data_event.load()) {
      display_permanently_banned_ip_addresses();
      is_display_permanently_banned_players_data_event.store(false);
    } else if (is_display_banned_cities_data_event.load()) {
      display_banned_cities(main_app.get_game_server().get_banned_cities_set());
      is_display_banned_cities_data_event.store(false);
    } else if (is_display_banned_countries_data_event.load()) {
      display_banned_countries(main_app.get_game_server().get_banned_countries_set());
      is_display_banned_countries_data_event.store(false);
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
      {
        lock_guard ul{ mu };
        exit_flag.notify_all();
      }
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

    case ID_VIEWBANNEDCITIES:
      is_display_banned_cities_data_event.store(true);
      break;

    case ID_VIEWBANNEDCOUNTRIES:
      is_display_banned_countries_data_event.store(true);
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
    // if (show_user_confirmation_dialog("^3Do you really want to exit?", "Exit TinyRcon?")) {
    is_terminate_program.store(true);
    {
      lock_guard ul{ mu };
      exit_flag.notify_all();
    }
    DestroyWindow(app_handles.hwnd_main_window);
    // }
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
