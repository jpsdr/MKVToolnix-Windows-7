/** \brief command line parsing

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \file

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
   \author Modified by Steve Lhomme <steve.lhomme@free.fr>.
*/

#include "common/common_pch.h"

#if defined(SYS_UNIX) || defined(SYS_APPLE)
#include <signal.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(SYS_WINDOWS)
#include <windows.h>
#endif

#include <algorithm>
#include <iostream>
#include <list>
#include <sstream>
#include <tuple>
#include <typeinfo>

#include <QRegularExpression>

#if LIBMATROSKA_VERSION < 0x020000
# include <matroska/KaxInfoData.h>
#endif
#include <matroska/KaxSegment.h>

#include "common/chapters/chapters.h"
#include "common/checksums/base.h"
#include "common/command_line.h"
#include "common/ebml.h"
#include "common/file_types.h"
#include "common/fs_sys_helpers.h"
#include "common/iso639.h"
#include "common/kax_analyzer.h"
#include "common/list_utils.h"
#include "common/mime.h"
#include "common/mm_file_io.h"
#include "common/mm_mpls_multi_file_io.h"
#include "common/qt.h"
#include "common/random.h"
#include "common/segmentinfo.h"
#include "common/split_arg_parsing.h"
#include "common/strings/formatting.h"
#include "common/strings/parsing.h"
#include "common/unique_numbers.h"
#include "common/version.h"
#include "common/webm.h"
#include "common/xml/ebml_segmentinfo_converter.h"
#include "common/xml/ebml_tags_converter.h"
#include "merge/cluster_helper.h"
#include "merge/filelist.h"
#include "merge/generic_reader.h"
#include "merge/output_control.h"
#include "merge/reader_detection_and_creation.h"
#include "merge/track_info.h"

static std::string s_split_by_chapters_arg;

/** \brief Outputs usage information
*/
static void
set_usage() {
  std::string usage_text;

  auto nl     =   "\n"s;
  usage_text  =   "";
  usage_text += Y("mkvmerge -o out [global options] [options1] <file1> [@option-file.json] …\n");
  usage_text +=   "\n";
  usage_text += Y(" Global options:\n");
  usage_text +=   "  -v, --verbose            "s + Y("Increase verbosity.") + nl;
  usage_text +=   "  -q, --quiet              "s + Y("Suppress status output.") + nl;
  usage_text += Y("  -o, --output out         Write to the file 'out'.\n");
  usage_text += Y("  -w, --webm               Create WebM compliant file.\n");
  usage_text += Y("  --title <title>          Title for this destination file.\n");
  usage_text += Y("  --global-tags <file>     Read global tags from an XML file.\n");
  usage_text +=   "\n";
  usage_text += Y(" Chapter handling:\n");
  usage_text += Y("  --chapters <file>        Read chapter information from the file.\n");
  usage_text += Y("  --chapter-language <lng> Set the 'language' element in chapter entries.\n");
  usage_text += Y("  --chapter-charset <cset> Charset for a simple chapter file.\n");
  usage_text += Y("  --chapter-sync <d[,o[/p]]>\n"
                  "                           Synchronize, adjust the chapters's timestamps\n"
                  "                           by 'd' ms.\n"
                  "                           'o/p': Adjust the timestamps by multiplying with\n"
                  "                           'o/p' to fix linear drifts. 'p' defaults to\n"
                  "                           1 if omitted. Both 'o' and 'p' can be\n"
                  "                           floating point numbers.\n");
  usage_text += Y("  --cue-chapter-name-format <format>\n"
                  "                           Pattern for the conversion from cue sheet\n"
                  "                           entries to chapter names.\n");
  usage_text += Y("  --default-language <lng> Use this language for all tracks unless\n"
                  "                           overridden with the --language option.\n");
  usage_text += Y("  --generate-chapters <mode>\n"
                  "                           Automatically generate chapters according to\n"
                  "                           the mode ('when-appending' or 'interval:<duration>').\n");
  usage_text += Y("  --generate-chapters-name-template <template>\n"
                  "                           Template for newly generated chapter names\n"
                  "                           (default: 'Chapter <NUM:2>').\n");
  usage_text +=   "\n";
  usage_text += Y(" Segment info handling:\n");
  usage_text += Y("  --segmentinfo <file>     Read segment information from the file.\n");
  usage_text += Y("  --segment-uid <SID1,[SID2...]>\n"
                  "                           Set the segment UIDs to SID1, SID2 etc.\n");
  usage_text +=   "\n";
  usage_text += Y(" General output control (advanced global options):\n");
  usage_text += Y("  --track-order <FileID1:TID1,FileID2:TID2,FileID3:TID3,...>\n"
                  "                           A comma separated list of both file IDs\n"
                  "                           and track IDs that controls the order of the\n"
                  "                           tracks in the destination file. If not given,\n"
                  "                           the tracks will sorted by type (video, audio,\n"
                  "                           subtitles, others).\n");
  usage_text += Y("  --cluster-length <n[ms]> Put at most n data blocks into each cluster.\n"
                  "                           If the number is postfixed with 'ms' then\n"
                  "                           put at most n milliseconds of data into each\n"
                  "                           cluster.\n");
  usage_text += Y("  --clusters-in-meta-seek  Write meta seek data for clusters.\n");
  usage_text += Y("  --timestamp-scale <n>    Force the timestamp scale factor to n.\n");
  usage_text += Y("  --enable-durations       Enable block durations for all blocks.\n");
  usage_text += Y("  --no-cues                Do not write the cue data (the index).\n");
  usage_text += Y("  --no-date                Do not write the 'date' field in the segment\n"
                  "                           information headers.\n");
  usage_text += Y("  --disable-lacing         Do not use lacing.\n");
  usage_text += Y("  --disable-track-statistics-tags\n"
                  "                           Do not write tags with track statistics.\n");
  usage_text += Y("  --disable-language-ietf  Do not write IETF BCP 47 language elements in\n"
                  "                           track headers, chapters and tags.\n");
  usage_text += Y("  --normalize-language-ietf <canonical|extlang|off>\n"
                  "                           Normalize all IETF BCP 47 language tags to either\n"
                  "                           their canonical or their extended language subtags\n"
                  "                           form or not at all (default: canonical form).\n");
  usage_text += Y("  --stop-after-video-ends  Stops processing after the primary video track ends,\n"
                  "                           discarding any remaining packets of other tracks.\n");
  usage_text +=   "\n";
  usage_text += Y(" File splitting, linking, appending and concatenating (more global options):\n");
  usage_text += Y("  --split <d[K,M,G]|HH:MM:SS|s>\n"
                  "                           Create a new file after d bytes (KB, MB, GB)\n"
                  "                           or after a specific time.\n");
  usage_text += Y("  --split timestamps:A[,B...]\n"
                  "                           Create a new file after each timestamp A, B\n"
                  "                           etc.\n");
  usage_text += Y("  --split parts:start1-end1[,[+]start2-end2,...]\n"
                  "                           Keep ranges of timestamps start-end, either in\n"
                  "                           separate files or append to previous range's file\n"
                  "                           if prefixed with '+'.\n");
  usage_text += Y("  --split parts-frames:start1-end1[,[+]start2-end2,...]\n"
                  "                           Same as 'parts:', but 'startN'/'endN' are frame/\n"
                  "                           field numbers instead of timestamps.\n");
  usage_text += Y("  --split frames:A[,B...]\n"
                  "                           Create a new file after each frame/field A, B\n"
                  "                           etc.\n");
  usage_text += Y("  --split chapters:all|A[,B...]\n"
                  "                           Create a new file before each chapter (with 'all')\n"
                  "                           or before chapter numbers A, B etc.\n");
  usage_text += Y("  --split-max-files <n>    Create at most n files.\n");
  usage_text += Y("  --link                   Link splitted files.\n");
  usage_text += Y("  --link-to-previous <SID> Link the first file to the given SID.\n");
  usage_text += Y("  --link-to-next <SID>     Link the last file to the given SID.\n");
  usage_text += Y("  --append-to <SFID1:STID1:DFID1:DTID1,SFID2:STID2:DFID2:DTID2,...>\n"
                  "                           A comma separated list of file and track IDs\n"
                  "                           that controls which track of a file is\n"
                  "                           appended to another track of the preceding\n"
                  "                           file.\n");
  usage_text += Y("  --append-mode <file|track>\n"
                  "                           Selects how mkvmerge calculates timestamps when\n"
                  "                           appending files.\n");
  usage_text += Y("  <file1> + <file2>        Append file2 to file1.\n");
  usage_text += Y("  <file1> +<file2>         Same as \"<file1> + <file2>\".\n");
  usage_text += Y("  [ <file1> <file2> ]      Same as \"<file1> + <file2>\".\n");
  usage_text += Y("  = <file>                 Don't look for and concatenate files with the same\n"
                  "                           base name but with a different trailing number.\n");
  usage_text += Y("  =<file>                  Same as \"= <file>\".\n");
  usage_text += Y("  ( <file1> <file2> )      Treat file1 and file2 as if they were concatenated\n"
                  "                           into a single big file.\n");
  usage_text +=   "\n";
  usage_text += Y(" Attachment support (more global options):\n");
  usage_text += Y("  --attachment-description <desc>\n"
                  "                           Description for the following attachment.\n");
  usage_text += Y("  --attachment-mime-type <mime type>\n"
                  "                           Mime type for the following attachment.\n");
  usage_text += Y("  --attachment-name <name> The name should be stored for the \n"
                  "                           following attachment.\n");
  usage_text += Y("  --attach-file <file>     Creates a file attachment inside the\n"
                  "                           Matroska file.\n");
  usage_text += Y("  --attach-file-once <file>\n"
                  "                           Creates a file attachment inside the\n"
                  "                           first Matroska file written.\n");
  usage_text += Y("  --enable-legacy-font-mime-types\n"
                  "                           Use legacy font MIME types when adding new\n"
                  "                           attachments as well as for existing ones.\n");
  usage_text +=   "\n";
  usage_text += Y(" Options for each source file:\n");
  usage_text += Y("  -a, --audio-tracks <n,m,...>\n"
                  "                           Copy audio tracks n,m etc. Default: copy all\n"
                  "                           audio tracks.\n");
  usage_text += Y("  -A, --no-audio           Don't copy any audio track from this file.\n");
  usage_text += Y("  -d, --video-tracks <n,m,...>\n"
                  "                           Copy video tracks n,m etc. Default: copy all\n"
                  "                           video tracks.\n");
  usage_text += Y("  -D, --no-video           Don't copy any video track from this file.\n");
  usage_text += Y("  -s, --subtitle-tracks <n,m,...>\n"
                  "                           Copy subtitle tracks n,m etc. Default: copy\n"
                  "                           all subtitle tracks.\n");
  usage_text += Y("  -S, --no-subtitles       Don't copy any subtitle track from this file.\n");
  usage_text += Y("  -b, --button-tracks <n,m,...>\n"
                  "                           Copy buttons tracks n,m etc. Default: copy\n"
                  "                           all buttons tracks.\n");
  usage_text += Y("  -B, --no-buttons         Don't copy any buttons track from this file.\n");
  usage_text += Y("  -m, --attachments <n[:all|first],m[:all|first],...>\n"
                  "                           Copy the attachments with the IDs n, m etc. to\n"
                  "                           all or only the first destination file. Default:\n"
                  "                           copy all attachments to all destination files.\n");
  usage_text += Y("  -M, --no-attachments     Don't copy attachments from a source file.\n");
  usage_text += Y("  -t, --tags <TID:file>    Read tags for the track from an XML file.\n");
  usage_text += Y("  --track-tags <n,m,...>   Copy the tags for tracks n,m etc. Default: copy\n"
                  "                           tags for all tracks.\n");
  usage_text += Y("  -T, --no-track-tags      Don't copy tags for tracks from the source file.\n");
  usage_text += Y("  --no-global-tags         Don't keep global tags from the source file.\n");
  usage_text += Y("  --no-chapters            Don't keep chapters from the source file.\n");
  usage_text += Y("  --regenerate-track-uids  Generate new random track UIDs instead of keeping\n"
                  "                           existing ones.\n");
  usage_text += Y("  -y, --sync <TID:d[,o[/p]]>\n"
                  "                           Synchronize, adjust the track's timestamps with\n"
                  "                           the id TID by 'd' ms.\n"
                  "                           'o/p': Adjust the timestamps by multiplying with\n"
                  "                           'o/p' to fix linear drifts. 'p' defaults to\n"
                  "                           1 if omitted. Both 'o' and 'p' can be\n"
                  "                           floating point numbers.\n");
  usage_text += Y("  --default-track-flag <TID[:bool]>\n"
                  "                           Sets the \"default track\" flag for this track or\n"
                  "                           forces it not to be present if bool is 0.\n");
  usage_text += Y("  --forced-display-flag <TID[:bool]>\n"
                  "                           Sets the \"forced display\" flag for this track or\n"
                  "                           forces it not to be present if bool is 0.\n");
  usage_text += Y("  --track-enabled-flag <TID[:bool]>\n"
                  "                           Sets the \"track enabled\" flag for this track or\n"
                  "                           forces it not to be present if bool is 0.\n");
  usage_text += Y("  --hearing-impaired-flag <TID[:bool]>\n"
                  "                           Sets the \"hearing impaired\" flag for this track or\n"
                  "                           forces it not to be present if bool is 0.\n");
  usage_text += Y("  --visual-impaired-flag <TID[:bool]>\n"
                  "                           Sets the \"visual impaired\" flag for this track or\n"
                  "                           forces it not to be present if bool is 0.\n");
  usage_text += Y("  --text-descriptions-flag <TID[:bool]>\n"
                  "                           Sets the \"text descriptions\" flag for this track\n"
                  "                           or forces it not to be present if bool is 0.\n");
  usage_text += Y("  --original-flag <TID[:bool]>\n"
                  "                           Sets the \"original language\" flag for this track or\n"
                  "                           forces it not to be present if bool is 0.\n");
  usage_text += Y("  --commentary-flag <TID[:bool]>\n"
                  "                           Sets the \"commentary\" flag for this track or\n"
                  "                           forces it not to be present if bool is 0.\n");
  usage_text += Y("  --track-name <TID:name>  Sets the name for a track.\n");
  usage_text += Y("  --cues <TID:none|iframes|all>\n"
                  "                           Create cue (index) entries for this track:\n"
                  "                           None at all, only for I frames, for all.\n");
  usage_text += Y("  --language <TID:lang>    Sets the language for the track (IETF BCP 47/\n"
                  "                           RFC 5646 language tag).\n");
  usage_text += Y("  --timestamps <TID:file>  Read the timestamps to be used from a file.\n");
  usage_text += Y("  --default-duration <TID:Xs|ms|us|ns|fps>\n"
                  "                           Force the default duration of a track to X.\n"
                  "                           X can be a floating point number or a fraction.\n");
  usage_text +=   "\n";
  usage_text += Y(" Options that only apply to audio tracks:\n");
  usage_text += Y("  --aac-is-sbr <TID[:0|1]> The track with the ID is HE-AAC/AAC+/SBR-AAC\n"
                  "                           or not. The value ':1' can be omitted.\n");
  usage_text += Y("  --reduce-to-core <TID>   Keeps only the core of audio tracks that support\n"
                  "                           HD extensions instead of copying both the core\n"
                  "                           and the extensions.\n");
  usage_text += Y("  --remove-dialog-normalization-gain <TID>\n"
                  "                           Removes or minimizes the dialog normalization gain\n"
                  "                           by modifying audio frame headers.\n");
  usage_text += Y("  --audio-emphasis <TID:n|keyword>\n"
                  "                           Sets the audio emphasis track header value. It can\n"
                  "                           be either a number or a keyword (see\n"
                  "                           '--list-audio-emphasis' for the full list).\n");
  usage_text +=   "\n";
  usage_text += Y(" Options that only apply to video tracks:\n");
  usage_text += Y("  --fix-bitstream-timing-information <TID[:bool]>\n"
                  "                           Adjust the frame/field rate stored in the video\n"
                  "                           bitstream to match the track's default duration.\n");
  usage_text += Y("  -f, --fourcc <FOURCC>    Forces the FourCC to the specified value.\n"
                  "                           Works only for video tracks.\n");
  usage_text += Y("  --aspect-ratio <TID:f|a/b>\n"
                  "                           Sets the display dimensions by calculating\n"
                  "                           width and height for this aspect ratio.\n");
  usage_text += Y("  --aspect-ratio-factor <TID:f|a/b>\n"
                  "                           First calculates the aspect ratio by multi-\n"
                  "                           plying the video's original aspect ratio\n"
                  "                           with this factor and calculates the display\n"
                  "                           dimensions from this factor.\n");
  usage_text += Y("  --display-dimensions <TID:width>x<height>\n"
                  "                           Explicitly set the display dimensions.\n");
  usage_text += Y("  --cropping <TID:left,top,right,bottom>\n"
                  "                           Sets the cropping parameters.\n");
  usage_text += Y("  --field-order <TID:n>    Sets the video field order parameter\n"
                  "                           (see documentation for valid values).\n");
  usage_text += Y("  --stereo-mode <TID:n|keyword>\n"
                  "                           Sets the stereo mode parameter. It can\n"
                  "                           either be a number 0 - 14 or a keyword\n"
                  "                           (use '--list-stereo-modes' to see the full list).\n");
  usage_text += Y("  --color-matrix-coefficients <TID:n>\n"
                  "                           Sets the matrix coefficients of the video used\n"
                  "                           to derive luma and chroma values from red, green\n"
                  "                           and blue color primaries.\n");
  usage_text += Y("  --color-bits-per-channel <TID:n>\n"
                  "                           Sets the number of coded bits for a color \n"
                  "                           channel. A value of 0 indicates that the number is\n"
                  "                           unspecified.\n");
  usage_text += Y("  --chroma-subsample <TID:hori,vert>\n"
                  "                           The amount of pixels to remove in the Cr and Cb\n"
                  "                           channels for every pixel not removed horizontally\n"
                  "                           and vertically.\n");
  usage_text += Y("  --cb-subsample <TID:hori,vert>\n"
                  "                           The amount of pixels to remove in the Cb channel\n"
                  "                           for every pixel not removed horizontally and\n"
                  "                           vertically. This is additive with\n"
                  "                           --chroma-subsample.\n");
  usage_text += Y("  --chroma-siting <TID:hori,vert>\n "
                  "                           How chroma is sited horizontally/vertically.\n");
  usage_text += Y("  --color-range <TID:n>   Clipping of the color ranges.\n");
  usage_text += Y("  --color-transfer-characteristics <TID:n>\n"
                  "                           The transfer characteristics of the video.\n");
  usage_text += Y("  --color-primaries <TID:n>\n"
                  "                           The color primaries of the video.\n");
  usage_text += Y("  --max-content-light <TID:n>\n"
                  "                           Maximum brightness of a single pixel in candelas\n"
                  "                           per square meter (cd/m²).\n");
  usage_text += Y("  --max-frame-light <TID:n>\n"
                  "                           Maximum frame-average light level in candelas per\n"
                  "                           square meter (cd/m²).\n");
  usage_text += Y("  --chromaticity-coordinates <TID:red-x,red-y,green-x,green-y,blue-x,blue-y>\n"
                  "                           Red/Green/Blue chromaticity coordinates as defined\n"
                  "                           by CIE 1931.\n");
  usage_text += Y("  --white-color-coordinates <TID:x,y>\n"
                  "                           White color chromaticity coordinates as defined\n"
                  "                           by CIE 1931.\n");
  usage_text += Y("  --max-luminance <TID:float>\n"
                  "                           Maximum luminance in candelas per square meter\n"
                  "                           (cd/m²).\n");
  usage_text += Y("  --min-luminance <TID:float>\n"
                  "                           Minimum luminance in candelas per square meter\n"
                  "                           (cd/m²).\n");
  usage_text += Y("  --projection-type <TID:method>\n"
                  "                           Projection method used (0–3).\n");
  usage_text += Y("  --projection-private <TID:data>\n"
                  "                           Private data that only applies to a specific\n"
                  "                           projection (as hex digits).\n");
  usage_text += Y("  --projection-pose-yaw <TID:float>\n"
                  "                           A yaw rotation to the projection.\n");
  usage_text += Y("  --projection-pose-pitch <TID:float>\n"
                  "                           A pitch rotation to the projection.\n");
  usage_text += Y("  --projection-pose-roll <TID:float>\n"
                  "                           A roll rotation to the projection.\n");
  usage_text +=   "\n";
  usage_text += Y(" Options that only apply to text subtitle tracks:\n");
  usage_text += Y("  --sub-charset <TID:charset>\n"
                  "                           Determines the charset the text subtitles are\n"
                  "                           read as for the conversion to UTF-8.\n");
  usage_text +=   "\n";
  usage_text += Y(" Options that only apply to VobSub subtitle tracks:\n");
  usage_text += Y("  --compression <TID:method>\n"
                  "                           Sets the compression method used for the\n"
                  "                           specified track ('none' or 'zlib').\n");
  usage_text +=   "\n\n";
  usage_text += Y(" Other options:\n");
  usage_text += Y("  -i, --identify <file>    Print information about the source file.\n");
  usage_text += Y("  -J <file>                This is a convenient alias for\n"
                  "                           \"--identification-format json --identify file\".\n");
  usage_text += Y("  -F, --identification-format <format>\n"
                  "                           Set the identification results format\n"
                  "                           ('text' or 'json'; default is 'text').\n");
  usage_text += Y("  --probe-range-percentage <percent>\n"
                  "                           Sets maximum size to probe for tracks in percent\n"
                  "                           of the total file size for certain file types\n"
                  "                           (default: 0.3).\n");
  usage_text += Y("  -l, --list-types         Lists supported source file types.\n");
  usage_text += Y("  --list-audio-emphasis    Lists all supported values for the\n"
                  "                           '--audio-emphasis' option and their meaning.\n");
  usage_text += Y("  --list-languages         Lists all ISO 639 languages and their\n"
                  "                           ISO 639-2 codes.\n");
  usage_text += Y("  --list-stereo-modes      Lists all supported values for the '--stereo-mode'\n"
                  "                           option and their meaning.\n");
  usage_text += Y("  --capabilities           Lists optional features mkvmerge was compiled with.\n");
  usage_text += Y("  --priority <priority>    Set the priority mkvmerge runs with.\n");
  usage_text += Y("  --ui-language <code>     Force the translations for 'code' to be used.\n");
  usage_text += Y("  --command-line-charset <charset>\n"
                  "                           Charset for strings on the command line\n");
  usage_text += Y("  --output-charset <cset>  Output messages in this charset\n");
  usage_text += Y("  -r, --redirect-output <file>\n"
                  "                           Redirects all messages into this file.\n");
  usage_text += Y("  --flush-on-close         Flushes all cached data to storage when closing\n"
                  "                           a file opened for writing.\n");
  usage_text += Y("  --abort-on-warnings      Aborts the program after the first warning is\n"
                  "                           emitted.\n");
  usage_text += Y("  --deterministic <seed>   Enables the creation of byte-identical files\n"
                  "                           if the same source files with the same options\n"
                  "                           and the same seed are used.\n");
  usage_text += "\n";
  usage_text += Y("  --debug <topic>          Turns on debugging output for 'topic'.\n");
  usage_text += Y("  --engage <feature>       Turns on experimental feature 'feature'.\n");
  usage_text += Y("  @option-file.json        Reads additional command line options from\n"
                  "                           the specified JSON file (see man page).\n");
  usage_text += Y("  -h, --help               Show this help.\n");
  usage_text += Y("  -V, --version            Show version information.\n");
  usage_text +=   "\n\n";
  usage_text += Y("Please read the man page/the HTML documentation to mkvmerge. It\n"
                  "explains several details in great length which are not obvious from\n"
                  "this listing.\n");

  mtx::cli::g_usage_text = usage_text;
}

