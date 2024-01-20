#pragma once

#include <string>

using std::string;

struct player
{
  explicit player(const int pid = -1, const int score = 0, const char *country_code = "xy", const char *country_name = "Unknown", const char *region = "Unknown", const char *city = "Unknown") : pid{ pid }, score{ score }, country_code{ country_code }, country_name{ country_name }, region{ region }, city{ city } {}
  int pid{ -1 };
  int score{};
  unsigned long ip_hash_key{};
  const char *country_code{ "xy" };
  const char *country_name{ "Unknown" };
  const char *region{ "Unknown" };
  const char *city{ "Unknown" };
  size_t warned_times{};
  char ping[5]{};
  char geo_country_code[8];
  time_t banned_start_time{};
  time_t ban_duration_in_hours{ 24 };
  char player_name[33]{};
  char guid_key[33]{};
  char banned_date_time[33]{};
  char geo_information[128];
  std::string ip_address;
  std::string reason;
  std::string banned_by_user_name;
};
