#pragma once
#include <format>
#include <string>
#include <sstream>
#include <unordered_map>
#include "sqlite3.h"
#include "tiny_rcon_utility_data_types.h"

enum class table_id { tempborary_bans,
  ip_address_bans,
  ip_address_range_bans,
  city_bans,
  country_bans,
  protected_ip_addresses,
  protected_ip_address_ranges,
  protected_cities,
  protected_countries };

using std::format;
using std::string;
using std::ostringstream;

class tiny_rcon_client_sqlite_database
{
  using print_message_fn_callback = size_t (*)(HWND re_control, const char *text, const is_append_message_to_richedit_control print_to_richedit_control, const is_log_message log_to_file, const is_log_datetime is_log_current_date_time, const bool, const bool is_remove_color_codes_for_log_message, const bool is_display_message_to_remote_user);
  print_message_fn_callback print_colored_text{};
  HWND messages_window{};
  sqlite3 *db{};
  bool is_db_initialized_{};
  std::string db_file_path;
  const std::unordered_map<table_id, std::string> table_id_to_table_name{
    { table_id::tempborary_bans, "temporary_bans" },
    { table_id::ip_address_bans, "ip_address_bans" },
    { table_id::ip_address_range_bans, "ip_address_range_bans" },
    { table_id::city_bans, "city_bans" },
    { table_id::country_bans, "country_bans" },
    { table_id::protected_ip_addresses, "protected_ip_addresses" },
    { table_id::protected_ip_address_ranges, "protected_ip_address_ranges" },
    { table_id::protected_cities, "protected_cities" },
    { table_id::protected_countries, "protected_countries" }
  };

public:
  explicit tiny_rcon_client_sqlite_database(std::string database_file_path, HWND messages_window, print_message_fn_callback print_message_func);

  tiny_rcon_client_sqlite_database(const tiny_rcon_client_sqlite_database &) = delete;
  tiny_rcon_client_sqlite_database(tiny_rcon_client_sqlite_database &&) = delete;

  tiny_rcon_client_sqlite_database &operator=(const tiny_rcon_client_sqlite_database &) = delete;
  tiny_rcon_client_sqlite_database &operator=(tiny_rcon_client_sqlite_database &&) = delete;

  ~tiny_rcon_client_sqlite_database()
  {
    if (*this) { sqlite3_close_v2(db); }
  }

  bool create_database_tables() const;

  template<typename T, typename... Values>
  void append_value_to_string(std::ostringstream &oss, const T &arg, const Values &...values)
  {
    oss << arg;
    if (sizeof...(values) > 0) {
      oss << ',';
      append_value_to_string(oss, values...);
    }
  }

  template<typename... Values>
  bool insert_values_into_table(const table_id table, const Values &...values)
  {
    char *err_msg{};
    std::ostringstream oss;
    oss << format("INSERT OR REPLACE INTO {} VALUES(", table_id_to_table_name.at(table));
    append_value_to_string(oss, values...);
    oss << ");";
    std::string sql{ oss.str() };

    auto rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
      const std::string error_msg{ format("^3SQL error: ^1{}", err_msg) };
      print_colored_text(messages_window, error_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, false, true, false);
      sqlite3_free(err_msg);
      return false;
    }

    const std::string info_msg{ "^5Successfully executed SQL statement: ^1{}", sql };
    print_colored_text(messages_window, info_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, false, true, false);
    return true;
  }

  template<typename T>
  bool delete_row_from_table(const table_id table, const std::string &column_name, T value)
  {
    char *err_msg{};
    const std::string sql{ format("DELETE FROM {} WHERE {}='{}';", table_id_to_table_name.at(table), column_name, value) };

    auto rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);

    if (rc != SQLITE_OK) {
      const std::string error_msg{ format("^3SQL error: ^1{}", err_msg) };
      print_colored_text(messages_window, error_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, false, true, false);
      sqlite3_free(err_msg);

    } else {
      const std::string info_msg{ "^5Successfully executed SQL statement: ^1{}", sql };
      print_colored_text(messages_window, info_msg.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, false, true, false);
    }

    if (err_msg) sqlite3_free(err_msg);
  }

  bool is_db_initizalized() const noexcept
  {
    return is_db_initialized_;
  }

  explicit operator bool() const noexcept
  {
    return db != nullptr && is_db_initialized_;
  }

  const std::unordered_map<table_id, std::string> &get_table_id_to_table_name() const noexcept
  {
    return table_id_to_table_name;
  }
};
