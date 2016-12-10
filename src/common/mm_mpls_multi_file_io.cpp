#include "common/common_pch.h"

#include <unordered_map>

#include "common/debugging.h"
#include "common/file.h"
#include "common/id_info.h"
#include "common/mm_mpls_multi_file_io.h"
#include "common/strings/formatting.h"

debugging_option_c mm_mpls_multi_file_io_c::ms_debug{"mpls|mpls_multi_io"};

mm_mpls_multi_file_io_c::mm_mpls_multi_file_io_c(std::vector<bfs::path> const &file_names,
                                                 std::string const &display_file_name,
                                                 mtx::bluray::mpls::parser_cptr const &mpls_parser)
  : mm_file_io_c{file_names[0].string()}
  , m_files{file_names}
  , m_display_file_name{display_file_name}
  , m_mpls_parser{mpls_parser}
  , m_total_size{ boost::accumulate(m_files, 0ull, [](uint64_t accu, bfs::path const &file) { return accu + bfs::file_size(file); }) }
{
}

mm_mpls_multi_file_io_c::~mm_mpls_multi_file_io_c() {
}

std::vector<timestamp_c> const &
mm_mpls_multi_file_io_c::get_chapters()
  const {
  return m_mpls_parser->get_chapters();
}

mm_io_cptr
mm_mpls_multi_file_io_c::open_multi(std::string const &display_file_name) {
  try {
    mm_file_io_c in{display_file_name};
    return open_multi(&in);
  } catch (mtx::mm_io::exception &) {
    return mm_io_cptr{};
  }
}

mm_io_cptr
mm_mpls_multi_file_io_c::open_multi(mm_io_c *in) {
  auto mpls_parser = std::make_shared<mtx::bluray::mpls::parser_c>();

  if (!mpls_parser->parse(in) || mpls_parser->get_playlist().items.empty()) {
    mxdebug_if(ms_debug, boost::format("Not handling because %1%\n") % (mpls_parser->is_ok() ? "playlist is empty" : "parser not OK"));
    return mm_io_cptr{};
  }

  auto mpls_dir = bfs::system_complete(bfs::path(in->get_file_name())).remove_filename();

  std::vector<bfs::path> file_names;

  for (auto const &item : mpls_parser->get_playlist().items) {
    auto basename_upper = (boost::format("%1%.%2%") % item.clip_id % balg::to_upper_copy(item.codec_id)).str();
    auto basename_lower = (boost::format("%1%.%2%") % item.clip_id % balg::to_lower_copy(item.codec_id)).str();

    auto file = mtx::file::first_existing_path({
        mpls_dir / ".." / "STREAM" / basename_lower, mpls_dir / ".." / ".." / "STREAM" / basename_lower,
        mpls_dir / ".." / "STREAM" / basename_upper, mpls_dir / ".." / ".." / "STREAM" / basename_upper,
        mpls_dir / ".." / "stream" / basename_lower, mpls_dir / ".." / ".." / "stream" / basename_lower,
        mpls_dir / ".." / "stream" / basename_upper, mpls_dir / ".." / ".." / "stream" / basename_upper,
      });

    mxdebug_if(ms_debug, boost::format("Item clip ID: %1% codec ID: %2%: have file? %3% file: %4%\n") % item.clip_id % item.codec_id % !file.empty() % file.string());
    if (!file.empty())
      file_names.push_back(file);
  }

  mxdebug_if(ms_debug, boost::format("Number of files left: %1%\n") % file_names.size());

  if (file_names.empty())
    return mm_io_cptr{};

  return mm_io_cptr{new mm_mpls_multi_file_io_c{file_names, file_names[0].string(), mpls_parser}};
}

void
mm_mpls_multi_file_io_c::create_verbose_identification_info(mtx::id::info_c &info) {
  info.add(mtx::id::playlist,          true);
  info.add(mtx::id::playlist_duration, m_mpls_parser->get_playlist().duration.to_ns());
  info.add(mtx::id::playlist_size,     m_total_size);
  info.add(mtx::id::playlist_chapters, m_mpls_parser->get_chapters().size());

  auto file_names = nlohmann::json::array();
  for (auto &file : m_files)
    file_names.push_back(file.string());

  info.add(mtx::id::playlist_file, file_names);
}
