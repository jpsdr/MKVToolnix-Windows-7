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

#include "common/common_pch.h"
#include "common/memory.h"
#include "common/output.h"
#include "M2VParser.h"

constexpr auto BUFF_SIZE = 2 * 1024 * 1024;

MPEGFrame::MPEGFrame(binary *n_data, uint32_t n_size, bool n_bCopy):
  size(n_size), bCopy(n_bCopy) {

  if(bCopy) {
    data = (binary *)safemalloc(size);
    memcpy(data, n_data, size);
  } else {
    data = n_data;
  }
  stamped        = false;
  invisible      = false;
  refs[0]        = -1;
  refs[1]        = -1;
  seqHdrData     = nullptr;
  seqHdrDataSize = 0;
}

MPEGFrame::~MPEGFrame(){
  safefree(data);
  safefree(seqHdrData);
}

void M2VParser::SetEOS(){
  MPEGChunk * c;
  while((c = mpgBuf->ReadChunk())){
    chunks.push_back(c);
  }
  mpgBuf->ForceFinal();  //Force the last frame out.
  c = mpgBuf->ReadChunk();
  if(c) chunks.push_back(c);
  FillQueues();
  TimestampWaitingFrames();
  m_eos = true;
}


int32_t M2VParser::WriteData(binary* data, uint32_t dataSize){
  //If we are at EOS
  if(m_eos)
    return -1;

  if(mpgBuf->Feed(data, dataSize)){
    return -1;
  }

  //Fill the chunks buffer
  while(mpgBuf->GetState() == MPEG2_BUFFER_STATE_CHUNK_READY){
    MPEGChunk * c = mpgBuf->ReadChunk();
    if(c) chunks.push_back(c);
  }

  if(needInit){
    if(InitParser() == 0)
      needInit = false;
  }

  FillQueues();
  if(buffers.empty()){
    parserState = MPV_PARSER_STATE_NEED_DATA;
  }else{
    parserState = MPV_PARSER_STATE_FRAME;
  }

  return 0;
}

void M2VParser::DumpQueues(){
  while(!chunks.empty()){
    delete chunks.front();
    chunks.erase(chunks.begin());
  }
  while(!buffers.empty()){
    delete buffers.front();
    buffers.pop();
  }
}

M2VParser::M2VParser()
  : frameCounter{}
  , throwOnError{}
{
  mpgBuf = new MPEGVideoBuffer(BUFF_SIZE);

  notReachedFirstGOP = true;
  previousTimestamp = 0;
  previousDuration = 0;
  waitExpectedTime = 0;
  probing = false;
  b_frame_warning_printed = false;
  m_eos = false;
  mpegVersion = 1;
  needInit = true;
  parserState = MPV_PARSER_STATE_NEED_DATA;
  gopNum = -1;
  frameNum = 0;
  gopPts = 0;
  highestPts = 0;
  seqHdrChunk = nullptr;
  gopChunk = nullptr;
  firstField = nullptr;
  keepSeqHdrsInBitstream = true;
}

int32_t M2VParser::InitParser(){
  //Gotta find a sequence header now
  MPEGChunk* chunk;
  //MPEGChunk* seqHdrChunk;
  for (int i = 0, numChunks = chunks.size(); i < numChunks; i++){
    chunk = chunks[i];
    if(chunk->GetType() == MPEG_VIDEO_SEQUENCE_START_CODE){
      //Copy the header for later, we must copy because the actual chunk will be deleted in a bit
      binary * hdrData = new binary[chunk->GetSize()];
      memcpy(hdrData, chunk->GetPointer(), chunk->GetSize());
      seqHdrChunk = new MPEGChunk(hdrData, chunk->GetSize()); //Save this for adding as private data...
      ParseSequenceHeader(chunk, m_seqHdr);

      //Look for sequence extension to identify mpeg2
      binary* pData = chunk->GetPointer();
      for (int j = 3, chunkSize = chunk->GetSize() - 4; j < chunkSize; j++){
        if(pData[j] == 0x00 && pData[j+1] == 0x00 && pData[j+2] == 0x01 && pData[j+3] == 0xb5 && ((pData[j+4] & 0xF0) == 0x10)){
          mpegVersion = 2;
          break;
        }
      }
      return 0;
    }
  }
  return -1;
}