/** \brief Prints information about what has been compiled into mkvmerge
*/
static void
print_capabilities() {
  mxinfo(fmt::format("VERSION={0}\n", get_version_info("mkvmerge", vif_full)));
#if defined(HAVE_FLAC_FORMAT_H)
  mxinfo("FLAC\n");
#endif
}

static std::string
guess_mime_type_and_report(std::string file_name) {
  auto mime_type = ::mtx::mime::guess_type_for_file(file_name);
  mime_type      = ::mtx::mime::get_font_mime_type_to_use(mime_type, g_use_legacy_font_mime_types ? mtx::mime::font_mime_type_type_e::legacy : mtx::mime::font_mime_type_type_e::current);
  if (mime_type != "") {
    mxinfo(fmt::format(FY("Automatic MIME type recognition for '{0}': {1}\n"), file_name, mime_type));
    return mime_type;
  }

  mxerror(fmt::format(FY("No MIME type has been set for the attachment '{0}', and it could not be guessed.\n"), file_name));
  return "";
}

static void
handle_segmentinfo() {
  // segment families
  libmatroska::KaxSegmentFamily *family = find_child<libmatroska::KaxSegmentFamily>(g_kax_info_chap.get());
  while (family) {
    g_segfamily_uids.add_family_uid(*family);
    family = FindNextChild(*g_kax_info_chap, *family);
  }

  libebml::EbmlBinary *uid = find_child<libmatroska::KaxSegmentUID>(g_kax_info_chap.get());
  if (uid)
    g_forced_seguids.push_back(mtx::bits::value_cptr(new mtx::bits::value_c(*uid)));

  uid = find_child<libmatroska::KaxNextUID>(g_kax_info_chap.get());
  if (uid)
    g_seguid_link_next = mtx::bits::value_cptr(new mtx::bits::value_c(*uid));

  uid = find_child<libmatroska::KaxPrevUID>(g_kax_info_chap.get());
  if (uid)
    g_seguid_link_previous = mtx::bits::value_cptr(new mtx::bits::value_c(*uid));

  auto segment_filename = find_child<libmatroska::KaxSegmentFilename>(g_kax_info_chap.get());
  if (segment_filename)
    g_segment_filename = segment_filename->GetValueUTF8();

  auto next_segment_filename = find_child<libmatroska::KaxNextFilename>(g_kax_info_chap.get());
  if (next_segment_filename)
    g_next_segment_filename = next_segment_filename->GetValueUTF8();

  auto previous_segment_filename = find_child<libmatroska::KaxPrevFilename>(g_kax_info_chap.get());
  if (previous_segment_filename)
    g_previous_segment_filename = previous_segment_filename->GetValueUTF8();
}

static void
list_file_types() {
  auto &file_types = mtx::file_type_t::get_supported();

  mxinfo(Y("Supported file types:\n"));

  for (auto &file_type : file_types)
    mxinfo(fmt::format("  {0} [{1}]\n", file_type.title, file_type.extensions));
}

static void
display_unsupported_file_type_json(filelist_t const &file) {
  auto json = nlohmann::json{
    { "identification_format_version", ID_JSON_FORMAT_VERSION },
    { "file_name",                     file.name              },
    { "container", {
        { "recognized", false },
        { "supported",  false },
      } },
  };

  display_json_output(json);

  mxexit(0);
}

static void
display_unsupported_file_type(filelist_t const &file) {
  if (identification_output_format_e::json == g_identification_output_format)
    display_unsupported_file_type_json(file);

  mxerror(fmt::format(FY("The type of file '{0}' is not supported.\n"), file.name));
}

/** \brief Identify a file type and its contents

   This function called for \c --identify. It sets up dummy track info
   data for the reader, probes the input file, creates the file reader
   and calls its identify function.
*/
static void
identify(std::string &filename) {
  g_files.emplace_back(new filelist_t);
  auto &file = *g_files.back();
  file.ti    = std::make_unique<track_info_c>();

  if ('=' == filename[0]) {
    file.ti->m_disable_multi_file = true;
    filename                      = filename.substr(1);
  }

  verbose             = 0;
  g_suppress_warnings = true;
  g_identifying       = true;
  file.ti->m_fname    = filename;
  file.name           = filename;
  file.all_names.push_back(filename);

  file.reader = probe_file_format(file);

  if (!file.reader)
    display_unsupported_file_type(file);

  read_file_headers();

  file.reader->identify();
  file.reader->display_identification_results();

  g_files.clear();
}

/** \brief Parse tags and add them to the list of all tags

   Also tests the tags for missing mandatory elements.
*/
void
parse_and_add_tags(std::string const &file_name) {
  auto tags = mtx::xml::ebml_tags_converter_c::parse_file(file_name, false);
  if (!tags)
    return;

  for (auto element : *tags) {
    auto tag = dynamic_cast<libmatroska::KaxTag *>(element);
    if (tag) {
      if (!tag->CheckMandatory())
        mxerror(fmt::format(FY("Error parsing the tags in '{0}': some mandatory elements are missing.\n"), file_name));
      add_tags(*tag);
    }
  }

  tags->RemoveAll();
}

