/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   extracts attachments from Matroska files into other files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include <cassert>
#include <iostream>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxSegment.h>

#include "common/ebml.h"
#include "common/kax_analyzer.h"
#include "common/mm_io_x.h"
#include "common/mm_file_io.h"
#include "extract/mkvextract.h"

struct attachment_t {
  std::string name, type;
  uint64_t size, id;
  libmatroska::KaxFileData *fdata;
  bool valid;

  attachment_t()
    : size{std::numeric_limits<uint64_t>::max()}
    , id{}
    , fdata(nullptr)
    , valid(false)
  {
  };

  attachment_t &parse(libmatroska::KaxAttached &att);
  static attachment_t parse_new(libmatroska::KaxAttached &att);
};

attachment_t
attachment_t::parse_new(libmatroska::KaxAttached &att) {
  attachment_t attachment;
  return attachment.parse(att);
}

attachment_t &
attachment_t::parse(libmatroska::KaxAttached &att) {
  size_t k;
  for (k = 0; att.ListSize() > k; ++k) {
    libebml::EbmlElement *e = att[k];

    if (is_type<libmatroska::KaxFileName>(e))
      name = static_cast<libmatroska::KaxFileName *>(e)->GetValueUTF8();

    else if (is_type<libmatroska::KaxMimeType>(e))
      type = static_cast<libmatroska::KaxMimeType *>(e)->GetValue();

    else if (is_type<libmatroska::KaxFileUID>(e))
      id = static_cast<libmatroska::KaxFileUID *>(e)->GetValue();

    else if (is_type<libmatroska::KaxFileData>(e)) {
      fdata = static_cast<libmatroska::KaxFileData *>(e);
      size  = fdata->GetSize();
    }
  }

  valid = (std::numeric_limits<uint64_t>::max() != size) && !type.empty();

  return *this;
}

static void
handle_attachments(libmatroska::KaxAttachments *atts,
                   std::vector<track_spec_t> &tracks) {
  int64_t attachment_ui_id = 0;
  std::map<int64_t, attachment_t> attachments;

  size_t i;
  for (i = 0; atts->ListSize() > i; ++i) {
    libmatroska::KaxAttached *att = dynamic_cast<libmatroska::KaxAttached *>((*atts)[i]);
    assert(att);

    attachment_t attachment = attachment_t::parse_new(*att);
    if (!attachment.valid)
      continue;

    ++attachment_ui_id;
    attachments[attachment_ui_id] = attachment;
  }

  for (auto &track : tracks) {
    attachment_t attachment = attachments[ track.tid ];

    if (!attachment.valid)
      mxerror(fmt::format(FY("An attachment with the ID {0} was not found.\n"), track.tid));

    // check for output name
    if (track.out_name.empty())
      track.out_name = attachment.name;

    mxinfo(fmt::format(FY("The attachment #{0}, ID {1}, MIME type {2}, size {3}, is written to '{4}'.\n"),
                       track.tid, attachment.id, attachment.type, attachment.size, track.out_name));
    try {
      mm_file_io_c out(track.out_name, libebml::MODE_CREATE);
      out.write(attachment.fdata->GetBuffer(), attachment.fdata->GetSize());
    } catch (mtx::mm_io::exception &ex) {
      mxerror(fmt::format(FY("The file '{0}' could not be opened for writing: {1}.\n"), track.out_name, ex));
    }
  }
}

bool
extract_attachments(kax_analyzer_c &analyzer,
                    options_c::mode_options_c &options) {
  if (options.m_tracks.empty())
    return false;

  auto attachments = analyzer.read_all(EBML_INFO(libmatroska::KaxAttachments));
  if (!dynamic_cast<libmatroska::KaxAttachments *>(attachments.get()))
    return false;

  handle_attachments(static_cast<libmatroska::KaxAttachments *>(attachments.get()), options.m_tracks);

  return true;
}
