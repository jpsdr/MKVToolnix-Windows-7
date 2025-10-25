/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   Endian helper functions

   Written by Moritz Bunkus <mo@bunkus.online>.
*/

#pragma once

#include "common/common_pch.h"

uint16_t get_uint16_le(const void *buf);
uint32_t get_uint24_le(const void *buf);
uint32_t get_uint32_le(const void *buf);
uint64_t get_uint64_le(const void *buf);
uint64_t get_uint_le(const void *buf, int max_bytes);
uint16_t get_uint16_be(const void *buf);
uint32_t get_uint24_be(const void *buf);
uint32_t get_uint32_be(const void *buf);
uint64_t get_uint64_be(const void *buf);
uint64_t get_uint_be(const void *buf, int max_bytes);
void put_uint_le(void *buf, uint64_t value, size_t num_bytes);
void put_uint16_le(void *buf, uint16_t value);
void put_uint24_le(void *buf, uint32_t value);
void put_uint32_le(void *buf, uint32_t value);
void put_uint64_le(void *buf, uint64_t value);
void put_uint_be(void *buf, uint64_t value, size_t num_bytes);
void put_uint16_be(void *buf, uint16_t value);
void put_uint24_be(void *buf, uint32_t value);
void put_uint32_be(void *buf, uint32_t value);
void put_uint64_be(void *buf, uint64_t value);
