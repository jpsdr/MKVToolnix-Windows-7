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

#include "common/common_pch.h"

#include "MPEGVideoBuffer.h"
#include <cstring>

MPEG2SequenceHeader::MPEG2SequenceHeader() {
}

MPEG2GOPHeader::MPEG2GOPHeader() {
  memset(this, 0, sizeof(*this));
}

MPEG2PictureHeader::MPEG2PictureHeader() {
  memset(this, 0, sizeof(*this));
}

int32_t MPEGVideoBuffer::FindStartCode(uint32_t startPos){
  //How many bytes can we look through?
  uint32_t window = myBuffer->GetLength() - startPos;

  if(window < 4) //Make sure we have enough bytes to search.
    return -1;

  for(int i = startPos, endPos = startPos + window - 3; i < endPos; i++){
    CircBuffer& buf = *myBuffer;
    uint8_t a,b,c,d;
    a = buf[i];
    b = buf[i+1];
    c = buf[i+2];
    d = buf[i+3];
    if((a == 0x00) && (b == 0x00) && (c == 0x01)){
      switch(d){
        case MPEG_VIDEO_SEQUENCE_START_CODE:
        case MPEG_VIDEO_GOP_START_CODE:
        case MPEG_VIDEO_PICTURE_START_CODE:
          return i;  //Return our position if we found
          //one of the codes we want

      }
    }
  }

  //If we get here we have no _wanted_ start code found.
  return -1;
}

void MPEGVideoBuffer::UpdateState(){
  assert(myBuffer);
  int32_t test = 0;
  if(myBuffer->GetLength() == 0){
    state = MPEG2_BUFFER_STATE_EMPTY;
    return;
  }
  if(chunkStart == -1){
    test = FindStartCode(0);
    if(test != -1)  //We found a new startcode
      chunkStart = test;
  }
  if(chunkEnd == -1){
    test = FindStartCode(chunkStart+4);
    if(test != -1)  //We found a new startcode
      chunkEnd = test;
  }
  if(chunkStart == -1 || chunkEnd == -1){
    state = MPEG2_BUFFER_STATE_NEED_MORE_DATA;
  }else{
    assert(chunkStart >= 0 && chunkStart < chunkEnd && chunkEnd > 0);
    state = MPEG2_BUFFER_STATE_CHUNK_READY;
  }
}

MPEGChunk * MPEGVideoBuffer::ReadChunk(){
  MPEGChunk* myChunk = nullptr;
  if(state == MPEG2_BUFFER_STATE_CHUNK_READY){
    assert(chunkStart < chunkEnd && chunkStart != -1 && chunkEnd != -1);
    if(chunkStart != 0){ //we have to skip some bytes
      myBuffer->Skip(chunkStart);
    }
    uint32_t chunkLength = chunkEnd - chunkStart;
    uint8_t* chunkData = new uint8_t[chunkLength];
    myBuffer->Read(chunkData, chunkLength);
    chunkStart = 0; //we read up to the next start code
    chunkEnd = -1;
    UpdateState();
    myChunk = new MPEGChunk(chunkData, chunkLength);
    return myChunk;
  }else{
    return nullptr;
  }
}

void MPEGVideoBuffer::ForceFinal(){
  if(state == MPEG2_BUFFER_STATE_NEED_MORE_DATA){
    chunkStart = 0;
    chunkEnd = chunkStart + myBuffer->GetLength();
    UpdateState();
  }
}

int32_t MPEGVideoBuffer::Feed(uint8_t* data, uint32_t numBytes){
  uint32_t res = myBuffer->Write(data, numBytes);
  UpdateState();
  return res;
}

