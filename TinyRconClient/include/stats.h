#pragma once

#include "tiny_rcon_utility_classes.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::unordered_map;
using std::vector;

struct player;
struct player_stats;

class stats : public disabled_copy_operations, public disabled_move_operations
{
public:
  stats();
  std::recursive_mutex &get_stats_data_mutex() noexcept { return stats_data_mu; }
  void update_player_score(const player &p, const int score_diff, const time_t time_diff, const time_t timestamp);
  void set_stop_time_stamp();

  void set_stop_time_in_seconds_for_year(const time_t new_value)
  {
    if (new_value > stop_time_in_seconds_for_year)
      stop_time_in_seconds_for_year = new_value;
  }

  void set_stop_time_in_seconds_for_month(const time_t new_value)
  {
    if (new_value > stop_time_in_seconds_for_month)
      stop_time_in_seconds_for_month = new_value;
  }

  void set_stop_time_in_seconds_for_day(const time_t new_value)
  {
    if (new_value > stop_time_in_seconds_for_day)
      stop_time_in_seconds_for_day = new_value;
  }

  time_t get_stop_time_in_seconds_for_year() const noexcept { return stop_time_in_seconds_for_year; }
  time_t get_stop_time_in_seconds_for_month() const noexcept { return stop_time_in_seconds_for_month; }
  time_t get_stop_time_in_seconds_for_day() const noexcept { return stop_time_in_seconds_for_day; }

  std::vector<player_stats> &get_scores_vector() noexcept { return scores_vector; }
  std::vector<player_stats> &get_scores_for_year_vector() noexcept { return scores_for_year_vector; }
  std::vector<player_stats> &get_scores_for_month_vector() noexcept { return scores_for_month_vector; }
  std::vector<player_stats> &get_scores_for_day_vector() noexcept { return scores_for_day_vector; }

  std::unordered_map<std::string, player_stats> &get_scores_map() noexcept { return scores_map; }
  std::unordered_map<std::string, player_stats> &get_scores_for_year_map() noexcept { return scores_for_year_map; }
  std::unordered_map<std::string, player_stats> &get_scores_for_month_map() noexcept { return scores_for_month_map; }
  std::unordered_map<std::string, player_stats> &get_scores_for_day_map() noexcept { return scores_for_day_map; }

  std::unordered_map<std::string, std::pair<int, time_t>> &get_previous_player_score_and_timestamp() noexcept { return previous_player_score_and_timestamp; };

private:
  /*
  If the year is evenly divisible by 4, go to step 2. Otherwise, go to step 5.
  If the year is evenly divisible by 100, go to step 3. Otherwise, go to step 4.
  If the year is evenly divisible by 400, go to step 4. Otherwise, go to step 5.
  The year is a leap year (it has 366 days).
  The year is not a leap year (it has 365 days).
  */
  static bool is_leap_year(const int year)
  {
    return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
  }

  std::recursive_mutex stats_data_mu{};
  time_t stop_time_in_seconds_for_year{};
  time_t stop_time_in_seconds_for_month{};
  time_t stop_time_in_seconds_for_day{};

  static constexpr int days_in_months[12]{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  std::vector<player_stats> scores_vector;
  std::vector<player_stats> scores_for_year_vector;
  std::vector<player_stats> scores_for_month_vector;
  std::vector<player_stats> scores_for_day_vector;

  std::unordered_map<std::string, player_stats> scores_map;
  std::unordered_map<std::string, player_stats> scores_for_year_map;
  std::unordered_map<std::string, player_stats> scores_for_month_map;
  std::unordered_map<std::string, player_stats> scores_for_day_map;

  std::unordered_map<std::string, std::pair<int, time_t>> previous_player_score_and_timestamp;
};
