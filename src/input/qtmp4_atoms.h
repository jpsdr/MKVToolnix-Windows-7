/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   structs for various Quicktime and MP4 atoms

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

// 'Movie header' atom
#if defined(COMP_MSC)
#pragma pack(push,1)
#endif
struct PACKED_STRUCTURE mvhd_atom_t {
  uint8_t  version;
  uint8_t  flags[3];
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t time_scale;
  uint32_t duration;
  uint32_t preferred_rate;
  uint16_t preferred_volume;
  uint8_t  reserved[10];
  int32_t  display_matrix[3][3];
  uint32_t preview_time;
  uint32_t preview_duration;
  uint32_t poster_time;
  uint32_t selection_time;
  uint32_t selection_duration;
  uint32_t current_time;
  uint32_t next_track_id;
};

// 'Media header' atom
struct PACKED_STRUCTURE mdhd_atom_t {
  uint8_t  version;              // == 0
  uint8_t  flags[3];
  uint32_t creation_time;
  uint32_t modification_time;
  uint32_t time_scale;
  uint32_t duration;
  uint16_t language;
  uint16_t quality;
};

// 'Media header' atom, 64bit version
struct PACKED_STRUCTURE mdhd64_atom_t {
  uint8_t  version;              // == 1
  uint8_t  flags[3];
  uint64_t creation_time;
  uint64_t modification_time;
  uint32_t time_scale;
  uint64_t duration;
  uint16_t language;
  uint16_t quality;
};

// 'Handler reference' atom
struct PACKED_STRUCTURE hdlr_atom_t {
  uint8_t  version;
  uint8_t  flags[3];
  uint32_t type;
  uint32_t subtype;
  uint32_t manufacturer;
  uint32_t flags2;
  uint32_t flags_mask;
};

// Base for all 'sample data description' atoms
struct PACKED_STRUCTURE base_stsd_atom_t {
  uint32_t size;
  char     fourcc[4];
  uint8_t  reserved[6];
  uint16_t data_reference_index;
};

// 'sound sample description' atom
struct PACKED_STRUCTURE sound_v0_stsd_atom_t {
  base_stsd_atom_t base;
  uint16_t         version;
  uint16_t         revision;
  uint32_t         vendor;
  uint16_t         channels;
  uint16_t         sample_size;
  int16_t          compression_id;
  uint16_t         packet_size;
  uint32_t         sample_rate;         // 32bit fixed-point number
};

// 'sound sample description' atom v1
struct PACKED_STRUCTURE sound_v1_stsd_atom_t {
  sound_v0_stsd_atom_t v0;
  struct PACKED_STRUCTURE {
    uint32_t samples_per_packet;
    uint32_t bytes_per_packet;
    uint32_t bytes_per_frame;
    uint32_t bytes_per_sample;
  } v1;
};

// 'sound sample description' atom v2
struct PACKED_STRUCTURE sound_v2_stsd_atom_t {
  sound_v0_stsd_atom_t v0;
  struct PACKED_STRUCTURE {
    uint32_t v2_struct_size;
    uint64_t sample_rate;       // 64bit float
    // uint32_t unknown1;
    uint32_t channels;

    // 16
    uint32_t const1;            // always 0x7f000000
    uint32_t bits_per_channel;  // for uncompressed audio
    uint32_t flags;
    uint32_t bytes_per_frame;   // if constant
    uint32_t samples_per_frame; // lpcm frames per audio packet if constant
  } v2;
};

// 'video sample description' atom
struct PACKED_STRUCTURE video_stsd_atom_t {
  base_stsd_atom_t base;
  uint16_t         version;
  uint16_t         revision;
  uint32_t         vendor;
  uint32_t         temporal_quality;
  uint32_t         spatial_quality;
  uint16_t         width;
  uint16_t         height;
  uint32_t         horizontal_resolution; // 32bit fixed-point number
  uint32_t         vertical_resolution;   // 32bit fixed-point number
  uint32_t         data_size;
  uint16_t         frame_count;
  char             compressor_name[32];
  uint16_t         depth;
  uint16_t         color_table_id;
};

// 'color information' atom
struct PACKED_STRUCTURE colr_atom_t {
  uint32_t color_type;
  uint16_t color_primaries;
  uint16_t transfer_characteristics;
  uint16_t matrix_coefficients;
};

struct PACKED_STRUCTURE qt_image_description_t {
  uint32_t size;
  video_stsd_atom_t id;
};
#if defined(COMP_MSC)
#pragma pack(pop)
#endif

// MPEG4 esds structure
struct esds_t {
  uint8_t        version{};
  uint32_t       flags{};
  uint16_t       esid{};
  uint8_t        stream_priority{};
  uint8_t        object_type_id{};
  uint8_t        stream_type{};
  uint32_t       buffer_size_db{};
  uint32_t       max_bitrate{};
  uint32_t       avg_bitrate{};
  memory_cptr    decoder_config;
  memory_cptr    sl_config;
};
