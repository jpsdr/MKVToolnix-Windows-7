/*
   mkvpropedit -- utility for editing properties of existing Matroska files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include <boost/optional.hpp>

#include <matroska/KaxAttached.h>

#include "propedit/target.h"
#include "propedit/target_id_manager.h"

using namespace libebml;
using namespace libmatroska;

using attachment_id_manager_c    = target_id_manager_c<KaxAttached>;
using attachment_id_manager_cptr = std::shared_ptr<attachment_id_manager_c>;

class attachment_target_c: public target_c {
public:
  struct options_t {
    boost::optional<std::string> m_name, m_description, m_mime_type;
    boost::optional<uint64_t> m_uid;

    options_t &
    name(std::string const &p_name) {
      m_name.reset(p_name);
      return *this;
    }

    options_t &
    description(std::string const &p_description) {
      m_description.reset(p_description);
      return *this;
    }

    options_t &
    mime_type(std::string const &p_mime_type) {
      m_mime_type.reset(p_mime_type);
      return *this;
    }

    options_t &
    uid(uint64_t p_uid) {
      m_uid.reset(p_uid);
      return *this;
    }
  };

  enum command_e {
    ac_add,
    ac_delete,
    ac_replace,
    ac_update,
  };

  enum selector_type_e {
    st_id,
    st_uid,
    st_name,
    st_mime_type,
  };

protected:
  std::string m_spec;
  command_e m_command;
  options_t m_options;
  selector_type_e m_selector_type;
  uint64_t m_selector_num_arg;
  std::string m_selector_string_arg;
  memory_cptr m_file_content;
  attachment_id_manager_cptr m_id_manager;
  bool m_attachments_modified;

public:
  attachment_target_c();
  virtual ~attachment_target_c() override;

  virtual void set_id_manager(attachment_id_manager_cptr const &id_manager);

  virtual void validate() override;

  virtual bool operator ==(target_c const &cmp) const override;

  virtual void parse_spec(command_e command, const std::string &spec, options_t const &options);
  virtual void dump_info() const override;

  virtual bool has_changes() const override;
  virtual bool has_content_been_modified() const override;

  virtual void execute() override;

protected:
  virtual void execute_add();
  virtual void execute_delete();
  virtual void execute_replace();

  virtual bool delete_by_id();
  virtual bool delete_by_uid_name_mime_type();

  virtual bool replace_by_id();
  virtual bool replace_by_uid_name_mime_type();
  virtual void replace_attachment_values(KaxAttached &att);

  virtual bool matches_by_uid_name_or_mime_type(KaxAttached &att);
};

inline bool
operator ==(attachment_target_c::options_t const &a,
            attachment_target_c::options_t const &b) {
  return (a.m_name        == b.m_name)
      && (a.m_description == b.m_description)
      && (a.m_mime_type   == b.m_mime_type);
}

inline std::ostream &
operator <<(std::ostream &out,
            attachment_target_c::options_t const &opt) {
  auto format = [](std::string const &name, boost::optional<std::string> const &value) -> std::string {
    return value ? name + ":yes(" + *value + ")" : name + ":no";
  };

  out << "{" << format("name", opt.m_name) << " " << format("description", opt.m_description) << " " << format("MIME type", opt.m_mime_type) << "}";

  return out;
}
