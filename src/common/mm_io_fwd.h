/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   IO callback class definitions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

enum class byte_order_mark_e {utf8, utf16_le, utf16_be, utf32_le, utf32_be, none};

class mm_io_c;
using mm_io_cptr = std::shared_ptr<mm_io_c>;

class mm_file_io_c;
using mm_file_io_cptr = std::shared_ptr<mm_file_io_c>;

class mm_mem_io_c;
using mm_mem_io_cptr = std::shared_ptr<mm_mem_io_c>;

class mm_mpls_multi_file_io_c;
using mm_mpls_multi_file_io_cptr = std::shared_ptr<mm_mpls_multi_file_io_c>;

class mm_multi_file_io_c;
using mm_multi_file_io_cptr = std::shared_ptr<mm_multi_file_io_c>;

class mm_null_io_c;
using mm_null_io_cptr = std::shared_ptr<mm_null_io_c>;

class mm_proxy_io_c;
using mm_proxy_io_cptr = std::shared_ptr<mm_proxy_io_c>;

class mm_read_buffer_io_c;
using mm_read_buffer_io_cptr = std::shared_ptr<mm_read_buffer_io_c>;

class mm_stdio_c;
using mm_stdio_cptr = std::shared_ptr<mm_stdio_c>;

class mm_text_io_c;
using mm_text_io_cptr = std::shared_ptr<mm_text_io_c>;

class mm_write_buffer_io_c;
using mm_write_buffer_io_cptr = std::shared_ptr<mm_write_buffer_io_c>;