static mtx::bcp47::language_c
parse_language(std::string const &arg,
               std::string const &option) {
  auto language = mtx::bcp47::language_c::parse(arg);

  if (!language.is_valid())
    mxerror(fmt::format(FY("'{0}' is not a valid IETF BCP 47/RFC 5646 language tag in '{1}'. "
                           "Additional information from the parser: {2}\n"),
                        arg, option, language.get_error()));

  return language;
}

/** \brief Parse the \c --xtracks arguments

   The argument is a comma separated list of track IDs.
*/
static void
parse_arg_tracks(std::string s,
                 item_selector_c<bool> &tracks,
                 std::string const &opt) {
  tracks.clear();

  if (balg::starts_with(s, "!")) {
    s.erase(0, 1);
    tracks.set_reversed();
  }

  auto elements = mtx::string::split(s, ",");
  mtx::string::strip(elements);

  for (auto const &element : elements) {
    int64_t tid;
    if (mtx::string::parse_number(element, tid)) {
      tracks.add(tid);
      continue;
    }

    auto language = parse_language(element, fmt::format("{0} {1}", opt, s));

    tracks.add(language);
  }
}

/** \brief Parse the \c --sync argument

   The argument must have the form <tt>TID:d</tt> or
   <tt>TID:d,l1/l2</tt>, e.g. <tt>0:200</tt>.  The part before the
   comma is the displacement in ms.
   The optional part after comma is the linear factor which defaults
   to 1 if not given.
*/
static void
parse_arg_sync(std::string s,
               std::string const &opt,
               track_info_c &ti,
               std::optional<int64_t> force_track_id) {
  timestamp_sync_t tcsync;

  // Extract the track number.
  auto orig  = s;
  int64_t id = 0;

  if (!force_track_id) {
    auto parts = mtx::string::split(s, ":", 2);
    if (parts.size() != 2)
      mxerror(fmt::format(FY("Invalid sync option. No track ID specified in '{0} {1}'.\n"), opt, s));

    if (!mtx::string::parse_number(parts[0], id))
      mxerror(fmt::format(FY("Invalid track ID specified in '{0} {1}'.\n"), opt, s));

    s = parts[1];

    if (s.size() == 0)
      mxerror(fmt::format(FY("Invalid sync option specified in '{0} {1}'.\n"), opt, orig));

  } else
    id = *force_track_id;

  if (s == "reset") {
    ti.m_reset_timestamps_specs[id] = true;
    return;
  }

  // Now parse the actual sync values.
  int idx = s.find(',');
  if (idx >= 0) {
    std::string linear = s.substr(idx + 1);
    s.erase(idx);
    idx = linear.find('/');
    if (idx < 0)
      mtx::string::parse_floating_point_number_as_rational(linear, tcsync.factor);

    else {
      mtx_mp_rational_t numerator, denominator;
      std::string div = linear.substr(idx + 1);
      linear.erase(idx);
      if (   !mtx::string::parse_floating_point_number_as_rational(linear, numerator)
          || !mtx::string::parse_floating_point_number_as_rational(div,    denominator)
          || (denominator == 0))
        mxerror(fmt::format(FY("Invalid sync option specified in '{0} {1}'. The divisor is zero.\n"), opt, orig));

      tcsync.factor = numerator / denominator;
    }
    if ((tcsync.factor) <= mtx_mp_rational_t{0, 1})
      mxerror(fmt::format(FY("Invalid sync option specified in '{0} {1}'. The linear sync value may not be equal to or smaller than zero.\n"), opt, orig));

  }

  if (!mtx::string::parse_number(s, tcsync.displacement))
    mxerror(fmt::format(FY("Invalid sync option specified in '{0} {1}'.\n"), opt, orig));

  tcsync.displacement     *= 1000000ll;
  ti.m_timestamp_syncs[id] = tcsync;
}

/** \brief Parse the \c --aspect-ratio argument

   The argument must have the form \c TID:w/h or \c TID:float, e.g. \c 0:16/9
*/
static void
parse_arg_aspect_ratio(std::string const &s,
                       std::string const &opt,
                       bool is_factor,
                       track_info_c &ti) {
  display_properties_t dprop;

  std::string msg      = is_factor ? Y("Aspect ratio factor") : Y("Aspect ratio");

  dprop.ar_factor      = is_factor;
  std::vector<std::string> parts = mtx::string::split(s, ":", 2);
  if (parts.size() != 2)
    mxerror(fmt::format(FY("{0}: missing track ID in '{1} {2}'.\n"), msg, opt, s));

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("{0}: invalid track ID in '{1} {2}'.\n"), msg, opt, s));

  dprop.width  = -1;
  dprop.height = -1;

  int idx      = parts[1].find('/');
  if (0 > idx)
    idx = parts[1].find(':');
  if (0 > idx) {
    dprop.aspect_ratio          = strtod(parts[1].c_str(), nullptr);
    ti.m_display_properties[id] = dprop;
    return;
  }

  std::string div = parts[1].substr(idx + 1);
  parts[1].erase(idx);
  if (parts[1].empty())
    mxerror(fmt::format(FY("{0}: missing dividend in '{1} {2}'.\n"), msg, opt,s));

  if (div.empty())
    mxerror(fmt::format(FY("{0}: missing divisor in '{1} {2}'.\n"), msg, opt, s));

  double w = strtod(parts[1].c_str(), nullptr);
  double h = strtod(div.c_str(), nullptr);
  if (0.0 == h)
    mxerror(fmt::format(FY("{0}: divisor is 0 in '{1} {2}'.\n"), msg, opt, s));

  dprop.aspect_ratio          = w / h;
  ti.m_display_properties[id] = dprop;
}

/** \brief Parse the \c --display-dimensions argument

   The argument must have the form \c TID:wxh, e.g. \c 0:640x480.
*/
static void
parse_arg_display_dimensions(const std::string s,
                             track_info_c &ti) {
  display_properties_t dprop;

  std::vector<std::string> parts = mtx::string::split(s, ":", 2);
  mtx::string::strip(parts);
  if (parts.size() != 2)
    mxerror(fmt::format(FY("Display dimensions: not given in the form <TID>:<width>x<height>, e.g. 1:640x480 (argument was '{0}').\n"), s));

  std::vector<std::string> dims = mtx::string::split(parts[1], "x", 2);
  int64_t id = 0;
  int w = 0, h = 0;
  if ((dims.size() != 2) || !mtx::string::parse_number(parts[0], id) || !mtx::string::parse_number(dims[0], w) || !mtx::string::parse_number(dims[1], h) || (0 >= w) || (0 >= h))
    mxerror(fmt::format(FY("Display dimensions: not given in the form <TID>:<width>x<height>, e.g. 1:640x480 (argument was '{0}').\n"), s));

  dprop.aspect_ratio          = -1.0;
  dprop.width                 = w;
  dprop.height                = h;

  ti.m_display_properties[id] = dprop;
}

/** \brief Parse the \c --cropping argument

   The argument must have the form \c TID:left,top,right,bottom e.g.
   \c 0:10,5,10,5
*/
static void
parse_arg_cropping(std::string const &s,
                   track_info_c &ti) {
  pixel_crop_t crop;

  std::string err_msg        = Y("Cropping parameters: not given in the form <TID>:<left>,<top>,<right>,<bottom> e.g. 0:10,5,10,5 (argument was '{0}').\n");

  std::vector<std::string> v = mtx::string::split(s, ":");
  if (v.size() != 2)
    mxerror(fmt::format(fmt::runtime(err_msg), s));

  int64_t id = 0;
  if (!mtx::string::parse_number(v[0], id))
    mxerror(fmt::format(fmt::runtime(err_msg), s));

  v = mtx::string::split(v[1], ",");
  if (v.size() != 4)
    mxerror(fmt::format(fmt::runtime(err_msg), s));

  if (!mtx::string::parse_number(v[0], crop.left) || !mtx::string::parse_number(v[1], crop.top) || !mtx::string::parse_number(v[2], crop.right) || !mtx::string::parse_number(v[3], crop.bottom))
    mxerror(fmt::format(fmt::runtime(err_msg), s));

  ti.m_pixel_crop_list[id] = crop;
}

/** \brief Parse the \c --color-matrix-coefficients argument

   The argument must have the form \c TID:n e.g. \c 0:2
   The number n must be one of the following integer numbers
   0: GBR
   1: BT709
   2: Unspecified
   3: Reserved
   4: FCC
   5: BT470BG
   6: SMPTE 170M
   7: SMPTE 240M
   8: YCOCG
   9: BT2020 Non-constant Luminance
   10: BT2020 Constant Luminance)
*/
static void
parse_arg_color_matrix_coefficients(std::string const &s,
                                     track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<uint64_t>(s, ti.m_color_matrix_coeff_list))
    mxerror(fmt::format("Color matrix coefficients parameter: not given in the form <TID>:n (argument was '{0}').", s));
}

/** \brief Parse the \c --color-bits-per-channel argument
   The argument must have the form \c TID:n e.g. \c 0:8
*/
static void
parse_arg_color_bits_per_channel(std::string const &s,
                                  track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<uint64_t>(s, ti.m_bits_per_channel_list))
    mxerror(fmt::format("Bits per channel parameter: not given in the form <TID>:n (argument was '{0}').", s));
}

/** \brief Parse the \c --chroma-subsample argument
   The argument must have the form \c TID:hori,vert e.g. \c 0:1,1
*/
static void
parse_arg_chroma_subsample(std::string const &s,
                           track_info_c &ti) {
  if (!mtx::string::parse_property_to_struct<subsample_or_siting_t, uint64_t>(s, ti.m_chroma_subsample_list))
    mxerror(fmt::format("Chroma subsampling parameter: not given in the form <TID>:hori,vert (argument was '{0}').", s));
}

/** \brief Parse the \c --cb-subsample argument
   The argument must have the form \c TID:hori,vert e.g. \c 0:1,1
*/
static void
parse_arg_cb_subsample(std::string const &s,
                       track_info_c &ti) {
  if (!mtx::string::parse_property_to_struct<subsample_or_siting_t, uint64_t>(s, ti.m_cb_subsample_list))
    mxerror(fmt::format("Cb subsampling parameter: not given in the form <TID>:hori,vert (argument was '{0}').", s));
}

/** \brief Parse the \c --chroma-siting argument
   The argument must have the form \c TID:hori,vert e.g. \c 0:1,1
*/
static void
parse_arg_chroma_siting(std::string const &s,
                        track_info_c &ti) {
  if (!mtx::string::parse_property_to_struct<subsample_or_siting_t, uint64_t>(s, ti.m_chroma_siting_list))
    mxerror(fmt::format("Chroma siting parameter: not given in the form <TID>:hori,vert (argument was '{0}').", s));
}

/** \brief Parse the \c --color-range argument
   The argument must have the form \c TID:n e.g. \c 0:1
*/
static void
parse_arg_color_range(std::string const &s,
                      track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<uint64_t>(s, ti.m_color_range_list))
    mxerror(fmt::format("Color range parameters: not given in the form <TID>:n (argument was '{0}').", s));
}

/** \brief Parse the \c --color-transfer-characteristics argument
   The argument must have the form \c TID:n e.g. \c 0:1
*/
static void
parse_arg_color_transfer(std::string const &s,
                          track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<uint64_t>(s, ti.m_color_transfer_list))
    mxerror(fmt::format("Color transfer characteristics parameter : not given in the form <TID>:n (argument was '{0}').", s));
}

/** \brief Parse the \c --color-primaries argument
   The argument must have the form \c TID:n e.g. \c 0:1
*/
static void
parse_arg_color_primaries(std::string const &s,
                           track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<uint64_t>(s, ti.m_color_primaries_list))
    mxerror(fmt::format("Color primaries parameter: not given in the form <TID>:n (argument was '{0}').", s));
}

/** \brief Parse the \c --max-content-light argument
   The argument must have the form \c TID:n e.g. \c 0:1
*/
static void
parse_arg_max_content_light(std::string const &s,
                            track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<uint64_t>(s, ti.m_max_cll_list))
    mxerror(fmt::format("Max content light parameter: not given in the form <TID>:n (argument was '{0}').", s));
}

/** \brief Parse the \c --max-frame-light argument
   The argument must have the form \c TID:n e.g. \c 0:1
*/
static void
parse_arg_max_frame_light(std::string const &s,
                          track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<uint64_t>(s, ti.m_max_fall_list))
    mxerror(fmt::format("Max frame light parameter: not given in the form <TID>:n (argument was '{0}').", s));
}

/** \brief Parse the \c --chromaticity-coordinates argument
   The argument must have the form \c
   TID:TID:Red_x,Red_y,Green_x,Green_y,Blue_x,Blue_y
*/
static void
parse_arg_chroma_coordinates(std::string const &s,
                             track_info_c &ti) {
  if (!mtx::string::parse_property_to_struct<chroma_coordinates_t, double>(s, ti.m_chroma_coordinates_list))
    mxerror(fmt::format("chromaticity coordinates parameter: not given in the form <TID>:hori,vert (argument was '{0}').", s));
}

/** \brief Parse the \c --white-color-coordinates argument
   The argument must have the form \c TID:TID:x,y
*/
static void
parse_arg_white_coordinates(std::string const &s,
                            track_info_c &ti) {
  if (!mtx::string::parse_property_to_struct<white_color_coordinates_t, double>(s, ti.m_white_coordinates_list))
    mxerror(fmt::format("white color coordinates parameter: not given in the form <TID>:hori,vert (argument was '{0}').", s));
}

/** \brief Parse the \c --max-luminance argument
   The argument must have the form \c TID:float e.g. \c 0:1235.7
*/
static void
parse_arg_max_luminance(std::string const &s,
                        track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<double>(s, ti.m_max_luminance_list))
    mxerror(fmt::format("Max luminance parameter: not given in the form <TID>:n (argument was '{0}').", s));
}

/** \brief Parse the \c --min-luminance argument
   The argument must have the form \c TID:float e.g. \c 0:0.7
*/
static void
parse_arg_min_luminance(std::string const &s,
                        track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<double>(s, ti.m_min_luminance_list))
    mxerror(fmt::format("Min luminance parameter: not given in the form <TID>:n (argument was '{0}').", s));
}

static void
parse_arg_projection_type(std::string const &s,
                          track_info_c &ti) {
  if (!mtx::string::parse_property_to_value<uint64_t>(s, ti.m_projection_type_list))
    mxerror(fmt::format("Parameter {0}: not given in the form <TID>:n (argument was '{1}').\n", "--projection-type", s));
}

