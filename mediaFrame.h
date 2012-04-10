#ifndef INC_MEDIAFRAME_H
#define INC_MEDIAFRAME_H

#include <iostream>

// system-wide installed libgd
#include <gd.h>

extern "C"{ // using system-wide ffmpeg install directory
 #include <avformat.h>
 #include <swscale.h>
}

using namespace std;


class mediaFrame{

 public:

  mediaFrame(bool, AVCodecContext*, int, AVFrame*, int, int);
  ~mediaFrame();
  int get_id();
  AVFrame* get_pFrameRGB();
  AVFrame* get_pFrameRGB_resized();
  gdImagePtr get_pGdImage();
  int initMV();
  int initKF();
  bool createGdImage();

  //void saveJpegFrame(int, int, char*, const char*, const char*);
  void saveJpegFrame(int, int, const char*, const char*, const char*);

  //void saveJpegFrame(int, int, char*, const char*, const char*);
  void savePngFrame(int, int, const char*, const char*, const char*);

  //void saveJpegFrame(int, int, char*, const char*, const char*);
  void saveGifFrame(int, int, const char*, const char*, const char*);

  //void initiateAGIF(int, int, char*, const char*, const char*);
  void initiateAGIF(int, int, const char*, const char*);
  void pushAGIF(gdImagePtr);
  void closeAGIF();


 private:
  
  bool debug;

  AVCodecContext* pCodecCtx;
  int iFrame;
  float t_sec;
  AVFrame *pFrame;
  int width, height;
  int target_x, target_y;
  gdImagePtr gdimage;
  //gdImagePtr gdOut;

  gdImagePtr aGIFOut;
  int aGIFw;
  int aGIFh;
  FILE *pFileAGIF;

  // handle to RGB raw frame from pFrame
  AVFrame *pFrameRGB;
  int numBytesRGB;
  uint8_t *buffer;

  // Allocatre video frame for resized raw RGB
  AVFrame *pFrameRGB_resized;
  int numBytesRGB_resized;
  uint8_t *buffer_resized;

  struct SwsContext *toRGB_ctx;
  struct SwsContext *toRGB_ctx_resized;

};

#endif // INC_MEDIAFRAME_H
