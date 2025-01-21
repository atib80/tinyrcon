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

extern const string program_version{ "2.7.7.3" };

extern const std::regex ip_address_and_port_regex;
extern const std::unordered_map<char, COLORREF> rich_edit_colors;

extern std::atomic<bool> is_terminate_program;
extern std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window;
extern string g_message_data_contents;
extern string online_admins_information;

tiny_rcon_client_application main_app;
sort_type type_of_sort{ sort_type::geo_asc };

volatile atomic<size_t> atomic_counter{ 0U };
volatile std::atomic<bool> is_refresh_players_data_event{ false };
volatile std::atomic<bool> is_refreshed_players_data_ready_event{ false };
volatile std::atomic<bool> is_display_players_data{ true };
// volatile std::atomic<bool> is_refreshing_game_servers_data_event{ false };
volatile std::atomic<bool> is_player_being_spectated{ false };
volatile std::atomic<int> spectated_player_pid{ -1 };
static std::atomic<size_t> number_of_entries_to_display{ 25u };

extern const int screen_width{ GetSystemMetrics(SM_CXSCREEN) };
extern const int screen_height{ GetSystemMetrics(SM_CYSCREEN) - 30 };
RECT client_rect{ 0, 0, screen_width, screen_height };

extern const char *user_help_message =
  R"(^5For a list of possible commands type ^1help ^5or ^1list user ^5or ^1list rcon ^5in the console.
^3Type ^1!say "Public message" to all players" ^3[Enter] to send "Public message" to all online players.
^5Type ^1!tell 12 "Private message" ^5[Enter] to send "Private message" to only player whose pid = ^112
^5Type ^1!w 12 optional_reason ^5[Enter] to warn player whose pid = ^112
       ^5(Player with ^12 warnings ^5is automatically kicked.)
^3Type ^1!k 12 optional_reason ^3[Enter] to kick player whose pid = ^112
^5Type ^1!tb 12 24 optional_reason ^5[Enter] to temporarily ban (for 24 hours) IP address of player whose pid = ^112
^3Type ^1!gb 12 optional_reason ^3[Enter] to ban IP address of player whose pid = ^112
^3Type ^1!ub 123.123.123.123 ^3[Enter] to remove temporarily and/or permanently banned IP address.
^5Type ^1bans 20 ^5[Enter] to see the ^1last 20 IP bans ^5or type ^1bans all ^5to see ^1all ^5of the ^1IP bans.
^3Type ^1tempbans 20 ^3[Enter] to see the ^1last 20 temporary IP bans ^3or type ^1tempbans all ^3to see 
 ^1all ^3of the ^1temporary IP bans.
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
^5Type ^1!br 12|123.123.123.* optional_reason ^5[Enter] to ban IP address range of player 
 whose pid = ^112 ^5or IP address range = ^1123.123.123.*
^3Type ^1!ubr 123.123.123.* optional_reason ^3[Enter] to remove previously banned IP address range ^1123.123.123.*
^5Type ^1ranges 20 ^5[Enter] to see the ^1last 20 banned IP address ranges ^5or type ^1ranges all ^5to see 
 ^1all ^5of the ^1banned IP address ranges.
^3Type ^1!protectip pid|ip_address optional_reason ^3to protect player's ^1IP address ^3whose ^1'pid' ^3number or
  ^1'IP address' ^3is equal to specified ^1'pid' ^3or ^1'ip_address'^3, respectively.
^5Type ^1!unprotectip pid|ip_address optional_reason ^5to remove protected ^1IP address ^5of player whose ^1'pid' ^5number or
  ^1'IP address' ^5is equal to specified ^1'pid' ^5or ^1'ip_address'^5, respectively.
^3Type ^1!protectiprange pid|ip_address_range optional_reason ^3to protect player's ^1IP address range ^3whose ^1'pid' ^3number or
  ^1'IP address range' ^3is equal to specified ^1'pid' ^3or ^1'ip_address_range'^3, respectively.
^5Type ^1!unprotectiprange pid|ip_address_range optional_reason ^5to remove protected ^1IP address range ^5of player 
 whose ^1'pid' ^5number or ^1'IP address range' ^5is equal to specified ^1'pid' ^5or ^1'ip_address_range'^5, respectively.
^3Type ^1!protectcity pid|city_name ^3to protect player's ^1city ^3whose ^1'pid' ^3number or ^1'city name' ^3is equal to 
  specified ^1'pid' ^3or ^1'city_name'^3, respectively.
^5Type ^1!unprotectcity pid|city_name ^5to remove protected city of player whose ^1'pid' ^5number or ^1'city name' ^5is equal 
  to specified ^1'pid' ^5or ^1'city_name'^5, respectively.
^3Type ^1!protectcountry pid|country_name ^3to protect player's country whose ^1'pid' ^3number or ^1'country name' ^3is equal to 
 specified ^1'pid' ^3or ^1'valid_ip_address'^3, respectively.
^5Type ^1!unprotectcountry pid|country_name ^5to remove protected ^1IP address ^5of player whose ^1'pid' ^5number or 
 ^1'country name' ^5is equal to specified ^1'pid' ^5or ^1'country_name'^5, respectively.
