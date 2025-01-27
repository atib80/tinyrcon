#pragma once

#include <string>

using std::string;

struct player
{
  explicit player(const int pid = -1, const int score = 0, const char *country_code = "xy", const char *country_name = "Unknown", const char *region = "Unknown", const char *city = "Unknown") : pid{ pid }, score{ score }, country_code{ country_code }, country_name{ country_name }, region{ region }, city{ city } {}
  bool is_muted{};
  int pid{ -1 };
  int score{};
  unsigned long ip_hash_key{};
  char player_name[33]{};
  char guid_key[33]{};
  char banned_date_time[33]{};
  char ping[5]{};
  size_t warned_times{};
  time_t banned_start_time{};
  time_t ban_duration_in_hours{ 24 };
  const char *country_code{ "xy" };
  const char *country_name{};
  const char *region{};
  const char *city{};
  std::string player_name_index;
  std::string previous_ip_address;
  std::string ip_address;
  std::string reason;
  std::string banned_by_user_name;
};