void ParseSequenceHeader(MPEGChunk* chunk, MPEG2SequenceHeader & hdr){
  uint8_t* pos = chunk->GetPointer();
  //Parse out the resolution info, horizontal first
  pos+=4; //Skip the start code
  hdr.width = (((unsigned int)pos[0]) << 4) | (((unsigned int) pos[1])>>4);  //xx x0 00
  pos+=1;
  hdr.height = (((unsigned int)pos[0] & 0x0F) << 8) | (((unsigned int) pos[1]));  //00 0x xx
  pos+=2;
  switch(pos[0] & 0xF0){
    case 0x10:
      hdr.aspectRatio = 1;
      break;
    case 0x20:
      hdr.aspectRatio = mtx::rational(4, 3);
      break;
    case 0x30:
      hdr.aspectRatio = mtx::rational(16, 9);
      break;
    case 0x40:
      hdr.aspectRatio = mtx::rational(221, 100);
      break;
    default:
      hdr.aspectRatio = 1;
  }
  switch(pos[0] & 0x0F){
    case 0x01:
      hdr.frameRate = mtx::rational(24000, 1001); // 23.976
      break;
    case 0x02:
      hdr.frameRate = 24;       // 24
      break;
    case 0x03:
      hdr.frameRate = 25;       // 25
      break;
    case 0x04:
      hdr.frameRate = mtx::rational(30000, 1001); // 29.97
      break;
    case 0x05:
      hdr.frameRate = 30;       // 30
      break;
    case 0x06:
      hdr.frameRate = 50;       // 50
      break;
    case 0x07:
      hdr.frameRate = mtx::rational(60000, 1001); // 59.94
      break;
    case 0x08:
      hdr.frameRate = 60;       // 60
      break;
    default:
      hdr.frameRate = 0;
  }

  //Seek to extension
  hdr.progressiveSequence = 0;
  while(pos < (chunk->GetPointer() + chunk->GetSize() - 6)){
    if((pos[0] == 0x00) && (pos[1] == 0x00) && (pos[2] == 0x01) && (pos[3] == MPEG_VIDEO_EXT_START_CODE)){
      if((pos[4] & 0xF0) == 0x10){ // Sequence extension
        hdr.profileLevelIndication = ((pos[4] & 0x0F) << 4) | ((pos[5] & 0xF0) >> 4);
        hdr.progressiveSequence = (pos[5] & 0x08) >> 3;
        pos+=6;
        break;
      }
    }
    pos++;
  }
}

bool ParseGOPHeader(MPEGChunk* chunk, MPEG2GOPHeader & hdr){
  if(chunk->GetType() != MPEG_VIDEO_GOP_START_CODE){
    //printf("Don't feed parse_gop_header a chunk that isn't a gop header!!!\n");
    return false;
  }
  uint8_t* pos = chunk->GetPointer();
  uint32_t timestamp;
  pos+=4; //skip the startcode
  //Parse GOP timestamp structure
  timestamp = ((uint32_t)pos[0] << 24) |
    ((uint32_t)pos[1] << 16) |
    ((uint32_t)pos[2] << 8) |
    ((uint32_t)pos[3]);
  hdr.timeHours   = (timestamp << 1) >> 27;
  hdr.timeMinutes = (timestamp << 6)>>26;
  hdr.timeSeconds = (timestamp << 13) >> 26;
  hdr.timeFrames  = (timestamp << 19) >> 26;
  pos+=3;
  hdr.closedGOP   = (pos[0] & 0x40) >> 6;
  hdr.brokenLink  = (pos[0] & 0x20) >> 5;;
  return true;
}

bool ParsePictureHeader(MPEGChunk* chunk, MPEG2PictureHeader & hdr){
  if(chunk->GetType() != MPEG_VIDEO_PICTURE_START_CODE){
    //printf("Don't feed parse_picture_header a chunk that isn't a picture!!!\n");
    return false;
  }

  if (chunk->GetSize() < 7)
    return false;

  uint8_t* pos = chunk->GetPointer();
  int havePicExt = 0;
  uint32_t temp = 0;
  pos+=4;
  temp = (((uint32_t)pos[0]) << 8) | (pos[1] & 0xC0);
  hdr.temporalReference = temp >> 6;
  pos+=1;
  temp = ((uint32_t)(pos[0] & 0x38)) >> 3 ;
  hdr.frameType = (uint8_t) temp;

  //Seek to extension
  while(pos < (chunk->GetPointer() + chunk->GetSize() - 4)){
    if((pos[0] == 0x00) && (pos[1] == 0x00) && (pos[2] == 0x01) && (pos[3] == MPEG_VIDEO_EXT_START_CODE)){
      if((pos[4] & 0xF0) == 0x80){ //Picture coding extension
        //printf("Found a picture_coding_extension\n");
        havePicExt = 1;
        break;
      }
    }
    pos++;
  }
  if(!havePicExt){
    hdr.pictureStructure = MPEG2_PICTURE_TYPE_FRAME;
    hdr.repeatFirstField = 0;
    hdr.topFieldFirst = 1;
    hdr.progressive = 1;
  }else{
    pos+=4;//skip start code
    pos+=2;//skip f_code shit
    hdr.pictureStructure = (pos[0] & 0x03);
    pos++;
    hdr.topFieldFirst = (pos[0] & 0x80);
    //pos++;
    hdr.repeatFirstField = (pos[0] & 0x02);
    /*//let's play ;)
      hdr->repeatFirstField = (pos[0] = pos[0] & 0xFD) & 0x02;*/
    pos++;
    hdr.progressive = (pos[0] & 0x80);
  }

  return true;
}
