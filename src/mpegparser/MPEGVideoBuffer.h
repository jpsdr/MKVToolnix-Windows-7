/*****************************************************************************

    MPEG Video Packetizing Buffer

    Copyright(C) 2004 John Cannon <spyder@matroska.org>

    This program is free software ; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation ; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY ; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program ; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

 **/

#pragma once

#include "common/common_pch.h"

#include "common/math_fwd.h"

#include "Types.h"
#include "CircBuffer.h"

constexpr auto MPEG_VIDEO_PICTURE_START_CODE  = 0x00;
constexpr auto MPEG_VIDEO_SEQUENCE_START_CODE = 0xb3;
constexpr auto MPEG_VIDEO_EXT_START_CODE      = 0xb5;
constexpr auto MPEG_VIDEO_GOP_START_CODE      = 0xb8;
constexpr auto MPEG_VIDEO_USER_START_CODE     = 0xb2;

enum MPEG2BufferState_e {
  MPEG2_BUFFER_STATE_NEED_MORE_DATA,
  MPEG2_BUFFER_STATE_CHUNK_READY,
  MPEG2_BUFFER_STATE_EMPTY,
  MPEG2_BUFFER_INVALID
};

constexpr auto MPEG2_I_FRAME = 1;
constexpr auto MPEG2_P_FRAME = 2;
constexpr auto MPEG2_B_FRAME = 3;

struct MPEG2SequenceHeader {
  uint32_t width{};
  uint32_t height{};
  mtx_mp_rational_t aspectRatio;
  mtx_mp_rational_t frameRate;
  uint8_t profileLevelIndication{};
  uint8_t progressiveSequence{};

  MPEG2SequenceHeader();
};

struct MPEG2GOPHeader {
  uint8_t closedGOP;
  uint8_t brokenLink;
  uint32_t timeFrames;
  uint32_t timeSeconds;
  uint32_t timeMinutes;
  uint32_t timeHours;

  MPEG2GOPHeader();
};

constexpr auto MPEG2_PICTURE_TYPE_FRAME        = 0x03;
constexpr auto MPEG2_PICTURE_TYPE_TOP_FIELD    = 0x01;
constexpr auto MPEG2_PICTURE_TYPE_BOTTOM_FIELD = 0x02;

struct MPEG2PictureHeader {
  uint32_t temporalReference;
  uint8_t frameType;
  uint32_t pictureStructure;
  uint8_t repeatFirstField;
  uint8_t topFieldFirst;
  uint8_t progressive;

  MPEG2PictureHeader();
};

class MPEGChunk{
private:
  uint8_t * data;
  uint32_t size;
  uint8_t type;
public:
  MPEGChunk(uint8_t* n_data, uint32_t n_size):
    data(n_data), size(n_size) {

    assert(data);
    assert(4 <= size);

    type = data[3];
  }

  ~MPEGChunk(){
    if(data)
      delete [] data;
  }

  inline uint8_t GetType() const {
    return type;
  }

  inline uint32_t GetSize() const{
    return size;
  }

  uint8_t & operator[](unsigned int i){
    return data[i];
  }

  uint8_t & at(unsigned int i) {
    return data[i];
  }

  inline uint8_t * GetPointer(){
    return data;
  }
};

void ParseSequenceHeader(MPEGChunk* chunk, MPEG2SequenceHeader & hdr);
bool ParsePictureHeader(MPEGChunk* chunk, MPEG2PictureHeader & hdr);
bool ParseGOPHeader(MPEGChunk* chunk, MPEG2GOPHeader & hdr);

class MPEGVideoBuffer{
private:
  CircBuffer * myBuffer;
  MPEG2BufferState_e state;
  int32_t chunkStart;
  int32_t chunkEnd;
  void UpdateState();
  int32_t FindStartCode(uint32_t startPos = 0);
public:
  MPEGVideoBuffer(uint32_t size){
    myBuffer = new CircBuffer(size);
    state = MPEG2_BUFFER_STATE_EMPTY;
    chunkStart = -1;
    chunkEnd = -1;
  }

  ~MPEGVideoBuffer(){
    delete myBuffer;
  }

  inline MPEG2BufferState_e GetState() const { return state; }

  int32_t GetFreeBufferSpace(){
    return (myBuffer->buf_capacity - myBuffer->bytes_in_buf);
  }

  void SetEndOfData(){
    chunkEnd = myBuffer->GetLength() - 1;
  }

  void ForceFinal();  //prepares the remaining data as a chunk
  MPEGChunk * ReadChunk();
  int32_t Feed(uint8_t* data, uint32_t numBytes);
};
