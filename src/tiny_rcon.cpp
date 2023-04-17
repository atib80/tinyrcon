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
using namespace std::string_literals;
using namespace std::chrono;

extern const string program_version{ "2.4" };

extern char const *const tempbans_file_path =
  "data\\tempbans.txt";

extern char const *const banned_ip_addresses_file_path =
  "data\\bans.txt";

extern const std::regex ip_address_and_port_regex;

extern volatile std::atomic<bool> is_terminate_program;
extern volatile std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window;
// volatile std::atomic<bool> is_refreshing_players_data{false};
extern string g_message_data_contents;

tiny_cod2_rcon_client_application main_app;
sort_type type_of_sort{ sort_type::pid_asc };

PROCESS_INFORMATION pr_info{};

static once_flag task_to_run_one_time{};
condition_variable exit_flag{};
mutex mu{};

volatile atomic<size_t> atomic_counter{ 0 };
volatile std::atomic<bool> is_refresh_players_data_event{ false };
volatile std::atomic<bool> is_refreshed_players_data_ready_event{ false };
volatile std::atomic<bool> is_display_permanently_banned_players_data_event{ false };
volatile std::atomic<bool> is_display_temporarily_banned_players_data_event{ false };

extern const int screen_width{ GetSystemMetrics(SM_CXSCREEN) };
extern const int screen_height{ GetSystemMetrics(SM_CYSCREEN) - 20 };
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
)";


extern const std::unordered_map<string, sort_type> sort_mode_names_dict;

extern const std::unordered_map<int, string> button_id_to_label_text{
  { ID_WARNBUTTON, "Warn" },
  { ID_KICKBUTTON, "Kick" },
  { ID_TEMPBANBUTTON, "Tempban" },
  { ID_IPBANBUTTON, "Ban IP" },
  { ID_VIEWTEMPBANSBUTTON, "View temporary bans" },
  { ID_VIEWIPBANSBUTTON, "View IP bans" },
  { ID_REFRESHDATABUTTON, "Refresh players data" },
  { ID_CONNECTBUTTON, "Join server" },
  { ID_CONNECTPRIVATESLOTBUTTON, "Join server (private slot)" },
  { ID_SAY_BUTTON, "Send public message" },
  { ID_TELL_BUTTON, "Send private message" },
  { ID_QUITBUTTON, "Exit" },
  { ID_LOADBUTTON, "Load selected map" },
  { ID_YES_BUTTON, "Yes" },
  { ID_NO_BUTTON, "No" },
  { ID_BUTTON_SAVE_CHANGES, "Save changes" },
  { ID_BUTTON_TEST_CONNECTION, "Test connection" },
  { ID_BUTTON_CANCEL, "Cancel" },
  { ID_BUTTON_CONFIGURE_SERVER_SETTINGS, "Configure server settings" },
  { ID_CLEARMESSAGESCREENBUTTON, "Clear messages screen" },
  { ID_BUTTON_CONFIGURATION_EXIT_TINYRCON, "Exit TinyRcon" },
  { ID_BUTTON_CONFIGURATION_COD1_PATH, "Browse for codmp.exe" },
  { ID_BUTTON_CONFIGURATION_COD2_PATH, "Browse for cod2mp_s.exe" },
  { ID_BUTTON_CONFIGURATION_COD4_PATH, "Browse for iw3mp.exe" },
  { ID_BUTTON_CONFIGURATION_COD5_PATH, "Browse for cod5mp.exe" }
};


unordered_map<size_t, string> rcon_status_grid_column_header_titles;
unordered_map<size_t, string> get_status_grid_column_header_titles;

static constexpr const char *prompt_message{ "Administrator >>" };
static constexpr const char *refresh_players_data_fmt_str{ "Refreshing players data in %zu seconds..." };
extern const size_t max_players_grid_rows;

tiny_rcon_handles app_handles{};

