#pragma once

#include "tiny_rcon_utility_functions.h"
#include <stdexcept>
#include <string>

struct stack_trace_element
{
  explicit stack_trace_element(HWND hwnd_re_control, std::string message) : number_of_unhandled_exceptions{ std::uncaught_exceptions() },
                                                                            hwnd_re_control{ hwnd_re_control }, message{ std::move(message) }
  {}
  ~stack_trace_element()
  {
    if (number_of_unhandled_exceptions != std::uncaught_exceptions() && !message.empty())
      print_colored_text(hwnd_re_control, message.c_str());
  }

  const int number_of_unhandled_exceptions{};
  HWND hwnd_re_control;
  const std::string message;
};