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

extern const string program_version{ "2.5.1.6" };

extern const std::regex ip_address_and_port_regex;

extern std::atomic<bool> is_terminate_program;
extern volatile std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window;
extern string g_message_data_contents;

tiny_rcon_client_application main_app;
sort_type type_of_sort{ sort_type::geo_asc };

PROCESS_INFORMATION pr_info{};

condition_variable exit_flag{};
mutex mu{};

volatile atomic<size_t> atomic_counter{ 0 };
volatile std::atomic<bool> is_refresh_players_data_event{ false };
volatile std::atomic<bool> is_refreshed_players_data_ready_event{ false };
volatile std::atomic<bool> is_display_temporarily_banned_players_data_event{ false };
volatile std::atomic<bool> is_display_permanently_banned_players_data_event{ false };
volatile std::atomic<bool> is_display_banned_ip_address_ranges_data_event{ false };
volatile std::atomic<bool> is_display_banned_cities_data_event{ false };
volatile std::atomic<bool> is_display_banned_countries_data_event{ false };
volatile std::atomic<bool> is_display_admins_data_event{ false };

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
^5Type ^1!br 12|123.123.123.* optional_reason ^5[Enter] to ban IP address range of player whose pid = ^112 ^5or IP address range = ^1123.123.123.*
^3Type ^1!ubr 123.123.123.* optional_reason ^3[Enter] to remove previously banned IP address range ^1123.123.123.*
^5Type ^1!ranges ^5[Enter] to see all currently banned IP address ranges.
^3Type ^1!admins ^3[Enter] to see all registered admins' data and their statistics.
^5Type ^1!t message ^5[Enter] to send ^1message ^5to all logged in ^1admins.
^3Type ^1!y username message ^3[Enter] to send ^1message ^3to ^1admin ^3whose user name is ^1username. 
^5Examples: ^3!y }|{opuk smotri Pronik#123, !y username privet. ^1Color codes in username are ignored.
^1Capital letters in username are converted into small letters. ACID = acid, ^1W^4W ^3 = ww
)";

extern const std::unordered_map<string, sort_type> sort_mode_names_dict;

