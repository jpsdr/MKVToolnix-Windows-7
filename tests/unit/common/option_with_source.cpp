#include "common/common_pch.h"

#include "common/option_with_source.h"

#include "tests/unit/init.h"

namespace {

TEST(OptionWithSource, CreationWithoutValue) {
  option_with_source_c<int64_t> o;

  ASSERT_EQ(static_cast<bool>(o), false);
  ASSERT_THROW(o.get(),           std::logic_error);
  ASSERT_THROW(o.get_source(),    std::logic_error);
}

TEST(OptionWithSource, CreationWithValue) {
  option_with_source_c<int64_t> o(42, option_source_e::container);

  ASSERT_EQ(static_cast<bool>(o), true);
  ASSERT_EQ(o.get(),              42);
  ASSERT_EQ(o.get_source(),       option_source_e::container);
}

TEST(OptionWithSource, SetValueOnce) {
  option_with_source_c<int64_t> o;

  ASSERT_EQ(static_cast<bool>(o), false);
  o.set(42, option_source_e::container);
  ASSERT_EQ(static_cast<bool>(o), true);
  ASSERT_EQ(o.get(),              42);
  ASSERT_EQ(o.get_source(),       option_source_e::container);
}

TEST(OptionWithSource, SetValueLowerSource) {
  option_with_source_c<int64_t> o;

  ASSERT_EQ(static_cast<bool>(o), false);
  o.set(42, option_source_e::container);
  ASSERT_EQ(static_cast<bool>(o), true);
  ASSERT_EQ(o.get(),              42);
  ASSERT_EQ(o.get_source(),       option_source_e::container);
  o.set(54, option_source_e::bitstream);
  ASSERT_EQ(static_cast<bool>(o), true);
  ASSERT_EQ(o.get(),              42);
  ASSERT_EQ(o.get_source(),       option_source_e::container);
}

TEST(OptionWithSource, SetValueSameSource) {
  option_with_source_c<int64_t> o;

  ASSERT_EQ(static_cast<bool>(o), false);
  o.set(42, option_source_e::container);
  ASSERT_EQ(static_cast<bool>(o), true);
  ASSERT_EQ(o.get(),              42);
  ASSERT_EQ(o.get_source(),       option_source_e::container);
  o.set(54, option_source_e::container);
  ASSERT_EQ(static_cast<bool>(o), true);
  ASSERT_EQ(o.get(),              54);
  ASSERT_EQ(o.get_source(),       option_source_e::container);
}

TEST(OptionWithSource, SetValueHigherSource) {
  option_with_source_c<int64_t> o;

  ASSERT_EQ(static_cast<bool>(o), false);
  o.set(42, option_source_e::container);
  ASSERT_EQ(static_cast<bool>(o), true);
  ASSERT_EQ(o.get(),              42);
  ASSERT_EQ(o.get_source(),       option_source_e::container);
  o.set(54, option_source_e::command_line);
  ASSERT_EQ(static_cast<bool>(o), true);
  ASSERT_EQ(o.get(),              54);
  ASSERT_EQ(o.get_source(),       option_source_e::command_line);
}

}
