/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/* Compile this with the following code
   
   FFMPEG_LIBS="libavdevice libavformat libavfilter libavcodec libswscale libavutil"; g++ encoding-example.c  $(pkg-config  --cflags $FFMPEG_LIBS) $(pkg-config --libs $FFMPEG_LIBS) -I ~/dynamo/src/magnet/ -O3
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
}

#include <magnet/exception.hpp>

class VideoEncoder
{
public:
  VideoEncoder(std::string filename, 
	       const size_t width, const size_t height, size_t fps = 25):
    _frameCounter(0),
    _fps(fps)
  {
    _inputWidth = width;
    //Force the video to have a pixel multiple of 2 by cutting a row
    //or column of pixels if needed. The encode frame will trim each
    //frame down.
    _videoWidth = width - (width % 2);
    _videoHeight = height - (height % 2);

    if (!_videoWidth || !_videoHeight) 
      M_throw() << "Can only encode images with a size of at least 2x2 pixels";

    size_t size = _videoWidth * _videoHeight;

    initialiseLibrary();

    AVCodec* _codec = avcodec_find_encoder(CODEC_ID_H264);
    if (!_codec) M_throw() << "Could not find the MPEG2 video codec";

    _context = avcodec_alloc_context();
    /* put sample parameters */
    _context->bit_rate = 400000;
    /* resolution must be a multiple of two */
    _context->width = _videoWidth;
    _context->height = _videoHeight;
    /* frames per second */
    _context->time_base= (AVRational){1,_fps};
    //_context->gop_size = _fps; /* emit one intra frame every second */
    //_context->max_b_frames=100;
    _context->pix_fmt = PIX_FMT_YUV420P;
    //_context->profile = FF_PROFILE_H264_HIGH;
    //_context->profile = FF_PROFILE_H264_BASELINE;
    //_context->compression_level = 10;
    //_context->level = 10;
    //_context->global_quality = 0;

    _context->max_b_frames=0;
    _context->profile= FF_PROFILE_H264_BASELINE;
    _context->level = 10;
    _context->gop_size = 25;
    _context->max_qdiff = 4;
    _context->qmin = 10;
    _context->qmax=51;
    _context->qcompress=0.6;
    _context->keyint_min=10;
    _context->trellis=0;
    _context->weighted_p_pred = 2;
    
    if (avcodec_open(_context, _codec) < 0) 
      M_throw() << "Could not open the MPEG1 video codec context";

    _picture = avcodec_alloc_frame();
    _pictureBuffer.resize((size * 3) / 2);
    _picture->data[0] = &(_pictureBuffer[0]);
    _picture->data[1] = _picture->data[0] + size;
    _picture->data[2] = _picture->data[1] + size / 4;
    _picture->linesize[0] = _videoWidth;
    _picture->linesize[1] = _videoWidth / 2;
    _picture->linesize[2] = _videoWidth / 2;
    
    //Allocate a reasonably large buffer for the output packets
    _outputBuffer.resize(100000);
    
    _outputFile.open(filename.c_str(), std::ios_base::out | std::ios_base::binary  | std::ios_base::binary);
    if (!_outputFile.is_open())  
      M_throw() << "Could not open the movie file for output";
  }

  ~VideoEncoder() { close(); }

  void addFrame(const std::vector<uint8_t>& RGB24Frame)
  {
    if (RGB24Frame.size() < 3 * _inputWidth * _videoHeight) 
      M_throw() << "The image is too small for the video size!";
    
    //* convert the RGB image to YUV420p and copy it into the picture buffer
    {
      size_t i = 0;
      size_t numpixels = _context->height * _context->width;
      size_t ui = numpixels;
      size_t vi = numpixels + numpixels/4;

      for (int j = 0; j < _context->height; j++)
	for (int k = 0; k < _context->width; k++)
	  {
	    size_t s = (k + j * _inputWidth) * 3;

	    _pictureBuffer[i] = (uint8_t)((66 * RGB24Frame[s+0] + 129*RGB24Frame[s+1] + 25*RGB24Frame[s+2] + 128) >> 8) + 16;
	      
	    if (0 == j%2 && 0 == k%2)
	      {
		_pictureBuffer[ui++] = (uint8_t)( (-38*RGB24Frame[s+0] - 74*RGB24Frame[s+1] + 112*RGB24Frame[s+2] + 128) >> 8) + 128;
		_pictureBuffer[vi++] = (uint8_t)( (112*RGB24Frame[s+0] - 94*RGB24Frame[s+1] - 18*RGB24Frame[s+2] + 128) >> 8) + 128;
	      }
	    i++;
	  }
    }

    /* Set the presentation time correctly, to remove the warning */
    _picture->pts = (1000 * 90 / _fps) * _frameCounter++;

    /* encode the image and write out any data returned from the encoder */
    int out_size = avcodec_encode_video(_context, &(_outputBuffer[0]), _outputBuffer.size(), _picture);
    _outputFile.write(reinterpret_cast<const char*>(&(_outputBuffer[0])), out_size);
  }

  void close()
  {
    if (!_outputFile.is_open()) return;

    /* get the delayed frames */
    int out_size;
    while(out_size = avcodec_encode_video(_context, &(_outputBuffer[0]), _outputBuffer.size(), NULL))
      _outputFile.write(reinterpret_cast<const char*>(&(_outputBuffer[0])), out_size);

    /* add sequence end code to have a real mpeg file */
    _outputBuffer[0] = 0x00;
    _outputBuffer[1] = 0x00;
    _outputBuffer[2] = 0x01;
    _outputBuffer[3] = 0xb7;
    _outputFile.write(reinterpret_cast<const char*>(&(_outputBuffer[0])), 4);
    _outputFile.close();

    avcodec_close(_context);
    av_free(_context);
    av_free(_picture);
    _outputBuffer.clear();
    _pictureBuffer.clear();
  }

  
private:
  AVCodecContext* _context;
  AVFrame* _picture;
  std::ofstream _outputFile;
  std::vector<uint8_t> _outputBuffer;
  std::vector<uint8_t> _pictureBuffer;

  size_t _videoWidth, _videoHeight;
  size_t _inputWidth;
  size_t _frameCounter;
  size_t _fps;

  struct LibavcodecInitialiser
  { LibavcodecInitialiser() { avcodec_init(); avcodec_register_all(); } };
    
  static LibavcodecInitialiser& initialiseLibrary()
  {
    static LibavcodecInitialiser _avinit;
    return _avinit;
  }

};


static void video_encode_example(const char *filename)
{
  size_t width = 1023;
  size_t height = 1023;
  size_t size = width * height;

  VideoEncoder encoder(filename, width, height);

  /* Allocate the RGB image in advance */
  std::vector<uint8_t> rgb_buf(size * 3);
  
  /* encode 3 seconds of video */
  for(int i=0; i<75; ++i) 
    {
      for(int y=0; y < height; y++)
	for(int x=0; x< width; x++)
	  {
	    rgb_buf[3*(x + width * y) + 0] = 10*i; //R
	    rgb_buf[3*(x + width * y) + 1] = 51*i; //G
	    rgb_buf[3*(x + width * y) + 2] = std::min(x,y); //B
	  }
      encoder.addFrame(rgb_buf);
    }
  
  encoder.close();
}

int main(int argc, char **argv)
{
    const char *filename;

    /* must be called before using avcodec lib */
    avcodec_init();

    /* register all the codecs */
    avcodec_register_all();

    video_encode_example("/tmp/test.mpg");

    return 0;
}