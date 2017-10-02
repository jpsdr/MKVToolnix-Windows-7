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

class mm_mpls_multi_file_io_c;
using mm_mpls_multi_file_io_cptr = std::shared_ptr<mm_mpls_multi_file_io_c>;
