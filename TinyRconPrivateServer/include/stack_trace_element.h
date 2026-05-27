#pragma once

#include "tiny_rcon_utility_data_types.h"
#include <string>

struct stack_trace_element
{
  explicit stack_trace_element(HWND hwnd_re_control, std::string message);
  ~stack_trace_element();

  const int number_of_unhandled_exceptions{};
  HWND hwnd_re_control;
  std::string message;
};