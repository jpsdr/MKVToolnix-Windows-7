/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IO callback class definitions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

enum byte_order_e {BO_UTF8, BO_UTF16_LE, BO_UTF16_BE, BO_UTF32_LE, BO_UTF32_BE, BO_NONE};

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
