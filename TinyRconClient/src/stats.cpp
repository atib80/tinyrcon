#include "player.h"
#include "stats.h"
#include "tiny_rcon_utility_functions.h"

#include <cstdint>
#include <ctime>

stats::stats()
{
  set_stop_time_stamp();
}

void stats::update_player_score(const player &p, const int score_diff, const time_t time_diff, const time_t current_timestamp)
{
  std::lock_guard lg{stats_data_mu};

  bool new_player_stats_data{scores_map.contains(p.player_name_index)};

  if (!new_player_stats_data)
  {
    scores_map.emplace(p.player_name_index, player_stats{});
    size_t no_of_chars_to_copy{std::min<size_t>(p.player_name_index.length(), std::size(scores_map.at(p.player_name_index).index_name) - 1)};
    strncpy_s(scores_map.at(p.player_name_index).index_name, std::size(scores_map.at(p.player_name_index).index_name), p.player_name_index.c_str(), no_of_chars_to_copy);
    scores_map.at(p.player_name_index).index_name[no_of_chars_to_copy] = '\0';
    no_of_chars_to_copy = std::min<size_t>(p.ip_address.length(), 15u);
    strncpy_s(scores_map.at(p.player_name_index).ip_address, std::size(scores_map.at(p.player_name_index).ip_address), p.ip_address.c_str(), no_of_chars_to_copy);
    scores_map.at(p.player_name_index).ip_address[no_of_chars_to_copy] = '\0';
    no_of_chars_to_copy = std::min<size_t>(stl::helper::len(p.player_name), std::size(scores_map.at(p.player_name_index).player_name) - 1);
    strncpy_s(scores_map.at(p.player_name_index).player_name, std::size(scores_map.at(p.player_name_index).player_name), p.player_name, no_of_chars_to_copy);
    scores_map.at(p.player_name_index).player_name[no_of_chars_to_copy] = '\0';

    if (0 == scores_map.at(p.player_name_index).first_seen)
    {
      scores_map.at(p.player_name_index).first_seen = current_timestamp;
    }

    if (0 == scores_map.at(p.player_name_index).last_seen)
    {
      scores_map.at(p.player_name_index).last_seen = current_timestamp;
    }
  }

  scores_map[p.player_name_index].score += score_diff;
  scores_map[p.player_name_index].time_spent_on_server_in_seconds += time_diff;
  scores_map[p.player_name_index].last_seen = current_timestamp;

  if (!new_player_stats_data)
  {
    scores_vector.emplace_back(scores_map[p.player_name_index]);
  }

  new_player_stats_data = scores_for_year_map.contains(p.player_name_index);

  if (!new_player_stats_data)
  {
    scores_for_year_map.emplace(p.player_name_index, player_stats{});
    size_t no_of_chars_to_copy{std::min<size_t>(p.player_name_index.length(), std::size(scores_for_year_map.at(p.player_name_index).index_name) - 1)};
    strncpy_s(scores_for_year_map.at(p.player_name_index).index_name, std::size(scores_for_year_map.at(p.player_name_index).index_name), p.player_name_index.c_str(), no_of_chars_to_copy);
    scores_for_year_map.at(p.player_name_index).index_name[no_of_chars_to_copy] = '\0';
    no_of_chars_to_copy = std::min<size_t>(p.ip_address.length(), 15u);
    strncpy_s(scores_for_year_map.at(p.player_name_index).ip_address, std::size(scores_for_year_map.at(p.player_name_index).ip_address), p.ip_address.c_str(), no_of_chars_to_copy);
    scores_for_year_map.at(p.player_name_index).ip_address[no_of_chars_to_copy] = '\0';
    no_of_chars_to_copy = std::min<size_t>(stl::helper::len(p.player_name), std::size(scores_for_year_map.at(p.player_name_index).player_name) - 1);
    strncpy_s(scores_for_year_map.at(p.player_name_index).player_name, std::size(scores_for_year_map.at(p.player_name_index).player_name), p.player_name, no_of_chars_to_copy);
    scores_for_year_map.at(p.player_name_index).player_name[no_of_chars_to_copy] = '\0';

    if (0 == scores_for_year_map.at(p.player_name_index).first_seen)
    {
      scores_for_year_map.at(p.player_name_index).first_seen = current_timestamp;
    }

    if (0 == scores_for_year_map.at(p.player_name_index).last_seen)
    {
      scores_for_year_map.at(p.player_name_index).last_seen = current_timestamp;
    }
  }

  scores_for_year_map[p.player_name_index].score += score_diff;
  scores_for_year_map[p.player_name_index].time_spent_on_server_in_seconds += time_diff;
  scores_for_year_map[p.player_name_index].last_seen = current_timestamp;

  if (!new_player_stats_data)
  {
    scores_for_year_vector.emplace_back(scores_for_year_map[p.player_name_index]);
  }

  new_player_stats_data = scores_for_month_map.contains(p.player_name_index);

  if (!new_player_stats_data)
  {
    scores_for_month_map.emplace(p.player_name_index, player_stats{});
    size_t no_of_chars_to_copy{std::min<size_t>(p.player_name_index.length(), std::size(scores_for_month_map.at(p.player_name_index).index_name) - 1)};
    strncpy_s(scores_for_month_map.at(p.player_name_index).index_name, std::size(scores_for_month_map.at(p.player_name_index).index_name), p.player_name_index.c_str(), no_of_chars_to_copy);
    scores_for_month_map.at(p.player_name_index).index_name[no_of_chars_to_copy] = '\0';
    no_of_chars_to_copy = std::min<size_t>(p.ip_address.length(), 15u);
    strncpy_s(scores_for_month_map.at(p.player_name_index).ip_address, std::size(scores_for_month_map.at(p.player_name_index).ip_address), p.ip_address.c_str(), no_of_chars_to_copy);
    scores_for_month_map.at(p.player_name_index).ip_address[no_of_chars_to_copy] = '\0';
    no_of_chars_to_copy = std::min<size_t>(stl::helper::len(p.player_name), std::size(scores_for_month_map.at(p.player_name_index).player_name) - 1);
    strncpy_s(scores_for_month_map.at(p.player_name_index).player_name, std::size(scores_for_month_map.at(p.player_name_index).player_name), p.player_name, no_of_chars_to_copy);
    scores_for_month_map.at(p.player_name_index).player_name[no_of_chars_to_copy] = '\0';

    if (0 == scores_for_month_map.at(p.player_name_index).first_seen)
    {
      scores_for_month_map.at(p.player_name_index).first_seen = current_timestamp;
    }

    if (0 == scores_for_month_map.at(p.player_name_index).last_seen)
    {
      scores_for_month_map.at(p.player_name_index).last_seen = current_timestamp;
    }
  }

  scores_for_month_map[p.player_name_index].score += score_diff;
  scores_for_month_map[p.player_name_index].time_spent_on_server_in_seconds += time_diff;
  scores_for_month_map[p.player_name_index].last_seen = current_timestamp;

  if (!new_player_stats_data)
  {
    scores_for_month_vector.emplace_back(scores_for_month_map[p.player_name_index]);
  }

  new_player_stats_data = scores_for_day_map.contains(p.player_name_index);

  if (!new_player_stats_data)
  {
    scores_for_day_map.emplace(p.player_name_index, player_stats{});
    size_t no_of_chars_to_copy{std::min<size_t>(p.player_name_index.length(), std::size(scores_for_day_map.at(p.player_name_index).index_name) - 1)};
    strncpy_s(scores_for_day_map.at(p.player_name_index).index_name, std::size(scores_for_day_map.at(p.player_name_index).index_name), p.player_name_index.c_str(), no_of_chars_to_copy);
    scores_for_day_map.at(p.player_name_index).index_name[no_of_chars_to_copy] = '\0';
    no_of_chars_to_copy = std::min<size_t>(p.ip_address.length(), 15u);
    strncpy_s(scores_for_day_map.at(p.player_name_index).ip_address, std::size(scores_for_day_map.at(p.player_name_index).ip_address), p.ip_address.c_str(), no_of_chars_to_copy);
    scores_for_day_map.at(p.player_name_index).ip_address[no_of_chars_to_copy] = '\0';
    no_of_chars_to_copy = std::min<size_t>(stl::helper::len(p.player_name), std::size(scores_for_day_map.at(p.player_name_index).player_name) - 1);
    strncpy_s(scores_for_day_map.at(p.player_name_index).player_name, std::size(scores_for_day_map.at(p.player_name_index).player_name), p.player_name, no_of_chars_to_copy);
    scores_for_day_map.at(p.player_name_index).player_name[no_of_chars_to_copy] = '\0';

    if (0 == scores_for_day_map.at(p.player_name_index).first_seen)
    {
      scores_for_day_map.at(p.player_name_index).first_seen = current_timestamp;
    }

    if (0 == scores_for_day_map.at(p.player_name_index).last_seen)
    {
      scores_for_day_map.at(p.player_name_index).last_seen = current_timestamp;
    }
  }

  scores_for_day_map[p.player_name_index].score += score_diff;
  scores_for_day_map[p.player_name_index].time_spent_on_server_in_seconds += time_diff;
  scores_for_day_map[p.player_name_index].last_seen = current_timestamp;

  if (!new_player_stats_data)
  {
    scores_for_day_vector.emplace_back(scores_for_day_map[p.player_name_index]);
  }
}