static void
parse_arg_projection_private(std::string const &s,
                             track_info_c &ti) {
  auto parts = mtx::string::split(s, ":");
  mtx::string::strip(parts);

  uint64_t tid{};

  if ((parts.size() != 2) || !mtx::string::parse_number(parts[0], tid))
    mxerror(fmt::format("Parameter {0}: not given in the form <TID>:n (argument was '{1}').\n", "--projection-private", s));

  try {
    mtx::bits::value_c value{parts[1]};
    ti.m_projection_private_list[tid] = memory_c::clone(value.data(), value.byte_size());

  } catch (...) {
    mxerror(fmt::format(FY("Unknown format in '{0} {1}'.\n"), "--projection-private", s));
  }
}

static void
parse_arg_projection_pose_xyz(std::string const &s,
                              std::map<int64_t, double> &list,
                              std::string const &arg_suffix) {
  if (!mtx::string::parse_property_to_value<double>(s, list))
    mxerror(fmt::format("Parameter {0}: not given in the form <TID>:n (argument was '{1}').\n", fmt::format("--projection-pose-{0}", arg_suffix), s));
}

/** \brief Parse the \c --stereo-mode argument

   The argument must either be a number starting at 0 or
   one of these keywords:

   0: mono
   1: side by side (left eye is first)
   2: top-bottom (right eye is first)
   3: top-bottom (left eye is first)
   4: checkerboard (right is first)
   5: checkerboard (left is first)
   6: row interleaved (right is first)
   7: row interleaved (left is first)
   8: column interleaved (right is first)
   9: column interleaved (left is first)
   10: anaglyph (cyan/red)
   11: side by side (right eye is first)
*/
static void
parse_arg_stereo_mode(const std::string &s,
                      track_info_c &ti) {
  std::string errmsg = Y("Stereo mode parameter: not given in the form <TID>:<n|keyword> where n is a number between 0 and {0} "
                         "or one of these keywords: {1} (argument was '{2}').\n");

  std::vector<std::string> v = mtx::string::split(s, ":");
  if (v.size() != 2)
    mxerror(fmt::format(fmt::runtime(errmsg), stereo_mode_c::max_index(), stereo_mode_c::displayable_modes_list(), s));

  int64_t id = 0;
  if (!mtx::string::parse_number(v[0], id))
    mxerror(fmt::format(fmt::runtime(errmsg), stereo_mode_c::max_index(), stereo_mode_c::displayable_modes_list(), s));

  stereo_mode_c::mode mode = stereo_mode_c::parse_mode(v[1]);
  if (stereo_mode_c::invalid != mode) {
    ti.m_stereo_mode_list[id] = mode;
    return;
  }

  int index;
  if (!mtx::string::parse_number(v[1], index) || !stereo_mode_c::valid_index(index))
    mxerror(fmt::format(fmt::runtime(errmsg), stereo_mode_c::max_index(), stereo_mode_c::displayable_modes_list(), s));

  ti.m_stereo_mode_list[id] = static_cast<stereo_mode_c::mode>(index);
}

static void
parse_arg_field_order(const std::string &s,
                      track_info_c &ti) {
  std::vector<std::string> parts = mtx::string::split(s, ":");
  if (parts.size() != 2)
    mxerror(fmt::format("{0} {1}\n",
                        fmt::format(FY("The argument '{0}' to '{1}' is invalid."), s, "--field-order"),
                        Y("It does not consist of a track ID and a value separated by a colon.")));

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format("{0} {1}\n",
                        fmt::format(FY("The argument '{0}' to '{1}' is invalid."), s, "--field-order"),
                        fmt::format(FY("'{0}' is not a valid track ID."), parts[0])));

  uint64_t order;
  if (!mtx::string::parse_number(parts[1], order) || !mtx::included_in(order, 0u, 1u, 2u, 6u, 9u, 14u))
    mxerror(fmt::format("{0} {1}\n",
                        fmt::format(FY("The argument '{0}' to '{1}' is invalid."), s, "--field-order"),
                        fmt::format(FY("'{0}' is not a valid field order."), parts[0])));

  ti.m_field_order_list[id] = order;
}

static void
parse_arg_audio_emphasis(const std::string &s,
                              track_info_c &ti) {
  auto error_message = Y("Audio emphasis mode option: not given in the form <TID>:<mode> where the mode is either a number or keyword from this list: {0} (argument was '{1}').\n");
  auto error         = fmt::format(fmt::runtime(error_message), audio_emphasis_c::displayable_modes_list(), s);

  auto v = mtx::string::split(s, ":");
  if (v.size() != 2)
    mxerror(error);

  int64_t id = 0;
  if (!mtx::string::parse_number(v[0], id))
    mxerror(error);

  auto mode = audio_emphasis_c::parse_mode(v[1]);
  if (audio_emphasis_c::invalid != mode) {
    ti.m_audio_emphasis_list[id] = mode;
    return;
  }

  int index;
  if (!mtx::string::parse_number(v[1], index) || !audio_emphasis_c::valid_index(index))
    mxerror(error);

  ti.m_audio_emphasis_list[id] = static_cast<audio_emphasis_c::mode_e>(index);
}

/** \brief Parse the duration formats to \c --split

  This function is called by ::parse_split if the format specifies
  a duration after which a new file should be started.
*/
static void
parse_arg_split_duration(const std::string &arg) {
  std::string s = arg;

  if (balg::istarts_with(s, "duration:"))
    s.erase(0, strlen("duration:"));

  int64_t split_after;
  if (!mtx::string::parse_timestamp(s, split_after))
    mxerror(fmt::format(FY("Invalid time for '--split' in '--split {0}'. Additional error message: {1}\n"), arg, mtx::string::timestamp_parser_error));

  g_cluster_helper->add_split_point(split_point_c(split_after, split_point_c::duration, false));
}

/** \brief Parse the timestamp format to \c --split

  This function is called by ::parse_split if the format specifies
  timestamps after which a new file should be started.
*/
static void
parse_arg_split_timestamps(const std::string &arg) {
  std::string s = arg;

  if (auto qs = Q(s); qs.contains(QRegularExpression{"^time(?:stamps|codes):", QRegularExpression::CaseInsensitiveOption}))
    s = to_utf8(qs.replace(QRegularExpression{"^.*?:"}, {}));

  auto timestamps = mtx::string::split(s, ",");
  for (auto &timestamp : timestamps) {
    int64_t split_after;
    if (!mtx::string::parse_timestamp(timestamp, split_after))
      mxerror(fmt::format(FY("Invalid time for '--split' in '--split {0}'. Additional error message: {1}.\n"), arg, mtx::string::timestamp_parser_error));
    g_cluster_helper->add_split_point(split_point_c(split_after, split_point_c::timestamp, true));
  }
}

/** \brief Parse the frames format to \c --split

  This function is called by ::parse_split if the format specifies
  frames after which a new file should be started.
*/
static void
parse_arg_split_frames(std::string const &arg) {
  std::string s = arg;

  if (balg::istarts_with(s, "frames:"))
    s.erase(0, 7);

  auto frames = mtx::string::split(s, ",");
  for (auto &frame : frames) {
    uint64_t split_after = 0;
    if (!mtx::string::parse_number(frame, split_after) || (0 == split_after))
      mxerror(fmt::format(FY("Invalid frame for '--split' in '--split {0}'.\n"), arg));
    g_cluster_helper->add_split_point(split_point_c(split_after, split_point_c::frame_field, true));
  }
}

void
parse_arg_split_chapters(std::string const &arg) {
  s_split_by_chapters_arg = arg;
  auto s                  = arg;

  if (balg::istarts_with(s, "chapters:"))
    s.erase(0, 9);

  if (s == "all") {
    g_splitting_by_all_chapters = true;
    return;
  }

  for (auto &number_str : mtx::string::split(s, ",")) {
    auto number = 0u;
    if (!mtx::string::parse_number(number_str, number) || !number)
      mxerror(fmt::format(FY("Invalid chapter number '{0}' for '--split' in '--split {1}': {2}\n"), number_str, arg, Y("Not a valid number or not positive.")));

    g_splitting_by_chapter_numbers[number] = 1;
  }

  if (g_splitting_by_chapter_numbers.empty())
    mxerror(fmt::format(FY("No chapter numbers listed after '--split {0}'.\n"), arg));
}

static void
parse_arg_split_parts(const std::string &arg,
                      bool frames_fields) {
  try {
    auto split_points = mtx::args::parse_split_parts(arg, frames_fields);
    for (auto &point : split_points)
      g_cluster_helper->add_split_point(point);

  } catch (mtx::args::format_x &ex) {
    mxerror(ex.what());
  }
}

/** \brief Parse the size format to \c --split

  This function is called by ::parse_split if the format specifies
  a size after which a new file should be started.
*/
static void
parse_arg_split_size(const std::string &arg) {
  std::string s       = arg;
  std::string err_msg = Y("Invalid split size in '--split {0}'.\n");

  if (balg::istarts_with(s, "size:"))
    s.erase(0, strlen("size:"));

  if (s.empty())
    mxerror(fmt::format(fmt::runtime(err_msg), arg));

  // Size in bytes/KB/MB/GB
  char mod         = tolower(s[s.length() - 1]);
  int64_t modifier = 1;
  if ('k' == mod)
    modifier = 1024;
  else if ('m' == mod)
    modifier = 1024 * 1024;
  else if ('g' == mod)
    modifier = 1024 * 1024 * 1024;
  else if (!isdigit(mod))
    mxerror(fmt::format(fmt::runtime(err_msg), arg));

  if (1 != modifier)
    s.erase(s.size() - 1);

  int64_t split_after = 0;
  if (!mtx::string::parse_number(s, split_after))
    mxerror(fmt::format(fmt::runtime(err_msg), arg));

  g_cluster_helper->add_split_point(split_point_c(split_after * modifier, split_point_c::size, false));
}

/** \brief Parse the \c --split argument

   The \c --split option takes several formats.

   \arg size based: If only a number is given or the number is
   postfixed with '<tt>K</tt>', '<tt>M</tt>' or '<tt>G</tt>' this is
   interpreted as the size after which to split.
   This format is parsed by ::parse_split_size

   \arg time based: If a number postfixed with '<tt>s</tt>' or in a
   format matching '<tt>HH:MM:SS</tt>' or '<tt>HH:MM:SS.mmm</tt>' is
   given then this is interpreted as the time after which to split.
   This format is parsed by ::parse_split_duration
*/
static void
parse_arg_split(const std::string &arg) {
  std::string err_msg = Y("Invalid format for '--split' in '--split {0}'.\n");

  if (arg.size() < 2)
    mxerror(fmt::format(fmt::runtime(err_msg), arg));

  std::string s = arg;

  // HH:MM:SS
  if (balg::istarts_with(s, "duration:"))
    parse_arg_split_duration(arg);

  else if (balg::istarts_with(s, "size:"))
    parse_arg_split_size(arg);

  else if (Q(s).contains(QRegularExpression{"^time(?:stamps|codes):", QRegularExpression::CaseInsensitiveOption}))
    parse_arg_split_timestamps(arg);

  else if (balg::istarts_with(s, "parts:"))
    parse_arg_split_parts(arg, false);

  else if (balg::istarts_with(s, "parts-frames:"))
    parse_arg_split_parts(arg, true);

  else if (balg::istarts_with(s, "frames:"))
    parse_arg_split_frames(arg);

  else if (balg::istarts_with(s, "chapters:"))
    parse_arg_split_chapters(arg);

  else if ((   (s.size() == 8)
            || (s.size() == 12))
           && (':' == s[2]) && (':' == s[5])
           && isdigit(s[0]) && isdigit(s[1]) && isdigit(s[3])
           && isdigit(s[4]) && isdigit(s[6]) && isdigit(s[7]))
    // HH:MM:SS
    parse_arg_split_duration(arg);

  else {
    char mod = tolower(s[s.size() - 1]);

    if ('s' == mod)
      parse_arg_split_duration(arg);

    else if (('k' == mod) || ('m' == mod) || ('g' == mod) || isdigit(mod))
      parse_arg_split_size(arg);

    else
      mxerror(fmt::format(fmt::runtime(err_msg), arg));
  }
}

/** \brief Parse the \c --default-track-flag argument

   The argument must have the form \c TID or \c TID:boolean. The former
   is equivalent to \c TID:1.
*/
static void
parse_arg_boolean_track_option(std::string const &option,
                               std::string const &value,
                               std::map<int64_t, bool> &storage) {
  auto is_set = true;
  auto parts  = mtx::string::split(value, ":", 2);
  int64_t id  = 0;

  mtx::string::strip(parts);
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("Invalid track ID specified in '{0} {1}'.\n"), option, value));

  try {
    if (2 == parts.size())
      is_set = mtx::string::parse_bool(parts[1]);
  } catch (...) {
    mxerror(fmt::format(FY("Invalid boolean option specified in '{0} {1}'.\n"), option, value));
  }

  storage[id] = is_set;
}

/** \brief Parse the \c --cues argument

   The argument must have the form \c TID:cuestyle, e.g. \c 0:none.
*/
static void
parse_arg_cues(const std::string &s,
               track_info_c &ti) {

  // Extract the track number.
  auto parts = mtx::string::split(s, ":", 2);
  mtx::string::strip(parts);
  if (parts.size() != 2)
    mxerror(fmt::format(FY("Invalid cues option. No track ID specified in '--cues {0}'.\n"), s));

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("Invalid track ID specified in '--cues {0}'.\n"), s));

  if (parts[1].empty())
    mxerror(fmt::format(FY("Invalid cues option specified in '--cues {0}'.\n"), s));

  if (parts[1] == "all")
    ti.m_cue_creations[id] = CUE_STRATEGY_ALL;
  else if (parts[1] == "iframes")
    ti.m_cue_creations[id] = CUE_STRATEGY_IFRAMES;
  else if (parts[1] == "none")
    ti.m_cue_creations[id] = CUE_STRATEGY_NONE;
  else
    mxerror(fmt::format(FY("'{0}' is an unsupported argument for --cues.\n"), s));
}

