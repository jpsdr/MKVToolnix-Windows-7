/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <QRegularExpression>

#include "common/id_info.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "common/mm_multi_file_io.h"
#include "common/mm_multi_file_io_p.h"
#include "common/output.h"
#include "common/path.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"

mm_multi_file_io_c::mm_multi_file_io_c(std::vector<boost::filesystem::path> const &file_names,
                                       std::string const &display_file_name)
  : mm_io_c{*new mm_multi_file_io_private_c{file_names, display_file_name}}
{
}

mm_multi_file_io_c::mm_multi_file_io_c(mm_multi_file_io_private_c &p)
  : mm_io_c{p}
{
}

mm_multi_file_io_c::~mm_multi_file_io_c() {
  close_multi_file_io();
}

uint64_t
mm_multi_file_io_c::getFilePointer() {
  return p_func()->current_pos;
}

void
mm_multi_file_io_c::setFilePointer(int64_t offset,
                                   libebml::seek_mode mode) {
  auto p = p_func();

  int64_t new_pos
    = libebml::seek_beginning == mode ? offset
    : libebml::seek_end       == mode ? p->total_size  + offset // offsets from the end are negative already
    :                                   p->current_pos + offset;

  if ((0 > new_pos) || (static_cast<int64_t>(p->total_size) < new_pos))
    throw mtx::mm_io::seek_x();

  p->current_file = 0;
  for (auto &file : p->files) {
    if ((file.global_start + file.size) < static_cast<uint64_t>(new_pos)) {
      ++p->current_file;
      continue;
    }

    p->current_pos       = new_pos;
    p->current_local_pos = new_pos - file.global_start;
    file.file->setFilePointer(p->current_local_pos);
    break;
  }
}

uint32_t
mm_multi_file_io_c::_read(void *buffer,
                          size_t size) {
  auto p                = p_func();

  size_t num_read_total = 0;
  auto buffer_ptr       = static_cast<uint8_t *>(buffer);

  while (!eof() && (num_read_total < size)) {
    auto &file       = p->files[p->current_file];
    auto num_to_read = static_cast<uint64_t>(std::min(static_cast<uint64_t>(size) - static_cast<uint64_t>(num_read_total), file.size - p->current_local_pos));

    if (0 != num_to_read) {
      size_t num_read       = file.file->read(buffer_ptr, num_to_read);
      num_read_total       += num_read;
      buffer_ptr           += num_read;
      p->current_local_pos += num_read;
      p->current_pos       += num_read;

      if (num_read != num_to_read)
        break;
    }

    if ((p->current_local_pos >= file.size) && (p->files.size() > (p->current_file + 1))) {
      ++p->current_file;
      p->current_local_pos = 0;
      p->files[p->current_file].file->setFilePointer(0);
    }
  }

  return num_read_total;
}

size_t
mm_multi_file_io_c::_write(const void *,
                           size_t) {
  throw mtx::mm_io::wrong_read_write_access_x();
}

void
mm_multi_file_io_c::close() {
  close_multi_file_io();
}

void
mm_multi_file_io_c::close_multi_file_io() {
  auto p = p_func();

  for (auto &file : p->files)
    file.file->close();

  p->files.clear();
  p->total_size        = 0;
  p->current_pos       = 0;
  p->current_local_pos = 0;
}

bool
mm_multi_file_io_c::eof() {
  auto p = p_func();

  return p->files.empty() || ((p->current_file == (p->files.size() - 1)) && (p->current_local_pos >= p->files[p->current_file].size));
}

std::vector<boost::filesystem::path>
mm_multi_file_io_c::get_file_names() {
  auto p = p_func();

  std::vector<boost::filesystem::path> file_names;
  for (auto &file : p->files)
    file_names.push_back(file.file_name);

  return file_names;
}

void
mm_multi_file_io_c::create_verbose_identification_info(mtx::id::info_c &info) {
  auto p          = p_func();
  auto file_names = nlohmann::json::array();
  for (auto &file : p->files)
    if (file.file_name != p->files.front().file_name)
    file_names.push_back(file.file_name.string());

  info.add(mtx::id::other_file, file_names);
}

void
mm_multi_file_io_c::display_other_file_info() {
  auto p = p_func();

  std::stringstream out;

  for (auto &file : p->files)
    if (file.file_name != p->files.front().file_name) {
      if (!out.str().empty())
        out << ", ";
      out << file.file_name.filename();
    }

  if (!out.str().empty())
    mxinfo(fmt::format(FY("'{0}': Processing the following files as well: {1}\n"), p->display_file_name, out.str()));
}

void
mm_multi_file_io_c::enable_buffering(bool enable) {
  auto p = p_func();

  for (auto &file : p->files)
    file.file->enable_buffering(enable);
}

std::string
mm_multi_file_io_c::get_file_name()
  const {
  return p_func()->display_file_name;
}

mm_io_cptr
mm_multi_file_io_c::open_multi(const std::string &display_file_name,
                               bool single_only) {
  auto first_file_name = boost::filesystem::absolute(mtx::fs::to_path(display_file_name));
  auto base_name       = first_file_name.stem().string();
  auto extension       = balg::to_lower_copy(first_file_name.extension().string());

  QRegularExpression file_name_re{"(.+[_\\-])(\\d+)$"};
  auto matches = file_name_re.match(Q(base_name));

  if (!matches.hasMatch() || single_only) {
    std::vector<boost::filesystem::path> file_names;
    file_names.push_back(first_file_name);
    return mm_io_cptr(new mm_multi_file_io_c(file_names, display_file_name));
  }

  int start_number = 1;
  mtx::string::parse_number(to_utf8(matches.captured(2)), start_number);

  base_name = to_utf8(matches.captured(1).toLower());

  std::vector<std::pair<int, boost::filesystem::path>> paths;
  paths.emplace_back(start_number, first_file_name);

  boost::filesystem::directory_iterator end_itr;
  for (boost::filesystem::directory_iterator itr(first_file_name.parent_path()); itr != end_itr; ++itr) {
    if (   boost::filesystem::is_directory(itr->status())
        || !balg::iequals(itr->path().extension().string(), extension))
      continue;

    auto stem          = itr->path().stem().string();
    int current_number = 0;
    matches            = file_name_re.match(Q(stem));

    if (   !matches.hasMatch()
        || (to_utf8(matches.captured(1).toLower()) != base_name)
        || !mtx::string::parse_number(to_utf8(matches.captured(2)), current_number)
        || (current_number <= start_number))
      continue;

    paths.emplace_back(current_number, itr->path());
  }

  std::sort(paths.begin(), paths.end());

  std::vector<boost::filesystem::path> file_names;
  for (auto &path : paths)
    file_names.emplace_back(std::get<1>(path));

  return mm_io_cptr(new mm_multi_file_io_c(file_names, display_file_name));
}