^3Type ^1!admins ^3[Enter] to see all registered admins' data and their statistics.
^5Type ^1!t message ^5[Enter] to send ^1message ^5to all logged in ^1admins.
^3Type ^1!y username message ^3[Enter] to send ^1message ^3to ^1admin ^3whose user name is ^1username. 
^5Examples: ^3!y }|{opuk smotri Pronik#123, !y username privet. ^1Color codes in username are ignored.
^1Capital letters in username are converted into small letters. ^3ACID ^7= acid, ^1W^4W ^7= ww
^5Type ^1!banname pid:12|name ^5[Enter] to ban ^1player name ^5of online player whose ^1pid ^5is equal to ^112
 ^5or to ban specified player name ^1'name'.
^3Type ^1!unbanname name ^3[Enter] to remove previously banned player name ^1name.
^5Type ^1!names 25 ^5[Enter] to see the last ^125 ^5banned player names ^5or type ^1!names all 
  ^5to see ^1all banned player names.
^3Type ^1!stats ^3[Enter] to view up-to-date ^5Tiny^6Rcon ^1admins' ^3related stats data.
^5Type ^1!report 12 optional_reason ^5[Enter] to report player whose ^1pid ^5is equal to ^112 
  ^5with reason ^1optional_reason.
^3Type ^1!unreport 123.123.123.123 optional_reason ^3[Enter] to remove previously reported player 
  whose ^1IP address is ^1123.123.123.123.
^5Type ^1!reports 25 ^3[Enter] to display the last ^125 ^5reported players.
^3Type ^1!tp  optional_number ^3[Enter] to view top ^125 ^3or ^1optional_number ^3players' stats data.
^5Type ^1!tpd optional_number ^5[Enter] to view top ^125 ^5or ^1optional_number ^5players' stats data for current day.
^3Type ^1!tpm optional_number ^3[Enter] to view top ^125 ^3or ^1optional_number ^3players' stats data for current month.
^5Type ^1!tpy optional_number ^5[Enter] to view top ^125 ^5or ^1optional_number ^5players' stats data for current year.
^3Type ^1!s player_name ^3[Enter] to view stats data for player whose partially or fully specified name is ^1player_name.
  ^3You can omit ^1color codes ^3in partially or fully specified ^1player_name^3.
^5Type ^1!addip 123.123.123.123 wh ^5[Enter] to ban custom IP address ^1123.123.123.123 ^5with reason ^1wh.
^3Type ^1!addip 123.123.123.123 n:'^7Pro100Nik#^2123^1' r:wh ^3[Enter] to ban custom IP address ^1123.123.123.123
  ^3(of player named ^7Pro100Nik#^2123^3) with specified reason: ^1wh 
  ^7If player name contains space characters surround it with a pair of ' characters as shown in the example.
^5Type ^1!rc custom_rcon_command [optional_parameters] ^5[Enter] to send ^1custom_rcon_command [optional_parameters] 
  ^5to currently viewed ^3game server^5. ^7Examples: ^1!rc map_restart ^7| ^1!rc fast_restart ^7| ^1!rc sv_iwdnames
^3Type ^1!spec 12 ^3[Enter] to force your game to spectate player whose ^1pid number ^3is ^112.
^5Type ^1!nospec ^5[Enter] or press the ^1Escape key ^5in the game to force your game to ^1stop spectating 
  ^5the previously selected player.
^3Type ^1!std 100-2000 ^3[Enter] to set the ^1current time delay ^3which has to elapse between switching from
  one player to another (^5minimum allowed time delay: ^1100 ms ^3| ^5maximum allowed time delay: ^12000 ms^3)
^5Type ^1!mute 12 ^5[Enter] to mute player whose ^1pid number ^5is ^112.
^3Type ^1!unmute 12 ^3[Enter] to unmute previously muted player whose ^1pid number ^3is ^112.
^5Type ^1!muted 25 ^5[Enter] to see the last ^125 ^5muted player entries ^5or type ^1!muted all 
  ^5to see ^1all muted player entries.)";


extern const std::unordered_map<int, string> button_id_to_label_text{
  { ID_MUTE_PLAYER, "Mute player" },
  { ID_UNMUTE_PLAYER, "Unmute player" },
  { ID_VIEWMUTEDPLAYERS, "View muted players" },
  { ID_WARNBUTTON, "&Warn" },
  { ID_KICKBUTTON, "&Kick" },
  { ID_TEMPBANBUTTON, "&Tempban" },
  { ID_IPBANBUTTON, "&Ban IP" },
  { ID_VIEWTEMPBANSBUTTON, "View temporary bans" },
  { ID_VIEWIPBANSBUTTON, "View &IP bans" },
  { ID_VIEWADMINSDATA, "View &admins' data" },
  { ID_RCONVIEWBUTTON, "&Show rcon server" },
  { ID_SHOWPLAYERSBUTTON, "Show players" },
  { ID_REFRESH_PLAYERS_DATA_BUTTON, "&Refresh players" },
  // { ID_SHOWSERVERSBUTTON, "Show servers" },
  // { ID_REFRESHSERVERSBUTTON, "Refresh servers" },
  { ID_CONNECTBUTTON, "&Join server" },
  { ID_CONNECTPRIVATESLOTBUTTON, "Joi&n server (private slot)" },
  { ID_SAY_BUTTON, "Send public message" },
  { ID_TELL_BUTTON, "Send private message" },
  { ID_QUITBUTTON, "E&xit" },
  { ID_LOADBUTTON, "Load &map" },
  { ID_YES_BUTTON, "Yes" },
  { ID_NO_BUTTON, "No" },
  { ID_BUTTON_SAVE_CHANGES, "Save changes" },
  { ID_BUTTON_TEST_CONNECTION, "Test connection" },
  { ID_BUTTON_CANCEL, "Cancel" },
  { ID_BUTTON_CONFIGURE_SERVER_SETTINGS, "Confi&gure settings" },
  { ID_CLEARMESSAGESCREENBUTTON, "C&lear messages" },
  { ID_BUTTON_CONFIGURATION_EXIT_TINYRCON, "Exit TinyRcon" },
  { ID_BUTTON_CONFIGURATION_COD1_PATH, "Browse for codmp.exe" },
  { ID_BUTTON_CONFIGURATION_COD2_PATH, "Browse for cod2mp_s.exe" },
  { ID_BUTTON_CONFIGURATION_COD4_PATH, "Browse for iw3mp.exe" },
  { ID_BUTTON_CONFIGURATION_COD5_PATH, "Browse for cod5mp.exe" }
};

extern const std::unordered_map<string, sort_type> sort_mode_names_dict;

unordered_map<size_t, string> rcon_status_grid_column_header_titles;
unordered_map<size_t, string> get_status_grid_column_header_titles;
// unordered_map<size_t, string> servers_grid_column_header_titles;

extern const char *prompt_message{ "Administrator >>" };
// extern const char *refresh_players_data_fmt_str{ "Refreshing players data in %zu %s." };
extern const size_t max_players_grid_rows;
// extern const size_t max_servers_grid_rows;

tiny_rcon_handles app_handles{};

WNDCLASSEX wcex, wcex_confirmation_dialog, wcex_configuration_dialog;

player selected_player{};
size_t selected_server_index{};
int selected_player_row{};
int selected_player_col{};
int selected_server_row{};
int selected_server_col{};

extern bool sort_by_pid_asc;
extern bool sort_by_score_asc;
extern bool sort_by_ping_asc;
extern bool sort_by_name_asc;
extern bool sort_by_ip_asc;
extern bool sort_by_geo_asc;

bool is_main_window_constructed{};
bool is_tinyrcon_initialized{};
bool is_process_combobox_item_selection_event{ true };
bool is_first_left_mouse_button_click_in_prompt_edit_control{ true };
bool is_first_left_mouse_button_click_in_reason_edit_control{ true };
extern const HBRUSH red_brush{ CreateSolidBrush(color::red) };
extern const HBRUSH black_brush{ CreateSolidBrush(color::black) };
atomic<int> admin_choice{ 0 };
string admin_reason{ "not specified" };
string copied_game_server_address;
extern HIMAGELIST hImageList;
// HFONT font_for_players_grid_data{};

extern const map<string, string> user_commands_help;
extern const map<string, string> rcon_commands_help;

ATOM register_window_classes(HINSTANCE hInstance);
bool initialize_main_app(HINSTANCE, const int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR);
LRESULT CALLBACK WndProcForConfirmationDialog(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProcForConfigurationDialog(HWND, UINT, WPARAM, LPARAM);
char selected_re_text[8196];
static std::random_device rd{};
static std::mt19937 gen(rd());
static std::uniform_int_distribution<> random_number_distribution(1, 44);
HBITMAP g_hBitMap;
shared_ptr<tiny_rcon_client_user> me{};

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE,
  _In_ LPSTR,
  _In_ int nCmdShow)
{
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

  InitCommonControls();
  LoadLibrary("Riched20.dll");

  HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
  // GdiplusStartupInput gdiplusStartupInput{};
  // ULONG_PTR gdiplusToken{};
  // GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

  register_window_classes(hInstance);

  if (!initialize_main_app(hInstance, nCmdShow))
    return 0;

  main_app.set_current_working_directory();

  /*const string config_folder_path{ format("\"{}{}\"", main_app.get_current_working_directory(), "config") };
  const string data_folder_path{ format("\"{}{}\"", main_app.get_current_working_directory(), "data") };
  const string log_folder_path{ format("\"{}{}\"", main_app.get_current_working_directory(), "log") };
  const string plugins_geoIP_folder_path{ format("\"{}{}\"", main_app.get_current_working_directory(), "plugins\\geoIP") };
  const string config_file_path{ format("\"{}{}\"", main_app.get_current_working_directory(), "config\\tinyrcon.json") };*/
  const string config_file_path{ format("{}{}", main_app.get_current_working_directory(), main_app.get_tinyrcon_config_file_path()) };

  if (auto [status, file_path] = create_necessary_folders_and_files({
        "config",
        "data",
        "data\\images\\maps",
        "log",
        "plugins\\geoIP",
        "temp",
        "tools",
        main_app.get_tinyrcon_config_file_path(),
        main_app.get_user_data_file_path(),
        main_app.get_temp_bans_file_path(),
        main_app.get_ip_bans_file_path(),
        main_app.get_ip_range_bans_file_path(),
        main_app.get_banned_countries_file_path(),
        main_app.get_banned_cities_file_path(),
        main_app.get_banned_names_file_path(),
        main_app.get_protected_ip_addresses_file_path(),
        main_app.get_protected_ip_address_ranges_file_path(),
        main_app.get_protected_cities_file_path(),
        main_app.get_protected_countries_file_path(),
      });
      !status) {

    show_error(app_handles.hwnd_main_window, "Error creating necessary program folders and files!", 0);
  }

  SetFileAttributes(config_file_path.c_str(),
    GetFileAttributes(config_file_path.c_str()) & ~FILE_ATTRIBUTE_READONLY);
  SetFileAttributes(config_file_path.c_str(), FILE_ATTRIBUTE_NORMAL);

  rcon_status_grid_column_header_titles[0] = main_app.get_header_player_pid_color() + "Pid"s;
  rcon_status_grid_column_header_titles[1] = main_app.get_header_player_score_color() + "Score"s;
  rcon_status_grid_column_header_titles[2] = main_app.get_header_player_ping_color() + "Ping"s;
  rcon_status_grid_column_header_titles[3] = main_app.get_header_player_name_color() + "Player name"s;
  rcon_status_grid_column_header_titles[4] = main_app.get_header_player_ip_color() + "IP address"s;
  rcon_status_grid_column_header_titles[5] = main_app.get_header_player_geoinfo_color() + "Geological information"s;
  rcon_status_grid_column_header_titles[6] = main_app.get_header_player_geoinfo_color() + "Flag"s;

  get_status_grid_column_header_titles[0] = main_app.get_header_player_pid_color() + "Player no."s;
  get_status_grid_column_header_titles[1] = main_app.get_header_player_score_color() + "Score"s;
  get_status_grid_column_header_titles[2] = main_app.get_header_player_ping_color() + "Ping"s;
  get_status_grid_column_header_titles[3] = main_app.get_header_player_name_color() + "Player name"s;

  /*servers_grid_column_header_titles[0] = "Id"s;
  servers_grid_column_header_titles[1] = "Server name"s;
  servers_grid_column_header_titles[2] = "Server address"s;
  servers_grid_column_header_titles[3] = "Players"s;
  servers_grid_column_header_titles[4] = "Current map"s;
  servers_grid_column_header_titles[5] = "Gametype"s;
  servers_grid_column_header_titles[6] = "Voice"s;
  servers_grid_column_header_titles[7] = "Flag"s;*/

  construct_tinyrcon_gui(app_handles.hwnd_main_window);

  main_app.open_log_file("log\\commands_history.log");

  std::jthread print_messages_thread{
    [&](stop_token st) {
      IsGUIThread(TRUE);

      while (!st.stop_requested() && !is_terminate_program.load()) {

        try {

          while (!st.stop_requested() && !is_terminate_program.load() && !main_app.is_tinyrcon_message_queue_empty()) {
            print_message_t msg{ main_app.get_tinyrcon_message_from_queue() };
            print_message(app_handles.hwnd_re_messages_data, msg.message_, msg.log_to_file_, msg.is_log_current_date_time_, msg.is_remove_color_codes_for_log_message_);
          }
        } catch (std::exception &ex) {
          const string error_message{ format("^3A specific exception was caught in print_messages_thread!\n^1Exception: {}", ex.what()) };
          print_message(app_handles.hwnd_re_messages_data, error_message);
        } catch (...) {
          char buffer[512];
          strerror_s(buffer, GetLastError());
          const string error_message{ format("^3A generic error was caught in print_messages_thread!\n^1Exception: {}", buffer) };
          print_message(app_handles.hwnd_re_messages_data, error_message);
        }

        Sleep(5);
      }
    }
  };

  parse_tinyrcon_tool_config_file(main_app.get_tinyrcon_config_file_path());

  find_call_of_duty_1_installation_path(false);
  find_call_of_duty_2_installation_path(false);
  find_call_of_duty_4_installation_path(false);
  find_call_of_duty_5_installation_path(false);

  main_app.get_bitmap_image_handler().set_bitmap_images_folder_path(format("{}data\\images\\maps", main_app.get_current_working_directory()));

  main_app.set_game_server_index(0);
  main_app.set_user_ip_address(trim(get_tiny_rcon_client_external_ip_address()));

  initialize_and_verify_server_connection_settings();

  string username{ main_app.get_username() };
  remove_all_color_codes(username);
  const string welcome_message{ format("Welcome back, private {} {}", main_app.get_is_connection_settings_valid() ? "admin" : "player", username) };
  Edit_SetText(app_handles.hwnd_e_user_input, welcome_message.c_str());

  load_tinyrcon_client_user_data(main_app.get_user_data_file_path());
  me = main_app.get_user_for_name(main_app.get_username());
  me->ip_address = main_app.get_user_ip_address();
  me->is_admin = true;
  me->is_logged_in = true;

  // load_image_files_information("data\\image_files_information.lst");

  main_app.add_command_handler({ "cls", "!cls" }, [](const vector<string> &) {
    Edit_SetText(app_handles.hwnd_re_messages_data, "");
    g_message_data_contents.clear();
  });


  main_app.add_command_handler({ "list", "!list", "help", "!help", "h", "!h" }, [](const vector<string> &user_cmd) {
    print_help_information(user_cmd);
  });

  main_app.add_command_handler({ "!r", "!report" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };

      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!ur", "!unreport" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };

      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "reports", "!reports" }, [](const vector<string> &user_cmd) {
    string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
    trim_in_place(user_command);
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!w", "!warn" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };

      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!mute" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command(
            "!mute", user_cmd[1])) {
        string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
        stack_trace_element ste{
          app_handles.hwnd_re_messages_data,
          std::move(ex_msg)
        };

        string ip_address;
        unsigned long guid_key{};
        if (int pid{ -1 }; is_valid_decimal_whole_number(user_cmd[1], pid) || check_ip_address_validity(user_cmd[1], guid_key)) {
          string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
          trim_in_place(user_command);
          main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        } else {
          const string re_msg{ format("^3Provided IP address ^1{} ^3is not a ^1valid IP address or there isn't an online player whose ^1pid number is equal to ^1{}.\n", user_cmd[1], user_cmd[1]) };
          print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        }
      }
    }
  });

  main_app.add_command_handler({ "!unmute" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      unsigned long ip_key{};
      if (int pid{ -1 }; !check_ip_address_validity(user_cmd[1], ip_key) && !is_valid_decimal_whole_number(user_cmd[1], pid)) {
        const string re_msg{ format("^3Provided IP address ^1{} ^3is not a ^1valid IP address or there isn't an online player whose ^1pid number is equal to ^1{}.\n", user_cmd[1], user_cmd[1]) };
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      } else {
        string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
        trim_in_place(user_command);
        main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      }
    }
  });

  main_app.add_command_handler({ "!k", "!kick" }, [](const std::vector<std::string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };

      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!tb", "!tempban" }, [](const std::vector<std::string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!b", "!ban" }, [](const std::vector<std::string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!gb", "!globalban", "!banip", "!addip" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!bn", "!banname" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;

    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };

      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!ubn", "!unbanname" }, [](const vector<string> &user_cmd) {
    if (!validate_admin_and_show_missing_admin_privileges_message(false)) return;
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };

      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  // !std
  main_app.add_command_handler({ "!std" }, [](const vector<string> &user_cmd) {
    string ex_msg{ "^1Exception ^3thrown from ^1command handler ^3for ^1!stats ^3user command." };
    stack_trace_element ste{
      app_handles.hwnd_re_messages_data,
      std::move(ex_msg)
    };

    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command("!std", user_cmd[1])) {
        const size_t std = stoul(user_cmd[1]);
        if (std != main_app.get_spec_time_delay()) {
          main_app.set_spec_time_delay(std);
          write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
        }
      }
    }
  });

  // !spec
  main_app.add_command_handler({ "!spec" }, [](const vector<string> &user_cmd) {
    string ex_msg{ "^1Exception ^3thrown from ^1command handler ^3for ^1!stats ^3user command." };
    stack_trace_element ste{
      app_handles.hwnd_re_messages_data,
      std::move(ex_msg)
    };

    if (is_player_being_spectated.load()) {
      selected_player = get_player_data_for_pid(spectated_player_pid.load());
      if (selected_player.pid != -1) {
        const string warning_message{ format("^7{} ^3stopped spectating player ^7{} ^3(^1pid = {}^3).\n", main_app.get_username(), selected_player.player_name, selected_player.pid) };
        print_colored_text(app_handles.hwnd_re_messages_data, warning_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, true, true);
      }
      is_player_being_spectated.store(false);

    } else if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      if (check_if_user_provided_argument_is_valid_for_specified_command("!spec", user_cmd[1])) {
        const int pid{ stoi(user_cmd[1]) };
        selected_player = get_player_data_for_pid(pid);
        if (-1 != selected_player.pid && selected_player.ip_address != main_app.get_user_ip_address()) {
          std::thread spectate_player_task{ spectate_player_for_specified_pid, pid };
          spectate_player_task.detach();
        }
      }
    }
  });

  // !nospec
  main_app.add_command_handler({ "!nospec" }, [](const vector<string> &) {
    string ex_msg{ "^1Exception ^3thrown from ^1command handler ^3for ^1!stats ^3user command." };
    stack_trace_element ste{
      app_handles.hwnd_re_messages_data,
      std::move(ex_msg)
    };
    if (is_player_being_spectated.load()) {
      selected_player = get_player_data_for_pid(spectated_player_pid.load());
      if (selected_player.pid != -1) {
        const string warning_message{ format("^7{} ^3stopped spectating player ^7{} ^3(^1pid = {}^3).\n", main_app.get_username(), selected_player.player_name, selected_player.pid) };
        print_colored_text(app_handles.hwnd_re_messages_data, warning_message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, true, true);
      }
      is_player_being_spectated.store(false);
    }
  });

  main_app.add_command_handler({ "!rc" }, [](const vector<string> &user_cmd) {
    string ex_msg{ "^1Exception ^3thrown from ^1command handler ^3for ^1!stats ^3user command." };
    stack_trace_element ste{
      app_handles.hwnd_re_messages_data,
      std::move(ex_msg)
    };

    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      const string rcon_command{ stl::helper::to_lower_case(stl::helper::trim(stl::helper::str_join(cbegin(user_cmd) + 1, cend(user_cmd), " "))) };
      if (rcon_command.find("rcon_password") != string::npos || rcon_command.find("sv_privatepassword") != string::npos) {
        const string warning_msg{ format("^3Cannot send disallowed ^1rcon command '^5{}^1' ^3to currently viewed game server!\n", rcon_command) };
        print_colored_text(app_handles.hwnd_re_messages_data, warning_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, true, true);
        return;
      }
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", rcon_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    } });

  main_app.add_command_handler({ "!stats" }, [](const vector<string> &) {
    string ex_msg{ "^1Exception ^3thrown from ^1command handler ^3for ^1!stats ^3user command." };
    stack_trace_element ste{
      app_handles.hwnd_re_messages_data,
      std::move(ex_msg)
    };

    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!stats", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!protectip" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!unprotectip" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!protectiprange" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!unprotectiprange" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!protectcity" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!unprotectcity" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!protectcountry" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!unprotectcountry" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });


  main_app.add_command_handler({ "!br", "!banrange" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!egb" }, [](const vector<string> &) {
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!egb", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!dgb" }, [](const vector<string> &) {
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!dgb", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!bancity" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });
  main_app.add_command_handler({ "!unbancity" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!ecb" }, [](const vector<string> &) {
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!ecb", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!dcb" }, [](const vector<string> &) {
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!dcb", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!bancountry" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!unbancountry" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() > 1 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      trim_in_place(user_command);
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "status", "!status" }, [](const vector<string> &) {
    initiate_sending_rcon_status_command_now();
  });

  main_app.add_command_handler({ "gs", "!gs", "getstatus", "!getstatus" }, [](const vector<string> &) {
    initiate_sending_rcon_status_command_now();
  });

  main_app.add_command_handler({ "getinfo" }, [](const vector<string> &) {
    string rcon_reply;
    main_app.get_connection_manager().send_and_receive_non_rcon_data("getinfo", rcon_reply, main_app.get_current_game_server().get_server_ip_address().c_str(), main_app.get_current_game_server().get_server_port(), main_app.get_current_game_server(), true, true);
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

  main_app.add_command_handler({ "tempbans", "!tempbans" }, [](const vector<string> &user_cmd) {
    int number{};
    if ((user_cmd.size() >= 2U) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all")) {
      number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos : static_cast<size_t>(number));
    } else {
      number_of_entries_to_display.store(25);
    }
    const string user_command_to_send{ format("!tempbans {}", number_of_entries_to_display.load()) };
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command_to_send, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "bans", "!bans" }, [](const vector<string> &user_cmd) {
    int number{};
    if ((user_cmd.size() >= 2U) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all")) {
      number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos : static_cast<size_t>(number));
    } else {
      number_of_entries_to_display.store(25);
    }
    const string user_command_to_send{ format("!bans {}", number_of_entries_to_display.load()) };
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command_to_send, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "ranges", "!ranges" }, [](const vector<string> &user_cmd) {
    int number{};
    if ((user_cmd.size() >= 2U) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all")) {
      number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos : static_cast<size_t>(number));
    } else {
      number_of_entries_to_display.store(25);
    }
    const string user_command_to_send{ format("!ranges {}", number_of_entries_to_display.load()) };
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command_to_send, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "names", "!names" }, [](const vector<string> &user_cmd) {
    int number{};
    if ((user_cmd.size() >= 2U) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all")) {
      number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos : static_cast<size_t>(number));
    } else {
      number_of_entries_to_display.store(25);
    }
    const string user_command_to_send{ format("!names {}", number_of_entries_to_display.load()) };
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command_to_send, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "admins", "!admins" }, [](const vector<string> &) {
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!admins", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });


  main_app.add_command_handler({ "!bannedcities" }, [](const vector<string> &) {
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!bannedcities", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!bannedcountries" }, [](const vector<string> &) {
    main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!bannedcountries", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!banned" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() == 2) {
      if (user_cmd[1] == "cities") {
        main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!bannedcities", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      } else if (user_cmd[1] == "countries") {
        main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!bannedcountries", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      }
    }
  });

  main_app.add_command_handler({ "!s" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      main_app.get_connection_manager_for_messages().process_and_send_message("request-topplayers", format("!s\\{}", user_cmd[1]), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "tp", "!tp" }, [](const vector<string> &user_cmd) {
    const size_t number_of_top_players_to_display = [&]() -> size_t {
      if (int number{}; (user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all")) {
        return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 50 : static_cast<size_t>(number);
      }
      return 50;
    }();

    main_app.get_connection_manager_for_messages().process_and_send_message("request-topplayers", format("!tp\\{}", number_of_top_players_to_display), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "tpy", "!tpy" }, [](const vector<string> &user_cmd) {
    const size_t number_of_top_players_to_display = [&]() -> size_t {
      if (int number{}; (user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all")) {
        return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 50 : static_cast<size_t>(number);
      }
      return 50;
    }();

    main_app.get_connection_manager_for_messages().process_and_send_message("request-topplayers", format("!tpy\\{}", number_of_top_players_to_display), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "tpm", "!tpm" }, [](const vector<string> &user_cmd) {
    const size_t number_of_top_players_to_display = [&]() -> size_t {
      if (int number{}; (user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all")) {
        return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 50 : static_cast<size_t>(number);
      }
      return 50;
    }();

    main_app.get_connection_manager_for_messages().process_and_send_message("request-topplayers", format("!tpm\\{}", number_of_top_players_to_display), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "tpd", "!tpd" }, [](const vector<string> &user_cmd) {
    const size_t number_of_top_players_to_display = [&]() -> size_t {
      if (int number{}; (user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all")) {
        return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 50 : static_cast<size_t>(number);
      }
      return 50;
    }();

    main_app.get_connection_manager_for_messages().process_and_send_message("request-topplayers", format("!tpd\\{}", number_of_top_players_to_display), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_command_handler({ "!ub", "!unban" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      const string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!ubr", "!unbanrange" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", user_cmd[0], user_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      const string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "!c", "!cp" }, [](const vector<string> &user_cmd) {
    const bool use_private_slot{ user_cmd[0] == "!cp" && !main_app.get_current_game_server().get_private_slot_password().empty() };
    const string &user_input{ user_cmd[1] };
    smatch ip_port_match{};
    const string ip_port_server_address{
      (user_cmd.size() > 1 && regex_search(user_input, ip_port_match, ip_address_and_port_regex)) ? (ip_port_match[1].str() + ":"s + ip_port_match[2].str()) : (main_app.get_current_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_current_game_server().get_server_port()))
    };
    const size_t sep_pos{ ip_port_server_address.find(':') };
    const string ip_address{ ip_port_server_address.substr(0, sep_pos) };
    const uint16_t port_number{ static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1))) };
    game_server gs{};
    gs.set_server_ip_address(ip_address);
    gs.set_server_port(port_number);
    const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(gs);

    const game_name_t game_name{ result.second != game_name_t::unknown ? result.second : convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()) };

    connect_to_the_game_server(ip_port_server_address, game_name, use_private_slot, true);
  });

  main_app.add_command_handler({ "!m", "!map" }, [](const vector<string> &user_cmd) {
    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
      const string map_name{ stl::helper::trim(user_cmd[1]) };
      const string game_type{ user_cmd.size() >= 3 ? stl::helper::trim(user_cmd[2]) : "ctf" };
      const string user_command{ str_join(cbegin(user_cmd), cend(user_cmd), " ") };
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", user_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
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

  main_app.add_command_handler({ "say", "!say" }, [](const vector<string> &rcon_cmd) {
    if (rcon_cmd.size() >= 2) {
      string command{ stl::helper::str_join(rcon_cmd, " ") };
      if (!command.empty() && '!' == command[0])
        command.erase(0, 1);
      string reply;
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "tell", "!tell" }, [](const vector<string> &rcon_cmd) {
    if (rcon_cmd.size() >= 3) {
      const string player_pid{
        trim(rcon_cmd[1])
      };
      string command{ "tell "s + player_pid };
      for (size_t j{ 2 }; j < rcon_cmd.size(); ++j) {
        command.append(" ").append(rcon_cmd[j]);
      }

      string reply;
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "clientkick" }, [](const vector<string> &rcon_cmd) {
    if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", rcon_cmd[0], rcon_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      const string rcon_command{ str_join(cbegin(rcon_cmd), cend(rcon_cmd), " ") };
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", rcon_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "tempbanclient" }, [](const vector<string> &rcon_cmd) {
    if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", rcon_cmd[0], rcon_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };

      const string rcon_command{ str_join(cbegin(rcon_cmd), cend(rcon_cmd), " ") };
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", rcon_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "banclient" }, [](const vector<string> &rcon_cmd) {
    if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
      string ex_msg{ format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.", rcon_cmd[0], rcon_cmd[1]) };
      stack_trace_element ste{
        app_handles.hwnd_re_messages_data,
        std::move(ex_msg)
      };
      const string rcon_command{ str_join(cbegin(rcon_cmd), cend(rcon_cmd), " ") };
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", rcon_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "kick", "onlykick", "tempbanuser", "banuser" }, [](const vector<string> &rcon_cmd) {
    if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty()) {
      const string rcon_command{ str_join(cbegin(rcon_cmd), cend(rcon_cmd), " ") };
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", rcon_command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_command_handler({ "unknown-rcon" }, [](const vector<string> &rcon_cmd) {
    const string command{ rcon_cmd.size() > 1 ? trim(str_join(rcon_cmd, " ")) : rcon_cmd[0] };
    const string re_msg{ "^5Sending rcon command '"s + command + "' to the server.\n"s };
    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
    string reply;
    main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", command, true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  });

  main_app.add_message_handler("request-login", [](const string &, const time_t, const string &, bool) {
  });

  main_app.add_message_handler("confirm-login", [](const string &, const time_t, const string &, bool) {
  });

  main_app.add_message_handler("receive-mapnames", [](const string &, const time_t, const string &data, bool) {
    auto lines = stl::helper::str_split(data, "\n", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
    size_t custom_maps_count{};
    for (auto &line : lines) {
      trim_in_place(line);
      if (line.empty()) continue;
      auto parts = stl::helper::str_split(line, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
      if (parts.size() >= 3) {
        for (auto &part : parts) {
          trim_in_place(part);
        }
        main_app.get_available_rcon_to_full_map_names()[parts[0]] = make_pair(parts[1], parts[2]);
        main_app.get_available_full_map_to_rcon_map_names()[parts[1]] = parts[0];
        ++custom_maps_count;
      }
    }

    const string received_mapnames_message{ format("^5Received information about ^1{} ^5custom maps.\n", custom_maps_count) };
    print_colored_text(app_handles.hwnd_re_messages_data, received_mapnames_message.c_str());

    ComboBox_ResetContent(app_handles.hwnd_combo_box_map);
    for (const auto &[rcon_map_name, full_map_name] : main_app.get_available_rcon_to_full_map_names()) {
      ComboBox_AddString(app_handles.hwnd_combo_box_map, full_map_name.first.c_str());
    }
    if (main_app.get_available_rcon_to_full_map_names().contains("mp_toujane")) {
      SendMessage(app_handles.hwnd_combo_box_map, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(main_app.get_available_rcon_to_full_map_names().at("mp_toujane").first.c_str()));
    }

    // SendMessage(app_handles.hwnd_combo_box_gametype, CB_SELECTSTRING, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>("ctf"));

    main_app.set_is_custom_map_names_message_received(true);
  });

  // receive-imagesdata
  main_app.add_message_handler("receive-imagesdata", [](const string &, const time_t, const string &data, bool) {
    auto task = std::async(std::launch::async, [d = data]() {
      auto parts = stl::helper::str_split(d, "\n", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
      for (auto &part : parts) {
        stl::helper::trim_in_place(part);
        auto image_file_info_parts = stl::helper::str_split(part, ",", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
        if (image_file_info_parts.size() >= 2) {
          stl::helper::trim_in_place(image_file_info_parts[0]);
          stl::helper::trim_in_place(image_file_info_parts[1]);
          const string image_file_absolute_path{ format("{}{}", main_app.get_current_working_directory(), image_file_info_parts[0]) };
          const bool is_download_file = [&]() {
            if (!check_if_file_path_exists(image_file_absolute_path.c_str())) return true;
            const auto image_file_md5 = calculate_md5_checksum_of_file(image_file_absolute_path.c_str());
            return image_file_md5 != image_file_info_parts[1];
          }();

          if (is_download_file) {
            string ftp_download_link{ format("ftp://{}/{}/{}", main_app.get_ftp_download_site_ip_address(), main_app.get_ftp_download_folder_path(), image_file_info_parts[0]) };
            replace_backward_slash_with_forward_slash(ftp_download_link);
            const string image_file_name{ image_file_info_parts[0].substr(image_file_info_parts[0].rfind('/') + 1) };
            const string information_before_download{ format("^3Starting to download missing map image file: ^5{}", image_file_name) };
            print_colored_text(app_handles.hwnd_re_messages_data, information_before_download.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true, true);
            if (main_app.get_auto_update_manager().download_file(ftp_download_link.c_str(), image_file_absolute_path.c_str())) {
              const string information_after_download{ format("^2Finished downloading missing map image file: ^5{}", image_file_name) };
              print_colored_text(app_handles.hwnd_re_messages_data, information_after_download.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::yes, true, true);
              this_thread::sleep_for(50ms);
            }
          }
        }
      }
    });
  });

  main_app.add_message_handler("receive-welcome-message", [](const string &, const time_t, const string &data, bool) {
    string message_to_display{ data };
    size_t start_pos{ message_to_display.find("^1admin ") };
    if (start_pos != string::npos) {
      message_to_display.replace(start_pos, len("^1admin "), "^1private admin ");
    }

    start_pos = message_to_display.find("Marcus");
    if (start_pos != string::npos) {
      message_to_display.replace(start_pos, len("Marcus"), format("{}", main_app.get_username()));
    }

    print_colored_text(app_handles.hwnd_re_messages_data, message_to_display.c_str());
  });

  // main_app.add_message_handler("rcon-heartbeat", [](const string &, const time_t, const string &, bool) {
  //   const auto current_ts{ get_current_time_stamp() };
  //   main_app.get_connection_manager_for_messages().process_and_send_message("rcon-heartbeat", format("{}\\{}", main_app.get_username(), current_ts), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
  // });

  main_app.add_message_handler("public-message", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    // for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 2) {
      const string public_msg{ format("^7{} ^7sent all admins a ^1public message:\n^5Public message: ^7{}\n", parts[0], parts[1]) };
      print_colored_text(app_handles.hwnd_re_messages_data, public_msg.c_str());
    }
  });

  main_app.add_message_handler("private-message", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    // for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {
      const string private_msg{ format("^7{} ^7sent you a ^1private message.\n^5Private message: ^7{}\n", parts[0], parts[2]) };
      print_colored_text(app_handles.hwnd_re_messages_data, private_msg.c_str());
    }
  });

  main_app.add_message_handler("inform-login", [](const string &, const time_t, const string &, bool) {
    // print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
  });

  main_app.add_message_handler("inform-logout", [](const string &, const time_t, const string &, bool) {
    // print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
  });

  main_app.add_message_handler("request-logout", [](const string &, const time_t, const string &, bool) {
    const string message{ format("^1Private admin ^7{} ^7has sent a ^1logout request ^7to ^1private ^5Tiny^6Rcon ^7server.", main_app.get_username()) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
  });

  main_app.add_message_handler("confirm-logout", [](const string &, const time_t, const string &, bool) {
    auto &current_user = main_app.get_remote_user_for_name(main_app.get_username(), main_app.get_user_ip_address());
    current_user->is_logged_in = false;
    current_user->last_logout_time_stamp = get_current_time_stamp();
    const string message{ format("^1Private admin ^7{} ^3has ^1logged out ^3of ^1private ^5Tiny^6Rcon ^3server.\n^3Number of logins: ^1{}", current_user->user_name, current_user->no_of_logins) };
    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
  });

  main_app.add_message_handler("request-admindata", [](const string &, const time_t, const string &, bool) {
    main_app.get_is_user_data_received().clear();
  });

  main_app.add_message_handler("receive-admindata", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 17) {
      auto &u = main_app.get_remote_user_for_name(parts[0], parts[4]);
      main_app.set_is_user_data_received_for_user(parts[0], parts[4]);
      u->user_name = std::move(parts[0]);
      u->is_admin = parts[1] == "true";
      u->is_logged_in = parts[2] == "true";
      unsigned long guid{};
      if (check_ip_address_validity(parts[4], guid)) {
        player admin{};
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
      if (parts.size() >= 18) {
        u->no_of_namebans = stoul(parts[17]);
      }
    }
  });

  main_app.add_message_handler("inform-message", [](const string &, const time_t, const string &data, bool) {
    print_colored_text(app_handles.hwnd_re_messages_data, data.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no, false, true);
  });


  main_app.add_message_handler("restart-tinyrcon", [](const string &, const time_t, const string &, bool) {
    const string info_message{ "^5Tiny^6Rcon ^5server ^3has sent a ^1request ^3to restart your ^5Tiny^6Rcon ^3program.\n" };
    print_colored_text(app_handles.hwnd_re_messages_data, info_message.c_str());
    restart_tinyrcon_client();
  });

  main_app.add_message_handler("rcon-heartbeat", [](const string &, const time_t, const string &data, bool) {
    auto parts = stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
    for (auto &part : parts) stl::helper::trim_in_place(part);
    if (parts.size() >= 3) {
      if (unsigned long guid_key{}; check_ip_address_validity(parts[2], guid_key)) {
        player pd{};
        me->ip_address = parts[2];
        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), me->ip_address, pd);
        me->geo_information = format("{}, {}", pd.country_name, pd.city);
        me->country_code = pd.country_code;
      }
      me->is_logged_in = true;
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-heartbeat", format("{}\\{}\\{}", main_app.get_username(), get_current_time_stamp(), me->ip_address), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }
  });

  main_app.add_message_handler("rcon-reply", [](const string &, const time_t, const string &data, bool) {
    string received_rcon_reply{ data };
    main_app.get_connection_manager().receive_rcon_reply_from_server(main_app.get_current_game_server(), received_rcon_reply);
  });

  main_app.add_message_handler("rcon-online-admins-info", [](const string &, const time_t, const string &data, bool) {
    online_admins_information = data;
    /*Edit_SetText(app_handles.hwnd_online_admins_information, "");
    print_colored_text(app_handles.hwnd_online_admins_information, data.c_str(), is_append_message_to_richedit_control::yes, is_log_message::no, is_log_datetime::no);
    SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
    SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE, (WPARAM)2, (LPARAM)NULL);*/
  });

  const string program_title{ "Private remote TinyRcon client v2 | "s + main_app.get_current_game_server().get_server_name() + " | "s + "version: "s + program_version };
  main_app.set_program_title(program_title);
  SetWindowText(app_handles.hwnd_main_window, program_title.c_str());

  main_app.set_command_line_info(user_help_message);

  CenterWindow(app_handles.hwnd_main_window);

  SetFocus(app_handles.hwnd_main_window);
  PostMessage(app_handles.hwnd_progress_bar, PBM_SETMARQUEE, (WPARAM)TRUE, (LPARAM)5);

  std::jthread task_thread{
    [&](stop_token st) {
      IsGUIThread(TRUE);

      SendMessageA(app_handles.hwnd_main_window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
      ShowWindow(app_handles.hwnd_main_window, SW_MAXIMIZE);
      UpdateWindow(app_handles.hwnd_main_window);

      main_app.get_auto_update_manager().check_for_updates(main_app.get_exe_file_path());

      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started importing geological data from ^1'geo.dat' ^3file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      // const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\IP2LOCATION-LITE-DB3.CSV" };
      // parse_geodata_lite_csv_file(geo_dat_file_path.c_str());
      const string geo_dat_file_path{ main_app.get_current_working_directory() + "plugins\\geoIP\\geo.dat" };
      import_geoip_data(main_app.get_connection_manager().get_geoip_data(), geo_dat_file_path.c_str());
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished importing geological data from ^1'geo.dat' ^2file.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

      unsigned long guid{};
      if (check_ip_address_validity(me->ip_address, guid)) {
        print_colored_text(app_handles.hwnd_re_messages_data, format("^5Your current external IP address is ^1{}\n", me->ip_address).c_str());
        player pl{};
        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), me->ip_address, pl);
        me->country_code = pl.country_code;
        me->geo_information = format("{}, {}", pl.country_name, pl.city);
      } else {
        me->ip_address = "n/a";
        me->geo_information = "n/a";
        me->country_code = "xy";
      }

      me->is_admin = true;

      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-heartbeat", format("{}\\{}\\{}", main_app.get_username(), get_current_time_stamp(), me->ip_address), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      this_thread::sleep_for(20ms);
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", "rcon status", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      load_current_map_image(main_app.get_current_game_server().get_current_map());

      string game_version_number{ "1.0" };

      main_app.set_player_name(find_users_player_name_for_installed_cod2_game(me));
      try {

        game_version_number = find_version_of_installed_cod2_game();
        main_app.set_player_name(find_users_player_name_for_installed_cod2_game(me));
        main_app.get_current_game_server().set_game_version_number(game_version_number);
        main_app.set_game_version_number(game_version_number);

      } catch (...) {
        main_app.get_current_game_server().set_game_version_number(game_version_number);
        main_app.set_game_version_number(game_version_number);
      }


      is_main_window_constructed = true;
      auto &game_servers = main_app.get_game_servers();
      main_app.set_game_server_index(0);
      string rcon_reply;

      main_app.get_bitmap_image_handler().load_bitmap_images();
      check_if_exists_and_download_missing_custom_map_files_downloader();

      while (!st.stop_requested() && !is_terminate_program.load()) {
        try {

          {
            unique_lock ul{ main_app.get_command_queue_mutex() };
            main_app.get_command_queue_cv().wait_for(ul, 20ms, [&]() {
              return st.stop_requested() || is_terminate_program.load() || !main_app.is_command_queue_empty();
            });
          }

          while (!st.stop_requested() && !is_terminate_program.load() && !main_app.is_command_queue_empty()) {
            auto cmd = main_app.get_command_from_queue();
            main_app.process_queue_command(std::move(cmd));
          }

          if (!st.stop_requested() && !is_terminate_program.load() && is_refresh_players_data_event.load()) {

            const size_t game_server_index{ main_app.get_game_server_index() };

            if (game_server_index >= main_app.get_game_servers_count()) {
              main_app.set_game_server_index(0U);
            }
            const auto current_ts = get_current_time_stamp();
            const auto time_elapsed_in_sec{ current_ts - main_app.get_connection_manager_for_messages().get_last_rcon_status_received() };
            if (game_server_index < main_app.get_rcon_game_servers_count() && time_elapsed_in_sec >= 10) {
              game_server &gs = game_servers[game_server_index];
              main_app.get_connection_manager_for_messages().process_and_send_message("rcon-heartbeat", format("{}\\{}\\{}", main_app.get_username(), current_ts, me->ip_address), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
              this_thread::sleep_for(20ms);
              main_app.get_connection_manager_for_messages().process_and_send_message("rcon-command", "rcon status", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
              this_thread::sleep_for(20ms);
              main_app.get_connection_manager().send_and_receive_non_rcon_data("getstatus", rcon_reply, gs.get_server_ip_address().c_str(), gs.get_server_port(), gs, true, true);
            } else if (game_server_index >= main_app.get_rcon_game_servers_count() && game_server_index < main_app.get_game_servers_count()) {
              game_server &gs = game_servers[game_server_index];
              main_app.get_connection_manager().send_and_receive_non_rcon_data("getstatus", rcon_reply, gs.get_server_ip_address().c_str(), gs.get_server_port(), gs, true, true);
            }

            is_refresh_players_data_event.store(false);
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

  std::jthread remote_messaging_thread{
    [&](stop_token st) {
      const auto &tiny_rcon_server_ip = main_app.get_tiny_rcon_server_ip_address();
      const auto tiny_rcon_server_port = static_cast<uint_least16_t>(main_app.get_tiny_rcon_server_port());

      while (!st.stop_requested() && !is_terminate_program.load()) {

        try {

          while (!st.stop_requested() && !is_terminate_program.load() && !main_app.is_message_queue_empty()) {
            message_t message{ main_app.get_message_from_queue() };
            const bool is_call_message_handler{ message.command != "inform-message" && message.command != "public-message" };
            main_app.get_connection_manager_for_messages().process_and_send_message(message.command, message.data, message.is_show_in_messages, tiny_rcon_server_ip, tiny_rcon_server_port, is_call_message_handler);
          }

          if (!st.stop_requested() && !is_terminate_program.load()) {
            main_app.get_connection_manager_for_messages().wait_for_and_process_response_message(tiny_rcon_server_ip, tiny_rcon_server_port);
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
        Sleep(5);
      }
    }
  };

  HHOOK hHook{ SetWindowsHookEx(WH_KEYBOARD_LL, monitor_game_key_press_events, GetModuleHandle(nullptr), 0) };

  SetTimer(app_handles.hwnd_main_window, ID_TIMER, 1000, nullptr);
  MSG win32_msg{};

  try {

    while (GetMessage(&win32_msg, nullptr, 0, 0) > 0) {
      if (TranslateAccelerator(app_handles.hwnd_main_window, hAccel, &win32_msg) != 0) {
        TranslateMessage(&win32_msg);
        DispatchMessage(&win32_msg);
      } else if (IsDialogMessage(app_handles.hwnd_main_window, &win32_msg) == 0) {
        TranslateMessage(&win32_msg);
        if (win32_msg.message == WM_KEYDOWN) {
          process_key_down_message(win32_msg);
        } else if (app_handles.hwnd_e_user_input == win32_msg.hwnd && WM_LBUTTONDOWN == win32_msg.message) {
          const int x{ GET_X_LPARAM(win32_msg.lParam) };
          const int y{ GET_Y_LPARAM(win32_msg.lParam) };
          RECT rect{};
          GetClientRect(app_handles.hwnd_e_user_input, &rect);
          if (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom) {
            SetFocus(app_handles.hwnd_e_user_input);
            if (is_first_left_mouse_button_click_in_prompt_edit_control) {
              Edit_SetText(app_handles.hwnd_e_user_input, "");
              is_first_left_mouse_button_click_in_prompt_edit_control = false;
            }
          }
        }

        DispatchMessage(&win32_msg);
      } else if (win32_msg.message == WM_KEYDOWN) {
        process_key_down_message(win32_msg);
      } else if (app_handles.hwnd_e_user_input == win32_msg.hwnd && WM_LBUTTONDOWN == win32_msg.message) {
        const int x{ GET_X_LPARAM(win32_msg.lParam) };
        const int y{ GET_Y_LPARAM(win32_msg.lParam) };
        RECT rect{};
        GetClientRect(app_handles.hwnd_e_user_input, &rect);
        if (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom) {
          SetFocus(app_handles.hwnd_e_user_input);
          if (is_first_left_mouse_button_click_in_prompt_edit_control) {
            Edit_SetText(app_handles.hwnd_e_user_input, "");
            is_first_left_mouse_button_click_in_prompt_edit_control = false;
          }
        }
      }


      if (is_main_window_constructed && !is_tinyrcon_initialized) {

        ifstream temp_messages_file{ "log\\temporary_message.log" };
        if (temp_messages_file) {
          for (string line; getline(temp_messages_file, line);) {
            print_colored_text(app_handles.hwnd_re_messages_data, line.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
          }
          temp_messages_file.close();
        }

        STARTUPINFO si{};
        PROCESS_INFORMATION pi{};
        char command_to_run[256];
        strcpy_s(command_to_run, std::size(command_to_run), "cmd.exe /C ping -n 1 -4 cod2master.activision.com > resolved_hostname.log");
        CreateProcessA(nullptr, command_to_run, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (si.hStdError != nullptr) {
          CloseHandle(si.hStdError);
        }
        if (si.hStdInput != nullptr) {
          CloseHandle(si.hStdInput);
        }
        if (si.hStdOutput != nullptr) {
          CloseHandle(si.hStdOutput);
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        const std::regex player_ip_address_regex{ R"(\[(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})\])" };
        smatch matches;
        ifstream input_file{ "resolved_hostname.log" };
        for (string line; getline(input_file, line);) {
          if (regex_search(line, matches, player_ip_address_regex)) {
            main_app.set_cod2_master_server_ip_address(matches[1].str());
            break;
          }
        }

        input_file.close();

        is_tinyrcon_initialized = true;

        const auto current_ts{ get_current_time_stamp() };
        const auto user_details{ format(R"({}\{}\{})", me->user_name, me->ip_address, current_ts) };

        main_app.add_message_to_queue(message_t("request-login", format(R"({}\{}\{}\{}\{})", me->user_name, me->ip_address, current_ts, main_app.get_player_name(), main_app.get_game_version_number()), true));

        main_app.add_message_to_queue(message_t("request-mapnames-player", user_details, true));

        main_app.add_message_to_queue(message_t("request-imagesdata", user_details, true));

        PostMessage(app_handles.hwnd_progress_bar, PBM_SETMARQUEE, (WPARAM)FALSE, (LPARAM)0);
        auto progress_bar_style = GetWindowStyle(app_handles.hwnd_progress_bar);
        progress_bar_style = progress_bar_style & ~PBS_MARQUEE;
        progress_bar_style = progress_bar_style | PBS_SMOOTH;
        SetWindowLong(app_handles.hwnd_progress_bar, -16, progress_bar_style);
        SendMessage(app_handles.hwnd_progress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, main_app.get_check_for_banned_players_time_period()));
        SendMessage(app_handles.hwnd_progress_bar, PBM_SETSTEP, 1, 0);
        SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, 0, 0);
      }
    }

    is_terminate_program.store(true);
    print_messages_thread.request_stop();
    task_thread.request_stop();
    remote_messaging_thread.request_stop();

    log_message("Exiting TinyRcon program.", is_log_datetime::yes);

    DestroyAcceleratorTable(hAccel);

    if (wcex.hbrBackground != nullptr)
      DeleteObject((HGDIOBJ)wcex.hbrBackground);
    if (red_brush != nullptr)
      DeleteBrush(red_brush);
    if (black_brush != nullptr)
      DeleteBrush(black_brush);
    if (hImageList)
      ImageList_Destroy(hImageList);
    UnregisterClass(wcex.lpszClassName, app_handles.hInstance);
    UnregisterClass(wcex_confirmation_dialog.lpszClassName, app_handles.hInstance);
    UnregisterClass(wcex_configuration_dialog.lpszClassName, app_handles.hInstance);

  } catch (std::exception &ex) {
    const string error_message{ format("^3A specific exception was caught in WinMain's thread of execution!\n^1Exception: {}", ex.what()) };
    print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
    if (hHook)
      UnhookWindowsHookEx(hHook);
  } catch (...) {
    char buffer[512];
    strerror_s(buffer, GetLastError());
    const string error_message{ format("^3A generic error was caught in WinMain's thread of execution\n^1Exception: {}", buffer) };
    print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
    if (hHook)
      UnhookWindowsHookEx(hHook);
  }

  if (hHook)
    UnhookWindowsHookEx(hHook);
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
    MessageBoxA(nullptr, error_msg, "Window Class Failed", MB_ICONERROR);
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
    MessageBoxA(nullptr, error_msg, "Window Class Failed", MB_ICONERROR);
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
    MessageBoxA(nullptr, error_msg, "Window Class Failed", MB_ICONERROR);
  }

  return status;
}

bool initialize_main_app(HINSTANCE hInstance, const int)
{
  app_handles.hInstance = hInstance;

  // InitSimpleGrid(app_handles.hInstance);

  INITCOMMONCONTROLSEX icex{};

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_PROGRESS_CLASS;
  InitCommonControlsEx(&icex);

  /*font_for_players_grid_data = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_DECORATIVE, "Lucida Console");*/
  // RECT desktop_work_area{};
  // SystemParametersInfoA(SPI_GETWORKAREA, 0, &desktop_work_area, 0);
  // AdjustWindowRectEx(&desktop_work_area, WS_OVERLAPPEDWINDOW /*| WS_HSCROLL | WS_VSCROLL*/, FALSE, 0);

  app_handles.hwnd_main_window = CreateWindowExA(WS_EX_OVERLAPPEDWINDOW, wcex.lpszClassName, "TinyRcon client", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN /*| WS_HSCROLL | WS_VSCROLL*/, 0, 0, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, nullptr, nullptr, hInstance, nullptr);

  // app_handles.hwnd_main_window = CreateWindowEx(0, wcex.lpszClassName, "TinyRcon client", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME /*WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL*/, 0, 0, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top, nullptr, nullptr, hInstance, nullptr);

  if (!app_handles.hwnd_main_window)
    return false;

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
  static char report_player_command[128]{};
  static char spectate_player_command[128]{};
  static char info_player_command[128]{};
  static char warn_player_command[128]{};
  static char mute_player_command[128]{};
  static char unmute_player_command[128]{};
  static char kick_player_command[128]{};
  static char tempban_player_command[128]{};
  static char guidban_player_command[128]{};
  static char ipban_player_command[128]{};
  static char iprangeban_player_command[128]{};
  static char city_ban_player_command[128]{};
  static char country_ban_player_command[128]{};
  static char name_ban_command[128]{};
  HBRUSH orig_textEditBrush{};
  HBRUSH comboBrush1{}, comboBrush2{}, comboBrush3{}, comboBrush4{}, comboBrush5{};
  HBRUSH defaultbrush{};
  HBRUSH hotbrush{};
  HBRUSH selectbrush{};
  HMENU hPopupMenu;
  PAINTSTRUCT ps;
  static int counter{};
  HDC hdcMem;
  HBITMAP hbmMem;
  HANDLE hOld;
  HDC hdc;
  BITMAP bitmap;
  HGDIOBJ oldBitmap;

  static HPEN red_pen{};
  static HPEN light_blue_pen{};

  switch (message) {

  case WM_CONTEXTMENU: {
    if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_re_messages_data) {
      SetFocus(app_handles.hwnd_re_messages_data);
      hPopupMenu = CreatePopupMenu();
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWMUTEDPLAYERS, "View muted players");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWREPORTEDPLAYERS, "View reported players");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWTEMPBANSBUTTON, "View temporarily banned IP addresses");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPBANSBUTTON, "View permanently banned IP addresses");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPRANGEBANSBUTTON, "View banned IP address ranges");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDPLAYERNAMES, "View banned player names");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCITIES, "View banned cities");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCOUNTRIES, "View banned countries");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSES, "View protected IP addresses");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSRANGES, "View protected IP address ranges");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCITIES, "View protected cities");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCOUNTRIES, "View protected countries");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWADMINSDATA, "View &admins' data");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, IDC_COPY, "&Copy (Ctrl + C)");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "C&lear messages");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    } else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_players_grid) {
      SetFocus(app_handles.hwnd_players_grid);
      hPopupMenu = CreatePopupMenu();
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        if (!is_player_being_spectated.load()) {
          (void)snprintf(spectate_player_command, std::size(spectate_player_command), "Spectate player (Name: %s | PID: %d)", selected_player.player_name, pid);
        } else {
          (void)snprintf(spectate_player_command, std::size(spectate_player_command), "Stop spectating player");
        }
        remove_all_color_codes(spectate_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SPECTATEPLAYER, spectate_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
        (void)snprintf(report_player_command, std::size(report_player_command), "Report player with reason (Name: %s | PID: %d)", selected_player.player_name, pid);
        remove_all_color_codes(report_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_REPORTPLAYER, report_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
        (void)snprintf(info_player_command, std::size(info_player_command), "Display information about player (Name: %s | PID: %d)", selected_player.player_name, pid);
        remove_all_color_codes(info_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_PRINTPLAYERINFORMATION_ACTION, info_player_command);
        (void)snprintf(warn_player_command, std::size(warn_player_command), "Warn player (Name: %s | PID: %d)", selected_player.player_name, pid);
        remove_all_color_codes(warn_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_WARNBUTTON, warn_player_command);
        // =====================================
        // Mute player menu command
        (void)snprintf(mute_player_command, std::size(mute_player_command), "Mute %s (PID: %d | IP address: %s)?", selected_player.player_name, pid, selected_player.ip_address.c_str());
        remove_all_color_codes(mute_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_MUTE_PLAYER, mute_player_command);
        // Unmute player menu command
        (void)snprintf(unmute_player_command, std::size(unmute_player_command), "Unmute %s (PID: %d | IP address: %s)?", selected_player.player_name, pid, selected_player.ip_address.c_str());
        remove_all_color_codes(unmute_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_UNMUTE_PLAYER, unmute_player_command);
        // =====================================
        (void)snprintf(kick_player_command, std::size(kick_player_command), "Kick player (Name: %s | PID: %d)", selected_player.player_name, pid);
        remove_all_color_codes(kick_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_KICKBUTTON, kick_player_command);
        (void)snprintf(tempban_player_command, std::size(tempban_player_command), "Temporarily ban player's IP address (Name: %s | PID: %d)", selected_player.player_name, pid);
        remove_all_color_codes(tempban_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TEMPBANBUTTON, tempban_player_command);
        if (strcmp(selected_player.guid_key, "0") != 0) {
          (void)snprintf(guidban_player_command, std::size(guidban_player_command), "Ban player's GUID key (Name: %s | PID: %d | GUID: %s)", selected_player.player_name, pid, selected_player.guid_key);
          remove_all_color_codes(guidban_player_command);
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_GUIDBANBUTTON, guidban_player_command);
        }
        (void)snprintf(ipban_player_command, std::size(ipban_player_command), "Ban player's IP address (Name: %s | PID: %d)", selected_player.player_name, pid);
        remove_all_color_codes(ipban_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_IPBANBUTTON, ipban_player_command);
        const string ip_address_range{ get_narrow_ip_address_range_for_specified_ip_address(selected_player.ip_address) };
        (void)snprintf(iprangeban_player_command, std::size(iprangeban_player_command), "Ban player's IP address range (Name: %s | PID: %d | IP address range: %s)", selected_player.player_name, pid, ip_address_range.c_str());
        remove_all_color_codes(iprangeban_player_command);
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_IPRANGEBANBUTTON, iprangeban_player_command);
        if (main_app.get_is_automatic_city_kick_enabled()) {
          (void)snprintf(city_ban_player_command, std::size(city_ban_player_command), "Ban player's city (Name: %s | City: %s)", selected_player.player_name, selected_player.city);
          remove_all_color_codes(city_ban_player_command);
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CITYBANBUTTON, city_ban_player_command);
        }
        if (main_app.get_is_automatic_country_kick_enabled()) {
          (void)snprintf(country_ban_player_command, std::size(country_ban_player_command), "Ban player's country (Name: %s | Country: %s)", selected_player.player_name, selected_player.country_name);
          remove_all_color_codes(country_ban_player_command);
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_COUNTRYBANBUTTON, country_ban_player_command);
        }

        string player_name{ selected_player.player_name };
        remove_all_color_codes(player_name);
        trim_in_place(player_name);
        to_lower_case_in_place(player_name);
        const bool is_player_name_valid{ !player_name.empty() };
        if (is_player_name_valid) {
          (void)snprintf(name_ban_command, std::size(name_ban_command), "Ban player name: %s (PID: %d)?", player_name.c_str(), pid);
          remove_all_color_codes(name_ban_command);
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_BANNAMEBUTTON, name_ban_command);
        }

        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      }
      // }

      if (main_app.get_is_connection_settings_valid()) {
        if (!main_app.get_is_automatic_city_kick_enabled()) {
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_ENABLECITYBANBUTTON, "Enable city ban feature");
        } else {
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_DISABLECITYBANBUTTON, "Disable city ban feature");
        }

        if (!main_app.get_is_automatic_country_kick_enabled()) {
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_ENABLECOUNTRYBANBUTTON, "Enable country ban feature");
        } else {
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_DISABLECOUNTRYBANBUTTON, "Disable country ban feature");
        }

        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      }

      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWMUTEDPLAYERS, "View muted players");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWREPORTEDPLAYERS, "View reported players");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWTEMPBANSBUTTON, "View temporarily banned IP addresses");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPBANSBUTTON, "View banned IP addresses");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPRANGEBANSBUTTON, "View banned IP address ranges");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDPLAYERNAMES, "View banned player names");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCITIES, "View banned cities");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCOUNTRIES, "View banned countries");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSES, "View protected IP addresses");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSRANGES, "View protected IP address ranges");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCITIES, "View protected cities");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCOUNTRIES, "View protected countries");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      if (main_app.get_is_connection_settings_valid()) {
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWADMINSDATA, "View &admins' data");
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      }
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_REFRESH_PLAYERS_DATA_BUTTON, "Refresh players' data");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      // if (main_app.get_current_game_server().get_number_of_players() > 0)
      // {
      // 	InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_PID, "Sort players' data by 'PID'");
      // 	InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_SCORE, "Sort players' data by 'SCORE'");
      // 	InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_PING, "Sort players' data by 'PING'");
      // 	InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_NAME, "Sort players' data by 'PLAYER NAME'");
      // 	if (me->is_admin)
      // 	{
      // 		InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_IP, "Sort players' data by 'IP ADDRESS'");
      // 		InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SORT_PLAYERS_DATA_BY_GEO, "Sort players' data by 'GEOINFORMATION'");
      // 	}
      // }
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_MAP_RESTART, "Restart current map (map_restart)?");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_FAST_RESTART, "Restart current match (fast_restart)?");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CONNECTBUTTON, "Join the game server");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CONNECTPRIVATESLOTBUTTON, "Join the game server using a private slot");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "C&lear messages");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    } /*else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_servers_grid) {
      SetFocus(app_handles.hwnd_servers_grid);
      hPopupMenu = CreatePopupMenu();
      if (check_if_selected_cell_indices_are_valid_for_game_servers_grid(selected_server_row, 2)) {
        string selected_server_name{ GetCellContents(app_handles.hwnd_servers_grid, selected_server_row, 1) };
        if (selected_server_name.length() >= 2 && '^' == selected_server_name[0] && is_decimal_digit(selected_server_name[1]))
          selected_server_name.erase(0, 2);
        string selected_server_address{ GetCellContents(app_handles.hwnd_servers_grid, selected_server_row, 2) };
        if (selected_server_address.length() >= 2 && '^' == selected_server_address[0] && is_decimal_digit(selected_server_address[1]))
          selected_server_address.erase(0, 2);
        snprintf(message_buffer, std::size(message_buffer), "Connect to %s %s game server?", selected_server_address.c_str(), selected_server_name.c_str());
        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CONNECTBUTTON, message_buffer);
      }
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_REFRESHSERVERSBUTTON, "Refresh servers");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SHOWPLAYERSBUTTON, "Show players");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_RCONVIEWBUTTON, "Show rcon server");
      if (check_if_selected_cell_indices_are_valid_for_game_servers_grid(selected_server_row, 2)) {
        string selected_server_address{ GetCellContents(app_handles.hwnd_servers_grid, selected_server_row, 2) };
        if (selected_server_address.length() >= 2 && '^' == selected_server_address[0] && is_decimal_digit(selected_server_address[1]))
          selected_server_address.erase(0, 2);
        auto parts = stl::helper::str_split(selected_server_address, ":", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
        for (auto &&part : parts) stl::helper::trim_in_place(part);
        unsigned long guid{};
        int port_number{};
        if (parts.size() == 2 && check_ip_address_validity(parts[0], guid) && is_valid_decimal_whole_number(parts[1], port_number)) {
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
          InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_COPYGAMESERVERADDRESS, "Copy game server address");
        }
      }
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "C&lear messages");
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
      InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    }*/
    else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_e_user_input) {
      SetFocus(app_handles.hwnd_e_user_input);
      hPopupMenu = CreatePopupMenu();
      InsertMenu(hPopupMenu, 0, MF_BYCOMMAND | MF_STRING, IDC_PASTE, "Paste (Ctrl + V)");
      TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, hWnd, nullptr);
    }
  } break;

  case WM_TIMER: {

    main_app.update_download_and_upload_speed_statistics();

    ++atomic_counter;
    if (atomic_counter.load() > 5U)
      atomic_counter.store(0);

    SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, (WPARAM)atomic_counter.load(), 0);
    ++counter;

    if (counter % 5 == 0) {
      main_app.get_connection_manager_for_messages().process_and_send_message("rcon-heartbeat", format("{}\\{}\\{}", main_app.get_username(), get_current_time_stamp(), me->ip_address), true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    }

    if (counter % 30 == 0) {
      counter = 0;
      const auto current_ts{ get_current_time_stamp() };
      if (main_app.get_is_connection_settings_valid()) {
        main_app.add_message_to_queue(message_t{ "request-admindata", format("{}\\{}\\{}", me->user_name, me->ip_address, current_ts), true });
        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
      }
    }

    if (5U == atomic_counter.load()) {
      if (is_first_left_mouse_button_click_in_prompt_edit_control) {
        Edit_SetText(app_handles.hwnd_e_user_input, "");
        is_first_left_mouse_button_click_in_prompt_edit_control = false;
      }
      is_refresh_players_data_event.store(true);
    }

    if (is_refreshed_players_data_ready_event.load()) {
      load_current_map_image(main_app.get_current_game_server().get_current_map());
      display_players_data_in_players_grid(app_handles.hwnd_players_grid);
      is_refreshed_players_data_ready_event.store(false);
    }
  }

  break;

  case WM_NOTIFY: {
    static char unknown_button_label[4]{ "..." };

    auto some_item = (LPNMHDR)lParam;

    if ((some_item->idFrom == ID_WARNBUTTON || some_item->idFrom == ID_KICKBUTTON || some_item->idFrom == ID_TEMPBANBUTTON || some_item->idFrom == ID_IPBANBUTTON || some_item->idFrom == ID_VIEWTEMPBANSBUTTON || some_item->idFrom == ID_VIEWIPBANSBUTTON || some_item->idFrom == ID_VIEWADMINSDATA || some_item->idFrom == ID_CONNECTBUTTON || some_item->idFrom == ID_CONNECTPRIVATESLOTBUTTON || some_item->idFrom == ID_SAY_BUTTON || some_item->idFrom == ID_TELL_BUTTON || some_item->idFrom == ID_QUITBUTTON || some_item->idFrom == ID_LOADBUTTON || some_item->idFrom == ID_BUTTON_CONFIGURE_SERVER_SETTINGS || some_item->idFrom == ID_CLEARMESSAGESCREENBUTTON || some_item->idFrom == ID_SHOWPLAYERSBUTTON || some_item->idFrom == ID_REFRESH_PLAYERS_DATA_BUTTON || /*some_item->idFrom == ID_SHOWSERVERSBUTTON || some_item->idFrom == ID_REFRESHSERVERSBUTTON ||*/ some_item->idFrom == ID_RCONVIEWBUTTON || some_item->idFrom == ID_SPECTATEPLAYER || some_item->idFrom == ID_MUTE_PLAYER || some_item->idFrom == ID_UNMUTE_PLAYER)
        && (some_item->code == NM_CUSTOMDRAW)) {

      auto item = (LPNMCUSTOMDRAW)some_item;

      if (item->uItemState & CDIS_FOCUS) {

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
        if (button_id_to_label_text.contains(some_item->idFrom)) {
          DrawTextEx(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
        } else {
          DrawTextEx(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
        }
        return CDRF_SKIPDEFAULT;
      }
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
        if (button_id_to_label_text.contains(some_item->idFrom)) {
          DrawTextEx(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
        } else {
          DrawTextEx(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
        }
        return CDRF_SKIPDEFAULT;
      }

      if (!defaultbrush)
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
      if (button_id_to_label_text.contains(some_item->idFrom)) {
        DrawTextEx(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
      } else {
        DrawTextEx(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
      }
      return CDRF_SKIPDEFAULT;
      // return CDRF_DODEFAULT;
    }

    return CDRF_DODEFAULT;

  } break;

  case WM_COMMAND: {

    const int wmId = LOWORD(wParam);
    const int wparam_high_word = HIWORD(wParam);

    switch (wmId) {

    case IDC_COPY: {
      const auto sel_text_len = SendMessage(app_handles.hwnd_re_messages_data, EM_GETSELTEXT, NULL, (LPARAM)selected_re_text);
      selected_re_text[sel_text_len] = '\0';
      if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(app_handles.hwnd_main_window)) {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sel_text_len + 1);
        memcpy(GlobalLock(hMem), selected_re_text, sel_text_len + 1);
        GlobalUnlock(hMem);
        OpenClipboard(app_handles.hwnd_main_window);
        EmptyClipboard();
        SetClipboardData(CF_TEXT, hMem);
        CloseClipboard();
      }
      // INPUT ip;
      // ip.type = INPUT_KEYBOARD;
      // ip.ki.wScan = 0;
      // ip.ki.time = 0;
      // ip.ki.dwExtraInfo = 0;
      //// Press the "Ctrl" key
      // ip.ki.wVk = VK_CONTROL;
      // ip.ki.dwFlags = 0;// 0 for key press
      // SendInput(1, &ip, sizeof(INPUT));

      //// Press the "C" key
      // ip.ki.wVk = 'C';
      // ip.ki.dwFlags = 0;// 0 for key press
      // SendInput(1, &ip, sizeof(INPUT));

      //// Release the "C" key
      // ip.ki.wVk = 'C';
      // ip.ki.dwFlags = KEYEVENTF_KEYUP;
      // SendInput(1, &ip, sizeof(INPUT));

      //// Release the "Ctrl" key
      // ip.ki.wVk = VK_CONTROL;
      // ip.ki.dwFlags = KEYEVENTF_KEYUP;
      // SendInput(1, &ip, sizeof(INPUT));
    } break;

    case IDC_PASTE: {
      // SetFocus(app_handles.hwnd_e_user_input);
      if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(app_handles.hwnd_main_window)) {

        auto hglb = GetClipboardData(CF_TEXT);
        const auto clipboard_text = reinterpret_cast<const char *>(GlobalLock(hglb));

        const auto text_len = GetWindowTextA(app_handles.hwnd_e_user_input, selected_re_text, std::size(selected_re_text));
        selected_re_text[text_len] = '\0';
        strcat_s(selected_re_text, clipboard_text);
        GlobalUnlock(hglb);
        CloseClipboard();
      }

      // INPUT ip;
      // ip.type = INPUT_KEYBOARD;
      // ip.ki.wScan = 0;
      // ip.ki.time = 0;
      // ip.ki.dwExtraInfo = 0;
      //// Press the "Ctrl" key
      // ip.ki.wVk = VK_CONTROL;
      // ip.ki.dwFlags = 0;// 0 for key press
      // SendInput(1, &ip, sizeof(INPUT));

      //// Press the "V" key
      // ip.ki.wVk = 'V';
      // ip.ki.dwFlags = 0;// 0 for key press
      // SendInput(1, &ip, sizeof(INPUT));

      //// Release the "V" key
      // ip.ki.wVk = 'V';
      // ip.ki.dwFlags = KEYEVENTF_KEYUP;
      // SendInput(1, &ip, sizeof(INPUT));

      //// Release the "Ctrl" key
      // ip.ki.wVk = VK_CONTROL;
      // ip.ki.dwFlags = KEYEVENTF_KEYUP;
      // SendInput(1, &ip, sizeof(INPUT));

    } break;

    case ID_QUITBUTTON:
      execute_at_exit();
      PostQuitMessage(0);
      // is_terminate_program.store(true);
      break;

    case ID_CLEARMESSAGESCREENBUTTON: {
      Edit_SetText(app_handles.hwnd_re_messages_data, "");
      g_message_data_contents.clear();
    } break;

    case ID_PATCHGAME_V1_0: {
      main_app.add_command_to_queue({ "!patch", "1.0" }, command_type::user, false);
    } break;

    case ID_PATCHGAME_V1_01: {
      main_app.add_command_to_queue({ "!patch", "1.01" }, command_type::user, false);
    } break;

    case ID_PATCHGAME_V1_2: {
      main_app.add_command_to_queue({ "!patch", "1.2" }, command_type::user, false);
    } break;

    case ID_PATCHGAME_V1_3: {
      main_app.add_command_to_queue({ "!patch", "1.3" }, command_type::user, false);
    } break;

      /*case ID_COPYGAMESERVERADDRESS: {
          copied_game_server_address = get_server_address_for_connect_command(selected_server_row);
        } break;*/

    case ID_CONNECTBUTTON: {
      string ip_port_server_address;
      /*if (!is_display_players_data.load()) {
        ip_port_server_address = get_server_address_for_connect_command(selected_server_row);
      } else {*/
      match_results<string::const_iterator> ip_port_match{};
      ip_port_server_address = regex_search(admin_reason, ip_port_match, ip_address_and_port_regex) ? ip_port_match[1].str() + ":"s + ip_port_match[2].str() : main_app.get_current_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_current_game_server().get_server_port());
      // }
      const size_t sep_pos{ ip_port_server_address.find(':') };
      const string ip_address{ ip_port_server_address.substr(0, sep_pos) };
      const uint16_t port_number{ static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1))) };
      game_server gs{};
      gs.set_server_ip_address(ip_address);
      gs.set_server_port(port_number);
      const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(gs);
      const game_name_t game_name{ result.second != game_name_t::unknown ? result.second : convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()) };
      connect_to_the_game_server(ip_port_server_address, game_name, false, true);
    } break;

    case ID_CONNECTPRIVATESLOTBUTTON: {

      string ip_port_server_address;
      match_results<string::const_iterator> ip_port_match{};
      ip_port_server_address = regex_search(admin_reason, ip_port_match, ip_address_and_port_regex) ? ip_port_match[1].str() + ":"s + ip_port_match[2].str() : main_app.get_current_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_current_game_server().get_server_port());
      // }
      const size_t sep_pos{ ip_port_server_address.find(':') };
      const string ip_address{ ip_port_server_address.substr(0, sep_pos) };
      const uint16_t port_number{ static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1))) };
      game_server gs{};
      gs.set_server_ip_address(ip_address);
      gs.set_server_port(port_number);
      const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(gs);

      const game_name_t game_name{ result.second != game_name_t::unknown ? result.second : convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()) };
      connect_to_the_game_server(ip_port_server_address, game_name, true, true);
    } break;

    case ID_REFRESH_PLAYERS_DATA_BUTTON: {
      if (!is_display_players_data.load()) {
        is_display_players_data.store(true);
        const string current_non_rcon_game_server_name{ main_app.get_game_servers()[main_app.get_game_server_index()].get_server_name() };
        string window_title_message{ format("Currently displayed game server: {}:{} {}", main_app.get_game_servers()[main_app.get_game_server_index()].get_server_ip_address(), main_app.get_game_servers()[main_app.get_game_server_index()].get_server_port(), current_non_rcon_game_server_name) };
        append_to_title(app_handles.hwnd_main_window, std::move(window_title_message));
      }
      initiate_sending_rcon_status_command_now();
    } break;


    case ID_LOADBUTTON: {
      char full_map[256];
      char rcon_gametype[16];
      ComboBox_GetText(app_handles.hwnd_combo_box_map, full_map, 256);
      ComboBox_GetText(app_handles.hwnd_combo_box_gametype, rcon_gametype, 16);
      const auto &full_map_names_to_rcon_map_names = get_full_map_names_to_rcon_map_names_for_specified_game_name(convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()));
      const auto &full_gametype_names = get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()));
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

    case ID_MAP_RESTART: {

      const string current_rcon_map_name{ main_app.get_current_game_server().get_current_map() };
      const auto &available_rcon_to_full_map_names = main_app.get_available_rcon_to_full_map_names();

      if (!current_rcon_map_name.empty()) {
        const string current_full_map_name{ available_rcon_to_full_map_names.contains(current_rcon_map_name) ? available_rcon_to_full_map_names.at(current_rcon_map_name).second : current_rcon_map_name };
        string current_game_type{ main_app.get_current_game_server().get_current_game_type() };
        stl::helper::to_upper_case_in_place(current_game_type);
        (void)snprintf(message_buffer, std::size(message_buffer), "^2Do you want to restart ^1(!rc map_restart) ^2current map ^3%s ^2in ^1%s ^2game type?\n", current_full_map_name.c_str(), current_game_type.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!rc map_restart", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        }
      }
    } break;

    case ID_FAST_RESTART: {

      const string current_rcon_map_name{ main_app.get_current_game_server().get_current_map() };
      const auto &available_rcon_to_full_map_names = main_app.get_available_rcon_to_full_map_names();

      if (!current_rcon_map_name.empty()) {
        const string current_full_map_name{ available_rcon_to_full_map_names.contains(current_rcon_map_name) ? available_rcon_to_full_map_names.at(current_rcon_map_name).second : current_rcon_map_name };

        string current_game_type{ main_app.get_current_game_server().get_current_game_type() };
        stl::helper::to_upper_case_in_place(current_game_type);
        (void)snprintf(message_buffer, std::size(message_buffer), "^2Do you want to restart ^1(!rc fast_restart) ^2current match ^3%s ^2in ^1%s ^2game type?\n", current_full_map_name.c_str(), current_game_type.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!rc fast_restart", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
        }
      }
    } break;

    case ID_SPECTATEPLAYER: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (!is_player_being_spectated.load()) {
        (void)snprintf(message_buffer, std::size(message_buffer), "!spec %d", pid);
      } else {
        (void)snprintf(message_buffer, std::size(message_buffer), "!nospec");
      }
      Edit_SetText(app_handles.hwnd_e_user_input, message_buffer);
      get_user_input();
    } break;


    case ID_REPORTPLAYER: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "Reported") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to report selected player?\n ^7%s", player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!report %d %s", pid, admin_reason.c_str());
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          admin_reason.assign("not specified");
          get_user_input();
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_PRINTPLAYERINFORMATION_ACTION: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "Player information printed") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^5Information about selected player:\n ^7%s\n", player_information.c_str());
        print_colored_text(app_handles.hwnd_re_messages_data, message_buffer, is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_MUTE_PLAYER: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        if (unsigned long guid_key{}; check_ip_address_validity(selected_player.ip_address, guid_key)) {
          (void)snprintf(message_buffer, std::size(message_buffer), "!mute %d", pid);
          Edit_SetText(app_handles.hwnd_e_user_input, message_buffer);
          get_user_input();
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data,
          "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data "
          "table!\n^5Please, select a non-empty, valid player's row.\n",
          is_append_message_to_richedit_control::yes,
          is_log_message::yes,
          is_log_datetime::yes);
      }
    } break;

    case ID_UNMUTE_PLAYER: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        if (unsigned long guid_key{}; check_ip_address_validity(selected_player.ip_address, guid_key)) {
          (void)snprintf(message_buffer, std::size(message_buffer), "!unmute %d", pid);
          Edit_SetText(app_handles.hwnd_e_user_input, message_buffer);
          get_user_input();
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data,
          "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data "
          "table!\n^5Please, select a non-empty, valid player's row.\n",
          is_append_message_to_richedit_control::yes,
          is_log_message::yes,
          is_log_datetime::yes);
      }
    } break;


    case ID_WARNBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "Warned") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to warn selected player?\n ^7%s", player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!w %d %s", pid, admin_reason.c_str());
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          admin_reason.assign("not specified");
          get_user_input();
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_KICKBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "Kicked") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to kick selected player?\n ^7%s", player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!k %d %s", pid, admin_reason.c_str());
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          admin_reason.assign("not specified");
          get_user_input();
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_TEMPBANBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "Temporarily banned") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to temporarily ban IP address of selected player?\n ^7%s", player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!tb %d 24 %s", pid, admin_reason.c_str());
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          get_user_input();
          admin_reason.assign("not specified");
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_GUIDBANBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "GUID banned") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban GUID key of selected player?\n ^7%s", player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!b %d %s", pid, admin_reason.c_str());
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          get_user_input();
          admin_reason.assign("not specified");
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_IPBANBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "IP address banned") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban IP address of selected player?\n ^7%s", player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!gb %d %s", pid, admin_reason.c_str());
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          get_user_input();
          admin_reason.assign("not specified");
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;


    case ID_IPRANGEBANBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "IP address range banned") };
        const string narrow_ip_address_range{ get_narrow_ip_address_range_for_specified_ip_address(selected_player.ip_address) };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban IP address range ^1%s ^3of selected player?\n ^7%s", narrow_ip_address_range.c_str(), player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!br %d %s", pid, admin_reason.c_str());
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          get_user_input();
          admin_reason.assign("not specified");
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_CITYBANBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "City banned") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban city of selected player?\n ^7%s", player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!bancity %s", selected_player.city);
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          get_user_input();
          admin_reason.assign("not specified");
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_COUNTRYBANBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "Country banned") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban country of selected player?\n ^7%s", player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!bancountry %s", selected_player.country_name);
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          get_user_input();
          admin_reason.assign("not specified");
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
      }
    } break;

    case ID_BANNAMEBUTTON: {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        string player_name{ selected_player.player_name };
        remove_all_color_codes(player_name);
        trim_in_place(player_name);
        to_lower_case_in_place(player_name);
        const string player_information{ get_player_information(pid, true, "Player name banned") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to ban name ^7%s ^3of selected player?\n ^7%s", player_name.c_str(), player_information.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "!banname pid:%d", pid);
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          get_user_input();
          admin_reason.assign("not specified");
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
      // if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col)) {
      const int pid{ get_selected_players_pid_number(app_handles.hwnd_players_grid) };
      if (pid != -1) {
        selected_player = get_player_data_for_pid(pid);
        const string player_information{ get_player_information(pid, true, "Private message sent") };
        (void)snprintf(message_buffer, std::size(message_buffer), "^2Are you sure you want to send a ^1private message ^2to selected player?\n ^7%s", player_information.c_str());
        admin_reason = "Type your private message in here...";
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action", "Message:")) {
          (void)snprintf(msg_buffer, std::size(msg_buffer), "tell %d \"%s\"", pid, admin_reason.c_str());
          Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
          get_user_input();
          admin_reason.assign("not specified");
        }
      }
      // }
    } break;

    case ID_RCONVIEWBUTTON: {
      selected_server_row = 0;
      main_app.set_game_server_index(0);
      is_display_players_data.store(true);
      // ShowWindow(app_handles.hwnd_servers_grid, SW_HIDE);
      // ShowWindow(app_handles.hwnd_players_grid, SW_SHOWNORMAL);
      string window_title_message{ format("Rcon game server: {}:{} {}", main_app.get_game_servers()[0].get_server_ip_address(), main_app.get_game_servers()[0].get_server_port(), main_app.get_game_server_name()) };
      append_to_title(app_handles.hwnd_main_window, std::move(window_title_message));
      type_of_sort = sort_type::geo_asc;
      // clear_players_data_in_players_grid(app_handles.hwnd_players_grid, 0, max_players_grid_rows, 7);
      initiate_sending_rcon_status_command_now();
    } break;

    case ID_SHOWPLAYERSBUTTON: {

      is_display_players_data.store(true);
      // ShowWindow(app_handles.hwnd_servers_grid, SW_HIDE);
      // ShowWindow(app_handles.hwnd_players_grid, SW_SHOWNORMAL);
      const string current_non_rcon_game_server_name{ main_app.get_game_servers()[main_app.get_game_server_index()].get_server_name() };
      string window_title_message{ format("Currently displayed game server: {}:{} {}", main_app.get_game_servers()[main_app.get_game_server_index()].get_server_ip_address(), main_app.get_game_servers()[main_app.get_game_server_index()].get_server_port(), current_non_rcon_game_server_name) };
      append_to_title(app_handles.hwnd_main_window, std::move(window_title_message));
      if (main_app.get_game_server_index() >= main_app.get_rcon_game_servers_count())
        type_of_sort = sort_type::score_desc;
      else
        type_of_sort = sort_type::geo_asc;
      initiate_sending_rcon_status_command_now();
    } break;

    case ID_VIEWMUTEDPLAYERS:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!muted 25", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;

    case ID_VIEWREPORTEDPLAYERS:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!reports 25", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;

    case ID_VIEWTEMPBANSBUTTON:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!tempbans 25", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;

    case ID_VIEWIPBANSBUTTON:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!bans 25", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;

    case ID_VIEWIPRANGEBANSBUTTON:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!ranges 25", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;

    case ID_VIEWBANNEDPLAYERNAMES:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!names 25", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;

    case ID_VIEWBANNEDCITIES:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!bannedcities", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;

    case ID_VIEWBANNEDCOUNTRIES:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!bannedcountries", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;

    case ID_VIEWADMINSDATA: {
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!admins", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      display_online_admins_information();
    } break;

    case ID_VIEWPROTECTEDIPADDRESSES:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!protectedipaddresses", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;
    case ID_VIEWPROTECTEDIPADDRESSRANGES:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!protectedipaddressranges", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;
    case ID_VIEWPROTECTEDCITIES:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!protectedcities", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
      break;
    case ID_VIEWPROTECTEDCOUNTRIES:
      main_app.get_connection_manager_for_messages().process_and_send_message("user-command", "!protectedcountries", true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
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
        execute_at_exit();
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

    if (wparam_high_word == BN_CLICKED) {
      switch (wmId) {
      case ID_REPORTPLAYER:
      case ID_MUTE_PLAYER:
      case ID_UNMUTE_PLAYER:
      case ID_WARNBUTTON:
      case ID_KICKBUTTON:
      case ID_TEMPBANBUTTON:
      case ID_IPBANBUTTON:
      case ID_VIEWTEMPBANSBUTTON:
      case ID_VIEWIPBANSBUTTON:
      case ID_VIEWADMINSDATA:
      case ID_RCONVIEWBUTTON:
      case ID_SHOWPLAYERSBUTTON:
      case ID_REFRESH_PLAYERS_DATA_BUTTON:
      // case ID_SHOWSERVERSBUTTON:
      // case ID_REFRESHSERVERSBUTTON:
      case ID_CONNECTBUTTON:
      case ID_CONNECTPRIVATESLOTBUTTON:
      case ID_SAY_BUTTON:
      case ID_TELL_BUTTON:
      case ID_LOADBUTTON:
      case ID_QUITBUTTON:
      case ID_YES_BUTTON:
      case ID_NO_BUTTON:
      case ID_BUTTON_SAVE_CHANGES:
      case ID_BUTTON_TEST_CONNECTION:
      case ID_BUTTON_CANCEL:
      case ID_BUTTON_CONFIGURE_SERVER_SETTINGS:
      case ID_CLEARMESSAGESCREENBUTTON:
      case ID_BUTTON_CONFIGURATION_EXIT_TINYRCON:
      case ID_BUTTON_CONFIGURATION_COD1_PATH:
      case ID_BUTTON_CONFIGURATION_COD2_PATH:
      case ID_BUTTON_CONFIGURATION_COD4_PATH:
      case ID_BUTTON_CONFIGURATION_COD5_PATH:
        SetFocus(app_handles.hwnd_main_window);
      default:
        break;
      }
    }
  } break;

    // case WM_SETCURSOR: {
    //   static HCURSOR default_cursor{ LoadCursor(nullptr, IDC_ARROW) };
    //   SetCursor(default_cursor);
    //   return TRUE;
    // }

  case WM_PAINT: {

    // Get DC for window
    hdc = BeginPaint(hWnd, &ps);

    // Create an off-screen DC for double-buffering
    hdcMem = CreateCompatibleDC(hdc);
    hbmMem = CreateCompatibleBitmap(hdc, screen_width, screen_height);

    hOld = SelectObject(hdcMem, hbmMem);

    // Draw into hdcMem here
    SetBkMode(hdcMem, OPAQUE);
    SetBkColor(hdcMem, color::black);
    SetTextColor(hdcMem, color::red);

    RECT bounding_rectangle = {
      screen_width / 2 + 170, screen_height / 2 - 28, screen_width / 2 + 210, screen_height / 2 - 8
    };
    DrawText(hdcMem, "Map:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);

    bounding_rectangle = { screen_width / 2 + 370, screen_height / 2 - 28, screen_width / 2 + 450, screen_height / 2 - 8 };
    DrawText(hdcMem, "Gametype:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);

    bounding_rectangle = {
      10,
      screen_height - 70,
      130,
      screen_height - 50
    };

    DrawText(hdcMem, prompt_message, -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT | DT_END_ELLIPSIS);

    atomic_counter.store(std::min<size_t>(atomic_counter.load(), 5U));

    // BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
    // Transfer the off-screen DC to the screen
    BitBlt(hdc, 0, 0, screen_width, screen_height, hdcMem, 0, 0, SRCCOPY);

    oldBitmap = SelectObject(hdcMem, g_hBitMap);
    GetObject(g_hBitMap, sizeof(bitmap), &bitmap);
    BitBlt(hdc, screen_width / 2 + 340, screen_height - 180, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, oldBitmap);

    // Free-up the off-screen DC
    SelectObject(hdcMem, hOld);

    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    EndPaint(hWnd, &ps);
    return 0;

  } break;

  case WM_KEYDOWN:
    if (wParam == VK_DOWN || wParam == VK_UP)
      return 0;
    break;
  case WM_CLOSE:
    // is_terminate_program.store(true);
    execute_at_exit();
    DestroyWindow(hWnd);
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
    if (hWnd == app_handles.hwnd_main_window && is_main_window_constructed) {
      return 0;
    }
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
      auto dc = (HDC)wParam;
      SetBkMode(dc, OPAQUE);
      SetTextColor(dc, color::blue);
      SetBkColor(dc, color::yellow);
      if (comboBrush1 == nullptr)
        comboBrush1 = CreateSolidBrush(color::yellow);
      return (INT_PTR)comboBrush1;
    }
    if ((HWND)lParam == info2.hwndList) {
      auto dc = (HDC)wParam;
      SetBkMode(dc, OPAQUE);
      SetTextColor(dc, /*is_focused ? color::red : */ color::blue);
      SetBkColor(dc, color::yellow);
      if (comboBrush2 == nullptr)
        comboBrush2 = CreateSolidBrush(color::yellow);
      return (INT_PTR)comboBrush2;
    }

    if ((HWND)lParam == info3.hwndList) {
      auto dc = (HDC)wParam;
      SetBkMode(dc, OPAQUE);
      SetTextColor(dc, /*is_focused ? color::red : */ color::blue);
      SetBkColor(dc, color::yellow);
      if (comboBrush5 == nullptr)
        comboBrush5 = CreateSolidBrush(color::yellow);
      return (INT_PTR)comboBrush5;
    }
  } break;

  case WM_CTLCOLOREDIT: {
    auto dc = (HDC)wParam;
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
  HBRUSH orig_textEditBrush{}, comboBrush1{}, comboBrush2{}, comboBrush3{}, comboBrush4{};
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
    static char unknown_button_label[4]{ "..." };
    auto some_item = (LPNMHDR)lParam;

    if ((some_item->idFrom == ID_YES_BUTTON || some_item->idFrom == ID_NO_BUTTON)
        && (some_item->code == NM_CUSTOMDRAW)) {
      auto item = (LPNMCUSTOMDRAW)some_item;

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
        if (button_id_to_label_text.contains(some_item->idFrom)) {
          DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        } else {
          DrawText(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        }
        return CDRF_SKIPDEFAULT;
      }
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
        if (button_id_to_label_text.contains(some_item->idFrom)) {
          DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        } else {
          DrawText(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        }
        return CDRF_SKIPDEFAULT;
      }

      if (defaultbrush == nullptr)
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
      if (button_id_to_label_text.contains(some_item->idFrom)) {
        DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
      } else {
        DrawText(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
      }
      return CDRF_SKIPDEFAULT;
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
      SetFocus(app_handles.hwnd_players_grid);
      DestroyWindow(hWnd);
      break;

    case ID_NO_BUTTON:
      admin_choice.store(0);
      admin_reason.assign("not specified");
      EnableWindow(app_handles.hwnd_main_window, TRUE);
      SetFocus(app_handles.hwnd_players_grid);
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
    SetFocus(app_handles.hwnd_players_grid);
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
    PostQuitMessage(admin_choice.load());
    return 0;

  case WM_CTLCOLOREDIT: {
    auto dc = (HDC)wParam;
    SetTextColor(dc, color::red);
    SetBkColor(dc, color::yellow);
    SetBkMode(dc, OPAQUE);
    if (!orig_textEditBrush)
      orig_textEditBrush = CreateSolidBrush(color::yellow);
    return (INT_PTR)orig_textEditBrush;
  }

  case WM_CTLCOLORSTATIC: {
    auto hEdit = (HDC)wParam;
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
    static char unknown_button_label[4]{ "..." };
    auto some_item = (LPNMHDR)lParam;

    if ((some_item->idFrom == ID_BUTTON_SAVE_CHANGES || some_item->idFrom == ID_BUTTON_TEST_CONNECTION || some_item->idFrom == ID_BUTTON_CANCEL || some_item->idFrom == ID_BUTTON_CONFIGURATION_EXIT_TINYRCON || some_item->idFrom == ID_BUTTON_CONFIGURATION_COD1_PATH || some_item->idFrom == ID_BUTTON_CONFIGURATION_COD2_PATH || some_item->idFrom == ID_BUTTON_CONFIGURATION_COD4_PATH || some_item->idFrom == ID_BUTTON_CONFIGURATION_COD5_PATH)
        && (some_item->code == NM_CUSTOMDRAW)) {
      auto item = (LPNMCUSTOMDRAW)some_item;

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
        if (button_id_to_label_text.contains(some_item->idFrom)) {
          DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        } else {
          DrawText(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        }
        return CDRF_SKIPDEFAULT;
      }
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
        if (button_id_to_label_text.contains(some_item->idFrom)) {
          DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        } else {
          DrawText(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        }
        return CDRF_SKIPDEFAULT;
      }

      if (defaultbrush == nullptr)
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
      if (button_id_to_label_text.contains(some_item->idFrom)) {
        DrawText(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
      } else {
        DrawText(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
      }
      return CDRF_SKIPDEFAULT;
    }
    return CDRF_DODEFAULT;

  } break;


  case WM_COMMAND: {
    const int wmId = LOWORD(wParam);
    switch (wmId) {

    case ID_ENABLE_CITY_BAN_CHECKBOX: {
      if (Button_GetCheck(app_handles.hwnd_enable_city_ban) == BST_UNCHECKED) {
        main_app.set_is_automatic_city_kick_enabled(true);
        Button_SetCheck(app_handles.hwnd_enable_city_ban, BST_CHECKED);
      } else {
        main_app.set_is_automatic_city_kick_enabled(false);
        Button_SetCheck(app_handles.hwnd_enable_city_ban, BST_UNCHECKED);
      }
    } break;

    case ID_ENABLE_COUNTRY_BAN_CHECKBOX: {

      if (Button_GetCheck(app_handles.hwnd_enable_country_ban) == BST_UNCHECKED) {
        main_app.set_is_automatic_country_kick_enabled(true);
        Button_SetCheck(app_handles.hwnd_enable_country_ban, BST_CHECKED);
      } else {
        main_app.set_is_automatic_country_kick_enabled(false);
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
      SetFocus(app_handles.hwnd_players_grid);
      DestroyWindow(app_handles.hwnd_configuration_dialog);
    } break;

    case ID_BUTTON_CONFIGURATION_EXIT_TINYRCON: {
      EnableWindow(app_handles.hwnd_main_window, TRUE);
      SetFocus(app_handles.hwnd_players_grid);
      // execute_at_exit();
      DestroyWindow(app_handles.hwnd_configuration_dialog);
      is_terminate_program.store(true);
    } break;

    case ID_BUTTON_CONFIGURATION_COD1_PATH: {

      str_copy(install_path, "C:\\");
      (void)snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty 1 game's installation folder (codmp.exe) and click OK.");

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
    SetFocus(app_handles.hwnd_players_grid);
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
    auto dc = (HDC)wParam;
    SetTextColor(dc, color::red);
    SetBkColor(dc, color::yellow);
    SetBkMode(dc, OPAQUE);
    if (!orig_textEditBrush)
      orig_textEditBrush = CreateSolidBrush(color::yellow);
    return (INT_PTR)orig_textEditBrush;
  }

  case WM_CTLCOLORSTATIC: {
    auto hEdit = (HDC)wParam;
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
