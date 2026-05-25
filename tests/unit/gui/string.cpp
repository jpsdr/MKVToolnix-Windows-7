#include "common/common_pch.h"

#include <QString>

#include "common/qt.h"
#include "mkvtoolnix-gui/util/string.h"

#include "tests/unit/init.h"

using namespace Qt::StringLiterals;

namespace {

TEST(GuiStringEscaping, EscapingShellUnixString) {
  EXPECT_EQ("hello"s,               to_utf8(mtx::gui::Util::escape(u"hello"_s,           mtx::gui::Util::EscapeShellUnix)));
  EXPECT_EQ("hello_world"s,         to_utf8(mtx::gui::Util::escape(u"hello_world"_s,     mtx::gui::Util::EscapeShellUnix)));
  EXPECT_EQ("'hello world'"s,       to_utf8(mtx::gui::Util::escape(u"hello world"_s,     mtx::gui::Util::EscapeShellUnix)));
  EXPECT_EQ("'\"hello world\"'"s,   to_utf8(mtx::gui::Util::escape(u"\"hello world\""_s, mtx::gui::Util::EscapeShellUnix)));
  EXPECT_EQ("\\''hello world'\\'"s, to_utf8(mtx::gui::Util::escape(u"'hello world'"_s,   mtx::gui::Util::EscapeShellUnix)));
  EXPECT_EQ("'`hello`'"s,           to_utf8(mtx::gui::Util::escape(u"`hello`"_s,         mtx::gui::Util::EscapeShellUnix)));
}

TEST(GuiStringEscaping, EscapingShellUnixStringList) {
  EXPECT_EQ("hello"s,               to_utf8(mtx::gui::Util::escape(QStringList{u"hello"_s},           mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
  EXPECT_EQ("hello_world"s,         to_utf8(mtx::gui::Util::escape(QStringList{u"hello_world"_s},     mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
  EXPECT_EQ("'hello world'"s,       to_utf8(mtx::gui::Util::escape(QStringList{u"hello world"_s},     mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
  EXPECT_EQ("'\"hello world\"'"s,   to_utf8(mtx::gui::Util::escape(QStringList{u"\"hello world\""_s}, mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
  EXPECT_EQ("\\''hello world'\\'"s, to_utf8(mtx::gui::Util::escape(QStringList{u"'hello world'"_s},   mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));

  EXPECT_EQ("hello!world"s,                            to_utf8(mtx::gui::Util::escape(QStringList{u"hello"_s,           u"world"_s},              mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
  EXPECT_EQ("'hello world'"s,                          to_utf8(mtx::gui::Util::escape(QStringList{u"hello world"_s},                              mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
  EXPECT_EQ("'\"hello world\"'!'\"same stuff\"'"s,     to_utf8(mtx::gui::Util::escape(QStringList{u"\"hello world\""_s, u"\"same stuff\""_s},     mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
  EXPECT_EQ("\\''hello world'\\'!\\''same stuff'\\'"s, to_utf8(mtx::gui::Util::escape(QStringList{u"'hello world'"_s,   u"'same stuff'"_s},       mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
  EXPECT_EQ("'`evil`'!'more`evil`thanthis'"s,          to_utf8(mtx::gui::Util::escape(QStringList{u"`evil`"_s,          u"more`evil`thanthis"_s}, mtx::gui::Util::EscapeShellUnix).join(u"!"_s)));
}

TEST(GuiStringEscaping, EscapingShellCmdExeProgramString) {
  EXPECT_EQ("hello"s,                       to_utf8(mtx::gui::Util::escape(u"hello"_s,           mtx::gui::Util::EscapeShellCmdExeProgram)));
  EXPECT_EQ("hello_world"s,                 to_utf8(mtx::gui::Util::escape(u"hello_world"_s,     mtx::gui::Util::EscapeShellCmdExeProgram)));
  EXPECT_EQ("\"hello world\""s,             to_utf8(mtx::gui::Util::escape(u"hello world"_s,     mtx::gui::Util::EscapeShellCmdExeProgram)));
  EXPECT_EQ("\"'hello world'\""s,           to_utf8(mtx::gui::Util::escape(u"'hello world'"_s,   mtx::gui::Util::EscapeShellCmdExeProgram)));
  EXPECT_EQ("\"`hello`\""s,                 to_utf8(mtx::gui::Util::escape(u"`hello`"_s,         mtx::gui::Util::EscapeShellCmdExeProgram)));
}

TEST(GuiStringEscaping, EscapingShellCmdExeArgumentString) {
  EXPECT_EQ("hello"s,                       to_utf8(mtx::gui::Util::escape(u"hello"_s,           mtx::gui::Util::EscapeShellCmdExeArgument)));
  EXPECT_EQ("hello_world"s,                 to_utf8(mtx::gui::Util::escape(u"hello_world"_s,     mtx::gui::Util::EscapeShellCmdExeArgument)));
  EXPECT_EQ("^\"hello world^\""s,           to_utf8(mtx::gui::Util::escape(u"hello world"_s,     mtx::gui::Util::EscapeShellCmdExeArgument)));
  EXPECT_EQ("^\"\\^\"hello world\\^\"^\""s, to_utf8(mtx::gui::Util::escape(u"\"hello world\""_s, mtx::gui::Util::EscapeShellCmdExeArgument)));
  EXPECT_EQ("^\"'hello world'^\""s,         to_utf8(mtx::gui::Util::escape(u"'hello world'"_s,   mtx::gui::Util::EscapeShellCmdExeArgument)));
  EXPECT_EQ("^\"`hello`^\""s,               to_utf8(mtx::gui::Util::escape(u"`hello`"_s,         mtx::gui::Util::EscapeShellCmdExeArgument)));
}

// NOT IMPLEMENTED YET:

// TEST(GuiStringEscaping, UnescapingShellUnixString) {
//   EXPECT_EQ("hello"s,           to_utf8(mtx::gui::Util::unescape(u"hello"_s,               mtx::gui::Util::EscapeShellUnix)));
//   EXPECT_EQ("hello_world"s,     to_utf8(mtx::gui::Util::unescape(u"hello_world"_s,         mtx::gui::Util::EscapeShellUnix)));
//   EXPECT_EQ("hello world"s,     to_utf8(mtx::gui::Util::unescape(u"'hello world'"_s,       mtx::gui::Util::EscapeShellUnix)));
//   EXPECT_EQ("\"hello world\""s, to_utf8(mtx::gui::Util::unescape(u"'\"hello world\"'"_s,   mtx::gui::Util::EscapeShellUnix)));
//   EXPECT_EQ("'hello world'"s,   to_utf8(mtx::gui::Util::unescape(u"\\''hello world'\\'"_s, mtx::gui::Util::EscapeShellUnix)));
//   EXPECT_EQ("`hello`"s,         to_utf8(mtx::gui::Util::unescape(u"'`hello`'"_s,           mtx::gui::Util::EscapeShellUnix)));
// }

// TEST(GuiStringEscaping, UnescapingShellUnixStringList) {
//   EXPECT_EQ("hello"s,           to_utf8(mtx::gui::Util::unescape(QStringList{u"hello"_s},               mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
//   EXPECT_EQ("hello_world"s,     to_utf8(mtx::gui::Util::unescape(QStringList{u"hello_world"_s},         mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
//   EXPECT_EQ("hello world"s,     to_utf8(mtx::gui::Util::unescape(QStringList{u"'hello world'"_s},       mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
//   EXPECT_EQ("\"hello world\""s, to_utf8(mtx::gui::Util::unescape(QStringList{u"'\"hello world\"'"_s},   mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
//   EXPECT_EQ("'hello world'"s,   to_utf8(mtx::gui::Util::unescape(QStringList{u"\\''hello world'\\'"_s}, mtx::gui::Util::EscapeShellUnix).join(u" "_s)));

//   EXPECT_EQ("hello world"s,                    to_utf8(mtx::gui::Util::unescape(QStringList{u"hello"_s,               u"world"_s},                mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
//   EXPECT_EQ("hello world"s,                    to_utf8(mtx::gui::Util::unescape(QStringList{u"'hello world'"_s},                                  mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
//   EXPECT_EQ("\"hello world\" \"same stuff\""s, to_utf8(mtx::gui::Util::unescape(QStringList{u"'\"hello world\"'"_s,   u"'\"same stuff\"'"_s,},    mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
//   EXPECT_EQ("'hello world' 'same stuff'"s,     to_utf8(mtx::gui::Util::unescape(QStringList{u"\\''hello world'\\'"_s, u"\\''same stuff'\\'"_s},   mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
//   EXPECT_EQ("`evil` more`evil`thanthis"s,      to_utf8(mtx::gui::Util::unescape(QStringList{u"'`evil`'"_s,            u"'more`evil`thanthis'"_s}, mtx::gui::Util::EscapeShellUnix).join(u" "_s)));
// }

// TEST(GuiStringEscaping, UnescapingShellCmdExeProgramString) {
//   EXPECT_EQ("hello"s,         to_utf8(mtx::gui::Util::unescape(u"hello"_s,             mtx::gui::Util::EscapeShellCmdExeProgram)));
//   EXPECT_EQ("hello_world"s,   to_utf8(mtx::gui::Util::unescape(u"hello_world"_s,       mtx::gui::Util::EscapeShellCmdExeProgram)));
//   EXPECT_EQ("hello world"s,   to_utf8(mtx::gui::Util::unescape(u"\"hello world\""_s,   mtx::gui::Util::EscapeShellCmdExeProgram)));
//   EXPECT_EQ("'hello world'"s, to_utf8(mtx::gui::Util::unescape(u"\"'hello world'\""_s, mtx::gui::Util::EscapeShellCmdExeProgram)));
//   EXPECT_EQ("`hello`"s,       to_utf8(mtx::gui::Util::unescape(u"\"`hello`\""_s,       mtx::gui::Util::EscapeShellCmdExeProgram)));
// }

// TEST(GuiStringEscaping, UnescapingShellCmdExeArgumentString) {
//   EXPECT_EQ("hello"s,           to_utf8(mtx::gui::Util::unescape(u"hello"_s,                       mtx::gui::Util::EscapeShellCmdExeArgument)));
//   EXPECT_EQ("hello_world"s,     to_utf8(mtx::gui::Util::unescape(u"hello_world"_s,                 mtx::gui::Util::EscapeShellCmdExeArgument)));
//   EXPECT_EQ("hello world"s,     to_utf8(mtx::gui::Util::unescape(u"^\"hello world^\""_s,           mtx::gui::Util::EscapeShellCmdExeArgument)));
//   EXPECT_EQ("\"hello world\""s, to_utf8(mtx::gui::Util::unescape(u"^\"\\^\"hello world\\^\"^\""_s, mtx::gui::Util::EscapeShellCmdExeArgument)));
//   EXPECT_EQ("'hello world'"s,   to_utf8(mtx::gui::Util::unescape(u"^\"'hello world'^\""_s,         mtx::gui::Util::EscapeShellCmdExeArgument)));
//   EXPECT_EQ("`hello`"s,         to_utf8(mtx::gui::Util::unescape(u"^\"`hello`^\""_s,               mtx::gui::Util::EscapeShellCmdExeArgument)));
// }

}