/** \brief Parse the \c --compression argument

   The argument must have the form \c TID:compression, e.g. \c 0:zlib.
*/
static void
parse_arg_compression(const std::string &s,
                  track_info_c &ti) {
  // Extract the track number.
  auto parts = mtx::string::split(s, ":", 2);
  mtx::string::strip(parts);
  if (parts.size() != 2)
    mxerror(fmt::format(FY("Invalid compression option. No track ID specified in '--compression {0}'.\n"), s));

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("Invalid track ID specified in '--compression {0}'.\n"), s));

  if (parts[1].size() == 0)
    mxerror(fmt::format(FY("Invalid compression option specified in '--compression {0}'.\n"), s));

  std::vector<std::string> available_compression_methods;
  available_compression_methods.push_back("none");
  available_compression_methods.push_back("zlib");
  available_compression_methods.push_back("mpeg4_p2");
  available_compression_methods.push_back("analyze_header_removal");

  ti.m_compression_list[id] = COMPRESSION_UNSPECIFIED;
  balg::to_lower(parts[1]);

  if (parts[1] == "zlib")
    ti.m_compression_list[id] = COMPRESSION_ZLIB;

  if (parts[1] == "none")
    ti.m_compression_list[id] = COMPRESSION_NONE;

  if ((parts[1] == "mpeg4_p2") || (parts[1] == "mpeg4p2"))
    ti.m_compression_list[id] = COMPRESSION_MPEG4_P2;

  if (parts[1] == "analyze_header_removal")
      ti.m_compression_list[id] = COMPRESSION_ANALYZE_HEADER_REMOVAL;

  if (ti.m_compression_list[id] == COMPRESSION_UNSPECIFIED)
    mxerror(fmt::format(FY("'{0}' is an unsupported argument for --compression. Available compression methods are: {1}\n"), s, mtx::string::join(available_compression_methods, ", ")));
}

static std::tuple<int64_t, std::string>
parse_arg_tid_and_string(std::string const &s,
                         std::string const &opt,
                         std::string const &topic,
                         bool empty_ok = false) {
  // Extract the track number.
  auto parts = mtx::string::split(s, ":", 2);
  mtx::string::strip(parts);
  if (parts.empty())
    mxerror(fmt::format(FY("No track ID specified in '--{0} {1}'.\n"), opt, s));
  if (1 == parts.size()) {
    if (!empty_ok)
      mxerror(fmt::format(FY("No {0} specified in '--{1} {2}'.\n"), topic, opt, s));
    parts.push_back("");
  }

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("Invalid track ID specified in '--{0} {1}'.\n"), opt, s));

  return std::make_tuple(id, parts[1]);
}

/** \brief Parse the argument for a couple of options

   Some options have similar parameter styles. The arguments must have
   the form \c TID:value, e.g. \c 0:XVID.
*/
static void
parse_arg_language(std::string const &s,
                   std::map<int64_t, mtx::bcp47::language_c> &storage,
                   std::string const &option) {
  auto [tid, string] = parse_arg_tid_and_string(s, "language", Y("language"));
  storage[tid]       = parse_language(string, option);
}

/** \brief Parse the \c --subtitle-charset argument

   The argument must have the form \c TID:charset, e.g. \c 0:ISO8859-15.
*/
static void
parse_arg_sub_charset(const std::string &s,
                      track_info_c &ti) {
  // Extract the track number.
  auto parts = mtx::string::split(s, ":", 2);
  mtx::string::strip(parts);
  if (parts.size() != 2)
    mxerror(fmt::format(FY("Invalid sub charset option. No track ID specified in '--sub-charset {0}'.\n"), s));

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("Invalid track ID specified in '--sub-charset {0}'.\n"), s));

  if (parts[1].empty())
    mxerror(fmt::format(FY("Invalid sub charset specified in '--sub-charset {0}'.\n"), s));

  ti.m_sub_charsets[id] = parts[1];
}

/** \brief Parse the \c --tags argument

   The argument must have the form \c TID:filename, e.g. \c 0:tags.xml.
*/
static void
parse_arg_tags(const std::string &s,
               const std::string &opt,
               track_info_c &ti) {
  // Extract the track number.
  auto parts = mtx::string::split(s, ":", 2);
  mtx::string::strip(parts);
  if (parts.size() != 2)
    mxerror(fmt::format(FY("Invalid tags option. No track ID specified in '{0} {1}'.\n"), opt, s));

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("Invalid track ID specified in '{0} {1}'.\n"), opt, s));

  if (parts[1].empty())
    mxerror(fmt::format(FY("Invalid tags file name specified in '{0} {1}'.\n"), opt, s));

  ti.m_all_tags[id] = parts[1];
}

/** \brief Parse the \c --fourcc argument

   The argument must have the form \c TID:fourcc, e.g. \c 0:XVID.
*/
static void
parse_arg_fourcc(const std::string &s,
                 const std::string &opt,
                 track_info_c &ti) {
  std::string orig               = s;
  auto parts = mtx::string::split(s, ":", 2);

  if (parts.size() != 2)
    mxerror(fmt::format(FY("FourCC: Missing track ID in '{0} {1}'.\n"), opt, orig));

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("FourCC: Invalid track ID in '{0} {1}'.\n"), opt, orig));

  if (parts[1].size() != 4)
    mxerror(fmt::format(FY("The FourCC must be exactly four characters long in '{0} {1}'.\n"), opt, orig));

  ti.m_all_fourccs[id] = parts[1];
}

/** \brief Parse the argument for \c --track-order

   The argument must be a comma separated list of track IDs.
*/
static void
parse_arg_track_order(const std::string &s) {
  track_order_t to;

  auto parts = mtx::string::split(s, ",");
  mtx::string::strip(parts);

  size_t i;
  for (i = 0; i < parts.size(); i++) {
    auto pair = mtx::string::split(parts[i].c_str(), ":");

    if (pair.size() != 2)
      mxerror(fmt::format(FY("'{0}' is not a valid pair of file ID and track ID in '--track-order {1}'.\n"), parts[i], s));

    if (!mtx::string::parse_number(pair[0], to.file_id))
      mxerror(fmt::format(FY("'{0}' is not a valid file ID in '--track-order {1}'.\n"), pair[0], s));

    if (!mtx::string::parse_number(pair[1], to.track_id))
      mxerror(fmt::format(FY("'{0}' is not a valid file ID in '--track-order {1}'.\n"), pair[1], s));

    if (std::find_if(g_track_order.begin(), g_track_order.end(), [&to](auto const &ref) { return (ref.file_id == to.file_id) && (ref.track_id == to.track_id); }) == g_track_order.end())
      g_track_order.push_back(to);
  }
}

/** \brief Parse the argument for \c --append-to

   The argument must be a comma separated list. Each of the list's items
   consists of four numbers separated by colons. These numbers are:
   -# the source file ID,
   -# the source track ID,
   -# the destination file ID and
   -# the destination track ID.

   File IDs are simply the file's position in the command line regarding
   all input files starting at zero. The first input file has the file ID
   0, the second one the ID 1 etc. The track IDs are just the usual track IDs
   used everywhere.

   The "destination" file and track ID identifies the track that is to be
   appended to the one specified by the "source" file and track ID.
*/
static void
parse_arg_append_to(const std::string &s) {
  g_append_mapping.clear();
  auto entries = mtx::string::split(s, ",");
  mtx::string::strip(entries);

  for (auto &entry : entries) {
    append_spec_t mapping;

    auto parts = mtx::string::split(entry, ":");

    if (   (parts.size() != 4)
        || !mtx::string::parse_number(parts[0], mapping.src_file_id)
        || !mtx::string::parse_number(parts[1], mapping.src_track_id)
        || !mtx::string::parse_number(parts[2], mapping.dst_file_id)
        || !mtx::string::parse_number(parts[3], mapping.dst_track_id))
      mxerror(fmt::format(FY("'{0}' is not a valid mapping of file and track IDs in '--append-to {1}'.\n"), entry, s));

    g_append_mapping.push_back(mapping);
  }
}

static void
parse_arg_append_mode(const std::string &s) {
  if ((s == "track") || (s == "track-based"))
    g_append_mode = APPEND_MODE_TRACK_BASED;

  else if ((s == "file") || (s == "file-based"))
    g_append_mode = APPEND_MODE_FILE_BASED;

  else
    mxerror(fmt::format(FY("'{0}' is not a valid append mode in '--append-mode {0}'.\n"), s));
}

/** \brief Parse the argument for \c --default-duration

   The argument must consist of a track ID and the default duration
   separated by a colon. The duration must be postfixed by 'ms', 'us',
   'ns', 'fps', 'p' or 'i' (see \c parse_number_with_unit).
*/
static void
parse_arg_default_duration(const std::string &s,
                           track_info_c &ti) {
  auto parts = mtx::string::split(s, ":");
  if (parts.size() != 2)
    mxerror(fmt::format(FY("'{0}' is not a valid pair of track ID and default duration in '--default-duration {0}'.\n"), s));

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id))
    mxerror(fmt::format(FY("'{0}' is not a valid track ID in '--default-duration {1}'.\n"), parts[0], s));

  int64_t default_duration{};
  if (!mtx::string::parse_duration_number_with_unit(parts[1], default_duration))
    mxerror(fmt::format(FY("'{0}' is not recognized as a valid number format or it doesn't contain a valid unit ('s', 'ms', 'us', 'ns', 'fps', 'p' or 'i') in '{1}'.\n"),
                        parts[1], fmt::format("--default-duration {0}", s)));

  ti.m_default_durations[id].first  = default_duration;
  ti.m_default_durations[id].second = Q(parts[1]).contains(QRegularExpression{".*i$"});
}

static void
parse_arg_reduce_to_core(const std::string &s,
                         track_info_c &ti) {
  int64_t id = 0;

  if (!mtx::string::parse_number(s, id))
    mxerror(fmt::format(FY("Invalid track ID specified in '{0} {1}'.\n"), "--reduce-to-core", s));

  ti.m_reduce_to_core[id] = true;
}

static void
parse_arg_remove_dialog_normalization_gain(const std::string &s,
                                           track_info_c &ti) {
  int64_t id = 0;

  if (!mtx::string::parse_number(s, id))
    mxerror(fmt::format(FY("Invalid track ID specified in '{0} {1}'.\n"), "--remove-dialog-normalization-gain", s));

  ti.m_remove_dialog_normalization_gain[id] = true;
}

/** \brief Parse the argument for \c --aac-is-sbr

   The argument can either be just a number (the track ID) or a tupel
   "trackID:number" where the second number is either "0" or "1". If only
   a track ID is given then "number" is assumed to be "1".
*/
static void
parse_arg_aac_is_sbr(const std::string &s,
                     track_info_c &ti) {
  auto parts = mtx::string::split(s, ":", 2);

  int64_t id = 0;
  if (!mtx::string::parse_number(parts[0], id) || (id < 0))
    mxerror(fmt::format(FY("Invalid track ID specified in '--aac-is-sbr {0}'.\n"), s));

  if ((parts.size() == 2) && (parts[1] != "0") && (parts[1] != "1"))
    mxerror(fmt::format(FY("Invalid boolean specified in '--aac-is-sbr {0}'.\n"), s));

  ti.m_all_aac_is_sbr[id] = (1 == parts.size()) || (parts[1] == "1");
}

static void
parse_arg_priority(const std::string &arg) {
  static const char *s_process_priorities[5] = {"lowest", "lower", "normal", "higher", "highest"};

  int i;
  for (i = 0; 5 > i; ++i)
    if ((arg == s_process_priorities[i])) {
      mtx::sys::set_process_priority(i - 2);
      return;
    }

  mxerror(fmt::format(FY("'{0}' is not a valid priority class.\n"), arg));
}

static mtx::bits::value_cptr
parse_segment_uid_or_read_from_file(std::string const &arg) {
  if ((arg.length() < 2) || (arg[0] != '='))
    return std::make_shared<mtx::bits::value_c>(arg, 128);

  auto file_name = std::string{&arg[1], arg.length() - 1};

  try {
    return kax_analyzer_c::read_segment_uid_from(file_name);

  } catch (mtx::kax_analyzer_x &ex) {
    mxerror(fmt::format("{0}\n", ex));
  }

  return {};
}

static void
parse_arg_previous_segment_uid(const std::string &param,
                               const std::string &arg) {
  if (g_seguid_link_previous)
    mxerror(fmt::format(FY("The previous UID was already given in '{0} {1}'.\n"), param, arg));

  try {
    g_seguid_link_previous = parse_segment_uid_or_read_from_file(arg);
  } catch (...) {
    mxerror(fmt::format(FY("Unknown format for the previous UID in '{0} {1}'.\n"), param, arg));
  }
}

static void
parse_arg_next_segment_uid(const std::string &param,
                           const std::string &arg) {
  if (g_seguid_link_next)
    mxerror(fmt::format(FY("The next UID was already given in '{0} {1}'.\n"), param, arg));

  try {
    g_seguid_link_next = parse_segment_uid_or_read_from_file(arg);
  } catch (...) {
    mxerror(fmt::format(FY("Unknown format for the next UID in '{0} {1}'.\n"), param, arg));
  }
}

static void
parse_arg_segment_uid(const std::string &param,
                      const std::string &arg) {
  auto parts = mtx::string::split(arg, ",");
  for (auto &part : parts) {
    try {
      g_forced_seguids.emplace_back(parse_segment_uid_or_read_from_file(part));
    } catch (...) {
      mxerror(fmt::format(FY("Unknown format for the segment UID '{2}' in '{0} {1}'.\n"), param, arg, part));
    }
  }
}

static void
parse_arg_cluster_length(std::string arg) {
  int idx = arg.find("ms");
  if (0 <= idx) {
    arg.erase(idx);
    int64_t max_ms_per_cluster;
    if (!mtx::string::parse_number(arg, max_ms_per_cluster) || (100 > max_ms_per_cluster) || (32000 < max_ms_per_cluster))
      mxerror(fmt::format(FY("Cluster length '{0}' out of range (100..32000).\n"), arg));

    g_max_ns_per_cluster     = max_ms_per_cluster * 1000000;
    g_max_blocks_per_cluster = 65535;

  } else {
    if (!mtx::string::parse_number(arg, g_max_blocks_per_cluster) || (0 > g_max_blocks_per_cluster) || (65535 < g_max_blocks_per_cluster))
      mxerror(fmt::format(FY("Cluster length '{0}' out of range (0..65535).\n"), arg));

    g_max_ns_per_cluster = 32000000000ull;
  }
}

static void
parse_arg_attach_file(attachment_cptr const &attachment,
                      const std::string &arg,
                      bool attach_once) {
  try {
    mm_file_io_c test(arg);
    auto size = test.get_size();

    if (size > 0x7fffffff)
      mxerror(fmt::format("{0} {1}\n",
                          fmt::format(FY("The attachment ({0}) is too big ({1})."), arg, mtx::string::format_file_size(size)),
                          Y("Only files smaller than 2 GiB are supported.")));

  } catch (...) {
    mxerror(fmt::format(FY("The file '{0}' cannot be attached because it does not exist or cannot be read.\n"), arg));
  }

  attachment->name         = arg;
  attachment->to_all_files = !attach_once;

  if (attachment->mime_type.empty())
    attachment->mime_type  = guess_mime_type_and_report(arg);

  try {
    mm_io_cptr io = mm_file_io_c::open(attachment->name);

    if (0 == io->get_size())
      mxerror(fmt::format(FY("The size of attachment '{0}' is 0.\n"), attachment->name));

    attachment->data = memory_c::alloc(io->get_size());
    io->read(attachment->data->get_buffer(), attachment->data->get_size());

  } catch (...) {
    mxerror(fmt::format(FY("The attachment '{0}' could not be read.\n"), attachment->name));
  }

  add_attachment(attachment);
}

