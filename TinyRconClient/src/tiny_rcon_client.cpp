#include "framework.h"
#include "resource.h"

#undef min

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace std;
using namespace stl::helper;
using namespace std::string_literals;
using namespace std::chrono;
using namespace std::filesystem;

extern const string program_version{"2.7.7.7"};

extern const std::regex ip_address_and_port_regex;
extern const unordered_set<string> rcon_status_commands;
extern const std::unordered_map<char, COLORREF> rich_edit_colors;

extern std::atomic<bool> is_terminate_program;
extern std::atomic<bool> is_terminate_tinyrcon_settings_configuration_dialog_window;
extern string g_message_data_contents;
extern string online_admins_information;

tiny_rcon_client_application main_app;
sort_type type_of_sort{sort_type::geo_asc};

volatile atomic<size_t> atomic_counter{0U};
volatile std::atomic<bool> is_refresh_players_data_event{false};
volatile std::atomic<bool> is_refreshed_players_data_ready_event{false};
volatile std::atomic<bool> is_display_muted_players_data_event{false};
volatile std::atomic<bool> is_display_temporarily_banned_players_data_event{false};
volatile std::atomic<bool> is_display_permanently_banned_players_data_event{false};
volatile std::atomic<bool> is_display_banned_ip_address_ranges_data_event{false};
volatile std::atomic<bool> is_display_banned_cities_data_event{false};
volatile std::atomic<bool> is_display_banned_countries_data_event{false};
volatile std::atomic<bool> is_display_banned_player_names_data_event{false};
volatile std::atomic<bool> is_display_protected_ip_addresses_data_event{false};
volatile std::atomic<bool> is_display_protected_ip_address_ranges_data_event{false};
volatile std::atomic<bool> is_display_protected_cities_data_event{false};
volatile std::atomic<bool> is_display_protected_countries_data_event{false};
volatile std::atomic<bool> is_display_admins_data_event{false};
volatile std::atomic<bool> is_display_players_data{true};
volatile std::atomic<bool> is_refreshing_game_servers_data_event{false};
volatile std::atomic<bool> is_display_geoinformation_data_for_players{false};
volatile std::atomic<bool> is_executed_tasks_at_exit{false};
volatile std::atomic<bool> is_player_being_spectated{false};
volatile std::atomic<int> spectated_player_pid{-1};
static std::atomic<size_t> number_of_entries_to_display{25u};

std::mutex reports_mutex;
std::once_flag synchronize_bans_flag{};
static int operation_completed_flag[6]{};

extern const int screen_width{GetSystemMetrics(SM_CXSCREEN)};
extern const int screen_height{GetSystemMetrics(SM_CYSCREEN) - 30};
RECT client_rect{0, 0, screen_width, screen_height};

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

std::unordered_map<int, string> button_id_to_label_text{{ID_MUTE_PLAYER, "Mute player"},
                                                        {ID_UNMUTE_PLAYER, "Unmute player"},
                                                        {ID_VIEWMUTEDPLAYERS, "View muted players"},
                                                        {ID_WARNBUTTON, "&Warn"},
                                                        {ID_KICKBUTTON, "&Kick"},
                                                        {ID_TEMPBANBUTTON, "&Tempban"},
                                                        {ID_IPBANBUTTON, "&Ban IP"},
                                                        {ID_VIEWTEMPBANSBUTTON, "View temporary bans"},
                                                        {ID_VIEWIPBANSBUTTON, "View &IP bans"},
                                                        {ID_VIEWADMINSDATA, "View &admins' data"},
                                                        {ID_RCONVIEWBUTTON, "&Show rcon server"},
                                                        {ID_SHOWPLAYERSBUTTON, "Show players"},
                                                        {ID_REFRESH_PLAYERS_DATA_BUTTON, "&Refresh players"},
                                                        {ID_SHOWSERVERSBUTTON, "Show servers"},
                                                        {ID_REFRESHSERVERSBUTTON, "Refresh servers"},
                                                        {ID_CONNECTBUTTON, "&Join server"},
                                                        {ID_CONNECTPRIVATESLOTBUTTON, "Joi&n server (private slot)"},
                                                        {ID_SAY_BUTTON, "Send public message"},
                                                        {ID_TELL_BUTTON, "Send private message"},
                                                        {ID_SPECTATEPLAYER, "Spectate player"},
                                                        {ID_QUITBUTTON, "E&xit"},
                                                        {ID_LOADBUTTON, "Load &map"},
                                                        {ID_YES_BUTTON, "Yes"},
                                                        {ID_NO_BUTTON, "No"},
                                                        {ID_BUTTON_SAVE_CHANGES, "Save changes"},
                                                        {ID_BUTTON_TEST_CONNECTION, "Test connection"},
                                                        {ID_BUTTON_CANCEL, "Cancel"},
                                                        {ID_BUTTON_CONFIGURE_SERVER_SETTINGS, "Confi&gure settings"},
                                                        {ID_CLEARMESSAGESCREENBUTTON, "C&lear messages"},
                                                        {ID_BUTTON_CONFIGURATION_EXIT_TINYRCON, "Exit TinyRcon"},
                                                        {ID_BUTTON_CONFIGURATION_COD1_PATH, "Browse for codmp.exe"},
                                                        {ID_BUTTON_CONFIGURATION_COD2_PATH, "Browse for cod2mp_s.exe"},
                                                        {ID_BUTTON_CONFIGURATION_COD4_PATH, "Browse for iw3mp.exe"},
                                                        {ID_BUTTON_CONFIGURATION_COD5_PATH, "Browse for cod5mp.exe"}};

extern const std::unordered_map<string, sort_type> sort_mode_names_dict;

unordered_map<size_t, string> rcon_status_grid_column_header_titles;
unordered_map<size_t, string> get_status_grid_column_header_titles;
unordered_map<size_t, string> servers_grid_column_header_titles;

extern const char *prompt_message{"Administrator >>"};
extern const char *refresh_players_data_fmt_str{"Refreshing players data in %zu %s."};
extern const size_t max_players_grid_rows;
extern const size_t max_servers_grid_rows;

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
bool is_process_combobox_item_selection_event{true};
bool is_first_left_mouse_button_click_in_prompt_edit_control{true};
bool is_first_left_mouse_button_click_in_reason_edit_control{true};
extern const HBRUSH red_brush{CreateSolidBrush(color::red)};
extern const HBRUSH black_brush{CreateSolidBrush(color::black)};
atomic<int> admin_choice{0};
string admin_reason{"not specified"};
string copied_game_server_address;
extern HIMAGELIST hImageList;
extern HIMAGELIST hImageListForChat;
HFONT font_for_players_grid_data{};

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

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nCmdShow)
{
    // _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    InitCommonControls();
    LoadLibraryA("Riched20.dll");

    HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
    // GdiplusStartupInput gdiplusStartupInput{};
    // ULONG_PTR gdiplusToken{};
    // GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    register_window_classes(hInstance);

    if (!initialize_main_app(hInstance, nCmdShow))
        return 0;

    main_app.set_current_working_directory();

    const string config_file_path{
        format("{}{}", main_app.get_current_working_directory(), main_app.get_tinyrcon_config_file_path())};

    if (auto [status, file_path] = create_necessary_folders_and_files(
            {"config", "data\\images\\maps", "log", "plugins\\geoIP", "temp", "tools",
             main_app.get_tinyrcon_config_file_path(), main_app.get_user_data_file_path(),
             main_app.get_temp_bans_file_path(), main_app.get_ip_bans_file_path(),
             main_app.get_ip_range_bans_file_path(), main_app.get_banned_countries_file_path(),
             main_app.get_banned_cities_file_path(), main_app.get_banned_names_file_path(),
             main_app.get_protected_ip_addresses_file_path(), main_app.get_protected_ip_address_ranges_file_path(),
             main_app.get_protected_cities_file_path(), main_app.get_protected_countries_file_path(),
             main_app.get_reported_players_file_path()});
        !status)
    {

        show_error(app_handles.hwnd_main_window, "Error creating necessary program folders and files!", 0);
    }

    SetFileAttributes(config_file_path.c_str(), GetFileAttributes(config_file_path.c_str()) & ~FILE_ATTRIBUTE_READONLY);
    SetFileAttributes(config_file_path.c_str(), FILE_ATTRIBUTE_NORMAL);

    // parse_tinyrcon_tool_config_file(main_app.get_tinyrcon_config_file_path());

    // find_call_of_duty_1_installation_path(false);
    // find_call_of_duty_2_installation_path(false);
    // find_call_of_duty_4_installation_path(false);
    // find_call_of_duty_5_installation_path(false);

    // char exe_file_path[MAX_PATH]{};
    // GetModuleFileName(nullptr, exe_file_path, MAX_PATH);

    rcon_status_grid_column_header_titles[0] = main_app.get_header_player_pid_color() + "Pid"s;
    rcon_status_grid_column_header_titles[1] = main_app.get_header_player_score_color() + "Score"s;
    rcon_status_grid_column_header_titles[2] = main_app.get_header_player_ping_color() + "Ping"s;
    rcon_status_grid_column_header_titles[3] = main_app.get_header_player_name_color() + "Player name"s;
    rcon_status_grid_column_header_titles[4] = main_app.get_header_player_ip_color() + "IP address"s;
    rcon_status_grid_column_header_titles[5] = main_app.get_header_player_geoinfo_color() + "Geological information"s;
    rcon_status_grid_column_header_titles[6] = main_app.get_header_player_geoinfo_color() + "Flag"s;
    rcon_status_grid_column_header_titles[7] = main_app.get_header_player_geoinfo_color() + "Chat"s;

    get_status_grid_column_header_titles[0] = main_app.get_header_player_pid_color() + "Player no."s;
    get_status_grid_column_header_titles[1] = main_app.get_header_player_score_color() + "Score"s;
    get_status_grid_column_header_titles[2] = main_app.get_header_player_ping_color() + "Ping"s;
    get_status_grid_column_header_titles[3] = main_app.get_header_player_name_color() + "Player name"s;

    servers_grid_column_header_titles[0] = "Id"s;
    servers_grid_column_header_titles[1] = "Server name"s;
    servers_grid_column_header_titles[2] = "Server address"s;
    servers_grid_column_header_titles[3] = "Players"s;
    servers_grid_column_header_titles[4] = "Current map"s;
    servers_grid_column_header_titles[5] = "Gametype"s;
    servers_grid_column_header_titles[6] = "Voice"s;
    servers_grid_column_header_titles[7] = "Flag"s;

    construct_tinyrcon_gui(app_handles.hwnd_main_window);

    main_app.open_log_file("log\\commands_history.log");

    std::thread print_messages_thread{[&]() {
        IsGUIThread(TRUE);
        // HWND re_control{app_handles.hwnd_re_messages_data};

        while (true)
        {

            try
            {

                while (!is_terminate_program.load() && !main_app.is_tinyrcon_message_queue_empty())
                {
                    print_message_t msg{main_app.get_tinyrcon_message_from_queue()};
                    print_message(app_handles.hwnd_re_messages_data, msg.message_.c_str(), msg.log_to_file_,
                                  msg.is_log_current_date_time_, msg.is_remove_color_codes_for_log_message_);
                }
            }
            catch (std::exception &ex)
            {
                const string error_message{format("^3A specific exception was caught in "
                                                  "print_messages_thread!\n^1Exception: {}",
                                                  ex.what())};
                print_message(app_handles.hwnd_re_messages_data, error_message);
            }
            catch (...)
            {
                char buffer[512];
                strerror_s(buffer, GetLastError());
                const string error_message{format("^3A generic error was caught in "
                                                  "print_messages_thread!\n^1Exception: {}",
                                                  buffer)};
                print_message(app_handles.hwnd_re_messages_data, error_message);
            }

            Sleep(20);
        }
    }};

    print_messages_thread.detach();

    parse_tinyrcon_tool_config_file(main_app.get_tinyrcon_config_file_path());

    find_call_of_duty_1_installation_path(false);
    find_call_of_duty_2_installation_path(false);
    find_call_of_duty_4_installation_path(false);
    find_call_of_duty_5_installation_path(false);

    main_app.get_bitmap_image_handler().set_bitmap_images_folder_path(
        format("{}data\\images\\maps", main_app.get_current_working_directory()));

    main_app.set_game_server_index(0U);
    main_app.set_user_ip_address(trim(get_tiny_rcon_client_external_ip_address()));
    initialize_and_verify_server_connection_settings();

    string username{main_app.get_username()};
    remove_all_color_codes(username);
    const string welcome_message{format("Welcome back, {}", username)};
    SetWindowTextA(app_handles.hwnd_e_user_input, welcome_message.c_str());

    load_tinyrcon_client_user_data(main_app.get_user_data_file_path());

    if (main_app.get_current_game_server().get_is_connection_settings_valid())
    {
        me = main_app.get_user_for_name(main_app.get_username());
        me->ip_address = main_app.get_user_ip_address();
        me->is_admin = true;
        me->is_logged_in = true;
    }
    else
    {
        me = main_app.get_player_for_name(main_app.get_username(), main_app.get_user_ip_address());
        me->ip_address = main_app.get_user_ip_address();
        me->is_admin = false;
        me->is_logged_in = true;
    }

    /*if (main_app.get_is_enable_players_stats_feature())
    {

            const string stats_data_dir_path{ format("{}data",
    main_app.get_current_working_directory()) }; auto file_name_matches =
    get_file_name_matches_for_specified_file_path_pattern(stats_data_dir_path.c_str(),
    "player_stats_for_year-*.dat");

            time_t max_time_stamp = get_current_time_stamp();
            string stats_data_file_name;
            for (const auto& file_name : file_name_matches)
            {
                    size_t start_pos{ file_name.rfind('-') };
                    const size_t end_pos{ file_name.rfind(".dat") };
                    if (start_pos != string::npos && end_pos != string::npos)
                    {
                            ++start_pos;
                            const auto ts = stoll(file_name.substr(start_pos,
    end_pos - start_pos)); if (ts > max_time_stamp)
                            {
                                    max_time_stamp = ts;
                                    stats_data_file_name = file_name;
                            }
                    }
            }
            if (!stats_data_file_name.empty())
            {
                    const string file_name_path_for_stats_data_for_year{
    format("data\\{}", stats_data_file_name) };
                    main_app.get_stats_data().set_stop_time_in_seconds_for_year(max_time_stamp);
                    load_players_stats_data(file_name_path_for_stats_data_for_year.c_str(),
    main_app.get_stats_data().get_scores_for_year_vector(),
    main_app.get_stats_data().get_scores_for_year_map());
            }

            file_name_matches =
    get_file_name_matches_for_specified_file_path_pattern(stats_data_dir_path.c_str(),
    "player_stats_for_month-*.dat");

            max_time_stamp = get_current_time_stamp();
            stats_data_file_name.clear();
            for (const auto& file_name : file_name_matches)
            {
                    size_t start_pos{ file_name.rfind('-') };
                    const size_t end_pos{ file_name.rfind(".dat") };
                    if (start_pos != string::npos && end_pos != string::npos)
                    {
                            ++start_pos;
                            const auto ts = stoll(file_name.substr(start_pos,
    end_pos - start_pos)); if (ts > max_time_stamp)
                            {
                                    max_time_stamp = ts;
                                    stats_data_file_name = file_name;
                            }
                    }
            }
            if (!stats_data_file_name.empty())
            {
                    const string file_name_path_for_stats_data_for_month{
    format("data\\{}", stats_data_file_name) };
                    main_app.get_stats_data().set_stop_time_in_seconds_for_month(max_time_stamp);
                    load_players_stats_data(file_name_path_for_stats_data_for_month.c_str(),
    main_app.get_stats_data().get_scores_for_month_vector(),
    main_app.get_stats_data().get_scores_for_month_map());
            }

            file_name_matches =
    get_file_name_matches_for_specified_file_path_pattern(stats_data_dir_path.c_str(),
    "player_stats_for_day-*.dat");

            max_time_stamp = get_current_time_stamp();
            stats_data_file_name.clear();
            for (const auto& file_name : file_name_matches)
            {
                    size_t start_pos{ file_name.rfind('-') };
                    const size_t end_pos{ file_name.rfind(".dat") };
                    if (start_pos != string::npos && end_pos != string::npos)
                    {
                            ++start_pos;
                            const auto ts = stoll(file_name.substr(start_pos,
    end_pos - start_pos)); if (ts > max_time_stamp)
                            {
                                    max_time_stamp = ts;
                                    stats_data_file_name = file_name;
                            }
                    }
            }
            if (!stats_data_file_name.empty())
            {
                    const string file_name_path_for_stats_data_for_day{
    format("data\\{}", stats_data_file_name) };
                    main_app.get_stats_data().set_stop_time_in_seconds_for_day(max_time_stamp);
                    load_players_stats_data(file_name_path_for_stats_data_for_day.c_str(),
    main_app.get_stats_data().get_scores_for_day_vector(),
    main_app.get_stats_data().get_scores_for_day_map());
            }

            load_players_stats_data("data\\player_stats.dat",
    main_app.get_stats_data().get_scores_vector(),
    main_app.get_stats_data().get_scores_map());
    }*/

    main_app.add_command_handler({"cls", "!cls"}, [](const vector<string> &) {
        SetWindowTextA(app_handles.hwnd_re_messages_data, "");
        g_message_data_contents.clear();
    });

    main_app.add_command_handler({"list", "!list", "help", "!help", "h", "!h"},
                                 [](const vector<string> &user_cmd) { print_help_information(user_cmd); });

    main_app.add_command_handler({"!geoinfo"}, [](const vector<string> &user_cmd) {
        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

            if (check_if_user_provided_argument_is_valid_for_specified_command(user_cmd[0].c_str(), user_cmd[1]))
            {
                is_display_geoinformation_data_for_players.store(user_cmd[1] == "on");
            }
        }
    });

    main_app.add_command_handler({"!r", "!report"}, [](const vector<string> &user_cmd) {
        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

            if (check_if_user_provided_argument_is_valid_for_specified_command(user_cmd[0].c_str(), user_cmd[1]))
            {
                const int pid{stoi(user_cmd[1])};

                player &pd{main_app.get_current_game_server().get_player_data(pid)};
                string reason{remove_disallowed_characters_in_string(
                    user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
                stl::helper::trim_in_place(reason);
                pd.reason = reason;
                main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
                    "add-report",
                    format(R"({}\{}\{}\{}\{})", main_app.get_username(), main_app.get_user_ip_address(), pid,
                           pd.player_name, reason),
                    true, main_app.get_private_tiny_rcon_server_ip_address(),
                    main_app.get_private_tiny_rcon_server_port(), false);

                const string message{format("^3You have successfully reported player ^7{} ^3whose ^1pid ^3is "
                                            "^1{}^3!\nInformation: ^7{}\n",
                                            pd.player_name, pid, get_player_information(pid, false, "Reported"))};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                {
                    lock_guard lk{reports_mutex};
                    main_app.get_reported_players().emplace_back(pd);
                }

                me->no_of_reports++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
            }
        }
        else
        {
            const string re_msg2{format("^3Invalid command syntax for user command: ^2{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg2.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"!ur", "!unreport"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

            if (!check_if_user_provided_argument_is_valid_for_specified_command(user_cmd[0].c_str(), user_cmd[1]))
            {
                const string re_msg{format("^5Provided IP address ^1{} ^5is not a valid IP address or there "
                                           "isn't a reported player entry whose ^1serial no. ^5is equal to "
                                           "provided number: ^1{}!\n",
                                           user_cmd[1], user_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
            else
            {
                string message;
                string report_key{user_cmd[1]};
                auto [status, player]{remove_reported_player(report_key, message, true)};
                if (status)
                {
                    main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
                        "remove-report",
                        format(R"({}\{}\{}\{}\{}\{}\{})", player.ip_address, player.player_name,
                               player.banned_date_time, player.banned_start_time, player.reason,
                               player.banned_by_user_name, main_app.get_username()),
                        true, main_app.get_private_tiny_rcon_server_ip_address(),
                        main_app.get_private_tiny_rcon_server_port(), false);
                    const string private_msg{format("^1Admin ^7{} ^2has successfully removed ^1reported player "
                                                    "^7{}\n ^3IP address: ^1{} ^3geoinfo: ^1{}, {}\n ^3Date/time: "
                                                    "^1{} ^5| ^3Reason: ^1{} ^5| ^3Reported by: ^1{}\n",
                                                    main_app.get_username(), player.player_name, player.ip_address,
                                                    player.country_name, player.city, player.banned_date_time,
                                                    player.reason, player.banned_by_user_name)};
                    const string inform_msg{format("{}\\{}\\{}", main_app.get_username(), message, private_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                }
                else
                {
                    const string re_msg{format("^1Admin ^7{} ^3failed to remove ^1reported player ^7for IP "
                                               "address or player no. ^1{}!\n^1Reason: ^3Provided IP address "
                                               "^1{} ^3hasn't been ^1reported ^3yet.\n",
                                               main_app.get_username(), user_cmd[1], user_cmd[1])};
                    const string inform_msg{format("{}\\{}", main_app.get_username(), re_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                }
            }
        }
    });

    main_app.add_command_handler({"reports", "!reports"}, [](const vector<string> &user_cmd) {
        string ex_msg{format("^1Exception ^3thrown from ^1command handler ^3for "
                             "^1'{}' ^3user command.",
                             user_cmd[0])};
        stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
        int number{};
        const size_t number_of_reports_to_retrieve{
            (user_cmd.size() > 1u && !user_cmd[1].empty() && is_valid_decimal_whole_number(user_cmd[1], number))
                ? static_cast<size_t>(number)
                : 25u};

        main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
            "request-reports-player",
            format(R"({}\{}\{})", main_app.get_username(), main_app.get_user_ip_address(),
                   number_of_reports_to_retrieve),
            true, main_app.get_private_tiny_rcon_server_ip_address(), main_app.get_private_tiny_rcon_server_port(),
            false);
    });

    main_app.add_command_handler({"!w", "!warn"}, [](const vector<string> &user_cmd) {
        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

            if (check_if_user_provided_argument_is_valid_for_specified_command(user_cmd[0].c_str(), user_cmd[1]))
            {
                const int pid{stoi(user_cmd[1])};

                player &pd{main_app.get_current_game_server().get_player_data(pid)};
                string protected_player_msg;
                if (check_if_player_is_protected(pd, user_cmd[0].c_str(), protected_player_msg))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                    replace_br_with_new_line(protected_player_msg);
                    const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                    return;
                }

                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                string reason{remove_disallowed_characters_in_string(
                    user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
                stl::helper::trim_in_place(reason);
                specify_reason_for_player_pid(pid, reason);
                main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                string command{main_app.get_user_defined_warn_message()};
                build_tiny_rcon_message(command);
                rcon_say(command);
                auto &warned_players = main_app.get_current_game_server().get_warned_players_data();
                auto [player, is_online] = get_online_player_for_specified_pid(pid);
                if (is_online)
                {
                    if (!warned_players.contains(pid))
                    {
                        warned_players[pid] = move(player);
                        warned_players[pid].warned_times = 1;
                    }
                    else
                    {
                        ++warned_players[pid].warned_times;
                    }

                    const string message{format("^3You have successfully executed ^5!warn ^3on player ({}^3)\n",
                                                get_player_information(pid, false, "Warned"))};
                    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);

                    const string date_time_info{get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}")};
                    strcpy_s(warned_players[pid].banned_date_time, std::size(warned_players[pid].banned_date_time),
                             date_time_info.c_str());
                    warned_players[pid].reason = reason;
                    warned_players[pid].banned_by_user_name = main_app.get_username();
                    auto &current_user = main_app.get_user_for_name(main_app.get_username());
                    current_user->no_of_warnings++;
                    save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                    main_app.get_connection_manager_for_messages().process_and_send_message(
                        "add-warning",
                        format(R"({}\{}\{}\{}\{}\{})", warned_players[pid].ip_address, warned_players[pid].guid_key,
                               warned_players[pid].player_name, warned_players[pid].banned_date_time, reason,
                               main_app.get_username()),
                        true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);

                    const size_t number_of_warnings_for_automatic_kick =
                        main_app.get_current_game_server().get_maximum_number_of_warnings_for_automatic_kick();
                    if (warned_players[pid].warned_times >= number_of_warnings_for_automatic_kick)
                    {
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = warned_players[pid].player_name;
                        string reason2{remove_disallowed_characters_in_string(format(
                            "^1Received {} warnings from admin: {}",
                            main_app.get_current_game_server().get_maximum_number_of_warnings_for_automatic_kick(),
                            main_app.get_username()))};
                        stl::helper::trim_in_place(reason2);
                        specify_reason_for_player_pid(pid, reason2);
                        main_app.get_tinyrcon_dict()["{REASON}"] = reason2;
                        string command2{main_app.get_user_defined_kick_message()};
                        build_tiny_rcon_message(command2);
                        warned_players[pid].reason = reason;
                        current_user->no_of_kicks++;
                        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "add-kick",
                            format(R"({}\{}\{}\{}\{}\{})", warned_players[pid].ip_address, warned_players[pid].guid_key,
                                   warned_players[pid].player_name, warned_players[pid].banned_date_time, reason,
                                   main_app.get_username()),
                            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                            false);
                        kick_player(pid, command2);
                        warned_players.erase(pid);
                    }
                }
            }
        }
        else
        {
            const string re_msg2{format("^3Invalid command syntax for user command: ^2{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg2.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"!k", "!kick"}, [](const std::vector<std::string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

            if (check_if_user_provided_argument_is_valid_for_specified_command(user_cmd[0].c_str(), user_cmd[1]))
            {
                const int pid{stoi(user_cmd[1])};
                string protected_player_msg;
                if (check_if_player_is_protected(get_player_data_for_pid(pid), user_cmd[0].c_str(),
                                                 protected_player_msg))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                    replace_br_with_new_line(protected_player_msg);
                    const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                    return;
                }
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
                string reason{remove_disallowed_characters_in_string(
                    user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
                stl::helper::trim_in_place(reason);
                specify_reason_for_player_pid(pid, reason);
                main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                            get_player_information(pid, false, "Kicked"))};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                string command{main_app.get_user_defined_kick_message()};
                build_tiny_rcon_message(command);
                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                current_user->no_of_kicks++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                player &player{get_player_data_for_pid(pid)};
                player.banned_start_time = get_current_time_stamp();
                const string date_time_info{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", player.banned_start_time)};
                strcpy_s(player.banned_date_time, std::size(player.banned_date_time), date_time_info.c_str());
                player.reason = reason;
                player.banned_by_user_name = main_app.get_username();
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "add-kick",
                    format(R"({}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, player.player_name,
                           player.banned_date_time, reason, player.banned_by_user_name),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                kick_player(pid, command);
            }
            else
            {
                const string re_msg{format("^2{} ^3is not a valid pid number for "
                                           "the ^2!k ^3(^2!kick^3) command!\n",
                                           user_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^2{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"!tb", "!tempban"}, [](const std::vector<std::string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            if (check_if_user_provided_argument_is_valid_for_specified_command(user_cmd[0].c_str(), user_cmd[1]))
            {
                if (int number{}; is_valid_decimal_whole_number(user_cmd[1], number))
                {
                    const int pid{stoi(user_cmd[1])};
                    string protected_player_msg;
                    if (check_if_player_is_protected(get_player_data_for_pid(pid), user_cmd[0].c_str(),
                                                     protected_player_msg))
                    {
                        print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                        replace_br_with_new_line(protected_player_msg);
                        const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                        main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                        return;
                    }
                    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{BANNED_BY}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
                    size_t temp_ban_duration{24};
                    string reason{"not specified"};
                    if (user_cmd.size() > 2)
                    {
                        if (is_valid_decimal_whole_number(user_cmd[2], number))
                        {
                            temp_ban_duration = number > 0 && number <= 9999 ? number : 24;
                            if (user_cmd.size() > 3)
                            {
                                reason = remove_disallowed_characters_in_string(
                                    str_join(cbegin(user_cmd) + 3, cend(user_cmd), " "));
                                stl::helper::trim_in_place(reason);
                            }
                        }
                        else
                        {
                            reason = remove_disallowed_characters_in_string(
                                str_join(cbegin(user_cmd) + 2, cend(user_cmd), " "));
                            stl::helper::trim_in_place(reason);
                        }
                    }

                    main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                    main_app.get_tinyrcon_dict()["{TEMPBAN_DURATION}"] = to_string(temp_ban_duration);
                    const string message{format("^3You have successfully executed "
                                                "^5!tempban ^3on player ({}^3)\n",
                                                get_player_information(pid, false, "Temporarily banned"))};
                    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                    string command{main_app.get_user_defined_tempban_message()};
                    build_tiny_rcon_message(command);
                    player &pd = get_player_data_for_pid(pid);
                    pd.ban_duration_in_hours = temp_ban_duration;
                    pd.reason = reason;
                    pd.banned_by_user_name = main_app.get_username();
                    tempban_player(pd, command);
                    auto &current_user = main_app.get_user_for_name(main_app.get_username());
                    current_user->no_of_tempbans++;
                    save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                    main_app.get_connection_manager_for_messages().process_and_send_message(
                        "add-tempban",
                        format(R"({}\{}\{}\{}\{}\{}\{})", pd.ip_address, pd.player_name, pd.banned_date_time,
                               pd.banned_start_time, pd.ban_duration_in_hours, pd.reason, pd.banned_by_user_name),
                        true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                }
                else if (unsigned long ip_key{}; check_ip_address_validity(user_cmd[1], ip_key))
                {
                    string protected_player_msg;
                    player pd{};
                    pd.ip_address = user_cmd[1];
                    if (check_if_player_is_protected(pd, user_cmd[0].c_str(), protected_player_msg))
                    {
                        print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                        replace_br_with_new_line(protected_player_msg);
                        const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                        main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                        return;
                    }
                    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{BANNED_BY}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = "John Doe";
                    size_t temp_ban_duration{24};
                    string reason{"not specified"};
                    if (user_cmd.size() > 2)
                    {
                        if (int n{}; is_valid_decimal_whole_number(user_cmd[2], n))
                        {
                            temp_ban_duration = n > 0 && n <= 9999 ? n : 24;
                            if (user_cmd.size() > 3)
                            {
                                reason = remove_disallowed_characters_in_string(
                                    str_join(cbegin(user_cmd) + 3, cend(user_cmd), " "));
                                stl::helper::trim_in_place(reason);
                            }
                        }
                        else
                        {
                            reason = remove_disallowed_characters_in_string(
                                str_join(cbegin(user_cmd) + 2, cend(user_cmd), " "));
                            stl::helper::trim_in_place(reason);
                        }
                    }

                    main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                    main_app.get_tinyrcon_dict()["{TEMPBAN_DURATION}"] = to_string(temp_ban_duration);
                    const string message{format("^3You have successfully executed ^5!tempban ^3on IP address "
                                                "^1{} for ^1{} {}^3!\n",
                                                user_cmd[1], temp_ban_duration,
                                                temp_ban_duration != 1 ? "hours" : "hour")};
                    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                    string command{main_app.get_user_defined_tempban_message()};
                    build_tiny_rcon_message(command);
                    strcpy_s(pd.player_name, std::size(pd.player_name), "John Doe");
                    pd.ban_duration_in_hours = temp_ban_duration;
                    pd.reason = reason;
                    pd.banned_by_user_name = main_app.get_username();
                    tempban_player(pd, command);
                    auto &current_user = main_app.get_user_for_name(main_app.get_username());
                    current_user->no_of_tempbans++;
                    save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                    main_app.get_connection_manager_for_messages().process_and_send_message(
                        "add-tempban",
                        format(R"({}\{}\{}\{}\{}\{}\{})", pd.ip_address, pd.player_name, pd.banned_date_time,
                               pd.banned_start_time, pd.ban_duration_in_hours, pd.reason, pd.banned_by_user_name),
                        true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                }
            }
            else
            {

                if (int n{}; is_valid_decimal_whole_number(user_cmd[1], n))
                {
                    const string re_msg{format("^2{} ^3is not a valid pid number for "
                                               "the ^2!tb ^3(^2!tempban^3) command!\n",
                                               user_cmd[1])};
                    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                }
                else
                {
                    const string re_msg{format("^2{} ^3is not a valid IP address for "
                                               "the ^2!tb ^3(^2!tempban^3) command!\n",
                                               user_cmd[1])};
                    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                }
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^2{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"!b", "!ban"}, [](const std::vector<std::string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            if (check_if_user_provided_argument_is_valid_for_specified_command(user_cmd[0].c_str(), user_cmd[1]))
            {
                const int pid{stoi(user_cmd[1])};
                string protected_player_msg;
                if (check_if_player_is_protected(get_player_data_for_pid(pid), user_cmd[0].c_str(),
                                                 protected_player_msg))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                    replace_br_with_new_line(protected_player_msg);
                    const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                    return;
                }
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
                auto &pd = get_player_data_for_pid(pid);
                pd.banned_start_time = get_current_time_stamp();
                const string banned_date_time_str{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time)};
                strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
                main_app.get_tinyrcon_dict()["{BAN_DATE}"] = pd.banned_date_time;
                string reason{remove_disallowed_characters_in_string(
                    user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
                stl::helper::trim_in_place(reason);
                specify_reason_for_player_pid(pid, reason);
                main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                const string message{format("^3You have successfully executed ^5banclient ^3on player ({}^3)\n",
                                            get_player_information(pid, false, "GUID banned"))};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                string command{main_app.get_user_defined_ban_message()};
                build_tiny_rcon_message(command);
                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                current_user->no_of_guidbans++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                player &player{get_player_data_for_pid(pid)};
                player.banned_by_user_name = main_app.get_username();
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "add-guidban",
                    format(R"({}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, player.player_name,
                           player.banned_date_time, reason, player.banned_by_user_name),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                ban_player(pid, command);
            }
            else
            {
                const string re_msg{format("^2{} ^3is not a valid pid number for "
                                           "the ^2!b ^3(^2!ban^3) command!\n",
                                           user_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^2{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"!mute"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!mute", user_cmd[1]))
            {
                string ex_msg{format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.",
                                     user_cmd[0], user_cmd[1])};
                stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

                string ip_address;

                if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
                {
                    auto &player = main_app.get_current_game_server().get_player_data(pid);
                    string protected_player_msg;
                    if (check_if_player_is_protected(player, user_cmd[0].c_str(), protected_player_msg))
                    {
                        print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                        replace_br_with_new_line(protected_player_msg);
                        const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                        main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                        return;
                    }
                    if (pid == player.pid)
                    {
                        ip_address = player.ip_address;
                        unsigned long ip_key{};
                        if (check_ip_address_validity(player.ip_address, ip_key))
                        {
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
                            string reason{remove_disallowed_characters_in_string(
                                user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                    : "not specified")};
                            stl::helper::trim_in_place(reason);
                            specify_reason_for_player_pid(player.pid, reason);
                            mute_player_ip_address(player);
                            player.is_muted = true;
                            auto &players = main_app.get_current_game_server().get_players_data();
                            for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                            {
                                if (players[i].ip_address == ip_address)
                                {
                                    players[i].is_muted = true;
                                }
                            }
                            const string re_msg{format("^2You have successfully muted player with IP address: ^1{}\n",
                                                       player.ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                            const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                        user_cmd[0], get_player_information(pid, false, "Muted"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_mute_message()};
                            if (main_app.get_is_disable_automatic_kick_messages())
                            {
                                command.clear();
                            }
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            auto &current_user = main_app.get_user_for_name(main_app.get_username());
                            current_user->no_of_muted_ip_addresses++;
                            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "add-muted-ip",
                                format(R"({}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, player.player_name,
                                       player.banned_date_time, reason, main_app.get_username()),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
                else
                {
                    ip_address = user_cmd[1];
                    // if (!muted_players_map.contains(ip_address)) {
                    bool is_ip_address_already_muted{};
                    auto &players_data = main_app.get_current_game_server().get_players_data();
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        auto &player = players_data[i];
                        if (player.ip_address == ip_address)
                        {
                            string protected_player_msg;
                            if (check_if_player_is_protected(player, user_cmd[0].c_str(), protected_player_msg))
                            {
                                print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                                replace_br_with_new_line(protected_player_msg);
                                const string inform_msg{
                                    format("{}\\{}", main_app.get_username(), protected_player_msg)};
                                main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                                continue;
                            }

                            for (size_t j{}; j < main_app.get_current_game_server().get_number_of_players(); ++j)
                            {
                                if (players_data[j].ip_address == ip_address)
                                {
                                    players_data[j].is_muted = true;
                                }
                            }
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
                            string reason{remove_disallowed_characters_in_string(
                                user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                    : "not specified")};
                            stl::helper::trim_in_place(reason);
                            specify_reason_for_player_pid(player.pid, reason);
                            mute_player_ip_address(player);
                            player.is_muted = true;
                            const string re_msg{
                                format("^2You have successfully muted player with IP address: ^1{}\n", user_cmd[1])};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                            const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                        user_cmd[0],
                                                        get_player_information(player.pid, false, "Muted"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_mute_message()};
                            if (main_app.get_is_disable_automatic_kick_messages())
                            {
                                command.clear();
                            }
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            is_ip_address_already_muted = true;
                            auto &current_user = main_app.get_user_for_name(main_app.get_username());
                            current_user->no_of_muted_ip_addresses++;
                            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "add-muted-ip",
                                format(R"({}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, player.player_name,
                                       player.banned_date_time, reason, main_app.get_username()),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                            break;
                        }
                    }

                    if (!is_ip_address_already_muted)
                    {
                        player player_offline{};
                        player_offline.pid = -1;
                        strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                        player_offline.ip_address = ip_address;
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;

                        string reason{trim(remove_disallowed_characters_in_string(
                            user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                : "not specified"))};

                        size_t name_start_pos{string::npos};
                        size_t name_length{0u};
                        for (const auto needle : {"n=", "n:", "name=", "name:"})
                        {
                            const size_t found_pos{reason.find(needle)};
                            if (string::npos != found_pos)
                            {
                                name_start_pos = found_pos;
                                name_length = len(needle);
                                break;
                            }
                        }

                        size_t reason_start_pos{string::npos};
                        size_t reason_length{0u};
                        for (const auto needle : {"r=", "r:", "reason=", "reason:"})
                        {
                            const size_t found_pos{reason.find(needle)};
                            if (string::npos != found_pos)
                            {
                                reason_start_pos = found_pos;
                                reason_length = len(needle);
                                break;
                            }
                        }

                        if (name_start_pos != string::npos)
                        {
                            string player_name{trim(reason.substr(name_start_pos + name_length,
                                                                  reason_start_pos - (name_start_pos + name_length)))};

                            if (player_name.length() >= 2u &&
                                ((player_name.front() == '\'' && player_name.back() == '\'') ||
                                 (player_name.front() == '"' && player_name.back() == '"')))
                            {
                                player_name.pop_back();
                                player_name.erase(0, 1);
                            }
                            else
                            {
                                const size_t name_end_pos{player_name.find_first_of(" \t\n")};
                                player_name = player_name.substr(0, name_end_pos);
                            }

                            const size_t no_of_chars_to_copy{
                                std::min<size_t>(std::size(player_offline.player_name) - 1, player_name.length())};
                            strncpy_s(player_offline.player_name, std::size(player_offline.player_name),
                                      player_name.c_str(), no_of_chars_to_copy);
                            player_offline.player_name[no_of_chars_to_copy] = '\0';
                        }

                        if (reason_start_pos != string::npos)
                            reason = reason.substr(reason_start_pos + reason_length);

                        if (reason.length() >= 2 && ((reason.front() == '\'' && reason.back() == '\'') ||
                                                     (reason.front() == '"' && reason.back() == '"')))
                        {
                            reason.pop_back();
                            reason.erase(0, 1);
                        }

                        player_offline.reason = std::move(reason);
                        player_offline.is_muted = true;
                        convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                                         player_offline.ip_address, player_offline);
                        string protected_player_msg;
                        if (check_if_player_is_protected(player_offline, user_cmd[0].c_str(), protected_player_msg))
                        {
                            print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                            replace_br_with_new_line(protected_player_msg);
                            const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                            main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                            return;
                        }
                        mute_player_ip_address(player_offline);
                        main_app.get_tinyrcon_dict()["{REASON}"] = player_offline.reason;
                        string command{main_app.get_user_defined_mute_message()};
                        if (main_app.get_is_disable_automatic_kick_messages())
                        {
                            command.clear();
                        }
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        const string re_msg{format("^2You have successfully muted IP address: ^1{}\n", ip_address)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        const string message{format("^2You have successfully executed ^5{} ^2on player ({}^2)\n",
                                                    user_cmd[0],
                                                    get_player_information_for_player(player_offline, "Muted"))};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        auto &current_user = main_app.get_user_for_name(main_app.get_username());
                        current_user->no_of_muted_ip_addresses++;
                        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "add-muted-ip",
                            format(R"({}\0\{}\{}\{}\{})", player_offline.ip_address, player_offline.player_name,
                                   player_offline.banned_date_time, player_offline.reason, main_app.get_username()),
                            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                            false);
                    }
                    // }
                }
            }
        }
    });

    // *** !unmute ***
    main_app.add_command_handler({"!unmute"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler ^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            unsigned long ip_key{};
            const bool is_ip_address_valid{check_ip_address_validity(user_cmd[1], ip_key)};
            if (!is_ip_address_valid && !check_if_user_provided_pid_is_valid(user_cmd[1]))
            {
                const string re_msg{
                    format("^5Provided IP address ^1{} ^5is not a valid IP address or there isn't a muted player entry "
                           "whose ^1serial no. ^5is equal to the provided number: ^1{}!\n",
                           user_cmd[1], user_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
            else
            {
                string message;
                string ip_address{user_cmd[1]};
                int pid{-1};
                if (!is_ip_address_valid && check_if_user_provided_pid_is_valid(user_cmd[1]) &&
                    is_valid_decimal_whole_number(user_cmd[1], pid))
                {
                    ip_address = get_player_ip_address_for_pid(pid);
                }

                auto [status, player]{remove_muted_ip_address(ip_address, message, true)};
                if (status)
                {
                    auto &players_data = main_app.get_current_game_server().get_players_data();
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        if (players_data[i].ip_address == ip_address)
                        {
                            players_data[i].is_muted = false;
                        }
                    }
                    main_app.get_connection_manager_for_messages().process_and_send_message(
                        "remove-muted-ip",
                        format(R"({}\{}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, player.player_name,
                               player.banned_date_time, player.reason, player.banned_by_user_name,
                               main_app.get_username()),
                        true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                    const string private_msg{format(
                        "^1Admin ^7{} ^2has successfully removed ^1muted IP address: ^5{} ^2for player name: ^7{}\n",
                        main_app.get_username(), ip_address, player.player_name)};
                    const string inform_msg{format("{}\\{}\\{}", main_app.get_username(), message, private_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                }
                else
                {
                    const string re_msg{format("^1Admin ^7{} ^3failed to remove ^1muted IP address: ^5{}\n^1Reason: "
                                               "^3Provided IP address (^1{}^3) hasn't been ^1muted ^3yet.\n",
                                               main_app.get_username(), user_cmd[1], user_cmd[1])};
                    const string inform_msg{format("{}\\{}", main_app.get_username(), re_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                }
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });
    // ***

    main_app.add_command_handler({"!gb", "!globalban", "!banip", "!addip"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!gb", user_cmd[1]))
            {
                string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                     "^3for ^1'{} {}' ^3user command.",
                                     user_cmd[0], user_cmd[1])};
                stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
                const auto &banned_ip_addresses = main_app.get_current_game_server().get_banned_ip_addresses_map();
                if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
                {
                    auto &player = main_app.get_current_game_server().get_player_data(pid);
                    string protected_player_msg;
                    if (check_if_player_is_protected(player, user_cmd[0].c_str(), protected_player_msg))
                    {
                        print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                        replace_br_with_new_line(protected_player_msg);
                        const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                        main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                        return;
                    }
                    if (pid == player.pid)
                    {
                        unsigned long ip_key{};
                        if (check_ip_address_validity(player.ip_address, ip_key) &&
                            !banned_ip_addresses.contains(player.ip_address))
                        {
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
                            string reason{remove_disallowed_characters_in_string(
                                user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                    : "not specified")};
                            stl::helper::trim_in_place(reason);
                            specify_reason_for_player_pid(player.pid, reason);
                            global_ban_player_ip_address(player);
                            const string re_msg{
                                format("^2You have successfully banned IP address: ^1{}\n", player.ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                            const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                        user_cmd[0],
                                                        get_player_information(pid, false, "IP address banned"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_ipban_message()};
                            build_tiny_rcon_message(command);
                            auto &current_user = main_app.get_user_for_name(main_app.get_username());
                            current_user->no_of_ipbans++;
                            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "add-ipban",
                                format(R"({}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, player.player_name,
                                       player.banned_date_time, reason, main_app.get_username()),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                            kick_player(player.pid, command);
                        }
                    }
                }
                else
                {
                    const auto &ip_address = user_cmd[1];
                    if (!banned_ip_addresses.contains(ip_address))
                    {
                        bool is_ip_address_already_banned{};
                        auto &players_data = main_app.get_current_game_server().get_players_data();
                        for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                        {
                            auto &player = players_data[i];
                            if (player.ip_address == ip_address)
                            {
                                string protected_player_msg;
                                if (check_if_player_is_protected(player, user_cmd[0].c_str(), protected_player_msg))
                                {
                                    print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                                    replace_br_with_new_line(protected_player_msg);
                                    const string inform_msg{
                                        format("{}\\{}", main_app.get_username(), protected_player_msg)};
                                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                                    return;
                                }
                                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
                                string reason{remove_disallowed_characters_in_string(
                                    user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                        : "not specified")};
                                stl::helper::trim_in_place(reason);
                                specify_reason_for_player_pid(player.pid, reason);
                                global_ban_player_ip_address(player);
                                const string re_msg{
                                    format("^2You have successfully banned IP address: ^1{}\n", user_cmd[1])};
                                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                                const string message{format(
                                    "^3You have successfully executed ^5{} ^3on player "
                                    "({}^3)\n",
                                    user_cmd[0], get_player_information(player.pid, false, "IP address banned"))};
                                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                string command{main_app.get_user_defined_ipban_message()};
                                build_tiny_rcon_message(command);
                                is_ip_address_already_banned = true;
                                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                                current_user->no_of_ipbans++;
                                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                                main_app.get_connection_manager_for_messages().process_and_send_message(
                                    "add-ipban",
                                    format(R"({}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key,
                                           player.player_name, player.banned_date_time, reason,
                                           main_app.get_username()),
                                    true, main_app.get_tiny_rcon_server_ip_address(),
                                    main_app.get_tiny_rcon_server_port(), false);
                                kick_player(player.pid, command);
                                break;
                            }
                        }

                        if (!is_ip_address_already_banned)
                        {
                            player player_offline{};
                            player_offline.pid = -1;
                            strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                            player_offline.ip_address = ip_address;
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                            string reason{remove_disallowed_characters_in_string(
                                user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                    : "not specified")};
                            player_offline.reason = std::move(reason);
                            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                                             player_offline.ip_address, player_offline);
                            string protected_player_msg;
                            if (check_if_player_is_protected(player_offline, user_cmd[0].c_str(), protected_player_msg))
                            {
                                print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                                replace_br_with_new_line(protected_player_msg);
                                const string inform_msg{
                                    format("{}\\{}", main_app.get_username(), protected_player_msg)};
                                main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                                return;
                            }
                            global_ban_player_ip_address(player_offline);
                            main_app.get_tinyrcon_dict()["{REASON}"] = player_offline.reason;
                            string command{main_app.get_user_defined_ipban_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            const string re_msg{
                                format("^2You have successfully banned IP address: ^1{}\n", ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{
                                format("^2You have successfully executed ^5{} ^2on player ({}^2)\n", user_cmd[0],
                                       get_player_information_for_player(player_offline, "Banned IP address"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            auto &current_user = main_app.get_user_for_name(main_app.get_username());
                            current_user->no_of_ipbans++;
                            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "add-ipban",
                                format(R"({}\0\{}\{}\{}\{})", player_offline.ip_address, player_offline.player_name,
                                       player_offline.banned_date_time, player_offline.reason, main_app.get_username()),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!protectedipaddresses"}, [](const vector<string> &) {
        is_display_protected_ip_addresses_data_event.store(true);
    });

    main_app.add_command_handler({"!protectedipaddressranges"}, [](const vector<string> &) {
        is_display_protected_ip_address_ranges_data_event.store(true);
    });

    main_app.add_command_handler({"!protectedcities"},
                                 [](const vector<string> &) { is_display_protected_cities_data_event.store(true); });

    main_app.add_command_handler({"!protectedcountries"},
                                 [](const vector<string> &) { is_display_protected_countries_data_event.store(true); });

    main_app.add_command_handler({"!protectip"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!protectip", user_cmd[1]))
            {
                string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                     "^3for ^1'{} {}' ^3user command.",
                                     user_cmd[0], user_cmd[1])};
                stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
                auto &protected_ip_addresses = main_app.get_current_game_server().get_protected_ip_addresses();
                if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
                {
                    auto &player = main_app.get_current_game_server().get_player_data(pid);
                    if (pid == player.pid)
                    {
                        unsigned long ip_key{};
                        if (check_ip_address_validity(player.ip_address, ip_key) &&
                            !protected_ip_addresses.contains(player.ip_address))
                        {
                            protected_ip_addresses.emplace(player.ip_address);
                            save_protected_entries_file(main_app.get_protected_ip_addresses_file_path(),
                                                        protected_ip_addresses);
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                            string reason{remove_disallowed_characters_in_string(
                                user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                    : "not specified")};
                            stl::helper::trim_in_place(reason);
                            specify_reason_for_player_pid(player.pid, reason);
                            const string re_msg{
                                format("^2You have successfully protected ^1IP address: {}\n", player.ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                            const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                        user_cmd[0],
                                                        get_player_information(pid, false, "Protected IP address"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_protect_ip_address_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "protect-ipaddress",
                                format("{}\\{}\\{}", main_app.get_username(), player.ip_address, player.reason), true,
                                main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
                else
                {
                    const auto &ip_address = user_cmd[1];
                    string reason{remove_disallowed_characters_in_string(
                        user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
                    stl::helper::trim_in_place(reason);
                    if (!protected_ip_addresses.contains(ip_address))
                    {
                        protected_ip_addresses.emplace(ip_address);
                        save_protected_entries_file(main_app.get_protected_ip_addresses_file_path(),
                                                    protected_ip_addresses);
                        bool is_ip_address_already_protected{};
                        auto &players_data = main_app.get_current_game_server().get_players_data();
                        for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                        {
                            auto &player = players_data[i];
                            if (player.ip_address == ip_address)
                            {
                                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                                specify_reason_for_player_pid(player.pid, reason);
                                const string re_msg{
                                    format("^2You have successfully protected ^1IP address: {}\n", user_cmd[1])};
                                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                                const string message{format(
                                    "^3You have successfully executed ^5{} ^3on player "
                                    "({}^3)\n",
                                    user_cmd[0], get_player_information(player.pid, false, "Protected IP address"))};
                                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                string command{main_app.get_user_defined_protect_ip_address_message()};
                                build_tiny_rcon_message(command);
                                rcon_say(command);
                                is_ip_address_already_protected = true;
                                main_app.get_connection_manager_for_messages().process_and_send_message(
                                    "protect-ipaddress",
                                    format("{}\\{}\\{}", main_app.get_username(), player.ip_address, player.reason),
                                    true, main_app.get_tiny_rcon_server_ip_address(),
                                    main_app.get_tiny_rcon_server_port(), false);
                                break;
                            }
                        }

                        if (!is_ip_address_already_protected)
                        {
                            player player_offline{};
                            player_offline.pid = -1;
                            strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                            player_offline.ip_address = ip_address;
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                            player_offline.reason = std::move(reason);
                            main_app.get_tinyrcon_dict()["{REASON}"] = player_offline.reason;
                            string command{main_app.get_user_defined_protect_ip_address_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            const string re_msg{
                                format("^2You have successfully protected ^1IP address: {}\n", ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{
                                format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                       get_player_information_for_player(player_offline, "Protected IP address"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "protect-ipaddress",
                                format("{}\\{}\\{}", main_app.get_username(), player_offline.ip_address,
                                       player_offline.reason),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!unprotectip"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!unprotectip", user_cmd[1]))
            {
                string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                     "^3for ^1'{} {}' ^3user command.",
                                     user_cmd[0], user_cmd[1])};
                stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
                string reason{remove_disallowed_characters_in_string(
                    user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
                stl::helper::trim_in_place(reason);
                auto &protected_ip_addresses = main_app.get_current_game_server().get_protected_ip_addresses();
                if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
                {
                    auto &player = main_app.get_current_game_server().get_player_data(pid);
                    if (pid == player.pid)
                    {
                        unsigned long ip_key{};
                        if (check_ip_address_validity(player.ip_address, ip_key) &&
                            protected_ip_addresses.contains(player.ip_address))
                        {
                            protected_ip_addresses.erase(player.ip_address);
                            save_protected_entries_file(main_app.get_protected_ip_addresses_file_path(),
                                                        protected_ip_addresses);
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                            specify_reason_for_player_pid(player.pid, reason);
                            const string re_msg{format("^2You have successfully removed "
                                                       "protected ^1IP address: {}\n",
                                                       player.ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                            const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                        user_cmd[0],
                                                        get_player_information(pid, false, "Unprotected IP address"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_unprotect_ip_address_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "unprotect-ipaddress",
                                format("{}\\{}\\{}", main_app.get_username(), player.ip_address, player.reason), true,
                                main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
                else
                {
                    const auto &ip_address = user_cmd[1];
                    if (protected_ip_addresses.contains(ip_address))
                    {
                        protected_ip_addresses.erase(ip_address);
                        save_protected_entries_file(main_app.get_protected_ip_addresses_file_path(),
                                                    protected_ip_addresses);
                        bool is_ip_address_already_protected{};
                        auto &players_data = main_app.get_current_game_server().get_players_data();
                        for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                        {
                            auto &player = players_data[i];
                            if (player.ip_address == ip_address)
                            {
                                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                                specify_reason_for_player_pid(player.pid, reason);
                                const string re_msg{format("^2You have successfully removed "
                                                           "protected ^1IP address: {}\n",
                                                           user_cmd[1])};
                                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                                const string message{format(
                                    "^3You have successfully executed ^5{} ^3on player "
                                    "({}^3)\n",
                                    user_cmd[0], get_player_information(player.pid, false, "Unprotected IP address"))};
                                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                string command{main_app.get_user_defined_unprotect_ip_address_message()};
                                build_tiny_rcon_message(command);
                                rcon_say(command);
                                is_ip_address_already_protected = true;
                                main_app.get_connection_manager_for_messages().process_and_send_message(
                                    "unprotect-ipaddress",
                                    format("{}\\{}\\{}", main_app.get_username(), player.ip_address, player.reason),
                                    true, main_app.get_tiny_rcon_server_ip_address(),
                                    main_app.get_tiny_rcon_server_port(), false);
                                break;
                            }
                        }

                        if (!is_ip_address_already_protected)
                        {
                            player player_offline{};
                            player_offline.pid = -1;
                            strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                            player_offline.ip_address = ip_address;
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                            player_offline.reason = std::move(reason);
                            main_app.get_tinyrcon_dict()["{REASON}"] = player_offline.reason;
                            string command{main_app.get_user_defined_protect_ip_address_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            const string re_msg{format("^2You have successfully removed "
                                                       "protected ^1IP address: {}\n",
                                                       ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{
                                format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                       get_player_information_for_player(player_offline, "Unprotected IP address"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "unprotect-ipaddress",
                                format("{}\\{}\\{}", main_app.get_username(), player_offline.ip_address,
                                       player_offline.reason),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!protectiprange"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!protectiprange", user_cmd[1]))
            {
                string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                     "^3for ^1'{} {}' ^3user command.",
                                     user_cmd[0], user_cmd[1])};
                stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
                string reason{remove_disallowed_characters_in_string(
                    user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
                stl::helper::trim_in_place(reason);
                auto &protected_ip_address_ranges =
                    main_app.get_current_game_server().get_protected_ip_address_ranges();
                if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
                {
                    auto &player = main_app.get_current_game_server().get_player_data(pid);
                    if (pid == player.pid)
                    {
                        unsigned long ip_key{};
                        if (check_ip_address_validity(player.ip_address, ip_key))
                        {
                            const string error_message{
                                format("^1{} ^3is not a valid IP address.\n", player.ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
                            return;
                        }
                        const string narrow_ip_address_range{
                            get_narrow_ip_address_range_for_specified_ip_address(player.ip_address)};
                        if (!protected_ip_address_ranges.contains(narrow_ip_address_range))
                        {
                            protected_ip_address_ranges.emplace(narrow_ip_address_range);
                            save_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(),
                                                        protected_ip_address_ranges);
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                            specify_reason_for_player_pid(player.pid, reason);
                            const string re_msg{format("^2You have successfully protected ^1IP address range: {}\n",
                                                       narrow_ip_address_range)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                            const string message{
                                format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                       get_player_information(pid, false, "Protected IP address range"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_protect_ip_address_range_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "protect-ipaddressrange",
                                format("{}\\{}\\{}", main_app.get_username(), narrow_ip_address_range, player.reason),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
                else
                {
                    const auto &ip_address_range = user_cmd[1];
                    if (!protected_ip_address_ranges.contains(ip_address_range))
                    {
                        protected_ip_address_ranges.emplace(ip_address_range);
                        save_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(),
                                                    protected_ip_address_ranges);
                        bool is_ip_address_range_already_protected{};
                        auto &players_data = main_app.get_current_game_server().get_players_data();
                        for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                        {
                            auto &player = players_data[i];
                            const string narrow_ip_address_range{
                                get_narrow_ip_address_range_for_specified_ip_address(player.ip_address)};
                            const string wide_ip_address_range{
                                get_narrow_ip_address_range_for_specified_ip_address(player.ip_address)};
                            if (ip_address_range == narrow_ip_address_range ||
                                ip_address_range == wide_ip_address_range)
                            {
                                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                                specify_reason_for_player_pid(player.pid, reason);
                                const string re_msg{format("^2You have successfully protected ^1IP address range: "
                                                           "{}\n",
                                                           ip_address_range)};
                                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                                const string message{
                                    format("^3You have successfully executed ^5{} ^3on player "
                                           "({}^3)\n",
                                           user_cmd[0],
                                           get_player_information(player.pid, false, "Protected IP address range"))};
                                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                string command{main_app.get_user_defined_protect_ip_address_range_message()};
                                build_tiny_rcon_message(command);
                                rcon_say(command);
                                is_ip_address_range_already_protected = true;
                                main_app.get_connection_manager_for_messages().process_and_send_message(
                                    "protect-ipaddressrange",
                                    format("{}\\{}\\{}", main_app.get_username(), ip_address_range, player.reason),
                                    true, main_app.get_tiny_rcon_server_ip_address(),
                                    main_app.get_tiny_rcon_server_port(), false);
                                break;
                            }
                        }

                        if (!is_ip_address_range_already_protected)
                        {
                            player player_offline{};
                            player_offline.pid = -1;
                            strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                            player_offline.ip_address = ip_address_range;
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                            player_offline.reason = std::move(reason);
                            main_app.get_tinyrcon_dict()["{REASON}"] = player_offline.reason;
                            string command{main_app.get_user_defined_protect_ip_address_range_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            const string re_msg{
                                format("^2You have successfully protected ^1IP address range: {}\n", ip_address_range)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{format(
                                "^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                get_player_information_for_player(player_offline, "Protected IP address range"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "protect-ipaddressrange",
                                format("{}\\{}\\{}", main_app.get_username(), ip_address_range, player_offline.reason),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!unprotectiprange"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!unprotectiprange", user_cmd[1]))
            {
                string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                     "^3for ^1'{} {}' ^3user command.",
                                     user_cmd[0], user_cmd[1])};
                stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
                string reason{remove_disallowed_characters_in_string(
                    user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
                stl::helper::trim_in_place(reason);
                auto &protected_ip_address_ranges =
                    main_app.get_current_game_server().get_protected_ip_address_ranges();
                if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
                {
                    auto &player = main_app.get_current_game_server().get_player_data(pid);
                    if (pid == player.pid)
                    {
                        unsigned long ip_key{};
                        if (check_ip_address_validity(player.ip_address, ip_key))
                        {
                            const string error_message{
                                format("^1{} ^3is not a valid IP address.\n", player.ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
                            return;
                        }
                        const string narrow_ip_address_range{
                            get_narrow_ip_address_range_for_specified_ip_address(player.ip_address)};
                        if (protected_ip_address_ranges.contains(narrow_ip_address_range))
                        {
                            protected_ip_address_ranges.erase(narrow_ip_address_range);
                            save_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(),
                                                        protected_ip_address_ranges);
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                            specify_reason_for_player_pid(player.pid, reason);
                            const string re_msg{format("^2You have successfully removed protected ^1IP address "
                                                       "range: {}\n",
                                                       narrow_ip_address_range)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                            const string message{
                                format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                       get_player_information(pid, false, "Unprotected IP address range"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_unprotect_ip_address_range_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "unprotect-ipaddressrange",
                                format("{}\\{}\\{}", main_app.get_username(), narrow_ip_address_range, player.reason),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
                else
                {
                    const auto &ip_address_range = user_cmd[1];
                    if (protected_ip_address_ranges.contains(ip_address_range))
                    {
                        protected_ip_address_ranges.erase(ip_address_range);
                        save_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(),
                                                    protected_ip_address_ranges);
                        bool is_ip_address_range_already_protected{};
                        auto &players_data = main_app.get_current_game_server().get_players_data();
                        for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                        {
                            auto &player = players_data[i];
                            const string narrow_ip_address_range{
                                get_narrow_ip_address_range_for_specified_ip_address(player.ip_address)};
                            const string wide_ip_address_range{
                                get_narrow_ip_address_range_for_specified_ip_address(player.ip_address)};
                            if (ip_address_range == narrow_ip_address_range ||
                                ip_address_range == wide_ip_address_range)
                            {
                                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                                specify_reason_for_player_pid(player.pid, reason);
                                const string re_msg{format("^2You have successfully removed protected ^1IP address "
                                                           "range: {}\n",
                                                           ip_address_range)};
                                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                                const string message{
                                    format("^3You have successfully executed ^5{} ^3on player "
                                           "({}^3)\n",
                                           user_cmd[0],
                                           get_player_information(player.pid, false, "Unprotected IP address range"))};
                                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                                   is_log_datetime::yes);
                                string command{main_app.get_user_defined_unprotect_ip_address_range_message()};
                                build_tiny_rcon_message(command);
                                rcon_say(command);
                                is_ip_address_range_already_protected = true;
                                main_app.get_connection_manager_for_messages().process_and_send_message(
                                    "unprotect-ipaddressrange",
                                    format("{}\\{}\\{}", main_app.get_username(), ip_address_range, player.reason),
                                    true, main_app.get_tiny_rcon_server_ip_address(),
                                    main_app.get_tiny_rcon_server_port(), false);
                                break;
                            }
                        }

                        if (!is_ip_address_range_already_protected)
                        {
                            player player_offline{};
                            player_offline.pid = -1;
                            strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                            player_offline.ip_address = ip_address_range;
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                            player_offline.reason = std::move(reason);
                            main_app.get_tinyrcon_dict()["{REASON}"] = player_offline.reason;
                            string command{main_app.get_user_defined_unprotect_ip_address_range_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            const string re_msg{format("^2You have successfully removed protected ^1IP address "
                                                       "range: {}\n",
                                                       ip_address_range)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{format(
                                "^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                get_player_information_for_player(player_offline, "Unprotected IP address range"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "unprotect-ipaddressrange",
                                format("{}\\{}\\{}", main_app.get_username(), ip_address_range, player_offline.reason),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                        }
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!protectcity"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string reason{remove_disallowed_characters_in_string(
                user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            stl::helper::trim_in_place(reason);
            auto &protected_cities = main_app.get_current_game_server().get_protected_cities();
            if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
            {
                auto &player = main_app.get_current_game_server().get_player_data(pid);
                if (pid == player.pid)
                {
                    if (!protected_cities.contains(player.city))
                    {
                        protected_cities.emplace(player.city);
                        save_protected_entries_file(main_app.get_protected_cities_file_path(), protected_cities);
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                        main_app.get_tinyrcon_dict()["{CITY_NAME}"] = player.city;
                        specify_reason_for_player_pid(player.pid, reason);
                        const string re_msg{format("^2You have successfully protected ^1city: {}\n", player.city)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                    user_cmd[0], get_player_information(pid, false, "Protected city"))};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        string command{main_app.get_user_defined_protect_city_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "protect-city", format("{}\\{}\\{}", main_app.get_username(), player.city, player.reason),
                            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                            false);
                    }
                }
            }
            else
            {
                const string city{stl::helper::trim(str_join(cbegin(user_cmd) + 1, cend(user_cmd), " "))};
                if (!protected_cities.contains(city))
                {
                    protected_cities.emplace(city);
                    save_protected_entries_file(main_app.get_protected_cities_file_path(), protected_cities);
                    bool is_city_already_protected{};
                    auto &players_data = main_app.get_current_game_server().get_players_data();
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        auto &player = players_data[i];
                        if (player.city == city)
                        {
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                            main_app.get_tinyrcon_dict()["{CITY_NAME}"] = city;

                            const string re_msg{format("^2You have successfully protected ^1city: {}\n", user_cmd[1])};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                        user_cmd[0],
                                                        get_player_information(player.pid, false, "Protected city"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_protect_city_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            is_city_already_protected = true;
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "protect-city", format("{}\\{}\\n/a", main_app.get_username(), player.city), true,
                                main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                            break;
                        }
                    }

                    if (!is_city_already_protected)
                    {
                        player player_offline{};
                        player_offline.pid = -1;
                        strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                        main_app.get_tinyrcon_dict()["{CITY_NAME}"] = city;
                        string command{main_app.get_user_defined_protect_city_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        const string re_msg{format("^2You have successfully protected ^1city: {}\n", city)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        const string message{
                            format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                   get_player_information_for_player(player_offline, "Protected city"))};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "protect-city", format("{}\\{}\\n/a", main_app.get_username(), city), true,
                            main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!unprotectcity"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            auto &protected_cities = main_app.get_current_game_server().get_protected_cities();
            if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
            {
                auto &player = main_app.get_current_game_server().get_player_data(pid);
                if (pid == player.pid)
                {
                    if (protected_cities.contains(player.city))
                    {
                        protected_cities.erase(player.city);
                        save_protected_entries_file(main_app.get_protected_cities_file_path(), protected_cities);
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                        main_app.get_tinyrcon_dict()["{CITY_NAME}"] = player.city;
                        const string re_msg{
                            format("^2You have successfully removed protected ^1city: {}\n", player.city)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                    user_cmd[0],
                                                    get_player_information(pid, false, "Unprotected city"))};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        string command{main_app.get_user_defined_unprotect_city_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "unprotect-city", format("{}\\{}\\n/a", main_app.get_username(), player.city), true,
                            main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                    }
                }
            }
            else
            {
                const string city{stl::helper::trim(str_join(cbegin(user_cmd) + 1, cend(user_cmd), " "))};
                if (protected_cities.contains(city))
                {
                    protected_cities.erase(city);
                    save_protected_entries_file(main_app.get_protected_cities_file_path(), protected_cities);
                    bool is_city_already_protected{};
                    auto &players_data = main_app.get_current_game_server().get_players_data();
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        auto &player = players_data[i];
                        if (player.city == city)
                        {
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                            main_app.get_tinyrcon_dict()["{CITY_NAME}"] = city;
                            const string re_msg{format("^2You have successfully removed protected ^1city: {}\n", city)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                        user_cmd[0],
                                                        get_player_information(player.pid, false, "Unprotected city"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_unprotect_city_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            is_city_already_protected = true;
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "unprotect-city", format("{}\\{}\\n/a", main_app.get_username(), city), true,
                                main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                            break;
                        }
                    }

                    if (!is_city_already_protected)
                    {
                        player player_offline{};
                        player_offline.pid = -1;
                        strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                        main_app.get_tinyrcon_dict()["{CITY_NAME}"] = city;
                        string command{main_app.get_user_defined_unprotect_city_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        const string re_msg{format("^2You have successfully removed protected ^1city: {}\n", city)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "unprotect-city", format("{}\\{}\\n/a", main_app.get_username(), city), true,
                            main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!protectcountry"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            auto &protected_countries = main_app.get_current_game_server().get_protected_countries();
            if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
            {
                auto &player = main_app.get_current_game_server().get_player_data(pid);
                if (pid == player.pid)
                {
                    if (!protected_countries.contains(player.country_name))
                    {
                        protected_countries.emplace(player.country_name);
                        save_protected_entries_file(main_app.get_protected_countries_file_path(), protected_countries);
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                        main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = player.country_name;
                        const string re_msg{
                            format("^2You have successfully protected ^1country: {}\n", player.country_name)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                    user_cmd[0],
                                                    get_player_information(pid, false, "Protected country"))};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        string command{main_app.get_user_defined_protect_country_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "protect-country", format("{}\\{}\\n/a", main_app.get_username(), player.country_name),
                            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                            false);
                    }
                }
            }
            else
            {
                const string country_name{stl::helper::trim(str_join(cbegin(user_cmd) + 1, cend(user_cmd), " "))};
                if (!protected_countries.contains(country_name))
                {
                    protected_countries.emplace(country_name);
                    save_protected_entries_file(main_app.get_protected_countries_file_path(), protected_countries);
                    bool is_country_already_protected{};
                    auto &players_data = main_app.get_current_game_server().get_players_data();
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        auto &player = players_data[i];
                        if (player.country_name == country_name)
                        {
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                            main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = country_name;

                            const string re_msg{
                                format("^2You have successfully protected ^1country: {}\n", user_cmd[1])};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{
                                format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                       get_player_information(player.pid, false, "Protected country"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_protect_country_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            is_country_already_protected = true;
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "protect-country", format("{}\\{}\\n/a", main_app.get_username(), country_name), true,
                                main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                            break;
                        }
                    }

                    if (!is_country_already_protected)
                    {
                        player player_offline{};
                        player_offline.pid = -1;
                        strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                        main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = country_name;
                        string command{main_app.get_user_defined_protect_country_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        const string re_msg{format("^2You have successfully protected ^1country: {}\n", country_name)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        const string message{
                            format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                   get_player_information_for_player(player_offline, "Protected country"))};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "protect-country", format("{}\\{}\\n/a", main_app.get_username(), country_name), true,
                            main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!unprotectcountry"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            auto &protected_countries = main_app.get_current_game_server().get_protected_countries();
            string reason{remove_disallowed_characters_in_string(
                user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ") : "not specified")};
            stl::helper::trim_in_place(reason);
            if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
            {
                auto &player = main_app.get_current_game_server().get_player_data(pid);
                if (pid == player.pid)
                {
                    if (protected_countries.contains(player.country_name))
                    {
                        protected_countries.erase(player.country_name);
                        save_protected_entries_file(main_app.get_protected_countries_file_path(), protected_countries);
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                        main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = player.country_name;
                        const string re_msg{
                            format("^2You have successfully removed protected ^1country: {}\n", player.country_name)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                        const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                    user_cmd[0],
                                                    get_player_information(pid, false, "Unprotected country"))};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        string command{main_app.get_user_defined_unprotect_country_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "unprotect-country",
                            format("{}\\{}\\{}", main_app.get_username(), player.country_name, reason), true,
                            main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                    }
                }
            }
            else
            {
                const string country_name{stl::helper::trim(str_join(cbegin(user_cmd) + 1, cend(user_cmd), " "))};
                if (protected_countries.contains(country_name))
                {
                    protected_countries.erase(country_name);
                    save_protected_entries_file(main_app.get_protected_countries_file_path(), protected_countries);
                    bool is_city_already_protected{};
                    auto &players_data = main_app.get_current_game_server().get_players_data();
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        auto &player = players_data[i];
                        if (player.country_name == country_name)
                        {
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player.player_name;
                            main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = country_name;
                            main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                            const string re_msg{
                                format("^2You have successfully removed protected ^1country: {}\n", country_name)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            const string message{
                                format("^3You have successfully executed ^5{} ^3on player ({}^3)\n", user_cmd[0],
                                       get_player_information(player.pid, false, "Unprotected country"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_unprotect_country_message()};
                            build_tiny_rcon_message(command);
                            rcon_say(command);
                            is_city_already_protected = true;
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "unprotect-country",
                                format("{}\\{}\\{}", main_app.get_username(), country_name, reason), true,
                                main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                            break;
                        }
                    }

                    if (!is_city_already_protected)
                    {
                        player player_offline{};
                        player_offline.pid = -1;
                        strcpy_s(player_offline.player_name, std::size(player_offline.player_name), "John Doe");
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = player_offline.player_name;
                        main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = country_name;
                        string command{main_app.get_user_defined_unprotect_country_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        const string re_msg{
                            format("^2You have successfully removed protected ^1country: {}\n", country_name)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "unprotect-country", format("{}\\{}\\n/a", main_app.get_username(), country_name), true,
                            main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                    }
                }
            }
        }
    });

    main_app.add_message_handler("add-muted-ip", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5)
        {
            if (main_app.get_muted_players_map().contains(parts[0]))
            {
                player &pd = main_app.get_muted_players_map().at(parts[0]);
                pd.ip_address = parts[0];
                strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
                strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
                strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
                pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
                pd.reason = remove_disallowed_characters_in_string(parts[4]);
                pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : "^1Admin";
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

                // auto &muted_players_vector = main_app.get_muted_players_vector();
                // const auto found_iter = find_if(begin(muted_players_vector), end(muted_players_vector),
                // [&parts](const player &p) { return p.ip_address == parts[0]; }); if (found_iter !=
                // end(muted_players_vector)) {
                //   *found_iter = pd;
                // } else {
                // muted_players_vector.emplace_back(pd);
                // }
            }
            else
            {
                player pd{};
                pd.ip_address = parts[0];
                strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
                strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
                strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
                pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
                pd.reason = remove_disallowed_characters_in_string(parts[4]);
                pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : "^1Admin";
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

                main_app.get_muted_players_map().emplace(parts[0], pd);
                // main_app.get_muted_players_vector().emplace_back(pd);
            }

            player &pd{main_app.get_muted_players_map().at(parts[0])};

            const string msg{
                format("^7{} ^5has muted the ^1IP address ^5of player:\n^3Name: ^7{} ^5| ^3IP address: ^1{} ^5| "
                       "^3geoinfo: ^1{}, {} ^5| ^3Date of getting muted: ^1{}\n^3Reason: ^1{} ^5| ^3Muted by: ^7{}\n",
                       pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.country_name, pd.city,
                       pd.banned_date_time, pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            // add_muted_ip_address(pd, main_app.get_current_game_server().get_muted_ip_addresses());
            save_muted_players_data_to_file(main_app.get_muted_players_file_path(), main_app.get_muted_players_map());
        }
    });

    main_app.add_message_handler("remove-muted-ip", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5 /* && main_app.get_muted_players_map().contains(parts[0])*/)
        {
            player pd{};
            pd.ip_address = parts[0];
            strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
            strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
            pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
            pd.reason = remove_disallowed_characters_in_string(parts[4]);
            pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
            const string removed_by{parts.size() >= 7 ? parts[6] : "^1Admin"};
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

            const string msg{
                format("^7{} ^5has unmuted ^1IP address {} ^5of player:\n^3Name: ^7{} ^5| ^3geoinfo: ^1{}, {} ^5| "
                       "^3Date of getting muted: ^1{}\n^3Reason of mute: ^1{} ^5| ^3Muted by: ^7{}\n",
                       removed_by, pd.ip_address, pd.player_name, pd.country_name, pd.city, pd.banned_date_time,
                       pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            string ip_address{pd.ip_address};
            string message_about_removal;
            remove_muted_ip_address(ip_address, message_about_removal, false);
        }
    });

    main_app.add_command_handler({"!br", "!banrange"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!br", user_cmd[1]))
            {
                string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                     "^3for ^1'{} {}' ^3user command.",
                                     user_cmd[0], user_cmd[1])};
                stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
                if (int pid{-1}; is_valid_decimal_whole_number(user_cmd[1], pid))
                {
                    auto &player = main_app.get_current_game_server().get_player_data(pid);
                    if (pid == player.pid)
                    {
                        string error_msg;
                        if (check_if_player_is_protected(player, user_cmd[0].c_str(), error_msg))
                        {
                            print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str());
                            return;
                        }
                        unsigned long ip_key{};
                        const string ip_address_range{
                            get_narrow_ip_address_range_for_specified_ip_address(player.ip_address)};
                        if (check_ip_address_validity(player.ip_address, ip_key) &&
                            !main_app.get_current_game_server().get_banned_ip_address_ranges_map().contains(
                                ip_address_range))
                        {
                            main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                            main_app.get_tinyrcon_dict()["{IP_ADDRESS_RANGE}"] = ip_address_range;
                            player.banned_start_time = get_current_time_stamp();
                            const string datetimeinfo{
                                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", player.banned_start_time)};
                            strcpy_s(player.banned_date_time, std::size(player.banned_date_time), datetimeinfo.c_str());
                            main_app.get_tinyrcon_dict()["{BAN_DATE}"] = datetimeinfo;
                            string reason{remove_disallowed_characters_in_string(
                                user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                    : "not specified")};
                            stl::helper::trim_in_place(reason);
                            const string re_msg{
                                format("^2You have successfully banned IP address: ^1{}\n", player.ip_address)};
                            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                            const string message{format("^2You have successfully executed ^1{} ^2on player ({}^3)\n",
                                                        user_cmd[0],
                                                        get_player_information(pid, false, "Banned IP address"))};
                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::yes,
                                               is_log_datetime::yes);
                            string command{main_app.get_user_defined_ip_address_range_ban_message()};
                            build_tiny_rcon_message(command);
                            player.reason = reason;
                            player.banned_by_user_name = main_app.get_username();
                            player.ip_address = ip_address_range;
                            main_app.get_current_game_server().get_banned_ip_address_ranges_map().emplace(
                                ip_address_range, player);
                            main_app.get_current_game_server().get_banned_ip_address_ranges_vector().emplace_back(
                                player);
                            save_banned_ip_address_range_entries_to_file(
                                main_app.get_ip_range_bans_file_path(),
                                main_app.get_current_game_server().get_banned_ip_address_ranges_vector());
                            auto &current_user = main_app.get_user_for_name(main_app.get_username());
                            current_user->no_of_iprangebans++;
                            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                            main_app.get_connection_manager_for_messages().process_and_send_message(
                                "add-iprangeban",
                                format(R"({}\{}\{}\{}\{}\{})", ip_address_range, player.guid_key, player.player_name,
                                       player.banned_date_time, player.reason, player.banned_by_user_name),
                                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                                false);
                            kick_player(pid, command);
                        }
                    }
                }
                else
                {
                    if (main_app.get_current_game_server().get_protected_ip_address_ranges().contains(user_cmd[1]))
                    {
                        const string error_msg{format("^3Cannot execute command ^1{} ^3on player whose ^1IP address "
                                                      "range ^3is ^1protected.\n^5Remove the protected ^1IP address "
                                                      "range ^5first by using the ^1!unprotectiprange {} ^5command.",
                                                      user_cmd[0], user_cmd[1])};
                        print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str());
                        return;
                    }
                    if (!main_app.get_current_game_server().get_banned_ip_address_ranges_map().contains(user_cmd[1]))
                    {
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{IP_ADDRESS_RANGE}"] = user_cmd[1];
                        string reason{remove_disallowed_characters_in_string(
                            user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                : "not specified")};
                        stl::helper::trim_in_place(reason);
                        const string re_msg{
                            format("^2You have successfully banned IP address range: ^1{}\n", user_cmd[1])};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                        const string message{format("^2You have successfully executed "
                                                    "^1{} ^2on IP address range: ^1{}\n",
                                                    user_cmd[0], user_cmd[1])};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        player player{};
                        strcpy_s(player.player_name, std::size(player.player_name), "John Doe");
                        player.ip_address = user_cmd[1];
                        const string datetimeinfo{get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}")};
                        strcpy_s(player.banned_date_time, std::size(player.banned_date_time), datetimeinfo.c_str());
                        main_app.get_tinyrcon_dict()["{BAN_DATE}"] = datetimeinfo;
                        player.reason = reason;
                        player.banned_by_user_name = main_app.get_username();
                        main_app.get_current_game_server().get_banned_ip_address_ranges_map().emplace(user_cmd[1],
                                                                                                      player);
                        main_app.get_current_game_server().get_banned_ip_address_ranges_vector().emplace_back(player);
                        string command{main_app.get_user_defined_ip_address_range_ban_message()};
                        build_tiny_rcon_message(command);
                        rcon_say(command);
                        save_banned_ip_address_range_entries_to_file(
                            main_app.get_ip_range_bans_file_path(),
                            main_app.get_current_game_server().get_banned_ip_address_ranges_vector());
                        auto &current_user = main_app.get_user_for_name(main_app.get_username());
                        current_user->no_of_iprangebans++;
                        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "add-iprangeban",
                            format(R"({}\0\{}\{}\{}\{})", user_cmd[1], player.player_name, player.banned_date_time,
                                   player.reason, main_app.get_username()),
                            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                            false);
                    }
                }
            }
        }
    });

    main_app.add_command_handler({"!bn", "!banname"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            const size_t pid_needle_pos{user_cmd[1].find("pid:")};

            if (int pid{-1}; pid_needle_pos != string::npos &&
                             is_valid_decimal_whole_number(user_cmd[1].substr(pid_needle_pos + len("pid:")), pid))
            {
                auto &player = main_app.get_current_game_server().get_player_data(pid);
                if (pid == player.pid)
                {
                    string error_msg;
                    if (check_if_player_is_protected(player, user_cmd[0].c_str(), error_msg))
                    {
                        print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str());
                        return;
                    }
                    string player_name{player.player_name};
                    trim_in_place(player_name);
                    string banned_player_name{player_name};
                    remove_all_color_codes(banned_player_name);
                    trim_in_place(banned_player_name);
                    to_lower_case_in_place(banned_player_name);
                    if (banned_player_name.length() > 32)
                        banned_player_name.erase(cbegin(banned_player_name) + 32, cend(banned_player_name));
                    if (!banned_player_name.empty() &&
                        !main_app.get_current_game_server().get_banned_names_map().contains(banned_player_name))
                    {
                        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                        main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = banned_player_name;
                        player.banned_start_time = get_current_time_stamp();
                        const string datetimeinfo{
                            get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", player.banned_start_time)};
                        strcpy_s(player.banned_date_time, std::size(player.banned_date_time), datetimeinfo.c_str());
                        main_app.get_tinyrcon_dict()["{BAN_DATE}"] = datetimeinfo;
                        string reason{remove_disallowed_characters_in_string(
                            user_cmd.size() > 2 ? str_join(cbegin(user_cmd) + 2, cend(user_cmd), " ")
                                                : "not specified")};
                        stl::helper::trim_in_place(reason);
                        const string re_msg{
                            format("^2You have successfully banned player name: ^1{}\n", banned_player_name)};
                        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                        const string message{format("^2You have successfully executed ^1{} ^2on player ({}^3)\n",
                                                    user_cmd[0],
                                                    get_player_information(pid, false, "Banned player name"))};
                        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                        string command{main_app.get_user_defined_name_ban_message()};
                        build_tiny_rcon_message(command);
                        strcpy_s(player.player_name, size(player.player_name), banned_player_name.c_str());
                        player.reason = reason;
                        player.banned_by_user_name = main_app.get_username();
                        player.ip_address = player.ip_address;
                        main_app.get_current_game_server().get_banned_names_map().emplace(banned_player_name, player);
                        main_app.get_current_game_server().get_banned_names_vector().emplace_back(player);
                        save_banned_ip_entries_to_file(main_app.get_banned_names_file_path(),
                                                       main_app.get_current_game_server().get_banned_names_vector());
                        auto &current_user = main_app.get_user_for_name(main_app.get_username());
                        current_user->no_of_namebans++;
                        save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "add-nameban",
                            format(R"({}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, banned_player_name,
                                   player.banned_date_time, player.reason, player.banned_by_user_name),
                            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                            false);
                        kick_player(pid, command);
                    }
                }
            }
            else
            {
                string banned_player_name{str_join(cbegin(user_cmd) + 1, cend(user_cmd), " ")};
                remove_all_color_codes(banned_player_name);
                trim_in_place(banned_player_name);
                to_lower_case_in_place(banned_player_name);
                if (banned_player_name.length() > 32)
                    banned_player_name.erase(cbegin(banned_player_name) + 32, cend(banned_player_name));

                if (!main_app.get_current_game_server().get_banned_names_map().contains(banned_player_name))
                {
                    main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                    main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = banned_player_name;
                    string reason{"not specified"};
                    const string re_msg{
                        format("^2You have successfully banned player name: ^1{}\n", banned_player_name)};
                    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                    main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                    const string message{format("^2You have successfully executed ^1{} ^2on player name: ^1{}\n",
                                                user_cmd[0], banned_player_name)};
                    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                    player p{};
                    strcpy_s(p.player_name, size(p.player_name), banned_player_name.c_str());
                    p.ip_address = "123.123.123.123";
                    convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), p.ip_address,
                                                     p);
                    const string datetimeinfo{get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}")};
                    strcpy_s(p.banned_date_time, std::size(p.banned_date_time), datetimeinfo.c_str());
                    main_app.get_tinyrcon_dict()["{BAN_DATE}"] = datetimeinfo;
                    p.reason = reason;
                    p.banned_by_user_name = main_app.get_username();
                    main_app.get_current_game_server().get_banned_names_map().emplace(banned_player_name, p);
                    main_app.get_current_game_server().get_banned_names_vector().emplace_back(p);
                    string command{main_app.get_user_defined_name_ban_message()};
                    build_tiny_rcon_message(command);
                    rcon_say(command);
                    save_banned_ip_entries_to_file(main_app.get_banned_names_file_path(),
                                                   main_app.get_current_game_server().get_banned_names_vector());
                    auto &current_user = main_app.get_user_for_name(main_app.get_username());
                    current_user->no_of_namebans++;
                    save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                    main_app.get_connection_manager_for_messages().process_and_send_message(
                        "add-nameban",
                        format(R"({}\0\{}\{}\{}\{})", p.ip_address, banned_player_name, p.banned_date_time, p.reason,
                               main_app.get_username()),
                        true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                }
            }
        }
    });

    main_app.add_command_handler({"!ubn", "!unbanname"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

            string message;
            string banned_player_name{str_join(cbegin(user_cmd) + 1, cend(user_cmd), " ")};
            remove_all_color_codes(banned_player_name);
            trim_in_place(banned_player_name);
            to_lower_case_in_place(banned_player_name);
            if (banned_player_name.length() > 32)
                banned_player_name.erase(cbegin(banned_player_name) + 32, cend(banned_player_name));

            player pd{};
            strcpy_s(pd.player_name, size(pd.player_name), banned_player_name.c_str());
            auto status =
                remove_permanently_banned_player_name(pd, main_app.get_current_game_server().get_banned_names_vector(),
                                                      main_app.get_current_game_server().get_banned_names_map());

            if (status)
            {
                const string re_msg{format("^2You have successfully removed "
                                           "previously banned ^1player name: ^5{}\n",
                                           banned_player_name)};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "remove-nameban",
                    format(R"({}\{}\{}\{}\{}\{}\{})", pd.ip_address, pd.guid_key, banned_player_name,
                           pd.banned_date_time, pd.reason, pd.banned_by_user_name, main_app.get_username()),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
            else
            {
                const string re_msg{
                    format("^3Provided player name (^1{}^3) hasn't been ^1banned ^3yet!\n", banned_player_name)};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains("!unbanname"))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"!stats"}, [](const vector<string> &) {
        // if (!validate_admin_and_show_missing_admin_privileges_message(false))
        // return;

        string ex_msg{"^1Exception ^3thrown from ^1command handler ^3for the "
                      "^1!stats ^3user command."};
        stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

        main_app.get_connection_manager_for_messages().process_and_send_message(
            "request-welcome-message",
            format("{}\\{}\\{}", main_app.get_username(), main_app.get_user_ip_address(), get_current_time_stamp()),
            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    });

    main_app.add_command_handler({"!egb"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        main_app.set_is_automatic_city_kick_enabled(true);
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
        const string message{format("^2You have successfully executed ^5{}\n", user_cmd[0])};
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        string rcon_message{main_app.get_user_defined_enable_city_ban_feature_msg()};
        build_tiny_rcon_message(rcon_message);
        rcon_say(rcon_message, true);
    });

    main_app.add_command_handler({"!dgb"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        main_app.set_is_automatic_city_kick_enabled(false);
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
        const string message{format("^2You have successfully executed ^5{}\n", user_cmd[0])};
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        string rcon_message{main_app.get_user_defined_disable_city_ban_feature_msg()};
        build_tiny_rcon_message(rcon_message);
        rcon_say(rcon_message, true);
    });

    main_app.add_command_handler({"!bancity"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            const string banned_city{trim(str_join(user_cmd.cbegin() + 1, user_cmd.cend(), " "))};
            player pd{};
            pd.city = banned_city.c_str();
            string error_msg;
            if (check_if_player_is_protected(pd, "!bancity", error_msg))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str());
                return;
            }
            if (!main_app.get_current_game_server().get_banned_cities_set().contains(banned_city))
            {
                main_app.get_current_game_server().get_banned_cities_set().emplace(banned_city);
                save_banned_entries_to_file(main_app.get_banned_cities_file_path(),
                                            main_app.get_current_game_server().get_banned_cities_set());
                const string message{
                    format("^2You have successfully executed ^5{} ^2on city: ^1{}\n", user_cmd[0], banned_city)};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{CITY_NAME}"] = banned_city;
                string rcon_message{main_app.get_user_defined_city_ban_msg()};
                build_tiny_rcon_message(rcon_message);
                rcon_say(rcon_message, true);
                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                current_user->no_of_citybans++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "add-cityban", format("{}\\{}\\{}", banned_city, main_app.get_username(), get_current_time_stamp()),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   format("^3City ^1{} ^3is already banned.", banned_city).c_str());
            }
        }
    });
    main_app.add_command_handler({"!unbancity"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            const string banned_city_to_unban{trim(str_join(user_cmd.cbegin() + 1, user_cmd.cend(), " "))};
            if (main_app.get_current_game_server().get_banned_cities_set().contains(banned_city_to_unban))
            {
                main_app.get_current_game_server().get_banned_cities_set().erase(banned_city_to_unban);
                save_banned_entries_to_file(main_app.get_banned_cities_file_path(),
                                            main_app.get_current_game_server().get_banned_cities_set());
                const string message{
                    format("^2You have successfully executed ^5{} ^2on city: ^1\n", user_cmd[0], banned_city_to_unban)};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{CITY_NAME}"] = banned_city_to_unban;
                string rcon_message{main_app.get_user_defined_city_unban_msg()};
                build_tiny_rcon_message(rcon_message);
                rcon_say(rcon_message, true);
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "remove-cityban",
                    format("{}\\{}\\{}", banned_city_to_unban, main_app.get_username(), get_current_time_stamp()), true,
                    main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   format("^2City ^1{} ^2is not banned.", banned_city_to_unban).c_str());
            }
        }
    });

    main_app.add_command_handler({"!ecb"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        // if (!main_app.get_game_server().get_is_automatic_country_kick_enabled())
        // {
        main_app.set_is_automatic_country_kick_enabled(true);
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
        const string message{format("^2You have successfully executed ^5{}\n", user_cmd[0])};
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        string rcon_message{main_app.get_user_defined_enable_country_ban_feature_msg()};
        build_tiny_rcon_message(rcon_message);
        rcon_say(rcon_message, true);
        // }
    });

    main_app.add_command_handler({"!dcb"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        // if (main_app.get_game_server().get_is_automatic_country_kick_enabled()) {
        main_app.set_is_automatic_country_kick_enabled(false);
        write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
        const string message{format("^2You have successfully executed ^5{}\n", user_cmd[0])};
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
        string rcon_message{main_app.get_user_defined_disable_country_ban_feature_msg()};
        build_tiny_rcon_message(rcon_message);
        rcon_say(rcon_message, true);
        // }
    });

    main_app.add_command_handler({"!bancountry"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            const string banned_country{trim(str_join(user_cmd.cbegin() + 1, user_cmd.cend(), " "))};
            player pd{};
            pd.city = banned_country.c_str();
            string error_msg;
            if (check_if_player_is_protected(pd, "!bancountry", error_msg))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, error_msg.c_str());
                return;
            }
            if (!main_app.get_current_game_server().get_banned_countries_set().contains(banned_country))
            {
                main_app.get_current_game_server().get_banned_countries_set().emplace(banned_country);
                save_banned_entries_to_file(main_app.get_banned_countries_file_path(),
                                            main_app.get_current_game_server().get_banned_countries_set());
                const string message{
                    format("^2You have successfully executed ^5{} ^2on country: ^1{}\n", user_cmd[0], banned_country)};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = banned_country;
                string rcon_message{main_app.get_user_defined_country_ban_msg()};
                build_tiny_rcon_message(rcon_message);
                rcon_say(rcon_message, true);
                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                current_user->no_of_countrybans++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "add-countryban",
                    format("{}\\{}\\{}", banned_country, main_app.get_username(), get_current_time_stamp()), true,
                    main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   format("^3Country ^1{} ^3is already banned.", banned_country).c_str());
            }
        }
    });

    main_app.add_command_handler({"!unbancountry"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (user_cmd.size() > 1 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            const string banned_country_to_unban{trim(str_join(user_cmd.cbegin() + 1, user_cmd.cend(), " "))};
            if (main_app.get_current_game_server().get_banned_countries_set().contains(banned_country_to_unban))
            {
                main_app.get_current_game_server().get_banned_countries_set().erase(banned_country_to_unban);
                save_banned_entries_to_file(main_app.get_banned_countries_file_path(),
                                            main_app.get_current_game_server().get_banned_countries_set());
                const string message{format("^2You have successfully executed ^5{} ^2on country: ^1{}\n", user_cmd[0],
                                            banned_country_to_unban)};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{COUNTRY_NAME}"] = banned_country_to_unban;
                string rcon_message{main_app.get_user_defined_country_unban_msg()};
                build_tiny_rcon_message(rcon_message);
                rcon_say(rcon_message, true);
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "remove-countryban",
                    format("{}\\{}\\{}", banned_country_to_unban, main_app.get_username(), get_current_time_stamp()),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   format("^2Country ^1{} ^2is not banned.", banned_country_to_unban).c_str());
            }
        }
    });

    main_app.add_command_handler({"status", "!status"},
                                 [](const vector<string> &) { initiate_sending_rcon_status_command_now(); });

    main_app.add_command_handler({"gs", "!gs", "getstatus", "!getstatus"},
                                 [](const vector<string> &) { initiate_sending_rcon_status_command_now(); });

    main_app.add_command_handler({"getinfo"}, [](const vector<string> &) {
        string rcon_reply;
        main_app.get_connection_manager().send_and_receive_non_rcon_data(
            "getinfo", rcon_reply, main_app.get_current_game_server().get_server_ip_address().c_str(),
            main_app.get_current_game_server().get_server_port(), main_app.get_current_game_server(), true, true);
    });

    main_app.add_command_handler({"sort", "!sort"}, [](const vector<string> &user_cmd) {
        if (user_cmd.size() == 3)
        {
            sort_type new_sort_type{sort_type::pid_asc};
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
            else
            {
                const string re_msg{format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                if (user_commands_help.contains(user_cmd[0]))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                    print_colored_text(app_handles.hwnd_re_messages_data, "\n",
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::no);
                }
            }

            is_process_combobox_item_selection_event = false;
            process_sort_type_change_request(new_sort_type);
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"muted", "!muted"}, [](const vector<string> &user_cmd) {
        int number{};
        if ((user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
        {
            number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos
                                                                                : static_cast<size_t>(number));
        }
        else
        {
            number_of_entries_to_display.store(25);
        }
        is_display_muted_players_data_event.store(true);
    });

    main_app.add_command_handler({"tempbans", "!tempbans"}, [](const vector<string> &user_cmd) {
        int number{};
        if ((user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
        {
            number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos
                                                                                : static_cast<size_t>(number));
        }
        else
        {
            number_of_entries_to_display.store(25);
        }
        is_display_temporarily_banned_players_data_event.store(true);
    });

    main_app.add_command_handler({"bans", "!bans"}, [](const vector<string> &user_cmd) {
        int number{};
        if ((user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
        {
            number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos
                                                                                : static_cast<size_t>(number));
        }
        else
        {
            number_of_entries_to_display.store(25);
        }
        is_display_permanently_banned_players_data_event.store(true);
    });

    main_app.add_command_handler({"ranges", "!ranges"}, [](const vector<string> &user_cmd) {
        int number{};
        if ((user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
        {
            number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos
                                                                                : static_cast<size_t>(number));
        }
        else
        {
            number_of_entries_to_display.store(25);
        }
        is_display_banned_ip_address_ranges_data_event.store(true);
    });

    main_app.add_command_handler({"names", "!names"}, [](const vector<string> &user_cmd) {
        int number{};
        if ((user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
        {
            number_of_entries_to_display.store(user_cmd[1] == "all" || number <= 0 ? string::npos
                                                                                : static_cast<size_t>(number));
        }
        else
        {
            number_of_entries_to_display.store(25);
        }
        is_display_banned_player_names_data_event.store(true);
    });

    main_app.add_command_handler({"admins", "!admins"},
                                 [](const vector<string> &) { is_display_admins_data_event.store(true); });

    main_app.add_command_handler({"!bannedcities"},
                                 [](const vector<string> &) { is_display_banned_cities_data_event.store(true); });

    main_app.add_command_handler({"!bannedcountries"},
                                 [](const vector<string> &) { is_display_banned_countries_data_event.store(true); });

    main_app.add_command_handler({"!banned"}, [](const vector<string> &user_cmd) {
        if (user_cmd.size() == 2)
        {
            if (user_cmd[1] == "cities")
                is_display_banned_cities_data_event.store(true);
            else if (user_cmd[1] == "countries")
                is_display_banned_countries_data_event.store(true);
        }
    });

    /*main_app.add_command_handler({ "!savestats" }, [](const vector<string>&
    user_cmd)
            {
                    const string stats_dat_file_path{ user_cmd.size() >= 2 &&
    !user_cmd[1].empty() ? format("data\\{}.dat", user_cmd[1]) :
    "data\\player_stats.dat"s };
                    save_players_stats_data(stats_dat_file_path.c_str(),
    main_app.get_stats_data().get_scores_vector(),
    main_app.get_stats_data().get_scores_map()); });

    main_app.add_command_handler({ "!rsfp", "!remove_stats_for_player" }, [](const
    vector<string>& user_cmd)
            {
                    if (user_cmd.size() >= 2 && !user_cmd[1].empty()) {
                            const string player_name_index{
    get_cleaned_user_name(str_join(cbegin(user_cmd) + 1, cend(user_cmd), " ")) };
                            const string status_msg{
    remove_stats_for_player_name(player_name_index) ? format("^2Successfully
    removed all player stats data for ^2'^7{}^2'!", user_cmd[1]) : format("^3Could
    not find any player stats data for ^3'^7{}^3'!", user_cmd[1]) };
                            print_colored_text(app_handles.hwnd_re_messages_data,
    status_msg.c_str(), is_append_message_to_richedit_control::yes,
    is_log_message::yes, is_log_datetime::yes); } });*/

    /*main_app.add_command_handler({ "tp", "!tp" }, [](const vector<string>&
    user_cmd)
            {
                    const size_t number_of_top_players_to_display = [&]() ->
    size_t { if (int number{}; (user_cmd.size() >= 2u) &&
    (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
    { return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 25u :
    static_cast<size_t>(number);
                            }
                            return 25u;
                            }();

                            process_topplayers_request(format("!tp\\{}",
    number_of_top_players_to_display)); });

    main_app.add_command_handler({ "tpy", "!tpy" }, [](const vector<string>&
    user_cmd)
            {
                    const size_t number_of_top_players_to_display = [&]() ->
    size_t { if (int number{}; (user_cmd.size() >= 2u) &&
    (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
    { return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 25u :
    static_cast<size_t>(number);
                            }
                            return 25u;
                            }();

                            process_topplayers_request(format("!tpy\\{}",
    number_of_top_players_to_display)); });

    main_app.add_command_handler({ "tpm", "!tpm" }, [](const vector<string>&
    user_cmd)
            {
                    const size_t number_of_top_players_to_display = [&]() ->
    size_t { if (int number{}; (user_cmd.size() >= 2u) &&
    (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
    { return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 25u :
    static_cast<size_t>(number);
                            }
                            return 25u;
                            }();

                            process_topplayers_request(format("!tpm\\{}",
    number_of_top_players_to_display)); });

    main_app.add_command_handler({ "tpd", "!tpd" }, [](const vector<string>&
    user_cmd)
            {
                    const size_t number_of_top_players_to_display = [&]()  ->
    size_t { if (int number{}; (user_cmd.size() >= 2u) &&
    (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
    { return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 25u :
    static_cast<size_t>(number);
                            }
                            return 25u;
                            }();

                            process_topplayers_request(format("!tpd\\{}",
    number_of_top_players_to_display)); });*/

    main_app.add_command_handler({"!s"}, [](const vector<string> &user_cmd) {
        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
                "request-topplayers", format("!s\\{}", str_join(cbegin(user_cmd) + 1, cend(user_cmd), " ")), true,
                main_app.get_private_tiny_rcon_server_ip_address(), main_app.get_private_tiny_rcon_server_port(),
                false);
        }
    });

    main_app.add_command_handler({"tp", "!tp"}, [](const vector<string> &user_cmd) {
        const size_t number_of_top_players_to_display = [&]() -> size_t {
            if (int number{};
                (user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
            {
                return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 50 : static_cast<size_t>(number);
            }
            return 50;
        }();

        main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
            "request-topplayers", format("!tp\\{}", number_of_top_players_to_display), true,
            main_app.get_private_tiny_rcon_server_ip_address(), main_app.get_private_tiny_rcon_server_port(), false);
    });

    main_app.add_command_handler({"tpy", "!tpy"}, [](const vector<string> &user_cmd) {
        const size_t number_of_top_players_to_display = [&]() -> size_t {
            if (int number{};
                (user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
            {
                return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 50 : static_cast<size_t>(number);
            }
            return 50;
        }();

        main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
            "request-topplayers", format("!tpy\\{}", number_of_top_players_to_display), true,
            main_app.get_private_tiny_rcon_server_ip_address(), main_app.get_private_tiny_rcon_server_port(), false);
    });

    main_app.add_command_handler({"tpm", "!tpm"}, [](const vector<string> &user_cmd) {
        const size_t number_of_top_players_to_display = [&]() -> size_t {
            if (int number{};
                (user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
            {
                return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 50 : static_cast<size_t>(number);
            }
            return 50;
        }();

        main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
            "request-topplayers", format("!tpm\\{}", number_of_top_players_to_display), true,
            main_app.get_private_tiny_rcon_server_ip_address(), main_app.get_private_tiny_rcon_server_port(), false);
    });

    main_app.add_command_handler({"tpd", "!tpd"}, [](const vector<string> &user_cmd) {
        const size_t number_of_top_players_to_display = [&]() -> size_t {
            if (int number{};
                (user_cmd.size() >= 2u) && (is_valid_decimal_whole_number(user_cmd[1], number) || user_cmd[1] == "all"))
            {
                return user_cmd[1] == "all" || number <= 0 || number > 1000 ? 50 : static_cast<size_t>(number);
            }
            return 50;
        }();

        main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
            "request-topplayers", format("!tpd\\{}", number_of_top_players_to_display), true,
            main_app.get_private_tiny_rcon_server_ip_address(), main_app.get_private_tiny_rcon_server_port(), false);
    });

    main_app.add_command_handler({"!ub", "!unban"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            unsigned long ip_key{};
            if (!check_ip_address_validity(user_cmd[1], ip_key) &&
                !check_if_user_provided_argument_is_valid_for_specified_command("!unban", user_cmd[1]))
            {
                const string re_msg{format("^5Provided IP address ^1{} ^5is not a valid IP address or there "
                                           "isn't a banned player entry whose ^1serial no. ^5is equal to "
                                           "provided number: ^1{}!\n",
                                           user_cmd[1], user_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
            else
            {
                string message;
                int no{-1};
                if (!is_valid_decimal_whole_number(user_cmd[1], no))
                {
                    auto [status, player]{remove_temp_banned_ip_address(user_cmd[1], message, false, true)};
                    if (status)
                    {
                        main_app.get_connection_manager_for_messages().process_and_send_message(
                            "remove-tempban",
                            format(R"({}\{}\{}\{}\{}\{}\{}\{})", player.ip_address, player.player_name,
                                   player.banned_date_time, player.banned_start_time, player.ban_duration_in_hours,
                                   player.reason, player.banned_by_user_name, main_app.get_username()),
                            true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(),
                            false);
                        const string private_msg{format("^1Admin ^7{} ^2has successfully removed ^1temporarily banned "
                                                        "IP address: ^5{}\n ^3Player name: ^7{} ^5| ^3geoinfo: ^1{}, "
                                                        "{} ^5| ^3Date of tempban: ^1{}\n ^3Tempban duration: ^1{} "
                                                        "hours ^5|^5Reason: ^1{} ^5| ^3Banned by: ^1{}\n",
                                                        main_app.get_username(), user_cmd[1], player.player_name,
                                                        player.country_name, player.city, player.banned_date_time,
                                                        player.ban_duration_in_hours, player.reason,
                                                        player.banned_by_user_name)};
                        const string inform_msg{format("{}\\{}\\{}", main_app.get_username(), message, private_msg)};
                        main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                    }
                    else
                    {
                        const string re_msg{format("^1Admin ^7{} ^3failed to remove ^1temporarily banned IP "
                                                   "address: ^5{}\n^1Reason: ^3Provided IP address (^1{}^3) "
                                                   "hasn't been ^1temporarily banned ^3yet.\n",
                                                   main_app.get_username(), user_cmd[1], user_cmd[1])};
                        const string inform_msg{format("{}\\{}", main_app.get_username(), re_msg)};
                        main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                    }
                }

                string ip_address{user_cmd[1]};
                auto [status, player]{remove_permanently_banned_ip_address(ip_address, message, true)};
                if (status)
                {
                    main_app.get_connection_manager_for_messages().process_and_send_message(
                        "remove-ipban",
                        format(R"({}\{}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, player.player_name,
                               player.banned_date_time, player.reason, player.banned_by_user_name,
                               main_app.get_username()),
                        true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                    const string private_msg{format("^1Admin ^7{} ^2has successfully removed ^1permanently banned IP "
                                                    "address: ^5{} ^2for player name: ^7{}\n",
                                                    main_app.get_username(), ip_address, player.player_name)};
                    const string inform_msg{format("{}\\{}\\{}", main_app.get_username(), message, private_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                }
                else
                {
                    const string re_msg{format("^1Admin ^7{} ^3failed to remove ^1permanently banned IP "
                                               "address: ^5{}\n^1Reason: ^3Provided IP address (^1{}^3) hasn't "
                                               "been ^1permanently banned ^3yet.\n",
                                               main_app.get_username(), user_cmd[1], user_cmd[1])};
                    const string inform_msg{format("{}\\{}", main_app.get_username(), re_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                }
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"!ubr", "!unbanrange"}, [](const vector<string> &user_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 user_cmd[0], user_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            if (!check_if_user_provided_argument_is_valid_for_specified_command("!unbanrange", user_cmd[1]))
            {
                const string re_msg{format("^5Provided IP address ^1{} ^5is not a valid IP address or there "
                                           "isn't a banned player entry whose ^1serial no. ^5is equal to "
                                           "provided number: ^1{}!\n",
                                           user_cmd[1], user_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
            else
            {
                string message;

                const string &ip_address_range{user_cmd[1]};
                player pd{};
                pd.ip_address = ip_address_range;
                auto status = remove_permanently_banned_ip_address_range(
                    pd, main_app.get_current_game_server().get_banned_ip_address_ranges_vector(),
                    main_app.get_current_game_server().get_banned_ip_address_ranges_map());

                if (status)
                {
                    const string re_msg{format("^2You have successfully removed previously banned ^1IP address "
                                               "range: ^5{}\n",
                                               ip_address_range)};
                    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                    main_app.get_connection_manager_for_messages().process_and_send_message(
                        "remove-iprangeban",
                        format(R"({}\{}\{}\{}\{}\{}\{})", ip_address_range, pd.guid_key, pd.player_name,
                               pd.banned_date_time, pd.reason, pd.banned_by_user_name, main_app.get_username()),
                        true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                }
                else
                {
                    const string re_msg{format("^3Provided IP address range (^1{}^3) "
                                               "hasn't been ^1banned ^3yet!\n",
                                               ip_address_range)};
                    print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                }
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^1{}\n", user_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(user_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(user_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"!c", "!cp"}, [](const vector<string> &user_cmd) {
        const bool use_private_slot{user_cmd[0] == "!cp" &&
                                    !main_app.get_current_game_server().get_private_slot_password().empty()};
        const string &user_input{user_cmd[1]};
        smatch ip_port_match{};
        const string ip_port_server_address{
            (user_cmd.size() > 1 && regex_search(user_input, ip_port_match, ip_address_and_port_regex))
                ? (ip_port_match[1].str() + ":"s + ip_port_match[2].str())
                : (main_app.get_current_game_server().get_server_ip_address() + ":"s +
                   to_string(main_app.get_current_game_server().get_server_port()))};
        const size_t sep_pos{ip_port_server_address.find(':')};
        const string ip_address{ip_port_server_address.substr(0, sep_pos)};
        const uint16_t port_number{static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1)))};
        const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(
            ip_address.c_str(), port_number, main_app.get_current_game_server().get_rcon_password().c_str());

        const game_name_t game_name{
            result.second != game_name_t::unknown
                ? result.second
                : convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name())};

        connect_to_the_game_server(ip_port_server_address, game_name, use_private_slot, true);
    });

    // !std
    main_app.add_command_handler({"!std"}, [](const vector<string> &user_cmd) {
        string ex_msg{"^1Exception ^3thrown from ^1command handler ^3for ^1!stats ^3user command."};
        stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!std", user_cmd[1]))
            {
                const size_t std = stoul(user_cmd[1]);
                if (std != main_app.get_spec_time_delay())
                {
                    main_app.set_spec_time_delay(std);
                    write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
                }
            }
        }
    });

    // !spec
    main_app.add_command_handler({"!spec"}, [](const vector<string> &user_cmd) {
        string ex_msg{"^1Exception ^3thrown from ^1command handler ^3for ^1!stats ^3user command."};
        stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

        if (is_player_being_spectated.load())
        {
            selected_player = get_player_data_for_pid(spectated_player_pid.load());
            if (selected_player.pid != -1)
            {
                const string warning_message{format("^7{} ^3stopped spectating player ^7{} ^3(^1pid = {}^3).\n",
                                                    main_app.get_username(), selected_player.player_name,
                                                    selected_player.pid)};
                print_colored_text(app_handles.hwnd_re_messages_data, warning_message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes, true, true);
            }
            is_player_being_spectated.store(false);
            spectated_player_pid.store(-1);
            if (button_id_to_label_text.contains(ID_SPECTATEPLAYER))
                button_id_to_label_text.at(ID_SPECTATEPLAYER) = "Spectate player";
            SetWindowText(app_handles.hwnd_spectate_player_button, "Spectate player");
        }
        else if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            if (check_if_user_provided_argument_is_valid_for_specified_command("!spec", user_cmd[1]))
            {
                const int pid{stoi(user_cmd[1])};
                selected_player = get_player_data_for_pid(pid);
                if (-1 != selected_player.pid && selected_player.ip_address != main_app.get_user_ip_address())
                {
                    std::thread spectate_player_task{spectate_player_for_specified_pid, pid};
                    spectate_player_task.detach();
                }
            }
        }
    });

    // !nospec
    main_app.add_command_handler({"!nospec"}, [](const vector<string> &) {
        string ex_msg{"^1Exception ^3thrown from ^1command handler ^3for ^1!stats ^3user command."};
        stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
        if (is_player_being_spectated.load())
        {
            selected_player = get_player_data_for_pid(spectated_player_pid.load());
            if (selected_player.pid != -1)
            {
                const string warning_message{format("^7{} ^3stopped spectating player ^7{} ^3(^1pid = {}^3).\n",
                                                    main_app.get_username(), selected_player.player_name,
                                                    selected_player.pid)};
                print_colored_text(app_handles.hwnd_re_messages_data, warning_message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes, true, true);
            }

            is_player_being_spectated.store(false);
            spectated_player_pid.store(-1);
            if (button_id_to_label_text.contains(ID_SPECTATEPLAYER))
                button_id_to_label_text.at(ID_SPECTATEPLAYER) = "Spectate player";
            SetWindowText(app_handles.hwnd_spectate_player_button, "Spectate player");
        }
    });

    main_app.add_command_handler({"!rc"}, [](const vector<string> &user_cmd) {
        using namespace stl::helper;

        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            // if (!me->is_admin) return;
            if (!validate_admin_and_show_missing_admin_privileges_message(false))
                return;
            string rcon_command{stl::helper::to_lower_case(
                stl::helper::trim(stl::helper::str_join(cbegin(user_cmd) + 1, cend(user_cmd), " ")))};
            if (rcon_command.find("rcon_password") != string::npos ||
                rcon_command.find("sv_privatepassword") != string::npos)
            {
                const string warning_msg{format(
                    "^3Cannot send disallowed rcon command ^1'{}' ^3to currently viewed game server!\n", rcon_command)};
                print_colored_text(app_handles.hwnd_re_messages_data, warning_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes, true, true);
                return;
            }
            const bool is_hide_information_message{rcon_command.find("--silent") != string::npos ||
                                                   rcon_command.find("/S") != string::npos};
            if (!is_hide_information_message)
            {

                const string information{
                    format("^2Sending rcon command ^1'{}' ^2to currently viewed ^3game server.\n", rcon_command)};
                print_colored_text(app_handles.hwnd_re_messages_data, information.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes, true, true);
            }
            else
            {
                const size_t start_pos1{rcon_command.find("--silent")};
                if (string::npos != start_pos1)
                {

                    const size_t needle_len1{len("--silent")};
                    rcon_command.erase(start_pos1, needle_len1);
                }

                const size_t start_pos2{rcon_command.find("/S")};
                if (string::npos != start_pos2)
                {
                    const size_t needle_len2{len("/S")};
                    rcon_command.erase(start_pos2, needle_len2);
                }

                trim_in_place(rcon_command);
            }

            string reply;
            main_app.get_connection_manager().send_and_receive_rcon_data(
                rcon_command.c_str(), reply, main_app.get_current_game_server().get_server_ip_address().c_str(),
                main_app.get_current_game_server().get_server_port(),
                main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(),
                true, false);

            if (!is_hide_information_message && reply.find("js_mute_guid") == string::npos && reply.find("js_unmute_guid") == string::npos)
            {

                print_colored_text(app_handles.hwnd_re_messages_data, reply.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes, true, true);
            }

            /*if (str_contains(reply, "js_pid_to_guid", 0U, true))
            {
                unordered_map<int, pair<std::string, bool>> pid_to_ip_address;
                const size_t end_pos{reply.rfind(";")};
                if (end_pos == string::npos)
                {
                    main_app.get_pid_to_ip_address().clear();
                    return;
                }

                reply.erase(begin(reply) + end_pos, end(reply));

                const size_t first_digit_pos{reply.find_first_of("-0123456789")};
                if (first_digit_pos != string::npos)
                {
                    reply.erase(0, first_digit_pos);
                }
                auto pid_to_guid_segments =
                    str_split(reply, ";", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
                for (auto &line : pid_to_guid_segments)
                {
                    trim_in_place(line);
                    auto pid_and_guid =
                        str_split(line, "=", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
                    if (pid_and_guid.size() >= 3u)
                    {
                        if (int pid{-1}; is_valid_decimal_whole_number(pid_and_guid[0], pid) && pid >= 0 && pid < 64)
                        {

                            pid_to_ip_address[pid] = {convert_guid_to_ip_address(stol(pid_and_guid[1])),
                                                      pid_and_guid[2][0] == 'y' ? true : false};
                        }
                    }
                }
                main_app.get_pid_to_ip_address() = std::move(pid_to_ip_address);
            }
            else*/ if (str_contains(reply, "g_muted_guids", 0U, true))
            {
                size_t end_pos{reply.find("is:")};
                if (string::npos == end_pos)
                {
                    return;
                }

                size_t start_pos{reply.find('"', end_pos + 3)};
                if (string::npos == start_pos)
                {
                    return;
                }

                ++start_pos;
                end_pos = reply.find("^7", start_pos);
                if (string::npos == end_pos)
                {
                    return;
                }

                reply = reply.substr(start_pos, end_pos - start_pos);
                if (!reply.empty() && reply.back() == ',')
                    reply.pop_back();

                auto guid_segments =
                    str_split(reply, ",", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
                if (!guid_segments.empty())
                {
                    // std::unordered_set<std::string> muted_ip_addresses;
                    bool is_save_muted_players_data_to_file{};
                    for (auto &guid_key : guid_segments)
                    {
                        stl::helper::trim_in_place(guid_key);
                        string ip_address{convert_guid_to_ip_address(stol(guid_key))};

                        if (!main_app.get_muted_players_map().contains(ip_address))
                        {
                            player pd{};
                            strcpy_s(pd.guid_key, std::size(pd.guid_key), guid_key.c_str());
                            strcpy_s(pd.player_name, std::size(pd.player_name), "John Doe");
                            pd.player_name_index = "john doe";
                            pd.banned_start_time = get_current_time_stamp();
                            const string date_time_info{
                                get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time)};
                            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), date_time_info.c_str());
                            pd.reason = "spam";
                            pd.is_muted = true;
                            pd.ip_address = std::move(ip_address);
                            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                                             pd.ip_address, pd);
                            // main_app.get_muted_players_vector().emplace_back(pd);
                            main_app.get_muted_players_map()[pd.ip_address] = pd;
                            is_save_muted_players_data_to_file = true;
                        }
                        // muted_ip_addresses.emplace(convert_guid_to_ip_address(stol(guid_key)));
                    }
                    if (is_save_muted_players_data_to_file)
                    {
                        save_muted_players_data_to_file(main_app.get_muted_players_file_path(),
                                                        main_app.get_muted_players_map());
                    }
                    // main_app.get_current_game_server().get_muted_ip_addresses() = std::move(muted_ip_addresses);
                }
            }
        }
    });

    main_app.add_command_handler({"!m", "!map"}, [](const vector<string> &user_cmd) {
        if (user_cmd.size() >= 2 && !user_cmd[1].empty())
        {
            if (!validate_admin_and_show_missing_admin_privileges_message(false))
                return;
            const string map_name{stl::helper::trim(user_cmd[1])};
            const string game_type{user_cmd.size() >= 3 ? stl::helper::trim(user_cmd[2]) : "ctf"};
            load_map(map_name, game_type, true);
        }
    });

    main_app.add_command_handler({"maps", "!maps"}, [](const vector<string> &) { display_all_available_maps(); });
    main_app.add_command_handler({"colors", "!colors"}, [](const vector<string> &) { change_colors(); });

    main_app.add_command_handler({"config", "!config"},
                                 [](const vector<string> &user_cmd) { change_server_setting(user_cmd); });

    /*main_app.add_command_handler({ "!rt", "!refreshtime" }, [](const
    vector<string> &user_cmd) { if (int number{}; (user_cmd.size() == 2) &&
    is_valid_decimal_whole_number(user_cmd[1], number)) {
            main_app.get_current_game_server().set_check_for_banned_players_time_period(number);
            KillTimer(app_handles.hwnd_main_window, ID_TIMER);
            SendMessage(app_handles.hwnd_progress_bar, PBM_SETRANGE, 0,
    MAKELPARAM(0,
    main_app.get_current_game_server().get_check_for_banned_players_time_period()));
            SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, 0, 0);
            SendMessage(app_handles.hwnd_progress_bar, PBM_SETSTEP, 1, 0);
            SetTimer(app_handles.hwnd_main_window, ID_TIMER, 1000, nullptr);
            const string re_msg{ format("^2You have successfully changed the
    ^1time period\n for automatic checking for banned IP addresses ^2to ^5{}\n",
    user_cmd[1]) }; print_colored_text(app_handles.hwnd_re_messages_data,
    re_msg.c_str(), is_append_message_to_richedit_control::yes,
    is_log_message::yes, is_log_datetime::yes);
            write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
      }
    });*/

    // main_app.add_command_handler({ "border", "!border" }, [](const
    // vector<string> &user_cmd) {
    //   if ((user_cmd.size() == 2) && (user_cmd[1] == "on" || user_cmd[1] ==
    //   "off")) {
    //     const bool new_setting{ user_cmd[1] == "on" ? true : false };
    //     const string re_msg{ format("^2You have successfully executed command:
    //     ^1{} {}\n", user_cmd[0], user_cmd[1]) };
    //     print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
    //     is_append_message_to_richedit_control::yes, is_log_message::yes,
    //     is_log_datetime::yes); if (main_app.get_is_draw_border_lines() !=
    //     new_setting) {
    //       main_app.set_is_draw_border_lines(new_setting);
    //       if (new_setting) {
    //         print_colored_text(app_handles.hwnd_re_messages_data, "^2You have
    //         successfully turned on the border lines\n around displayed ^3GUI
    //         controls^2.\n", is_append_message_to_richedit_control::yes,
    //         is_log_message::yes, is_log_datetime::yes);
    //       } else {
    //         print_colored_text(app_handles.hwnd_re_messages_data, "^2You have
    //         successfully turned off the border lines\n around displayed ^3GUI
    //         controls^2.\n", is_append_message_to_richedit_control::yes,
    //         is_log_message::yes, is_log_datetime::yes);
    //       }
    //       construct_tinyrcon_gui(app_handles.hwnd_main_window);
    //       write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
    //     }
    //   }
    // });

    main_app.add_command_handler({"messages", "!messages"}, [](const vector<string> &user_cmd) {
        if ((user_cmd.size() == 2) && (user_cmd[1] == "on" || user_cmd[1] == "off"))
        {
            const bool new_setting{user_cmd[1] == "on" ? false : true};
            const string re_msg{
                format("^2You have successfully executed command: ^1{} {}\n", user_cmd[0], user_cmd[1])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (main_app.get_is_disable_automatic_kick_messages() != new_setting)
            {
                main_app.set_is_disable_automatic_kick_messages(new_setting);
                if (!new_setting)
                {
                    print_colored_text(app_handles.hwnd_re_messages_data,
                                       "^2You have successfully turned on ^1automatic kick messages\n "
                                       "^2for ^1temporarily ^2and ^1permanently banned players^2.\n",
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                }
                else
                {
                    print_colored_text(app_handles.hwnd_re_messages_data,
                                       "^2You have successfully turned off ^1automatic kick messages\n "
                                       "^2for ^1temporarily ^2and ^1permanently banned players^2.\n",
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                }
                write_tiny_rcon_json_settings_to_file(main_app.get_tinyrcon_config_file_path());
            }
        }
    });

    main_app.add_command_handler({"say", "!say"}, [](const vector<string> &rcon_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (rcon_cmd.size() >= 2)
        {
            string command{stl::helper::str_join(rcon_cmd, " ")};
            if (!command.empty() && '!' == command[0])
                command.erase(0, 1);
            string reply;
            main_app.get_connection_manager().send_and_receive_rcon_data(
                command.c_str(), reply, main_app.get_current_game_server().get_server_ip_address().c_str(),
                main_app.get_current_game_server().get_server_port(),
                main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(),
                false, false);
        }
    });

    main_app.add_command_handler({"tell", "!tell"}, [](const vector<string> &rcon_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;

        if (rcon_cmd.size() >= 3 && check_if_user_provided_pid_is_valid(trim(rcon_cmd[1])))
        {
            const string player_pid{trim(rcon_cmd[1])};
            string command{"tell "s + player_pid};
            for (size_t j{2}; j < rcon_cmd.size(); ++j)
            {
                command.append(" ").append(rcon_cmd[j]);
            }

            string reply;
            main_app.get_connection_manager().send_and_receive_rcon_data(
                command.c_str(), reply, main_app.get_current_game_server().get_server_ip_address().c_str(),
                main_app.get_current_game_server().get_server_port(),
                main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(),
                false, false);
        }
    });

    main_app.add_command_handler({"t", "!t"}, [](const vector<string> &rcon_cmd) {
        if (rcon_cmd.size() >= 2)
        {
            const string message{format("{}\\{}", main_app.get_username(),
                                        stl::helper::str_join(cbegin(rcon_cmd) + 1, cend(rcon_cmd), " "))};
            main_app.add_message_to_queue(message_t{"public-message", message, true});
        }
    });

    main_app.add_command_handler({"y", "!y"}, [](const vector<string> &rcon_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (rcon_cmd.size() >= 3)
        {
            string send_to{rcon_cmd[1]};
            remove_disallowed_characters_in_string(send_to);
            const string private_message{format("{}\\{}\\{}", main_app.get_username(), send_to,
                                                stl::helper::str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " "))};
            main_app.get_connection_manager_for_messages().process_and_send_message(
                "private-message", private_message, true, main_app.get_tiny_rcon_server_ip_address(),
                main_app.get_tiny_rcon_server_port(), false);
        }
    });

    main_app.add_command_handler({"clientkick"}, [](const vector<string> &rcon_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 rcon_cmd[0], rcon_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};

            if (check_if_user_provided_argument_is_valid_for_specified_command(rcon_cmd[0].c_str(), rcon_cmd[1]))
            {
                const int pid{stoi(rcon_cmd[1])};
                string protected_player_msg;
                if (check_if_player_is_protected(get_player_data_for_pid(pid), rcon_cmd[0].c_str(),
                                                 protected_player_msg))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                    replace_br_with_new_line(protected_player_msg);
                    const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                    return;
                }
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
                string reason{remove_disallowed_characters_in_string(
                    rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified")};
                stl::helper::trim_in_place(reason);
                specify_reason_for_player_pid(pid, reason);
                main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                const string message{format("^3You have successfully executed "
                                            "^5clientkick ^3on player ({}^3)\n",
                                            get_player_information(pid, false, "Kicked"))};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                string command{main_app.get_user_defined_kick_message()};
                build_tiny_rcon_message(command);
                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                current_user->no_of_kicks++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                player &player{get_player_data_for_pid(pid)};
                player.banned_start_time = get_current_time_stamp();
                const string date_time_info{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", player.banned_start_time)};
                strcpy_s(player.banned_date_time, std::size(player.banned_date_time), date_time_info.c_str());
                player.reason = reason;
                player.banned_by_user_name = main_app.get_username();
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "add-kick",
                    format(R"({}\{}\{}\{}\{}\{})", player.ip_address, player.guid_key, player.player_name,
                           player.banned_date_time, reason, player.banned_by_user_name),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
                kick_player(pid, command);
            }
            else
            {
                const string re_msg2{format("^2{} ^3is not a valid pid number for "
                                            "the ^2!k ^3(^2!kick^3) command!\n",
                                            rcon_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg2.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^2{}\n", rcon_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (rcon_commands_help.contains(rcon_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, rcon_commands_help.at(rcon_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"tempbanclient"}, [](const vector<string> &rcon_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 rcon_cmd[0], rcon_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            if (check_if_user_provided_argument_is_valid_for_specified_command(rcon_cmd[0].c_str(), rcon_cmd[1]))
            {
                const int pid{stoi(rcon_cmd[1])};
                string protected_player_msg;
                if (check_if_player_is_protected(get_player_data_for_pid(pid), rcon_cmd[0].c_str(),
                                                 protected_player_msg))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                    replace_br_with_new_line(protected_player_msg);
                    const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                    return;
                }
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{BANNED_BY}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
                int number{};
                size_t temp_ban_duration{24};
                string reason{"not specified"};
                if (rcon_cmd.size() > 2)
                {
                    if (is_valid_decimal_whole_number(rcon_cmd[2], number))
                    {
                        temp_ban_duration = number > 0 && number <= 9999 ? number : 24;
                        if (rcon_cmd.size() > 3)
                        {
                            reason = remove_disallowed_characters_in_string(
                                str_join(cbegin(rcon_cmd) + 3, cend(rcon_cmd), " "));
                            stl::helper::trim_in_place(reason);
                        }
                    }
                    else
                    {
                        reason =
                            remove_disallowed_characters_in_string(str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " "));
                        stl::helper::trim_in_place(reason);
                    }
                }
                main_app.get_tinyrcon_dict()["{REASON}"] = reason;
                main_app.get_tinyrcon_dict()["{TEMPBAN_DURATION}"] = to_string(temp_ban_duration);
                const string message{format("^3You have successfully executed "
                                            "^5tempbanclient ^3on player ({}^3)\n",
                                            get_player_information(pid, false, "Temporarily banned"))};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                string command{main_app.get_user_defined_tempban_message()};
                build_tiny_rcon_message(command);
                player &pd = get_player_data_for_pid(pid);
                pd.ban_duration_in_hours = temp_ban_duration;
                pd.reason = reason;
                // pd.banned_by_user_name = main_app.get_username();
                tempban_player(pd, command);
                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                current_user->no_of_tempbans++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "add-tempban",
                    format(R"({}\{}\{}\{}\{}\{}\{})", pd.ip_address, pd.player_name, pd.banned_date_time,
                           pd.banned_start_time, pd.ban_duration_in_hours, pd.reason, pd.banned_by_user_name),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
            else
            {
                const string re_msg{format("^2{} ^3is not a valid pid number for "
                                           "the ^2!tb ^3(^2!tempban^3) command!\n",
                                           rcon_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^2{}\n", rcon_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (rcon_commands_help.contains(rcon_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, rcon_commands_help.at(rcon_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"banclient"}, [](const vector<string> &rcon_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty())
        {
            string ex_msg{format("^1Exception ^3thrown from ^1command handler "
                                 "^3for ^1'{} {}' ^3user command.",
                                 rcon_cmd[0], rcon_cmd[1])};
            stack_trace_element ste{app_handles.hwnd_re_messages_data, std::move(ex_msg)};
            if (check_if_user_provided_argument_is_valid_for_specified_command(rcon_cmd[0].c_str(), rcon_cmd[1]))
            {
                const int pid{stoi(rcon_cmd[1])};
                string protected_player_msg;
                if (check_if_player_is_protected(get_player_data_for_pid(pid), rcon_cmd[0].c_str(),
                                                 protected_player_msg))
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                    replace_br_with_new_line(protected_player_msg);
                    const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                    main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                    return;
                }
                main_app.get_tinyrcon_dict()["{ADMINNAME}"] = main_app.get_username();
                main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(pid);
                auto &pd = get_player_data_for_pid(pid);
                pd.banned_start_time = get_current_time_stamp();
                const string banned_date_time_str{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time)};
                strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), banned_date_time_str.c_str());
                main_app.get_tinyrcon_dict()["{BAN_DATE}"] = pd.banned_date_time;
                string reason{remove_disallowed_characters_in_string(
                    rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified")};
                trim_in_place(reason);
                specify_reason_for_player_pid(pid, reason);
                main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                auto &current_user = main_app.get_user_for_name(main_app.get_username());
                current_user->no_of_guidbans++;
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                const string message{format("^3You have successfully executed ^5banclient ^3on player ({}^3)\n",
                                            get_player_information(pid, false, "GUID banned"))};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                string command{main_app.get_user_defined_ban_message()};
                build_tiny_rcon_message(command);
                ban_player(pid, command);
            }
            else
            {
                const string re_msg{format("^2{} ^3is not a valid pid number for "
                                           "the ^2!b ^3(^2!ban^3) command!\n",
                                           rcon_cmd[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        else
        {
            const string re_msg{format("^3Invalid command syntax for user command: ^2{}\n", rcon_cmd[0])};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (rcon_commands_help.contains(rcon_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, rcon_commands_help.at(rcon_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"kick", "onlykick", "tempbanuser", "banuser"}, [](const vector<string> &rcon_cmd) {
        if (!validate_admin_and_show_missing_admin_privileges_message(false))
            return;
        if (rcon_cmd.size() > 1 && !rcon_cmd[1].empty())
        {
            bool is_player_found{};
            const bool remove_player_color_codes{rcon_cmd[0] == "onlykick"};
            const auto &players_data = main_app.get_current_game_server().get_players_data();
            for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
            {
                const player &player{players_data[i]};
                string player_name{player.player_name};
                if (remove_player_color_codes)
                    remove_all_color_codes(player_name);
                if (rcon_cmd[1] == player_name)
                {
                    string protected_player_msg;
                    if (check_if_player_is_protected(get_player_data_for_pid(player.pid), rcon_cmd[0].c_str(),
                                                     protected_player_msg))
                    {
                        print_colored_text(app_handles.hwnd_re_messages_data, protected_player_msg.c_str());
                        replace_br_with_new_line(protected_player_msg);
                        const string inform_msg{format("{}\\{}", main_app.get_username(), protected_player_msg)};
                        main_app.add_message_to_queue(message_t("inform-message", inform_msg, true));
                        return;
                    }
                    is_player_found = true;
                    main_app.get_tinyrcon_dict()["{PLAYERNAME}"] = get_player_name_for_pid(player.pid);
                    string reason{remove_disallowed_characters_in_string(
                        rcon_cmd.size() > 2 ? str_join(cbegin(rcon_cmd) + 2, cend(rcon_cmd), " ") : "not specified")};
                    stl::helper::trim_in_place(reason);
                    specify_reason_for_player_pid(player.pid, reason);
                    main_app.get_tinyrcon_dict()["{REASON}"] = std::move(reason);
                    const string message{format("^3You have successfully executed ^5{} ^3on player ({}^3)\n",
                                                rcon_cmd[0], get_player_information(player.pid, false, "Kicked"))};
                    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                    string public_message{(rcon_cmd[0] == "kick" || rcon_cmd[0] == "onlykick")
                                              ? main_app.get_user_defined_kick_message()
                                              : main_app.get_user_defined_tempban_message()};
                    build_tiny_rcon_message(public_message);
                    replace_br_with_new_line(public_message);
                    auto &current_user = main_app.get_user_for_name(main_app.get_username());
                    if (rcon_cmd[0] == "kick" || rcon_cmd[0] == "onlykick")
                    {
                        current_user->no_of_kicks++;
                    }
                    else if (rcon_cmd[0] == "tempbanuser")
                    {
                        current_user->no_of_tempbans++;
                    }
                    else
                    {
                        current_user->no_of_guidbans++;
                    }
                    save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                    /*const string inform_msg{ format("{}\\{}", main_app.get_username(),
                    public_message) };
                    main_app.add_message_to_queue(message_t("inform-message",
                    inform_msg, true));*/
                    const string command{rcon_cmd[0] + " "s + rcon_cmd[1]};
                    string reply;
                    main_app.get_connection_manager().send_and_receive_rcon_data(
                        command.c_str(), reply, main_app.get_current_game_server().get_server_ip_address().c_str(),
                        main_app.get_current_game_server().get_server_port(),
                        main_app.get_current_game_server().get_rcon_password().c_str(),
                        main_app.get_current_game_server(), true);
                    kick_player(player.pid, public_message);
                    break;
                }
            }

            if (!is_player_found)
            {
                if (string player_name{rcon_cmd[1]}; remove_player_color_codes)
                {
                    remove_all_color_codes(player_name);
                    ostringstream oss;
                    oss << "^3Could not find player with specified name (^5" << player_name << "^3) to use with the ^2"
                        << rcon_cmd[0] << " ^3command!\n";
                    print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                }
                else
                {
                    ostringstream oss;
                    oss << "^3Could not find player with specified name (^5" << rcon_cmd[1] << "^3) to use with the ^2"
                        << rcon_cmd[0] << " ^3command!\n";
                    print_colored_text(app_handles.hwnd_re_messages_data, oss.str().c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                }
            }
        }
        else
        {
            const string re_msg{"^3Invalid command syntax for user command: ^2"s + rcon_cmd[0] + "\n"s};
            print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                               is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
            if (user_commands_help.contains(rcon_cmd[0]))
            {
                print_colored_text(app_handles.hwnd_re_messages_data, user_commands_help.at(rcon_cmd[0]).c_str(),
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data, "\n", is_append_message_to_richedit_control::yes,
                                   is_log_message::yes, is_log_datetime::no);
            }
        }
    });

    main_app.add_command_handler({"unknown-rcon"}, [](const vector<string> &rcon_cmd) {
        const string command{rcon_cmd.size() > 1 ? trim(str_join(rcon_cmd, " ")) : rcon_cmd[0]};
        const string re_msg{"^5Sending rcon command '"s + command + "' to the server.\n"s};
        print_colored_text(app_handles.hwnd_re_messages_data, re_msg.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        string reply;
        main_app.get_connection_manager().send_and_receive_rcon_data(
            command.c_str(), reply, main_app.get_current_game_server().get_server_ip_address().c_str(),
            main_app.get_current_game_server().get_server_port(),
            main_app.get_current_game_server().get_rcon_password().c_str(), main_app.get_current_game_server(), true);
    });

    main_app.add_message_handler("request-login", [](const string &, const time_t, const string &, bool) {
        const string message{
            format("^7{} ^7has sent a ^1login request ^7to ^5Tiny^6Rcon ^7server.", main_app.get_username())};
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    });

    main_app.add_message_handler("confirm-login", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
            stl::helper::trim_in_place(part);
        if (parts.size() >= 3)
        {
            unsigned long guid_key{};
            if (!check_ip_address_validity(main_app.get_user_ip_address(), guid_key))
            {
                main_app.set_user_ip_address(std::move(parts[1]));
            }
            auto &current_user = main_app.get_user_for_name(main_app.get_username());
            current_user->no_of_logins++;
            current_user->last_login_time_stamp = get_current_time_stamp();
            current_user->is_logged_in = true;
            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
            string message{format("^7{} ^2has ^1logged in ^2to ^5Tiny^6Rcon "
                                  "^2server.\n^2Number of logins: ^1{}\n",
                                  current_user->user_name, current_user->no_of_logins)};
            if (parts.size() >= 4)
            {
                message += format("^2Player name: ^7{}\n", parts[3]);
            }
            if (parts.size() >= 5)
            {
                message += format("^2Game's version number: ^1{}\n", parts[4]);
            }
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            /*main_app.add_message_to_queue(message_t("request-mapnames",
            format(R"({}\{}\{})", current_user->user_name, current_user->ip_address,
            get_current_time_stamp()), true));
            main_app.add_remote_message_to_queue(message_t("request-imagesdata",
            format(R"({}\{}\{})", current_user->user_name, current_user->ip_address,
            get_current_time_stamp()), true));*/
        }
    });

    main_app.add_message_handler("welcome-message", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        // for (auto &part : parts) stl::helper::trim_in_place(part);
        if (parts.size() >= 3)
        {
            unsigned long guid_key{};
            if (!check_ip_address_validity(main_app.get_user_ip_address(), guid_key))
            {
                main_app.set_user_ip_address(std::move(parts[1]));
            }
            print_colored_text(app_handles.hwnd_re_messages_data, parts[2].c_str());
        }
    });

    main_app.add_message_handler("receive-welcome-message", [](const string &, const time_t, const string &data, bool) {
        print_colored_text(app_handles.hwnd_re_messages_data, data.c_str());
    });

    main_app.add_remote_message_handler("receive-welcome-message-player",
                                        [](const string &, const time_t, const string &data, bool) {
                                            print_colored_text(app_handles.hwnd_re_messages_data, data.c_str());
                                        });

    main_app.add_remote_message_handler("receive-reports-player", [](const string &, const time_t, const string &data,
                                                                     bool) {
        auto lines =
            stl::helper::str_split(data, "\n", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
        {
            lock_guard lk{reports_mutex};
            main_app.get_reported_players().clear();
        }
        for (auto &line : lines)
        {
            stl::helper::trim_in_place(line);
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
            if (parts.size() >= 5)
            {
                for (auto &part : parts)
                    stl::helper::trim_in_place(part);

                player pd{};
                size_t no_of_chars_to_copy{std::min<size_t>(std::size(pd.player_name) - 1, parts[0].length())};
                strncpy_s(pd.player_name, std::size(pd.player_name), parts[0].c_str(), no_of_chars_to_copy);
                pd.ip_address = parts[1];
                pd.banned_start_time = std::stoll(parts[2]);
                const string converted_ban_date_and_time_info{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", pd.banned_start_time)};
                no_of_chars_to_copy =
                    std::min<size_t>(std::size(pd.banned_date_time) - 1, converted_ban_date_and_time_info.length());
                strncpy_s(pd.banned_date_time, std::size(pd.banned_date_time), converted_ban_date_and_time_info.c_str(),
                          no_of_chars_to_copy);
                pd.reason = std::move(parts[3]);
                pd.banned_by_user_name = parts.size() >= 5 ? parts[4] : "^3Player";
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);
                {
                    lock_guard lk{reports_mutex};
                    main_app.get_reported_players().push_back(std::move(pd));
                }
            }
        }

        display_reported_players(main_app.get_reported_players().size());
    });

    main_app.add_message_handler("heartbeat", [](const string &, const time_t, const string &, bool) {
        const auto current_ts{get_current_time_stamp()};
        main_app.get_connection_manager_for_messages().process_and_send_message(
            "heartbeat", format("{}\\{}", main_app.get_username(), current_ts), true,
            main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
    });

    main_app.add_message_handler("public-message", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        // for (auto &part : parts) stl::helper::trim_in_place(part);
        if (parts.size() >= 2)
        {
            const string public_msg{format("^7{} ^7sent all admins a ^1public "
                                           "message:\n^5Public message: ^7{}\n",
                                           parts[0], parts[1])};
            print_colored_text(app_handles.hwnd_re_messages_data, public_msg.c_str());
        }
    });

    main_app.add_message_handler("private-message", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        // for (auto &part : parts) stl::helper::trim_in_place(part);
        if (parts.size() >= 3)
        {
            const string private_msg{
                format("^7{} ^7sent you a ^1private message.\n^5Private message: ^7{}\n", parts[0], parts[2])};
            print_colored_text(app_handles.hwnd_re_messages_data, private_msg.c_str());
        }
    });

    main_app.add_remote_message_handler("inform-login-player",
                                        [](const string &, const time_t, const string &message, bool) {
                                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
                                        });

    main_app.add_remote_message_handler("inform-logout-player",
                                        [](const string &, const time_t, const string &message, bool) {
                                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
                                        });

    main_app.add_message_handler("inform-login", [](const string &, const time_t, const string &message, bool) {
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    });

    main_app.add_message_handler("inform-logout", [](const string &, const time_t, const string &message, bool) {
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    });

    main_app.add_message_handler("request-logout", [](const string &, const time_t, const string &, bool) {
        const string message{
            format("^7{} ^7has sent a ^1logout request ^7to ^5Tiny^6Rcon ^7server.", main_app.get_username())};
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    });

    main_app.add_message_handler("confirm-logout", [](const string &, const time_t, const string &, bool) {
        auto &current_user = main_app.get_user_for_name(main_app.get_username());
        current_user->is_logged_in = false;
        current_user->last_logout_time_stamp = get_current_time_stamp();
        const string message{format("^7{} ^3has ^1logged out ^3of ^5Tiny^6Rcon "
                                    "^3server.\n^3Number of logins: ^1{}",
                                    current_user->user_name, current_user->no_of_logins)};
        print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
    });

    main_app.add_remote_message_handler("request-login-player",
                                        [](const string &, const time_t, const string &message, bool) {
                                            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
                                        });

    main_app.add_remote_message_handler("confirm-login-player", [](const string &, const time_t, const string &data,
                                                                   bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
            stl::helper::trim_in_place(part);
        if (parts.size() >= 3)
        {
            unsigned long guid_key{};
            if (!check_ip_address_validity(main_app.get_user_ip_address(), guid_key))
            {
                main_app.set_user_ip_address(std::move(parts[1]));
            }
            const auto ts{get_current_time_stamp()};
            auto &current_user = main_app.get_player_for_name(main_app.get_username(), main_app.get_user_ip_address());
            current_user->no_of_logins++;
            current_user->last_login_time_stamp = ts;
            current_user->is_admin = false;
            current_user->is_logged_in = true;
            save_current_user_data_to_json_file(main_app.get_user_data_file_path());
            string message{format("^2Player {} ^2has ^1logged in ^2to ^5Tiny^6Rcon ^2server.\n^2Number "
                                  "of logins: ^1{}",
                                  current_user->user_name, current_user->no_of_logins)};
            if (parts.size() >= 4)
            {
                message += format("^2Player name: ^7{}\n", parts[3]);
            }
            if (parts.size() >= 5)
            {
                message += format("^2Game's version number: ^1{}\n", parts[4]);
            }
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());

            /*const string user_details{ format("{}\\{}\\{}",
            current_user->user_name, current_user->ip_address, ts) };
            main_app.add_remote_message_to_queue(message_t{
            "request-admindata-player", current_user, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-tempbans-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-ipaddressbans-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-ipaddressrangebans-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-namebans-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-citybans-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-countrybans-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-protectedipaddresses-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-protectedipaddressranges-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-protectedcities-player", user_details, true });
            main_app.add_remote_message_to_queue(message_t{
            "request-protectedcountries-player", user_details, true });
            main_app.add_message_to_queue(message_t("request-mapnames",
            user_details, true));
            main_app.add_remote_message_to_queue(message_t("request-imagesdata",
            user_details, true));*/
        }
    });

    // receive-imagesdata
    main_app.add_remote_message_handler("receive-imagesdata", [](const string &, const time_t, const string &data,
                                                                 bool) {
        std::thread task{[d = data]() {
            auto parts =
                stl::helper::str_split(d, "\n", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
                auto image_file_info_parts = stl::helper::str_split(part, ",", nullptr, split_on_whole_needle_t::yes,
                                                                    ignore_empty_string_t::yes);
                if (image_file_info_parts.size() >= 2)
                {
                    stl::helper::trim_in_place(image_file_info_parts[0]);
                    stl::helper::trim_in_place(image_file_info_parts[1]);
                    const string image_file_absolute_path{
                        format("{}{}", main_app.get_current_working_directory(), image_file_info_parts[0])};
                    const bool is_download_file = [&]() {
                        if (!check_if_file_path_exists(image_file_absolute_path.c_str()))
                            return true;
                        const auto image_file_md5 = calculate_md5_checksum_of_file(image_file_absolute_path.c_str());
                        return image_file_md5 != image_file_info_parts[1];
                    }();

                    if (is_download_file)
                    {
                        string ftp_download_link{format("ftp://{}/{}/{}", main_app.get_ftp_download_site_ip_address(),
                                                        main_app.get_ftp_download_folder_path(),
                                                        image_file_info_parts[0])};
                        replace_backward_slash_with_forward_slash(ftp_download_link);
                        const string image_file_name{
                            image_file_info_parts[0].substr(image_file_info_parts[0].rfind('/') + 1)};
                        const string information_before_download{
                            format("^3Starting to download missing map image file: ^5{}", image_file_name)};
                        print_colored_text(app_handles.hwnd_re_messages_data, information_before_download.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::no,
                                           is_log_datetime::yes, true, true);
                        if (main_app.get_auto_update_manager().download_file(ftp_download_link.c_str(),
                                                                             image_file_absolute_path.c_str()))
                        {
                            const string information_after_download{
                                format("^2Finished downloading missing map image file: ^5{}", image_file_name)};
                            print_colored_text(app_handles.hwnd_re_messages_data, information_after_download.c_str(),
                                               is_append_message_to_richedit_control::yes, is_log_message::no,
                                               is_log_datetime::yes, true, true);
                            this_thread::sleep_for(50ms);
                        }
                    }
                }
            }
        }};

        task.detach();
    });

    main_app.add_remote_message_handler("rcon-heartbeat-player", [](const string &, const time_t, const string &,
                                                                    bool) {
        const auto current_ts{get_current_time_stamp()};
        main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
            "rcon-heartbeat-player", format("{}\\{}\\{}", main_app.get_username(), current_ts, me->ip_address), true,
            main_app.get_private_tiny_rcon_server_ip_address(), main_app.get_private_tiny_rcon_server_port(), false);
    });

    main_app.add_remote_message_handler(
        "rcon-reply-player", [](const string &, const time_t, const string &data, bool) {
            if (!me->is_admin)
            {
                main_app.get_connection_manager().process_rcon_status_messages(main_app.get_game_servers()[0], data);
            }
        });

    main_app.add_remote_message_handler("rcon-online-admins-info-player",
                                        [](const string &, const time_t, const string &data, bool) {
                                            online_admins_information = data;
                                            /*Edit_SetText(app_handles.hwnd_online_admins_information, "");
                                            print_colored_text(app_handles.hwnd_online_admins_information, data.c_str(),
                                            is_append_message_to_richedit_control::yes, is_log_message::no,
                                            is_log_datetime::no);
                                            SendMessage(app_handles.hwnd_online_admins_information, EM_SETSEL, 0, -1);
                                            SendMessage(app_handles.hwnd_online_admins_information, EM_SETFONTSIZE,
                                            (WPARAM)2, (LPARAM)NULL);*/
                                        });

    main_app.add_remote_message_handler(
        "rcon-inform-message", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
            if (parts.size() >= 2)
            {
                for (size_t i{}; i < parts.size(); ++i)
                {
                    if (!parts[i].empty())
                    {
                        print_colored_text(app_handles.hwnd_re_messages_data, parts[i].c_str());
                    }
                }
            }
        });

    // request-tempbans
    main_app.add_message_handler("request-tempbans", [](const string &, const time_t, const string &, bool) {
        // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
        // last ^1100 temporary bans ^5from Tiny^6Rcon ^5server...");
    });

    // request-ipaddressbans
    main_app.add_message_handler("request-ipaddressbans", [](const string &, const time_t, const string &, bool) {
        // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
        // last ^1100 IP address bans ^5from Tiny^6Rcon ^5server...");
    });

    // request-ipaddressrangebans
    main_app.add_message_handler("request-ipaddressrangebans", [](const string &, const time_t, const string &, bool) {
        // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
        // last ^1100 IP address range bans ^5from Tiny^6Rcon ^5server...");
    });

    // request-namebans
    main_app.add_message_handler("request-namebans", [](const string &, const time_t, const string &, bool) {
        // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
        // last ^1100 player name bans ^5from Tiny^6Rcon ^5server...");
    });

    // request-citybans
    main_app.add_message_handler("request-citybans", [](const string &, const time_t, const string &, bool) {
        // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
        // last ^1100 city bans ^5from Tiny^6Rcon ^5server...");
    });

    // request-countrybans
    main_app.add_message_handler("request-countrybans", [](const string &, const time_t, const string &, bool) {
        // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
        // last ^1100 country bans ^5from Tiny^6Rcon ^5server...");
    });

    // receive-tempbans
    main_app.add_message_handler("receive-tempbans", [](const string &, const time_t, const string &data, bool) {
        ofstream received_tempbans_file{L"last_tempbans.txt"};
        if (received_tempbans_file)
        {
            received_tempbans_file << data << endl;
        }
        received_tempbans_file.close();
        vector<player> last_tempbans_vector;
        unordered_map<string, player> last_tempbans_map;
        parse_tempbans_data_file("last_tempbans.txt", last_tempbans_vector, last_tempbans_map);
        DeleteFileW(L"last_tempbans.txt");
        // print_colored_text(app_handles.hwnd_re_messages_data,
        // format("^5Received last ^1{} temporary bans ^5from Tiny^6Rcon
        // ^5server.", last_tempbans_map.size()).c_str());
        for (auto &&last_tempban : last_tempbans_vector)
        {
            unsigned long guid_number{};
            if (!check_ip_address_validity(last_tempban.ip_address, guid_number) ||
                main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(last_tempban.ip_address))
                continue;

            main_app.get_current_game_server().get_temp_banned_ip_addresses_map().emplace(last_tempban.ip_address,
                                                                                          last_tempban);
            main_app.get_current_game_server().get_temp_banned_ip_addresses_vector().push_back(std::move(last_tempban));
        }

        sort(begin(main_app.get_current_game_server().get_temp_banned_ip_addresses_vector()),
             end(main_app.get_current_game_server().get_temp_banned_ip_addresses_vector()),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });

        save_tempbans_to_file(main_app.get_temp_bans_file_path(),
                              main_app.get_current_game_server().get_temp_banned_ip_addresses_vector());
        operation_completed_flag[0] = 1;
        if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
        {
            /*const string message{ "^2Received updated ^1ban entries ^2from
            ^5Tiny^6Rcon ^5server." };
            print_colored_text(app_handles.hwnd_re_messages_data,
            message.c_str());*/
            main_app.set_is_bans_synchronized(true);
        }
    });

    // receive-ipaddressbans
    main_app.add_message_handler("receive-ipaddressbans", [](const string &, const time_t, const string &data, bool) {
        ofstream received_tempbans_file{L"last_ipaddressbans.txt"};
        if (received_tempbans_file)
        {
            received_tempbans_file << data << endl;
        }
        received_tempbans_file.close();
        vector<player> last_ipaddressbans_vector;
        unordered_map<string, player> last_ipaddressbans_map;
        parse_banned_ip_addresses_file("last_ipaddressbans.txt", last_ipaddressbans_vector, last_ipaddressbans_map);
        DeleteFileW(L"last_ipaddressbans.txt");
        // print_colored_text(app_handles.hwnd_re_messages_data,
        // format("^5Received last ^1{} IP address bans ^5from Tiny^6Rcon
        // ^5server.", last_ipaddressbans_map.size()).c_str());
        for (auto &&last_ipaddressban : last_ipaddressbans_vector)
        {
            unsigned long guid_number{};
            if (!check_ip_address_validity(last_ipaddressban.ip_address, guid_number) ||
                main_app.get_current_game_server().get_banned_ip_addresses_map().contains(last_ipaddressban.ip_address))
                continue;

            main_app.get_current_game_server().get_banned_ip_addresses_map().emplace(last_ipaddressban.ip_address,
                                                                                     last_ipaddressban);
            main_app.get_current_game_server().get_banned_ip_addresses_vector().push_back(std::move(last_ipaddressban));
        }

        sort(begin(main_app.get_current_game_server().get_banned_ip_addresses_vector()),
             end(main_app.get_current_game_server().get_banned_ip_addresses_vector()),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });

        save_banned_ip_entries_to_file(main_app.get_ip_bans_file_path(),
                                       main_app.get_current_game_server().get_banned_ip_addresses_vector());
        operation_completed_flag[1] = 1;
        if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
        {
            /*const string message{ "^2Received updated ^1ban entries ^2from
            ^5Tiny^6Rcon ^5server." };
            print_colored_text(app_handles.hwnd_re_messages_data,
            message.c_str());*/
            main_app.set_is_bans_synchronized(true);
        }
    });

    // receive-ipaddressrangebans
    main_app.add_message_handler(
        "receive-ipaddressrangebans", [](const string &, const time_t, const string &data, bool) {
            ofstream received_tempbans_file{L"last_ipaddressrangebans.txt"};
            if (received_tempbans_file)
            {
                received_tempbans_file << data << endl;
            }
            received_tempbans_file.close();
            vector<player> last_ipaddressrangebans_vector;
            unordered_map<string, player> last_ipaddressrangebans_map;
            parse_banned_ip_address_ranges_file("last_ipaddressrangebans.txt", last_ipaddressrangebans_vector,
                                                last_ipaddressrangebans_map);
            DeleteFileW(L"last_ipaddressrangebans.txt");
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // format("^5Received last ^1{} IP address range bans ^5from Tiny^6Rcon
            // ^5server.", last_ipaddressrangebans_map.size()).c_str());
            for (auto &&last_ipaddressrangeban : last_ipaddressrangebans_vector)
            {

                if (!last_ipaddressrangeban.ip_address.ends_with(".*.*") &&
                    !last_ipaddressrangeban.ip_address.ends_with(".*"))
                    continue;

                if (main_app.get_current_game_server().get_banned_ip_address_ranges_map().contains(
                        last_ipaddressrangeban.ip_address))
                    continue;

                main_app.get_current_game_server().get_banned_ip_address_ranges_map().emplace(
                    last_ipaddressrangeban.ip_address, last_ipaddressrangeban);
                main_app.get_current_game_server().get_banned_ip_address_ranges_vector().push_back(
                    std::move(last_ipaddressrangeban));
            }

            sort(begin(main_app.get_current_game_server().get_banned_ip_address_ranges_vector()),
                 end(main_app.get_current_game_server().get_banned_ip_address_ranges_vector()),
                 [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });

            save_banned_ip_address_range_entries_to_file(
                main_app.get_ip_range_bans_file_path(),
                main_app.get_current_game_server().get_banned_ip_address_ranges_vector());
            operation_completed_flag[2] = 1;
            if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
            {
                /*const string message{ "^2Received updated ^1ban entries ^2from
                ^5Tiny^6Rcon ^5server." };
                print_colored_text(app_handles.hwnd_re_messages_data,
                message.c_str());*/
                main_app.set_is_bans_synchronized(true);
            }
        });

    // receive-namebans
    main_app.add_message_handler("receive-namebans", [](const string &, const time_t, const string &data, bool) {
        ofstream received_tempbans_file{L"last_namebans.txt"};
        if (received_tempbans_file)
        {
            received_tempbans_file << data << endl;
        }
        received_tempbans_file.close();
        vector<player> last_namebans_vector;
        unordered_map<string, player> last_namebans_map;
        parse_banned_names_file("last_namebans.txt", last_namebans_vector, last_namebans_map);
        DeleteFileW(L"last_namebans.txt");
        // print_colored_text(app_handles.hwnd_re_messages_data,
        // format("^5Received ^1{} banned player names ^5from Tiny^6Rcon
        // ^5server.", last_namebans_map.size()).c_str());
        for (auto &&last_nameban : last_namebans_vector)
        {

            if (len(last_nameban.player_name) == 0 ||
                main_app.get_current_game_server().get_banned_names_map().contains(last_nameban.player_name))
                continue;

            main_app.get_current_game_server().get_banned_names_map().emplace(last_nameban.player_name, last_nameban);
            main_app.get_current_game_server().get_banned_names_vector().push_back(std::move(last_nameban));

            add_permanently_banned_player_name(last_nameban,
                                               main_app.get_current_game_server().get_banned_names_vector(),
                                               main_app.get_current_game_server().get_banned_names_map());
        }

        sort(begin(main_app.get_current_game_server().get_banned_names_vector()),
             end(main_app.get_current_game_server().get_banned_names_vector()),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });

        save_banned_ip_entries_to_file(main_app.get_banned_names_file_path(),
                                       main_app.get_current_game_server().get_banned_names_vector());
        operation_completed_flag[5] = 1;
        if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
        {
            /*const string message{ "^2Received updated ^1ban entries ^2from
            ^5Tiny^6Rcon ^5server." };
            print_colored_text(app_handles.hwnd_re_messages_data,
            message.c_str());*/
            main_app.set_is_bans_synchronized(true);
        }
    });

    // receive-citybans
    main_app.add_message_handler("receive-citybans", [](const string &, const time_t, const string &data, bool) {
        ofstream received_tempbans_file{L"last_citybans.txt"};
        if (received_tempbans_file)
        {
            received_tempbans_file << data << endl;
        }
        received_tempbans_file.close();
        set<string> last_citybans_set;
        parse_banned_cities_file("last_citybans.txt", last_citybans_set);
        DeleteFileW(L"last_citybans.txt");
        // print_colored_text(app_handles.hwnd_re_messages_data,
        // format("^5Received ^1{} banned cities ^5from Tiny^6Rcon ^5server.",
        // last_citybans_set.size()).c_str());
        for (auto &&last_cityban : last_citybans_set)
        {
            if (main_app.get_current_game_server().get_banned_cities_set().contains(last_cityban))
                continue;
            main_app.get_current_game_server().get_banned_cities_set().emplace(last_cityban);
        }

        save_banned_cities_to_file(main_app.get_banned_cities_file_path(),
                                   main_app.get_current_game_server().get_banned_cities_set());
        operation_completed_flag[3] = 1;
        if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
        {
            /*const string message{ "^2Received updated ^1ban entries ^2from
            ^5Tiny^6Rcon ^5server." };
            print_colored_text(app_handles.hwnd_re_messages_data,
            message.c_str());*/
            main_app.set_is_bans_synchronized(true);
        }
    });

    // receive-countrybans
    main_app.add_message_handler("receive-countrybans", [](const string &, const time_t, const string &data, bool) {
        ofstream received_tempbans_file{L"last_countrybans.txt"};
        if (received_tempbans_file)
        {
            received_tempbans_file << data << endl;
        }
        received_tempbans_file.close();
        set<string> last_countrybans_set;
        parse_banned_countries_file("last_countrybans.txt", last_countrybans_set);
        DeleteFileW(L"last_countrybans.txt");
        // print_colored_text(app_handles.hwnd_re_messages_data,
        // format("^5Received ^1{} banned countries ^5from Tiny^6Rcon ^5server.",
        // last_countrybans_set.size()).c_str());

        for (auto &&last_countryban : last_countrybans_set)
        {
            if (main_app.get_current_game_server().get_banned_countries_set().contains(last_countryban))
                continue;
            main_app.get_current_game_server().get_banned_countries_set().emplace(last_countryban);
        }

        save_banned_countries_to_file(main_app.get_banned_countries_file_path(),
                                      main_app.get_current_game_server().get_banned_countries_set());
        operation_completed_flag[4] = 1;
        if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
        {
            /*const string message{ "^2Received updated ^1ban entries ^2from
            ^5Tiny^6Rcon ^5server." };
            print_colored_text(app_handles.hwnd_re_messages_data,
            message.c_str());*/
            main_app.set_is_bans_synchronized(true);
        }
    });

    // request and receive various types of ban entries for players
    // ************************************************************
    // request-tempbans-player
    main_app.add_remote_message_handler("request-tempbans-player",
                                        [](const string &, const time_t, const string &, bool) {
                                            // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
                                            // last ^1100 temporary bans ^5from Tiny^6Rcon ^5server...");
                                        });

    // request-ipaddressbans-player
    main_app.add_remote_message_handler("request-ipaddressbans-player",
                                        [](const string &, const time_t, const string &, bool) {
                                            // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
                                            // last ^1100 IP address bans ^5from Tiny^6Rcon ^5server...");
                                        });

    // request-ipaddressrangebans-player
    main_app.add_remote_message_handler("request-ipaddressrangebans-player",
                                        [](const string &, const time_t, const string &, bool) {
                                            // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
                                            // last ^1100 IP address range bans ^5from Tiny^6Rcon ^5server...");
                                        });

    // request-namebans-player
    main_app.add_remote_message_handler("request-namebans-player",
                                        [](const string &, const time_t, const string &, bool) {
                                            // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
                                            // last ^1100 player name bans ^5from Tiny^6Rcon ^5server...");
                                        });

    // request-citybans-player
    main_app.add_remote_message_handler("request-citybans-player",
                                        [](const string &, const time_t, const string &, bool) {
                                            // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
                                            // last ^1100 city bans ^5from Tiny^6Rcon ^5server...");
                                        });

    // request-countrybans-player
    main_app.add_remote_message_handler("request-countrybans-player",
                                        [](const string &, const time_t, const string &, bool) {
                                            // print_colored_text(app_handles.hwnd_re_messages_data, "^5Requesting
                                            // last ^1100 country bans ^5from Tiny^6Rcon ^5server...");
                                        });

    // receive-tempbans-player
    main_app.add_remote_message_handler("receive-tempbans-player", [](const string &, const time_t, const string &data,
                                                                      bool) {
        ofstream received_tempbans_file{L"last_tempbans.txt"};
        if (received_tempbans_file)
        {
            received_tempbans_file << data << endl;
        }
        received_tempbans_file.close();
        vector<player> last_tempbans_vector;
        unordered_map<string, player> last_tempbans_map;
        parse_tempbans_data_file("last_tempbans.txt", last_tempbans_vector, last_tempbans_map);
        DeleteFileW(L"last_tempbans.txt");
        // print_colored_text(app_handles.hwnd_re_messages_data,
        // format("^5Received last ^1{} temporary bans ^5from Tiny^6Rcon
        // ^5server.", last_tempbans_map.size()).c_str());
        for (auto &&last_tempban : last_tempbans_vector)
        {
            unsigned long guid_number{};
            if (!check_ip_address_validity(last_tempban.ip_address, guid_number) ||
                main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(last_tempban.ip_address))
                continue;

            main_app.get_current_game_server().get_temp_banned_ip_addresses_map().emplace(last_tempban.ip_address,
                                                                                          last_tempban);
            main_app.get_current_game_server().get_temp_banned_ip_addresses_vector().push_back(std::move(last_tempban));
        }

        sort(begin(main_app.get_current_game_server().get_temp_banned_ip_addresses_vector()),
             end(main_app.get_current_game_server().get_temp_banned_ip_addresses_vector()),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });

        // save_tempbans_to_file(main_app.get_temp_bans_file_path(),
        // main_app.get_current_game_server().get_temp_banned_ip_addresses_vector());
        operation_completed_flag[0] = 1;
        if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
        {
            // const string message{ "^2Received updated ^1ban entries ^2from
            // ^5Tiny^6Rcon ^5server." };
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // message.c_str());
            main_app.set_is_bans_synchronized(true);
        }
    });

    // receive-ipaddressbans-player
    main_app.add_remote_message_handler("receive-ipaddressbans-player", [](const string &, const time_t,
                                                                           const string &data, bool) {
        ofstream received_tempbans_file{L"last_ipaddressbans.txt"};
        if (received_tempbans_file)
        {
            received_tempbans_file << data << endl;
        }
        received_tempbans_file.close();
        vector<player> last_ipaddressbans_vector;
        unordered_map<string, player> last_ipaddressbans_map;
        parse_banned_ip_addresses_file("last_ipaddressbans.txt", last_ipaddressbans_vector, last_ipaddressbans_map);
        DeleteFileW(L"last_ipaddressbans.txt");
        // print_colored_text(app_handles.hwnd_re_messages_data,
        // format("^5Received last ^1{} IP address bans ^5from Tiny^6Rcon
        // ^5server.", last_ipaddressbans_map.size()).c_str());
        for (auto &&last_ipaddressban : last_ipaddressbans_vector)
        {
            unsigned long guid_number{};
            if (!check_ip_address_validity(last_ipaddressban.ip_address, guid_number) ||
                main_app.get_current_game_server().get_banned_ip_addresses_map().contains(last_ipaddressban.ip_address))
                continue;

            main_app.get_current_game_server().get_banned_ip_addresses_map().emplace(last_ipaddressban.ip_address,
                                                                                     last_ipaddressban);
            main_app.get_current_game_server().get_banned_ip_addresses_vector().push_back(std::move(last_ipaddressban));
        }

        sort(begin(main_app.get_current_game_server().get_banned_ip_addresses_vector()),
             end(main_app.get_current_game_server().get_banned_ip_addresses_vector()),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });

        // save_banned_ip_entries_to_file(main_app.get_ip_bans_file_path(),
        // main_app.get_current_game_server().get_banned_ip_addresses_vector());
        operation_completed_flag[1] = 1;
        if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
        {
            /*const string message{ "^2Received updated ^1ban entries ^2from
            ^5Tiny^6Rcon ^5server." };
            print_colored_text(app_handles.hwnd_re_messages_data,
            message.c_str());*/
            main_app.set_is_bans_synchronized(true);
        }
    });

    // receive-ipaddressrangebans-player
    main_app.add_remote_message_handler(
        "receive-ipaddressrangebans-player", [](const string &, const time_t, const string &data, bool) {
            ofstream received_tempbans_file{L"last_ipaddressrangebans.txt"};
            if (received_tempbans_file)
            {
                received_tempbans_file << data << endl;
            }
            received_tempbans_file.close();
            vector<player> last_ipaddressrangebans_vector;
            unordered_map<string, player> last_ipaddressrangebans_map;
            parse_banned_ip_address_ranges_file("last_ipaddressrangebans.txt", last_ipaddressrangebans_vector,
                                                last_ipaddressrangebans_map);
            DeleteFileW(L"last_ipaddressrangebans.txt");
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // format("^5Received last ^1{} IP address range bans ^5from Tiny^6Rcon
            // ^5server.", last_ipaddressrangebans_map.size()).c_str());
            for (auto &&last_ipaddressrangeban : last_ipaddressrangebans_vector)
            {

                if (!last_ipaddressrangeban.ip_address.ends_with(".*.*") &&
                    !last_ipaddressrangeban.ip_address.ends_with(".*"))
                    continue;

                if (main_app.get_current_game_server().get_banned_ip_address_ranges_map().contains(
                        last_ipaddressrangeban.ip_address))
                    continue;

                main_app.get_current_game_server().get_banned_ip_address_ranges_map().emplace(
                    last_ipaddressrangeban.ip_address, last_ipaddressrangeban);
                main_app.get_current_game_server().get_banned_ip_address_ranges_vector().push_back(
                    std::move(last_ipaddressrangeban));
            }

            sort(begin(main_app.get_current_game_server().get_banned_ip_address_ranges_vector()),
                 end(main_app.get_current_game_server().get_banned_ip_address_ranges_vector()),
                 [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });

            // save_banned_ip_address_range_entries_to_file(main_app.get_ip_range_bans_file_path(),
            // main_app.get_current_game_server().get_banned_ip_address_ranges_vector());
            operation_completed_flag[2] = 1;
            if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
            {
                // const string message{ "^2Received updated ^1ban entries ^2from
                // ^5Tiny^6Rcon ^5server." };
                // print_colored_text(app_handles.hwnd_re_messages_data,
                // message.c_str());
                main_app.set_is_bans_synchronized(true);
            }
        });

    // receive-namebans-player
    main_app.add_remote_message_handler("receive-namebans-player", [](const string &, const time_t, const string &data,
                                                                      bool) {
        ofstream received_tempbans_file{L"last_namebans.txt"};
        if (received_tempbans_file)
        {
            received_tempbans_file << data << endl;
        }
        received_tempbans_file.close();
        vector<player> last_namebans_vector;
        unordered_map<string, player> last_namebans_map;
        parse_banned_names_file("last_namebans.txt", last_namebans_vector, last_namebans_map);
        DeleteFileW(L"last_namebans.txt");
        // print_colored_text(app_handles.hwnd_re_messages_data,
        // format("^5Received ^1{} banned player names ^5from Tiny^6Rcon
        // ^5server.", last_namebans_map.size()).c_str());
        for (auto &&last_nameban : last_namebans_vector)
        {

            if (len(last_nameban.player_name) == 0 ||
                main_app.get_current_game_server().get_banned_names_map().contains(last_nameban.player_name))
                continue;

            main_app.get_current_game_server().get_banned_names_map().emplace(last_nameban.player_name, last_nameban);
            main_app.get_current_game_server().get_banned_names_vector().push_back(std::move(last_nameban));

            add_permanently_banned_player_name(last_nameban,
                                               main_app.get_current_game_server().get_banned_names_vector(),
                                               main_app.get_current_game_server().get_banned_names_map());
        }

        sort(begin(main_app.get_current_game_server().get_banned_names_vector()),
             end(main_app.get_current_game_server().get_banned_names_vector()),
             [](const player &p1, const player &p2) { return p1.banned_start_time < p2.banned_start_time; });

        // save_banned_ip_entries_to_file(main_app.get_banned_names_file_path(),
        // main_app.get_current_game_server().get_banned_names_vector());
        operation_completed_flag[5] = 1;
        if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
        {
            // const string message{ "^2Received updated ^1ban entries ^2from
            // ^5Tiny^6Rcon ^5server." };
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // message.c_str());
            main_app.set_is_bans_synchronized(true);
        }
    });

    // receive-citybans-player
    main_app.add_remote_message_handler(
        "receive-citybans-player", [](const string &, const time_t, const string &data, bool) {
            ofstream received_tempbans_file{L"last_citybans.txt"};
            if (received_tempbans_file)
            {
                received_tempbans_file << data << endl;
            }
            received_tempbans_file.close();
            set<string> last_citybans_set;
            parse_banned_cities_file("last_citybans.txt", last_citybans_set);
            DeleteFileW(L"last_citybans.txt");
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // format("^5Received ^1{} banned cities ^5from Tiny^6Rcon ^5server.",
            // last_citybans_set.size()).c_str());
            for (auto &&last_cityban : last_citybans_set)
            {
                if (main_app.get_current_game_server().get_banned_cities_set().contains(last_cityban))
                    continue;
                main_app.get_current_game_server().get_banned_cities_set().emplace(last_cityban);
            }

            // save_banned_cities_to_file(main_app.get_banned_cities_file_path(),
            // main_app.get_current_game_server().get_banned_cities_set());
            operation_completed_flag[3] = 1;
            if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
            {
                // const string message{ "^2Received updated ^1ban entries ^2from
                // ^5Tiny^6Rcon ^5server." };
                // print_colored_text(app_handles.hwnd_re_messages_data,
                // message.c_str());
                main_app.set_is_bans_synchronized(true);
            }
        });

    // receive-countrybans-player
    main_app.add_remote_message_handler(
        "receive-countrybans-player", [](const string &, const time_t, const string &data, bool) {
            ofstream received_tempbans_file{L"last_countrybans.txt"};
            if (received_tempbans_file)
            {
                received_tempbans_file << data << endl;
            }
            received_tempbans_file.close();
            set<string> last_countrybans_set;
            parse_banned_countries_file("last_countrybans.txt", last_countrybans_set);
            DeleteFileW(L"last_countrybans.txt");
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // format("^5Received ^1{} banned countries ^5from Tiny^6Rcon ^5server.",
            // last_countrybans_set.size()).c_str());

            for (auto &&last_countryban : last_countrybans_set)
            {
                if (main_app.get_current_game_server().get_banned_countries_set().contains(last_countryban))
                    continue;
                main_app.get_current_game_server().get_banned_countries_set().emplace(last_countryban);
            }

            // save_banned_countries_to_file(main_app.get_banned_countries_file_path(),
            // main_app.get_current_game_server().get_banned_countries_set());
            operation_completed_flag[4] = 1;
            if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
            {
                // const string message{ "^2Received updated ^1ban entries ^2from
                // ^5Tiny^6Rcon ^5server." };
                // print_colored_text(app_handles.hwnd_re_messages_data,
                // message.c_str());
                main_app.set_is_bans_synchronized(true);
            }
        });
    // ************************************************************

    main_app.add_message_handler("query-response", [](const string &, const time_t, const string &data, bool) {
        const string reply{trim(data.substr(data.rfind('=') + 1))};
        if (data.find(me->user_name) != string::npos &&
            data.find(main_app.get_game_servers()[0].get_rcon_password()) != string::npos && reply == "yes")
        {
            me->is_admin = true;
        }
    });

    main_app.add_message_handler("request-admindata", [](const string &, const time_t, const string &, bool) {});

    main_app.add_message_handler("receive-admindata", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
            stl::helper::trim_in_place(part);
        if (parts.size() >= 17)
        {
            auto &u = main_app.get_user_for_name(parts[0]);
            u->user_name = std::move(parts[0]);
            u->is_admin = parts[1] == "true";
            u->is_logged_in = parts[2] == "true";
            unsigned long guid{};
            if (check_ip_address_validity(parts[4], guid))
            {
                player admin{};
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), parts[4], admin);
                u->ip_address = std::move(parts[4]);
                u->country_code = admin.country_code;
                u->geo_information = format("{}, {}", admin.country_name, admin.city);
            }
            else
            {
                if (parts[5] != "n/a")
                {
                    u->geo_information = std::move(parts[5]);
                }
                else
                {
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
            if (parts.size() >= 18)
            {
                u->no_of_namebans = stoul(parts[17]);
            }
            if (parts.size() >= 19)
            {
                u->no_of_reports = stoul(parts[18]);
            }
        }
    });

    main_app.add_remote_message_handler(
        "receive-admindata-player", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
            for (auto &part : parts)
                stl::helper::trim_in_place(part);
            if (parts.size() >= 17)
            {
                auto &u = main_app.get_user_for_name(parts[0]);
                // main_app.set_is_user_data_received_for_user(parts[0]);
                u->user_name = std::move(parts[0]);
                u->is_admin = parts[1] == "true";
                u->is_logged_in = parts[2] == "true";
                u->is_online = parts[3] == "true";
                u->ip_address = std::move(parts[4]);
                u->geo_information = std::move(parts[5]);
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
                if (parts.size() >= 18)
                {
                    u->no_of_namebans = stoul(parts[17]);
                }
                if (parts.size() >= 19)
                {
                    u->no_of_reports = stoul(parts[18]);
                }
            }
        });

    main_app.add_message_handler("receive-mapnames", [](const string &, const time_t, const string &data, bool) {
        auto lines =
            stl::helper::str_split(data, "\n", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
        size_t custom_maps_count{};
        for (auto &line : lines)
        {
            trim_in_place(line);
            if (line.empty())
                continue;
            auto parts =
                stl::helper::str_split(line, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
            if (parts.size() >= 3)
            {
                for (auto &part : parts)
                {
                    trim_in_place(part);
                }
                main_app.get_available_rcon_to_full_map_names()[parts[0]] = make_pair(parts[1], parts[2]);
                main_app.get_available_full_map_to_rcon_map_names()[parts[1]] = parts[0];
                ++custom_maps_count;
            }
        }

        const string received_mapnames_message{
            format("^5Received information about ^1{} ^5custom maps.\n", custom_maps_count)};
        print_colored_text(app_handles.hwnd_re_messages_data, received_mapnames_message.c_str());

        ComboBox_ResetContent(app_handles.hwnd_combo_box_map);
        for (const auto &[rcon_map_name, full_map_name] : main_app.get_available_rcon_to_full_map_names())
        {
            ComboBox_AddString(app_handles.hwnd_combo_box_map, full_map_name.first.c_str());
        }
        if (main_app.get_available_rcon_to_full_map_names().contains("mp_toujane"))
        {
            SendMessage(app_handles.hwnd_combo_box_map, CB_SELECTSTRING, static_cast<WPARAM>(-1),
                        reinterpret_cast<LPARAM>(
                            main_app.get_available_rcon_to_full_map_names().at("mp_toujane").first.c_str()));
        }

        // SendMessage(app_handles.hwnd_combo_box_gametype, CB_SELECTSTRING,
        // static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>("ctf"));

        main_app.set_is_custom_map_names_message_received(true);
    });

    main_app.add_message_handler("upload-bans", [](const string &, const time_t, const string &data, bool) {
        const unordered_map<string, string> files_to_compress{
            {"tempbans", format("{}data\\tempbans.txt", main_app.get_current_working_directory())},
            {"bans", format("{}data\\bans.txt", main_app.get_current_working_directory())},
            {"ip_range_bans", format("{}data\\ip_range_bans.txt", main_app.get_current_working_directory())},
            {"banned_cities", format("{}data\\banned_cities.txt", main_app.get_current_working_directory())},
            {"banned_countries", format("{}data\\banned_countries.txt", main_app.get_current_working_directory())},
            {"banned_names", format("{}data\\banned_names.txt", main_app.get_current_working_directory())}};

        // const wstring wdata{ str_to_wstr(data) };
        const size_t first_digit_pos{data.find_first_of("0123456789")};
        if (first_digit_pos == string::npos)
            return;
        const string file_name_key{data.substr(0, first_digit_pos)};
        if (files_to_compress.contains(file_name_key))
        {

            upload_file_to_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(),
                                      main_app.get_tiny_rcon_ftp_server_username().c_str(),
                                      main_app.get_tiny_rcon_ftp_server_password().c_str(),
                                      files_to_compress.at(file_name_key).c_str(), data.c_str());
            // const string message{ format("^2Sent ^7{}^2's ^1{}.txt file ^2to
            // Tiny^6Rcon ^5server ^2for updating.", main_app.get_username(),
            // file_name_key) };
            // print_colored_text(app_handles.hwnd_re_messages_data,
            // message.c_str());
        }
    });

    main_app.add_message_handler("upload-bans-compressed", [](const string &, const time_t, const string &data, bool) {
        const unordered_map<string, string> files_to_compress{
            {"tempbans", format("{}data\\tempbans.txt", main_app.get_current_working_directory())},
            {"bans", format("{}data\\bans.txt", main_app.get_current_working_directory())},
            {"ip_range_bans", format("{}data\\ip_range_bans.txt", main_app.get_current_working_directory())},
            {"banned_cities", format("{}data\\banned_cities.txt", main_app.get_current_working_directory())},
            {"banned_countries", format("{}data\\banned_countries.txt", main_app.get_current_working_directory())},
            {"banned_names", format("{}data\\banned_names.txt", main_app.get_current_working_directory())}};

        // const wstring wdata{ str_to_wstr(data) };
        const string compressed_bans_file_path{format("{}data\\{}", main_app.get_current_working_directory(), data)};

        auto [success, error_msg] = create_7z_file_file_at_specified_path(
            {files_to_compress.at("tempbans"), files_to_compress.at("bans"), files_to_compress.at("ip_range_bans"),
             files_to_compress.at("banned_cities"), files_to_compress.at("banned_countries"),
             files_to_compress.at("banned_names")},
            compressed_bans_file_path);

        if (success && upload_file_to_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(),
                                                 main_app.get_tiny_rcon_ftp_server_username().c_str(),
                                                 main_app.get_tiny_rcon_ftp_server_password().c_str(),
                                                 compressed_bans_file_path.c_str(), data.c_str()))
        {
            const string message{format("^2Sent ^7{}^2's ^1{} archive file ^2to "
                                        "^5Tiny^6Rcon ^5server ^2for updating.",
                                        main_app.get_username(), data)};
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            // DeleteFileA(compressed_bans_file_path.c_str());
        }
        else
        {
            const string message{format("^3Could not create or upload ^7{}^3's compressed ^1{} archive file "
                                        "^3to ^5Tiny^6Rcon ^3server for updating!",
                                        main_app.get_username(), data)};
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            if (check_if_file_path_exists(compressed_bans_file_path.c_str()))
            {
                DeleteFileA(compressed_bans_file_path.c_str());
            }
        }
    });

    main_app.add_message_handler("receive-bans-compressed", [](const string &, const time_t, const string &data, bool) {
        const unordered_map<string, string> file_names_to_file_paths{
            {"tempbans", format("{}data\\tempbans.txt", main_app.get_current_working_directory())},
            {"bans", format("{}data\\bans.txt", main_app.get_current_working_directory())},
            {"ip_range_bans", format("{}data\\ip_range_bans.txt", main_app.get_current_working_directory())},
            {"banned_cities", format("{}data\\banned_cities.txt", main_app.get_current_working_directory())},
            {"banned_countries", format("{}data\\banned_countries.txt", main_app.get_current_working_directory())},
            {"banned_names", format("{}data\\banned_names.txt", main_app.get_current_working_directory())}};

        // const string wdata{ str_to_wstr(data) };
        const string updated_bans_7z_file_path{format("{}data\\{}", main_app.get_current_working_directory(), data)};
        if (check_if_file_path_exists(updated_bans_7z_file_path.c_str()))
        {
            DeleteFileA(updated_bans_7z_file_path.c_str());
        }

        if (download_file_from_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(),
                                          main_app.get_tiny_rcon_ftp_server_username().c_str(),
                                          main_app.get_tiny_rcon_ftp_server_password().c_str(),
                                          updated_bans_7z_file_path.c_str(), data.c_str()))
        {

            const string extract_to_path{format("{}data\\", main_app.get_current_working_directory())};

            auto [success, error_msg] =
                extract_7z_file_to_specified_path(updated_bans_7z_file_path.c_str(), extract_to_path.c_str());
            DeleteFileA(updated_bans_7z_file_path.c_str());

            if (success)
            {

                parse_tempbans_data_file(file_names_to_file_paths.at("tempbans").c_str(),
                                         main_app.get_current_game_server().get_temp_banned_ip_addresses_vector(),
                                         main_app.get_current_game_server().get_temp_banned_ip_addresses_map());

                parse_banned_ip_addresses_file(file_names_to_file_paths.at("bans").c_str(),
                                               main_app.get_current_game_server().get_banned_ip_addresses_vector(),
                                               main_app.get_current_game_server().get_banned_ip_addresses_map());

                parse_banned_ip_address_ranges_file(
                    file_names_to_file_paths.at("ip_range_bans").c_str(),
                    main_app.get_current_game_server().get_banned_ip_address_ranges_vector(),
                    main_app.get_current_game_server().get_banned_ip_address_ranges_map());

                parse_banned_cities_file(file_names_to_file_paths.at("banned_cities").c_str(),
                                         main_app.get_current_game_server().get_banned_cities_set());

                parse_banned_countries_file(file_names_to_file_paths.at("banned_countries").c_str(),
                                            main_app.get_current_game_server().get_banned_countries_set());

                parse_banned_names_file(file_names_to_file_paths.at("banned_names").c_str(),
                                        main_app.get_current_game_server().get_banned_names_vector(),
                                        main_app.get_current_game_server().get_banned_names_map());

                const string message{"^2Received updated ^1ban entries ^2from ^5Tiny^6Rcon ^5server."};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());

                main_app.set_is_bans_synchronized(true);
            }
        }
    });

    main_app.add_message_handler("receive-bans", [](const string &, const time_t, const string &data, bool) {
        const unordered_map<string, string> files_to_compress{
            {"tempbans", format("{}data\\tempbans.txt", main_app.get_current_working_directory())},
            {"bans", format("{}data\\bans.txt", main_app.get_current_working_directory())},
            {"ip_range_bans", format("{}data\\ip_range_bans.txt", main_app.get_current_working_directory())},
            {"banned_cities", format("{}data\\banned_cities.txt", main_app.get_current_working_directory())},
            {"banned_countries", format("{}data\\banned_countries.txt", main_app.get_current_working_directory())},
            {"banned_names", format("{}data\\banned_names.txt", main_app.get_current_working_directory())}};

        // const wstring wdata{ str_to_wstr(data) };
        const size_t first_digit_pos{data.find_first_of("0123456789")};
        if (first_digit_pos == string::npos)
            return;

        const string file_name_key{data.substr(0, first_digit_pos)};
        if (files_to_compress.contains(file_name_key))
        {

            if (download_file_from_ftp_server(main_app.get_tiny_rcon_server_ip_address().c_str(),
                                              main_app.get_tiny_rcon_ftp_server_username().c_str(),
                                              main_app.get_tiny_rcon_ftp_server_password().c_str(),
                                              files_to_compress.at(file_name_key).c_str(), data.c_str()))
            {
                if (file_name_key == "tempbans")
                {
                    parse_tempbans_data_file(main_app.get_temp_bans_file_path(),
                                             main_app.get_current_game_server().get_temp_banned_ip_addresses_vector(),
                                             main_app.get_current_game_server().get_temp_banned_ip_addresses_map());
                    operation_completed_flag[0] = 1;
                }
                else if (file_name_key == "bans")
                {
                    parse_banned_ip_addresses_file(main_app.get_ip_bans_file_path(),
                                                   main_app.get_current_game_server().get_banned_ip_addresses_vector(),
                                                   main_app.get_current_game_server().get_banned_ip_addresses_map());
                    operation_completed_flag[1] = 1;
                }
                else if (file_name_key == "ip_range_bans")
                {
                    parse_banned_ip_address_ranges_file(
                        main_app.get_ip_range_bans_file_path(),
                        main_app.get_current_game_server().get_banned_ip_address_ranges_vector(),
                        main_app.get_current_game_server().get_banned_ip_address_ranges_map());
                    operation_completed_flag[2] = 1;
                }
                else if (file_name_key == "banned_cities")
                {
                    parse_banned_cities_file(main_app.get_banned_cities_file_path(),
                                             main_app.get_current_game_server().get_banned_cities_set());
                    operation_completed_flag[3] = 1;
                }
                else if (file_name_key == "banned_countries")
                {
                    parse_banned_countries_file(main_app.get_banned_countries_file_path(),
                                                main_app.get_current_game_server().get_banned_countries_set());
                    operation_completed_flag[4] = 1;
                }
                else if (file_name_key == "banned_names")
                {
                    parse_banned_names_file(main_app.get_banned_names_file_path(),
                                            main_app.get_current_game_server().get_banned_names_vector(),
                                            main_app.get_current_game_server().get_banned_names_map());
                    operation_completed_flag[5] = 1;
                }
                if (6 == std::accumulate(operation_completed_flag, operation_completed_flag + 6, 0))
                {
                    // const string message{ "^2Received updated ^1ban entries ^2from
                    // ^5Tiny^6Rcon ^5server." };
                    // print_colored_text(app_handles.hwnd_re_messages_data,
                    // message.c_str());
                    main_app.set_is_bans_synchronized(true);
                }
            }
        }
    });

    main_app.add_message_handler(
        "request-protectedipaddresses", [](const string &, const time_t, const string &, bool) {
            const string message{"^3Requesting ^1protected IP addresses^3, ^1IP address ranges^3, "
                                 "^1cities ^3and ^1countries ^3from ^5Tiny^6Rcon ^5server."};
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        });

    main_app.add_remote_message_handler(
        "request-protectedipaddresses-player", [](const string &, const time_t, const string &, bool) {
            const string message{"^3Requesting ^1protected IP addresses^3, ^1IP address ranges^3, "
                                 "^1cities ^3and ^1countries ^3from ^5Tiny^6Rcon ^5server."};
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        });

    main_app.add_message_handler(
        "receive-protectedipaddresses", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
            auto &protected_ip_addresses = main_app.get_current_game_server().get_protected_ip_addresses();
            protected_ip_addresses.clear();
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
                unsigned long guid_key{};
                if (check_ip_address_validity(part, guid_key))
                {
                    protected_ip_addresses.insert(std::move(part));
                }
            }

            save_protected_entries_file(main_app.get_protected_ip_addresses_file_path(), protected_ip_addresses);
        });

    main_app.add_remote_message_handler(
        "receive-protectedipaddresses-player", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
            auto &protected_ip_addresses = main_app.get_current_game_server().get_protected_ip_addresses();
            protected_ip_addresses.clear();
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
                unsigned long guid_key{};
                if (check_ip_address_validity(part, guid_key))
                {
                    protected_ip_addresses.insert(std::move(part));
                }
            }
        });

    main_app.add_message_handler("request-protectedipaddressranges",
                                 [](const string &, const time_t, const string &, bool) {});

    main_app.add_message_handler("receive-protectedipaddressranges", [](const string &, const time_t,
                                                                        const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
        auto &protected_ip_address_ranges = main_app.get_current_game_server().get_protected_ip_address_ranges();
        protected_ip_address_ranges.clear();
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
            protected_ip_address_ranges.insert(std::move(part));
        }
        save_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(), protected_ip_address_ranges);
    });

    main_app.add_remote_message_handler(
        "receive-protectedipaddressranges-player", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
            auto &protected_ip_address_ranges = main_app.get_current_game_server().get_protected_ip_address_ranges();
            protected_ip_address_ranges.clear();
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
                protected_ip_address_ranges.insert(std::move(part));
            }
        });

    main_app.add_message_handler("request-protectedcities", [](const string &, const time_t, const string &, bool) {});

    main_app.add_message_handler("receive-protectedcities", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
        auto &protected_cities = main_app.get_current_game_server().get_protected_cities();
        protected_cities.clear();
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
            protected_cities.insert(std::move(part));
        }
        save_protected_entries_file(main_app.get_protected_cities_file_path(), protected_cities);
    });

    main_app.add_remote_message_handler(
        "receive-protectedcities-player", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
            auto &protected_cities = main_app.get_current_game_server().get_protected_cities();
            protected_cities.clear();
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
                protected_cities.insert(std::move(part));
            }
        });

    main_app.add_message_handler("request-protectedcountries",
                                 [](const string &, const time_t, const string &, bool) {});

    main_app.add_message_handler(
        "receive-protectedcountries", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
            auto &protected_countries = main_app.get_current_game_server().get_protected_countries();
            protected_countries.clear();
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
                protected_countries.insert(std::move(part));
            }
            save_protected_entries_file(main_app.get_protected_countries_file_path(), protected_countries);
            const string message{"^2Received ^1protected IP addresses^2, ^1IP address ranges^2, "
                                 "^1cities ^2and ^1countries ^2from ^5Tiny^6Rcon ^5server."};
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        });

    main_app.add_remote_message_handler(
        "receive-protectedcountries-player", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::yes);
            auto &protected_countries = main_app.get_current_game_server().get_protected_countries();
            protected_countries.clear();
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
                protected_countries.insert(std::move(part));
            }
            const string message{"^2Received ^1protected IP addresses^2, ^1IP address ranges^2, "
                                 "^1cities ^2and ^1countries ^2from ^5Tiny^6Rcon ^5server."};
            print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
        });

    main_app.add_message_handler("add-warning", [](const string &user, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5)
        {
            player warned_player{};
            warned_player.ip_address = parts[0];
            strcpy_s(warned_player.guid_key, std::size(warned_player.guid_key), parts[1].c_str());
            strcpy_s(warned_player.player_name, std::size(warned_player.player_name), parts[2].c_str());
            strcpy_s(warned_player.banned_date_time, std::size(warned_player.banned_date_time), parts[3].c_str());
            warned_player.banned_start_time =
                get_number_of_seconds_from_date_and_time_string(warned_player.banned_date_time);
            warned_player.reason = remove_disallowed_characters_in_string(parts[4]);
            warned_player.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : "^1Admin";
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                             warned_player.ip_address, warned_player);
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                             warned_player.ip_address, warned_player);
            const string msg{format("^7{} ^3has ^1warned ^3player ^7{} ^3[^5IP address ^1{} ^3| "
                                    "^5geoinfo: ^1{}, {}\n^5Date/time of warning: ^1{} ^3| ^5Reason of "
                                    "warning: ^1{} ^3| ^5Warned by: ^7{}\n",
                                    user, warned_player.player_name, warned_player.ip_address,
                                    warned_player.country_name, warned_player.city, warned_player.banned_date_time,
                                    warned_player.reason, user)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
        }
    });

    main_app.add_message_handler("inform-message", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() >= 3)
        {
            bool is_date_and_time_info_displayed{};
            /*const string received_from_message{ format("^2Received message from
            ^7{}:\n", parts[1]) };
            print_colored_text(app_handles.hwnd_re_messages_data,
            received_from_message.c_str());*/
            if (!parts[2].empty() && parts[0] == "accept")
            {
                rcon_say(parts[2], true);
                is_date_and_time_info_displayed = true;
            }

            for (size_t i{3}; i < parts.size(); ++i)
            {
                if (!parts[i].empty())
                {
                    print_colored_text(app_handles.hwnd_re_messages_data, parts[i].c_str(),
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       !is_date_and_time_info_displayed ? is_log_datetime::yes : is_log_datetime::no,
                                       true, true);
                    if (!is_date_and_time_info_displayed)
                        is_date_and_time_info_displayed = true;
                }
            }
        }
    });

    main_app.add_remote_message_handler("inform-message", [](const string &, const time_t, const string &data, bool) {
        print_colored_text(app_handles.hwnd_re_messages_data, data.c_str(), is_append_message_to_richedit_control::yes,
                           is_log_message::yes, is_log_datetime::yes, false, true);
    });

    main_app.add_remote_message_handler("inform-message-player", [](const string &, const time_t, const string &data,
                                                                    bool) {
        print_colored_text(app_handles.hwnd_re_messages_data, data.c_str(), is_append_message_to_richedit_control::yes,
                           is_log_message::yes, is_log_datetime::yes, false, true);
    });

    main_app.add_message_handler("protect-ipaddress", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() >= 3)
        {
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
            }
            if (!main_app.get_current_game_server().get_protected_ip_addresses().contains(parts[1]))
            {
                main_app.get_current_game_server().get_protected_ip_addresses().emplace(parts[1]);
                player protected_player{};
                protected_player.ip_address = parts[1];
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), parts[1],
                                                 protected_player);
                const string message{format("^7{} ^2added IP address ^1{} ^5[{}, {}] ^2to the list of "
                                            "protected ^1IP addresses^2.\n^5Reason: ^1{}",
                                            parts[0], parts[1], protected_player.country_name, protected_player.city,
                                            parts[2])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
            else
            {
                const string message{format("^3IP address ^1{} ^3is already protected.", parts[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
        }
    });

    main_app.add_message_handler("protect-ipaddressrange", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() >= 3)
        {
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
            }
            if (!main_app.get_current_game_server().get_protected_ip_address_ranges().contains(parts[1]))
            {
                main_app.get_current_game_server().get_protected_ip_address_ranges().emplace(parts[1]);
                const string message{format("^7{} ^2added IP address range ^1{} ^2to the list of protected "
                                            "^1IP address ranges^2.\n^5Reason: ^1{}",
                                            parts[0], parts[1], parts[2])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
            else
            {
                const string message{format("^3IP address range ^1{} ^3is already protected.", parts[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
        }
    });

    main_app.add_message_handler("protect-city", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() >= 3)
        {
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
            }
            if (!main_app.get_current_game_server().get_protected_cities().contains(parts[1]))
            {
                main_app.get_current_game_server().get_protected_cities().emplace(parts[1]);
                const string message{format("^7{} ^2added city ^1{} ^2to the list of protected "
                                            "^1cities^2.\n^5Reason: ^1{}",
                                            parts[0], parts[1], parts[2])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
            else
            {
                const string message{format("^3City ^1{} ^3is already protected.", parts[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
        }
    });

    main_app.add_message_handler("protect-country", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() >= 3)
        {
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
            }
            if (!main_app.get_current_game_server().get_protected_countries().contains(parts[1]))
            {
                main_app.get_current_game_server().get_protected_countries().emplace(parts[1]);
                const string message{format("^7{} ^2added country ^1{} ^2to the list of protected "
                                            "^1countries^2.\n^5Reason: ^1{}",
                                            parts[0], parts[1], parts[2])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
            else
            {
                const string message{format("^3Country ^1{} ^3is already protected.", parts[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
        }
    });

    main_app.add_message_handler("unprotect-ipaddress", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() >= 3)
        {
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
            }
            if (main_app.get_current_game_server().get_protected_ip_addresses().contains(parts[1]))
            {
                main_app.get_current_game_server().get_protected_ip_addresses().erase(parts[1]);
                player protected_player{};
                protected_player.ip_address = parts[1];
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), parts[1],
                                                 protected_player);
                const string message{format("^7{} ^3has removed protected IP address ^1{} ^5[{}, {}] ^3from "
                                            "the list of protected ^1IP addresses^3.\n^5Reason: ^1{}",
                                            parts[0], parts[1], protected_player.country_name, protected_player.city,
                                            parts[2])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
            else
            {
                const string message{format("^3IP address ^1{} ^3is not protected.", parts[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
        }
    });

    main_app.add_message_handler(
        "unprotect-ipaddressrange", [](const string &, const time_t, const string &data, bool) {
            auto parts =
                stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
            if (parts.size() >= 3)
            {
                for (auto &part : parts)
                {
                    stl::helper::trim_in_place(part);
                }
                if (main_app.get_current_game_server().get_protected_ip_address_ranges().contains(parts[1]))
                {
                    main_app.get_current_game_server().get_protected_ip_address_ranges().erase(parts[1]);
                    const string message{format("^7{} ^3has removed protected IP address range ^1{} ^3from the "
                                                "list of protected ^1IP address ranges^3.\n^5Reason: ^1{}",
                                                parts[0], parts[1], parts[2])};
                    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
                }
                else
                {
                    const string message{format("^3IP address range ^1{} ^3is not protected.", parts[1])};
                    print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
                }
            }
        });

    main_app.add_message_handler("unprotect-city", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() >= 3)
        {
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
            }
            if (main_app.get_current_game_server().get_protected_cities().contains(parts[1]))
            {
                main_app.get_current_game_server().get_protected_cities().erase(parts[1]);
                const string message{format("^7{} ^3has removed protected city ^1{} ^3from the list of "
                                            "protected ^1cities^3.\n^5Reason: ^1{}",
                                            parts[0], parts[1], parts[2])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
            else
            {
                const string message{format("^3City ^1{} ^3is not protected.", parts[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
        }
    });

    main_app.add_message_handler("unprotect-country", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        if (parts.size() >= 3)
        {
            for (auto &part : parts)
            {
                stl::helper::trim_in_place(part);
            }
            if (main_app.get_current_game_server().get_protected_countries().contains(parts[1]))
            {
                main_app.get_current_game_server().get_protected_countries().erase(parts[1]);
                const string message{format("^7{} ^3has removed protected country ^1{} ^3from the list of "
                                            "protected ^1countries^3.\n^5Reason: ^1{}",
                                            parts[0], parts[1], parts[2])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
            else
            {
                const string message{format("^3Country ^1{} ^3is not protected.", parts[1])};
                print_colored_text(app_handles.hwnd_re_messages_data, message.c_str());
            }
        }
    });

    main_app.add_remote_message_handler("add-report", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5)
        {
            int pid{-1};
            if (is_valid_decimal_whole_number(parts[2], pid) && pid >= 0 && pid < 64)
            {
                const auto current_ts = get_current_time_stamp();
                player &reported_player = main_app.get_game_servers()[0].get_player_data(pid);
                player pd{};
                strcpy_s(pd.player_name, std::size(pd.player_name), reported_player.player_name);
                strcpy_s(pd.guid_key, std::size(pd.guid_key), reported_player.guid_key);
                pd.ip_address = reported_player.ip_address;
                const string date_and_time_of_report{
                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", current_ts)};
                strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), date_and_time_of_report.c_str());
                pd.banned_start_time = current_ts;
                pd.reason = remove_disallowed_characters_in_string(parts[4]);
                pd.banned_by_user_name = parts[0];
                convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);
                {
                    lock_guard lk{reports_mutex};
                    main_app.get_reported_players().emplace_back(pd);
                }

                const string private_msg{format("^7{} ^5has ^1reported ^5player ^7{} ^5[^3PID: ^1{} ^5| ^3Score "
                                                "^1{} ^5| ^3Ping: ^1{}\n ^3IP: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| "
                                                "^3Date/time of report: ^1{} ^5| ^3Reason: ^1{}^5]\n",
                                                parts[0], pd.player_name, pd.pid, pd.score, pd.ping, pd.ip_address,
                                                pd.country_name, pd.city, pd.banned_date_time, pd.reason)};
                print_colored_text(app_handles.hwnd_re_messages_data, private_msg.c_str());
            }
        }
    });

    main_app.add_message_handler("add-kick", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5)
        {
            player kicked_player{};
            kicked_player.ip_address = parts[0];
            strcpy_s(kicked_player.guid_key, std::size(kicked_player.guid_key), parts[1].c_str());
            strcpy_s(kicked_player.player_name, std::size(kicked_player.player_name), parts[2].c_str());
            strcpy_s(kicked_player.banned_date_time, std::size(kicked_player.banned_date_time), parts[3].c_str());
            kicked_player.banned_start_time =
                get_number_of_seconds_from_date_and_time_string(kicked_player.banned_date_time);
            kicked_player.reason = remove_disallowed_characters_in_string(parts[4]);
            kicked_player.banned_by_user_name = parts.size() >= 6 ? parts[5] : "^1Admin";
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                             kicked_player.ip_address, kicked_player);
            const string msg{format("^7{} ^3has ^1kicked ^3player ^7{} ^3[^5IP address ^1{} ^3| "
                                    "^5geoinfo: ^1{}, {}\n^5Date/time of kick: ^1{} ^3| ^5Reason of "
                                    "kick: ^1{} ^3| ^5Kicked by: ^7{}\n",
                                    kicked_player.banned_by_user_name, kicked_player.player_name,
                                    kicked_player.ip_address, kicked_player.country_name, kicked_player.city,
                                    kicked_player.banned_date_time, kicked_player.reason,
                                    kicked_player.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
        }
    });

    main_app.add_message_handler("add-tempban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 6 &&
            !main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(parts[0]))
        {
            player temp_banned_player_data{};
            temp_banned_player_data.ip_address = parts[0];
            strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name),
                     parts[1].c_str());
            strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time),
                     parts[2].c_str());
            temp_banned_player_data.banned_start_time = stoll(parts[3]);
            temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
            temp_banned_player_data.reason = remove_disallowed_characters_in_string(parts[5]);
            temp_banned_player_data.banned_by_user_name = parts.size() >= 7 ? parts[6] : "^1Admin";
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                             temp_banned_player_data.ip_address, temp_banned_player_data);

            const string msg{format("^7{} ^5has temporarily banned ^1IP address {}\n ^5for ^3player "
                                    "name: ^7{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: "
                                    "^1{}\n^3Ban duration: ^1{} hours ^5| ^3Reason of ban: ^1{} ^5| "
                                    "^3Banned by: ^7{}\n",
                                    temp_banned_player_data.banned_by_user_name, temp_banned_player_data.ip_address,
                                    temp_banned_player_data.player_name, temp_banned_player_data.country_name,
                                    temp_banned_player_data.city, temp_banned_player_data.banned_date_time,
                                    temp_banned_player_data.ban_duration_in_hours, temp_banned_player_data.reason,
                                    temp_banned_player_data.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            add_temporarily_banned_ip_address(temp_banned_player_data,
                                              main_app.get_current_game_server().get_temp_banned_ip_addresses_vector(),
                                              main_app.get_current_game_server().get_temp_banned_ip_addresses_map());
        }
    });

    main_app.add_message_handler("remove-tempban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 6 &&
            main_app.get_current_game_server().get_temp_banned_ip_addresses_map().contains(parts[0]))
        {
            player temp_banned_player_data{};
            temp_banned_player_data.ip_address = parts[0];
            strcpy_s(temp_banned_player_data.player_name, std::size(temp_banned_player_data.player_name),
                     parts[1].c_str());
            strcpy_s(temp_banned_player_data.banned_date_time, std::size(temp_banned_player_data.banned_date_time),
                     parts[2].c_str());
            temp_banned_player_data.banned_start_time = stoll(parts[3]);
            temp_banned_player_data.ban_duration_in_hours = stoll(parts[4]);
            temp_banned_player_data.reason = remove_disallowed_characters_in_string(parts[5]);
            temp_banned_player_data.banned_by_user_name = parts.size() >= 7 ? parts[6] : "^1Admin";
            const string removed_by{parts.size() >= 8 ? parts[7] : "^1Admin"};
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(),
                                             temp_banned_player_data.ip_address, temp_banned_player_data);

            const string msg{format("^7{} ^5has removed temporarily banned ^1IP address {}\n ^5for "
                                    "^3player name: ^7{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: "
                                    "^1{}\n^3Ban duration: ^1{} hours ^5| ^3Reason of ban: ^1{} ^5| "
                                    "^3Banned by: ^7{}\n",
                                    removed_by, temp_banned_player_data.ip_address, temp_banned_player_data.player_name,
                                    temp_banned_player_data.country_name, temp_banned_player_data.city,
                                    temp_banned_player_data.banned_date_time,
                                    temp_banned_player_data.ban_duration_in_hours, temp_banned_player_data.reason,
                                    temp_banned_player_data.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            string message_about_removal;
            remove_temp_banned_ip_address(temp_banned_player_data.ip_address, message_about_removal, false, false);
        }
    });

    main_app.add_message_handler("add-guidban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5 && !main_app.get_current_game_server().get_banned_ip_addresses_map().contains(parts[0]))
        {
            player pd{};
            pd.ip_address = parts[0];
            strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
            strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
            pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
            pd.reason = remove_disallowed_characters_in_string(parts[4]);
            pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : "^1Admin";
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

            const string msg{format("^7{} ^5has banned the ^1GUID key ^5of player:\n^3Name: ^7{} ^5| "
                                    "^3IP address: ^1{} ^5| ^3GUID: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| "
                                    "^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: "
                                    "^7{}\n",
                                    pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.guid_key, pd.country_name,
                                    pd.city, pd.banned_date_time, pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
        }
    });

    main_app.add_message_handler("add-nameban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5 && !main_app.get_current_game_server().get_banned_names_map().contains(parts[2]))
        {
            player pd{};
            pd.ip_address = parts[0];
            strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
            strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
            pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
            pd.reason = remove_disallowed_characters_in_string(parts[4]);
            pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : "^1Admin";
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

            const string msg{format("^7{} ^5has banned ^1player name: ^7{}\n^3IP address: ^1{} ^5| "
                                    "^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: "
                                    "^1{} ^5| ^3Banned by: ^7{}\n",
                                    pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.country_name, pd.city,
                                    pd.banned_date_time, pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            add_permanently_banned_player_name(pd, main_app.get_current_game_server().get_banned_names_vector(),
                                               main_app.get_current_game_server().get_banned_names_map());
        }
    });

    main_app.add_message_handler("remove-nameban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5 && main_app.get_current_game_server().get_banned_names_map().contains(parts[2]))
        {
            player pd{};
            pd.ip_address = parts[0];
            strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
            strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
            pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
            pd.reason = remove_disallowed_characters_in_string(parts[4]);
            pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
            const string removed_by{parts.size() >= 7 ? parts[6] : "^1Admin"};
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

            const string msg{format("^7{} ^5has removed ^1banned player name ^7{}\n^3IP address: ^1{} "
                                    "^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of "
                                    "ban: ^1{} ^5| ^3Banned by: ^7{}\n",
                                    removed_by, pd.player_name, pd.ip_address, pd.country_name, pd.city,
                                    pd.banned_date_time, pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            remove_permanently_banned_player_name(pd, main_app.get_current_game_server().get_banned_names_vector(),
                                                  main_app.get_current_game_server().get_banned_names_map());
        }
    });

    main_app.add_message_handler("add-ipban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5 && !main_app.get_current_game_server().get_banned_ip_addresses_map().contains(parts[0]))
        {
            player pd{};
            pd.ip_address = parts[0];
            strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
            strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
            pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
            pd.reason = remove_disallowed_characters_in_string(parts[4]);
            pd.banned_by_user_name = (parts.size() >= 6) ? parts[5] : "^1Admin";
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

            const string msg{format("^7{} ^5has banned the ^1IP address ^5of player:\n^3Name: ^7{} ^5| "
                                    "^3IP address: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: "
                                    "^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}\n",
                                    pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.country_name, pd.city,
                                    pd.banned_date_time, pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            add_permanently_banned_ip_address(pd, main_app.get_current_game_server().get_banned_ip_addresses_vector(),
                                              main_app.get_current_game_server().get_banned_ip_addresses_map());
        }
    });

    main_app.add_message_handler("remove-ipban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5 && main_app.get_current_game_server().get_banned_ip_addresses_map().contains(parts[0]))
        {
            player pd{};
            pd.ip_address = parts[0];
            strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
            strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
            pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
            pd.reason = remove_disallowed_characters_in_string(parts[4]);
            pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
            const string removed_by{parts.size() >= 7 ? parts[6] : "^1Admin"};
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

            const string msg{format("^7{} ^5has removed ^1banned IP address {}\n ^5for ^3player name: "
                                    "^7{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason "
                                    "of ban: ^1{} ^5| ^3Banned by: ^7{}\n",
                                    removed_by, pd.ip_address, pd.player_name, pd.country_name, pd.city,
                                    pd.banned_date_time, pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            string ip_address{pd.ip_address};
            string message_about_removal;
            remove_permanently_banned_ip_address(ip_address, message_about_removal, false);
        }
    });

    main_app.add_message_handler("add-iprangeban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 5 &&
            !main_app.get_current_game_server().get_banned_ip_address_ranges_map().contains(parts[0]))
        {
            player pd{};
            pd.ip_address = parts[0];
            strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
            strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
            pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
            pd.reason = remove_disallowed_characters_in_string(parts[4]);
            pd.banned_by_user_name = (parts.size() >= 6) ? std::move(parts[5]) : "^1Admin";
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

            const string msg{format("^7{} ^5has banned ^1IP address range:\n^5[^3Player name: ^7{} ^5| "
                                    "^3IP address range: ^1{} ^5| ^3geoinfo: ^1{}, {} ^5| ^3Date/time of "
                                    "ban: ^1{}\n^3Reason of ban: ^1{} ^5| ^3Banned by: ^7{}^5]\n",
                                    pd.banned_by_user_name, pd.player_name, pd.ip_address, pd.country_name, pd.city,
                                    pd.banned_date_time, pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            add_permanently_banned_ip_address_range(
                pd, main_app.get_current_game_server().get_banned_ip_address_ranges_vector(),
                main_app.get_current_game_server().get_banned_ip_address_ranges_map());
        }
    });

    main_app.add_message_handler("remove-iprangeban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (main_app.get_current_game_server().get_banned_ip_address_ranges_map().contains(parts[0]))
        {
            player pd{};
            pd.ip_address = parts[0];
            strcpy_s(pd.guid_key, std::size(pd.guid_key), parts[1].c_str());
            strcpy_s(pd.player_name, std::size(pd.player_name), parts[2].c_str());
            strcpy_s(pd.banned_date_time, std::size(pd.banned_date_time), parts[3].c_str());
            pd.banned_start_time = get_number_of_seconds_from_date_and_time_string(pd.banned_date_time);
            pd.reason = remove_disallowed_characters_in_string(parts[4]);
            pd.banned_by_user_name = parts.size() >= 6 ? std::move(parts[5]) : "^1Admin";
            const string removed_by{parts.size() >= 7 ? parts[6] : "^1Admin"};
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), pd.ip_address, pd);

            const string msg{format("^7{} ^5has removed previously ^1banned IP address "
                                    "range:\n^5[^3Player name: ^7{} ^5| ^3IP range: ^1{} ^5| ^3geoinfo: "
                                    "^1{}, {} ^5| ^3Date/time of ban: ^1{}\n^3Reason of ban: ^1{} ^5| "
                                    "^3Banned by: ^7{}^5]\n",
                                    removed_by, pd.player_name, pd.ip_address, pd.country_name, pd.city,
                                    pd.banned_date_time, pd.reason, pd.banned_by_user_name)};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            string ip_address{pd.ip_address};
            string message_about_removal;
            remove_permanently_banned_ip_address_range(
                pd, main_app.get_current_game_server().get_banned_ip_address_ranges_vector(),
                main_app.get_current_game_server().get_banned_ip_address_ranges_map());
        }
    });

    main_app.add_message_handler("add-cityban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 3U)
        {
            const string city{parts[0]};
            const string admin{parts[1]};
            const time_t timestamp_of_ban{stoll(parts[2])};

            const string msg{format("^7{} ^3has banned city ^1{} ^3at ^1{}\n", admin, city,
                                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban))};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            add_permanently_banned_city(city, main_app.get_current_game_server().get_banned_cities_set());
        }
    });

    main_app.add_message_handler("remove-cityban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 3U)
        {
            const string city{parts[0]};
            const string admin{parts[1]};
            const time_t timestamp_of_ban{stoll(parts[2])};

            const string msg{format("^7{} ^2has removed banned city ^1{} ^2at ^1{}\n", admin, city,
                                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban))};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            remove_permanently_banned_city(city, main_app.get_current_game_server().get_banned_cities_set());
        }
    });

    main_app.add_message_handler("add-countryban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 3U)
        {
            const string country{parts[0]};
            const string admin{parts[1]};
            const time_t timestamp_of_ban{stoll(parts[2])};

            const string msg{format("^7{} ^3has banned country ^1{} ^3at ^1{}\n", admin, country,
                                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban))};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            add_permanently_banned_country(country, main_app.get_current_game_server().get_banned_countries_set());
        }
    });

    main_app.add_message_handler("remove-countryban", [](const string &, const time_t, const string &data, bool) {
        auto parts =
            stl::helper::str_split(data, "\\", nullptr, split_on_whole_needle_t::yes, ignore_empty_string_t::no);
        for (auto &part : parts)
        {
            stl::helper::trim_in_place(part);
        }

        if (parts.size() >= 3U)
        {
            const string city{parts[0]};
            const string admin{parts[1]};
            const time_t timestamp_of_ban{stoll(parts[2])};

            const string msg{format("^7{} ^2has removed banned country ^1{} ^2at ^1{}\n", admin, city,
                                    get_date_and_time_for_time_t("{DD}.{MM}.{Y} {hh}:{mm}", timestamp_of_ban))};
            print_colored_text(app_handles.hwnd_re_messages_data, msg.c_str());
            remove_permanently_banned_country(city, main_app.get_current_game_server().get_banned_countries_set());
        }
    });

    main_app.add_message_handler("restart-tinyrcon", [](const string &, const time_t, const string &, bool) {
        const string info_message{"^5Tiny^6Rcon ^5server ^3has sent a ^1request ^3to restart your "
                                  "^5Tiny^6Rcon ^3program.\n"};
        print_colored_text(app_handles.hwnd_re_messages_data, info_message.c_str());
        restart_tinyrcon_client(main_app.get_auto_update_manager().get_self_full_path().c_str());
    });

    const string program_title{main_app.get_program_title() + " | "s +
                               main_app.get_current_game_server().get_server_name() + " | "s + "version: "s +
                               program_version};
    main_app.set_program_title(program_title);
    SetWindowTextA(app_handles.hwnd_main_window, program_title.c_str());

    main_app.set_command_line_info(user_help_message);

    CenterWindow(app_handles.hwnd_main_window);

    SetFocus(app_handles.hwnd_main_window);
    PostMessage(app_handles.hwnd_progress_bar, PBM_SETMARQUEE, (WPARAM)TRUE, (LPARAM)5);

    std::thread task_thread{[&]() {
        IsGUIThread(TRUE);

        SendMessageA(app_handles.hwnd_main_window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        ShowWindow(app_handles.hwnd_main_window, SW_MAXIMIZE);
        UpdateWindow(app_handles.hwnd_main_window);

        version_data source_version{};
        unsigned long source_version_number{};

        const char *exe_file_path{main_app.get_exe_file_path()};

        main_app.get_auto_update_manager().get_file_version(exe_file_path, source_version, source_version_number);
        const string version_information{format("^2Current version of ^5Tiny^6Rcon ^2is ^5{}.{}.{}.{}\n",
                                                source_version.major, source_version.minor, source_version.revision,
                                                source_version.sub_revision)};
        print_colored_text(app_handles.hwnd_re_messages_data, version_information.c_str(),
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        main_app.get_connection_manager_for_messages().process_and_send_message(
            "tinyrcon-info",
            format("{}\\{}\\{}", main_app.get_username(), main_app.get_user_ip_address(), version_information), true,
            main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);

        main_app.get_auto_update_manager().check_for_updates(exe_file_path);

        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^3Started importing geological data from ^1'geo.dat' ^3file.\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);
        // const string geo_dat_file_path{main_app.get_current_working_directory() +
        // "plugins\\geoIP\\IP2LOCATION-LITE-DB3.CSV"}; parse_geodata_lite_csv_file(geo_dat_file_path.c_str()); const
        // string geo_dat_file_path{main_app.get_current_working_directory() + R"(plugins\geoIP\geo.dat)"};
        const string geo_dat_file_path{"plugins\\geoIP\\geo.dat"};
        import_geoip_data(main_app.get_connection_manager().get_geoip_data(), geo_dat_file_path.c_str());
        print_colored_text(app_handles.hwnd_re_messages_data,
                           "^2Finished importing geological data from ^1'geo.dat' ^2file.\n",
                           is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes);

        unsigned long guid{};
        if (check_ip_address_validity(me->ip_address, guid))
        {
            print_colored_text(app_handles.hwnd_re_messages_data,
                               format("^5Your current external IP address is ^1{}\n", me->ip_address).c_str());
            player pl{};
            convert_guid_key_to_country_name(main_app.get_connection_manager().get_geoip_data(), me->ip_address, pl);
            me->country_code = pl.country_code;
            me->geo_information = format("{}, {}", pl.country_name, pl.city);
        }
        else
        {
            me->ip_address = "n/a";
            me->geo_information = "n/a";
            me->country_code = "xy";
        }

        parse_protected_entries_file(main_app.get_protected_ip_addresses_file_path(),
                                     main_app.get_current_game_server().get_protected_ip_addresses());
        parse_protected_entries_file(main_app.get_protected_ip_address_ranges_file_path(),
                                     main_app.get_current_game_server().get_protected_ip_address_ranges());
        parse_protected_entries_file(main_app.get_protected_cities_file_path(),
                                     main_app.get_current_game_server().get_protected_cities());
        parse_protected_entries_file(main_app.get_protected_countries_file_path(),
                                     main_app.get_current_game_server().get_protected_countries());

        load_reported_players_to_file(main_app.get_reported_players_file_path(), main_app.get_reported_players());
        parse_tempbans_data_file(main_app.get_temp_bans_file_path(),
                                 main_app.get_current_game_server().get_temp_banned_ip_addresses_vector(),
                                 main_app.get_current_game_server().get_temp_banned_ip_addresses_map());
        parse_banned_ip_addresses_file(main_app.get_ip_bans_file_path(),
                                       main_app.get_current_game_server().get_banned_ip_addresses_vector(),
                                       main_app.get_current_game_server().get_banned_ip_addresses_map());
        parse_banned_ip_address_ranges_file(main_app.get_ip_range_bans_file_path(),
                                            main_app.get_current_game_server().get_banned_ip_address_ranges_vector(),
                                            main_app.get_current_game_server().get_banned_ip_address_ranges_map());
        parse_banned_cities_file(main_app.get_banned_cities_file_path(),
                                 main_app.get_current_game_server().get_banned_cities_set());
        parse_banned_countries_file(main_app.get_banned_countries_file_path(),
                                    main_app.get_current_game_server().get_banned_countries_set());
        parse_banned_names_file(main_app.get_banned_names_file_path(),
                                main_app.get_current_game_server().get_banned_names_vector(),
                                main_app.get_current_game_server().get_banned_names_map());

        main_app.get_bitmap_image_handler().load_bitmap_images();

        string rcon_reply;
        if (main_app.get_game_servers_count() != 0)
        {

            auto &gs = main_app.get_game_servers()[0];
            main_app.get_connection_manager().send_and_receive_rcon_data(
                "status", rcon_reply, gs.get_server_ip_address().c_str(), gs.get_server_port(),
                gs.get_rcon_password().c_str(), gs, true, true);
            load_current_map_image(main_app.get_current_game_server().get_current_map());            
        }

        string game_version_number{"1.0"};
        try
        {

            game_version_number = find_version_of_installed_cod2_game();
            main_app.set_player_name(find_users_player_name_for_installed_cod2_game(
                me, !main_app.get_current_game_server().get_game_mod_name().empty()
                        ? main_app.get_current_game_server().get_game_mod_name()
                        : "main"s));

            me->player_name = main_app.get_player_name();
            main_app.get_current_game_server().set_game_version_number(game_version_number);
            main_app.set_game_version_number(game_version_number);
        }
        catch (...)
        {
            main_app.get_current_game_server().set_game_version_number(game_version_number);
            main_app.set_game_version_number(game_version_number);
        }

        /*string game_version_number{"1.0"};

        if (!main_app.get_cod2mp_exe_path().empty() &&
            check_if_file_path_exists(main_app.get_cod2mp_exe_path().c_str()))
        {
            game_version_number = find_version_of_installed_cod2_game();
            main_app.set_player_name(find_users_player_name_for_installed_cod2_game(
                me, !main_app.get_current_game_server().get_game_mod_name().empty()
                        ? main_app.get_current_game_server().get_game_mod_name()
                        : "main"s));
        }
        main_app.get_current_game_server().set_game_version_number(game_version_number);
        main_app.set_game_version_number(game_version_number);*/

        display_tempbanned_players_remaining_time_period();
        is_main_window_constructed = true;
        auto &game_servers = main_app.get_game_servers();
        main_app.set_game_server_index(0U);

        check_if_exists_and_download_missing_custom_map_files_downloader();

        while (true)
        {
            try
            {

                {
                    unique_lock ul{main_app.get_command_queue_mutex()};
                    main_app.get_command_queue_cv().wait_for(
                        ul, 20ms, [&]() { return !main_app.is_command_queue_empty() || is_terminate_program.load(); });
                }

                while (!is_terminate_program.load() && !main_app.is_command_queue_empty())
                {
                    auto cmd = main_app.get_command_from_queue();
                    main_app.process_queue_command(std::move(cmd));
                }

                if (!is_terminate_program.load() && is_refresh_players_data_event.load())
                {

                    const size_t game_server_index{main_app.get_game_server_index()};

                    if (game_server_index >= main_app.get_game_servers_count())
                    {
                        main_app.set_game_server_index(0U);
                    }

                    if (game_server_index < main_app.get_rcon_game_servers_count())
                    {
                        if (me->is_admin)
                        {
                            // get_player_pid_to_guid_response();
                            // Sleep(500);
                            // get_muted_guid_keys_response();
                            // Sleep(500);
                            game_server &rcon_gs = game_servers[0];
                            main_app.get_connection_manager().send_and_receive_rcon_data(
                                "status", rcon_reply, rcon_gs.get_server_ip_address().c_str(),
                                rcon_gs.get_server_port(), rcon_gs.get_rcon_password().c_str(), rcon_gs, true, true);
                            Sleep(100);
                        }
                        else
                        {
                            game_server &rcon_gs = game_servers[game_server_index];
                            const auto current_ts = get_current_time_stamp();
                            const auto time_elapsed_in_sec{
                                current_ts - main_app.get_connection_manager().get_last_rcon_status_received()};
                            if (time_elapsed_in_sec >= 10)
                            {
                                main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
                                    "rcon-heartbeat-player",
                                    format("{}\\{}\\{}", main_app.get_username(), current_ts, me->ip_address), true,
                                    main_app.get_private_tiny_rcon_server_ip_address(),
                                    main_app.get_private_tiny_rcon_server_port(), false);
                                main_app.get_connection_manager().send_and_receive_non_rcon_data(
                                    "getstatus", rcon_reply, rcon_gs.get_server_ip_address().c_str(),
                                    rcon_gs.get_server_port(), rcon_gs, true, true);
                            }
                            /*this_thread::sleep_for(50ms);
                            auto [status, game_name] =
                            check_if_specified_server_ip_port_and_rcon_password_are_valid(rcon_gs.get_server_ip_address().c_str(),
                            rcon_gs.get_server_port(), rcon_gs.get_rcon_password().c_str());
                            rcon_gs.set_is_connection_settings_valid(status);*/
                        }
                    }
                    else
                    {
                        if (me->is_admin)
                        {
                            // get_player_pid_to_guid_response();
                            // Sleep(500);
                            // get_muted_guid_keys_response();
                            // Sleep(500);
                            game_server &rcon_gs = game_servers[0];
                            main_app.get_connection_manager().send_and_receive_rcon_data(
                                "status", rcon_reply, rcon_gs.get_server_ip_address().c_str(),
                                rcon_gs.get_server_port(), rcon_gs.get_rcon_password().c_str(), rcon_gs, true, true);

                        } /*else {
                          auto [status, game_name] =
                        check_if_specified_server_ip_port_and_rcon_password_are_valid(rcon_gs.get_server_ip_address().c_str(),
                        rcon_gs.get_server_port(), rcon_gs.get_rcon_password().c_str());
                          rcon_gs.set_is_connection_settings_valid(status);
                        }*/

                        game_server &gs = game_servers[game_server_index];
                        if (!is_rcon_game_server(gs))
                        {

                            main_app.get_connection_manager().send_and_receive_non_rcon_data(
                                "getstatus", rcon_reply, gs.get_server_ip_address().c_str(), gs.get_server_port(), gs,
                                true, true);
                        }
                    }

                    is_refresh_players_data_event.store(false);
                }
            }
            catch (std::exception &ex)
            {
                const string error_message{format("^3A specific exception was caught in command queue's "
                                                  "thread!\n^1Exception: {}",
                                                  ex.what())};
                print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
            }
            catch (...)
            {
                char buffer[512];
                strerror_s(buffer, GetLastError());
                const string error_message{format("^3A generic error was caught in command queue's "
                                                  "thread!\n^1Exception: {}",
                                                  buffer)};
                print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
            }
        }
    }};

    task_thread.detach();

    std::thread messaging_thread{[&]() {
        IsGUIThread(TRUE);
        const auto &tiny_rcon_server_ip = main_app.get_tiny_rcon_server_ip_address();
        const auto tiny_rcon_server_port = static_cast<uint_least16_t>(main_app.get_tiny_rcon_server_port());

        while (true)
        {

            try
            {

                while (!is_terminate_program.load() && !main_app.is_message_queue_empty())
                {
                    message_t message{main_app.get_message_from_queue()};
                    const bool is_call_message_handler{message.command != "inform-message" &&
                                                       message.command != "public-message"};
                    main_app.get_connection_manager_for_messages().process_and_send_message(
                        message.command, message.data, message.is_show_in_messages, tiny_rcon_server_ip,
                        tiny_rcon_server_port, is_call_message_handler);
                }

                if (!is_terminate_program.load())
                {
                    main_app.get_connection_manager_for_messages().wait_for_and_process_response_message(
                        tiny_rcon_server_ip, tiny_rcon_server_port);
                }
            }
            catch (std::exception &ex)
            {
                const string error_message{format("^3A specific exception was caught in message queue's "
                                                  "thread!\n^1Exception: {}",
                                                  ex.what())};
                print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
            }
            catch (...)
            {
                char buffer[512];
                strerror_s(buffer, GetLastError());
                const string error_message{format("^3A generic error was caught in message queue's "
                                                  "thread!\n^1Exception: {}",
                                                  buffer)};
                print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
            }

            Sleep(20);
        }
    }};

    messaging_thread.detach();

    std::thread remote_messaging_thread{[&]() {
        IsGUIThread(TRUE);
        const auto &private_tiny_rcon_server_ip = main_app.get_private_tiny_rcon_server_ip_address();
        const auto private_tiny_rcon_server_port = main_app.get_private_tiny_rcon_server_port();
        while (true)
        {

            try
            {

                while (!is_terminate_program.load() && !main_app.is_remote_message_queue_empty())
                {
                    message_t message{main_app.get_remote_message_from_queue()};
                    const bool is_call_message_handler{message.command != "inform-message" &&
                                                       message.command != "public-message"};
                    main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
                        message.command, message.data, message.is_show_in_messages, private_tiny_rcon_server_ip,
                        private_tiny_rcon_server_port, is_call_message_handler);
                }

                if (!is_terminate_program.load())
                {
                    main_app.get_connection_manager_for_rcon_messages().wait_for_and_process_response_message(
                        private_tiny_rcon_server_ip, private_tiny_rcon_server_port);
                }
            }
            catch (std::exception &ex)
            {
                const string error_message{format("^3A specific exception was caught in "
                                                  "remote_messaging_thread!\n^1Exception: {}",
                                                  ex.what())};
                print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
            }
            catch (...)
            {
                char buffer[512];
                strerror_s(buffer, GetLastError());
                const string error_message{format("^3A generic error was caught in the "
                                                  "remote_messaging_thread!\n^1Exception: {}",
                                                  buffer)};
                print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
            }
            Sleep(20);
        }
    }};

    remote_messaging_thread.detach();

    HHOOK hHook{SetWindowsHookEx(WH_KEYBOARD_LL, monitor_game_key_press_events, GetModuleHandle(nullptr), 0)};

    SetTimer(app_handles.hwnd_main_window, ID_TIMER, 1000, nullptr);
    MSG win32_msg{};

    try
    {

        while (GetMessage(&win32_msg, nullptr, 0, 0) > 0)
        {
            if (TranslateAccelerator(app_handles.hwnd_main_window, hAccel, &win32_msg) != 0)
            {
                TranslateMessage(&win32_msg);
                DispatchMessage(&win32_msg);
            }
            else if (IsDialogMessage(app_handles.hwnd_main_window, &win32_msg) == 0)
            {
                TranslateMessage(&win32_msg);
                if (win32_msg.message == WM_KEYDOWN)
                {
                    process_key_down_message(win32_msg);
                }
                else if (app_handles.hwnd_e_user_input == win32_msg.hwnd && WM_LBUTTONDOWN == win32_msg.message)
                {
                    const int x{GET_X_LPARAM(win32_msg.lParam)};
                    const int y{GET_Y_LPARAM(win32_msg.lParam)};
                    RECT rect{};
                    GetClientRect(app_handles.hwnd_e_user_input, &rect);
                    if (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom)
                    {
                        SetFocus(app_handles.hwnd_e_user_input);
                        if (is_first_left_mouse_button_click_in_prompt_edit_control)
                        {
                            SetWindowTextA(app_handles.hwnd_e_user_input, "");
                            is_first_left_mouse_button_click_in_prompt_edit_control = false;
                        }
                    }
                }

                DispatchMessage(&win32_msg);
            }
            else if (win32_msg.message == WM_KEYDOWN)
            {
                process_key_down_message(win32_msg);
            }
            else if (app_handles.hwnd_e_user_input == win32_msg.hwnd && WM_LBUTTONDOWN == win32_msg.message)
            {
                const int x{GET_X_LPARAM(win32_msg.lParam)};
                const int y{GET_Y_LPARAM(win32_msg.lParam)};
                RECT rect{};
                GetClientRect(app_handles.hwnd_e_user_input, &rect);
                if (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom)
                {
                    SetFocus(app_handles.hwnd_e_user_input);
                    if (is_first_left_mouse_button_click_in_prompt_edit_control)
                    {
                        SetWindowTextA(app_handles.hwnd_e_user_input, "");
                        is_first_left_mouse_button_click_in_prompt_edit_control = false;
                    }
                }
            }

            if (is_main_window_constructed && !is_tinyrcon_initialized)
            {

                ifstream temp_messages_file{"log\\temporary_message.log"};
                if (temp_messages_file)
                {
                    for (string line; getline(temp_messages_file, line);)
                    {
                        print_colored_text(app_handles.hwnd_re_messages_data, line.c_str(),
                                           is_append_message_to_richedit_control::yes, is_log_message::yes,
                                           is_log_datetime::yes);
                    }
                    temp_messages_file.close();
                }

                STARTUPINFO si{};
                PROCESS_INFORMATION pi{};
                char command_to_run[256]{"cmd.exe /C ping -n 1 -4 cod2master.activision.com > "
                                         "resolved_hostname.log"};
                // str_copy(command_to_run, std::size(command_to_run), L"cmd.exe /C ping
                // -n 1 -4 cod2master.activision.com > resolved_hostname.log");
                CreateProcessA(nullptr, command_to_run, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
                               &si, &pi);
                if (si.hStdError != NULL)
                {
                    CloseHandle(si.hStdError);
                }
                if (si.hStdInput != NULL)
                {
                    CloseHandle(si.hStdInput);
                }
                if (si.hStdOutput != NULL)
                {
                    CloseHandle(si.hStdOutput);
                }
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);

                const std::regex player_ip_address_regex{R"(\[(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})\])"};
                smatch matches;
                ifstream input_file{L"resolved_hostname.log"};
                for (string line; getline(input_file, line);)
                {
                    if (regex_search(line, matches, player_ip_address_regex))
                    {
                        main_app.set_cod2_master_server_ip_address(matches[1].str());
                        break;
                    }
                }

                input_file.close();

                std::thread th{[&]() {
                    IsGUIThread(TRUE);
                    try
                    {

                        refresh_game_servers_data(app_handles.hwnd_servers_grid);
                    }
                    catch (std::exception &ex)
                    {
                        const string error_message{format("^3A specific exception was caught in "
                                                          "refresh_game_servers_data(HWND) function's thread of "
                                                          "execution!\n^1Exception: {}",
                                                          ex.what())};
                        print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
                    }
                    catch (...)
                    {
                        char buffer[512];
                        strerror_s(buffer, GetLastError());
                        const string error_message{
                            format("^3A generic error was caught in refresh_game_servers_data(HWND) "
                                   "function's thread of execution\n^1Exception: {}",
                                   buffer)};
                        print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
                    }
                }};

                th.detach();

                is_tinyrcon_initialized = true;

                // const string gif_image_name{ format("gif{}",
                // random_number_distribution(gen)) }; map_image_jpg = new
                // ImageEx("GIF", gif_image_name.c_str());
                // map_image_jpg->InitAnimation(app_handles.hwnd_main_window, Point{
                // screen_width / 2 + 340, screen_height - 180 });

                const auto ts{get_current_time_stamp()};
                const string user_details{format("{}\\{}\\{}", me->user_name, me->ip_address, ts)};

                if (me->is_admin)
                {

                    main_app.add_message_to_queue(
                        message_t("request-login",
                                  format(R"({}\{}\{}\{}\{})", me->user_name, me->ip_address, get_current_time_stamp(),
                                         main_app.get_player_name(), main_app.get_game_version_number()),
                                  true));

                    if (main_app.get_is_ftp_server_online())
                    {
                        unsigned long guid_number{};

                        const string compressed_bans_file_name{
                            format("bans_{}_{}.7z", get_random_number(),
                                   check_ip_address_validity(main_app.get_user_ip_address(), guid_number)
                                       ? remove_disallowed_characters_in_ip_address(main_app.get_user_ip_address())
                                       : to_string(get_random_number()))};
                        std::fill(operation_completed_flag, operation_completed_flag + 6, 0);
                        main_app.add_message_to_queue(
                            message_t{"upload-bans-compressed", compressed_bans_file_name, true});
                    }

                    main_app.add_message_to_queue(message_t{"request-protectedipaddresses", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-protectedipaddressranges", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-protectedcities", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-protectedcountries", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-admindata", user_details, true});
                }
                else
                {
                    main_app.add_remote_message_to_queue(
                        message_t("request-login-player",
                                  format(R"({}\{}\{}\{}\{})", me->user_name, me->ip_address, ts,
                                         main_app.get_player_name(), main_app.get_game_version_number()),
                                  true));

                    main_app.add_remote_message_to_queue(message_t{"request-admindata-player", user_details, true});

                    main_app.add_remote_message_to_queue(message_t{"request-tempbans-player", user_details, true});
                    main_app.add_remote_message_to_queue(message_t{"request-ipaddressbans-player", user_details, true});
                    main_app.add_remote_message_to_queue(
                        message_t{"request-ipaddressrangebans-player", user_details, true});
                    main_app.add_remote_message_to_queue(message_t{"request-namebans-player", user_details, true});
                    main_app.add_remote_message_to_queue(message_t{"request-citybans-player", user_details, true});
                    main_app.add_remote_message_to_queue(message_t{"request-countrybans-player", user_details, true});

                    main_app.add_remote_message_to_queue(
                        message_t{"request-protectedipaddresses-player", user_details, true});
                    main_app.add_remote_message_to_queue(
                        message_t{"request-protectedipaddressranges-player", user_details, true});
                    main_app.add_remote_message_to_queue(
                        message_t{"request-protectedcities-player", user_details, true});
                    main_app.add_remote_message_to_queue(
                        message_t{"request-protectedcountries-player", user_details, true});
                }

                main_app.add_message_to_queue(message_t("request-mapnames", user_details, true));
                main_app.add_remote_message_to_queue(message_t("request-imagesdata", user_details, true));

                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "query-request",
                    format("is_user_admin?{}\\{}", me->user_name, main_app.get_game_servers()[0].get_rcon_password()),
                    true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);

                PostMessage(app_handles.hwnd_progress_bar, PBM_SETMARQUEE, (WPARAM)FALSE, (LPARAM)0);
                auto progress_bar_style = GetWindowStyle(app_handles.hwnd_progress_bar);
                progress_bar_style = progress_bar_style & ~PBS_MARQUEE;
                progress_bar_style = progress_bar_style | PBS_SMOOTH;
                SetWindowLong(app_handles.hwnd_progress_bar, -16, progress_bar_style);
                SendMessage(app_handles.hwnd_progress_bar, PBM_SETRANGE, 0,
                            MAKELPARAM(0, main_app.get_check_for_banned_players_time_period()));
                SendMessage(app_handles.hwnd_progress_bar, PBM_SETSTEP, 1, 0);
                SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, 0, 0);
            }
        }

        stl::helper::run_at_scope_exit<std::function<void(void)>> tasks_at_exit{[]() { execute_at_exit(); }};
        execute_at_exit();

        is_terminate_program.store(true);

        log_message("Exiting TinyRcon program.", is_log_datetime::yes);

        DestroyAcceleratorTable(hAccel);
        // const string geo_dat_file_path{main_app.get_current_working_directory() + R"(plugins\geoIP\geo.dat)"};
        // export_geoip_data(main_app.get_connection_manager().get_geoip_data(), geo_dat_file_path.c_str());

        if (wcex.hbrBackground != nullptr)
            DeleteObject((HGDIOBJ)wcex.hbrBackground);
        if (red_brush != nullptr)
            DeleteBrush(red_brush);
        if (black_brush != nullptr)
            DeleteBrush(black_brush);
        if (font_for_players_grid_data != nullptr)
            DeleteFont(font_for_players_grid_data);
        if (hImageList)
            ImageList_Destroy(hImageList);
        if (hImageListForChat)
            ImageList_Destroy(hImageListForChat);
        UnregisterClass(wcex.lpszClassName, app_handles.hInstance);
        UnregisterClass(wcex_confirmation_dialog.lpszClassName, app_handles.hInstance);
        UnregisterClass(wcex_configuration_dialog.lpszClassName, app_handles.hInstance);
    }
    catch (std::exception &ex)
    {
        const string error_message{format("^3A specific exception was caught in WinMain's thread of "
                                          "execution!\n^1Exception: {}",
                                          ex.what())};
        print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
        if (hHook)
            UnhookWindowsHookEx(hHook);
    }
    catch (...)
    {
        char buffer[512];
        strerror_s(buffer, GetLastError());
        const string error_message{format("^3A generic error was caught in WinMain's thread of "
                                          "execution\n^1Exception: {}",
                                          buffer)};
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
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXA);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONCLIENT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(color::black);
    wcex.lpszClassName = "tinyrconclient";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    auto status = RegisterClassEx(&wcex);
    if (!status)
    {
        char error_msg[256]{};
        (void)snprintf(error_msg, std::size(error_msg), "Windows class creation failed with error code: %d",
                       GetLastError());
        MessageBox(nullptr, error_msg, "Window Class Failed", MB_ICONERROR);
    }

    wcex_confirmation_dialog = {};
    wcex_confirmation_dialog.cbSize = sizeof(WNDCLASSEXA);
    wcex_confirmation_dialog.style = CS_HREDRAW | CS_VREDRAW;
    wcex_confirmation_dialog.lpfnWndProc = WndProcForConfirmationDialog;
    wcex_confirmation_dialog.hInstance = hInstance;
    wcex_confirmation_dialog.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONCLIENT));
    wcex_confirmation_dialog.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex_confirmation_dialog.hbrBackground = CreateSolidBrush(color::black);
    wcex_confirmation_dialog.lpszClassName = "ConfirmationDialog";
    wcex_confirmation_dialog.hIconSm = LoadIcon(wcex_confirmation_dialog.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    status = RegisterClassEx(&wcex_confirmation_dialog);
    if (!status)
    {
        char error_msg[256]{};
        (void)snprintf(error_msg, std::size(error_msg), "Windows class creation failed with error code: %d",
                       GetLastError());
        MessageBox(nullptr, error_msg, "Window Class Failed", MB_ICONERROR);
    }

    wcex_configuration_dialog = {};

    wcex_configuration_dialog.cbSize = sizeof(WNDCLASSEXA);
    wcex_configuration_dialog.style = CS_HREDRAW | CS_VREDRAW;
    wcex_configuration_dialog.lpfnWndProc = WndProcForConfigurationDialog;
    wcex_configuration_dialog.hInstance = hInstance;
    wcex_configuration_dialog.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TINYRCONCLIENT));
    wcex_configuration_dialog.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex_configuration_dialog.hbrBackground = CreateSolidBrush(color::black);
    wcex_configuration_dialog.lpszClassName = "ConfigurationDialog";
    wcex_configuration_dialog.hIconSm = LoadIcon(wcex_configuration_dialog.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    status = RegisterClassEx(&wcex_configuration_dialog);
    if (!status)
    {
        char error_msg[256]{};
        (void)snprintf(error_msg, std::size(error_msg), "Windows class creation failed with error code: %d",
                       GetLastError());
        MessageBoxA(nullptr, error_msg, "Window Class Failed", MB_ICONERROR);
    }

    return status;
}

bool initialize_main_app(HINSTANCE hInstance, const int)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    app_handles.hInstance = hInstance;

    InitSimpleGrid(app_handles.hInstance);

    INITCOMMONCONTROLSEX icex{};

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    font_for_players_grid_data =
        CreateFontA(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH | FF_DECORATIVE, "Lucida Console");
    // RECT desktop_work_area{};
    // SystemParametersInfoA(SPI_GETWORKAREA, 0, &desktop_work_area, 0);
    // AdjustWindowRectEx(&desktop_work_area, WS_OVERLAPPEDWINDOW /*| WS_HSCROLL |
    // WS_VSCROLL*/, FALSE, 0);

    app_handles.hwnd_main_window = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW, wcex.lpszClassName, "TinyRcon client",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN /*| WS_HSCROLL | WS_VSCROLL*/, 0, 0, client_rect.right - client_rect.left,
        client_rect.bottom - client_rect.top, nullptr, nullptr, hInstance, nullptr);

    // app_handles.hwnd_main_window = CreateWindowEx(0, wcex.lpszClassName,
    // "TinyRcon client", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME
    // /*WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL*/, 0, 0, client_rect.right
    // - client_rect.left, client_rect.bottom - client_rect.top, nullptr, nullptr,
    // hInstance, nullptr);

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
    // // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char msg_buffer[512];
    static char msg_buffer2[512];
    static char message_buffer[512];
    static char info_player_command[128]{};
    static char spectate_player_command[128]{};
    static char report_player_command[128]{};
    static char mute_player_command[128]{};
    static char unmute_player_command[128]{};
    static char warn_player_command[128]{};
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
    /*static const int counter_max =
            std::max<size_t>({ 60u,
       main_app.get_time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat(),
                                              main_app.get_time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon(),
                                              main_app.get_time_period_in_minutes_for_saving_players_stats_data()
       });*/
    HDC hdcMem;
    HBITMAP hbmMem;
    HANDLE hOld;
    HDC hdc;
    BITMAP bitmap;
    HGDIOBJ oldBitmap;
    static HPEN red_pen{};
    static HPEN light_blue_pen{};
    static const size_t rcon_status_time_period{main_app.get_check_for_banned_players_time_period()};

    switch (message)
    {

    case WM_CONTEXTMENU: {
        if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_re_messages_data)
        {
            SetFocus(app_handles.hwnd_re_messages_data);
            hPopupMenu = CreatePopupMenu();
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWTEMPBANSBUTTON, "View temporary bans");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPBANSBUTTON,
                       "View banned IP addresses");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPRANGEBANSBUTTON,
                       "View banned IP address ranges");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDPLAYERNAMES,
                       "View banned player names");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCITIES, "View banned cities");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCOUNTRIES,
                       "View banned countries");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWREPORTEDPLAYERS,
                       "View reported players");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWMUTEDPLAYERS, "View muted players");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSES,
                       "View protected IP addresses");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSRANGES,
                       "View protected IP address ranges");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCITIES,
                       "View protected cities");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCOUNTRIES,
                       "View protected countries");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWADMINSDATA, "View &admins' data");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, IDC_COPY, "&Copy (Ctrl + C)");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "C&lear messages");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
            TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0,
                           hWnd, nullptr);
        }
        else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_players_grid)
        {
            SetFocus(app_handles.hwnd_players_grid);
            hPopupMenu = CreatePopupMenu();
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, 0))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    if (main_app.get_user_ip_address() != get_player_ip_address_for_pid(pid))
                    {
                        if (!is_player_being_spectated.load())
                        {
                            (void)snprintf(spectate_player_command, std::size(spectate_player_command),
                                           "Spectate %s (PID: %d)?", selected_player.player_name, pid);
                        }
                        else
                        {
                            (void)snprintf(spectate_player_command, std::size(spectate_player_command),
                                           "Stop spectating player?");
                        }
                        remove_all_color_codes(spectate_player_command);
                        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SPECTATEPLAYER,
                                   spectate_player_command);
                        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
                    }
                    (void)snprintf(report_player_command, std::size(report_player_command),
                                   "Report %s with reason (PID: %d)?", selected_player.player_name, pid);
                    remove_all_color_codes(report_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_REPORTPLAYER, report_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
                }
            }
            if (me->is_admin && check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, 0))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    (void)snprintf(info_player_command, std::size(info_player_command),
                                   "Display information about %s (PID: %d)?", selected_player.player_name, pid);
                    remove_all_color_codes(info_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_PRINTPLAYERINFORMATION_ACTION,
                               info_player_command);
                    (void)snprintf(warn_player_command, std::size(warn_player_command), "Warn %s (PID: %d)?",
                                   selected_player.player_name, pid);
                    remove_all_color_codes(warn_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_WARNBUTTON, warn_player_command);
                    (void)snprintf(mute_player_command, std::size(mute_player_command),
                                   "Mute %s (PID: %d | IP address: %s)?", selected_player.player_name, pid,
                                   selected_player.ip_address.c_str());
                    remove_all_color_codes(mute_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_MUTE_PLAYER, mute_player_command);
                    (void)snprintf(unmute_player_command, std::size(unmute_player_command),
                                   "Unmute %s (PID: %d | IP address: %s)?", selected_player.player_name, pid,
                                   selected_player.ip_address.c_str());
                    remove_all_color_codes(unmute_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_UNMUTE_PLAYER,
                               unmute_player_command);
                    (void)snprintf(kick_player_command, std::size(kick_player_command), "Kick %s (PID: %d)?",
                                   selected_player.player_name, pid);
                    remove_all_color_codes(kick_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_KICKBUTTON, kick_player_command);
                    (void)snprintf(tempban_player_command, std::size(tempban_player_command),
                                   "Temporarily ban %s (PID: %d)?", selected_player.player_name, pid);
                    remove_all_color_codes(tempban_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_TEMPBANBUTTON,
                               tempban_player_command);
                    if (strcmp(selected_player.guid_key, "0") != 0)
                    {
                        (void)snprintf(guidban_player_command, std::size(guidban_player_command),
                                       "Ban %s's GUID key: %s (PID: %d)?", selected_player.player_name,
                                       selected_player.guid_key, pid);
                        remove_all_color_codes(guidban_player_command);
                        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_GUIDBANBUTTON,
                                   guidban_player_command);
                    }
                    (void)snprintf(ipban_player_command, std::size(ipban_player_command),
                                   "Ban %s's IP address: %s (PID: %d)?", selected_player.player_name,
                                   selected_player.ip_address.c_str(), pid);
                    remove_all_color_codes(ipban_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_IPBANBUTTON, ipban_player_command);
                    const string ip_address_range{
                        get_narrow_ip_address_range_for_specified_ip_address(selected_player.ip_address)};
                    (void)snprintf(iprangeban_player_command, std::size(iprangeban_player_command),
                                   "Ban %s's IP address range: %s (PID: %d)?", selected_player.player_name,
                                   ip_address_range.c_str(), pid);
                    remove_all_color_codes(iprangeban_player_command);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_IPRANGEBANBUTTON,
                               iprangeban_player_command);
                    if (/*main_app.get_is_automatic_city_kick_enabled() && */ !main_app.get_current_game_server()
                            .get_banned_cities_set()
                            .contains(selected_player.city))
                    {
                        (void)snprintf(city_ban_player_command, std::size(city_ban_player_command),
                                       "Ban %s's city %s (PID: %d)?", selected_player.player_name, selected_player.city,
                                       pid);
                        remove_all_color_codes(city_ban_player_command);
                        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CITYBANBUTTON,
                                   city_ban_player_command);
                    }
                    if (/*main_app.get_is_automatic_country_kick_enabled() && */ !main_app.get_current_game_server()
                            .get_banned_countries_set()
                            .contains(selected_player.country_name))
                    {
                        (void)snprintf(country_ban_player_command, std::size(country_ban_player_command),
                                       "Ban %s's country: %s (PID: %d)?", selected_player.player_name,
                                       selected_player.country_name, pid);
                        remove_all_color_codes(country_ban_player_command);
                        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_COUNTRYBANBUTTON,
                                   country_ban_player_command);
                    }

                    string player_name{selected_player.player_name};
                    remove_all_color_codes(player_name);
                    trim_in_place(player_name);
                    to_lower_case_in_place(player_name);
                    const bool is_player_name_valid{
                        !player_name.empty() &&
                        !main_app.get_current_game_server().get_banned_names_map().contains(player_name)};
                    if (is_player_name_valid)
                    {
                        (void)snprintf(name_ban_command, std::size(name_ban_command), "Ban player name: %s (PID: %d)?",
                                       player_name.c_str(), pid);
                        remove_all_color_codes(name_ban_command);
                        InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_BANNAMEBUTTON, name_ban_command);
                    }

                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
                }
            }

            if (main_app.get_current_game_server().get_is_connection_settings_valid())
            {
                if (!main_app.get_is_automatic_city_kick_enabled())
                {
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_ENABLECITYBANBUTTON,
                               "Enable city ban feature?");
                }
                else
                {
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_DISABLECITYBANBUTTON,
                               "Disable city ban feature?");
                }

                if (!main_app.get_is_automatic_country_kick_enabled())
                {
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_ENABLECOUNTRYBANBUTTON,
                               "Enable country ban feature?");
                }
                else
                {
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_DISABLECOUNTRYBANBUTTON,
                               "Disable country ban feature?");
                }

                InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            }

            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWTEMPBANSBUTTON, "View temporary bans");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPBANSBUTTON,
                       "View banned IP addresses");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWIPRANGEBANSBUTTON,
                       "View banned IP address ranges");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDPLAYERNAMES,
                       "View banned player names");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCITIES, "View banned cities");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWBANNEDCOUNTRIES,
                       "View banned countries");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWREPORTEDPLAYERS,
                       "View reported players");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWMUTEDPLAYERS, "View muted players");
            // InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            // InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSES,
            //            "View protected IP addresses");
            // InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDIPADDRESSRANGES,
            //            "View protected IP address ranges");
            // InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCITIES,
            //            "View protected cities");
            // InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWPROTECTEDCOUNTRIES,
            //            "View protected countries");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_VIEWADMINSDATA, "View &admins' data");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_REFRESH_PLAYERS_DATA_BUTTON,
                       "Refresh players' data");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            // if (main_app.get_current_game_server().get_number_of_players() > 0)
            // {
            // 	InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING,
            // ID_SORT_PLAYERS_DATA_BY_PID, "Sort players' data by 'PID'");
            // 	InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING,
            // ID_SORT_PLAYERS_DATA_BY_SCORE, "Sort players' data by 'SCORE'");
            // 	InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING,
            // ID_SORT_PLAYERS_DATA_BY_PING, "Sort players' data by 'PING'");
            // 	InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING,
            // ID_SORT_PLAYERS_DATA_BY_NAME, "Sort players' data by 'PLAYER NAME'");
            // 	if (me->is_admin)
            // 	{
            // 		InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION |
            // MF_STRING, ID_SORT_PLAYERS_DATA_BY_IP, "Sort players' data by 'IP
            // ADDRESS'"); 		InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION |
            // MF_STRING, ID_SORT_PLAYERS_DATA_BY_GEO, "Sort players' data by
            // 'GEOINFORMATION'");
            // 	}
            // }
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_MAP_RESTART,
                       "Restart current map (map_restart)?");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_FAST_RESTART,
                       "Restart current match (fast_restart)?");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CONNECTBUTTON, "Join the game server");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CONNECTPRIVATESLOTBUTTON,
                       "Join the game server using a private slot");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "C&lear messages");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
            TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0,
                           hWnd, nullptr);
        }
        else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_servers_grid)
        {
            SetFocus(app_handles.hwnd_servers_grid);
            hPopupMenu = CreatePopupMenu();
            if (check_if_selected_cell_indices_are_valid_for_game_servers_grid(selected_server_row, 2))
            {
                string selected_server_name{GetCellContents(app_handles.hwnd_servers_grid, selected_server_row, 1)};
                if (selected_server_name.length() >= 2 && '^' == selected_server_name[0] &&
                    is_decimal_digit(selected_server_name[1]))
                    selected_server_name.erase(0, 2);
                string selected_server_address{GetCellContents(app_handles.hwnd_servers_grid, selected_server_row, 2)};
                if (selected_server_address.length() >= 2 && '^' == selected_server_address[0] &&
                    is_decimal_digit(selected_server_address[1]))
                    selected_server_address.erase(0, 2);
                snprintf(msg_buffer2, std::size(msg_buffer2), "Connect to %s %s game server?",
                         selected_server_address.c_str(), selected_server_name.c_str());
                InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CONNECTBUTTON, msg_buffer2);
            }
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_REFRESHSERVERSBUTTON, "Refresh servers");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_SHOWPLAYERSBUTTON, "Show players");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_RCONVIEWBUTTON, "Show rcon server");
            if (check_if_selected_cell_indices_are_valid_for_game_servers_grid(selected_server_row, 2))
            {
                string selected_server_address{GetCellContents(app_handles.hwnd_servers_grid, selected_server_row, 2)};
                if (selected_server_address.length() >= 2 && '^' == selected_server_address[0] &&
                    is_decimal_digit(selected_server_address[1]))
                    selected_server_address.erase(0, 2);
                auto parts = stl::helper::str_split(selected_server_address, ":", nullptr, split_on_whole_needle_t::yes,
                                                    ignore_empty_string_t::yes);
                for (auto &&part : parts)
                    stl::helper::trim_in_place(part);
                unsigned long guid{};
                int port_number{};
                if (parts.size() == 2 && check_ip_address_validity(parts[0], guid) &&
                    is_valid_decimal_whole_number(parts[1], port_number))
                {
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
                    InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_COPYGAMESERVERADDRESS,
                               "Copy game server address");
                }
            }
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_CLEARMESSAGESCREENBUTTON, "C&lear messages");
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_SEPARATOR, NULL, nullptr);
            InsertMenu(hPopupMenu, (UINT)-1, MF_BYPOSITION | MF_STRING, ID_QUITBUTTON, "E&xit");
            TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0,
                           hWnd, nullptr);
        }
        else if (reinterpret_cast<HWND>(wParam) == app_handles.hwnd_e_user_input)
        {
            SetFocus(app_handles.hwnd_e_user_input);
            hPopupMenu = CreatePopupMenu();
            InsertMenu(hPopupMenu, 0, MF_BYCOMMAND | MF_STRING, IDC_PASTE, "Paste (Ctrl + V)");
            TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0,
                           hWnd, nullptr);
        }
    }
    break;

    case WM_TIMER: {

        main_app.update_download_and_upload_speed_statistics();

        ++atomic_counter;
        if (atomic_counter.load() > rcon_status_time_period)
            atomic_counter.store(0);

        SendMessage(app_handles.hwnd_progress_bar, PBM_SETPOS, (WPARAM)atomic_counter.load(), 0);
        ++counter;

        if (counter % 5 == 0)
        {
            if (!me->is_admin)
            {
                main_app.get_connection_manager_for_rcon_messages().process_and_send_message(
                    "rcon-heartbeat-player",
                    format("{}\\{}\\{}", main_app.get_username(), get_current_time_stamp(), me->ip_address), true,
                    main_app.get_private_tiny_rcon_server_ip_address(), main_app.get_private_tiny_rcon_server_port(),
                    false);
            }
            else
            {
                const auto current_ts{get_current_time_stamp()};
                main_app.get_connection_manager_for_messages().process_and_send_message(
                    "heartbeat", format("{}\\{}", main_app.get_username(), current_ts), true,
                    main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            }
        }

        if (counter % 15 == 0)
        {

            if (!main_app.get_is_bans_synchronized() && me->is_admin)
            {

                call_once(synchronize_bans_flag, [&]() {
                    unsigned long guid_number{};
                    std::fill(operation_completed_flag, operation_completed_flag + 6, 0);

                    const string data_for_tempbans{
                        format("tempbans{}_{}.txt", get_random_number(),
                               check_ip_address_validity(me->ip_address, guid_number)
                                   ? remove_disallowed_characters_in_ip_address(me->ip_address)
                                   : to_string(get_random_number()))};
                    main_app.add_message_to_queue(message_t{"upload-bans", data_for_tempbans, true});

                    const string data_for_bans{format("bans{}_{}.txt", get_random_number(),
                                                      check_ip_address_validity(me->ip_address, guid_number)
                                                          ? remove_disallowed_characters_in_ip_address(me->ip_address)
                                                          : to_string(get_random_number()))};
                    main_app.add_message_to_queue(message_t{"upload-bans", data_for_bans, true});

                    const string data_for_ip_range_bans{
                        format("ip_range_bans{}_{}.txt", get_random_number(),
                               check_ip_address_validity(me->ip_address, guid_number)
                                   ? remove_disallowed_characters_in_ip_address(me->ip_address)
                                   : to_string(get_random_number()))};
                    main_app.add_message_to_queue(message_t{"upload-bans", data_for_ip_range_bans, true});

                    const string data_for_banned_cities{
                        format("banned_cities{}_{}.txt", get_random_number(),
                               check_ip_address_validity(me->ip_address, guid_number)
                                   ? remove_disallowed_characters_in_ip_address(me->ip_address)
                                   : to_string(get_random_number()))};
                    main_app.add_message_to_queue(message_t{"upload-bans", data_for_banned_cities, true});

                    const string data_for_banned_countries{
                        format("banned_countries{}_{}.txt", get_random_number(),
                               check_ip_address_validity(me->ip_address, guid_number)
                                   ? remove_disallowed_characters_in_ip_address(me->ip_address)
                                   : to_string(get_random_number()))};
                    main_app.add_message_to_queue(message_t{"upload-bans", data_for_banned_countries, true});

                    const string data_for_banned_names{
                        format("banned_names{}_{}.txt", get_random_number(),
                               check_ip_address_validity(me->ip_address, guid_number)
                                   ? remove_disallowed_characters_in_ip_address(me->ip_address)
                                   : to_string(get_random_number()))};
                    main_app.add_message_to_queue(message_t{"upload-bans", data_for_banned_names, true});
                });
            }

            if (!me->is_admin)
            {
                main_app.set_player_name(find_users_player_name_for_installed_cod2_game(
                    me, !main_app.get_current_game_server().get_game_mod_name().empty()
                            ? main_app.get_current_game_server().get_game_mod_name()
                            : "main"s));
                me->player_name = main_app.get_player_name();
            }
        }

        if (counter % 30 == 0)
        {
            counter = 0;

            const auto current_ts{get_current_time_stamp()};
            if (me->is_admin)
            {
                main_app.add_message_to_queue(message_t{
                    "request-admindata", format("{}\\{}\\{}", me->user_name, me->ip_address, current_ts), true});
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());

                if (!main_app.get_is_bans_synchronized())
                {
                    std::fill(operation_completed_flag, operation_completed_flag + 6, 0);
                    const string user_details{
                        format("{}\\{}\\{}", me->user_name, me->ip_address, get_current_time_stamp())};
                    main_app.add_message_to_queue(message_t{"request-tempbans", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-ipaddressbans", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-ipaddressrangebans", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-namebans", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-citybans", user_details, true});
                    main_app.add_message_to_queue(message_t{"request-countrybans", user_details, true});
                }
            }
            else
            {
                main_app.add_remote_message_to_queue(message_t{
                    "request-admindata-player", format("{}\\{}\\{}", me->user_name, me->ip_address, current_ts), true});
                save_current_user_data_to_json_file(main_app.get_user_data_file_path());
                if (!main_app.get_is_bans_synchronized())
                {
                    const string user_details{
                        format("{}\\{}\\{}", me->user_name, me->ip_address, get_current_time_stamp())};
                    main_app.add_remote_message_to_queue(message_t{"request-tempbans-player", user_details, true});
                    main_app.add_remote_message_to_queue(message_t{"request-ipaddressbans-player", user_details, true});
                    main_app.add_remote_message_to_queue(
                        message_t{"request-ipaddressrangebans-player", user_details, true});
                    main_app.add_remote_message_to_queue(message_t{"request-namebans-player", user_details, true});
                    main_app.add_remote_message_to_queue(message_t{"request-citybans-player", user_details, true});
                    main_app.add_remote_message_to_queue(message_t{"request-countrybans-player", user_details, true});
                }
            }

            const auto &rcon_game_server = main_app.get_game_servers()[0];
            main_app.get_connection_manager_for_messages().process_and_send_message(
                "query-request", format("is_user_admin?{}\\{}", me->user_name, rcon_game_server.get_rcon_password()),
                true, main_app.get_tiny_rcon_server_ip_address(), main_app.get_tiny_rcon_server_port(), false);
            const auto status = check_if_specified_server_ip_port_and_rcon_password_are_valid(
                rcon_game_server.get_server_ip_address().c_str(), rcon_game_server.get_server_port(),
                rcon_game_server.get_rcon_password().c_str());
            me->is_admin = status.first;
        }

        /*if (main_app.get_is_enable_players_stats_feature() && (counter %
        main_app.get_time_period_in_minutes_for_saving_players_stats_data() == 0))
        {
                save_players_stats_data("data\\player_stats.dat",
        main_app.get_stats_data().get_scores_vector(),
        main_app.get_stats_data().get_scores_map());

                const string file_name_path_for_stats_data_for_year{
        format("data\\player_stats_for_year-{}.dat",
        main_app.get_stats_data().get_stop_time_in_seconds_for_year()) };
                save_players_stats_data(file_name_path_for_stats_data_for_year.c_str(),
        main_app.get_stats_data().get_scores_for_year_vector(),
        main_app.get_stats_data().get_scores_for_year_map());

                const string file_name_path_for_stats_data_for_month{
        format("data\\player_stats_for_month-{}.dat",
        main_app.get_stats_data().get_stop_time_in_seconds_for_month()) };
                save_players_stats_data(file_name_path_for_stats_data_for_month.c_str(),
        main_app.get_stats_data().get_scores_for_month_vector(),
        main_app.get_stats_data().get_scores_for_month_map());

                const string file_name_path_for_stats_data_for_day{
        format("data\\player_stats_for_day-{}.dat",
        main_app.get_stats_data().get_stop_time_in_seconds_for_day()) };
                save_players_stats_data(file_name_path_for_stats_data_for_day.c_str(),
        main_app.get_stats_data().get_scores_for_day_vector(),
        main_app.get_stats_data().get_scores_for_day_map());
        }

        if (main_app.get_is_enable_players_stats_feature() && (counter %
        main_app.get_time_period_in_minutes_for_displaying_top_players_stats_data_in_tinyrcon()
        == 0))
        {
                std::thread task{ []()
                                                 {
                                                         string public_message;
                                                         const string
        title{format("Top {} players:",
        main_app.get_number_of_top_players_to_display_in_tinyrcon())}; const
        string
        top_players_stats{get_top_players_stats_data(main_app.get_stats_data().get_scores_vector(),
        main_app.get_stats_data().get_scores_map(),
        main_app.get_number_of_top_players_to_display_in_tinyrcon(),
        public_message, title.c_str())};
                                                         print_colored_text(app_handles.hwnd_re_messages_data,
        top_players_stats.c_str(), is_append_message_to_richedit_control::yes,
        is_log_message::no, is_log_datetime::yes, true, true); } }; task.detach();
        }

        if (main_app.get_is_enable_players_stats_feature() && (counter %
        main_app.get_time_period_in_minutes_for_displaying_top_players_stats_data_in_game_chat()
        == 0))
        {
                string title{ format("^5Top {} players:",
        main_app.get_number_of_top_players_to_display_in_game_chat()) };
                std::thread task{ rcon_say_top_players, std::move(title) };
                task.detach();
        }*/

        /*if (counter % counter_max == 0)
        {
                counter = 0;
        }*/

        if (rcon_status_time_period == atomic_counter.load())
        {

            if (is_first_left_mouse_button_click_in_prompt_edit_control)
            {
                SetWindowTextA(app_handles.hwnd_e_user_input, "");
                is_first_left_mouse_button_click_in_prompt_edit_control = false;
            }

            is_refresh_players_data_event.store(true);
        }

        if (is_refreshed_players_data_ready_event.load())
        {
            is_refreshed_players_data_ready_event.store(false);
            load_current_map_image(main_app.get_current_game_server().get_current_map());
            if (is_display_players_data.load())
            {
                display_players_data_in_players_grid(app_handles.hwnd_players_grid);
            }
        }

        if (is_display_muted_players_data_event.load())
        {
            std::thread display_large_data_set{display_muted_players_information, number_of_entries_to_display.load(),
                                               false};
            display_large_data_set.detach();
            is_display_muted_players_data_event.store(false);
        }
        else if (is_display_temporarily_banned_players_data_event.load())
        {
            std::thread display_large_data_set{display_temporarily_banned_ip_addresses,
                                               number_of_entries_to_display.load(), false};
            display_large_data_set.detach();
            is_display_temporarily_banned_players_data_event.store(false);
        }
        else if (is_display_permanently_banned_players_data_event.load())
        {
            std::thread display_large_data_set{display_permanently_banned_ip_addresses,
                                               number_of_entries_to_display.load(), false};
            display_large_data_set.detach();
            is_display_permanently_banned_players_data_event.store(false);
        }
        else if (is_display_banned_ip_address_ranges_data_event.load())
        {
            std::thread display_large_data_set{display_banned_ip_address_ranges, number_of_entries_to_display.load(),
                                               false};
            display_large_data_set.detach();
            is_display_banned_ip_address_ranges_data_event.store(false);
        }
        else if (is_display_banned_player_names_data_event.load())
        {
            std::thread display_large_data_set{display_banned_player_names, "^1Banned player names",
                                               number_of_entries_to_display.load(), false};
            display_large_data_set.detach();
            is_display_banned_player_names_data_event.store(false);
        }
        else if (is_display_banned_cities_data_event.load())
        {
            std::thread display_large_data_set{
                []() { display_banned_cities(main_app.get_current_game_server().get_banned_cities_set()); }};
            display_large_data_set.detach();
            is_display_banned_cities_data_event.store(false);
        }
        else if (is_display_banned_countries_data_event.load())
        {
            std::thread display_large_data_set{
                []() { display_banned_countries(main_app.get_current_game_server().get_banned_countries_set()); }};
            display_large_data_set.detach();
            is_display_banned_countries_data_event.store(false);
        }
        else if (is_display_admins_data_event.load())
        {
            std::thread display_large_data_set{
                [&]() { display_admins_data(main_app.get_users(), "^5Tiny^6Rcon ^1Administrators"); }};
            display_large_data_set.detach();
            is_display_admins_data_event.store(false);
        }
        else if (is_display_protected_ip_addresses_data_event.load())
        {
            std::thread display_large_data_set{[]() {
                unordered_map<string, string> protected_ip_to_player_name;
                const auto &players_data = main_app.get_current_game_server().get_players_data();
                for (const auto &protected_ip_address : main_app.get_current_game_server().get_protected_ip_addresses())
                {
                    ostringstream oss;
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        if (protected_ip_address == players_data[i].ip_address)
                        {
                            oss << "^7" << players_data[i].player_name << " ^7| ";
                        }
                    }
                    string online_players_names{oss.str()};
                    if (online_players_names.length() >= 5)
                        online_players_names.erase(online_players_names.length() - 5, 5);
                    protected_ip_to_player_name.emplace(protected_ip_address, std::move(online_players_names));
                }
                display_protected_entries(
                    "Protected IP addresses:", main_app.get_current_game_server().get_protected_ip_addresses(),
                    protected_ip_to_player_name);
            }};
            display_large_data_set.detach();
            is_display_protected_ip_addresses_data_event.store(false);
        }
        else if (is_display_protected_ip_address_ranges_data_event.load())
        {
            std::thread display_large_data_set{[]() {
                unordered_map<string, string> protected_ip_address_range_to_player_name;
                const auto &players_data = main_app.get_current_game_server().get_players_data();
                for (const auto &protected_ip_address_range :
                     main_app.get_current_game_server().get_protected_ip_address_ranges())
                {
                    ostringstream oss;
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        const string player_narrow_ip_address_range{
                            get_narrow_ip_address_range_for_specified_ip_address(players_data[i].ip_address)};
                        const string player_wide_ip_address_range{
                            get_wide_ip_address_range_for_specified_ip_address(players_data[i].ip_address)};
                        if (protected_ip_address_range == player_narrow_ip_address_range ||
                            protected_ip_address_range == player_wide_ip_address_range)
                        {
                            oss << "^7" << players_data[i].player_name << " ^7| ";
                        }
                    }
                    string online_players_names{oss.str()};
                    if (online_players_names.length() >= 5)
                        online_players_names.erase(online_players_names.length() - 5, 5);
                    protected_ip_address_range_to_player_name.emplace(protected_ip_address_range,
                                                                      std::move(online_players_names));
                }
                display_protected_entries("Protected IP address ranges:",
                                          main_app.get_current_game_server().get_protected_ip_address_ranges(),
                                          protected_ip_address_range_to_player_name);
            }};
            display_large_data_set.detach();
            is_display_protected_ip_address_ranges_data_event.store(false);
        }
        else if (is_display_protected_cities_data_event.load())
        {
            std::thread display_large_data_set{[]() {
                unordered_map<string, string> protected_ip_to_player_city;
                const auto &players_data = main_app.get_current_game_server().get_players_data();
                for (const auto &protected_city : main_app.get_current_game_server().get_protected_cities())
                {
                    ostringstream oss;
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        if (protected_city == players_data[i].city)
                        {
                            oss << "^7" << players_data[i].player_name << " ^7| ";
                        }
                    }
                    string online_players_names{oss.str()};
                    if (online_players_names.length() >= 5)
                        online_players_names.erase(online_players_names.length() - 5, 5);
                    protected_ip_to_player_city.emplace(protected_city, std::move(online_players_names));
                }
                display_protected_entries(
                    "Protected cities:", main_app.get_current_game_server().get_protected_cities(),
                    protected_ip_to_player_city);
            }};
            display_large_data_set.detach();
            is_display_protected_cities_data_event.store(false);
        }
        else if (is_display_protected_countries_data_event.load())
        {
            std::thread display_large_data_set{[]() {
                unordered_map<string, string> protected_ip_to_player_country;
                const auto &players_data = main_app.get_current_game_server().get_players_data();
                for (const auto &protected_country : main_app.get_current_game_server().get_protected_cities())
                {
                    ostringstream oss;
                    for (size_t i{}; i < main_app.get_current_game_server().get_number_of_players(); ++i)
                    {
                        if (protected_country == players_data[i].country_name)
                        {
                            oss << "^7" << players_data[i].player_name << " ^7| ";
                        }
                    }
                    string online_players_names{oss.str()};
                    if (online_players_names.length() >= 5)
                        online_players_names.erase(online_players_names.length() - 5, 5);
                    protected_ip_to_player_country.emplace(protected_country, std::move(online_players_names));
                }
                display_protected_entries(
                    "Protected countries:", main_app.get_current_game_server().get_protected_countries(),
                    protected_ip_to_player_country);
            }};
            display_large_data_set.detach();
            is_display_protected_countries_data_event.store(false);
        }
    }

    break;

    case WM_NOTIFY: {
        static char unknown_button_label[4]{"..."};
        if (wParam == 501 && is_display_players_data.load())
        {

            auto nmhdr = (NMGRID *)lParam;
            int row_index = nmhdr->row;
            const int col_index = nmhdr->col;
            if (row_index < 0)
                row_index = 0;

            if (row_index < static_cast<int>(max_players_grid_rows))
            {
                selected_player_row = row_index;
            }

            if (col_index >= 0 && col_index < 6)
            {
                selected_player_col = col_index;
            }

            if (check_if_selected_player_has_my_ip_address())
            {
                EnableWindow(app_handles.hwnd_spectate_player_button, FALSE);
            }
            else
            {
                EnableWindow(app_handles.hwnd_spectate_player_button, TRUE);
            }

            const int pid{get_selected_players_pid_number(selected_player_row, 0)};
            const string ip_address{get_player_ip_address_for_pid(pid)};
            auto &p = get_player_data_for_pid(pid);
            if (unsigned long guid_key{}; check_ip_address_validity(ip_address, guid_key))
            {
                if (main_app.get_muted_players_map().contains(ip_address))
                {
                    p.is_muted = true;
                }
                else
                {
                    p.is_muted = false;
                }
            }
        }
        else if (wParam == 502 && !is_display_players_data.load())
        {
            auto nmhdr = (NMGRID *)lParam;
            int row_index = nmhdr->row;
            const int col_index = nmhdr->col;
            if (row_index < 0)
                row_index = 0;

            if (row_index < static_cast<int>(max_servers_grid_rows))
            {
                selected_server_row = row_index;
            }

            if (col_index >= 0 && col_index < 8)
            {
                selected_server_col = col_index;
            }

            if (static_cast<size_t>(selected_server_row) != main_app.get_game_server_index())
            {
                main_app.set_game_server_index(selected_server_row);
                initialize_elements_of_container_to_specified_value(
                    main_app.get_current_game_server().get_players_data(), player{}, 0);
                clear_players_data_in_players_grid(app_handles.hwnd_players_grid, 0, max_players_grid_rows, 8);
            }
        }

        auto some_item = (LPNMHDR)lParam;

        if ((some_item->idFrom == ID_WARNBUTTON || some_item->idFrom == ID_KICKBUTTON ||
             some_item->idFrom == ID_TEMPBANBUTTON || some_item->idFrom == ID_IPBANBUTTON ||
             some_item->idFrom == ID_VIEWTEMPBANSBUTTON || some_item->idFrom == ID_VIEWIPBANSBUTTON ||
             some_item->idFrom == ID_VIEWADMINSDATA || some_item->idFrom == ID_CONNECTBUTTON ||
             some_item->idFrom == ID_CONNECTPRIVATESLOTBUTTON || some_item->idFrom == ID_SAY_BUTTON ||
             some_item->idFrom == ID_TELL_BUTTON || some_item->idFrom == ID_QUITBUTTON ||
             some_item->idFrom == ID_LOADBUTTON || some_item->idFrom == ID_BUTTON_CONFIGURE_SERVER_SETTINGS ||
             some_item->idFrom == ID_CLEARMESSAGESCREENBUTTON || some_item->idFrom == ID_SHOWPLAYERSBUTTON ||
             some_item->idFrom == ID_REFRESH_PLAYERS_DATA_BUTTON || some_item->idFrom == ID_SHOWSERVERSBUTTON ||
             some_item->idFrom == ID_REFRESHSERVERSBUTTON || some_item->idFrom == ID_RCONVIEWBUTTON ||
             some_item->idFrom == ID_SPECTATEPLAYER || some_item->idFrom == ID_MUTE_PLAYER ||
             some_item->idFrom == ID_UNMUTE_PLAYER) &&
            (some_item->code == NM_CUSTOMDRAW))
        {

            auto item = (LPNMCUSTOMDRAW)some_item;

            if (item->uItemState & CDIS_FOCUS)
            {

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
                if (button_id_to_label_text.contains(some_item->idFrom))
                {
                    DrawTextExA(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                                DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
                }
                else
                {
                    DrawTextExA(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER,
                                nullptr);
                }
                return CDRF_SKIPDEFAULT;
            }
            if (item->uItemState & CDIS_HOT)
            {
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
                if (button_id_to_label_text.contains(some_item->idFrom))
                {
                    DrawTextEx(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                               DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
                }
                else
                {
                    DrawTextEx(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER,
                               nullptr);
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
            if (button_id_to_label_text.contains(some_item->idFrom))
            {
                DrawTextExA(item->hdc, (char *)button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                            DT_SINGLELINE | DT_CENTER | DT_VCENTER, nullptr);
            }
            else
            {
                DrawTextExA(item->hdc, unknown_button_label, -1, &item->rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER,
                            nullptr);
            }
            return CDRF_SKIPDEFAULT;
            // return CDRF_DODEFAULT;
        }

        return CDRF_DODEFAULT;
    }
    break;

    case WM_COMMAND: {

        const int wmId = LOWORD(wParam);
        const int wparam_high_word = HIWORD(wParam);

        switch (wmId)
        {

        case IDC_COPY: {
            const auto sel_text_len =
                SendMessage(app_handles.hwnd_re_messages_data, EM_GETSELTEXT, NULL, (LPARAM)selected_re_text);
            selected_re_text[sel_text_len] = '\0';
            if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(app_handles.hwnd_main_window))
            {
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
        }
        break;

        case IDC_PASTE: {
            // SetFocus(app_handles.hwnd_e_user_input);
            if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(app_handles.hwnd_main_window))
            {

                auto hglb = GetClipboardData(CF_TEXT);
                const auto clipboard_text = reinterpret_cast<const char *>(GlobalLock(hglb));

                const auto text_len =
                    GetWindowTextA(app_handles.hwnd_e_user_input, selected_re_text, std::size(selected_re_text));
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
        }
        break;

        case ID_QUITBUTTON:
            // execute_at_exit();
            // is_terminate_program.store(true);
            PostQuitMessage(0);
            break;

        case ID_CLEARMESSAGESCREENBUTTON: {
            SetWindowTextA(app_handles.hwnd_re_messages_data, "");
            g_message_data_contents.clear();
        }
        break;

        case ID_COPYGAMESERVERADDRESS: {
            copied_game_server_address = get_server_address_for_connect_command(selected_server_row);
        }
        break;

        case ID_CONNECTBUTTON: {
            string ip_port_server_address;
            if (!is_display_players_data.load())
            {
                ip_port_server_address = get_server_address_for_connect_command(selected_server_row);
            }
            else
            {
                match_results<string::const_iterator> ip_port_match{};
                ip_port_server_address = regex_search(admin_reason, ip_port_match, ip_address_and_port_regex)
                                             ? ip_port_match[1].str() + ":"s + ip_port_match[2].str()
                                             : main_app.get_current_game_server().get_server_ip_address() + ":"s +
                                                   to_string(main_app.get_current_game_server().get_server_port());
            }
            const size_t sep_pos{ip_port_server_address.find(':')};
            const string ip_address{ip_port_server_address.substr(0, sep_pos)};
            const uint16_t port_number{static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1)))};
            const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(
                ip_address.c_str(), port_number, main_app.get_current_game_server().get_rcon_password().c_str());
            const game_name_t game_name{
                result.second != game_name_t::unknown
                    ? result.second
                    : convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name())};
            connect_to_the_game_server(ip_port_server_address, game_name, false, true);
        }
        break;

        case ID_CONNECTPRIVATESLOTBUTTON: {

            string ip_port_server_address;
            if (!is_display_players_data.load())
            {
                ip_port_server_address = get_server_address_for_connect_command(selected_server_row);
            }
            else
            {
                match_results<string::const_iterator> ip_port_match{};
                ip_port_server_address = regex_search(admin_reason, ip_port_match, ip_address_and_port_regex)
                                             ? ip_port_match[1].str() + ":"s + ip_port_match[2].str()
                                             : main_app.get_current_game_server().get_server_ip_address() + ":"s +
                                                   to_string(main_app.get_current_game_server().get_server_port());
            }
            const size_t sep_pos{ip_port_server_address.find(':')};
            const string ip_address{ip_port_server_address.substr(0, sep_pos)};
            const uint16_t port_number{static_cast<uint16_t>(stoul(ip_port_server_address.substr(sep_pos + 1)))};
            const auto result = check_if_specified_server_ip_port_and_rcon_password_are_valid(
                ip_address.c_str(), port_number, main_app.get_current_game_server().get_rcon_password().c_str());

            const game_name_t game_name{
                result.second != game_name_t::unknown
                    ? result.second
                    : convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name())};
            connect_to_the_game_server(ip_port_server_address, game_name, true, true);
        }
        break;

        case ID_REFRESH_PLAYERS_DATA_BUTTON: {
            if (!is_display_players_data.load())
            {
                is_display_players_data.store(true);
                ShowWindow(app_handles.hwnd_servers_grid, SW_HIDE);
                ShowWindow(app_handles.hwnd_players_grid, SW_SHOWNORMAL);
                const string current_non_rcon_game_server_name{
                    main_app.get_game_servers()[main_app.get_game_server_index()].get_server_name()};
                string window_title_message{
                    format("Currently displayed game server: {}:{} {}",
                           main_app.get_game_servers()[main_app.get_game_server_index()].get_server_ip_address(),
                           main_app.get_game_servers()[main_app.get_game_server_index()].get_server_port(),
                           current_non_rcon_game_server_name)};
                append_to_title(app_handles.hwnd_main_window, std::move(window_title_message));
            }
            initiate_sending_rcon_status_command_now();
        }
        break;

        case ID_LOADBUTTON: {
            char full_map[256];
            char rcon_gametype[16];
            GetWindowTextA(app_handles.hwnd_combo_box_map, full_map, 256);
            GetWindowTextA(app_handles.hwnd_combo_box_gametype, rcon_gametype, 16);
            const auto &full_map_names_to_rcon_map_names = get_full_map_names_to_rcon_map_names_for_specified_game_name(
                convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()));
            const auto &full_gametype_names = get_rcon_gametype_names_to_full_gametype_names_for_specified_game_name(
                convert_game_name_to_game_name_t(main_app.get_current_game_server().get_game_name()));
            if (stl::helper::len(full_map) > 0U && stl::helper::len(rcon_gametype) > 0U &&
                full_map_names_to_rcon_map_names.contains(full_map) && full_gametype_names.contains(rcon_gametype))
            {
                const string gametype_uc{stl::helper::to_upper_case(rcon_gametype)};
                (void)snprintf(message_buffer, std::size(message_buffer),
                               "^2Are you sure you want to load map ^3%s ^2in ^3%s "
                               "^2game type?\n",
                               full_map, gametype_uc.c_str());
                if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                {
                    const string load_map_command{stl::helper::str_join(
                        std::vector<string>{"!m", full_map_names_to_rcon_map_names.at(full_map), rcon_gametype}, " ")};
                    SetWindowTextA(app_handles.hwnd_e_user_input, load_map_command.c_str());
                    get_user_input();
                }
            }
        }
        break;

        case ID_MAP_RESTART: {

            const string current_rcon_map_name{main_app.get_current_game_server().get_current_map()};
            const auto &available_rcon_to_full_map_names = main_app.get_available_rcon_to_full_map_names();

            if (!current_rcon_map_name.empty())
            {
                const string current_full_map_name{
                    available_rcon_to_full_map_names.contains(current_rcon_map_name)
                        ? available_rcon_to_full_map_names.at(current_rcon_map_name).second
                        : current_rcon_map_name};
                string current_game_type{main_app.get_current_game_server().get_current_game_type()};
                stl::helper::to_upper_case_in_place(current_game_type);
                (void)snprintf(message_buffer, std::size(message_buffer),
                               "^2Do you want to restart ^1(!rc map_restart) "
                               "^2current map ^3%s ^2in ^1%s ^2game type?\n",
                               current_full_map_name.c_str(), current_game_type.c_str());
                if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                {
                    const string map_restart_command{
                        stl::helper::str_join(std::vector<string>{"!rc", "map_restart"}, " ")};
                    SetWindowTextA(app_handles.hwnd_e_user_input, map_restart_command.c_str());
                    get_user_input();
                }
            }
        }
        break;

        case ID_FAST_RESTART: {

            const string current_rcon_map_name{main_app.get_current_game_server().get_current_map()};
            const auto &available_rcon_to_full_map_names = main_app.get_available_rcon_to_full_map_names();

            if (!current_rcon_map_name.empty())
            {
                const string current_full_map_name{
                    available_rcon_to_full_map_names.contains(current_rcon_map_name)
                        ? available_rcon_to_full_map_names.at(current_rcon_map_name).second
                        : current_rcon_map_name};

                string current_game_type{main_app.get_current_game_server().get_current_game_type()};
                stl::helper::to_upper_case_in_place(current_game_type);
                (void)snprintf(message_buffer, std::size(message_buffer),
                               "^2Do you want to restart ^1(!rc fast_restart) "
                               "^2current match ^3%s ^2in ^1%s ^2game type?\n",
                               current_full_map_name.c_str(), current_game_type.c_str());
                if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                {
                    const string fast_restart_command{
                        stl::helper::str_join(std::vector<string>{"!rc", "fast_restart"}, " ")};
                    SetWindowTextA(app_handles.hwnd_e_user_input, fast_restart_command.c_str());
                    get_user_input();
                }
            }
        }
        break;

        case ID_PRINTPLAYERINFORMATION_ACTION: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "Player information printed")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^5Information about selected player:\n ^7%s\n", player_information.c_str());
                    print_colored_text(app_handles.hwnd_re_messages_data, message_buffer,
                                       is_append_message_to_richedit_control::yes, is_log_message::yes,
                                       is_log_datetime::yes);
                }
            }
        }
        break;

        case ID_SPECTATEPLAYER: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (!is_player_being_spectated.load())
                {
                    (void)snprintf(message_buffer, std::size(message_buffer), "!spec %d", pid);
                }
                else
                {
                    (void)snprintf(message_buffer, std::size(message_buffer), "!nospec");
                }
                Edit_SetText(app_handles.hwnd_e_user_input, message_buffer);
                get_user_input();
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data "
                                   "table!\n^5Please, select a non-empty, valid player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_MUTE_PLAYER: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                const string ip_address{get_player_ip_address_for_pid(pid)};
                if (unsigned long guid_key{}; check_ip_address_validity(ip_address, guid_key))
                {
                    (void)snprintf(message_buffer, std::size(message_buffer), "!mute %d", pid);
                    Edit_SetText(app_handles.hwnd_e_user_input, message_buffer);
                    get_user_input();
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data "
                                   "table!\n^5Please, select a non-empty, valid player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_UNMUTE_PLAYER: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                const string ip_address{get_player_ip_address_for_pid(pid)};
                if (unsigned long guid_key{}; check_ip_address_validity(ip_address, guid_key))
                {
                    (void)snprintf(message_buffer, std::size(message_buffer), "!unmute %d", pid);
                    Edit_SetText(app_handles.hwnd_e_user_input, message_buffer);
                    get_user_input();
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in the players' data "
                                   "table!\n^5Please, select a non-empty, valid player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_REPORTPLAYER: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "Reported")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to report selected player?\n ^7%s",
                                   player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!report %d %s", pid, admin_reason.c_str());
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        admin_reason.assign("not specified");
                        get_user_input();
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_WARNBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "Warned")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to warn selected player?\n ^7%s",
                                   player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!w %d %s", pid, admin_reason.c_str());
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        admin_reason.assign("not specified");
                        get_user_input();
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_KICKBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "Kicked")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to kick selected player?\n ^7%s",
                                   player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!k %d %s", pid, admin_reason.c_str());
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        admin_reason.assign("not specified");
                        get_user_input();
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_TEMPBANBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "Temporarily banned")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to temporarily ban IP "
                                   "address of selected player?\n ^7%s",
                                   player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!tb %d 24 %s", pid, admin_reason.c_str());
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        get_user_input();
                        admin_reason.assign("not specified");
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_GUIDBANBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "GUID banned")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to ban GUID key of "
                                   "selected player?\n ^7%s",
                                   player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!b %d %s", pid, admin_reason.c_str());
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        get_user_input();
                        admin_reason.assign("not specified");
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_IPBANBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "IP address banned")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to ban IP address of "
                                   "selected player?\n ^7%s",
                                   player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!gb %d %s", pid, admin_reason.c_str());
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        get_user_input();
                        admin_reason.assign("not specified");
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_IPRANGEBANBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "IP address range banned")};
                    const string narrow_ip_address_range{
                        get_narrow_ip_address_range_for_specified_ip_address(selected_player.ip_address)};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to ban IP address range "
                                   "^1%s ^3of selected player?\n ^7%s",
                                   narrow_ip_address_range.c_str(), player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!br %d %s", pid, admin_reason.c_str());
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        get_user_input();
                        admin_reason.assign("not specified");
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_CITYBANBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "City banned")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to ban city of selected "
                                   "player?\n ^7%s",
                                   player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!bancity %s", selected_player.city);
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        get_user_input();
                        admin_reason.assign("not specified");
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_COUNTRYBANBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "Country banned")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to ban country of "
                                   "selected player?\n ^7%s",
                                   player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!bancountry %s",
                                       selected_player.country_name);
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        get_user_input();
                        admin_reason.assign("not specified");
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_BANNAMEBUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    string player_name{selected_player.player_name};
                    remove_all_color_codes(player_name);
                    trim_in_place(player_name);
                    to_lower_case_in_place(player_name);
                    const string player_information{get_player_information(pid, true, "Player name banned")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^3Are you sure you want to ban name ^7%s ^3of "
                                   "selected player?\n ^7%s",
                                   player_name.c_str(), player_information.c_str());
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "!banname pid:%d", pid);
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        get_user_input();
                        admin_reason.assign("not specified");
                    }
                }
            }
            else
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^3You have selected an empty line ^1(invalid pid index)\n ^3in "
                                   "the players' data table!\n^5Please, select a non-empty, valid "
                                   "player's row.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
        }
        break;

        case ID_ENABLECITYBANBUTTON: {
            SetWindowTextA(app_handles.hwnd_e_user_input, "!egb");
            get_user_input();
        }
        break;

        case ID_DISABLECITYBANBUTTON: {
            SetWindowTextA(app_handles.hwnd_e_user_input, "!dgb");
            get_user_input();
        }
        break;

        case ID_ENABLECOUNTRYBANBUTTON: {
            SetWindowTextA(app_handles.hwnd_e_user_input, "!ecb");
            get_user_input();
        }
        break;

        case ID_DISABLECOUNTRYBANBUTTON: {
            SetWindowTextA(app_handles.hwnd_e_user_input, "!dcb");
            get_user_input();
        }
        break;

        case ID_SAY_BUTTON: {

            admin_reason = "Type your public message in here...";
            if (show_user_confirmation_dialog("^2Are you sure you want to send a ^1public message ^2visible "
                                              "to all players?\n",
                                              "Confirm your action", "Message:"))
            {
                (void)snprintf(msg_buffer, std::size(msg_buffer), "say \"%s\"", admin_reason.c_str());
                SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                get_user_input();
                admin_reason.assign("not specified");
            }
        }
        break;

        case ID_TELL_BUTTON: {
            if (check_if_selected_cell_indices_are_valid_for_players_grid(selected_player_row, selected_player_col))
            {
                const int pid{get_selected_players_pid_number(selected_player_row, selected_player_col)};
                if (pid != -1)
                {
                    selected_player = get_player_data_for_pid(pid);
                    const string player_information{get_player_information(pid, true, "Private information sent")};
                    (void)snprintf(message_buffer, std::size(message_buffer),
                                   "^2Are you sure you want to send a ^1private "
                                   "message ^2to selected player?\n ^7%s",
                                   player_information.c_str());
                    admin_reason = "Type your private message in here...";
                    if (show_user_confirmation_dialog(message_buffer, "Confirm your action", "Message:"))
                    {
                        (void)snprintf(msg_buffer, std::size(msg_buffer), "tell %d \"%s\"", pid, admin_reason.c_str());
                        SetWindowTextA(app_handles.hwnd_e_user_input, msg_buffer);
                        get_user_input();
                        admin_reason.assign("not specified");
                    }
                }
            }
        }
        break;

        case ID_RCONVIEWBUTTON: {
            selected_server_row = 0;
            if (main_app.get_game_server_index() != static_cast<size_t>(selected_server_row))
            {
                main_app.set_game_server_index(selected_server_row);
                initialize_elements_of_container_to_specified_value(
                    main_app.get_current_game_server().get_players_data(), player{}, 0);
                clear_players_data_in_players_grid(app_handles.hwnd_players_grid, 0, max_players_grid_rows, 8);
            }
            is_display_players_data.store(true);
            ShowWindow(app_handles.hwnd_servers_grid, SW_HIDE);
            ShowWindow(app_handles.hwnd_players_grid, SW_SHOWNORMAL);
            string window_title_message{
                format("Rcon game server: {}:{} {}", main_app.get_game_servers()[0].get_server_ip_address(),
                       main_app.get_game_servers()[0].get_server_port(), main_app.get_game_server_name())};
            append_to_title(app_handles.hwnd_main_window, std::move(window_title_message));
            type_of_sort = sort_type::geo_asc;
            clear_players_data_in_players_grid(app_handles.hwnd_players_grid, 0, max_players_grid_rows, 8);
            initiate_sending_rcon_status_command_now();
        }
        break;

        case ID_SHOWPLAYERSBUTTON: {

            is_display_players_data.store(true);
            ShowWindow(app_handles.hwnd_servers_grid, SW_HIDE);
            ShowWindow(app_handles.hwnd_players_grid, SW_SHOWNORMAL);
            const string current_non_rcon_game_server_name{
                main_app.get_game_servers()[main_app.get_game_server_index()].get_server_name()};
            string window_title_message{
                format("Currently displayed game server: {}:{} {}",
                       main_app.get_game_servers()[main_app.get_game_server_index()].get_server_ip_address(),
                       main_app.get_game_servers()[main_app.get_game_server_index()].get_server_port(),
                       current_non_rcon_game_server_name)};
            append_to_title(app_handles.hwnd_main_window, std::move(window_title_message));
            if (main_app.get_game_server_index() >= main_app.get_rcon_game_servers_count())
                type_of_sort = sort_type::score_desc;
            else
                type_of_sort = sort_type::geo_asc;
            initiate_sending_rcon_status_command_now();
        }
        break;

        case ID_SHOWSERVERSBUTTON: {
            is_display_players_data.store(false);
            ShowWindow(app_handles.hwnd_players_grid, SW_HIDE);
            ShowWindow(app_handles.hwnd_servers_grid, SW_SHOWNORMAL);
            const string current_rcon_game_rcon_server_name{main_app.get_game_servers()[0].get_server_name()};
            string window_title_message{
                format("Rcon game server: {}:{} {}", main_app.get_game_servers()[0].get_server_ip_address(),
                       main_app.get_game_servers()[0].get_server_port(), current_rcon_game_rcon_server_name)};
            append_to_title(app_handles.hwnd_main_window, std::move(window_title_message));
            std::thread th{[&]() {
                IsGUIThread(TRUE);
                view_game_servers(app_handles.hwnd_servers_grid);
            }};
            th.detach();
        }
        break;

        case ID_REFRESHSERVERSBUTTON: {
            is_display_players_data.store(false);
            ShowWindow(app_handles.hwnd_players_grid, SW_HIDE);
            ShowWindow(app_handles.hwnd_servers_grid, SW_SHOWNORMAL);

            std::thread th{[&]() {
                IsGUIThread(TRUE);
                try
                {

                    refresh_game_servers_data(app_handles.hwnd_servers_grid);
                }
                catch (std::exception &ex)
                {
                    const string error_message{format("^3A specific exception was caught in "
                                                      "refresh_game_servers_data(HWND) function's thread of "
                                                      "execution!\n^1Exception: {}",
                                                      ex.what())};
                    print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
                }
                catch (...)
                {
                    char buffer[512];
                    strerror_s(buffer, GetLastError());
                    const string error_message{format("^3A generic error was caught in "
                                                      "refresh_game_servers_data(HWND) function's thread of "
                                                      "execution\n^1Exception: {}",
                                                      buffer)};
                    print_colored_text(app_handles.hwnd_re_messages_data, error_message.c_str());
                }
            }};

            th.detach();
        }
        break;

        case ID_VIEWMUTEDPLAYERS:
            number_of_entries_to_display.store(25);
            process_user_command({"!muted", "25"});
            // is_display_muted_players_data_event.store(true);
            break;

        case ID_VIEWTEMPBANSBUTTON:
            number_of_entries_to_display.store(25);
            process_user_command({"!tempbans", "25"});
            // is_display_temporarily_banned_players_data_event.store(true);
            break;

        case ID_VIEWIPBANSBUTTON:
            number_of_entries_to_display.store(25);
            process_user_command({"!bans", "25"});
            // is_display_permanently_banned_players_data_event.store(true);
            break;

        case ID_VIEWIPRANGEBANSBUTTON:
            number_of_entries_to_display.store(25);
            process_user_command({"!ranges", "25"});
            // is_display_banned_ip_address_ranges_data_event.store(true);
            break;
        case ID_VIEWBANNEDCITIES:
            process_user_command({"!bannedcities"});
            // is_display_banned_cities_data_event.store(true);
            break;

        case ID_VIEWBANNEDCOUNTRIES:
            process_user_command({"!bannedcountries"});
            // is_display_banned_countries_data_event.store(true);
            break;

        case ID_VIEWREPORTEDPLAYERS:
            number_of_entries_to_display.store(25);
            process_user_command({"!reports", "25"});
            break;

        case ID_VIEWBANNEDPLAYERNAMES:
            number_of_entries_to_display.store(25);
            process_user_command({"!names", "25"});
            // is_display_banned_player_names_data_event.store(true);
            break;

        case ID_VIEWADMINSDATA: {
            if (me->is_admin)
            {
                check_if_admins_are_online_and_get_admins_player_names(
                    main_app.get_current_game_server().get_players_data(),
                    main_app.get_current_game_server().get_number_of_players());
                display_online_admins_information();
            }
            process_user_command({"!admins"});
            // is_display_public_admins_data_event.store(true);
        }
        break;

        case ID_VIEWPROTECTEDIPADDRESSES:
            process_user_command({"!protectedipaddresses"});
            // is_display_protected_ip_addresses_data_event.store(true);
            break;
        case ID_VIEWPROTECTEDIPADDRESSRANGES:
            process_user_command({"!protectedipaddressranges"});
            // is_display_protected_ip_address_ranges_data_event.store(true);
            break;
        case ID_VIEWPROTECTEDCITIES:
            process_user_command({"!protectedcities"});
            // is_display_protected_cities_data_event.store(true);
            break;
        case ID_VIEWPROTECTEDCOUNTRIES:
            process_user_command({"!protectedcountries"});
            // is_display_protected_countries_data_event.store(true);
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
            if (!show_and_process_tinyrcon_configuration_panel("Configure and verify your game server's settings."))
            {
                show_error(app_handles.hwnd_main_window, "Failed to construct and show TinyRcon configuration dialog!",
                           0);
            }
            if (is_terminate_program.load())
            {
                // execute_at_exit();
                PostQuitMessage(0);
            }
            break;
        }

        if (is_process_combobox_item_selection_event && wparam_high_word == CBN_SELCHANGE)
        {
            const auto combo_box_id = LOWORD(wParam);
            if (app_handles.hwnd_combo_box_sortmode == (HWND)lParam && ID_COMBOBOX_SORTMODE == combo_box_id)
            {
                GetWindowTextA(app_handles.hwnd_combo_box_sortmode, message_buffer, std::size(message_buffer));
                if (sort_mode_names_dict.find(message_buffer) != cend(sort_mode_names_dict))
                {
                    const auto selected_sort_type = sort_mode_names_dict.at(message_buffer);
                    if (selected_sort_type != type_of_sort)
                    {
                        process_sort_type_change_request(selected_sort_type);
                    }
                }
            }
        }

        if (wparam_high_word == BN_CLICKED)
        {
            SetFocus(app_handles.hwnd_main_window);
        }
    }
    break;

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

        RECT bounding_rectangle = {screen_width / 2 + 170, screen_height / 2 - 28, screen_width / 2 + 210,
                                   screen_height / 2 - 8};
        DrawTextA(hdcMem, "Map:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);

        bounding_rectangle = {screen_width / 2 + 370, screen_height / 2 - 28, screen_width / 2 + 450,
                              screen_height / 2 - 8};
        DrawTextA(hdcMem, "Gametype:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT);

        bounding_rectangle = {10, screen_height - 70, 130, screen_height - 50};

        DrawTextA(hdcMem, prompt_message, -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT | DT_END_ELLIPSIS);

        atomic_counter.store(std::min<size_t>(atomic_counter.load(), rcon_status_time_period));

        // Transfer the off-screen DC to the screen
        BitBlt(hdc, 0, 0, screen_width, screen_height, hdcMem, 0, 0, SRCCOPY);

        oldBitmap = SelectObject(hdcMem, g_hBitMap);
        GetObject(g_hBitMap, sizeof(bitmap), &bitmap);
        BitBlt(hdc, screen_width / 2 + 340, screen_height - 180, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0,
               SRCCOPY);
        SelectObject(hdcMem, oldBitmap);

        // Free-up the off-screen DC
        SelectObject(hdcMem, hOld);

        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        EndPaint(hWnd, &ps);
        return 0;
    }
    break;

    case WM_KEYDOWN:
        if (wParam == VK_DOWN || wParam == VK_UP)
            return 0;
        break;
    case WM_CLOSE:
        // execute_at_exit();
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, ID_TIMER);
        if (orig_textEditBrush)
            DeleteBrush(orig_textEditBrush);
        if (comboBrush1)
            DeleteBrush(comboBrush1);
        if (comboBrush2)
            DeleteBrush(comboBrush2);
        if (comboBrush3)
            DeleteBrush(comboBrush3);
        if (comboBrush4)
            DeleteBrush(comboBrush4);
        if (comboBrush5)
            DeleteBrush(comboBrush5);
        if (defaultbrush)
            DeleteBrush(defaultbrush);
        if (hotbrush)
            DeleteBrush(hotbrush);
        if (selectbrush)
            DeleteBrush(selectbrush);
        if (red_pen)
            DeletePen(red_pen);
        if (light_blue_pen)
            DeletePen(light_blue_pen);
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (hWnd == app_handles.hwnd_main_window && is_main_window_constructed)
        {
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

        if ((HWND)lParam == info1.hwndList)
        {
            auto dc = (HDC)wParam;
            SetBkMode(dc, OPAQUE);
            SetTextColor(dc, color::blue);
            SetBkColor(dc, color::yellow);
            if (comboBrush1 == nullptr)
                comboBrush1 = CreateSolidBrush(color::yellow);
            return (INT_PTR)comboBrush1;
        }
        if ((HWND)lParam == info2.hwndList)
        {
            auto dc = (HDC)wParam;
            SetBkMode(dc, OPAQUE);
            SetTextColor(dc, /*is_focused ? color::red : */ color::blue);
            SetBkColor(dc, color::yellow);
            if (comboBrush2 == nullptr)
                comboBrush2 = CreateSolidBrush(color::yellow);
            return (INT_PTR)comboBrush2;
        }

        if ((HWND)lParam == info3.hwndList)
        {
            auto dc = (HDC)wParam;
            SetBkMode(dc, OPAQUE);
            SetTextColor(dc, /*is_focused ? color::red : */ color::blue);
            SetBkColor(dc, color::yellow);
            if (comboBrush5 == nullptr)
                comboBrush5 = CreateSolidBrush(color::yellow);
            return (INT_PTR)comboBrush5;
        }
    }
    break;

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
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char buf[512];
    static HPEN red_pen{};
    static HPEN black_pen{};
    HBRUSH selectbrush{};

    switch (msg)
    {

    case WM_PAINT: {
        DWORD style = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (!(style & CBS_DROPDOWNLIST))
            break;

        PAINTSTRUCT ps{};
        auto hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        // Select our color when the button is selected
        if (GetFocus() == hwnd)
        {
            if (!selectbrush)
                selectbrush = CreateSolidBrush(color::yellow);

            // Create red_pen for button border
            if (!red_pen)
                red_pen = CreatePen(PS_SOLID, 2, color::red);

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
        }
        else
        {

            auto brush = CreateSolidBrush(color::light_blue);
            if (!black_pen)
                black_pen = CreatePen(PS_SOLID, 2, color::black);
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

        if (GetFocus() == hwnd)
        {
            RECT temp = rc;
            InflateRect(&temp, -2, -2);
            DrawFocusRect(hdc, &temp);
        }

        int index = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
        if (index >= 0)
        {
            SendMessage(hwnd, CB_GETLBTEXT, index, (LPARAM)buf);
            rc.left += 5;
            DrawTextA(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            rc.left = rc.right - 15;
            DrawTextA(hdc, "v", -1, &rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCDESTROY: {
        if (selectbrush)
            DeleteBrush(selectbrush);
        if (red_pen)
            DeletePen(red_pen);
        if (black_pen)
            DeletePen(black_pen);
        RemoveWindowSubclass(hwnd, ComboProc, uIdSubClass);
        break;
    }
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProcForConfirmationDialog(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    static char msg_buffer[1024];
    HBRUSH orig_textEditBrush{}, comboBrush1{}, comboBrush2{}, comboBrush3{}, comboBrush4{};
    HBRUSH defaultbrush{};
    HBRUSH hotbrush{};
    HBRUSH selectbrush{};
    static HPEN red_pen{};
    static HPEN light_blue_pen{};

    switch (message)
    {

    case WM_CREATE:
        is_first_left_mouse_button_click_in_reason_edit_control = true;
        EnableWindow(app_handles.hwnd_main_window, FALSE);
        break;

    case WM_NOTIFY: {
        static char unknown_button_label[4]{"..."};
        auto some_item = (LPNMHDR)lParam;

        if ((some_item->idFrom == ID_YES_BUTTON || some_item->idFrom == ID_NO_BUTTON) &&
            (some_item->code == NM_CUSTOMDRAW))
        {
            auto item = (LPNMCUSTOMDRAW)some_item;

            if ((item->uItemState & CDIS_FOCUS) || (item->uItemState & CDIS_SELECTED))
            {
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
                if (button_id_to_label_text.contains(some_item->idFrom))
                {
                    DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                }
                else
                {
                    DrawTextA(item->hdc, unknown_button_label, -1, &item->rc,
                              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                }
                return CDRF_SKIPDEFAULT;
            }
            if (item->uItemState & CDIS_HOT)
            {
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
                if (button_id_to_label_text.contains(some_item->idFrom))
                {
                    DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                }
                else
                {
                    DrawTextA(item->hdc, unknown_button_label, -1, &item->rc,
                              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
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
            if (button_id_to_label_text.contains(some_item->idFrom))
            {
                DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                          DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
            }
            else
            {
                DrawTextA(item->hdc, unknown_button_label, -1, &item->rc,
                          DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
            }
            return CDRF_SKIPDEFAULT;
        }
        return CDRF_DODEFAULT;
    }
    break;

    case WM_COMMAND: {
        const int wmId = LOWORD(wParam);
        switch (wmId)
        {

        case ID_YES_BUTTON:
            admin_choice.store(1);
            GetWindowTextA(app_handles.hwnd_e_reason, msg_buffer, std::size(msg_buffer));
            if (stl::helper::len(msg_buffer) > 0)
            {
                admin_reason.assign(msg_buffer);
            }
            else
            {
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
    }
    break;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hWnd, &ps);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, color::black);
        SetTextColor(hdc, color::red);
        RECT bounding_rectangle{10, 220, 90, 240};
        DrawTextA(hdc, "Reason:", -1, &bounding_rectangle, DT_SINGLELINE | DT_TOP | DT_LEFT /*| DT_END_ELLIPSIS*/);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_CLOSE:
        admin_choice.store(0);
        admin_reason = "not specified";
        EnableWindow(app_handles.hwnd_main_window, TRUE);
        SetFocus(app_handles.hwnd_players_grid);
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        if (orig_textEditBrush)
            DeleteBrush(orig_textEditBrush);
        if (comboBrush1)
            DeleteBrush(comboBrush1);
        if (comboBrush2)
            DeleteBrush(comboBrush2);
        if (comboBrush3)
            DeleteBrush(comboBrush3);
        if (comboBrush4)
            DeleteBrush(comboBrush4);
        if (defaultbrush)
            DeleteBrush(defaultbrush);
        if (hotbrush)
            DeleteBrush(hotbrush);
        if (selectbrush)
            DeleteBrush(selectbrush);
        if (red_pen)
            DeletePen(red_pen);
        if (light_blue_pen)
            DeletePen(light_blue_pen);
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
    // print_trace_message(__FILE__, __LINE__, __FUNCTION__);
    constexpr size_t max_path_length{32768};
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

    switch (message)
    {

    case WM_CREATE:
        is_first_left_mouse_button_click_in_reason_edit_control = true;
        EnableWindow(app_handles.hwnd_main_window, FALSE);
        break;

    case WM_NOTIFY: {
        static char unknown_button_label[4]{"..."};
        auto some_item = (LPNMHDR)lParam;

        if ((some_item->idFrom == ID_BUTTON_SAVE_CHANGES || some_item->idFrom == ID_BUTTON_TEST_CONNECTION ||
             some_item->idFrom == ID_BUTTON_CANCEL || some_item->idFrom == ID_BUTTON_CONFIGURATION_EXIT_TINYRCON ||
             some_item->idFrom == ID_BUTTON_CONFIGURATION_COD1_PATH ||
             some_item->idFrom == ID_BUTTON_CONFIGURATION_COD2_PATH ||
             some_item->idFrom == ID_BUTTON_CONFIGURATION_COD4_PATH ||
             some_item->idFrom == ID_BUTTON_CONFIGURATION_COD5_PATH) &&
            (some_item->code == NM_CUSTOMDRAW))
        {
            auto item = (LPNMCUSTOMDRAW)some_item;

            if ((item->uItemState & CDIS_FOCUS) || (item->uItemState & CDIS_SELECTED))
            {
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
                if (button_id_to_label_text.contains(some_item->idFrom))
                {
                    DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                }
                else
                {
                    DrawTextA(item->hdc, unknown_button_label, -1, &item->rc,
                              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                }
                return CDRF_SKIPDEFAULT;
            }
            if (item->uItemState & CDIS_HOT)
            {
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
                if (button_id_to_label_text.contains(some_item->idFrom))
                {
                    DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                }
                else
                {
                    DrawTextA(item->hdc, unknown_button_label, -1, &item->rc,
                              DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
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
            if (button_id_to_label_text.contains(some_item->idFrom))
            {
                DrawTextA(item->hdc, button_id_to_label_text.at(some_item->idFrom).c_str(), -1, &item->rc,
                          DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
            }
            else
            {
                DrawTextA(item->hdc, unknown_button_label, -1, &item->rc,
                          DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
            }
            return CDRF_SKIPDEFAULT;
        }
        return CDRF_DODEFAULT;
    }
    break;

    case WM_COMMAND: {
        const int wmId = LOWORD(wParam);
        switch (wmId)
        {

        case ID_ENABLE_CITY_BAN_CHECKBOX: {
            if (Button_GetCheck(app_handles.hwnd_enable_city_ban) == BST_UNCHECKED)
            {
                main_app.set_is_automatic_city_kick_enabled(true);
                Button_SetCheck(app_handles.hwnd_enable_city_ban, BST_CHECKED);
            }
            else
            {
                main_app.set_is_automatic_city_kick_enabled(false);
                Button_SetCheck(app_handles.hwnd_enable_city_ban, BST_UNCHECKED);
            }
        }
        break;

        case ID_ENABLE_COUNTRY_BAN_CHECKBOX: {

            if (Button_GetCheck(app_handles.hwnd_enable_country_ban) == BST_UNCHECKED)
            {
                main_app.set_is_automatic_country_kick_enabled(true);
                Button_SetCheck(app_handles.hwnd_enable_country_ban, BST_CHECKED);
            }
            else
            {
                main_app.set_is_automatic_country_kick_enabled(false);
                Button_SetCheck(app_handles.hwnd_enable_country_ban, BST_UNCHECKED);
            }
        }
        break;

        case ID_BUTTON_SAVE_CHANGES: {
            process_button_save_changes_click_event(app_handles.hwnd_configuration_dialog);
        }
        break;

        case ID_BUTTON_TEST_CONNECTION: {
            process_button_test_connection_click_event(app_handles.hwnd_configuration_dialog);
        }
        break;

        case ID_BUTTON_CANCEL: {
            is_terminate_tinyrcon_settings_configuration_dialog_window.store(true);
            EnableWindow(app_handles.hwnd_main_window, TRUE);
            SetFocus(app_handles.hwnd_players_grid);
            DestroyWindow(app_handles.hwnd_configuration_dialog);
        }
        break;

        case ID_BUTTON_CONFIGURATION_EXIT_TINYRCON: {
            EnableWindow(app_handles.hwnd_main_window, TRUE);
            SetFocus(app_handles.hwnd_players_grid);
            // execute_at_exit();
            DestroyWindow(app_handles.hwnd_configuration_dialog);
            is_terminate_program.store(true);
        }
        break;

        case ID_BUTTON_CONFIGURATION_COD1_PATH: {

            str_copy(install_path, "C:\\");
            (void)snprintf(msg_buffer, std::size(msg_buffer),
                           "Please, select your Call of Duty 1 game's "
                           "installation folder (codmp.exe) and click OK.");

            const char *cod1_game_path = BrowseFolder(install_path, msg_buffer);

            if (lstrcmp(cod1_game_path, "") == 0 || lstrcmp(cod1_game_path, "C:\\") == 0)
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^1Error! You haven't selected a valid folder "
                                   "for your game installation.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^5You have to select your ^1game's installation directory\n "
                                   "^5and click the OK button.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
            else
            {
                (void)snprintf(exe_file_path, max_path_length, "%s\\codmp.exe", cod1_game_path);
            }

            stl::helper::trim_in_place(exe_file_path);

            SetWindowTextA(app_handles.hwnd_cod1_path_edit, exe_file_path);
        }
        break;

        case ID_BUTTON_CONFIGURATION_COD2_PATH: {

            str_copy(install_path, "C:\\");
            (void)snprintf(msg_buffer, std::size(msg_buffer),
                           "Please, select your Call of Duty 2 game's "
                           "installation folder (cod2mp_s.exe) and click OK.");

            const char *cod2_game_path = BrowseFolder(install_path, msg_buffer);

            if (lstrcmp(cod2_game_path, "") == 0 || lstrcmp(cod2_game_path, "C:\\") == 0)
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^1Error! You haven't selected a valid folder "
                                   "for your game installation.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^5You have to select your ^1game's installation directory\n "
                                   "^5and click the OK button.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
            else
            {
                (void)snprintf(exe_file_path, max_path_length, "%s\\cod2mp_s.exe", cod2_game_path);
            }

            stl::helper::trim_in_place(exe_file_path);

            SetWindowTextA(app_handles.hwnd_cod2_path_edit, exe_file_path);
        }
        break;

        case ID_BUTTON_CONFIGURATION_COD4_PATH: {

            str_copy(install_path, "C:\\");
            (void)snprintf(msg_buffer, std::size(msg_buffer),
                           "Please, select your Call of Duty 4: Modern Warfare game's "
                           "installation folder (iw3mp.exe) and click OK.");

            const char *cod4_game_path = BrowseFolder(install_path, msg_buffer);

            if (lstrcmp(cod4_game_path, "") == 0 || lstrcmp(cod4_game_path, "C:\\") == 0)
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^1Error! You haven't selected a valid folder "
                                   "for your game installation.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^5You have to select your ^1game's installation directory\n "
                                   "^5and click the OK button.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
            else
            {
                (void)snprintf(exe_file_path, max_path_length, "%s\\iw3mp.exe", cod4_game_path);
            }

            stl::helper::trim_in_place(exe_file_path);

            SetWindowTextA(app_handles.hwnd_cod4_path_edit, exe_file_path);
        }
        break;

        case ID_BUTTON_CONFIGURATION_COD5_PATH: {

            str_copy(install_path, "C:\\");
            (void)snprintf(msg_buffer, std::size(msg_buffer),
                           "Please, select your Call of Duty 5: World at War game's "
                           "installation folder (cod5mp.exe) and click OK.");

            const char *cod5_game_path = BrowseFolder(install_path, msg_buffer);

            if (lstrcmp(cod5_game_path, "") == 0 || lstrcmp(cod5_game_path, "C:\\") == 0)
            {
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^1Error! You haven't selected a valid folder "
                                   "for your game installation.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
                print_colored_text(app_handles.hwnd_re_messages_data,
                                   "^5You have to select your ^1game's installation directory\n "
                                   "^5and click the OK button.\n",
                                   is_append_message_to_richedit_control::yes, is_log_message::yes,
                                   is_log_datetime::yes);
            }
            else
            {
                (void)snprintf(exe_file_path, max_path_length, "%s\\cod5mp.exe", cod5_game_path);
            }

            stl::helper::trim_in_place(exe_file_path);

            SetWindowText(app_handles.hwnd_cod5_path_edit, exe_file_path);
        }
        break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_CLOSE:
        EnableWindow(app_handles.hwnd_main_window, TRUE);
        SetFocus(app_handles.hwnd_players_grid);
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        if (orig_textEditBrush)
            DeleteBrush(orig_textEditBrush);
        if (comboBrush1)
            DeleteBrush(comboBrush1);
        if (comboBrush2)
            DeleteBrush(comboBrush2);
        if (comboBrush3)
            DeleteBrush(comboBrush3);
        if (comboBrush4)
            DeleteBrush(comboBrush4);
        if (defaultbrush)
            DeleteBrush(defaultbrush);
        if (hotbrush)
            DeleteBrush(hotbrush);
        if (selectbrush)
            DeleteBrush(selectbrush);
        if (red_pen)
            DeletePen(red_pen);
        if (light_blue_pen)
            DeletePen(light_blue_pen);
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
