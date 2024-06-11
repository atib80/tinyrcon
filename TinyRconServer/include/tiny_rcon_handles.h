#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct tiny_rcon_handles
{
    HINSTANCE hInstance;
    HWND hwnd_main_window;
    HWND hwnd_users_table;
    HWND hwnd_online_admins_information;
    HWND hwnd_re_messages_data;
    HWND hwnd_e_user_input;
    HWND hwnd_confirmation_dialog;
    HWND hwnd_yes_button;
    HWND hwnd_no_button;
    HWND hwnd_re_confirmation_message;
    HWND hwnd_e_reason;
    HWND hwnd_download_and_upload_speed_info;
};