static void
parse_arg_chapter_language(const std::string &arg,
                           track_info_c &ti) {
  if (g_chapter_language.is_valid())
    mxerror(fmt::format(FY("'--chapter-language' may only be given once in '--chapter-language {0}'.\n"), arg));

  if (!g_chapter_file_name.empty())
    mxerror(fmt::format(FY("'--chapter-language' must be given before '--chapters' in '--chapter-language {0}'.\n"), arg));

  g_chapter_language    = parse_language(arg, fmt::format("--chapter-language {0}", arg));
  ti.m_chapter_language = g_chapter_language;
}

static void
parse_arg_chapter_charset(const std::string &arg,
                          track_info_c &ti) {
  if (!g_chapter_charset.empty())
    mxerror(fmt::format(FY("'--chapter-charset' may only be given once in '--chapter-charset {0}'.\n"), arg));

  if (!g_chapter_file_name.empty())
    mxerror(fmt::format(FY("'--chapter-charset' must be given before '--chapters' in '--chapter-charset {0}'.\n"), arg));

  g_chapter_charset    = arg;
  ti.m_chapter_charset = arg;
}

static void
parse_arg_chapters(const std::string &param,
                   const std::string &arg,
                   track_info_c &ti) {
  static debugging_option_c s_debug{"dump_chapters_after_parsing"};

  if (g_chapter_file_name != "")
    mxerror(fmt::format(FY("Only one chapter file allowed in '{0} {1}'.\n"), param, arg));

  auto format         = mtx::chapters::format_e::xml;
  g_chapter_file_name = arg;
  g_kax_chapters      = mtx::chapters::parse(g_chapter_file_name, 0, -1, 0, g_chapter_language, g_chapter_charset, false, &format, &g_tags_from_cue_chapters);

  if (s_debug)
    dump_ebml_elements(g_kax_chapters.get());

  auto sync = ti.m_timestamp_syncs.find(track_info_c::chapter_track_id);
  if (sync == ti.m_timestamp_syncs.end())
    sync = ti.m_timestamp_syncs.find(track_info_c::all_tracks_id);

  if (sync != ti.m_timestamp_syncs.end()) {
    mtx::chapters::adjust_timestamps(*g_kax_chapters, sync->second.displacement, sync->second.factor);
    ti.m_timestamp_syncs.erase(sync);
  }

  if (g_segment_title_set || !g_tags_from_cue_chapters || (mtx::chapters::format_e::cue != format))
    return;

  auto cue_title = mtx::tags::get_simple_value("TITLE", *g_tags_from_cue_chapters);

  maybe_set_segment_title(cue_title);
}

static void
parse_arg_generate_chapters(std::string const &arg) {
  auto parts = mtx::string::split(arg, ":", 2);

  if (parts[0] == "when-appending") {
    g_cluster_helper->enable_chapter_generation(chapter_generation_mode_e::when_appending, g_chapter_language);
    g_chapter_language.clear();
    return;
  }

  if (parts[0] != "interval")
    mxerror(fmt::format("Invalid chapter generation mode in '--generate-chapters {0}'.\n", arg));

  if (parts.size() < 2)
    parts.emplace_back("");

  auto interval = int64_t{};
  if (!mtx::string::parse_timestamp(parts[1], interval) || (interval < 0))
    mxerror(fmt::format("The chapter generation interval must be a positive number in '--generate-chapters {0}'.\n", arg));

  g_cluster_helper->enable_chapter_generation(chapter_generation_mode_e::interval, g_chapter_language);
  g_cluster_helper->set_chapter_generation_interval(timestamp_c::ns(interval));
  g_chapter_language.clear();
}

static void
parse_arg_segmentinfo(const std::string &param,
                      const std::string &arg) {
  if (g_segmentinfo_file_name != "")
    mxerror(fmt::format(FY("Only one segment info file allowed in '{0} {1}'.\n"), param, arg));

  g_segmentinfo_file_name = arg;
  g_kax_info_chap         = mtx::xml::ebml_segmentinfo_converter_c::parse_file(arg, false);

  handle_segmentinfo();
}

static void
parse_arg_timestamp_scale(const std::string &arg) {
  if (TIMESTAMP_SCALE_MODE_NORMAL != g_timestamp_scale_mode)
    mxerror(Y("'--timestamp-scale' was used more than once.\n"));

  int64_t temp = 0;
  if (!mtx::string::parse_number(arg, temp))
    mxerror(Y("The argument to '--timestamp-scale' must be a number.\n"));

  if (-1 == temp)
    g_timestamp_scale_mode = TIMESTAMP_SCALE_MODE_AUTO;
  else {
    if ((10000000 < temp) || (1 > temp))
      mxerror(Y("The given timestamp scale factor is outside the valid range (1...10000000 or -1 for 'sample precision even if a video track is present').\n"));

    g_timestamp_scale      = temp;
    g_timestamp_scale_mode = TIMESTAMP_SCALE_MODE_FIXED;
  }
}

static void
parse_arg_default_language(const std::string &arg) {
  g_default_language = parse_language(arg, fmt::format("--default-language {0}", arg));
}

static void
parse_arg_attachments(const std::string &param,
                      std::string arg,
                      track_info_c &ti) {
  if (balg::starts_with(arg, "!")) {
    arg.erase(0, 1);
    ti.m_attach_mode_list.set_reversed();
  }

  auto elements = mtx::string::split(arg, ",");

  size_t i;
  for (i = 0; elements.size() > i; ++i) {
    auto pair = mtx::string::split(elements[i], ":");

    if (1 == pair.size())
      pair.push_back("all");

    else if (2 != pair.size())
      mxerror(fmt::format(FY("The argument '{0}' to '{1}' is invalid: too many colons in element '{2}'.\n"), arg, param, elements[i]));

    int64_t id;
    if (!mtx::string::parse_number(pair[0], id))
      mxerror(fmt::format(FY("The argument '{0}' to '{1}' is invalid: '{2}' is not a valid track ID.\n"), arg, param, pair[0]));

    if (pair[1] == "all")
      ti.m_attach_mode_list.add(id, ATTACH_MODE_TO_ALL_FILES);

    else if (pair[1] == "first")
      ti.m_attach_mode_list.add(id, ATTACH_MODE_TO_FIRST_FILE);

    else
      mxerror(fmt::format(FY("The argument '{0}' to '{1}' is invalid: '{2}' must be either 'all' or 'first'.\n"), arg, param, pair[1]));
  }
}

void
handle_file_name_arg(const std::string &this_arg,
                     std::vector<std::string>::const_iterator &sit,
                     std::vector<std::string>::const_iterator const &end,
                     bool &append_next_file,
                     std::unique_ptr<track_info_c> &&ti) {
  std::vector<std::string> file_names;
  auto append_files = false;

  if (mtx::included_in(this_arg, "(", "[")) {
    append_files   = this_arg == "[";
    bool end_found = false;
    auto end_char  = std::string{ append_files ? "]" : ")" };

    while ((sit + 1) < end) {
      sit++;
      if (*sit == end_char) {
        end_found = true;
        break;
      }
      file_names.push_back(*sit);
    }

    if (!end_found)
      mxerror(fmt::format(FY("The closing character '{0}' is missing.\n"), end_char));

    if (file_names.empty())
      mxerror(fmt::format(FY("No file names were listed between '{0}' and '{1}'.\n"), this_arg, end_char));

    if (append_files) {
      for (auto const &file_name : file_names) {
        handle_file_name_arg(file_name, sit, end, append_next_file, std::move(ti));

        append_next_file = true;
        ti               = std::make_unique<track_info_c>();
      }

      append_next_file = false;

      return;
    }

    ti->m_disable_multi_file = true;

  } else
    file_names.push_back(this_arg);

  for (auto &file_name : file_names) {
    if (file_name.empty())
      mxerror(Y("An empty file name is not valid.\n"));

    else if (g_outfile == file_name)
      mxerror(fmt::format("{0} {1}\n",
                          fmt::format(FY("The name of the destination file '{0}' and of one of the source files is the same."), g_outfile),
                          Y("This would cause mkvmerge to overwrite one of your source files.")));
  }

  if (!ti->m_atracks.empty() && ti->m_atracks.none())
    mxerror(Y("'-A' and '-a' used on the same source file.\n"));

  if (!ti->m_vtracks.empty() && ti->m_vtracks.none())
    mxerror(Y("'-D' and '-d' used on the same source file.\n"));

  if (!ti->m_stracks.empty() && ti->m_stracks.none())
    mxerror(Y("'-S' and '-s' used on the same source file.\n"));

  if (!ti->m_btracks.empty() && ti->m_btracks.none())
    mxerror(Y("'-B' and '-b' used on the same source file.\n"));

  auto file_p    = std::make_shared<filelist_t>();
  auto &file     = *file_p;
  file.all_names = file_names;
  file.name      = file_names[0];
  file.id        = g_files.size();

  if (file_names.size() == 1) {
    if ('+' == this_arg[0]) {
      append_next_file = true;
      file.name.erase(0, 1);

    } else if ('=' == this_arg[0]) {
      ti->m_disable_multi_file = true;
      file.name.erase(0, 1);
    }
  }

  if (append_next_file) {
    if (g_files.empty())
      mxerror(Y("The first file cannot be appended because there are no files to append to.\n"));

    file.appending   = true;
    append_next_file = false;
  }

  ti->m_fname = file.name;

  file.reader = probe_file_format(file);

  if (!file.reader)
    mxerror(fmt::format(FY("The type of file '{0}' could not be recognized.\n"), file.name));

  if (file.is_playlist) {
    file.name   = file.playlist_mpls_in->get_file_name();
    ti->m_fname = file.name;
  }

  file.ti.swap(ti);

  g_files.push_back(file_p);

  g_chapter_charset.clear();
  g_chapter_language.clear();
}

/** \brief Parses and handles command line arguments

   Also probes input files for their type and creates the appropriate
   file reader.

   Options are parsed in several passes because some options must be
   handled/known before others. The first pass finds
   '<tt>--identify</tt>'. The second pass handles options that only
   print some information and exit right afterwards
   (e.g. '<tt>--version</tt>' or '<tt>--list-types</tt>'). The third
   pass looks for '<tt>--output-file</tt>'. The fourth pass handles
   everything else.
*/
std::vector<std::string>
parse_common_args(std::vector<std::string> args) {
  set_usage();
  while (mtx::cli::handle_common_args(args, ""))
    set_usage();

  return args;
}

static void
parse_arg_identification_format(std::vector<std::string>::const_iterator &sit,
                                std::vector<std::string>::const_iterator const &sit_end) {
  if ((sit + 1) == sit_end)
    mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), *sit));

  auto next_arg = balg::to_lower_copy(*(sit + 1));

  if (next_arg == "text")
    g_identification_output_format = identification_output_format_e::text;

  else if (next_arg == "json") {
    g_identification_output_format = identification_output_format_e::json;
    redirect_warnings_and_errors_to_json();

  } else
    mxerror(fmt::format(FY("Invalid identification format in '{0} {1}'.\n"), *sit, *(sit + 1)));

  ++sit;
}

static void
parse_arg_probe_range(std::optional<std::string> next_arg) {
  if (!next_arg)
    mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), "--probe-range-percentage"));

  mtx_mp_rational_t probe_range_percentage{0, 1};
  if (   !mtx::string::parse_number_as_rational(*next_arg, probe_range_percentage)
      || !boost::multiprecision::denominator(probe_range_percentage)
      || (probe_range_percentage <= 0)
      || (probe_range_percentage  > 100))
    mxerror(fmt::format(FY("The probe range percentage '{0}' is invalid.\n"), *next_arg));

  generic_reader_c::set_probe_range_percentage(probe_range_percentage);
}

void
parse_normalize_language_ietf(std::optional<std::string> const &next_arg) {
  if (!next_arg || next_arg->empty())
    mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), "--normalize-language-ietf"));

  if (!mtx::included_in(*next_arg, "canonical"s, "extlang"s, "no"s, "off"s, "none"s))
    mxerror(fmt::format(FY("'{0}' is not a valid language normalization mode.\n"), *next_arg));

  auto mode = *next_arg == "canonical"s ? mtx::bcp47::normalization_mode_e::canonical
            : *next_arg == "extlang"s   ? mtx::bcp47::normalization_mode_e::extlang
            :                             mtx::bcp47::normalization_mode_e::none;

  mtx::bcp47::language_c::set_normalization_mode(mode);
}

static void
handle_identification_args(std::vector<std::string> &args) {
  auto identification_command = std::optional<std::string>{};
  auto file_to_identify       = std::optional<std::string>{};
  auto this_arg_itr           = args.begin();

  while (this_arg_itr != args.end()) {
    auto next_arg_itr = this_arg_itr + 1;

    std::optional<std::string> next_arg;
    if (next_arg_itr != args.end())
      next_arg = *next_arg_itr;

    if (*this_arg_itr == "--probe-range-percentage") {
      parse_arg_probe_range(next_arg);
      args.erase(this_arg_itr, next_arg_itr + 1);

    } else if (*this_arg_itr == "--normalize-language-ietf") {
      parse_normalize_language_ietf(*next_arg_itr);
      args.erase(this_arg_itr, next_arg_itr + 1);

    } else
      ++this_arg_itr;
  }

  for (auto const &this_arg : args) {
    if (!mtx::included_in(this_arg, "-i", "--identify", "-J"))
      continue;

    identification_command = this_arg;

    if (this_arg == "-J") {
      g_identification_output_format = identification_output_format_e::json;
      redirect_warnings_and_errors_to_json();
    }
  }

  if (!identification_command)
    return;

  for (auto sit = args.cbegin(), sit_end = args.cend(); sit != sit_end; sit++) {
    auto const &this_arg = *sit;

    if (mtx::included_in(this_arg, "-i", "--identify", "-J"))
      continue;

    if (mtx::included_in(this_arg, "-F", "--identification-format"))
      parse_arg_identification_format(sit, sit_end);

    else if (file_to_identify)
      mxerror(fmt::format(FY("The argument '{0}' is not allowed in identification mode.\n"), this_arg));

    else
      file_to_identify = this_arg;
  }

  if (!file_to_identify)
    mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), *identification_command));

  identify(*file_to_identify);
  mxexit();
}