M2VParser::~M2VParser(){
  DumpQueues();
  FlushWaitQueue();
  delete seqHdrChunk;
  delete gopChunk;
  delete firstField;
  delete mpgBuf;
}

MediaTime M2VParser::GetFrameDuration(MPEG2PictureHeader picHdr){
  if (picHdr.repeatFirstField) {
    // picHdr.progressive shall be 1 if picHdr.repeatFirstField is 1
    if(m_seqHdr.progressiveSequence)
      return picHdr.topFieldFirst ? 6 : 4;
    else
      return 3;
  } else {
    return 2;
  }
}

MPEG2ParserState_e M2VParser::GetState(){
  FillQueues();
  if(!buffers.empty())
    parserState = MPV_PARSER_STATE_FRAME;
  else
    parserState = MPV_PARSER_STATE_NEED_DATA;

  if(m_eos && parserState == MPV_PARSER_STATE_NEED_DATA)
    parserState = MPV_PARSER_STATE_EOS;
  return parserState;
}

void M2VParser::FlushWaitQueue(){
  for (auto const &frame : waitQueue)
    delete frame;
  while (!buffers.empty()) {
    delete buffers.front();
    buffers.pop();
  }

  waitQueue.clear();
  m_timestamps.clear();
}

void M2VParser::StampFrame(MPEGFrame* frame){
  if (m_timestamps.empty())
    frame->timestamp = previousTimestamp + previousDuration;
  else {
    frame->timestamp = m_timestamps.front();
    m_timestamps.pop_front();
  }
  previousTimestamp = frame->timestamp;
  frame->duration = mtx::to_int(mtx::rational(frame->duration, 1) * 1'000'000'000 / (m_seqHdr.frameRate * 2));
  previousDuration = frame->duration;

  frame->stamped = true;
  frameTimestamps[frame->frameNumber] = frame->timestamp;

  // update affected ref timestamps
  for (int i = 0; i < 2; i++)
    if (refs[i].frameNumber == frame->frameNumber)
      TryUpdate(refs[i]);
}

void M2VParser::UpdateFrame(MPEGFrame* frame){
  // derive ref timestamps
  for (int i = 0; i < 2; i++) {
    if (!frame->tmpRefs[i].HasFrameNumber())
      continue;
    TryUpdate(frame->tmpRefs[i]);
    assert(frame->tmpRefs[i].HasTimestamp()); // ensure the timestamp indeed has been set (sometime before)
    frame->refs[i] = frame->tmpRefs[i].timestamp;
  }
}

int32_t M2VParser::OrderFrame(MPEGFrame* frame){
  MPEGFrame *p = frame;

  SetFrameRef(p);
  ShoveRef(p);

  if (('I' == p->frameType) && !waitQueue.empty())
    TimestampWaitingFrames();

  waitQueue.push_back(p);

  return 0;
}

void
M2VParser::TimestampWaitingFrames() {
  // mxinfo(fmt::format("  flushing {0}\n", waitQueue.size()));

  for (int idx = 0, numFrames = waitQueue.size(); idx < numFrames; ++idx)
    waitQueue[idx]->decodingOrder = idx;

  std::sort(waitQueue.begin(), waitQueue.end(), [](MPEGFrame *a, MPEGFrame *b) { return a->timestamp < b->timestamp; });

  for (auto const &frame : waitQueue)
    StampFrame(frame);
  for (auto const &frame : waitQueue)
    UpdateFrame(frame);

  std::sort(waitQueue.begin(), waitQueue.end(), [](MPEGFrame *a, MPEGFrame *b) { return a->decodingOrder < b->decodingOrder; });

  for (auto const &frame : waitQueue)
    buffers.push(frame);
  waitQueue.clear();
}

int32_t M2VParser::PrepareFrame(MPEGChunk* chunk, MediaTime timestamp, MPEG2PictureHeader picHdr, MPEGChunk* secondField){
  MPEGFrame* outBuf;
  bool bCopy = true;
  binary* pData = chunk->GetPointer();
  uint32_t dataLen = chunk->GetSize();

  if ((seqHdrChunk && keepSeqHdrsInBitstream &&
       (MPEG2_I_FRAME == picHdr.frameType)) || gopChunk || secondField) {
    uint32_t pos = 0;
    bCopy = false;
    dataLen +=
      (secondField ? secondField->GetSize() : 0) +
      (seqHdrChunk && keepSeqHdrsInBitstream && (MPEG2_I_FRAME == picHdr.frameType) ? seqHdrChunk->GetSize() : 0) +
      (gopChunk ? gopChunk->GetSize() : 0);
    pData = (binary *)safemalloc(dataLen);
    if (seqHdrChunk && keepSeqHdrsInBitstream &&
        (MPEG2_I_FRAME == picHdr.frameType)) {
      memcpy(pData, seqHdrChunk->GetPointer(), seqHdrChunk->GetSize());
      pos += seqHdrChunk->GetSize();
      delete seqHdrChunk;
      seqHdrChunk = nullptr;
    }
    if (gopChunk) {
      memcpy(pData + pos, gopChunk->GetPointer(), gopChunk->GetSize());
      pos += gopChunk->GetSize();
      delete gopChunk;
      gopChunk = nullptr;
    }
    memcpy(pData + pos, chunk->GetPointer(), chunk->GetSize());
    pos += chunk->GetSize();
    if (secondField)
      memcpy(pData + pos, secondField->GetPointer(), secondField->GetSize());
  }

  outBuf = new MPEGFrame(pData, dataLen, bCopy);
  outBuf->frameNumber = frameCounter++;

  if (seqHdrChunk && !keepSeqHdrsInBitstream &&
      (MPEG2_I_FRAME == picHdr.frameType)) {
    outBuf->seqHdrData = (binary *)safemalloc(seqHdrChunk->GetSize());
    outBuf->seqHdrDataSize = seqHdrChunk->GetSize();
    memcpy(outBuf->seqHdrData, seqHdrChunk->GetPointer(),
           outBuf->seqHdrDataSize);
    delete seqHdrChunk;
    seqHdrChunk = nullptr;
  }

  if(picHdr.frameType == MPEG2_I_FRAME){
    outBuf->frameType = 'I';
  }else if(picHdr.frameType == MPEG2_P_FRAME){
    outBuf->frameType = 'P';
  }else{
    outBuf->frameType = 'B';
  }

  outBuf->timestamp = timestamp; // Still the sequence number

  outBuf->invisible = invisible;
  outBuf->duration = GetFrameDuration(picHdr);
  outBuf->rff = (picHdr.repeatFirstField != 0);
  outBuf->tff = (picHdr.topFieldFirst != 0);
  outBuf->progressive = (picHdr.progressive != 0);
  outBuf->pictureStructure = (uint8_t) picHdr.pictureStructure;

  OrderFrame(outBuf);
  return 0;
}

void M2VParser::SetFrameRef(MPEGFrame *ref){
  if (ref->frameType == 'B') {
    ref->tmpRefs[0] = refs[0];
    ref->tmpRefs[1] = refs[1];
  } else if(ref->frameType == 'P')
    ref->tmpRefs[0] = refs[1];
}

void M2VParser::ShoveRef(MPEGFrame *ref){
  if(ref->frameType == 'I' || ref->frameType == 'P'){
    refs[0] = refs[1];
    refs[1].Clear();
    refs[1].frameNumber = ref->frameNumber;
  }
}

void M2VParser::ClearRef(){
  for (int i = 0; i < 2; i++)
    refs[i].Clear();
}

//Maintains the time of the last start of GOP and uses the temporal_reference
//field as an offset.
int32_t M2VParser::FillQueues(){
  if(chunks.empty()){
    return -1;
  }
  bool done = false;
  while(!done){
    MediaTime myTime;
    MPEGChunk* chunk;
    if (firstField) {
      chunk = firstField; // 2. point "chunk" to the leftover we kept
    } else {
      chunk = chunks.front();
    }
    while (chunk->GetType() != MPEG_VIDEO_PICTURE_START_CODE) {
      if (chunk->GetType() == MPEG_VIDEO_GOP_START_CODE) {
        ParseGOPHeader(chunk, m_gopHdr);
        if (frameNum != 0) {
          gopPts = highestPts + 1;
        }
        if (gopChunk)
          delete gopChunk;
        gopChunk = chunk;
        gopNum++;
        // There are too many broken videos to do the following so ReferenceBlock will be wrong for broken videos.
        /*
        if(m_gopHdr.closedGOP){
          ClearRef();
        }
        */
      } else if (chunk->GetType() == MPEG_VIDEO_SEQUENCE_START_CODE) {
        if (seqHdrChunk)
          delete seqHdrChunk;
        ParseSequenceHeader(chunk, m_seqHdr);
        seqHdrChunk = chunk;

      }

      chunks.erase(chunks.begin());
      if (chunks.empty())
        return -1;
      chunk = chunks.front();
    }
    MPEG2PictureHeader picHdr;
    ParsePictureHeader(chunk, picHdr);

    myTime = gopPts + picHdr.temporalReference;
    invisible = false;

    MPEGChunk* secondField = nullptr;
    if (picHdr.pictureStructure != MPEG2_PICTURE_TYPE_FRAME) {
      if (!firstField) {
        chunks.erase(chunks.begin());
        if (chunks.empty()) {
          firstField = chunk; // 1. keep the first field in the parser context
          return -1;
        }
      }
      firstField = nullptr; // 3. clear the pointer after the above check
      secondField = chunks.front();
      MPEG2PictureHeader sfPicHdr;
      ParsePictureHeader(secondField, sfPicHdr);
      if (sfPicHdr.pictureStructure == MPEG2_PICTURE_TYPE_FRAME ||
          sfPicHdr.pictureStructure == picHdr.pictureStructure) {
        auto error = Y("Single field picture detected. Fix the MPEG2 video stream before attempting to multiplex it.\n");
        if (throwOnError)
          throw error;
        mxerror(error);
        continue; // dropping the (earlier) field; "secondField" will be the new "chunk"
      }
    }

    if (myTime > highestPts)
      highestPts = myTime;

    switch(picHdr.frameType){
      case MPEG2_I_FRAME:
        PrepareFrame(chunk, myTime, picHdr, secondField);
        notReachedFirstGOP = false;
        break;
      case MPEG2_P_FRAME:
        if (!refs[1].HasFrameNumber())
          break;
        PrepareFrame(chunk, myTime, picHdr, secondField);
        break;
      default: //B-frames
        if (!refs[0].HasFrameNumber() || !refs[1].HasFrameNumber()) {
          if (!m_gopHdr.closedGOP && !m_gopHdr.brokenLink && !probing && !b_frame_warning_printed) {
            mxwarn(Y("Found at least one B frame without second reference in a non closed GOP.\n"));
            b_frame_warning_printed = true;
          }
          invisible = true;
        }
        PrepareFrame(chunk, myTime, picHdr, secondField);
    }
    frameNum++;
    chunks.erase(chunks.begin());
    delete chunk;
    if (secondField)
      delete secondField;
    if (chunks.empty())
      return -1;
  }
  return 0;
}

MPEGFrame* M2VParser::ReadFrame(){
  //if(m_eos) return nullptr;
  if(GetState() != MPV_PARSER_STATE_FRAME){
    return nullptr;
  }
  if(buffers.empty()){
    return nullptr; // OOPS!
  }
  MPEGFrame* frame = buffers.front();
  buffers.pop();
  return frame;
}

void
M2VParser::AddTimestamp(int64_t timestamp) {
  std::list<int64_t>::iterator idx = m_timestamps.begin();
  while ((idx != m_timestamps.end()) && (timestamp > *idx))
    idx++;
  m_timestamps.insert(idx, timestamp);
}

void
M2VParser::SetThrowOnError(bool doThrow) {
  throwOnError = doThrow;
}

void
M2VParser::TryUpdate(MPEGFrameRef &frame){
  // if frame set, stamped and no timestamp yet, derive it
  if (frame.HasTimestamp() || !frame.HasFrameNumber())
    return;

  auto itr = frameTimestamps.find(frame.frameNumber);
  if (itr != frameTimestamps.end())
    frame.timestamp = itr->second;
}
