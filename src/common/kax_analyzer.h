/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   quick Matroska file parsing

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <ebml/EbmlHead.h>
#include <matroska/KaxSegment.h>

#include "common/ebml.h"
#include "common/mm_io.h"

using namespace libebml;
using namespace libmatroska;

namespace mtx { namespace bits {
class value_c;
using value_cptr = std::shared_ptr<value_c>;
}}

class kax_analyzer_data_c;
using kax_analyzer_data_cptr = std::shared_ptr<kax_analyzer_data_c>;

class kax_analyzer_data_c {
public:
  EbmlId m_id;
  uint64_t m_pos;
  int64_t m_size;
  bool m_size_known;

public:                         // Static functions
  static kax_analyzer_data_cptr create(const EbmlId id, uint64_t pos, int64_t size, bool size_known = true) {
    return std::make_shared<kax_analyzer_data_c>(id, pos, size, size_known);
  }

public:
  kax_analyzer_data_c(const EbmlId id, uint64_t pos, int64_t size, bool size_known)
    : m_id{id}
    , m_pos{pos}
    , m_size{size}
    , m_size_known{size_known}
  {
  }

  std::string to_string() const;
};

bool operator <(const kax_analyzer_data_cptr &d1, const kax_analyzer_data_cptr &d2);

namespace mtx {
  class kax_analyzer_x: public exception {
  protected:
    std::string m_message;
  public:
    kax_analyzer_x(const std::string &message)  : m_message(message)       { }
    kax_analyzer_x(const boost::format &message): m_message(message.str()) { }
    virtual ~kax_analyzer_x() throw() { }

    virtual const char *what() const throw() {
      return m_message.c_str();
    }
  };
}

class kax_analyzer_c {
public:
  enum update_element_result_e {
    uer_success,
    uer_error_segment_size_for_element,
    uer_error_segment_size_for_meta_seek,
    uer_error_meta_seek,
    uer_error_not_indexable,
    uer_error_opening_for_reading,
    uer_error_opening_for_writing,
    uer_error_fixing_last_element_unknown_size_failed,
    uer_error_unknown,
  };

  enum parse_mode_e {
    parse_mode_fast,
    parse_mode_full,
  };

  enum placement_strategy_e {
    ps_anywhere,
    ps_end,
  };

private:
  std::vector<kax_analyzer_data_cptr> m_data;
  std::string m_file_name;
  mm_io_c *m_file{};
  bool m_close_file{true};
  std::shared_ptr<KaxSegment> m_segment;
  std::shared_ptr<EbmlHead> m_ebml_head;
  uint64_t m_segment_end{};
  std::map<int64_t, bool> m_meta_seeks_by_position;
  EbmlStream *m_stream{};
  debugging_option_c m_debug{"kax_analyzer"};
  parse_mode_e m_parse_mode{parse_mode_full};
  open_mode m_open_mode{MODE_WRITE};
  bool m_throw_on_error{};
  boost::optional<uint64_t> m_parser_start_position;
  bool m_is_webm{};

public:                         // Static functions
  static bool probe(std::string file_name);

public:
  kax_analyzer_c(std::string file_name);
  kax_analyzer_c(mm_io_c *file);
  virtual ~kax_analyzer_c();

  virtual update_element_result_e update_element(EbmlElement *e, bool write_defaults = false, bool add_mandatory_elements_if_missing = true);
  virtual update_element_result_e update_element(ebml_element_cptr const &e, bool write_defaults = false, bool add_mandatory_elements_if_missing = true);

  virtual update_element_result_e remove_elements(EbmlId const &id);

  virtual ebml_master_cptr read_all(const EbmlCallbacks &callbacks);
  virtual ebml_element_cptr read_element(kax_analyzer_data_c const &element_data);
  virtual ebml_element_cptr read_element(kax_analyzer_data_cptr const &element_data);
  virtual ebml_element_cptr read_element(unsigned int pos);