extern const std::unordered_map<int, const char *> button_id_to_label_text{
  { ID_WARNBUTTON, "&Warn" },
  { ID_KICKBUTTON, "&Kick" },
  { ID_TEMPBANBUTTON, "&Tempban" },
  { ID_IPBANBUTTON, "&Ban IP" },
  { ID_VIEWTEMPBANSBUTTON, "View temporary bans" },
  { ID_VIEWIPBANSBUTTON, "View &IP bans" },
  { ID_VIEWADMINSDATA, "View &admins' data" },
  { ID_REFRESHDATABUTTON, "Refre&sh data" },
  { ID_CONNECTBUTTON, "&Join server" },
  { ID_CONNECTPRIVATESLOTBUTTON, "Joi&n server (private slot)" },
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


unordered_map<size_t, string> rcon_status_grid_column_header_titles;
unordered_map<size_t, string> get_status_grid_column_header_titles;

extern const char *prompt_message{ "Administrator >>" };
extern const char *refresh_players_data_fmt_str{ "Refreshing players data in %zu %s." };
extern const size_t max_players_grid_rows;

tiny_rcon_handles app_handles{};

WNDCLASSEX wcex, wcex_confirmation_dialog, wcex_configuration_dialog;

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
LRESULT CALLBACK WndProcForConfigurationDialog(HWND, UINT, WPARAM, LPARAM);

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

  const string config_file_path{ format("{}{}", main_app.get_current_working_directory(), "config\\tinyrcon.json") };
  const string log_folder_path{ format("{}{}", main_app.get_current_working_directory(), "log") };
  const string plugins_geoIP_folder_path{ format("{}{}", main_app.get_current_working_directory(), "plugins\\geoIP") };

  rcon_status_grid_column_header_titles[0] = main_app.get_game_server().get_header_player_pid_color() + "Pid"s;
  rcon_status_grid_column_header_titles[1] = main_app.get_game_server().get_header_player_score_color() + "Score"s;
  rcon_status_grid_column_header_titles[2] = main_app.get_game_server().get_header_player_ping_color() + "Ping"s;
  rcon_status_grid_column_header_titles[3] = main_app.get_game_server().get_header_player_name_color() + "Player name"s;
  rcon_status_grid_column_header_titles[4] = main_app.get_game_server().get_header_player_ip_color() + "IP address"s;
  rcon_status_grid_column_header_titles[5] = main_app.get_game_server().get_header_player_geoinfo_color() + "Geological information"s;
  rcon_status_grid_column_header_titles[6] = main_app.get_game_server().get_header_player_geoinfo_color() + "Flag"s;

  get_status_grid_column_header_titles[0] = main_app.get_game_server().get_header_player_pid_color() + "Player no."s;
  get_status_grid_column_header_titles[1] = main_app.get_game_server().get_header_player_score_color() + "Score"s;
  get_status_grid_column_header_titles[2] = main_app.get_game_server().get_header_player_ping_color() + "Ping"s;
  get_status_grid_column_header_titles[3] = main_app.get_game_server().get_header_player_name_color() + "Player name"s;

  construct_tinyrcon_gui(app_handles.hwnd_main_window);

  parse_tinyrcon_tool_config_file(config_file_path.c_str());

  if (!create_necessary_folders_and_files({ main_app.get_tinyrcon_config_file_path(), main_app.get_tempbans_file_path(), main_app.get_banned_ip_addresses_file_path(), main_app.get_ip_range_bans_file_path(), main_app.get_banned_countries_list_file_path(), main_app.get_banned_cities_list_file_path(), log_folder_path, plugins_geoIP_folder_path })) {
    show_error(app_handles.hwnd_main_window, "Error creating necessary program folders and files!", 0);
  }
  const string log_file_path{ format("{}{}", log_folder_path, "\\commands_history.log") };
  main_app.open_log_file(log_file_path.c_str());

  load_tinyrcon_client_user_data(main_app.get_user_data_file_path());

  auto &me = main_app.get_current_user();

  me->user_name = main_app.get_username();
  me->ip_address = get_tiny_rcon_client_external_ip_address();
  // unsigned long guid{};
  // if (!check_ip_address_validity(me->ip_address, guid))
  //  removed_disallowed_character_in_string(me->ip_address);
  me->is_admin = main_app.get_game_server().get_is_connection_settings_valid();
  me->is_logged_in = true;

  main_app.add_command_handler({ "cls", "!cls" }, [](const vector<string> &) {
    Edit_SetText(app_handles.hwnd_re_messages_data, "");
    g_message_data_contents.clear();
  });


  main_app.add_command_handler({ "list", "!list", "help", "!help", "h", "!h" }, [](const vector<string> &user_cmd) {
    print_help_information(user_cmd);
  });

  main_app.add_command_handler({ "!w", "!warn" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {

      if (check_if_user_provided_argument_is_valid_for_specified_command(
            user_cmd[0].c_str(), user_cmd[1])) {
        const int pid{ stoi(user_cmd[1]) };
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        string reason{ remove_disallowed_character_in_string(user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified") };
        stl::helper::trim_in_place(reason);
        specify_reason_for_player_pid(pid, reason);
        main_app.get_tinyrcon_dict()["{REASON}"] = reason;
        string command{ main_app.get_user_defined_warn_message() };
        build_tiny_rcon_message(command);
        rcon_say(command);
        auto &warned_players = main_app.get_game_server().get_warned_players_data();
        auto [player, is_online] = get_online_player_for_specified_pid(pid);
        if (is_online) {
          if (!warned_players.contains(pid)) {
            warned_players[pid] = move(player);
            warned_players[pid].warned_times = 1;
          } else {
            ++warned_players[pid].warned_times;
          }

          const string message{ format("^3You have successfully executed ^5!warn ^3on player ({}^3)\n", get_player_information(pid)) };
          print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

          const string date_time_info{ get_date_and_time_for_time_t("{DD}/{MM}/{Y} {hh}:{mm}") };
          strcpy_s(warned_players[pid].banned_date_time, std::size(warned_players[pid].banned_date_time), date_time_info.c_str());
          warned_players[pid].reason = reason;
          warned_players[pid].banned_by_user_name = main_app.get_username();
          auto &current_user = main_app.get_user_for_name(main_app.get_username());
          current_user->no_of_warnings++;
          save_current_user_data_to_json_file(main_app.get_user_data_file_path());
          main_app.get_connection_manager_for_messages().process_and_send_message("add-warning", format("{}\\{}\\{}\\{}\\{}\\{}", warned_players[pid].ip_address, warned_players[pid].guid_key, warned_players[pid].player_name, warned_players[pid].banned_date_time, reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);

          const size_t number_of_warnings_for_automatic_kick = main_app.get_game_server().get_maximum_number_of_warnings_for_automatic_kick();
          if (warned_players[pid].warned_times >= number_of_warnings_for_automatic_kick) {
            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
            string reason2{ remove_disallowed_character_in_string(format("^1Received {} warnings from admin: {}", main_app.get_game_server().get_maximum_number_of_warnings_for_automatic_kick(), main_app.get_username())) };
            stl::helper::trim_in_place(reason2);
            specify_reason_for_player_pid(pid, reason2);
            main_app.get_tinyrcon_dict()["{REASON}"] = reason2;
            string command2{ main_app.get_user_defined_kick_message() };
            build_tiny_rcon_message(command2);
            warned_players[pid].reason = reason;
            current_user->no_of_kicks++;
            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
            main_app.get_connection_manager_for_messages().process_and_send_message("add-kick", format("{}\\{}\\{}\\{}\\{}\\{}", warned_players[pid].ip_address, warned_players[pid].guid_key, warned_players[pid].player_name, warned_players[pid].banned_date_time, reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            kick_player(pid, command2);
            warned_players.erase(pid);
          }
        }
      }
    } else {
      const string re_msg2{ format("^3Invalid command syntax for user command: ^2{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg2.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(user_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "!k", "!kick" }, [](const std::vector<std::string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;

    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {


      if (check_if_user_provided_argument_is_valid_for_specified_command(
            user_cmd[0].c_str(), user_cmd[1])) {
        const int pid{ stoi(user_cmd[1]) };
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
        string reason{ remove_disallowed_character_in_string(user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified") };
        stl::helper::trim_in_place(reason);
        specify_reason_for_player_pid(pid, reason);
        main_app.get_tinyrcon_dict()["{REASON}"] = reason;
        const string message{ format("^3You have successfully executed ^5clientkick ^3on player ({}^3)\n", get_player_information(pid)) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        string command{ main_app.get_user_defined_kick_message() };
        build_tiny_rcon_message(command);
        auto &current_user = main_app.get_user_for_name(main_app.get_username());
        current_user->no_of_kicks++;
        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
        player_data &player{ get_player_data_for_pid(pid) };
        const string date_time_info{ get_date_and_time_for_time_t("{DD}/{MM}/{Y} {hh}:{mm}") };
        strcpy_s(player.banned_date_time, std::size(player.banned_date_time), date_time_info.c_str());
        player.reason = reason;
        player.banned_by_user_name = main_app.get_username();
        main_app.get_connection_manager_for_messages().process_and_send_message("add-kick", format("{}\\{}\\{}\\{}\\{}\\{}", player.ip_address, player.guid_key, player.player_name, player.banned_date_time, reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        kick_player(pid, command);

      } else {
        const string re_msg{ format("^2{} ^3is not a valid pid number for the ^2!k ^3(^2!kick^3) command!\n", user_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } else {
      const string re_msg{ format(
        "^3Invalid command syntax for user command: ^2{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(user_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "!tb", "!tempban" }, [](const std::vector<std::string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;

    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command(
            user_cmd[0].c_str(), user_cmd[1])) {
        const int pid{ stoi(user_cmd[1]) };
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
        int number{};
        size_t temp_ban_duration{ 24 };
        string reason{
          "not specified"
        };
        if (user_cmd.size() > 2) {
          if (is_valid_decimal_whole_number(user_cmd[2], number)) {
            temp_ban_duration = number > 0 && number <= 9999 ? number : 24;
            if (user_cmd.size() > 3) {
              reason = remove_disallowed_character_in_string(str_join(cbegin(user_cmd) + 3, cend(user_cmd), " "));
              stl::helper::trim_in_place(reason);
            }
          } else {
            reason = remove_disallowed_character_in_string(str_join(cbegin(user_cmd) + 2, cend(user_cmd), " "));
            stl::helper::trim_in_place(reason);
          }
        }

        main_app.get_tinyrcon_dict()["{REASON}"] = reason;
        main_app.get_tinyrcon_dict()["{TEMPBAN_DURATION}"] = to_string(temp_ban_duration);
        const string message{ format("^3You have successfully executed ^5!tempban ^3on player ({}^3)\n", get_player_information(pid)) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        string command{ main_app.get_user_defined_tempban_message() };
        build_tiny_rcon_message(command);
        player_data &pd = get_player_data_for_pid(pid);
        pd.ban_duration_in_hours = temp_ban_duration;
        pd.reason = reason;
        tempban_player(pd, command);
        auto &current_user = main_app.get_user_for_name(main_app.get_username());
        current_user->no_of_tempbans++;
        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
        main_app.get_connection_manager_for_messages().process_and_send_message("add-tempban", format("{}\\{}\\{}\\{}\\{}\\{}\\{}", pd.ip_address, pd.player_name, pd.banned_date_time, pd.banned_start_time, pd.ban_duration_in_hours, pd.reason, pd.banned_by_user_name), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);

        // is_display_temporarily_banned_players_data_event.store(true);
      } else {
        const string re_msg{ format("^2{} ^3is not a valid pid number for the ^2!tb ^3(^2!tempban^3) command!\n", user_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }

    } else {
      const string re_msg{ format("^3Invalid command syntax for user command: ^2{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(user_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "!b", "!ban" }, [](const std::vector<std::string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;

    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command(
            user_cmd[0].c_str(), user_cmd[1])) {
        const int pid{ stoi(user_cmd[1]) };
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
        string reason{ remove_disallowed_character_in_string(user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified") };
        stl::helper::trim_in_place(reason);
        specify_reason_for_player_pid(pid, reason);
        main_app.get_tinyrcon_dict()["{REASON}"] = reason;
        const string message{ format("^3You have successfully executed ^5banclient ^3on player ({}^3)\n", get_player_information(pid)) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        string command{ main_app.get_user_defined_ban_message() };
        build_tiny_rcon_message(command);
        auto &current_user = main_app.get_user_for_name(main_app.get_username());
        current_user->no_of_guidbans++;
        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
        player_data &player{ get_player_data_for_pid(pid) };
        main_app.get_connection_manager_for_messages().process_and_send_message("add-guidban", format("{}\\{}\\{}\\{}\\{}\\{}", player.ip_address, player.guid_key, player.player_name, player.banned_date_time, reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        ban_player(pid, command);

      } else {
        const string re_msg{ format("^2{} ^3is not a valid pid number for the ^2!b ^3(^2!ban^3) command!\n", user_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } else {
      const string re_msg{ format("^3Invalid command syntax for user command: ^2{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(user_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "!gb", "!globalban", "!banip", "!addip" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;

    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command(
            "!gb", user_cmd[1])) {
        const auto &banned_ip_addresses =
          main_app.get_game_server().get_banned_ip_addresses_map();
        if (int pid{ -1 }; is_valid_decimal_whole_number(user_cmd[1], pid)) {
          auto &player = main_app.get_game_server().get_player_data(pid);
          if (pid == player.pid) {
            unsigned long ip_key{};
            if (check_ip_address_validity(player.ip_address, ip_key) && banned_ip_addresses.find(player.ip_address) == cend(banned_ip_addresses)) {
              main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
              main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
              string reason{ remove_disallowed_character_in_string(user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified") };
              stl::helper::trim_in_place(reason);
              specify_reason_for_player_pid(player.pid, reason);
              global_ban_player_ip_address(player);
              const string re_msg{ format("^2You have successfully banned IP address: ^1{}\n", player.ip_address) };
              print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              main_app.get_tinyrcon_dict()["{REASON}"] = reason;
              const string message{ format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0], get_player_information(pid)) };
              print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              string command{ main_app.get_user_defined_ipban_message() };
              build_tiny_rcon_message(command);
              kick_player(player.pid, command);
              auto &current_user = main_app.get_user_for_name(main_app.get_username());
              current_user->no_of_ipbans++;
              save_current_user_data_to_json_file(main_app.get_user_data_file_path());
              main_app.get_connection_manager_for_messages().process_and_send_message("add-ipban", format("{}\\{}\\{}\\{}\\{}\\{}", player.ip_address, player.guid_key, player.player_name, player.banned_date_time, reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
          }
        } else {
          const auto &ip_address = user_cmd[1];
          if (banned_ip_addresses.find(ip_address) == cend(banned_ip_addresses)) {
            bool is_ip_address_already_banned{};
            auto &players_data = main_app.get_game_server().get_players_data();
            for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
              auto &player = players_data[i];
              if (player.ip_address == ip_address) {
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
                string reason{ remove_disallowed_character_in_string(user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified") };
                stl::helper::trim_in_place(reason);
                specify_reason_for_player_pid(player.pid, reason);
                global_ban_player_ip_address(player);
                const string re_msg{ format("^2You have successfully banned IP address: ^1{}\n", user_cmd[1]) };
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
                main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                const string message{ format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0], get_player_information(player.pid)) };
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
                string command{ main_app.get_user_defined_ipban_message() };
                build_tiny_rcon_message(command);
                kick_player(player.pid, command);
                is_ip_address_already_banned = true;
                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                current_user->no_of_ipbans++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                main_app.get_connection_manager_for_messages().process_and_send_message("add-ipban", format("{}\\{}\\{}\\{}\\{}\\{}", player.ip_address, player.guid_key, player.player_name, player.banned_date_time, reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
              }
            }

            if (!is_ip_address_already_banned) {
              player_data player_offline{};
              player_offline.pid = -1;
              strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
              strcpy_s(player_offline.ip_address, std::size(player_offline.ip_address), ip_address.c_str());
              main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
              main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
              string reason{ remove_disallowed_character_in_string(user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified") };
              player_offline.reason = std::move(reason);
              global_ban_player_ip_address(player_offline);
              main_app.get_tinyrcon_dict()["{REASON}"] = player_offline.reason;
              string command{ main_app.get_user_defined_ipban_message() };
              build_tiny_rcon_message(command);
              rcon_say(command);
              const string re_msg{ format("^2You have successfully banned IP address: ^1{}\n", ip_address) };
              print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              const string message{ format(
                "^2You have successfully executed ^5{} ^2on player ({}^2)\n", user_cmd[0], get_player_information_for_player(player_offline)) };
              print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              auto &current_user = main_app.get_user_for_name(main_app.get_username());
              current_user->no_of_ipbans++;
              save_current_user_data_to_json_file(main_app.get_user_data_file_path());
              main_app.get_connection_manager_for_messages().process_and_send_message("add-ipban", format("{}\\0\\{}\\{}\\{}\\{}", player_offline.ip_address, player_offline.player_name, player_offline.banned_date_time, player_offline.reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
          }
        }
      }
    }
  });

  main_app.add_command_handler({ "!br", "!banrange" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;

    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command(
            "!br", user_cmd[1])) {
        if (int pid{ -1 }; is_valid_decimal_whole_number(user_cmd[1], pid)) {
          auto &player = main_app.get_game_server().get_player_data(pid);
          if (pid == player.pid) {
            unsigned long ip_key{};
            string ip_address_range{ player.ip_address };
            ip_address_range.replace(cbegin(ip_address_range) + ip_address_range.rfind('.') + 1, cend(ip_address_range), "*");
            if (check_ip_address_validity(player.ip_address, ip_key) && !main_app.get_game_server().get_banned_ip_address_ranges_map().contains(ip_address_range)) {
              main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
              main_app.get_tinyrcon_dict()["{IP_ADDRESS_RANGE}"] = ip_address_range;
              string reason{ remove_disallowed_character_in_string(user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified") };
              stl::helper::trim_in_place(reason);
              const string re_msg{ format("^2You have successfully banned IP address: ^1{}\n", player.ip_address) };
              print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              main_app.get_tinyrcon_dict()["{REASON}"] = reason;
              const string message{ format("^2You have successfully executed ^1{} ^2on player ({}^3)\n", user_cmd[0], get_player_information(pid)) };
              print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
              string command{ main_app.get_user_defined_ip_address_range_ban_message() };
              build_tiny_rcon_message(command);
              player.reason = reason;
              strcpy_s(player.ip_address, std::size(player.ip_address), ip_address_range.c_str());
              main_app.get_game_server().get_banned_ip_address_ranges_map().emplace(ip_address_range, player);
              main_app.get_game_server().get_banned_ip_address_ranges_vector().emplace_back(player);
              kick_player(pid, command);
              save_banned_ip_address_range_entries_to_file(main_app.get_ip_range_bans_file_path(), main_app.get_game_server().get_banned_ip_address_ranges_vector());
              auto &current_user = main_app.get_user_for_name(main_app.get_username());
              current_user->no_of_iprangebans++;
              save_current_user_data_to_json_file(main_app.get_user_data_file_path());
              main_app.get_connection_manager_for_messages().process_and_send_message("add-iprangeban", format("{}\\{}\\{}\\{}\\{}\\{}", ip_address_range, player.guid_key, player.player_name, player.banned_date_time, player.reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
          }
        } else {
          if (!main_app.get_game_server().get_banned_ip_address_ranges_map().contains(user_cmd[1])) {
            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
            main_app.get_tinyrcon_dict()["{IP_ADDRESS_RANGE}"] = user_cmd[1];
            string reason{ remove_disallowed_character_in_string(user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified") };
            stl::helper::trim_in_place(reason);
            const string re_msg{ format("^2You have successfully banned IP address range: ^1{}\n", user_cmd[1]) };
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            main_app.get_tinyrcon_dict()["{REASON}"] = reason;
            const string message{ format("^2You have successfully executed ^1{} ^2on IP address range: ^1{}\n", user_cmd[0], user_cmd[1]) };
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            player_data player{};
            strcpy_s(player.player_name, std::size(player.player_name), "John Doe");
            strcpy_s(player.ip_address, std::size(player.ip_address), user_cmd[1].c_str());
            const string datetimeinfo{ get_date_and_time_for_time_t("{DD}/{MM}/{Y} {hh}:{mm}") };
            strcpy_s(player.banned_date_time, std::size(player.banned_date_time), datetimeinfo.c_str());
            player.reason = reason;
            main_app.get_game_server().get_banned_ip_address_ranges_map().emplace(user_cmd[1], player);
            main_app.get_game_server().get_banned_ip_address_ranges_vector().emplace_back(player);
            string command{ main_app.get_user_defined_ip_address_range_ban_message() };
            build_tiny_rcon_message(command);
            rcon_say(command);
            save_banned_ip_address_range_entries_to_file(main_app.get_ip_range_bans_file_path(), main_app.get_game_server().get_banned_ip_address_ranges_vector());
            auto &current_user = main_app.get_user_for_name(main_app.get_username());
            current_user->no_of_iprangebans++;
            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
            main_app.get_connection_manager_for_messages().process_and_send_message("add-iprangeban", format("{}\\0\\{}\\{}\\{}\\{}", user_cmd[1], player.player_name, player.banned_date_time, player.reason, main_app.get_username()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
          }
        }
      }
    }
  });

  main_app.add_command_handler({ "!egb" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    // if (!main_app.get_game_server().get_is_automatic_city_kick_enabled()) {
    main_app.get_game_server().set_is_automatic_city_kick_enabled(true);
    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    const string message{ format("^2You have successfully executed ^5{}\n", user_cmd[0]) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
    string rcon_message{ main_app.get_user_defined_enable_city_ban_feature_msg() };
    build_tiny_rcon_message(rcon_message);
    rcon_say(rcon_message, true);
    // }
  });

  main_app.add_command_handler({ "!dgb" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    // if (main_app.get_game_server().get_is_automatic_city_kick_enabled()) {
    main_app.get_game_server().set_is_automatic_city_kick_enabled(false);
    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    const string message{ format("^2You have successfully executed ^5{}\n", user_cmd[0]) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
    string rcon_message{ main_app.get_user_defined_disable_city_ban_feature_msg() };
    build_tiny_rcon_message(rcon_message);
    rcon_say(rcon_message, true);
    // }
  });

  main_app.add_command_handler({ "!bancity" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      const string banned_city{ trim(str_join(user_cmd.cbegin() + 1, user_cmd.cend(), " ")) };
      main_app.get_game_server().get_banned_cities_set().insert(banned_city);
      save_banned_entries_to_file(main_app.get_banned_cities_list_file_path(), main_app.get_game_server().get_banned_cities_set());
      const string message{ format("^2You have successfully executed ^5{} ^2on city: ^1{}\n", user_cmd[0], banned_city) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
      main_app.get_tinyrcon_dict()["{CITY_NAME}"] = banned_city;
      string rcon_message{ main_app.get_user_defined_city_ban_msg() };
      build_tiny_rcon_message(rcon_message);
      rcon_say(rcon_message, true);
      auto &current_user = main_app.get_user_for_name(main_app.get_username());
      current_user->no_of_citybans++;
      save_current_user_data_to_json_file(main_app.get_user_data_file_path());
      main_app.get_connection_manager_for_messages().process_and_send_message("add-cityban", format("{}\\{}\\{}", banned_city, main_app.get_username(), get_current_time_stamp()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });
  main_app.add_command_handler({ "!unbancity" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      const string banned_city_to_unban{ trim(str_join(user_cmd.cbegin() + 1, user_cmd.cend(), " ")) };
      if (main_app.get_game_server().get_banned_cities_set().contains(banned_city_to_unban)) {
        main_app.get_game_server().get_banned_cities_set().erase(banned_city_to_unban);
        save_banned_entries_to_file(main_app.get_banned_cities_list_file_path(), main_app.get_game_server().get_banned_cities_set());
        const string message{ format("^2You have successfully executed ^5{} ^2on city: ^1\n", user_cmd[0], banned_city_to_unban) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{CITY_NAME}"] = banned_city_to_unban;
        string rcon_message{ main_app.get_user_defined_city_unban_msg() };
        build_tiny_rcon_message(rcon_message);
        rcon_say(rcon_message, true);
        main_app.get_connection_manager_for_messages().process_and_send_message("remove-cityban", format("{}\\{}\\{}", banned_city_to_unban, main_app.get_username(), get_current_time_stamp()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      }
    }
  });

  main_app.add_command_handler({ "!ecb" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (!main_app.get_game_server().get_is_automatic_country_kick_enabled()) {
      main_app.get_game_server().set_is_automatic_country_kick_enabled(true);
      write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
      const string message{ format("^2You have successfully executed ^5{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
      string rcon_message{ main_app.get_user_defined_enable_country_ban_feature_msg() };
      build_tiny_rcon_message(rcon_message);
      rcon_say(rcon_message, true);
    }
  });

  main_app.add_command_handler({ "!dcb" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (main_app.get_game_server().get_is_automatic_country_kick_enabled()) {
      main_app.get_game_server().set_is_automatic_country_kick_enabled(false);
      write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
      const string message{ format("^2You have successfully executed ^5{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
      string rcon_message{ main_app.get_user_defined_disable_country_ban_feature_msg() };
      build_tiny_rcon_message(rcon_message);
      rcon_say(rcon_message, true);
    }
  });

  main_app.add_command_handler({ "!bancountry" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      const string banned_country{ trim(str_join(user_cmd.cbegin() + 1, user_cmd.cend(), " ")) };
      main_app.get_game_server().get_banned_countries_set().insert(banned_country);
      save_banned_entries_to_file(main_app.get_banned_countries_list_file_path(), main_app.get_game_server().get_banned_countries_set());
      const string message{ format(
        "^2You have successfully executed ^5{} ^2on country: ^1{}\n", user_cmd[0], banned_country) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
      main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = banned_country;
      string rcon_message{ main_app.get_user_defined_country_ban_msg() };
      build_tiny_rcon_message(rcon_message);
      rcon_say(rcon_message, true);
      auto &current_user = main_app.get_user_for_name(main_app.get_username());
      current_user->no_of_countrybans++;
      save_current_user_data_to_json_file(main_app.get_user_data_file_path());
      main_app.get_connection_manager_for_messages().process_and_send_message("add-countryban", format("{}\\{}\\{}", banned_country, main_app.get_username(), get_current_time_stamp()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!unbancountry" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      const string banned_country_to_unban{ trim(str_join(user_cmd.cbegin() + 1, user_cmd.cend(), " ")) };
      if (main_app.get_game_server().get_banned_countries_set().contains(banned_country_to_unban)) {
        main_app.get_game_server().get_banned_countries_set().erase(banned_country_to_unban);
        save_banned_entries_to_file(main_app.get_banned_countries_list_file_path(), main_app.get_game_server().get_banned_countries_set());
        const string message{ format(
          "^2You have successfully executed ^5{} ^2on country: ^1{}\n", user_cmd[0], banned_country_to_unban) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = banned_country_to_unban;
        string rcon_message{ main_app.get_user_defined_country_unban_msg() };
        build_tiny_rcon_message(rcon_message);
        rcon_say(rcon_message, true);
        main_app.get_connection_manager_for_messages().process_and_send_message("remove-countryban", format("{}\\{}\\{}", banned_country_to_unban, main_app.get_username(), get_current_time_stamp()), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      }
    }
  });

  main_app.add_command_handler({ "s", "!s", "status", "!status" }, [](const vector<string> &) {
    initiate_sending_rcon_status_command_now();
  });

  main_app.add_command_handler({ "gs", "!gs", "getstatus", "!getstatus" }, [](const vector<string> &) {
    main_app.add_command_to_queue({ "getstatus" }, command_type::rcon, true);
  });

  main_app.add_command_handler({ "sort", "!sort" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() == 3) {
      sort_type new_sort_type{ sort_type::pid_asc };
      if (user_cmd[1] == "pid" && user_cmd[2] == "asc")
        new_sort_type = sort_type::pid_asc;
      else if (user_cmd[1] == "pid" && user_cmd[2] == "desc")
        new_sort_type = sort_type::pid_desc;
      else if (user_cmd[1] == "score" && user_cmd[2] == "asc")
        new_sort_type = sort_type::score_asc;
      else if (user_cmd[1] == "score" && user_cmd[2] == "desc")
        new_sort_type = sort_type::score_desc;
      else if (user_cmd[1] == "ping" && user_cmd[2] == "asc")
        new_sort_type = sort_type::ping_asc;
      else if (user_cmd[1] == "ping" && user_cmd[2] == "desc")
        new_sort_type = sort_type::ping_desc;
      else if (user_cmd[1] == "ip" && user_cmd[2] == "asc")
        new_sort_type = sort_type::ip_asc;
      else if (user_cmd[1] == "ip" && user_cmd[2] == "desc")
        new_sort_type = sort_type::ip_desc;
      else if (user_cmd[1] == "name" && user_cmd[2] == "asc")
        new_sort_type = sort_type::name_asc;
      else if (user_cmd[1] == "name" && user_cmd[2] == "desc")
        new_sort_type = sort_type::name_desc;
      else if (user_cmd[1] == "geo" && user_cmd[2] == "asc")
        new_sort_type = sort_type::geo_asc;
      else if (user_cmd[1] == "geo" && user_cmd[2] == "desc")
        new_sort_type = sort_type::geo_desc;
      else {
        const string re_msg{ format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        if (user_commands_help.contains(user_cmd[0])) {
          print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
        }
      }

      is_process_combobox_item_selection_event = false;
      process_sort_type_change_request(new_sort_type);

    } else {
      const string re_msg{ format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(user_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });
  main_app.add_command_handler({ "bans", "!bans" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() == 3U && user_cmd[1] == "clear" && user_cmd[2] == "all") {
      for (const auto &ip : main_app.get_game_server().get_banned_ip_addresses_map()) {
        string message;
        string ip_address{ ip.first };
        remove_permanently_banned_ip_address(ip_address, message, false);
      }
    }
    is_display_permanently_banned_players_data_event.store(true);
  });

  main_app.add_command_handler({ "ranges", "!ranges" }, [](const vector<string> &) {
    is_display_banned_ip_address_ranges_data_event.store(true);
  });

  main_app.add_command_handler({ "admins", "!admins" }, [](const vector<string> &) {
    is_display_admins_data_event.store(true);
  });

  main_app.add_command_handler({ "tempbans", "!tempbans" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() == 3U && user_cmd[1] == "clear" && user_cmd[2] == "all") {
      for (const auto &ip : main_app.get_game_server().get_temp_banned_ip_addresses_map()) {
        string message;
        remove_temp_banned_ip_address(ip.first, message, false);
      }
    }
    is_display_temporarily_banned_players_data_event.store(true);
  });

  main_app.add_command_handler({ "!bannedcities" }, [](const vector<string> &) {
    is_display_banned_cities_data_event.store(true);
  });

  main_app.add_command_handler({ "!bannedcountries" }, [](const vector<string> &) {
    is_display_banned_countries_data_event.store(true);
  });

  main_app.add_command_handler({ "!banned" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() == 2) {
      if (user_cmd[1] == "cities")
        is_display_banned_cities_data_event.store(true);
      else if (user_cmd[1] == "countries")
        is_display_banned_countries_data_event.store(true);
    }
  });

  main_app.add_command_handler({ "!ub", "!unban" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      unsigned long ip_key{};
      if (!check_ip_address_validity(user_cmd[1], ip_key) && !check_if_user_provided_argument_is_valid_for_specified_command("!unban", user_cmd[1])) {
        const string re_msg{ format("^5Provided IP address ^1{} ^5is not a valid IP address or there isn't a banned player entry whose ^1serial no. ^5is equal to provided number: ^1{}!\n", user_cmd[1], user_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      } else {
        string message;
        int no{ -1 };
        if (!is_valid_decimal_whole_number(user_cmd[1], no)) {
          auto [status, player]{ remove_temp_banned_ip_address(user_cmd[1], message, false, true) };
          if (status) {
            const string re_msg{ format("^2You have successfully removed previously ^1temporarily banned IP address: ^5{} ^2for player name: ^7{}\n", user_cmd[1], player.player_name) };
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            main_app.get_connection_manager_for_messages().process_and_send_message("remove-tempban", format("{}\\{}\\{}\\{}\\{}\\{}\\{}", player.ip_address, player.player_name, player.banned_date_time, player.banned_start_time, player.ban_duration_in_hours, player.reason, player.banned_by_user_name), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
          } else {
            const string re_msg{ format("^3Provided IP address (^1{}^3) hasn't been ^1temporarily banned ^3yet!\n", user_cmd[1]) };
            print_colored_text(app_handles.hwnd_re_messages_data,
              re_msg.c_str(),
              is_append_message_to_richedit_control::yes,
              is_log_message::yes,
              is_log_datetime::yes);
          }
        }

        string ip_address{ user_cmd[1] };
        auto [status, player]{ remove_permanently_banned_ip_address(ip_address, message, true) };
        if (status) {
          const string re_msg{ format(
            "^2You have successfully removed previously ^1permanently banned IP address: ^5{} ^2for player name: ^7{}\n", ip_address, player.player_name) };
          print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-ipban", format("{}\\{}\\{}\\{}\\{}\\{}", player.ip_address, player.guid_key, player.player_name, player.banned_date_time, player.reason, player.banned_by_user_name), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        } else {
          const string re_msg{ format("^3Provided IP address (^1{}^3) hasn't been ^1permanently banned ^3yet!\n", user_cmd[1]) };
          print_colored_text(app_handles.hwnd_re_messages_data,
            re_msg.c_str(),
            is_append_message_to_richedit_control::yes,
            is_log_message::yes,
            is_log_datetime::yes);
        }
      }
    } else {
      const string re_msg{ format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(user_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "!ubr", "!unbanrange" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      if (!check_if_user_provided_argument_is_valid_for_specified_command("!unbanrange", user_cmd[1])) {
        const string re_msg{ format("^5Provided IP address ^1{} ^5is not a valid IP address or there isn't a banned player entry whose ^1serial no. ^5is equal to provided number: ^1{}!\n", user_cmd[1], user_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      } else {
        string message;

        const string &ip_address_range{ user_cmd[1] };
        player_data pd{};
        strcpy_s(pd.ip_address, std::size(pd.ip_address), ip_address_range.c_str());

        auto status = remove_permanently_banned_ip_address_range(pd, main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());

        if (status) {
          const string re_msg{ format(
            "^2You have successfully removed previously banned ^1IP address range: ^5{}\n", ip_address_range) };
          print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          main_app.get_connection_manager_for_messages().process_and_send_message("remove-iprangeban", format("{}\\{}\\{}\\{}\\{}\\{}", ip_address_range, pd.guid_key, pd.player_name, pd.banned_date_time, pd.reason, pd.banned_by_user_name), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        } else {
          const string re_msg{ format("^3Provided IP address range (^1{}^3) hasn't been ^1banned ^3yet!\n", ip_address_range) };
          print_colored_text(app_handles.hwnd_re_messages_data,
            re_msg.c_str(),
            is_append_message_to_richedit_control::yes,
            is_log_message::yes,
            is_log_datetime::yes);
        }
      }
    } else {
      const string re_msg{ format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(user_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "!c", "!cp" }, [](const vector<string> &user_cmd) {
    const bool use_private_slot{ user_cmd[0] == "!cp" && !main_app.get_game_server().get_private_slot_password().empty() };
    const string user_input{ user_cmd[1] };
    smatch ip_port_match{};
    const string ip_port_server_address{
      (user_cmd.size() > 1 && regex_search(user_input, ip_port_match, ip_address_and_port_regex)) ? (ip_port_match[1].str() + ":"s + ip_port_match[2].str()) : (main_app.get_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_game_server().get_server_port()))
    };
    const size_t sep_pos{ ip_port_server_address.find(':') };
    const string ip_address{ ip_port_server_address.substr(0, sep_pos) };
    const uint16_t port_number{ static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1))) };
    const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(ip_address.c_str(), port_number, main_app.get_game_server().get_rcon_password().c_str());

    const game_name_t game_name{ result.second != game_name_t::unknown ? result.second : main_app.get_game_name() };

    connect_to_the_game_server(ip_port_server_address, game_name, use_private_slot, true);
  });

  main_app.add_command_handler({ "!m", "!map" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
      const string map_name{ stl::helper::trim(user_cmd[1]) };
      const string game_type{ user_cmd.size() >= 3 ? stl::helper::trim(user_cmd[2]) : "ctf" };
      load_map(map_name, game_type, true);
    }
  });

  main_app.add_command_handler({ "maps", "!maps" }, [](const vector<string> &) {
    display_all_available_maps();
  });
  main_app.add_command_handler({ "colors", "!colors" }, [](const vector<string> &) {
    change_colors();
  });

  main_app.add_command_handler({ "config", "!config" }, [](const vector<string> &user_cmd) {
    change_server_setting(user_cmd);
  });

  main_app.add_command_handler({ "!rt", "!refreshtime" }, [](const vector<string> &user_cmd) {
    if (int number{}; (user_cmd.size() == 2) && is_valid_decimal_whole_number(user_cmd[1], number)) {
      main_app.get_game_server().set_check_for_banned_players_time_period(number);
      KillTimer(app_handles.hwnd_main_window, ID_TIMER);
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, main_app.get_game_server().get_check_for_banned_players_time_period()));
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, 0, 0);
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETSTEP, 1, 0);
      SetTimer(app_handles.hwnd_main_window, ID_TIMER, 1000, nullptr);

      const string re_msg{ format("^2You have successfully changed the ^1time period\n for automatic checking for banned IP addresses ^2to ^5{}\n", user_cmd[1]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    }
  });

  main_app.add_command_handler({ "border", "!border" }, [](const vector<string> &user_cmd) {
    if ((user_cmd.size() == 2) && (user_cmd[1] == "on" || user_cmd[1] == "off")) {
      const bool new_setting{ user_cmd[1] == "on" ? true : false };
      const string re_msg{ format("^2You have successfully executed command: ^1{} {}\n", user_cmd[0], user_cmd[1]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (main_app.get_is_draw_border_lines() != new_setting) {
        main_app.set_is_draw_border_lines(new_setting);

        if (new_setting) {
          print_colored_text(app_handles.hwnd_re_messages_data, "^2You have successfully turned on the border lines\n around displayed ^3GUI controls^2.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, "^2You have successfully turned off the border lines\n around displayed ^3GUI controls^2.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }

        construct_tinyrcon_gui(app_handles.hwnd_main_window);

        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
      }
    }
  });

  main_app.add_command_handler({ "messages", "!messages" }, [](const vector<string> &user_cmd) {
    if ((user_cmd.size() == 2) && (user_cmd[1] == "on" || user_cmd[1] == "off")) {
      const bool new_setting{ user_cmd[1] == "on" ? false : true };
      const string re_msg{ format("^2You have successfully executed command: ^1{} {}\n", user_cmd[0], user_cmd[1]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (main_app.get_is_disable_automatic_kick_messages() != new_setting) {
        main_app.set_is_disable_automatic_kick_messages(new_setting);
        if (!new_setting) {
          print_colored_text(app_handles.hwnd_re_messages_data, "^2You have successfully turned on ^1automatic kick messages\n ^2for ^1temporarily ^2and ^1permanently banned players^2.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        } else {
          print_colored_text(app_handles.hwnd_re_messages_data, "^2You have successfully turned off ^1automatic kick messages\n ^2for ^1temporarily ^2and ^1permanently banned players^2.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
      }
    }
  });

  main_app.add_command_handler({ "say", "!say" }, [](const vector<string> &rcon_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (rcon_cmd.size() >= 2) {
      string command{ stl::helper::str_join(rcon_cmd, " ") };
      if (!command.empty() && '!' == command[0])
        command.erase(0, 1);
      string reply;
      main_app.get_connection_manager().send_and_receive_rcon_data(command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
    }
  });

  main_app.add_command_handler({ "tell", "!tell" }, [](const vector<string> &rcon_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;

    if (rcon_cmd.size() >= 3 && check_if_user_provided_pid_is_valid(trim(rcon_cmd[1]))) {
      const string player_pid{
        trim(rcon_cmd[1])
      };
      string command{ "tell "s + player_pid };
      for (size_t j{ 2 }; j < rcon_cmd.size(); ++j) {
        command.append(" ").append(rcon_cmd[j]);
      }

      string reply;
      main_app.get_connection_manager().send_and_receive_rcon_data(command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
    }
  });

  main_app.add_command_handler({ "t", "!t" }, [](const vector<string> &rcon_cmd) {
    if (rcon_cmd.size() >= 2) {
      const string message{ format("{}\\{}", main_app.get_username(), stl::helper::str_join(cbegin(rcon_cmd) + 1, cend(rcon_cmd), " ")) };
      main_app.add_message_to_queue(message_t{ "public-message", message, true });
    }
  });

  main_app.add_command_handler({ "y", "!y" }, [](const vector<string> &rcon_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (rcon_cmd.size() >= 3) {
      string send_to{ rcon_cmd[1] };
      remove_disallowed_character_in_string(send_to);
      const string private_message{ format("{}\\{}\\{}", main_app.get_username(), send_to, stl::helper::str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ")) };
      main_app.get_connection_manager_for_messages().process_and_send_message("private-message", private_message, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });


  main_app.add_command_handler({ "clientkick" }, [](const vector<string> &rcon_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {

      if (check_if_user_provided_argument_is_valid_for_specified_command(
            rcon_cmd[0].c_str(), rcon_cmd[1])) {
        const int pid{ stoi(rcon_cmd[1]) };
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
        string reason{ remove_disallowed_character_in_string(rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified") };
        stl::helper::trim_in_place(reason);
        specify_reason_for_player_pid(pid, reason);
        main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
        const string message{ format("^3You have successfully executed ^5clientkick ^3on player ({}^3)\n", get_player_information(pid)) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        string command{ main_app.get_user_defined_kick_message() };
        build_tiny_rcon_message(command);
        kick_player(pid, command);

      } else {
        const string re_msg2{ format("^2{} ^3is not a valid pid number for the ^2!k ^3(^2!kick^3) command!\n", rcon_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg2.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } else {
      const string re_msg{ format("^3Invalid command syntax for user command: ^2{}\n", rcon_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(rcon_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "tempbanclient" }, [](const vector<string> &rcon_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command(
            rcon_cmd[0].c_str(), rcon_cmd[1])) {
        const int pid{ stoi(rcon_cmd[1]) };
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
        int number{};
        size_t temp_ban_duration{ 24 };
        string reason{
          "not specified"
        };
        if (rcon_cmd.size() > 2) {
          if (is_valid_decimal_whole_number(rcon_cmd[2], number)) {
            temp_ban_duration = number > 0 && number <= 9999 ? number : 24;
            if (rcon_cmd.size() > 3) {
              reason = remove_disallowed_character_in_string(str_join(cbegin(rcon_cmd) + 3, cend(rcon_cmd), " "));
              stl::helper::trim_in_place(reason);
            }
          } else {
            reason = remove_disallowed_character_in_string(str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " "));
            stl::helper::trim_in_place(reason);
          }
        }
        main_app.get_tinyrcon_dict()["{REASON}"] = reason;
        main_app.get_tinyrcon_dict()["{TEMPBAN_DURATION}"] = to_string(temp_ban_duration);
        const string message{ "^3You have successfully executed ^5!tempban ^3on player ("s + get_player_information(pid) + "^3)\n"s };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        string command{ main_app.get_user_defined_tempban_message() };
        build_tiny_rcon_message(command);
        player_data &pd = get_player_data_for_pid(pid);
        pd.ban_duration_in_hours = temp_ban_duration;
        specify_reason_for_player_pid(pid, reason);
        pd.reason = std::move(reason);
        tempban_player(pd, command);
        // is_display_temporarily_banned_players_data_event.store(true);
      } else {
        const string re_msg{ format("^2{} ^3is not a valid pid number for the ^2!tb ^3(^2!tempban^3) command!\n", rcon_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } else {
      const string re_msg{ format("^3Invalid command syntax for user command: ^2{}\n", rcon_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(rcon_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "banclient" }, [](const vector<string> &rcon_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command(
            rcon_cmd[0].c_str(), rcon_cmd[1])) {
        const int pid{ stoi(rcon_cmd[1]) };
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
        string reason{ remove_disallowed_character_in_string(rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified") };
        trim_in_place(reason);
        specify_reason_for_player_pid(pid, reason);
        main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
        const string message{ format("^3You have successfully executed ^5banclient ^3on player ({}^3)\n", get_player_information(pid)) };
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        string command{ main_app.get_user_defined_ban_message() };
        build_tiny_rcon_message(command);
        ban_player(pid, command);
      } else {
        const string re_msg{ format("^2{} ^3is not a valid pid number for the ^2!b ^3(^2!ban^3) command!\n", rcon_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } else {
      const string re_msg{ format("^3Invalid command syntax for user command: ^2{}\n", rcon_cmd[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(rcon_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "kick", "onlykick", "tempbanuser", "banuser" }, [](const vector<string> &rcon_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
      bool is_player_found{};
      const bool remove_player_color_codes{ rcon_cmd[0] == "onlykick" };
      const auto &players_data = main_app.get_game_server().get_players_data();
      for (size_t i{}; i < main_app.get_game_server().get_number_of_players(); ++i) {
        const player_data &player{ players_data[i] };
        string player_name{ player.player_name };
        if (remove_player_color_codes)
          remove_all_color_codes(player_name);
        if (rcon_cmd[1] == player_name) {
          is_player_found = true;
          main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
          string reason{ remove_disallowed_character_in_string(rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified") };
          stl::helper::trim_in_place(reason);
          specify_reason_for_player_pid(player.pid, reason);
          main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
          const string message{ format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", rcon_cmd[0], get_player_information(player.pid)) };
          print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          string public_message{ (rcon_cmd[0] == "kick" || rcon_cmd[0] == "onlykick") ? main_app.get_user_defined_kick_message() : main_app.get_user_defined_tempban_message() };
          build_tiny_rcon_message(public_message);
          const string command{ rcon_cmd[0] + " "s + rcon_cmd[1] };
          const string r_command{ command };
          string reply;
          main_app.get_connection_manager().send_and_receive_rcon_data(
            r_command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), false);
          kick_player(player.pid, public_message);
          break;
        }
      }

      if (!is_player_found) {
        if (string player_name{ rcon_cmd[1] }; remove_player_color_codes) {
          remove_all_color_codes(player_name);
          ostringstream oss;
          oss << "^3Could not find player with specified name (^5" << player_name << "^3) to use with the ^2" << rcon_cmd[0] << " ^3command!\n";
          print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        } else {
          ostringstream oss;
          oss << "^3Could not find player with specified name (^5" << rcon_cmd[1] << "^3) to use with the ^2" << rcon_cmd[0] << " ^3command!\n";
          print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }
      }
    } else {
      const string re_msg{ "^3Invalid command syntax for user command: ^2"s + rcon_cmd[0] + "\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      if (user_commands_help.contains(rcon_cmd[0])) {
        print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      }
    }
  });

  main_app.add_command_handler({ "unknown-rcon" }, [](const vector<string> &rcon_cmd) {
    string reply;
    if (rcon_cmd.size() > 1) {
      const string command{ str_join(rcon_cmd, " ") };
      const string re_msg{ "^5Sending rcon command '"s + command + "' to the server.\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      const string r_command{ command };
      main_app.get_connection_manager().send_and_receive_rcon_data(
        r_command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), true);
    } else {
      const string re_msg{ "^5Sending rcon command '"s + rcon_cmd[0] + "' to the server.\n"s };
      print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      const string r_command{ rcon_cmd[0] };
      main_app.get_connection_manager().send_and_receive_rcon_data(
        r_command.c_str(), reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str(), true);
    }
    if (!reply.empty() && rcon_cmd[0] != "getstatus") {
      while (true) {
        const size_t pos =
          stl::helper::str_index_of(reply, "\377\377\377\377print\n");
        if (string::npos == pos)
          break;
        reply.erase(pos, 10);
      }
      print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
      print_colored_text(app_handles.hwnd_re_messages_data, reply.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::no);
    }
  });

  main_app.add_message_handler("request-login", [](const string &, const time_t, const string &, bool is_print_in_messages) {
    if (is_print_in_messages) {
      const string message{ format("^7{} ^7has sent a ^1login request ^7to ^5Tiny^6Rcon ^7server.", main_app.get_username()) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    }
  });

  main_app.add_message_handler("confirm-login", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {
      auto &current_user = main_app.get_user_for_name(parts[0]);
      current_user->no_of_logins++;
      current_user->last_login_time_stamp = get_current_time_stamp();
      current_user->is_logged_in = true;
      save_current_user_data_to_json_file(main_app.get_user_data_file_path());
      const string message{ format("^7{} ^2has ^1logged in ^2to ^5Tiny^6Rcon ^2server.\n^2Number of logins: ^1{}", current_user->user_name, current_user->no_of_logins) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    }
  });

  main_app.add_message_handler("public-message", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 2) {
      const string public_msg{ format("^7{} ^7sent all admins a ^1public message:\n^5Public message: ^7{}\n", parts[0], parts[1]) };
      print_colored_text(app_handles.hwnd_re_messages_data, public_msg.c_str());
    }
  });

  main_app.add_message_handler("private-message", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {
      const string private_msg{ format("^7{} ^7sent you a ^1private message.\n^5Private message: ^7{}\n", parts[0], parts[2]) };
      print_colored_text(app_handles.hwnd_re_messages_data, private_msg.c_str());
    }
  });

  main_app.add_message_handler("inform-login", [](const string &, const time_t, const string &message, bool) {
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
  });

  main_app.add_message_handler("inform-logout", [](const string &, const time_t, const string &message, bool) {
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
  });

  main_app.add_message_handler("request-logout", [](const string &, const time_t, const string &, bool) {
    const string message{ format("^7{} ^7has sent a ^1logout request ^7to ^5Tiny^6Rcon ^7server.", main_app.get_username()) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
  });

  main_app.add_message_handler("confirm-logout", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {
      auto &current_user = main_app.get_user_for_name(parts[0]);
      current_user->is_logged_in = false;
      current_user->last_logout_time_stamp = get_current_time_stamp();
      const string message{ format("^7{} ^3has ^1logged out ^3of ^5Tiny^6Rcon ^3server.\n^3Number of logins: ^1{}", current_user->user_name, current_user->no_of_logins) };
      print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    }
  });

  main_app.add_message_handler("upload-admindata", [](const string &, const time_t, const string &, bool) {
    /* const string message{ format("^3Sending ^7{}^3's user data to ^5Tiny^6Rcon ^3server.", main_app.get_username()) };
     print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());*/
  });

  main_app.add_message_handler("request-admindata", [](const string &, const time_t, const string &, bool) {
    /*const string message{ "^3Receiving all registered ^1admins' data ^3from ^5Tiny^6Rcon ^5server." };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());*/
  });

  main_app.add_message_handler("receive-admindata", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 17) {
      auto &cu = main_app.get_current_user();
      auto &u = main_app.get_user_for_name(parts[0]);
      if (cu.get() != u.get()) {
        u->user_name = std::move(parts[0]);
        u->is_admin = parts[1] == "true";
        u->is_logged_in = parts[2] == "true";
        u->is_online = parts[3] == "true";

        unsigned long guid{};
        if (check_ip_address_validity(parts[4], guid)) {
          player_data admin{};
          convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), parts[4], admin);
          u->ip_address = std::move(parts[4]);
          u->country_code = admin.country_code;
          u->geo_information = format("{}, {}", admin.country_name, admin.city);
        } else {
          if (parts[5] != "n/a") {
            u->geo_information = std::move(parts[5]);
          } else {
            u->ip_address = "n/a";
            u->geo_information = "n/a";
            u->country_code = "xy";
          }
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
      }
    }
  });

  main_app.add_message_handler("upload-ipbans", [](const string &, const time_t, const string &data, bool) {
    const string message{ format("^5Sending ^7{}^5's ^1IP bans ^5to Tiny^6Rcon ^5server.", main_app.get_username()) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string banned_ips_file_path{ main_app.get_banned_ip_addresses_file_path() };
    upload_file_to_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(), main_app.get_tiny_rcon_ftp_server_username().c_str(), main_app.get_tiny_rcon_ftp_server_password().c_str(), banned_ips_file_path.c_str(), data.c_str());
  });

  main_app.add_message_handler("receive-ipbans", [](const string &, const time_t, const string &data, bool) {
    const string message{ "^2Received updated ^1IP bans ^2from ^5Tiny^6Rcon ^5server." };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string banned_ips_file_path{ main_app.get_banned_ip_addresses_file_path() };
    download_file_from_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(), main_app.get_tiny_rcon_ftp_server_username().c_str(), main_app.get_tiny_rcon_ftp_server_password().c_str(), banned_ips_file_path.c_str(), data.c_str());
    parse_banned_ip_addresses_file(banned_ips_file_path.c_str(), main_app.get_game_server().get_banned_ip_addresses_vector(), main_app.get_game_server().get_banned_ip_addresses_map());
  });

  main_app.add_message_handler("upload-iprangebans", [](const string &, const time_t, const string &data, bool) {
    const string message{ format("^5Sending ^7{}^5's ^1IP address range bans ^5to Tiny^6Rcon ^5server.", main_app.get_username()) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string banned_ips_file_path{ main_app.get_ip_range_bans_file_path() };
    upload_file_to_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(), main_app.get_tiny_rcon_ftp_server_username().c_str(), main_app.get_tiny_rcon_ftp_server_password().c_str(), banned_ips_file_path.c_str(), data.c_str());
  });

  main_app.add_message_handler("receive-iprangebans", [](const string &, const time_t, const string &data, bool) {
    const string message{ "^2Received updated ^1IP address range bans ^2from ^5Tiny^6Rcon ^5server." };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string banned_ips_file_path{ main_app.get_ip_range_bans_file_path() };
    download_file_from_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(), main_app.get_tiny_rcon_ftp_server_username().c_str(), main_app.get_tiny_rcon_ftp_server_password().c_str(), banned_ips_file_path.c_str(), data.c_str());
    parse_banned_ip_address_ranges_file(banned_ips_file_path.c_str(), main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
  });

  main_app.add_message_handler("upload-citybans", [](const string &, const time_t, const string &data, bool) {
    const string message{ format("^5Sending ^7{}^5's ^1banned cities ^5to Tiny^6Rcon ^5server.", main_app.get_username()) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string banned_ips_file_path{ main_app.get_banned_cities_list_file_path() };
    upload_file_to_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(), main_app.get_tiny_rcon_ftp_server_username().c_str(), main_app.get_tiny_rcon_ftp_server_password().c_str(), banned_ips_file_path.c_str(), data.c_str());
  });

  main_app.add_message_handler("receive-citybans", [](const string &, const time_t, const string &data, bool) {
    const string message{ "^2Received updated ^1banned cities ^2from ^5Tiny^6Rcon ^5server." };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string banned_ips_file_path{ main_app.get_banned_cities_list_file_path() };
    download_file_from_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(), main_app.get_tiny_rcon_ftp_server_username().c_str(), main_app.get_tiny_rcon_ftp_server_password().c_str(), banned_ips_file_path.c_str(), data.c_str());
    parse_banned_cities_file(banned_ips_file_path.c_str(), main_app.get_game_server().get_banned_cities_set());
  });


  main_app.add_message_handler("upload-countrybans", [](const string &, const time_t, const string &data, bool) {
    const string message{ format("^5Sending ^7{}^5's ^1banned countries ^5to Tiny^6Rcon ^5server.", main_app.get_username()) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string banned_ips_file_path{ main_app.get_banned_countries_list_file_path() };
    upload_file_to_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(), main_app.get_tiny_rcon_ftp_server_username().c_str(), main_app.get_tiny_rcon_ftp_server_password().c_str(), banned_ips_file_path.c_str(), data.c_str());
  });

  main_app.add_message_handler("receive-countrybans", [](const string &, const time_t, const string &data, bool) {
    const string message{ "^2Received updated ^1banned countries ^2from ^5Tiny^6Rcon ^5server." };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    const string banned_ips_file_path{ main_app.get_banned_countries_list_file_path() };
    download_file_from_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(), main_app.get_tiny_rcon_ftp_server_username().c_str(), main_app.get_tiny_rcon_ftp_server_password().c_str(), banned_ips_file_path.c_str(), data.c_str());
    parse_banned_countries_file(banned_ips_file_path.c_str(), main_app.get_game_server().get_banned_countries_set());
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
      warned_player.reason = remove_disallowed_character_in_string(parts[4]);
      warned_player.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        warned_player.ip_address,
        warned_player);

      const string msg{ format("^7{} ^3has ^1warned ^3player ^7{} ^3[^5IP address ^1{} ^3| ^5GUID: ^1{}\n^5Date/time of warning: ^1{} ^3| ^5Reason of warning: ^1{} ^3| ^5Warned by: ^7{}\n", user, warned_player.player_name, warned_player.ip_address, warned_player.guid_key, warned_player.banned_date_time, warned_player.reason, user) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
    }
  });

  main_app.add_message_handler("inform-message", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }
    if (parts.size() >= 2) {
      const string received_from_message{ format("^2Received message from ^7{}:\n", parts[0]) };
      print_colored_text(app_handles.hwnd_re_messages_data, received_from_message.c_str());
      for (size_t i{ 1 }; i < parts.size(); ++i) {
        if (!parts[i].empty()) {
          print_colored_text(app_handles.hwnd_re_messages_data, parts[i].c_str());
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
      kicked_player.reason = remove_disallowed_character_in_string(parts[4]);
      kicked_player.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        kicked_player.ip_address,
        kicked_player);

      const string msg{ format("^7{} ^3has ^1kicked ^3player ^7{} ^3[^5IP address ^1{} ^3| ^5GUID: ^1{}\n^5Date/time of kick: ^1{} ^3| ^5Reason of kick: ^1{} ^3| ^5Kicked by: ^7{}\n", user, kicked_player.player_name, kicked_player.ip_address, kicked_player.guid_key, kicked_player.banned_date_time, kicked_player.reason, user) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
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
      temp_banned_player_data.reason = remove_disallowed_character_in_string(parts[5]);
      temp_banned_player_data.banned_by_user_name = parts.size() >= 7 ? std::move(parts[6]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        temp_banned_player_data.ip_address,
        temp_banned_player_data);

      const string msg{ format("^7{} ^5has temporarily banned ^1IP address {}\n ^5for ^3player name: ^7{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Ban duration: ^1{} hours ^5| ^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, temp_banned_player_data.ip_address, temp_banned_player_data.player_name, temp_banned_player_data.guid_key, temp_banned_player_data.banned_date_time, temp_banned_player_data.ban_duration_in_hours, temp_banned_player_data.reason, temp_banned_player_data.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_temporarily_banned_ip_address(temp_banned_player_data, main_app.get_game_server().get_temp_banned_players_data(), main_app.get_game_server().get_temp_banned_ip_addresses_map());
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
      temp_banned_player_data.reason = remove_disallowed_character_in_string(parts[5]);
      temp_banned_player_data.banned_by_user_name = parts.size() >= 7 ? std::move(parts[6]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        temp_banned_player_data.ip_address,
        temp_banned_player_data);

      const string msg{ format("^7{} ^5has removed temporarily banned ^1IP address {}\n ^5for ^3player name: ^7{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Ban duration: ^1{} hours ^5| ^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, temp_banned_player_data.ip_address, temp_banned_player_data.player_name, temp_banned_player_data.guid_key, temp_banned_player_data.banned_date_time, temp_banned_player_data.ban_duration_in_hours, temp_banned_player_data.reason, temp_banned_player_data.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      string message_about_removal;
      remove_temp_banned_ip_address(temp_banned_player_data.ip_address, message_about_removal, false, false);
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
      pd.reason = remove_disallowed_character_in_string(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has banned the ^1GUID key ^5of player:\n^3Name: ^7{} ^5| ^3IP address: ^1{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, pd.player_name, pd.ip_address, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
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
      pd.reason = remove_disallowed_character_in_string(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has banned the ^1IP address ^5of player:\n^3Name: ^7{} ^5| ^3IP address: ^1{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, pd.player_name, pd.ip_address, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_ip_address(pd, main_app.get_game_server().get_banned_ip_addresses_vector(), main_app.get_game_server().get_banned_ip_addresses_map());
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
      pd.reason = remove_disallowed_character_in_string(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has removed ^1banned IP address {}\n ^5for ^3player name: ^7{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n", user, pd.ip_address, pd.player_name, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      string ip_address{ pd.ip_address };
      string message_about_removal;
      remove_permanently_banned_ip_address(ip_address, message_about_removal, false);
    }
  });

  main_app.add_message_handler("add-iprangeban", [](const string &, const time_t, const string &data, bool) {
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
      pd.reason = remove_disallowed_character_in_string(parts[4]);
      pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has banned ^1IP address range:\n^5[^3Player name: ^7{} ^5| ^3IP address range: ^1{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}^5]\n", pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_ip_address_range(pd, main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());

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
      pd.reason = remove_disallowed_character_in_string(parts[4]);
      pd.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : "^1Admin";
      convert_guid_key_to_country_name(
        main_app.get_connection_manager().get_geoip_data(),
        pd.ip_address,
        pd);

      const string msg{ format("^7{} ^5has removed previously ^1banned IP address range:\n^5[^3Player name: ^7{} ^5| ^3IP range: ^1{} ^5| ^3GUID: ^1{} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}^5]\n", user, pd.player_name, pd.ip_address, pd.guid_key, pd.banned_date_time, pd.reason, pd.banned_by_user_name) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      string ip_address{ pd.ip_address };
      string message_about_removal;
      remove_permanently_banned_ip_address_range(pd, main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
    }
  });

  main_app.add_message_handler("add-cityban", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string city{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };

      const string msg{ format("^7{} ^3has banned city ^1{} ^3at ^1{}\n", admin, city, get_date_and_time_for_time_t("{DD}/{MM}/{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_city(city, main_app.get_game_server().get_banned_cities_set());
    }
  });

  main_app.add_message_handler("remove-cityban", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string city{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };

      const string msg{ format("^7{} ^2has removed banned city ^1{} ^2at ^1{}\n", admin, city, get_date_and_time_for_time_t("{DD}/{MM}/{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      remove_permanently_banned_city(city, main_app.get_game_server().get_banned_cities_set());
    }
  });

  main_app.add_message_handler("add-countryban", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string country{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };

      const string msg{ format("^7{} ^3has banned country ^1{} ^3at ^1{}\n", admin, country, get_date_and_time_for_time_t("{DD}/{MM}/{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      add_permanently_banned_country(country, main_app.get_game_server().get_banned_countries_set());
    }
  });

  main_app.add_message_handler("remove-countryban", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) {
      stl::helper::trim_in_place(part);
    }

    if (parts.size() >= 3U) {
      const string city{ parts[0] };
      const string admin{ parts[1] };
      const time_t timestamp_of_ban{ stoll(parts[2]) };

      const string msg{ format("^7{} ^2has removed banned country ^1{} ^2at ^1{}\n", admin, city, get_date_and_time_for_time_t("{DD}/{MM}/{Y} {hh}:{mm}", timestamp_of_ban)) };
      print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
      remove_permanently_banned_country(city, main_app.get_game_server().get_banned_countries_set());
    }
  });

  main_app.set_command_line_info(user_help_message);

  const string program_title{ main_app.get_program_title() + " | "s + main_app.get_game_server_name() + " | "s + "version: "s + program_version };
  SetWindowText(app_handles.hwnd_main_window, program_title.c_str());

  CenterWindow(app_handles.hwnd_main_window);

  SetFocus(app_handles.hwnd_e_user_input);
  PostMessage(app_handles.hwnd_progress_bar, PBM_SETMARQUEE, (WPARAM)TRUE, (LPARAM)5);

  std::thread task_thread{
    [&]() {
      IsGUIThread(TRUE);
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started parsing ^1tinyrcon.json ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished parsing ^1tinyrcon.json ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      {
        auto_update_manager au{};
        // main_app.set_current_working_directory(au.get_self_current_working_directory());
      }
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started importing serialized binary geological data from\n ^1'plugins/geoIP/geo.dat' ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      // const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\IP2LOCATION-LITE-DB3.CSV" };
      // parse_geodata_lite_csv_file(geo_dat_file_path.c_str());
      const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\geo.dat" };
      import_geoip_data(main_app.get_connection_manager().get_geoip_data(), geo_dat_file_path.c_str());

      unsigned long guid{};
      const auto &me = main_app.get_user_for_name(main_app.get_username());
      if (check_ip_address_validity(me->ip_address, guid)) {
        player_data pl{};
        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), me->ip_address, pl);
        me->country_code = pl.country_code;
        me->geo_information = format("{}, {}", pl.country_name, pl.city);
      } else {
        me->ip_address = "n/a";
        me->geo_information = "n/a";
        me->country_code = "xy";
      }

      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished importing serialized binary geological data from\n ^1'plugins/geoIP/geo.dat' ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_tempbans_data_file(main_app.get_tempbans_file_path(), main_app.get_game_server().get_temp_banned_players_data(), main_app.get_game_server().get_temp_banned_ip_addresses_map());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1temporarily banned IP addresses ^2from ^5data\\tempbans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_ip_addresses_file(main_app.get_banned_ip_addresses_file_path(), main_app.get_game_server().get_banned_ip_addresses_vector(), main_app.get_game_server().get_banned_ip_addresses_map());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1permanently banned IP addresses ^2from ^5data\\bans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_ip_address_ranges_file(main_app.get_ip_range_bans_file_path(), main_app.get_game_server().get_banned_ip_address_ranges_vector(), main_app.get_game_server().get_banned_ip_address_ranges_map());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned IP address ranges ^2from ^5data\\ip_range_bans.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_cities_file(main_app.get_banned_cities_list_file_path(), main_app.get_game_server().get_banned_cities_set());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned cities ^2from ^5data\\banned_cities.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      parse_banned_countries_file(main_app.get_banned_countries_list_file_path(), main_app.get_game_server().get_banned_countries_set());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Processed ^1banned countries ^2from ^5data\\banned_countries.txt ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      initialize_and_verify_server_connection_settings();
      main_app.get_current_user()->is_admin = main_app.get_game_server().get_is_connection_settings_valid();

      display_tempbanned_players_remaining_time_period();

      is_main_window_constructed = true;

      while (true) {
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

        string rcon_reply;

        if (is_refresh_players_data_event.load()) {
          if (main_app.get_game_server().get_is_connection_settings_valid()) {
            main_app.get_connection_manager().send_and_receive_rcon_data("g_gametype", rcon_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
            main_app.get_connection_manager().send_and_receive_rcon_data("status", rcon_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
            prepare_players_data_for_display(false);
          } else {
            main_app.get_connection_manager().send_and_receive_rcon_data("getstatus", rcon_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
            prepare_players_data_for_display_of_getstatus_response(false);
          }

          is_refresh_players_data_event.store(false);
          is_refreshed_players_data_ready_event.store(true);
        }
      }
    }
  };

  std::jthread messaging_thread{
    [&]() {
      const auto &tiny_rcon_server_ip = main_app.get_tiny_rcon_server_ip_address();
      const uint_least16_t tiny_rcon_server_port = static_cast<uint_least16_t>(main_app.get_tiny_rcon_server_port());

      while (true) {
        /*{
          unique_lock ul{ mu };
          exit_flag.wait_for(ul, 20ms, [&]() {
            return is_terminate_program.load();
          });
        }

        if (is_terminate_program.load()) {
          break;
        }*/

        while (!main_app.is_message_queue_empty()) {
          message_t message{ main_app.get_message_from_queue() };
          main_app.get_connection_manager_for_messages().process_and_send_message(message.command, message.data, message.is_show_in_messages, tiny_rcon_server_ip, tiny_rcon_server_port, true);
        }

        main_app.get_connection_manager_for_messages().wait_for_and_process_response_message(tiny_rcon_server_ip, tiny_rcon_server_port);
      }
    }
  };

  messaging_thread.detach();

  MSG win32_msg{};
  while (GetMessage(&win32_msg, nullptr, 0, 0) > 0) {
    if (TranslateAccelerator(app_handles.hwnd_main_window, hAccel, &win32_msg) != 0) {
      TranslateMessage(&win32_msg);
      DispatchMessage(&win32_msg);
      SetWindowText(app_handles.hwnd_e_user_input, "");
    } else if (IsDialogMessage(app_handles.hwnd_main_window, &win32_msg) == 0) {
      TranslateMessage(&win32_msg);
      if (win32_msg.message == WM_KEYDOWN) {
        process_key_down_message(win32_msg);
      }
      DispatchMessage(&win32_msg);
    } else if (win32_msg.message == WM_KEYDOWN) {
      process_key_down_message(win32_msg);
    } else if (win32_msg.message == WM_RBUTTONDOWN && app_handles.hwnd_players_grid == GetFocus()) {
      const int x{ GET_X_LPARAM(win32_msg.lParam) };
      const int y{ GET_Y_LPARAM(win32_msg.lParam) };
      display_context_menu_over_grid(x, y, selected_row);
    }

    if (is_main_window_constructed && !is_tinyrcon_initialized) {

      ifstream temp_messages_file{ "log\\temporary_message.log" };
      if (temp_messages_file) {
        for (string line; getline(temp_messages_file, line);) {
          print_colored_text(app_handles.hwnd_re_messages_data, line.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }
        temp_messages_file.close();
      }

      string rcon_task_reply;
      is_tinyrcon_initialized = true;
      if (main_app.get_game_server().get_is_connection_settings_valid()) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Sending rcon command ^1'g_gametype' ^2to the game server.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_connection_manager().send_and_receive_rcon_data("g_gametype", rcon_task_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Sending rcon command ^1'status' ^2to the game server.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_connection_manager().send_and_receive_rcon_data("status", rcon_task_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
        prepare_players_data_for_display();

      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Sending rcon command ^1'getstatus' ^2to the game server.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_connection_manager().send_and_receive_rcon_data("getstatus", rcon_task_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
        prepare_players_data_for_display_of_getstatus_response();
      }

      display_players_data_in_players_grid(app_handles.hwnd_players_grid);

      // auto &me = main_app.get_current_user();

      if (main_app.get_game_server().get_is_connection_settings_valid()) {

        main_app.add_message_to_queue(message_t("request-login", format("{}\\{}\\{}", me->user_name, me->ip_address, get_current_time_stamp()), true));

        if (main_app.get_is_ftp_server_online()) {

          main_app.add_message_to_queue(message_t{ "upload-ipbans", format("ipbans_{}_{}.txt", main_app.get_random_number(), get_current_time_stamp()), true });

          main_app.add_message_to_queue(message_t{ "upload-iprangebans", format("ip_range_bans_{}_{}.txt", main_app.get_random_number(), get_current_time_stamp()), true });

          main_app.add_message_to_queue(message_t{ "upload-citybans", format("banned_cities_{}_{}.txt", main_app.get_random_number(), get_current_time_stamp()), true });

          main_app.add_message_to_queue(message_t{ "upload-countrybans", format("banned_countries_{}_{}.txt", main_app.get_random_number(), get_current_time_stamp()), true });
        }

        const string admin_data{ format(R"({}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{})", me->user_name, (me->is_admin ? "true" : "false"), (me->is_logged_in ? "true" : "false"), (me->is_online ? "true" : "false"), me->ip_address, me->geo_information, me->last_login_time_stamp, me->last_logout_time_stamp, me->no_of_logins, me->no_of_warnings, me->no_of_kicks, me->no_of_tempbans, me->no_of_guidbans, me->no_of_ipbans, me->no_of_iprangebans, me->no_of_citybans, me->no_of_countrybans) };

        main_app.add_message_to_queue(message_t("upload-admindata", admin_data, true));
        main_app.add_message_to_queue(message_t{ "request-admindata", format("{}\\{}\\{}", me->user_name, me->ip_address, get_current_time_stamp()), true });
      }

      PostMessage(app_handles.hwnd_progress_bar, PBM_SETMARQUEE, (WPARAM)FALSE, (LPARAM)0);
      auto progress_bar_style = GetWindowStyle(app_handles.hwnd_progress_bar);
      progress_bar_style = progress_bar_style & ~PBS_MARQUEE;
      progress_bar_style = progress_bar_style | PBS_SMOOTH;
      SetWindowLong(app_handles.hwnd_progress_bar, -16, progress_bar_style);
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, main_app.get_game_server().get_check_for_banned_players_time_period()));
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETSTEP, 1, 0);
      SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, 0, 0);
      SetTimer(app_handles.hwnd_main_window, ID_TIMER, 1000, nullptr);
    }
  }
  // const auto &me = main_app.get_current_user();
  const auto current_ts{ get_current_time_stamp() };
  main_app.get_connection_manager_for_messages().process_and_send_message("request-logout", format("{}\\{}\\{}", me->user_name, me->ip_address, current_ts), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port());
  me->is_logged_in = false;
  me->last_logout_time_stamp = current_ts;
  save_current_user_data_to_json_file(main_app.get_user_data_file_path());

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
  // const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\geo.dat" };
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
  UnregisterClass(wcex_configuration_dialog.lpszClassName, app_handles.hInstance);

  return static_cast<int>(win32_msg.wParam);
}

ATOM register_window_classes(HINSTANCE hInstance)
{
  wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONCLIENT));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = CreateSolidBrush(color::black);
  wcex.lpszClassName = "tinyrconclient";
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
  wcex_confirmation_dialog.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONCLIENT));
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

  wcex_configuration_dialog = {};

  wcex_configuration_dialog.cbSize = sizeof(WNDCLASSEX);
  wcex_configuration_dialog.style = CS_HREDRAW | CS_VREDRAW;
  wcex_configuration_dialog.lpfnWndProc = WndProcForConfigurationDialog;
  wcex_configuration_dialog.hInstance = hInstance;
  wcex_configuration_dialog.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONCLIENT));
  wcex_configuration_dialog.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex_configuration_dialog.hbrBackground = CreateSolidBrush(color::black);
  wcex_configuration_dialog.lpszClassName = "ConfigurationDialog";
  wcex_configuration_dialog.hIconSm = LoadIcon(wcex_configuration_dialog.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  status = RegisterClassEx(&wcex_configuration_dialog);
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
  HBRUSH orig_textEditBrush{};
  HBRUSH comboBrush1{}, comboBrush2{}, comboBrush3{}, comboBrush4{}, comboBrush5{};
  HBRUSH defaultbrush{};
  HBRUSH hotbrush{};
  HBRUSH selectbrush{};
  HMENU hPopupMenu;
  PAINTSTRUCT ps;
  static int counter{};

  static HPEN red_pen{};
  static HPEN light_blue_pen{};

  switch (message) {

  case WM_CONTEXTMENU: {
    if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_re_messages_data) {
      hPopupMenu = CreatePopupMenu();
      InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, ID_VIEWTEMPBANSBUTTON, "View temporarily banned IP addresses");
      InsertMenu(hPopupMenu, 1, MF_BYPOSITION | MF_STRING, ID_VIEWIPBANSBUTTON, "View permanently banned IP addresses");
      InsertMenu(hPopupMenu, 2, MF_BYPOSITION | MF_STRING, ID_VIEWIPRANGEBANSBUTTON, "View banned IP address ranges");
      InsertMenu(hPopupMenu, 3, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCITIES, "View banned cities");
      InsertMenu(hPopupMenu, 4, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCOUNTRIES, "View banned countries");
      InsertMenu(hPopupMenu, 5, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, 6, MF_BYPOSITION | MF_STRING, ID_VIEWADMINSDATA, "View &admins' data");
      InsertMenu(hPopupMenu, 7, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, 8, MF_BYPOSITION | MF_STRING | MF_ENABLED, IDC_COPY, "&Copy");
      InsertMenu(hPopupMenu, 9, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "Clear messages");
      InsertMenu(hPopupMenu, 10, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, 11, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    } else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_e_user_input) {
      hPopupMenu = CreatePopupMenu();
      InsertMenu(hPopupMenu, 0, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDC_PASTE, "&Paste");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    }
  } break;

  case WM_TIMER: {

    ++atomic_counter;
    if (atomic_counter.load() > time_period)
      atomic_counter.store(0);

    SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, (WPARAM)atomic_counter.load(), 0);
    ++counter;
    if (counter % 30 == 0) {
      counter = 0;
      if (main_app.get_game_server().get_is_connection_settings_valid()) {
        auto &me = main_app.get_user_for_name(main_app.get_username());
        const string admin_data{ format(R"({}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{}\{})", me->user_name, (me->is_admin ? "true" : "false"), (me->is_logged_in ? "true" : "false"), (me->is_online ? "true" : "false"), me->ip_address, me->geo_information, me->last_login_time_stamp, me->last_logout_time_stamp, me->no_of_logins, me->no_of_warnings, me->no_of_kicks, me->no_of_tempbans, me->no_of_guidbans, me->no_of_ipbans, me->no_of_iprangebans, me->no_of_citybans, me->no_of_countrybans) };
        main_app.add_message_to_queue(message_t("upload-admindata", admin_data, true));
        main_app.add_message_to_queue(message_t{ "request-admindata", format("{}\\{}\\{}", me->user_name, me->ip_address, get_current_time_stamp()), true });
        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
      }
    }

    if (time_period == atomic_counter.load()) {
      is_refresh_players_data_event.store(true);
    }

    if (is_refreshed_players_data_ready_event.load()) {
      is_refreshed_players_data_ready_event.store(false);
      display_players_data_in_players_grid(app_handles.hwnd_players_grid);
    }

    HDC hdc = BeginPaint(hWnd, &ps);

    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, color::black);
    SetTextColor(hdc, color::red);

    RECT bounding_rectangle = {
      screen_width - 270,
      screen_height - 105,
      screen_width - 5,
      screen_height - 85,
    };

    InvalidateRect(hWnd, &bounding_rectangle, TRUE);

    bounding_rectangle = {
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
    } else if (is_display_banned_ip_address_ranges_data_event.load()) {
      display_banned_ip_address_ranges();
      is_display_banned_ip_address_ranges_data_event.store(false);
    } else if (is_display_banned_cities_data_event.load()) {
      display_banned_cities(main_app.get_game_server().get_banned_cities_set());
      is_display_banned_cities_data_event.store(false);
    } else if (is_display_banned_countries_data_event.load()) {
      display_banned_countries(main_app.get_game_server().get_banned_countries_set());
      is_display_banned_countries_data_event.store(false);
    } else if (is_display_admins_data_event.load()) {
      display_admins_data();
      is_display_admins_data_event.store(false);
    }

  }

  break;

  case WM_NOTIFY: {
    if (wParam == 501) {
      auto nmhdr = (NMGRID *)lParam;
      const int row_index = nmhdr->row;
      const int col_index = nmhdr->col;

      if (row_index < (int)max_players_grid_rows) {
        selected_row = row_index;
      }

      if (col_index >= 0 && col_index < 6) {
        selected_col = col_index;
      }
    }

    LPNMHDR some_item = (LPNMHDR)lParam;

    if ((some_item->idFrom == ID_WARNBUTTON || some_item->idFrom == ID_KICKBUTTON || some_item->idFrom == ID_TEMPBANBUTTON || some_item->idFrom == ID_IPBANBUTTON || some_item->idFrom == ID_VIEWTEMPBANSBUTTON || some_item->idFrom == ID_VIEWIPBANSBUTTON || some_item->idFrom == ID_VIEWADMINSDATA || some_item->idFrom == ID_CONNECTBUTTON || some_item->idFrom == ID_CONNECTPRIVATESLOTBUTTON || some_item->idFrom == ID_SAY_BUTTON || some_item->idFrom == ID_TELL_BUTTON || some_item->idFrom == ID_QUITBUTTON || some_item->idFrom == ID_LOADBUTTON || some_item->idFrom == ID_BUTTON_CONFIGURE_SERVER_SETTINGS || some_item->idFrom == ID_CLEARMESSAGESCREENBUTTON || some_item->idFrom == ID_REFRESHDATABUTTON)
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
      // if (show_user_confirmation_dialog("^3Do you really want to exit?\n", "Exit TinyRcon?")) {
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

    case ID_CONNECTBUTTON: {
      (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to connect to ^1%s ^3game server?\n", main_app.get_game_server_name().c_str());
      if (show_user_confirmation_dialog(message_buffer, "Connect to game server?")) {
        match_results<string::const_iterator> ip_port_match{};
        const string ip_port_server_address{ regex_search(admin_reason, ip_port_match, ip_address_and_port_regex) ? ip_port_match[1].str() + ":"s + ip_port_match[2].str() : main_app.get_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_game_server().get_server_port()) };
        const size_t sep_pos{ ip_port_server_address.find(':') };
        const string ip_address{ ip_port_server_address.substr(0, sep_pos) };
        const uint16_t port_number{ static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1))) };
        const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(ip_address.c_str(), port_number, main_app.get_game_server().get_rcon_password().c_str());

        const game_name_t game_name{ result.second != game_name_t::unknown ? result.second : main_app.get_game_name() };
        connect_to_the_game_server(ip_port_server_address, game_name, false, true);
      }
    } break;

    case ID_CONNECTPRIVATESLOTBUTTON: {

      (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to connect to ^1%s ^3game server using a ^1private slot^3?\n", main_app.get_game_server_name().c_str());
      if (show_user_confirmation_dialog(message_buffer, "Connect to game server using a private slot?")) {
        match_results<string::const_iterator> ip_port_match{};
        const string ip_port_server_address{ regex_search(admin_reason, ip_port_match, ip_address_and_port_regex) ? ip_port_match[1].str() + ":"s + ip_port_match[2].str() : main_app.get_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_game_server().get_server_port()) };
        const size_t sep_pos{ ip_port_server_address.find(':') };
        const string ip_address{ ip_port_server_address.substr(0, sep_pos) };
        const uint16_t port_number{ static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1))) };
        const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(ip_address.c_str(), port_number, main_app.get_game_server().get_rcon_password().c_str());

        const game_name_t game_name{ result.second != game_name_t::unknown ? result.second : main_app.get_game_name() };
        connect_to_the_game_server(ip_port_server_address, game_name, true, true);
      }
    } break;

    case ID_REFRESHDATABUTTON: {
      initiate_sending_rcon_status_command_now();
    } break;


    case ID_LOADBUTTON: {
      char full_map[256];
      char rcon_gametype[16];
      ComboBox_GetText(app_handles.hwnd_combo_box_map, full_map, 256);
      ComboBox_GetText(app_handles.hwnd_combo_box_gametype, rcon_gametype, 16);
      const auto &full_map_names_to_rcon_map_names = get_full_map_names_to_rcon_map_names_for_specified_game_name(main_app.get_game_name());
      const auto &full_gametype_names = get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(main_app.get_game_name());
      if (stl::helper::len(full_map) > 0U && stl::helper::len(rcon_gametype) > 0U && full_map_names_to_rcon_map_names.contains(full_map) && full_gametype_names.contains(rcon_gametype)) {
        const string gametype_uc{ stl::helper::to_upper_case(rcon_gametype) };
        (void)snprintf(message_buffer, std::size(message_buffer), "^2Are you sure you want to load map ^3%s ^2in ^3%s ^2game type?\n", full_map, gametype_uc.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          const string load_map_command{ stl::helper::str_join(std::vector<string>{ "!m", full_map_names_to_rcon_map_names.at(full_map), rcon_gametype }, " ") };
          Edit_SetText(app_handles.hwnd_e_user_input, load_map_command.c_str());
          get_user_input();
        }
      }
    } break;

    case ID_PRINTPLAYERINFORMATION_ACTION: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          (void)snprintf(message_buffer, std::size(message_buffer), "^5Information about selected player:\n ^7%s\n", player_information.c_str());
          print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }
      }

    } break;

    case ID_WARNBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to warn selected player?\n ^7%s", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "!w %d %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            admin_reason.assign("not specified");
            get_user_input();
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_KICKBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to kick selected player?\n ^7%s", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "!k %d %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            admin_reason.assign("not specified");
            get_user_input();
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }

    } break;

    case ID_TEMPBANBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to temporarily ban IP address of selected player?\n ^7%s", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "!tb %d 24 %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }

    } break;

    case ID_GUIDBANBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban GUID key of selected player?\n ^7%s", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "!b %d %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }

    } break;

    case ID_IPBANBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban IP address of selected player?\n ^7%s", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "!gb %d %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }

    } break;


    case ID_IPRANGEBANBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          auto &pd = get_player_data_for_pid(pid);
          const string narrow_ip_address_range{ get_narrow_ip_address_range_for_specified_ip_address(pd.ip_address) };
          (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban IP address range ^1%s ^3of selected player?\n ^7%s", narrow_ip_address_range.c_str(), player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "!br %d %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_CITYBANBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          auto &pd = get_player_data_for_pid(pid);
          (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban city of selected player?\n ^7%s", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "!bancity %s", pd.city);
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }

    } break;

    case ID_COUNTRYBANBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          auto &pd = get_player_data_for_pid(pid);
          (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban country of selected player?\n ^7%s", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "!bancountry %s", pd.country_name);
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }

    } break;

    case ID_ENABLECITYBANBUTTON: {
      Edit_SetText(app_handles.hwnd_e_user_input, "!egb");
      get_user_input();
    } break;

    case ID_DISABLECITYBANBUTTON: {
      Edit_SetText(app_handles.hwnd_e_user_input, "!dgb");
      get_user_input();
    } break;

    case ID_ENABLECOUNTRYBANBUTTON: {
      Edit_SetText(app_handles.hwnd_e_user_input, "!ecb");
      get_user_input();
    } break;

    case ID_DISABLECOUNTRYBANBUTTON: {
      Edit_SetText(app_handles.hwnd_e_user_input, "!dcb");
      get_user_input();

    } break;

    case ID_SAY_BUTTON: {

      admin_reason = "Type your public message in here...";
      if (show_user_confirmation_dialog("^2Are you sure you want to send a ^1public message ^2visible to all players?\n", "Confirm your action", "Message:")) {
        (void)snprintf(msg_buffer, std::size(msg_buffer), "say \"%s\"", admin_reason.c_str());
        Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
        get_user_input();
        admin_reason.assign("not specified");
      }

    } break;

    case ID_TELL_BUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid, true) };
          (void)snprintf(message_buffer, std::size(message_buffer), "^2Are you sure you want to send a ^1private message ^2to selected player?\n^7(%s^7)", player_information.c_str());
          admin_reason = "Type your private message in here...";
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action", "Message:")) {
            (void)snprintf(msg_buffer, std::size(msg_buffer), "tell %d \"%s\"", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      }
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

    case ID_VIEWADMINSDATA:
      is_display_admins_data_event.store(true);
      break;

    case ID_SORT_PLAYERS_DATA_BY_PID:
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_pid_asc ? sort_type::pid_asc : sort_type::pid_desc;
      sort_by_pid_asc = !sort_by_pid_asc;
      process_sort_type_change_request(type_of_sort);
      break;

    case ID_SORT_PLAYERS_DATA_BY_SCORE:
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_score_asc ? sort_type::score_asc : sort_type::score_desc;
      sort_by_score_asc = !sort_by_score_asc;
      process_sort_type_change_request(type_of_sort);
      break;

    case ID_SORT_PLAYERS_DATA_BY_PING:
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_ping_asc ? sort_type::ping_asc : sort_type::ping_desc;
      sort_by_ping_asc = !sort_by_ping_asc;
      process_sort_type_change_request(type_of_sort);
      break;

    case ID_SORT_PLAYERS_DATA_BY_NAME:
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_name_asc ? sort_type::name_asc : sort_type::name_desc;
      sort_by_name_asc = !sort_by_name_asc;
      process_sort_type_change_request(type_of_sort);
      break;

    case ID_SORT_PLAYERS_DATA_BY_IP:
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_ip_asc ? sort_type::ip_asc : sort_type::ip_desc;
      sort_by_ip_asc = !sort_by_ip_asc;
      process_sort_type_change_request(type_of_sort);
      break;

    case ID_SORT_PLAYERS_DATA_BY_GEO:
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_geo_asc ? sort_type::geo_asc : sort_type::geo_desc;
      sort_by_geo_asc = !sort_by_geo_asc;
      process_sort_type_change_request(type_of_sort);
      break;

    case ID_BUTTON_CONFIGURE_SERVER_SETTINGS:
      if (!show_and_process_tinyrcon_configuration_panel("Configure and verify your game server's settings.")) {
        show_error(app_handles.hwnd_main_window, "Failed to construct and show TinyRcon configuration dialog!", 0);
      }
      if (is_terminate_program.load()) {
        lock_guard ul{ mu };
        exit_flag.notify_all();
        PostQuitMessage(0);
      }
      break;
    }

    if (is_process_combobox_item_selection_event && wparam_high_word == CBN_SELCHANGE) {
      const auto combo_box_id = LOWORD(wParam);
      if (app_handles.hwnd_combo_box_sortmode == (HWND)lParam && ID_COMBOBOX_SORTMODE == combo_box_id) {
        ComboBox_GetText(app_handles.hwnd_combo_box_sortmode, message_buffer, std::size(message_buffer));
        if (sort_mode_names_dict.find(message_buffer) != cend(sort_mode_names_dict)) {
          const auto selected_sort_type = sort_mode_names_dict.at(message_buffer);
          if (selected_sort_type != type_of_sort) {
            process_sort_type_change_request(selected_sort_type);
          }
        }
      }
    }
  } break;
  case WM_PAINT: {

    HDC hdc = BeginPaint(hWnd, &ps);

    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, color::black);
    SetTextColor(hdc, color::red);

    RECT bounding_rectangle = {
      screen_width / 2 + 170, screen_height / 2 + 27, screen_width / 2 + 210, screen_height / 2 + 47
    };
    DrawText(hdc, "Map:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);

    bounding_rectangle = { screen_width / 2 + 370, screen_height / 2 + 27, screen_width / 2 + 450, screen_height / 2 + 47 };
    DrawText(hdc, "Gametype:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);

    bounding_rectangle = {
      10,
      screen_height - 75,
      130,
      screen_height - 55
    };

    DrawText(hdc, prompt_message, -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT | DT_END_ELLIPSIS);

    bounding_rectangle = {
      screen_width - 270,
      screen_height - 105,
      screen_width - 5,
      screen_height - 85,
    };

    if (!is_tinyrcon_initialized) {
      DrawText(hdc, "Configuring and initializing tinyrcon...", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);
    } else {

      // const size_t time_period = main_app.get_game_server().get_check_for_banned_players_time_period();

      atomic_counter.store(std::min<size_t>(atomic_counter.load(), time_period));
      DrawText(hdc, "", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);

      /*if (atomic_counter.load() == time_period) {
        DrawText(hdc, "Refreshing players data now...", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);
      } else {
        const size_t remaining_seconds{ time_period - atomic_counter.load() };
        (void)snprintf(msg_buffer, std::size(msg_buffer), refresh_players_data_fmt_str, remaining_seconds, (remaining_seconds != 1 ? "seconds" : "second"));
        DrawText(hdc, msg_buffer, -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);
      }*/
    }

    EndPaint(hWnd, &ps);

    /*if (is_display_temporarily_banned_players_data_event.load()) {
      display_temporarily_banned_ip_addresses();
      is_display_temporarily_banned_players_data_event.store(false);
    } else if (is_display_permanently_banned_players_data_event.load()) {
      display_permanently_banned_ip_addresses();
      is_display_permanently_banned_players_data_event.store(false);
    } else if (is_display_banned_cities_data_event.load()) {
      display_banned_cities(main_app.get_game_server().get_list_of_cities_for_automatic_kick());
      is_display_banned_cities_data_event.store(false);
    } else if (is_display_banned_countries_data_event.load()) {
      display_banned_countries(main_app.get_game_server().get_list_of_countries_for_automatic_kick());
      is_display_banned_countries_data_event.store(false);
    }*/
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

  case WM_CTLCOLORLISTBOX: {
    COMBOBOXINFO info1{};
    info1.cbSize = sizeof(info1);
    SendMessage(app_handles.hwnd_combo_box_map, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info1);
    COMBOBOXINFO info2{};
    info2.cbSize = sizeof(info2);
    SendMessage(app_handles.hwnd_combo_box_gametype, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info2);

    COMBOBOXINFO info3{};
    info3.cbSize = sizeof(info3);
    SendMessage(app_handles.hwnd_combo_box_sortmode, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info3);

    if ((HWND)lParam == info1.hwndList) {
      HDC dc = (HDC)wParam;
      SetBkMode(dc, OPAQUE);
      SetTextColor(dc, color::blue);
      SetBkColor(dc, color::yellow);
      if (comboBrush1 == NULL)
        comboBrush1 = CreateSolidBrush(color::yellow);
      return (INT_PTR)comboBrush1;
    }
    if ((HWND)lParam == info2.hwndList) {
      HDC dc = (HDC)wParam;
      SetBkMode(dc, OPAQUE);
      SetTextColor(dc, /*is_focused ? color::red : */ color::blue);
      SetBkColor(dc, color::yellow);
      if (comboBrush2 == NULL)
        comboBrush2 = CreateSolidBrush(color::yellow);
      return (INT_PTR)comboBrush2;
    }

    if ((HWND)lParam == info3.hwndList) {
      HDC dc = (HDC)wParam;
      SetBkMode(dc, OPAQUE);
      SetTextColor(dc, /*is_focused ? color::red : */ color::blue);
      SetBkColor(dc, color::yellow);
      if (comboBrush5 == NULL)
        comboBrush5 = CreateSolidBrush(color::yellow);
      return (INT_PTR)comboBrush5;
    }
  } break;

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


LRESULT CALLBACK WndProcForConfigurationDialog(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  constexpr size_t max_path_length{ 32768 };
  static char install_path[max_path_length];
  static char exe_file_path[max_path_length];
  static char msg_buffer[1024];
  HBRUSH orig_textEditBrush{};
  HBRUSH comboBrush1{}, comboBrush2{}, comboBrush3{}, comboBrush4{};
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

    if ((some_item->idFrom == ID_BUTTON_SAVE_CHANGES || some_item->idFrom == ID_BUTTON_TEST_CONNECTION || some_item->idFrom == ID_BUTTON_CANCEL || some_item->idFrom == ID_BUTTON_CONFIGURATION_EXIT_TINYRCON || some_item->idFrom == ID_BUTTON_CONFIGURATION_COD1_PATH || some_item->idFrom == ID_BUTTON_CONFIGURATION_COD2_PATH || some_item->idFrom == ID_BUTTON_CONFIGURATION_COD4_PATH || some_item->idFrom == ID_BUTTON_CONFIGURATION_COD5_PATH)
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

    case ID_ENABLE_CITY_BAN_CHECKBOX: {
      if (Button_GetCheck(app_handles.hwnd_enable_city_ban) == BST_UNCHECKED) {
        main_app.get_game_server().set_is_automatic_city_kick_enabled(true);
        Button_SetCheck(app_handles.hwnd_enable_city_ban, BST_CHECKED);
      } else {
        main_app.get_game_server().set_is_automatic_city_kick_enabled(false);
        Button_SetCheck(app_handles.hwnd_enable_city_ban, BST_UNCHECKED);
      }
    } break;

    case ID_ENABLE_COUNTRY_BAN_CHECKBOX: {

      if (Button_GetCheck(app_handles.hwnd_enable_country_ban) == BST_UNCHECKED) {
        main_app.get_game_server().set_is_automatic_country_kick_enabled(true);
        Button_SetCheck(app_handles.hwnd_enable_country_ban, BST_CHECKED);
      } else {
        main_app.get_game_server().set_is_automatic_country_kick_enabled(false);
        Button_SetCheck(app_handles.hwnd_enable_country_ban, BST_UNCHECKED);
      }
    } break;

    case ID_BUTTON_SAVE_CHANGES: {
      process_button_save_changes_click_event(app_handles.hwnd_configuration_dialog);

    } break;

    case ID_BUTTON_TEST_CONNECTION: {
      process_button_test_connection_click_event(app_handles.hwnd_configuration_dialog);

    } break;

    case ID_BUTTON_CANCEL: {
      is_terminate_tinyrcon_settings_configuration_dialog_window.store(true);
      EnableWindow(app_handles.hwnd_main_window, TRUE);
      SetFocus(app_handles.hwnd_e_user_input);
      DestroyWindow(app_handles.hwnd_configuration_dialog);
    } break;

    case ID_BUTTON_CONFIGURATION_EXIT_TINYRCON: {
      EnableWindow(app_handles.hwnd_main_window, TRUE);
      SetFocus(app_handles.hwnd_e_user_input);
      DestroyWindow(app_handles.hwnd_configuration_dialog);
      is_terminate_program.store(true);
      {
        lock_guard ul{ mu };
        exit_flag.notify_all();
      }
    } break;

    case ID_BUTTON_CONFIGURATION_COD1_PATH: {

      str_copy(install_path, "C:\\");
      (void)snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty game's installation folder (codmp.exe) and click OK.");

      const char *cod1_game_path = BrowseFolder(install_path, msg_buffer);

      if (lstrcmp(cod1_game_path, "") == 0 || lstrcmp(cod1_game_path, "C:\\") == 0) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^1Error! You haven't selected a valid folder for your game installation.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "^5You have to select your ^1game's installation directory\n ^5and click the OK button.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      } else {
        (void)snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", cod1_game_path);
      }

      stl::helper::trim_in_place(exe_file_path);

      Edit_SetText(app_handles.hwnd_cod1_path_edit, exe_file_path);

    } break;

    case ID_BUTTON_CONFIGURATION_COD2_PATH: {

      str_copy(install_path, "C:\\");
      (void)snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty 2 game's installation folder (cod2mp_s.exe) and click OK.");

      const char *cod2_game_path = BrowseFolder(install_path, msg_buffer);

      if (lstrcmp(cod2_game_path, "") == 0 || lstrcmp(cod2_game_path, "C:\\") == 0) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^1Error! You haven't selected a valid folder for your game installation.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "^5You have to select your ^1game's installation directory\n ^5and click the OK button.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      } else {
        (void)snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", cod2_game_path);
      }

      stl::helper::trim_in_place(exe_file_path);

      Edit_SetText(app_handles.hwnd_cod2_path_edit, exe_file_path);

    } break;

    case ID_BUTTON_CONFIGURATION_COD4_PATH: {

      str_copy(install_path, "C:\\");
      (void)snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty 4: Modern Warfare game's installation folder (iw3mp.exe) and click OK.");

      const char *cod4_game_path = BrowseFolder(install_path, msg_buffer);

      if (lstrcmp(cod4_game_path, "") == 0 || lstrcmp(cod4_game_path, "C:\\") == 0) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^1Error! You haven't selected a valid folder for your game installation.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "^5You have to select your ^1game's installation directory\n ^5and click the OK button.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      } else {
        (void)snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", cod4_game_path);
      }

      stl::helper::trim_in_place(exe_file_path);

      Edit_SetText(app_handles.hwnd_cod4_path_edit, exe_file_path);

    } break;

    case ID_BUTTON_CONFIGURATION_COD5_PATH: {

      str_copy(install_path, "C:\\");
      (void)snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty 5: World at War game's installation folder (cod5mp.exe) and click OK.");

      const char *cod5_game_path = BrowseFolder(install_path, msg_buffer);

      if (lstrcmp(cod5_game_path, "") == 0 || lstrcmp(cod5_game_path, "C:\\") == 0) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^1Error! You haven't selected a valid folder for your game installation.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        print_colored_text(app_handles.hwnd_re_messages_data, "^5You have to select your ^1game's installation directory\n ^5and click the OK button.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      } else {
        (void)snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", cod5_game_path);
      }

      stl::helper::trim_in_place(exe_file_path);

      Edit_SetText(app_handles.hwnd_cod5_path_edit, exe_file_path);

    } break;


    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }

  } break;

  case WM_CLOSE:
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
    if (defaultbrush) DeleteBrush(defaultbrush);
    if (hotbrush) DeleteBrush(hotbrush);
    if (selectbrush) DeleteBrush(selectbrush);
    if (red_pen) DeletePen(red_pen);
    if (light_blue_pen) DeletePen(light_blue_pen);
    PostQuitMessage(0);
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