void stats::set_stop_time_stamp()
{
  const auto current_time_stamp_value = get_current_time_stamp();
  std::tm t{};
  std::tm stop_tm{};

  localtime_s(&t, &current_time_stamp_value);
  const auto current_month = t.tm_mon;
  const auto current_year = t.tm_year;

  const auto stop_day = [&]()
  {
    const auto next_day = t.tm_mday + 1;
    if (1 == current_month)
    {
      if (next_day == 29)
      {
        return is_leap_year(1900 + current_year) ? 29 : (next_day < 29 ? next_day : 1);
      }
    }
    return next_day <= days_in_months[current_month] ? next_day : 1;
  }();

  if (current_time_stamp_value >= stop_time_in_seconds_for_year)
  {
    {
      std::lock_guard lg{stats_data_mu};
      get_scores_for_year_vector().clear();
      get_scores_for_year_map().clear();
    }
    tm next_year_stop_time{};
    next_year_stop_time.tm_year = t.tm_year + 1;
    next_year_stop_time.tm_mon = 0;
    next_year_stop_time.tm_mday = 1;
    next_year_stop_time.tm_hour = 0;
    next_year_stop_time.tm_min = 0;
    stop_time_in_seconds_for_year = mktime(&next_year_stop_time);
  }

  if (current_time_stamp_value >= stop_time_in_seconds_for_month)
  {
    {
      std::lock_guard lg{stats_data_mu};
      get_scores_for_month_vector().clear();
      get_scores_for_month_map().clear();
    }

    const auto stop_month = current_month < 11 ? current_month + 1 : 0;
    const auto stop_year{0 == current_month ? t.tm_year + 1 : t.tm_year};

    tm next_month_stop_time{};
    next_month_stop_time.tm_year = stop_year;
    next_month_stop_time.tm_mon = stop_month;
    next_month_stop_time.tm_mday = 1;
    next_month_stop_time.tm_hour = 0;
    next_month_stop_time.tm_min = 0;
    stop_time_in_seconds_for_month = mktime(&next_month_stop_time);
  }

  if (current_time_stamp_value >= stop_time_in_seconds_for_day)
  {
    {
      std::lock_guard lg{stats_data_mu};
      get_scores_for_day_vector().clear();
      get_scores_for_day_map().clear();
    }

    auto stop_month{current_month};
    auto stop_year{t.tm_year};

    if (1 == stop_day)
    {
      if (11 == current_month)
      {
        stop_month = 0;
        stop_year = current_year + 1;
      }
      else
      {
        ++stop_month;
      }
    }

    tm next_day_stop_time{};
    next_day_stop_time.tm_year = stop_year;
    next_day_stop_time.tm_mon = stop_month;
    next_day_stop_time.tm_mday = stop_day;
    next_day_stop_time.tm_hour = 0;
    next_day_stop_time.tm_min = 0;
    stop_time_in_seconds_for_day = mktime(&next_day_stop_time);
  }
}
