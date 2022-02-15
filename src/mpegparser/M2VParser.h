/*****************************************************************************

    MPEG Video Packetizer

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

#include "MPEGVideoBuffer.h"
#include <queue>

enum MPEG2ParserState_e {
  MPV_PARSER_STATE_FRAME,
  MPV_PARSER_STATE_NEED_DATA,
  MPV_PARSER_STATE_EOS,
  MPV_PARSER_STATE_ERROR
};

// stores a ref to another frame (first of all a frame pointer, sometime later also its timestamp)
class MPEGFrame;
class MPEGFrameRef {
public:
  uint64_t frameNumber;
  MediaTime timestamp;

  MPEGFrameRef() {
    Clear();
  }
  void Clear() {
    frameNumber = std::numeric_limits<uint64_t>::max();
    timestamp    = -1;
  }
  bool HasFrameNumber() const {
    return std::numeric_limits<uint64_t>::max() != frameNumber;
  }
  bool HasTimestamp() const {
    return -1 != timestamp;
  }
};

class MPEGFrame {
public:
  binary *data;
  uint32_t size;
  binary *seqHdrData;
  uint32_t seqHdrDataSize;
  MediaTime duration;
  char frameType;
  MediaTime timestamp; // before stamping: sequence number; after stamping: real timestamp
  unsigned int decodingOrder;
  MediaTime refs[2];
  MPEGFrameRef tmpRefs[2];
  bool stamped;
  bool invisible;
  bool rff;
  bool tff;
  bool progressive;
  uint8_t pictureStructure;
  bool bCopy;
  uint64_t frameNumber;

  MPEGFrame(binary* data, uint32_t size, bool bCopy);
  ~MPEGFrame();
};

class M2VParser {
private:
  std::vector<MPEGChunk*> chunks; //Hold the chunks until we can order them
  std::vector<MPEGFrame*> waitQueue; //Holds unstamped buffers until we can stamp them.
  std::queue<MPEGFrame*> buffers; //Holds stamped buffers until they are requested.
  std::unordered_map<uint64_t, uint64_t> frameTimestamps;
  uint64_t frameCounter;
  MediaTime previousTimestamp;
  MediaTime previousDuration;
  //Added to allow reading the header's raw data, contains first found seq hdr.
  MPEGChunk* seqHdrChunk, *gopChunk, *firstField;
  MPEG2SequenceHeader m_seqHdr; //current sequence header
  MPEG2GOPHeader m_gopHdr; //current GOP header
  MediaTime waitExpectedTime;
  bool      waitSecondField;
  bool      probing;
  bool      b_frame_warning_printed;
  MPEGFrameRef refs[2];
  bool needInit;
  bool m_eos;
  bool notReachedFirstGOP;
  bool keepSeqHdrsInBitstream;
  bool invisible;
  int32_t   gopNum;
  MediaTime frameNum;
  MediaTime gopPts;
  MediaTime highestPts;
  bool usePictureFrames;
  uint8_t mpegVersion;
  MPEG2ParserState_e parserState;
  MPEGVideoBuffer * mpgBuf;
  std::list<int64_t> m_timestamps;
  bool throwOnError;

  int32_t InitParser();
  void DumpQueues();
  int32_t FillQueues();
  void SetFrameRef(MPEGFrame *ref);
  void ShoveRef(MPEGFrame *ref);
  void ClearRef();
  MediaTime GetFrameDuration(MPEG2PictureHeader picHdr);
  void FlushWaitQueue();
  int32_t OrderFrame(MPEGFrame* frame);
  void StampFrame(MPEGFrame* frame);
  void UpdateFrame(MPEGFrame* frame);
  int32_t PrepareFrame(MPEGChunk* chunk, MediaTime timestamp, MPEG2PictureHeader picHdr, MPEGChunk* secondField);
public:
  M2VParser();
  virtual ~M2VParser();

  MPEG2SequenceHeader GetSequenceHeader(){
    return m_seqHdr;
  }

  //BE VERY CAREFUL WITH THIS CALL
  MPEGChunk * GetRealSequenceHeader(){
    return seqHdrChunk;
  }

  uint8_t GetMPEGVersion() const{
    return mpegVersion;
  }

  //Returns a pointer to a frame that has been read
  virtual MPEGFrame * ReadFrame();

  //Returns the max amount of data that can be written to the buffer
  int32_t GetFreeBufferSpace(){
    return mpgBuf->GetFreeBufferSpace();
  }

  //Writes data to the internal buffer.
  int32_t WriteData(binary* data, uint32_t dataSize);

  //Returns the current state of the parser
  MPEG2ParserState_e GetState();

  //Sets "end of stream" status on the buffer, forces timestamping of frames waiting.
  //Do not call this without good reason.
  void SetEOS();

  void SeparateSequenceHeaders() {
    keepSeqHdrsInBitstream = false;
  }

  //Using this method if the M2VParser will process only the beginning of the file.
  void SetProbeMode() {
    probing = true;
  }

  void AddTimestamp(int64_t timestamp);

  void SetThrowOnError(bool doThrow);

  void TimestampWaitingFrames();
  void TryUpdate(MPEGFrameRef &frame);
};
