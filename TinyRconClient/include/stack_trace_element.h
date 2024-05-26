#pragma once

#include <stdexcept>
#include <string>

extern size_t print_colored_text(HWND re_control, const char *text, const is_append_message_to_richedit_control, const is_log_message, is_log_datetime, const bool is_prevent_auto_vertical_scrolling, const bool is_remove_color_codes_for_log_message);

struct stack_trace_element
{
  explicit stack_trace_element(HWND hwnd_re_control, std::string message) : number_of_unhandled_exceptions{ std::uncaught_exceptions() },
                                                                            hwnd_re_control{ hwnd_re_control }, message{ std::move(message) }
  {}
  ~stack_trace_element()
  {
    if (number_of_unhandled_exceptions != std::uncaught_exceptions() && !message.empty())
      print_colored_text(hwnd_re_control, message.c_str(), is_append_message_to_richedit_control::yes, is_log_message::yes, is_log_datetime::yes, false, false);
  }

  const int number_of_unhandled_exceptions{};
  HWND hwnd_re_control;
  const std::string message;
};