WNDCLASSEX wcex, wcex_confirmation_dialog, wcex_configuration_dialog;
HFONT hfontbody{};

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

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
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

  MyRegisterClass(hInstance);

  if (!InitInstance(hInstance, nCmdShow)) {
    return FALSE;
  }

  parse_tiny_cod2_rcon_tool_config_file("config\\tinyrcon.json");

  main_app.set_command_line_info(user_help_message);

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
  main_app.open_log_file("log\\commands_history.log");

  print_colored_text(app_handles.hwnd_re_messages_data, "^3Started parsing ^1tinyrcon.json ^3file.\n", true, true, true);
  print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished parsing ^1tinyrcon.json ^3file.\n", true, true, true);
  const string program_title{ main_app.get_program_title() + " | "s + main_app.get_game_server_name() + " | "s + "version: "s + program_version };
  SetWindowTextA(app_handles.hwnd_main_window, program_title.c_str());

  CenterWindow(app_handles.hwnd_main_window);

  MSG msg{};

  SetFocus(app_handles.hwnd_e_user_input);
  PostMessage(app_handles.hwnd_progress_bar, PBM_SETMARQUEE, (WPARAM)TRUE, (LPARAM)5);

  std::thread task_thread{
    [&]() {
      IsGUIThread(TRUE);
      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started importing serialized binary geological data from\n ^1'plugins/geoIP/geo.dat' ^3file.\n", true, true, true);
      // parse_geodata_lite_csv_file("plugins/geoIP/IP2LOCATION-LITE-DB3.csv");
      import_geoip_data(main_app.get_connection_manager().get_geoip_data(), "plugins/geoIP/geo.dat");

      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished importing serialized binary geological data from\n ^1'plugins/geoIP/geo.dat' ^2file.\n", true, true, true);

      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started parsing ^1tempbans.txt ^3file.\n", true, true, true);
      parse_tempbans_data_file();
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished parsing ^1tempbans.txt ^3file.\n", true, true, true);

      print_colored_text(app_handles.hwnd_re_messages_data, "^3Started parsing ^1banned_ip_addresses.txt ^3file.\n", true, true, true);
      parse_banned_ip_addresses_file();
      print_colored_text(app_handles.hwnd_re_messages_data, "^2Finished parsing ^1banned_ip_addresses.txt ^3file.\n", true, true, true);

      if (!initialize_and_verify_server_connection_settings()) {
        PostQuitMessage(0);
      }

      display_tempbanned_players_remaining_time_period();


      for (auto &temp_banned_player_data : main_app.get_game_server().get_tempbanned_players_to_unban()) {

        const time_t ban_expires_time = temp_banned_player_data.banned_start_time + (temp_banned_player_data.ban_duration_in_hours * 3600);
        string message{ main_app.get_automatic_remove_temp_ban_msg() };
        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = temp_banned_player_data.player_name;
        main_app.get_tinyrcon_dict()["{TEMP_BAN_START_DATE}"] = get_date_and_time_for_time_t(temp_banned_player_data.banned_start_time);
        main_app.get_tinyrcon_dict()["{TEMP_BAN_END_DATE}"] = get_date_and_time_for_time_t(ban_expires_time);
        main_app.get_tinyrcon_dict()["{REASON}"] = temp_banned_player_data.reason;
        build_tiny_rcon_message(message);
        remove_temp_banned_ip_address(temp_banned_player_data.ip_address, message, true);
      }

      main_app.get_game_server().get_tempbanned_players_to_unban().clear();
      is_main_window_constructed = true;

      while (!should_program_terminate().load()) {
        {
          unique_lock<mutex> ul{ mu };
          exit_flag.wait_for(ul, std::chrono::milliseconds(25), [&]() {
            return is_terminate_program.load();
          });
        }

        string rcon_reply;

        while (!main_app.is_command_queue_empty()) {
          auto cmd = main_app.get_command_from_queue();
          main_app.process_queue_command(std::move(cmd));
        }

        if (is_refresh_players_data_event.load()) {
          if (main_app.get_is_connection_settings_valid()) {
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

  task_thread.detach();

  while (GetMessage(&msg, nullptr, 0, 0)) {

    if (IsDialogMessage(app_handles.hwnd_main_window, &msg) == 0) {
      TranslateMessage(&msg);
      if (msg.message == WM_KEYDOWN) {
        process_key_down_message(msg);
      }
      DispatchMessage(&msg);
    }

    if (is_main_window_constructed && !is_tinyrcon_initialized) {
      string rcon_task_reply;
      is_tinyrcon_initialized = true;
      if (main_app.get_is_connection_settings_valid()) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Sending rcon command ^1'g_gametype' ^2to the game server.\n", true, true, true);
        main_app.get_connection_manager().send_and_receive_rcon_data("g_gametype", rcon_task_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Sending rcon command ^1'status' ^2to the game server.\n", true, true, true);
        main_app.get_connection_manager().send_and_receive_rcon_data("status", rcon_task_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
        prepare_players_data_for_display();

      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "^2Sending rcon command ^1'getstatus' ^2to the game server.\n", true, true, true);
        main_app.get_connection_manager().send_and_receive_rcon_data("getstatus", rcon_task_reply, main_app.get_game_server().get_server_ip_address().c_str(), main_app.get_game_server().get_server_port(), main_app.get_game_server().get_rcon_password().c_str());
        prepare_players_data_for_display_of_getstatus_response();
      }
      display_players_data_in_players_grid(app_handles.hwnd_players_grid);
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


    if (msg.message == WM_KEYDOWN) {
      process_key_down_message(msg);
    } else if (msg.message == WM_RBUTTONDOWN && app_handles.hwnd_players_grid == GetFocus()) {
      const int x{ GET_X_LPARAM(msg.lParam) };
      const int y{ GET_Y_LPARAM(msg.lParam) };
      display_context_menu_over_grid(x, y, selected_row);
    }
  }

  is_terminate_program.store(true);
  {
    unique_lock<mutex> ul{ mu };
    exit_flag.notify_one();
  }

  if (!check_if_call_of_duty_2_game_is_running()) {
    CloseHandle(pr_info.hProcess);
    CloseHandle(pr_info.hThread);
  }

  log_message("Exiting TinyRcon program.", true);

  // export_geoip_data(main_app.get_connection_manager().get_geoip_data(), "plugins/geoIP/geo.dat");

  if (wcex.hbrBackground != nullptr)
    DeleteObject((HGDIOBJ)wcex.hbrBackground);
  if (RED_BRUSH != NULL)
    DeleteObject((HGDIOBJ)RED_BRUSH);
  if (hfontbody != NULL)
    DeleteFont(hfontbody);
  if (hImageList)
    ImageList_Destroy(hImageList);
  UnregisterClass(wcex.lpszClassName, app_handles.hInstance);
  UnregisterClass(wcex_confirmation_dialog.lpszClassName, app_handles.hInstance);
  UnregisterClass(wcex_configuration_dialog.lpszClassName, app_handles.hInstance);

  return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
  wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONGUI));
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = CreateSolidBrush(color::black);
  wcex.lpszClassName = "TINYRCONGUI";
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  auto status = RegisterClassEx(&wcex);
  if (!status) {
    char errorMessage[256]{};
    sprintf_s(errorMessage, "Windows class creation failed with error code: %d", GetLastError());
    MessageBox(nullptr, errorMessage, "Window Class Failed", MB_ICONERROR);
  }

  wcex_confirmation_dialog = {};
  wcex_confirmation_dialog.cbSize = sizeof(WNDCLASSEX);
  wcex_confirmation_dialog.style = CS_HREDRAW | CS_VREDRAW;
  wcex_confirmation_dialog.lpfnWndProc = WndProcForConfirmationDialog;
  wcex_confirmation_dialog.hInstance = hInstance;
  wcex_confirmation_dialog.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONGUI));
  wcex_confirmation_dialog.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex_confirmation_dialog.hbrBackground = CreateSolidBrush(color::black);
  wcex_confirmation_dialog.lpszClassName = "ConfirmationDialog";
  wcex_confirmation_dialog.hIconSm = LoadIcon(wcex_confirmation_dialog.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  status = RegisterClassEx(&wcex_confirmation_dialog);
  if (!status) {
    char errorMessage[256]{};
    sprintf_s(errorMessage, "Windows class creation failed with error code: %d", GetLastError());
    MessageBox(nullptr, errorMessage, "Window Class Failed", MB_ICONERROR);
  }

  wcex_configuration_dialog = {};

  wcex_configuration_dialog.cbSize = sizeof(WNDCLASSEX);
  wcex_configuration_dialog.style = CS_HREDRAW | CS_VREDRAW;
  wcex_configuration_dialog.lpfnWndProc = WndProcForConfigurationDialog;
  wcex_configuration_dialog.hInstance = hInstance;
  wcex_configuration_dialog.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONGUI));
  wcex_configuration_dialog.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex_configuration_dialog.hbrBackground = CreateSolidBrush(color::black);
  wcex_configuration_dialog.lpszClassName = "ConfigurationDialog";
  wcex_configuration_dialog.hIconSm = LoadIcon(wcex_configuration_dialog.hInstance, MAKEINTRESOURCE(IDI_SMALL));
  status = RegisterClassEx(&wcex_configuration_dialog);
  if (!status) {
    char errorMessage[256]{};
    sprintf_s(errorMessage, "Windows class creation failed with error code: %d", GetLastError());
    MessageBox(nullptr, errorMessage, "Window Class Failed", MB_ICONERROR);
  }

  return status;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
  app_handles.hInstance = hInstance;// Store instance handle in our global variable

  InitSimpleGrid(app_handles.hInstance);

  INITCOMMONCONTROLSEX icex;

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_PROGRESS_CLASS;
  InitCommonControlsEx(&icex);

  hfontbody = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_DECORATIVE, "Lucida Console");

  app_handles.hwnd_main_window = CreateWindow("TINYRCONGUI", "Welcome to TinyRcon | For admins of the 185.158.113.146:28995 CoD2 CTF | version: 2.4", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME, 0, 0, screen_width, screen_height, nullptr, nullptr, hInstance, nullptr);

  if (!app_handles.hwnd_main_window)
    return FALSE;

  ShowWindow(app_handles.hwnd_main_window, nCmdShow);
  UpdateWindow(app_handles.hwnd_main_window);
  return TRUE;
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
  HBRUSH orig_textEditBrush{};
  HBRUSH comboBrush1{}, comboBrush2{}, comboBrush3{}, comboBrush4{}, comboBrush5{};
  HBRUSH defaultbrush{};
  HBRUSH hotbrush{};
  HBRUSH selectbrush{};
  HPEN pen{};
  PAINTSTRUCT ps;

  switch (message) {

  case WM_TIMER: {

    ++atomic_counter;
    if (atomic_counter.load() > main_app.get_game_server().get_check_for_banned_players_time_period())
      atomic_counter.store(0);

    SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, (WPARAM)atomic_counter.load(), 0);

    RECT bounding_rectangle{
      screen_width - 480,
      screen_height - 78,
      screen_width - 200,
      screen_height - 58,
    };

    if (main_app.get_game_server().get_check_for_banned_players_time_period() == atomic_counter.load()) {
      is_refresh_players_data_event.store(true);
    }

    if (is_refreshed_players_data_ready_event.load()) {
      is_refreshed_players_data_ready_event.store(false);
      display_players_data_in_players_grid(app_handles.hwnd_players_grid);
    }

    InvalidateRect(hWnd, &bounding_rectangle, TRUE);
    // is_refreshing_players_data.store(false);
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

    if ((some_item->idFrom == ID_WARNBUTTON || some_item->idFrom == ID_KICKBUTTON || some_item->idFrom == ID_TEMPBANBUTTON || some_item->idFrom == ID_IPBANBUTTON || some_item->idFrom == ID_VIEWTEMPBANSBUTTON || some_item->idFrom == ID_VIEWIPBANSBUTTON || some_item->idFrom == ID_CONNECTBUTTON || some_item->idFrom == ID_CONNECTPRIVATESLOTBUTTON || some_item->idFrom == ID_SAY_BUTTON || some_item->idFrom == ID_TELL_BUTTON || some_item->idFrom == ID_QUITBUTTON || some_item->idFrom == ID_LOADBUTTON || some_item->idFrom == ID_BUTTON_CONFIGURE_SERVER_SETTINGS || some_item->idFrom == ID_CLEARMESSAGESCREENBUTTON || some_item->idFrom == ID_REFRESHDATABUTTON)
        && (some_item->code == NM_CUSTOMDRAW)) {
      LPNMCUSTOMDRAW item = (LPNMCUSTOMDRAW)some_item;

      if ((item->uItemState & CDIS_FOCUS) || (item->uItemState & CDIS_SELECTED)) {
        if (!selectbrush)
          selectbrush = CreateSolidBrush(color::yellow);

        if (!pen) pen = CreatePen(PS_SOLID, 2, color::red);

        HGDIOBJ old_pen = SelectObject(item->hdc, pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, selectbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);
        DeleteObject(pen);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        return CDRF_SKIPDEFAULT;
      } else {
        if (item->uItemState & CDIS_HOT) {
          if (!hotbrush)
            hotbrush = CreateSolidBrush(color::yellow);

          if (!pen) pen = CreatePen(PS_SOLID, 2, color::red);

          HGDIOBJ old_pen = SelectObject(item->hdc, pen);
          HGDIOBJ old_brush = SelectObject(item->hdc, hotbrush);

          Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

          SelectObject(item->hdc, old_pen);
          SelectObject(item->hdc, old_brush);
          DeleteObject(pen);

          SetTextColor(item->hdc, color::red);
          SetBkMode(item->hdc, TRANSPARENT);
          DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
          return CDRF_SKIPDEFAULT;
        }

        if (defaultbrush == NULL)
          defaultbrush = CreateSolidBrush(color::light_blue);

        if (!pen) pen = CreatePen(PS_SOLID, 2, color::light_blue);

        HGDIOBJ old_pen = SelectObject(item->hdc, pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, defaultbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);
        DeleteObject(pen);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        return CDRF_SKIPDEFAULT;
      }
    }
    return CDRF_DODEFAULT;

  } break;


  case WM_COMMAND: {

    const int wmId = LOWORD(wParam);
    const int wparam_high_word = HIWORD(wParam);

    switch (wmId) {

    case ID_QUITBUTTON:
      if (show_user_confirmation_dialog("^3Do you really want to quit?\n", "Quit TinyRcon?")) {
        is_terminate_program.store(true);
        {
          unique_lock<mutex> ul{ mu };
          exit_flag.notify_one();
        }
        PostQuitMessage(0);
      }
      break;

    case ID_CLEARMESSAGESCREENBUTTON: {
      Edit_SetText(app_handles.hwnd_re_messages_data, "");
      g_message_data_contents.clear();
    } break;

    case ID_CONNECTBUTTON: {
      snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to connect to ^1%s ^3game server?\n", main_app.get_game_server_name().c_str());
      if (show_user_confirmation_dialog(message_buffer, "Connect to game server?")) {
        smatch ip_port_match{};
        const string ip_port_server_address{ regex_search(admin_reason, ip_port_match, ip_address_and_port_regex) ? (ip_port_match[1].str() + ":"s + ip_port_match[2].str()) : (main_app.get_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_game_server().get_server_port())) };
        const size_t sep_pos{ ip_port_server_address.find(':') };
        const string ip_address{ ip_port_server_address.substr(0, sep_pos) };
        const uint16_t port_number{ static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1))) };
        const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(ip_address.c_str(), port_number, main_app.get_game_server().get_rcon_password().c_str());

        const game_name_t game_name{ result.second != game_name_t::unknown ? result.second : main_app.get_game_name() };
        connect_to_the_game_server(ip_port_server_address, game_name, false, true);
      }
    } break;

    case ID_CONNECTPRIVATESLOTBUTTON: {

      snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to connect to ^1%s ^3game server using a ^1private slot^3?\n", main_app.get_game_server_name().c_str());
      if (show_user_confirmation_dialog(message_buffer, "Connect to game server using a private slot?")) {
        smatch ip_port_match{};
        const string ip_port_server_address{ regex_search(admin_reason, ip_port_match, ip_address_and_port_regex) ? (ip_port_match[1].str() + ":"s + ip_port_match[2].str()) : (main_app.get_game_server().get_server_ip_address() + ":"s + to_string(main_app.get_game_server().get_server_port())) };
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
        snprintf(message_buffer, std::size(message_buffer), "^2Are you sure you want to load map ^3%s ^2in ^3%s ^2game type?\n", full_map, gametype_uc.c_str());
        if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
          const string load_map_command{ stl::helper::str_join(std::vector<string>{ "!m", full_map_names_to_rcon_map_names.at(full_map), rcon_gametype }, " ") };
          Edit_SetText(app_handles.hwnd_e_user_input, load_map_command.c_str());
          get_user_input();
        }
      }
    } break;

    case ID_WARNBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid) };
          snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to warn player (^4%s^3)?\n", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            snprintf(msg_buffer, std::size(msg_buffer), "!w %d %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            admin_reason.assign("not specified");
            get_user_input();
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "\n^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", true, true, true);
      }
    } break;

    case ID_KICKBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid) };
          snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to kick player (^4%s^3)?\n", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            snprintf(msg_buffer, std::size(msg_buffer), "!k %d %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            admin_reason.assign("not specified");
            get_user_input();
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "\n^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", true, true, true);
      }

    } break;

    case ID_TEMPBANBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid) };
          snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to temporarily ban IP address of player: (^4%s^3)?\n", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            snprintf(msg_buffer, std::size(msg_buffer), "!tb %d 24 %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "\n^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", true, true, true);
      }

    } break;

    case ID_IPBANBUTTON: {
      if (check_if_selected_cell_indices_are_valid(selected_row, selected_col)) {
        string selected_pid_str{ GetCellContents(app_handles.hwnd_players_grid, selected_row, 0) };
        if (selected_pid_str.length() >= 2 && '^' == selected_pid_str[0] && is_decimal_digit(selected_pid_str[1]))
          selected_pid_str.erase(0, 2);
        if (int pid{ -1 }; is_valid_decimal_whole_number(selected_pid_str, pid)) {
          const string player_information{ get_player_information(pid) };
          snprintf(message_buffer, std::size(message_buffer), "^3Are you sure you want to permanently ban IP address of player: (^4%s^3)?\n", player_information.c_str());
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action")) {
            snprintf(msg_buffer, std::size(msg_buffer), "!gb %d %s", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      } else {
        print_colored_text(app_handles.hwnd_re_messages_data, "\n^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data table!\n^5Please, select a non-empty, valid player's row.\n", true, true, true);
      }

    } break;

    case ID_SAY_BUTTON: {

      admin_reason = "Type your public message in here...";
      if (show_user_confirmation_dialog("^2Are you sure you want to send a ^1public message ^2visible to all players?\n", "Confirm your action", "Message:")) {
        snprintf(msg_buffer, std::size(msg_buffer), "say \"%s\"", admin_reason.c_str());
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
          const string player_information{ get_player_information(pid) };
          snprintf(message_buffer, std::size(message_buffer), "^2Are you sure you want to send a ^1private message ^2to player: (^4%s^3)?\n", player_information.c_str());
          admin_reason = "Type your private message in here...";
          if (show_user_confirmation_dialog(message_buffer, "Confirm your action", "Message:")) {
            snprintf(msg_buffer, std::size(msg_buffer), "tell %d \"%s\"", pid, admin_reason.c_str());
            Edit_SetText(app_handles.hwnd_e_user_input, msg_buffer);
            get_user_input();
            admin_reason.assign("not specified");
          }
        }
      }
    } break;

    case ID_VIEWTEMPBANSBUTTON: {
      is_display_temporarily_banned_players_data_event.store(true);
    } break;

    case ID_VIEWIPBANSBUTTON: {
      is_display_permanently_banned_players_data_event.store(true);
    } break;

    case ID_SORT_PLAYERS_DATA_BY_PID: {
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_pid_asc ? sort_type::pid_asc : sort_type::pid_desc;
      sort_by_pid_asc = !sort_by_pid_asc;
      process_sort_type_change_request(type_of_sort);
    } break;

    case ID_SORT_PLAYERS_DATA_BY_SCORE: {
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_score_asc ? sort_type::score_asc : sort_type::score_desc;
      sort_by_score_asc = !sort_by_score_asc;
      process_sort_type_change_request(type_of_sort);
    } break;

    case ID_SORT_PLAYERS_DATA_BY_PING: {
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_ping_asc ? sort_type::ping_asc : sort_type::ping_desc;
      sort_by_ping_asc = !sort_by_ping_asc;
      process_sort_type_change_request(type_of_sort);
    } break;

    case ID_SORT_PLAYERS_DATA_BY_NAME: {
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_name_asc ? sort_type::name_asc : sort_type::name_desc;
      sort_by_name_asc = !sort_by_name_asc;
      process_sort_type_change_request(type_of_sort);
    } break;

    case ID_SORT_PLAYERS_DATA_BY_IP: {
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_ip_asc ? sort_type::ip_asc : sort_type::ip_desc;
      sort_by_ip_asc = !sort_by_ip_asc;
      process_sort_type_change_request(type_of_sort);
    } break;

    case ID_SORT_PLAYERS_DATA_BY_GEO: {
      is_process_combobox_item_selection_event = false;
      type_of_sort = sort_by_geo_asc ? sort_type::geo_asc : sort_type::geo_desc;
      sort_by_geo_asc = !sort_by_geo_asc;
      process_sort_type_change_request(type_of_sort);
    } break;

    case ID_BUTTON_CONFIGURE_SERVER_SETTINGS: {
      if (!show_and_process_tinyrcon_configuration_panel("Configure and verify your game server's settings.")) {
        show_error(app_handles.hwnd_main_window, "Failed to construct and show TinyRcon configuration dialog!", 0);
      }
      if (is_terminate_program.load()) {
        lock_guard<mutex> ul{ mu };
        exit_flag.notify_one();
        PostQuitMessage(0);
      }
    } break;
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

    RECT bounding_rectangle{
      screen_width / 2 + 170,
      13,
      screen_width / 2 + 340,
      30
    };

    DrawText(hdc, "TinyRcon messages:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT /*| DT_END_ELLIPSIS*/);

    bounding_rectangle = {
      screen_width / 2 + 180, screen_height / 2 + 113, screen_width / 2 + 220, screen_height / 2 + 135
    };
    DrawText(hdc, "Map:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT /*| DT_END_ELLIPSIS*/);

    bounding_rectangle = { screen_width / 2 + 500, screen_height / 2 + 113, screen_width / 2 + 570, screen_height / 2 + 135 };
    DrawText(hdc, "Gametype:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT /*| DT_END_ELLIPSIS*/);

    bounding_rectangle = {
      10,
      screen_height - 80,
      120,
      screen_height - 60
    };

    DrawText(hdc, prompt_message, -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT /*| DT_END_ELLIPSIS*/);

    bounding_rectangle = {
      screen_width - 480,
      screen_height - 78,
      screen_width - 200,
      screen_height - 58,
    };

    if (!is_tinyrcon_initialized) {
      DrawText(hdc, "Configuring and initializing tinyrcon ...", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);

    } else {

      const size_t time_period = main_app.get_game_server().get_check_for_banned_players_time_period();

      atomic_counter.store(std::min<size_t>(atomic_counter.load(), time_period));

      if (atomic_counter.load() == time_period) {
        DrawText(hdc, "Refreshing players data now...", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);
      } else {
        snprintf(msg_buffer, std::size(msg_buffer), refresh_players_data_fmt_str, time_period - atomic_counter.load());
        DrawText(hdc, msg_buffer, -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);
      }
    }

    EndPaint(hWnd, &ps);

    if (is_display_temporarily_banned_players_data_event.load()) {
      display_temporarily_banned_ip_addresses();
      is_display_temporarily_banned_players_data_event.store(false);
    } else if (is_display_permanently_banned_players_data_event.load()) {
      display_permanently_banned_ip_addresses();
      is_display_permanently_banned_players_data_event.store(false);
    }
  } break;

  case WM_KEYDOWN:
    if (wParam == VK_DOWN || wParam == VK_UP)
      return 0;
    break;
  case WM_CLOSE:
    if (show_user_confirmation_dialog("^3Do you really want to quit?", "Quit TinyRcon?")) {
      is_terminate_program.store(true);
      {
        lock_guard<mutex> ul{ mu };
        exit_flag.notify_one();
      }
      DestroyWindow(app_handles.hwnd_main_window);
    }
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
    PostQuitMessage(0);
    return 0;

  case WM_SIZE:
    if (hWnd == app_handles.hwnd_main_window && is_main_window_constructed)
      return 0;
    return DefWindowProcA(hWnd, message, wParam, lParam);

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

      // Create pen for button border
      auto pen = CreatePen(PS_SOLID, 2, color::red);

      // Select our brush into hDC
      HGDIOBJ old_pen = SelectObject(hdc, pen);
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
      DeleteObject(pen);

    } else {

      auto brush = CreateSolidBrush(color::light_blue);
      auto pen = CreatePen(PS_SOLID, 2, color::black);
      auto oldbrush = SelectObject(hdc, brush);
      auto oldpen = SelectObject(hdc, pen);
      SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
      SetBkMode(hdc, OPAQUE);
      SetBkColor(hdc, color::light_blue);
      SetTextColor(hdc, color::red);

      Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

      SelectObject(hdc, oldpen);
      SelectObject(hdc, oldbrush);
      DeleteObject(brush);
      DeleteObject(pen);
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
  HPEN pen{};

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

        if (!pen)
          pen = CreatePen(PS_SOLID, 2, color::red);

        HGDIOBJ old_pen = SelectObject(item->hdc, pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, selectbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);
        DeleteObject(pen);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        return CDRF_SKIPDEFAULT;
      } else {
        if (item->uItemState & CDIS_HOT) {
          if (!hotbrush)
            hotbrush = CreateSolidBrush(color::yellow);

          if (!pen)
            pen = CreatePen(PS_SOLID, 2, color::red);

          HGDIOBJ old_pen = SelectObject(item->hdc, pen);
          HGDIOBJ old_brush = SelectObject(item->hdc, hotbrush);

          Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

          SelectObject(item->hdc, old_pen);
          SelectObject(item->hdc, old_brush);
          DeleteObject(pen);

          SetTextColor(item->hdc, color::red);
          SetBkMode(item->hdc, TRANSPARENT);
          DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
          return CDRF_SKIPDEFAULT;
        }

        if (defaultbrush == NULL)
          defaultbrush = CreateSolidBrush(color::light_blue);

        if (!pen)
          pen = CreatePen(PS_SOLID, 2, color::light_blue);

        HGDIOBJ old_pen = SelectObject(item->hdc, pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, defaultbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);
        DeleteObject(pen);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
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
    if (pen) DeletePen(pen);
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
  HPEN pen{};

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

        if (!pen)
          pen = CreatePen(PS_SOLID, 2, color::red);

        HGDIOBJ old_pen = SelectObject(item->hdc, pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, selectbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);
        DeleteObject(pen);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        return CDRF_SKIPDEFAULT;
      } else {
        if (item->uItemState & CDIS_HOT) {
          if (!hotbrush)
            hotbrush = CreateSolidBrush(color::yellow);

          if (!pen)
            pen = CreatePen(PS_SOLID, 2, color::red);

          HGDIOBJ old_pen = SelectObject(item->hdc, pen);
          HGDIOBJ old_brush = SelectObject(item->hdc, hotbrush);

          Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

          SelectObject(item->hdc, old_pen);
          SelectObject(item->hdc, old_brush);
          DeleteObject(pen);

          SetTextColor(item->hdc, color::red);
          SetBkMode(item->hdc, TRANSPARENT);
          DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
          return CDRF_SKIPDEFAULT;
        }

        if (defaultbrush == NULL)
          defaultbrush = CreateSolidBrush(color::light_blue);

        if (!pen)
          pen = CreatePen(PS_SOLID, 2, color::light_blue);

        HGDIOBJ old_pen = SelectObject(item->hdc, pen);
        HGDIOBJ old_brush = SelectObject(item->hdc, defaultbrush);

        Rectangle(item->hdc, item->rc.left, item->rc.top, item->rc.right, item->rc.bottom);

        SelectObject(item->hdc, old_pen);
        SelectObject(item->hdc, old_brush);
        DeleteObject(pen);

        SetTextColor(item->hdc, color::red);
        SetBkMode(item->hdc, TRANSPARENT);
        DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
        return CDRF_SKIPDEFAULT;
      }
    }
    return CDRF_DODEFAULT;

  } break;


  case WM_COMMAND: {
    const int wmId = LOWORD(wParam);
    switch (wmId) {

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
        lock_guard<mutex> ul{ mu };
        exit_flag.notify_one();
      }
    } break;

    case ID_BUTTON_CONFIGURATION_COD1_PATH: {

      strcpy_s(install_path, max_path_length, "C:\\");
      snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty game's installation folder (codmp.exe) and click OK.");

      const char *cod1_game_path = BrowseFolder(install_path, msg_buffer);

      if (lstrcmpA(cod1_game_path, "") == 0 || lstrcmpA(cod1_game_path, "C:\\") == 0) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^1Error! You haven't selected a valid folder for your game installation.\n");
        print_colored_text(app_handles.hwnd_re_messages_data, "^5You have to select your ^1game's installation directory\n ^5and click the OK button.\n");
      } else {
        snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", cod1_game_path);
      }

      stl::helper::trim_in_place(exe_file_path);

      Edit_SetText(app_handles.hwnd_cod1_path_edit, exe_file_path);

    } break;

    case ID_BUTTON_CONFIGURATION_COD2_PATH: {

      strcpy_s(install_path, max_path_length, "C:\\");
      snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty 2 game's installation folder (cod2mp_s.exe) and click OK.");

      const char *cod2_game_path = BrowseFolder(install_path, msg_buffer);

      if (lstrcmpA(cod2_game_path, "") == 0 || lstrcmpA(cod2_game_path, "C:\\") == 0) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^1Error! You haven't selected a valid folder for your game installation.\n");
        print_colored_text(app_handles.hwnd_re_messages_data, "^5You have to select your ^1game's installation directory\n ^5and click the OK button.\n");
      } else {
        snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", cod2_game_path);
      }

      stl::helper::trim_in_place(exe_file_path);

      Edit_SetText(app_handles.hwnd_cod2_path_edit, exe_file_path);

    } break;

    case ID_BUTTON_CONFIGURATION_COD4_PATH: {

      strcpy_s(install_path, max_path_length, "C:\\");
      snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty 4: Modern Warfare game's installation folder (iw3mp.exe) and click OK.");

      const char *cod4_game_path = BrowseFolder(install_path, msg_buffer);

      if (lstrcmpA(cod4_game_path, "") == 0 || lstrcmpA(cod4_game_path, "C:\\") == 0) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^1Error! You haven't selected a valid folder for your game installation.\n");
        print_colored_text(app_handles.hwnd_re_messages_data, "^5You have to select your ^1game's installation directory\n ^5and click the OK button.\n");
      } else {
        snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", cod4_game_path);
      }

      stl::helper::trim_in_place(exe_file_path);

      Edit_SetText(app_handles.hwnd_cod4_path_edit, exe_file_path);

    } break;

    case ID_BUTTON_CONFIGURATION_COD5_PATH: {

      strcpy_s(install_path, max_path_length, "C:\\");
      snprintf(msg_buffer, std::size(msg_buffer), "Please, select your Call of Duty 5: World at War game's installation folder (cod5mp.exe) and click OK.");

      const char *cod5_game_path = BrowseFolder(install_path, msg_buffer);

      if (lstrcmpA(cod5_game_path, "") == 0 || lstrcmpA(cod5_game_path, "C:\\") == 0) {
        print_colored_text(app_handles.hwnd_re_messages_data, "^1Error! You haven't selected a valid folder for your game installation.\n");
        print_colored_text(app_handles.hwnd_re_messages_data, "^5You have to select your ^1game's installation directory\n ^5and click the OK button.\n");
      } else {
        snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", cod5_game_path);
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
    if (pen) DeletePen(pen);
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
