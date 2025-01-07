#pragma once


#define DISABLE_COPY_SEMANTICS(TYPE) \
  TYPE(const TYPE&) = delete;\
  TYPE& operator=(const TYPE&) = delete;

#define DISABLE_MOVE_SEMANTICS(TYPE) \
  TYPE(TYPE&&) = delete;\
  TYPE& operator=(TYPE&&) = delete;

class disabled_copy_operations {
	public:
	disabled_copy_operations() = default;
	disabled_copy_operations(const disabled_copy_operations&) = delete;
	disabled_copy_operations& operator=(const disabled_copy_operations&) = delete;
};

class disabled_move_operations {
	public:
	disabled_move_operations() = default;
	disabled_move_operations(disabled_move_operations&&) = delete;
	disabled_move_operations& operator=(disabled_move_operations&&) = delete;
};