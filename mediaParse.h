#ifndef INC_MEDIAPARSE_H
#define INC_MEDIAPARSE_H

#include <iostream>
#include <vector>

extern "C"{ // using system-wide ffmpeg install directory
 #include <avformat.h>
 #include <swscale.h>
}

#include "mediaConfig.h"
#include "mediaFrame.h"
#include "mediaAnalysis.h"

using namespace std;


class mediaParse{

 public:

  mediaParse(bool);
  ~mediaParse();

  // parser returns number of scene change occurence
  bool parse(mediaConfig*);
  
 private:

  AVStream *add_video_stream(AVFormatContext*, int, int, int);
  AVFrame *alloc_picture(int, int, int);
  void open_video(AVFormatContext*, AVStream*);
  void write_video_frame(AVFormatContext*, AVStream*, AVFrame*);
  void close_video(AVFormatContext*, AVStream*);

  AVStream *add_audio_stream(AVFormatContext *oc, int codec_id);
  void open_audio(AVFormatContext *oc, AVStream *st);
  void get_audio_frame(int16_t *samples, int frame_size, int nb_channels);
  void write_audio_frame(AVFormatContext *oc, AVStream *st);
  void close_audio(AVFormatContext *oc, AVStream *st);

  bool debug;

  // mediaFactory* alloc_mngr;

  // frame_list holds list of indivisual frames from skip sampling
  vector<mediaFrame*>* frame_list;

  // main handle to source video format context
  AVFormatContext *pFormatCtx;
  // main handle to source video/audio codec context
  AVCodecContext *pCodecCtx; // video
  AVCodecContext *aCodecCtx; // audio
  // handle to particular codec that concerns us
  AVCodec *pCodec; // video
  AVCodec *aCodec; // audio
  
  // handle to raw frame
  AVFrame *pFrame;

  // output for FF video
  AVOutputFormat *outputFormat;
  AVFormatContext *outputContext;
  AVStream *video_stream;
  AVStream *audio_stream;
  
  /*
  AVOutputFormat *outputFormat_excerpt;
  AVFormatContext *outputContext_excerpt;
  AVStream *video_stream_excerpt;
  AVStream *audio_stream_excerpt;
  char ofname_excerpt[128];
  */

/* video output */
  uint8_t *video_outbuf;
  AVFrame *picture, *tmp_picture;
  int frame_count, video_outbuf_size;
  
  /* audio output */
  float t, tincr, tincr2;
  int16_t *samples;
  uint8_t *audio_outbuf;
  int audio_outbuf_size;
  int audio_input_frame_size;
  
  // test items
  mediaFrame* pMediaFrame;
  mediaFrame* pMediaFrame_excerpt;

};

#endif // INC_MEDIAPARSE_H