  virtual void with_elements(const EbmlId &id, std::function<void(kax_analyzer_data_c const &)> worker) const;
  virtual int find(EbmlId const &id);

  virtual EbmlHead &get_ebml_head();
  virtual bool is_webm() const;

  virtual uint64_t get_segment_pos() const;
  virtual uint64_t get_segment_data_start_pos() const;

  virtual kax_analyzer_c &set_parse_mode(parse_mode_e parse_mode);
  virtual kax_analyzer_c &set_open_mode(open_mode mode);
  virtual kax_analyzer_c &set_throw_on_error(bool throw_on_error);
  virtual kax_analyzer_c &set_parser_start_position(uint64_t position);

  virtual bool process();

  virtual void show_progress_start(int64_t /* size */) {
  }
  virtual bool show_progress_running(int /* percentage */) {
    return true;
  }
  virtual void show_progress_done() {
  }

  virtual void log_debug_message(const std::string &message) {
    _log_debug_message(message);
  }
  virtual void log_debug_message(const boost::format &message) {
    _log_debug_message(message.str());
  }
  virtual void debug_abort_process() {
    mxexit(1);
  }

  virtual void close_file();
  virtual void reopen_file();
  virtual void reopen_file_for_writing();
  virtual mm_io_c &get_file() {
    return *m_file;
  }

  static placement_strategy_e get_placement_strategy_for(EbmlElement *e);
  static placement_strategy_e get_placement_strategy_for(ebml_element_cptr e) {
    return get_placement_strategy_for(e.get());
  }

  static mtx::bits::value_cptr read_segment_uid_from(std::string const &file_name);

protected:
  virtual void _log_debug_message(const std::string &message);

  virtual void remove_from_meta_seeks(EbmlId id);
  virtual void overwrite_all_instances(EbmlId id);
  virtual void merge_void_elements();
  virtual void write_element(EbmlElement *e, bool write_defaults, placement_strategy_e strategy);
  virtual void add_to_meta_seek(EbmlElement *e);
  virtual std::pair<bool, int> try_adding_to_existing_meta_seek(EbmlElement *e);
  virtual void move_seek_head_to_end_and_create_new_one_at_start(EbmlElement *e, int first_seek_head_idx);
  virtual bool create_new_meta_seek_at_start(EbmlElement *e);
  virtual bool move_level1_element_before_cluster_to_end_of_file();
  virtual int ensure_front_seek_head_links_to(unsigned int seek_head_idx);

  virtual void adjust_segment_size();
  virtual bool handle_void_elements(size_t data_idx);

  virtual bool analyzer_debugging_requested(const std::string &section);
  virtual void debug_dump_elements();
  virtual void debug_dump_elements_maybe(const std::string &hook_name);
  virtual void validate_data_structures(const std::string &hook_name);
  virtual void verify_data_structures_against_file(const std::string &hook_name);

  virtual void read_all_meta_seeks();
  virtual void read_meta_seek(uint64_t pos, std::map<int64_t, bool> &positions_found);
  virtual void fix_element_sizes(uint64_t file_size);
  virtual void fix_unknown_size_for_last_level1_element();

  virtual void determine_webm();

protected:
  virtual bool process_internal();
};
using kax_analyzer_cptr = std::shared_ptr<kax_analyzer_c>;

class console_kax_analyzer_c: public kax_analyzer_c {
private:
  bool m_show_progress;
  int m_previous_percentage;

public:
  console_kax_analyzer_c(std::string file_name);
  virtual ~console_kax_analyzer_c();

  virtual void set_show_progress(bool show_progress);

  virtual void show_progress_start(int64_t size);
  virtual bool show_progress_running(int percentage);
  virtual void show_progress_done();

  virtual void debug_abort_process();
};
using console_kax_analyzer_cptr = std::shared_ptr<console_kax_analyzer_c>;
