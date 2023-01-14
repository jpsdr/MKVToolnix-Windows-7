#include "common/common_pch.h"

#include <unordered_map>

#include "common/bluray/util.h"
#include "common/debugging.h"
#include "common/id_info.h"
#include "common/mm_file_io.h"
#include "common/mm_mpls_multi_file_io.h"
#include "common/mm_mpls_multi_file_io_p.h"
#include "common/path.h"
#include "common/strings/formatting.h"

namespace {
debugging_option_c s_debug{"mpls|mpls_multi_io"};
}

mm_mpls_multi_file_io_c::mm_mpls_multi_file_io_c(std::vector<boost::filesystem::path> const &file_names,
                                                 std::string const &display_file_name,
                                                 mtx::bluray::mpls::parser_cptr const &mpls_parser)
  : mm_file_io_c{*new mm_mpls_multi_file_io_private_c{file_names, display_file_name, mpls_parser}}
{
}

mm_mpls_multi_file_io_c::mm_mpls_multi_file_io_c(mm_mpls_multi_file_io_private_c &p)
  : mm_file_io_c{p}
{
}

mm_mpls_multi_file_io_c::~mm_mpls_multi_file_io_c() { // NOLINT(modernize-use-equals-default) due to pimpl idiom requiring explicit dtor declaration somewhere
}

std::vector<mtx::bluray::mpls::chapter_t> const &
mm_mpls_multi_file_io_c::get_chapters()
  const {
  return p_func()->mpls_parser->get_chapters();
}

mm_io_cptr
mm_mpls_multi_file_io_c::open_multi(std::string const &display_file_name) {
  try {
    mm_file_io_c in{display_file_name};
    return open_multi(in);
  } catch (mtx::mm_io::exception &) {
    return mm_io_cptr{};
  }
}

mm_io_cptr
mm_mpls_multi_file_io_c::open_multi(mm_io_c &in) {
  auto mpls_parser = std::make_shared<mtx::bluray::mpls::parser_c>();

  if (!mpls_parser->parse(in) || mpls_parser->get_playlist().items.empty()) {
    mxdebug_if(s_debug, fmt::format("Not handling because {0}\n", mpls_parser->is_ok() ? "playlist is empty" : "parser not OK"));
    return mm_io_cptr{};
  }

  std::vector<boost::filesystem::path> file_names;

  for (auto const &item : mpls_parser->get_playlist().items) {
    auto file = mtx::bluray::find_other_file(mtx::fs::to_path(in.get_file_name()), mtx::fs::to_path("STREAM") / mtx::fs::to_path(fmt::format("{0}.{1}", item.clip_id, balg::to_lower_copy(item.codec_id))));
    if (file.empty())
      file = mtx::bluray::find_other_file(mtx::fs::to_path(in.get_file_name()), mtx::fs::to_path("STREAM") / mtx::fs::to_path(fmt::format("{0}.{1}", item.clip_id, "m2ts")));

    if (!file.empty())
      file_names.push_back(file);

    mxdebug_if(s_debug, fmt::format("Item clip ID: {0} codec ID: {1}: have file? {2} file: {3}\n", item.clip_id, item.codec_id, !file.empty(), file.string()));
  }

  mxdebug_if(s_debug, fmt::format("Number of files left: {0}\n", file_names.size()));

  if (file_names.empty())
    return mm_io_cptr{};

  return mm_io_cptr{new mm_mpls_multi_file_io_c{file_names, file_names[0].string(), mpls_parser}};
}

void
mm_mpls_multi_file_io_c::create_verbose_identification_info(mtx::id::info_c &info) {
  auto p = p_func();

  info.add(mtx::id::playlist,          true);
  info.add(mtx::id::playlist_duration, p->mpls_parser->get_playlist().duration.to_ns());
  info.add(mtx::id::playlist_size,     p->total_size);
  info.add(mtx::id::playlist_chapters, p->mpls_parser->get_chapters().size());

  auto file_names = nlohmann::json::array();
  for (auto &file : p->files)
    file_names.push_back(file.string());

  info.add(mtx::id::playlist_file, file_names);
}

std::string
mm_mpls_multi_file_io_c::get_file_name()
  const {
  return p_func()->display_file_name;
}

std::vector<boost::filesystem::path> const &
mm_mpls_multi_file_io_c::get_file_names()
  const {
  return p_func()->files;
}

mtx::bluray::mpls::parser_c const &
mm_mpls_multi_file_io_c::get_mpls_parser()
  const {
  return *p_func()->mpls_parser;
}
