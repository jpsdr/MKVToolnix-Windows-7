/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#include "common/common_pch.h"

#include "common/vobsub.h"

namespace mtx::vobsub {

std::string
create_default_index(unsigned int width,
                     unsigned int height,
                     std::string const &palette) {
  auto palette_to_use = palette.empty() ? "bebebe, 171717, 5f5f5f, e7e7e7, 828282, 828282, 828282, 828282, 828282, 828282, 828282, 828282, bebebe, 000000, f6f6f6, 828282"s : palette;

  return fmt::format("# VobSub index file, v7 (do not modify this line!)\n"
                     "#\n"
                     "# To repair desyncronization, you can insert gaps this way:\n"
                     "# (it usually happens after vob id changes)\n"
                     "#\n"
                     "#        delay: [sign]hh:mm:ss:ms\n"
                     "#\n"
                     "# Where:\n"
                     "#        [sign]: +, - (optional)\n"
                     "#        hh: hours (0 <= hh)\n"
                     "#        mm/ss: minutes/seconds (0 <= mm/ss <= 59)\n"
                     "#        ms: milliseconds (0 <= ms <= 999)\n"
                     "#\n"
                     "#        Note: You can't position a sub before the previous with a negative value.\n"
                     "#\n"
                     "# You can also modify timestamps or delete a few subs you don't like.\n"
                     "# Just make sure they stay in increasing order.\n"
                     "\n"
                     "\n"
                     "# Settings\n"
                     "\n"
                     "# Original frame size\n"
                     "size: {0}x{1}\n"
                     "\n"
                     "# Origin, relative to the upper-left corner, can be overloaded by aligment\n"
                     "org: 0, 0\n"
                     "\n"
                     "# Image scaling (hor,ver), origin is at the upper-left corner or at the alignment coord (x, y)\n"
                     "scale: 100%, 100%\n"
                     "\n"
                     "# Alpha blending\n"
                     "alpha: 100%\n"
                     "\n"
                     "# Smoothing for very blocky images (use OLD for no filtering)\n"
                     "smooth: OFF\n"
                     "\n"
                     "# In millisecs\n"
                     "fadein/out: 50, 50\n"
                     "\n"
                     "# Force subtitle placement relative to (org.x, org.y)\n"
                     "align: OFF at LEFT TOP\n"
                     "\n"
                     "# For correcting non-progressive desync. (in millisecs or hh:mm:ss:ms)\n"
                     "# Note: Not effective in DirectVobSub, use \"delay: ... \" instead.\n"
                     "time offset: 0\n"
                     "\n"
                     "# ON: displays only forced subtitles, OFF: shows everything\n"
                     "forced subs: OFF\n"
                     "\n"
                     "# The original palette of the DVD\n"
                     "palette: {2}\n"
                     "\n"
                     "# Custom colors (transp idxs and the four colors)\n"
                     "custom colors: OFF, tridx: 0000, colors: 000000, 000000, 000000, 000000\n",
                     width, height, palette_to_use);
}

}
