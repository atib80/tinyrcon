#include "tiny_rcon_client_sqlite_database.h"
#include "tiny_rcon_utility_functions.h"
#include "player.h"

extern tiny_rcon_handles app_handles;

using namespace std;

tiny_rcon_client_sqlite_database::tiny_rcon_client_sqlite_database(std::string database_file_path, HWND messages_window,
                                                                   print_message_fn_callback print_message_func)
    : db_file_path{std::move(database_file_path)}, messages_window{messages_window},
      print_colored_text{print_message_func}
{
    is_db_initialized_ = true;
    if (SQLITE_OK != sqlite3_open_v2(db_file_path.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr))
    {
        is_db_initialized_ = false;
        const string error_msg{format("^3Could not open ^1sqlite3 database ^3file at specified file "
                                      "path: ^5{}",
                                      db_file_path)};
        print_colored_text(messages_window, error_msg.c_str(), is_append_message_to_richedit_control::yes,
                           is_log_message::yes, is_log_datetime::yes, false, true, false);
        print_colored_text(messages_window, sqlite3_errmsg(db), is_append_message_to_richedit_control::yes,
                           is_log_message::yes, is_log_datetime::yes, false, true, false);
        sqlite3_close_v2(db);
    }
}

bool tiny_rcon_client_sqlite_database::create_database_tables() const
{
    bool is_created_all_tables{true};
    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS temporary_bans (ip_address TEXT NOT NULL "
                                        "PRIMARY KEY, player_name TEXT NOT NULL, banned_start_time TIMESTAMP "
                                        "NOT NULL, ban_duration_in_hours INTEGER NOT NULL, reason TEXT "
                                        "NOT_NULL, banned_by_user_name TEXT NOT_NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS removed_temporary_bans (ip_address TEXT "
                                        "NOT NULL PRIMARY KEY, player_name TEXT NOT NULL, banned_start_time "
                                        "TIMESTAMP NOT NULL, ban_duration_in_hours INTEGER NOT NULL, reason "
                                        "TEXT NOT_NULL, banned_by_user_name TEXT NOT_NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS ip_address_bans (ip_address TEXT NOT "
                                        "NULL PRIMARY KEY, guid_key TEXT NOT NULL, player_name TEXT NOT "
                                        "NULL, banned_start_time TIMESTAMP NOT NULL, reason TEXT NOT_NULL, "
                                        "banned_by_user_name TEXT NOT_NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS removed_ip_address_bans (ip_address TEXT "
                                        "NOT NULL PRIMARY KEY, player_name TEXT NOT NULL, banned_start_time "
                                        "TIMESTAMP NOT NULL, ban_duration_in_hours INTEGER NOT NULL, reason "
                                        "TEXT NOT_NULL, banned_by_user_name TEXT NOT_NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS ip_address_range_bans (ip_address_range "
                                        "TEXT NOT NULL PRIMARY KEY, guid_key TEXT NOT NULL, player_name TEXT "
                                        "NOT NULL, banned_start_time TIMESTAMP NOT NULL, reason TEXT "
                                        "NOT_NULL, banned_by_user_name TEXT NOT_NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS removed_ip_address_range_bans "
                                        "(ip_address_range TEXT NOT NULL PRIMARY KEY, guid_key TEXT NOT "
                                        "NULL, player_name TEXT NOT NULL, banned_start_time TIMESTAMP NOT "
                                        "NULL, reason TEXT NOT_NULL, banned_by_user_name TEXT NOT_NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS city_bans (city TEXT NOT NULL PRIMARY "
                                        "KEY, banned_start_time TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS removed_city_bans (city TEXT NOT NULL "
                                        "PRIMARY KEY, banned_start_time TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS country_bans (country TEXT NOT NULL "
                                        "PRIMARY KEY, banned_start_time TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS removed_country_bans (country TEXT NOT "
                                        "NULL PRIMARY KEY, banned_start_time TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS protected_ip_addresses (ip_address TEXT "
                                        "NOT NULL PRIMARY KEY, protected_start_time TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS protected_ip_address_ranges "
                                        "(ip_address_range TEXT NOT NULL PRIMARY KEY, "
                                        "protected_start_time TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS protected_cities (city TEXT NOT NULL "
                                        "PRIMARY KEY, protected_start_time TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS protected_countries (country TEXT NOT "
                                        "NULL PRIMARY KEY, protected_start_time TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    if (sqlite3_stmt * sql_statement{};
        SQLITE_OK == sqlite3_prepare_v2(db,
                                        "CREATE TABLE IF NOT EXISTS users (username TEXT NOT NULL PRIMARY "
                                        "KEY, is_admin TINYINT(1) NOT_NULL, is_logged_in TINYINT(1) "
                                        "NOT_NULL, is_online TINYINT(1) NOT NULL, last_login TIMESTAMP NOT "
                                        "NULL, last_logout TIMESTAMP NOT NULL);",
                                        -1, &sql_statement, nullptr))
    {
        auto status = sqlite3_step(sql_statement);
        sqlite3_finalize(sql_statement);
    }
    else
    {
        is_created_all_tables = false;
    }

    return is_created_all_tables;
}

bool tiny_rcon_client_sqlite_database::load_file_contents_into_sqlite3_table(const table_id table,
                                                                             const char *file_path) const
{
    if (!check_if_file_path_exists(file_path))
        return false;

    if (table == table_id::tempborary_bans)
    {
        vector<player> temporary_ip_bans_vector;
        unordered_map<string, player> temporary_ip_bans_map;
        parse_tempbans_data_file(file_path, temporary_ip_bans_vector, temporary_ip_bans_map);
    }

    return true;
}
