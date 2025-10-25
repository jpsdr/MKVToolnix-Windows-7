/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   common definitions for MP4 files

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

namespace mtx::mp4 {

// Object type identifications.
// See http://gpac.sourceforge.net/tutorial/mediatypes.htm
constexpr auto OBJECT_TYPE_MPEG4Systems1                   = 0x01;
constexpr auto OBJECT_TYPE_MPEG4Systems2                   = 0x02;
constexpr auto OBJECT_TYPE_MPEG4Visual                     = 0x20;
constexpr auto OBJECT_TYPE_MPEG4Audio                      = 0x40;
constexpr auto OBJECT_TYPE_MPEG2VisualSimple               = 0x60;
constexpr auto OBJECT_TYPE_MPEG2VisualMain                 = 0x61;
constexpr auto OBJECT_TYPE_MPEG2VisualSNR                  = 0x62;
constexpr auto OBJECT_TYPE_MPEG2VisualSpatial              = 0x63;
constexpr auto OBJECT_TYPE_MPEG2VisualHigh                 = 0x64;
constexpr auto OBJECT_TYPE_MPEG2Visual422                  = 0x65;
constexpr auto OBJECT_TYPE_MPEG2AudioMain                  = 0x66;
constexpr auto OBJECT_TYPE_MPEG2AudioLowComplexity         = 0x67;
constexpr auto OBJECT_TYPE_MPEG2AudioScaleableSamplingRate = 0x68;
constexpr auto OBJECT_TYPE_MPEG2AudioPart3                 = 0x69;
constexpr auto OBJECT_TYPE_MPEG1Visual                     = 0x6A;
constexpr auto OBJECT_TYPE_MPEG1Audio                      = 0x6B;
constexpr auto OBJECT_TYPE_JPEG                            = 0x6C;
constexpr auto OBJECT_TYPE_DTS                             = 0xA9;
constexpr auto OBJECT_TYPE_VORBIS                          = 0xDD;
constexpr auto OBJECT_TYPE_VOBSUB                          = 0xE0;

// Audio object type identifactors
// This list comes from
constexpr auto AUDIO_OBJECT_TYPE_AAC_MAIN                  = 0x01; // Main
constexpr auto AUDIO_OBJECT_TYPE_AAC_LC                    = 0x02; // Low Complexity
constexpr auto AUDIO_OBJECT_TYPE_AAC_SSR                   = 0x03; // Scalable Sample Rate
constexpr auto AUDIO_OBJECT_TYPE_AAC_LTP                   = 0x04; // Long Term Prediction
constexpr auto AUDIO_OBJECT_TYPE_SBR                       = 0x05; // Spectral Band Replication
constexpr auto AUDIO_OBJECT_TYPE_AAC_SCALABLE              = 0x06; // Scalable
constexpr auto AUDIO_OBJECT_TYPE_TWINVQ                    = 0x07; // TwinVQ Vector Quantizer
constexpr auto AUDIO_OBJECT_TYPE_CELP                      = 0x08; // Code Excited Linear Prediction
constexpr auto AUDIO_OBJECT_TYPE_HVXC                      = 0x09; // Harmonic Vector eXcitation Coding
constexpr auto AUDIO_OBJECT_TYPE_TTSI                      = 0x0c; // Text-To-Speech Interface
constexpr auto AUDIO_OBJECT_TYPE_MAINSYNTH                 = 0x0d; // Main Synthesis
constexpr auto AUDIO_OBJECT_TYPE_WAVESYNTH                 = 0x0e; // Wavetable Synthesis
constexpr auto AUDIO_OBJECT_TYPE_MIDI                      = 0x0f; // General MIDI
constexpr auto AUDIO_OBJECT_TYPE_SAFX                      = 0x10; // Algorithmic Synthesis and Audio Effects
constexpr auto AUDIO_OBJECT_TYPE_ER_AAC_LC                 = 0x11; // Error Resilient Low Complexity
constexpr auto AUDIO_OBJECT_TYPE_ER_AAC_LTP                = 0x13; // Error Resilient Long Term Prediction
constexpr auto AUDIO_OBJECT_TYPE_ER_AAC_SCALABLE           = 0x14; // Error Resilient Scalable
constexpr auto AUDIO_OBJECT_TYPE_ER_TWINVQ                 = 0x15; // Error Resilient Twin Vector Quantizer
constexpr auto AUDIO_OBJECT_TYPE_ER_BSAC                   = 0x16; // Error Resilient Bit-Sliced Arithmetic Coding
constexpr auto AUDIO_OBJECT_TYPE_ER_AAC_LD                 = 0x17; // Error Resilient Low Delay
constexpr auto AUDIO_OBJECT_TYPE_ER_CELP                   = 0x18; // Error Resilient Code Excited Linear Prediction
constexpr auto AUDIO_OBJECT_TYPE_ER_HVXC                   = 0x19; // Error Resilient Harmonic Vector eXcitation Coding
constexpr auto AUDIO_OBJECT_TYPE_ER_HILN                   = 0x1a; // Error Resilient Harmonic and Individual Lines plus Noise
constexpr auto AUDIO_OBJECT_TYPE_ER_PARAM                  = 0x1b; // Error Resilient Parametric
constexpr auto AUDIO_OBJECT_TYPE_SSC                       = 0x1c; // SinuSoidal Coding
constexpr auto AUDIO_OBJECT_TYPE_PS                        = 0x1d; // Parametric Stereo
constexpr auto AUDIO_OBJECT_TYPE_SURROUND                  = 0x1e; // MPEG Surround
constexpr auto AUDIO_OBJECT_TYPE_L1                        = 0x20; // Layer 1
constexpr auto AUDIO_OBJECT_TYPE_L2                        = 0x21; // Layer 2
constexpr auto AUDIO_OBJECT_TYPE_L3                        = 0x22; // Layer 3
constexpr auto AUDIO_OBJECT_TYPE_DST                       = 0x23; // Direct Stream Transfer
constexpr auto AUDIO_OBJECT_TYPE_ALS                       = 0x24; // Audio LosslesS
constexpr auto AUDIO_OBJECT_TYPE_SLS                       = 0x25; // Scalable LosslesS
constexpr auto AUDIO_OBJECT_TYPE_SLS_NON_CORE              = 0x26; // Scalable LosslesS (non core)
constexpr auto AUDIO_OBJECT_TYPE_ER_AAC_ELD                = 0x27; // Error Resilient Enhanced Low Delay
constexpr auto AUDIO_OBJECT_TYPE_SMR_SIMPLE                = 0x28; // Symbolic Music Representation Simple
constexpr auto AUDIO_OBJECT_TYPE_SMR_MAIN                  = 0x29; // Symbolic Music Representation Main
constexpr auto AUDIO_OBJECT_TYPE_USAC_NOSBR                = 0x2a; // Unified Speech and Audio Coding (no SBR)
constexpr auto AUDIO_OBJECT_TYPE_SAOC                      = 0x2b; // Spatial Audio Object Coding
constexpr auto AUDIO_OBJECT_TYPE_LD_SURROUND               = 0x2c; // Low Delay MPEG Surround
constexpr auto AUDIO_OBJECT_TYPE_USAC                      = 0x2d; // Unified Speech and Audio Coding

// Atom data types for free-form data
constexpr auto ATOM_DATA_TYPE_JPEG                         = 0x0d;        // JPEG image
constexpr auto ATOM_DATA_TYPE_PNG                          = 0x0e;        // PNG image
constexpr auto ATOM_DATA_TYPE_BMP                          = 0x1b;        // BMP image

// ESDS descriptor types (tags)
constexpr auto DESCRIPTOR_TYPE_O                           = 0x01;
constexpr auto DESCRIPTOR_TYPE_IO                          = 0x02;
constexpr auto DESCRIPTOR_TYPE_ES                          = 0x03;
constexpr auto DESCRIPTOR_TYPE_DEC_CONFIG                  = 0x04;
constexpr auto DESCRIPTOR_TYPE_DEC_SPECIFIC                = 0x05;
constexpr auto DESCRIPTOR_TYPE_SL_CONFIG                   = 0x06;
constexpr auto DESCRIPTOR_TYPE_CONTENT_ID                  = 0x07;
constexpr auto DESCRIPTOR_TYPE_SUPPL_CONTENT_ID            = 0x08;
constexpr auto DESCRIPTOR_TYPE_IP_PTR                      = 0x09;
constexpr auto DESCRIPTOR_TYPE_IPMP_PTR                    = 0x0A;
constexpr auto DESCRIPTOR_TYPE_IPMP                        = 0x0B;
constexpr auto DESCRIPTOR_TYPE_REGISTRATION                = 0x0D;
constexpr auto DESCRIPTOR_TYPE_ESID_INC                    = 0x0E;
constexpr auto DESCRIPTOR_TYPE_ESID_REF                    = 0x0F;
constexpr auto DESCRIPTOR_TYPE_FILE_IO                     = 0x10;
constexpr auto DESCRIPTOR_TYPE_FILE_O                      = 0x11;
constexpr auto DESCRIPTOR_TYPE_EXT_PROFILE_LEVEL           = 0x13;
constexpr auto DESCRIPTOR_TYPE_TAGS_START                  = 0x80;
constexpr auto DESCRIPTOR_TYPE_TAGS_END                    = 0xFE;

} // namespace mtx::mp4