static void
parse_args(std::vector<std::string> args) {
  handle_identification_args(args);

  // First parse options that either just print some infos and then exit.
  for (auto sit = args.cbegin(), sit_end = args.cend(); sit != sit_end; sit++) {
    auto const &this_arg = *sit;
    auto sit_next        = sit + 1;
    auto next_arg        = sit_next != sit_end ? *sit_next : "";

    if ((this_arg == "-l") || (this_arg == "--list-types")) {
      list_file_types();
      mxexit();

    } else if (this_arg == "--list-audio-emphasis") {
      audio_emphasis_c::list();
      mxexit();

    } else if (this_arg == "--list-languages") {
      mtx::iso639::list_languages();
      mxexit();

    } else if (this_arg == "--list-stereo-modes") {
      stereo_mode_c::list();
      mxexit();

    } else if (mtx::included_in(this_arg, "-i", "--identify", "-J"))
      mxerror(fmt::format(FY("'{0}' can only be used with a file name. No further options are allowed if this option is used.\n"), this_arg));

    else if (this_arg == "--capabilities") {
      print_capabilities();
      mxexit();

    }

  }

  mxinfo(fmt::format("{0}\n", get_version_info("mkvmerge", vif_full)));

  std::vector<std::string> unhandled_args;

  // Now parse options that are needed right at the beginning.
  for (auto sit = args.cbegin(), sit_end = args.cend(); sit != sit_end; sit++) {
    std::optional<std::string> next_arg;

    auto const &this_arg = *sit;
    auto sit_next        = sit + 1;
    auto num_handled     = 0;

    if (sit_next != sit_end)
      next_arg = *sit_next;

    if ((this_arg == "-o") || (this_arg == "--output")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks a file name.\n"), this_arg));

      if (g_outfile != "")
        mxerror(Y("Only one destination file allowed.\n"));

      g_outfile   = *next_arg;
      num_handled = 2;

    } else if (this_arg == "--generate-chapters-name-template") {
      if (!next_arg)
        mxerror(Y("'--generate-chapters-name-template' lacks the name template.\n"));

      mtx::chapters::g_chapter_generation_name_template.override(*next_arg);
      num_handled = 2;

    } else if ((this_arg == "-w") || (this_arg == "--webm")) {
      set_output_compatibility(OC_WEBM);
      num_handled = 1;

    } else if (this_arg == "--deterministic") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      g_deterministic = true;
      g_write_date    = false;
      auto seed       = mtx::checksum::calculate_as_uint(mtx::checksum::algorithm_e::adler32, next_arg->c_str(), next_arg->size());
      num_handled     = 2;
      random_c::init(seed);


    } else if (this_arg == "--disable-language-ietf") {
      mtx::bcp47::language_c::disable();
      num_handled = 1;

    } else if (this_arg == "--normalize-language-ietf") {
      parse_normalize_language_ietf(next_arg);
      num_handled = 2;

    } else if (this_arg == "--enable-legacy-font-mime-types") {
      g_use_legacy_font_mime_types = true;
      num_handled                  = 1;
    }

    if (num_handled == 2)
      ++sit;

    else if (!num_handled)
      unhandled_args.emplace_back(this_arg);
  }

  if (g_outfile.empty()) {
    mxinfo(Y("Error: no destination file name was given.\n\n"));
    mtx::cli::display_usage(2);
  }

  if (!outputting_webm() && is_webm_file_name(g_outfile)) {
    set_output_compatibility(OC_WEBM);
    mxinfo(fmt::format(FY("Automatically enabling WebM compliance mode due to destination file name extension.\n")));
  }

  auto ti               = std::make_unique<track_info_c>();
  bool inputs_found     = false;
  bool append_next_file = false;
  auto attachment       = std::make_shared<attachment_t>();

  for (auto sit = unhandled_args.cbegin(), sit_end = unhandled_args.cend(); sit != sit_end; sit++) {
    std::optional<std::string> next_arg;
    auto const &this_arg = *sit;
    auto sit_next        = sit + 1;

    if (sit_next != sit_end)
      next_arg = *sit_next;

    // Global options
    if ((this_arg == "--priority")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_priority(*next_arg);
      sit++;

    } else if ((this_arg == "-q") || (this_arg == "--quiet"))
      verbose = 0;

    else if ((this_arg == "-v") || (this_arg == "--verbose"))
      verbose++;

    else if (this_arg == "--title") {
      if (!next_arg)
        mxerror(Y("'--title' lacks the title.\n"));

      g_segment_title     = *next_arg;
      g_segment_title_set = true;
      sit++;

    } else if (this_arg == "--split") {
      if (!next_arg || next_arg->empty())
        mxerror(Y("'--split' lacks the size.\n"));

      parse_arg_split(*next_arg);
      sit++;

    } else if (this_arg == "--split-max-files") {
      if (!next_arg || next_arg->empty())
        mxerror(Y("'--split-max-files' lacks the number of files.\n"));

      if (!mtx::string::parse_number(*next_arg, g_split_max_num_files) || (2 > g_split_max_num_files))
        mxerror(Y("Wrong argument to '--split-max-files'.\n"));

      sit++;

    } else if (this_arg == "--link") {
      g_no_linking = false;

    } else if (this_arg == "--link-to-previous") {
      if (!next_arg || next_arg->empty())
        mxerror(Y("'--link-to-previous' lacks the previous UID.\n"));

      parse_arg_previous_segment_uid(this_arg, *next_arg);
      sit++;

    } else if (this_arg == "--link-to-next") {
      if (!next_arg || next_arg->empty())
        mxerror(Y("'--link-to-next' lacks the next UID.\n"));

      parse_arg_next_segment_uid(this_arg, *next_arg);
      sit++;

    } else if (this_arg == "--segment-uid") {
      if (!next_arg || next_arg->empty())
        mxerror(Y("'--segment-uid' lacks the segment UID.\n"));

      parse_arg_segment_uid(this_arg, *next_arg);
      sit++;

    } else if (this_arg == "--cluster-length") {
      if (!next_arg)
        mxerror(Y("'--cluster-length' lacks the length.\n"));

      parse_arg_cluster_length(*next_arg);
      sit++;

    } else if (this_arg == "--no-cues")
      g_write_cues = false;

    else if (this_arg == "--no-date")
      g_write_date = false;

    else if (this_arg == "--clusters-in-meta-seek")
      g_write_meta_seek_for_clusters = true;

    else if (this_arg == "--disable-lacing")
      g_no_lacing = true;

    else if (this_arg == "--enable-durations")
      g_use_durations = true;

    else if (this_arg == "--disable-track-statistics-tags")
      g_no_track_statistics_tags = true;

    else if (this_arg == "--stop-after-video-ends")
      g_stop_after_video_ends = true;

    else if (this_arg == "--attachment-description") {
      if (!next_arg)
        mxerror(Y("'--attachment-description' lacks the description.\n"));

      if (attachment->description != "")
        mxwarn(Y("More than one description was given for a single attachment.\n"));
      attachment->description = *next_arg;
      sit++;

    } else if (this_arg == "--attachment-mime-type") {
      if (!next_arg)
        mxerror(Y("'--attachment-mime-type' lacks the MIME type.\n"));

      if (attachment->mime_type != "")
        mxwarn(fmt::format(FY("More than one MIME type was given for a single attachment. '{0}' will be discarded and '{1}' used instead.\n"),
                           attachment->mime_type, *next_arg));
      attachment->mime_type = *next_arg;
      sit++;

    } else if (this_arg == "--attachment-name") {
      if (!next_arg)
        mxerror(Y("'--attachment-name' lacks the name.\n"));

      if (attachment->stored_name != "")
        mxwarn(fmt::format(FY("More than one name was given for a single attachment. '{0}' will be discarded and '{1}' used instead.\n"),
                           attachment->stored_name, *next_arg));
      attachment->stored_name = *next_arg;
      sit++;

    } else if ((this_arg == "--attach-file") || (this_arg == "--attach-file-once")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the file name.\n"), this_arg));

      parse_arg_attach_file(attachment, *next_arg, this_arg == "--attach-file-once");
      attachment = std::make_shared<attachment_t>();
      sit++;

      inputs_found = true;

    } else if (this_arg == "--global-tags") {
      if (!next_arg)
        mxerror(Y("'--global-tags' lacks the file name.\n"));

      parse_and_add_tags(*next_arg);
      sit++;

      inputs_found = true;

    } else if (this_arg == "--chapter-language") {
      if (!next_arg)
        mxerror(Y("'--chapter-language' lacks the language.\n"));

      parse_arg_chapter_language(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--chapter-charset") {
      if (!next_arg)
        mxerror(Y("'--chapter-charset' lacks the charset.\n"));

      parse_arg_chapter_charset(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--chapter-sync") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the delay.\n"), this_arg));

      parse_arg_sync(*next_arg, this_arg, *ti, track_info_c::chapter_track_id);
      sit++;

    } else if (this_arg == "--cue-chapter-name-format") {
      if (!next_arg)
        mxerror(Y("'--cue-chapter-name-format' lacks the format.\n"));

      if (g_chapter_file_name != "")
        mxerror(Y("'--cue-chapter-name-format' must be given before '--chapters'.\n"));

      mtx::chapters::g_cue_name_format = *next_arg;
      sit++;

    } else if (this_arg == "--chapters") {
      if (!next_arg)
        mxerror(Y("'--chapters' lacks the file name.\n"));

      parse_arg_chapters(this_arg, *next_arg, *ti);
      sit++;

      inputs_found = true;

    } else if (this_arg == "--generate-chapters") {
      if (!next_arg)
        mxerror(Y("'--generate-chapters' lacks the mode.\n"));

      parse_arg_generate_chapters(*next_arg);
      sit++;

    } else if (this_arg == "--segmentinfo") {
      if (!next_arg)
        mxerror(Y("'--segmentinfo' lacks the file name.\n"));

      parse_arg_segmentinfo(this_arg, *next_arg);
      sit++;

      inputs_found = true;

    } else if (this_arg == "--no-chapters")
      ti->m_no_chapters = true;

    else if ((this_arg == "-M") || (this_arg == "--no-attachments"))
      ti->m_attach_mode_list.set_none();

    else if ((this_arg == "-m") || (this_arg == "--attachments")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_attachments(this_arg, *next_arg, *ti);
      sit++;

    } else if (this_arg == "--no-global-tags")
      ti->m_no_global_tags = true;

    else if (this_arg == "--regenerate-track-uids")
      ti->m_regenerate_track_uids = true;

    else if (this_arg == "--meta-seek-size") {
      mxwarn(Y("The option '--meta-seek-size' is no longer supported. Please read mkvmerge's documentation, especially the section about the MATROSKA FILE LAYOUT.\n"));
      sit++;

    } else if (mtx::included_in(this_arg, "--timecode-scale", "--timestamp-scale")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_timestamp_scale(*next_arg);
      sit++;
    }

    // Options that apply to the next input file only.
    else if ((this_arg == "-A") || (this_arg == "--noaudio") || (this_arg == "--no-audio"))
      ti->m_atracks.set_none();

    else if ((this_arg == "-D") || (this_arg == "--novideo") || (this_arg == "--no-video"))
      ti->m_vtracks.set_none();

    else if ((this_arg == "-S") || (this_arg == "--nosubs") || (this_arg == "--no-subs") || (this_arg == "--no-subtitles"))
      ti->m_stracks.set_none();

    else if ((this_arg == "-B") || (this_arg == "--nobuttons") || (this_arg == "--no-buttons"))
      ti->m_btracks.set_none();

    else if ((this_arg == "-T") || (this_arg == "--no-track-tags"))
      ti->m_track_tags.set_none();

    else if ((this_arg == "-a") || (this_arg == "--atracks") || (this_arg == "--audio-tracks")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the track numbers.\n"), this_arg));

      parse_arg_tracks(*next_arg, ti->m_atracks, this_arg);
      sit++;

    } else if ((this_arg == "-d") || (this_arg == "--vtracks") || (this_arg == "--video-tracks")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the track numbers.\n"), this_arg));

      parse_arg_tracks(*next_arg, ti->m_vtracks, this_arg);
      sit++;

    } else if ((this_arg == "-s") || (this_arg == "--stracks") || (this_arg == "--sub-tracks") || (this_arg == "--subtitle-tracks")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the track numbers.\n"), this_arg));

      parse_arg_tracks(*next_arg, ti->m_stracks, this_arg);
      sit++;

    } else if ((this_arg == "-b") || (this_arg == "--btracks") || (this_arg == "--button-tracks")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the track numbers.\n"), this_arg));

      parse_arg_tracks(*next_arg, ti->m_btracks, this_arg);
      sit++;

    } else if ((this_arg == "--track-tags")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the track numbers.\n"), this_arg));

      parse_arg_tracks(*next_arg, ti->m_track_tags, this_arg);
      sit++;

    } else if ((this_arg == "-f") || (this_arg == "--fourcc")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the FourCC.\n"), this_arg));

      parse_arg_fourcc(*next_arg, this_arg, *ti);
      sit++;

    } else if (this_arg == "--aspect-ratio") {
      if (!next_arg)
        mxerror(Y("'--aspect-ratio' lacks the aspect ratio.\n"));

      parse_arg_aspect_ratio(*next_arg, this_arg, false, *ti);
      sit++;

    } else if (this_arg == "--aspect-ratio-factor") {
      if (!next_arg)
        mxerror(Y("'--aspect-ratio-factor' lacks the aspect ratio factor.\n"));

      parse_arg_aspect_ratio(*next_arg, this_arg, true, *ti);
      sit++;

    } else if (this_arg == "--display-dimensions") {
      if (!next_arg)
        mxerror(Y("'--display-dimensions' lacks the dimensions.\n"));

      parse_arg_display_dimensions(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--cropping") {
      if (!next_arg)
        mxerror(Y("'--cropping' lacks the crop parameters.\n"));

      parse_arg_cropping(*next_arg, *ti);
      sit++;

    } else if (mtx::included_in(this_arg, "--color-matrix", "--color-matrix-coefficients", "--colour-matrix", "--colour-matrix-coefficients")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_color_matrix_coefficients(*next_arg, *ti);
      sit++;

    } else if (mtx::included_in(this_arg, "--color-bits-per-channel", "--colour-bits-per-channel")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_color_bits_per_channel(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--chroma-subsample") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_chroma_subsample(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--cb-subsample") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_cb_subsample(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--chroma-siting") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_chroma_siting(*next_arg, *ti);
      sit++;

    } else if (mtx::included_in(this_arg, "--color-range", "--colour-range")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_color_range(*next_arg, *ti);
      sit++;

    } else if (mtx::included_in(this_arg, "--color-transfer-characteristics", "--colour-transfer-characteristics")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_color_transfer(*next_arg, *ti);
      sit++;

    } else if (mtx::included_in(this_arg, "--color-primaries", "--colour-primaries")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_color_primaries(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--max-content-light") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_max_content_light(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--max-frame-light") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_max_frame_light(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--chromaticity-coordinates") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_chroma_coordinates(*next_arg, *ti);
      sit++;

    } else if (mtx::included_in(this_arg, "--white-color-coordinates", "--white-colour-coordinates")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_white_coordinates(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--max-luminance") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_max_luminance(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--min-luminance") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_min_luminance(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--projection-type") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_projection_type(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--projection-private") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_projection_private(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--projection-pose-yaw") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_projection_pose_xyz(*next_arg, ti->m_projection_pose_yaw_list, "yaw");
      sit++;

    } else if (this_arg == "--projection-pose-pitch") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_projection_pose_xyz(*next_arg, ti->m_projection_pose_pitch_list, "pitch");
      sit++;

    } else if (this_arg == "--projection-pose-roll") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_projection_pose_xyz(*next_arg, ti->m_projection_pose_roll_list, "roll");
      sit++;

    } else if (this_arg == "--field-order") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the parameter.\n"), this_arg));

      parse_arg_field_order(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--stereo-mode") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_stereo_mode(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--audio-emphasis") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_audio_emphasis(*next_arg, *ti);
      sit++;

    } else if ((this_arg == "-y") || (this_arg == "--sync")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the delay.\n"), this_arg));

      parse_arg_sync(*next_arg, this_arg, *ti, std::nullopt);
      sit++;

    } else if (this_arg == "--cues") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_cues(*next_arg, *ti);
      sit++;

    } else if ((this_arg == "--default-track-flag") || (this_arg == "--default-track")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_default_track_flags);
      sit++;

    } else if ((this_arg == "--forced-display-flag") || (this_arg == "--forced-track")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_forced_track_flags);
      sit++;

    } else if (this_arg == "--track-enabled-flag") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_enabled_track_flags);
      sit++;

    } else if (this_arg == "--hearing-impaired-flag") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_hearing_impaired_flags);
      sit++;

    } else if (this_arg == "--visual-impaired-flag") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_visual_impaired_flags);
      sit++;

    } else if (this_arg == "--text-descriptions-flag") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_text_descriptions_flags);
      sit++;

    } else if (this_arg == "--original-flag") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_original_flags);
      sit++;

    } else if (this_arg == "--commentary-flag") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_commentary_flags);
      sit++;

    } else if (this_arg == "--language") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_language(*next_arg, ti->m_languages, fmt::format("--language {0}", *next_arg));
      sit++;

    } else if (this_arg == "--default-language") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_default_language(*next_arg);
      sit++;

    } else if ((this_arg == "--sub-charset") || (this_arg == "--subtitle-charset")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_sub_charset(*next_arg, *ti);
      sit++;

    } else if ((this_arg == "-t") || (this_arg == "--tags")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_tags(*next_arg, this_arg, *ti);
      sit++;

    } else if (this_arg == "--aac-is-sbr") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks the track ID.\n"), this_arg));

      parse_arg_aac_is_sbr(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--compression") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_compression(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--track-name") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      auto [tid, string]     = parse_arg_tid_and_string(*next_arg, "track-name", Y("track name"), true);
      ti->m_track_names[tid] = string;
      sit++;

    } else if (mtx::included_in(this_arg, "--timecodes", "--timestamps")) {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      auto [tid, string]            = parse_arg_tid_and_string(*next_arg, this_arg.substr(2), Y("timestamps"));
      ti->m_all_ext_timestamps[tid] = string;
      sit++;

    } else if (this_arg == "--track-order") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      if (!g_track_order.empty())
        mxerror(Y("'--track-order' may only be given once.\n"));

      parse_arg_track_order(*next_arg);
      sit++;

    } else if (this_arg == "--append-to") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_append_to(*next_arg);
      sit++;

    } else if (this_arg == "--append-mode") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_append_mode(*next_arg);
      sit++;

    } else if (this_arg == "--default-duration") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_default_duration(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--nalu-size-length") {
      // Option was removed. Recognize it for not breaking existing
      // applications.
      sit++;

    } else if (this_arg == "--fix-bitstream-timing-information") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_boolean_track_option(this_arg, *next_arg, ti->m_fix_bitstream_frame_rate_flags);
      sit++;

    } else if (this_arg == "--reduce-to-core") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_reduce_to_core(*next_arg, *ti);
      sit++;

    } else if (this_arg == "--remove-dialog-normalization-gain") {
      if (!next_arg)
        mxerror(fmt::format(FY("'{0}' lacks its argument.\n"), this_arg));

      parse_arg_remove_dialog_normalization_gain(*next_arg, *ti);
      sit++;

    } else if (this_arg == "+")
      append_next_file = true;

    else if (this_arg == "=")
      ti->m_disable_multi_file = true;

    // The argument is an input file.
    else {
      handle_file_name_arg(this_arg, sit, sit_end, append_next_file, std::move(ti));
      ti = std::make_unique<track_info_c>();
    }
  }

  if (!g_cluster_helper->splitting() && !g_no_linking)
    mxwarn(Y("'--link' is only useful in combination with '--split'.\n"));

  if (!inputs_found && g_files.empty())
    mxerror(Y("No source files were given.\n"));
}

