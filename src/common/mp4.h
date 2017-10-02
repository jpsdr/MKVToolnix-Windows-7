/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   common definitions for MP4 files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

// Object type identifications.
// See http://gpac.sourceforge.net/tutorial/mediatypes.htm
#define MP4OTI_MPEG4Systems1                   0x01
#define MP4OTI_MPEG4Systems2                   0x02
#define MP4OTI_MPEG4Visual                     0x20
#define MP4OTI_MPEG4Audio                      0x40
#define MP4OTI_MPEG2VisualSimple               0x60
#define MP4OTI_MPEG2VisualMain                 0x61
#define MP4OTI_MPEG2VisualSNR                  0x62
#define MP4OTI_MPEG2VisualSpatial              0x63
#define MP4OTI_MPEG2VisualHigh                 0x64
#define MP4OTI_MPEG2Visual422                  0x65
#define MP4OTI_MPEG2AudioMain                  0x66
#define MP4OTI_MPEG2AudioLowComplexity         0x67
#define MP4OTI_MPEG2AudioScaleableSamplingRate 0x68
#define MP4OTI_MPEG2AudioPart3                 0x69
#define MP4OTI_MPEG1Visual                     0x6A
#define MP4OTI_MPEG1Audio                      0x6B
#define MP4OTI_JPEG                            0x6C
#define MP4OTI_DTS                             0xA9
#define MP4OTI_VORBIS                          0xDD
#define MP4OTI_VOBSUB                          0xE0

// Audio object type identifactors
// This list comes from
#define MP4AOT_AAC_MAIN        0x01 // Main
#define MP4AOT_AAC_LC          0x02 // Low Complexity
#define MP4AOT_AAC_SSR         0x03 // Scalable Sample Rate
#define MP4AOT_AAC_LTP         0x04 // Long Term Prediction
#define MP4AOT_SBR             0x05 // Spectral Band Replication
#define MP4AOT_AAC_SCALABLE    0x06 // Scalable
#define MP4AOT_TWINVQ          0x07 // TwinVQ Vector Quantizer
#define MP4AOT_CELP            0x08 // Code Excited Linear Prediction
#define MP4AOT_HVXC            0x09 // Harmonic Vector eXcitation Coding
#define MP4AOT_TTSI            0x0c // Text-To-Speech Interface
#define MP4AOT_MAINSYNTH       0x0d // Main Synthesis
#define MP4AOT_WAVESYNTH       0x0e // Wavetable Synthesis
#define MP4AOT_MIDI            0x0f // General MIDI
#define MP4AOT_SAFX            0x10 // Algorithmic Synthesis and Audio Effects
#define MP4AOT_ER_AAC_LC       0x11 // Error Resilient Low Complexity
#define MP4AOT_ER_AAC_LTP      0x13 // Error Resilient Long Term Prediction
#define MP4AOT_ER_AAC_SCALABLE 0x14 // Error Resilient Scalable
#define MP4AOT_ER_TWINVQ       0x15 // Error Resilient Twin Vector Quantizer
#define MP4AOT_ER_BSAC         0x16 // Error Resilient Bit-Sliced Arithmetic Coding
#define MP4AOT_ER_AAC_LD       0x17 // Error Resilient Low Delay
#define MP4AOT_ER_CELP         0x18 // Error Resilient Code Excited Linear Prediction
#define MP4AOT_ER_HVXC         0x19 // Error Resilient Harmonic Vector eXcitation Coding
#define MP4AOT_ER_HILN         0x1a // Error Resilient Harmonic and Individual Lines plus Noise
#define MP4AOT_ER_PARAM        0x1b // Error Resilient Parametric
#define MP4AOT_SSC             0x1c // SinuSoidal Coding
#define MP4AOT_PS              0x1d // Parametric Stereo
#define MP4AOT_SURROUND        0x1e // MPEG Surround
#define MP4AOT_L1              0x20 // Layer 1
#define MP4AOT_L2              0x21 // Layer 2
#define MP4AOT_L3              0x22 // Layer 3
#define MP4AOT_DST             0x23 // Direct Stream Transfer
#define MP4AOT_ALS             0x24 // Audio LosslesS
#define MP4AOT_SLS             0x25 // Scalable LosslesS
#define MP4AOT_SLS_NON_CORE    0x26 // Scalable LosslesS (non core)
#define MP4AOT_ER_AAC_ELD      0x27 // Error Resilient Enhanced Low Delay
#define MP4AOT_SMR_SIMPLE      0x28 // Symbolic Music Representation Simple
#define MP4AOT_SMR_MAIN        0x29 // Symbolic Music Representation Main
#define MP4AOT_USAC_NOSBR      0x2a // Unified Speech and Audio Coding (no SBR)
#define MP4AOT_SAOC            0x2b // Spatial Audio Object Coding
#define MP4AOT_LD_SURROUND     0x2c // Low Delay MPEG Surround
#define MP4AOT_USAC            0x2d // Unified Speech and Audio Coding