static void
display_playlist_scan_progress(size_t num_scanned,
                               size_t total_num_to_scan) {
  static auto s_no_progress = debugging_option_c{"no_progress"};

  if (s_no_progress)
    return;

  auto current_percentage = (num_scanned * 1000 + 5) / total_num_to_scan / 10;

  if (mtx::cli::g_gui_mode)
    mxinfo(fmt::format("#GUI#progress {0}%\n", current_percentage));
  else
    mxinfo(fmt::format(FY("Progress: {0}%{1}"), current_percentage, "\r"));
}

static filelist_cptr
create_filelist_for_playlist(boost::filesystem::path const &file_name,
                             size_t previous_filelist_id,
                             size_t current_filelist_id,
                             size_t idx,
                             track_info_c const &src_ti) {
  auto new_filelist_p                        = std::make_shared<filelist_t>();
  auto &new_filelist                         = *new_filelist_p;
  new_filelist.name                          = file_name.string();
  new_filelist.all_names                     = std::vector<std::string>{ new_filelist.name };
  new_filelist.size                          = boost::filesystem::file_size(file_name);
  new_filelist.id                            = current_filelist_id;
  new_filelist.appending                     = true;
  new_filelist.is_playlist                   = true;
  new_filelist.playlist_index                = idx;
  new_filelist.playlist_previous_filelist_id = previous_filelist_id;

  new_filelist.reader                        = probe_file_format(new_filelist);

  if (!new_filelist.reader)
    mxerror(fmt::format(FY("The type of file '{0}' could not be recognized.\n"), new_filelist.name));

  new_filelist.ti                       = std::make_unique<track_info_c>();
  new_filelist.ti->m_fname              = new_filelist.name;
  new_filelist.ti->m_disable_multi_file = true;
  new_filelist.ti->m_no_chapters        = true;
  new_filelist.ti->m_no_global_tags     = true;
  new_filelist.ti->m_atracks            = src_ti.m_atracks;
  new_filelist.ti->m_vtracks            = src_ti.m_vtracks;
  new_filelist.ti->m_stracks            = src_ti.m_stracks;
  new_filelist.ti->m_btracks            = src_ti.m_btracks;
  new_filelist.ti->m_track_tags         = src_ti.m_track_tags;

  return new_filelist_p;
}

static void
add_filelists_for_playlists() {
  auto num_playlists = 0u, num_files_in_playlists = 0u;

  for (auto const &filelist : g_files) {
    if (!filelist->is_playlist)
      continue;

    num_playlists          += 1;
    num_files_in_playlists += filelist->playlist_mpls_in->get_file_names().size();
  }

  if (num_playlists == num_files_in_playlists)
    return;

  if (mtx::cli::g_gui_mode)
    mxinfo(fmt::format("#GUI#begin_scanning_playlists#num_playlists={0}#num_files_in_playlists={1}\n", num_playlists, num_files_in_playlists));
  mxinfo(fmt::format(FNY("Scanning {0} files in {1} playlist.\n", "Scanning {0} files in {1} playlists.\n", num_playlists), num_files_in_playlists, num_playlists));

  std::vector<filelist_cptr> new_filelists;
  auto num_scanned_playlists = 0u;

  display_playlist_scan_progress(0, num_files_in_playlists);

  for (auto const &filelist : g_files) {
    if (!filelist->is_playlist)
      continue;

    auto &file_names          = filelist->playlist_mpls_in->get_file_names();
    auto previous_filelist_id = filelist->id;
    auto const &play_items    = filelist->playlist_mpls_in->get_mpls_parser().get_playlist().items;

    assert(file_names.size() == play_items.size());

    filelist->restricted_timestamp_min = play_items[0].in_time;
    filelist->restricted_timestamp_max = play_items[0].out_time;

    for (int idx = 1, idx_end = file_names.size(); idx < idx_end; ++idx) {
      auto current_filelist_id               = g_files.size() + new_filelists.size();
      auto new_filelist                      = create_filelist_for_playlist(file_names[idx], previous_filelist_id, current_filelist_id, idx, *filelist->ti);
      new_filelist->restricted_timestamp_min = play_items[idx].in_time;
      new_filelist->restricted_timestamp_max = play_items[idx].out_time;

      new_filelists.push_back(new_filelist);

      previous_filelist_id = new_filelist->id;

      display_playlist_scan_progress(++num_scanned_playlists, num_files_in_playlists);
    }
  }

  std::copy(new_filelists.begin(), new_filelists.end(), std::back_inserter(g_files));

  display_playlist_scan_progress(num_files_in_playlists, num_files_in_playlists);

  if (mtx::cli::g_gui_mode)
    mxinfo("#GUI#end_scanning_playlists\n");
  mxinfo(fmt::format(FY("Done scanning playlists.\n")));
}

static void
create_append_mappings_for_playlists() {
  for (auto &src_file : g_files) {
    if (!src_file->is_playlist || !src_file->playlist_index)
      continue;

    // Default mapping.
    for (auto track_id : src_file->reader->m_used_track_ids) {
      append_spec_t new_amap;

      new_amap.src_file_id  = src_file->id;
      new_amap.src_track_id = track_id;
      new_amap.dst_file_id  = src_file->playlist_previous_filelist_id;
      new_amap.dst_track_id = track_id;

      g_append_mapping.push_back(new_amap);
    }
  }
}

static void
check_for_unused_chapter_numbers_while_spliting_by_chapters() {
  std::optional<unsigned int> smallest_unused_chapter_number, largest_existing_chapter_number;

  for (auto &item : g_splitting_by_chapter_numbers) {
    // 0 == exists but not requested
    // 1 == requested
    // 2 == exists and requested
    if (   (item.second != 1)
        && (   !largest_existing_chapter_number
            || (item.first > *largest_existing_chapter_number)))
      largest_existing_chapter_number = item.first;

    if (   (item.second == 1)
        && (   !smallest_unused_chapter_number
            || (item.first < *smallest_unused_chapter_number)))
      smallest_unused_chapter_number = item.first;
  }

  if (!smallest_unused_chapter_number)
    return;

  auto details = largest_existing_chapter_number
    ? fmt::format(FNY("Only {0} chapter found in source files & chapter files.", "Only {0} chapters found in source files & chapter files.", *largest_existing_chapter_number), *largest_existing_chapter_number)
    : Y("There are no chapters in source files & chapter files.");

  mxwarn(fmt::format(FY("Invalid chapter number '{0}' for '--split' in '--split {1}': {2}\n"), *smallest_unused_chapter_number, s_split_by_chapters_arg, details));
}

/** \brief Global program initialization

   Both platform dependant and independant initialization is done here.
   For Unix like systems a signal handler is installed. The locale's
   \c LC_MESSAGES is set.
*/
static std::vector<std::string>
setup(int argc,
      char **argv) {
  clear_list_of_unique_numbers(UNIQUE_ALL_IDS);

  mtx_common_init("mkvmerge", argv[0]);

  g_kax_tracks       = std::make_unique<libmatroska::KaxTracks>();
  g_default_language = mtx::bcp47::language_c::parse("und");

#if defined(SYS_UNIX) || defined(SYS_APPLE)
  signal(SIGUSR1, sighandler);
  signal(SIGINT, sighandler);
#endif

  auto args = parse_common_args(mtx::cli::args_in_utf8(argc, argv));

  g_cluster_helper = std::make_unique<cluster_helper_c>();

  return args;
}

/** \brief Setup and high level program control

   Calls the functions for setup, handling the command line arguments,
   creating the readers, the main loop, finishing the current output
   file and cleaning up.
*/
int
main(int argc,
     char **argv) {
  mxrun_before_exit(cleanup);

  auto args = setup(argc, argv);

  parse_args(args);

  int64_t start = mtx::sys::get_current_time_millis();

  add_filelists_for_playlists();
  read_file_headers();

  if (!g_identifying) {
    create_packetizers();
    check_track_id_validity();
    create_append_mappings_for_playlists();
    check_append_mapping();
    check_split_support();
    g_cluster_helper->verify_and_report_chapter_generation_parameters();
    calc_attachment_sizes();
    calc_max_chapter_size();
  }

  add_split_points_from_remainig_chapter_numbers();

  g_cluster_helper->dump_split_points();

  try {
    create_next_output_file();
    main_loop();
    finish_file(true);
  } catch (mtx::mm_io::exception &ex) {
    force_close_output_file();
    mxerror(fmt::format("{0} {1} {2} {3}; {4}\n",
                        Y("An exception occurred when writing the destination file."), Y("The drive may be full."), Y("Exception details:"),
                        ex.what(), ex.error()));
  }

  check_for_unused_chapter_numbers_while_spliting_by_chapters();

  mxinfo(fmt::format(FY("Multiplexing took {0}.\n"), mtx::string::create_minutes_seconds_time_string((mtx::sys::get_current_time_millis() - start + 500) / 1000, true)));

  cleanup();

  mxexit();
}
