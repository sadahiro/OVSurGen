
#include "mediaFrame.h"

using namespace std;

mediaFrame::mediaFrame(bool debug_flag,
		       AVCodecContext* pCodecCtxSrc,
		       int frame_id,
		       AVFrame* pSrcFrame,
		       int x, int y):
  debug(debug_flag),
  pCodecCtx(pCodecCtxSrc),
  iFrame(frame_id),
  pFrame(pSrcFrame),
  width(pCodecCtx->width),
  height(pCodecCtx->height),
  target_x(x), target_y(y){
  
  if(debug) cout << "OVSurGen debug: media Frame constructor" << endl;

  gdimage= NULL;
  toRGB_ctx_resized= toRGB_ctx= NULL;
  buffer_resized= buffer= NULL;
  pFrameRGB_resized= pFrameRGB= NULL;

}


mediaFrame::~mediaFrame(){

  if(debug) cout << "OVSurGen debug: media Frame destructor called" << endl;

  // Free RGB image buffers
  if(NULL!=buffer_resized) delete [] buffer_resized;
  if(NULL!=buffer) delete [] buffer;
  
  // input video frames
  if(NULL!=pFrameRGB_resized) av_free(pFrameRGB_resized);
  if(NULL!=pFrameRGB) av_free(pFrameRGB);

  // free context
  if(NULL!=toRGB_ctx_resized) sws_freeContext(toRGB_ctx_resized);
  if(NULL!=toRGB_ctx) sws_freeContext(toRGB_ctx);

  // free gd temp resource
  if(NULL!=gdimage) gdImageDestroy(gdimage);

  if(debug) cout << "OVSurGen debug: media Frame destructor done" << endl;

}

int mediaFrame::get_id(){

  return iFrame;
}


AVFrame* mediaFrame::get_pFrameRGB(){

  return pFrameRGB;
}


AVFrame* mediaFrame::get_pFrameRGB_resized(){

  return pFrameRGB_resized;
}


gdImagePtr mediaFrame::get_pGdImage(){

  return gdimage;
}


int mediaFrame::initMV(){

  if(debug) cout << "OVSurGen debug: initialize begin" << endl;

  // for producing video file
  // original size buffer
  pFrameRGB=avcodec_alloc_frame();
  if(pFrameRGB==NULL){
    cerr << "OVSurGen error: allocation for RGB frame failed" << endl;
    return -1;
  }

  numBytesRGB=avpicture_get_size(PIX_FMT_RGB24,
				 pCodecCtx->width,
				 pCodecCtx->height);

  buffer= new uint8_t[numBytesRGB];
  avpicture_fill((AVPicture *)pFrameRGB,
		 buffer,
		 PIX_FMT_RGB24,
		 pCodecCtx->width, pCodecCtx->height);

  if(toRGB_ctx==NULL){
    toRGB_ctx= sws_getContext(pCodecCtx->width,
			      pCodecCtx->height,
			      pCodecCtx->pix_fmt,
			      pCodecCtx->width,
			      pCodecCtx->height,
			      PIX_FMT_RGB24,
			      SWS_BICUBIC, NULL, NULL, NULL);
    if(toRGB_ctx==NULL){
      cerr << "OVSurGen error: sws_getContext(toRGB) failed" << endl;
      exit(1);
    }
  }

  sws_scale(toRGB_ctx,
	    pFrame->data,
	    pFrame->linesize,
	    0, pCodecCtx->height,
	    pFrameRGB->data,
	    pFrameRGB->linesize);

  // prepare resized buffer
  pFrameRGB_resized=avcodec_alloc_frame();
  if(pFrameRGB_resized==NULL){
    cerr << "OVSurGen error: allocation for RGB_resized frame failed" << endl;
    return -1;
  }

  numBytesRGB_resized=avpicture_get_size(PIX_FMT_RGB24,
					 target_x,
					 target_y);

  buffer_resized= new uint8_t[numBytesRGB_resized];
  avpicture_fill((AVPicture *)pFrameRGB_resized,
		 buffer_resized,
		 PIX_FMT_RGB24,
		 target_x, target_y);

  if(toRGB_ctx_resized==NULL){
    toRGB_ctx_resized= sws_getContext(pCodecCtx->width,
				      pCodecCtx->height,
				      pCodecCtx->pix_fmt,
				      target_x,
				      target_y,
				      PIX_FMT_RGB24,
				      SWS_BICUBIC, NULL, NULL, NULL);

    if(toRGB_ctx_resized==NULL){
      cerr << "OVSurGen error: sws_getContext_resized(toRGB) failed" << endl;
      exit(1);
    }
  }

  sws_scale(toRGB_ctx_resized,
	    pFrame->data,
	    pFrame->linesize,
	    0, pCodecCtx->height,
	    pFrameRGB_resized->data,
	    pFrameRGB_resized->linesize);

  if(debug)
    cerr << "OVSurGen debug: iFrame= " << iFrame
	 << ", pFrameRGB= " << pFrameRGB
	 << ", pFrameRGB_resized= " << pFrameRGB_resized
	 << endl;


/* redundant because of new initKF and createGD

  // for producing jpeg file
  gdimage= gdImageCreateTrueColor(width, height);

  // Write pixel data
  for(int y=0; y<height; y++)
    for(int x=0; x<width; x++)
      gdimage->tpixels[y][x]=
    	  (((*(char *)(pFrameRGB->data[0] + y * pFrameRGB->linesize[0] + x * 3)) << 16) & 0xff0000)
    	| (((*(char *)(pFrameRGB->data[0] + y * pFrameRGB->linesize[0] + x * 3 + 1)) << 8) & 0xff00)
    	| ((*(char *)(pFrameRGB->data[0] + y * pFrameRGB->linesize[0] + x * 3 + 2)) & 0xFF);

  // Calculate time stamp for file name
  t_sec= floor(
	       ((float)(iFrame))
	       *((float)(pCodecCtx->time_base.num))
	       *1000
	       /((float)(pCodecCtx->time_base.den))
	       +0.5
	       )
    *0.001;

  if(debug) cout << "OVSurGen debug: t_sec= " << t_sec << endl;

  // Free RGB image buffers
  //if(NULL!=buffer) delete [] buffer;
  //if(NULL!=pFrameRGB) av_free(pFrameRGB);

*/

  return 0;
}

int mediaFrame::initKF(){
  
  if(debug) cout << "OVSurGen debug: initKF begin" << endl;

  // for producing video file
  // original size buffer
  pFrameRGB=avcodec_alloc_frame();
  if(pFrameRGB==NULL){
    cerr << "OVSurGen error: allocation for RGB frame failed" << endl;
    return -1;
  }

  numBytesRGB=avpicture_get_size(PIX_FMT_RGB24,
				 pCodecCtx->width,
				 pCodecCtx->height);
  
  buffer= new uint8_t[numBytesRGB];
  avpicture_fill((AVPicture *)pFrameRGB,
		 buffer,
		 PIX_FMT_RGB24,
		 pCodecCtx->width, pCodecCtx->height);

  if(toRGB_ctx==NULL){
    toRGB_ctx= sws_getContext(pCodecCtx->width,
			      pCodecCtx->height,
			      pCodecCtx->pix_fmt,
			      pCodecCtx->width,
			      pCodecCtx->height,
			      PIX_FMT_RGB24,
			      SWS_BICUBIC, NULL, NULL, NULL);
    if(toRGB_ctx==NULL){
      cerr << "OVSurGen error: sws_getContext(toRGB) failed" << endl;
      return -1;  // exit(1);
    }
  }
    
  sws_scale(toRGB_ctx,
	    pFrame->data,
	    pFrame->linesize,
	    0, pCodecCtx->height,
	    pFrameRGB->data,
	    pFrameRGB->linesize);
  
  if(debug) cerr << "OVSurGen debug: iFrame= " << iFrame << ", pFrameRGB= " << pFrameRGB << endl;
  
  // Calculate time stamp for file name
  t_sec= floor(
	       ((float)(iFrame))
	       *((float)(pCodecCtx->time_base.num))
	       *1000
	       /((float)(pCodecCtx->time_base.den))
	       +0.5
	       )
    *0.001;
  
  if(debug) cout << "OVSurGen debug: t_sec= " << t_sec << endl;

  // Free RGB image buffers
  //if(NULL!=buffer) delete [] buffer;
  //if(NULL!=pFrameRGB) av_free(pFrameRGB);

  return 0;
}


bool mediaFrame::createGdImage(){

	// if not NULL, the gdImage already exist
	// just return to caller as normal
	if(gdimage!=NULL) return true;

	// else proceed following

	// for producing jpeg file
	gdimage = gdImageCreateTrueColor(width, height);
	if(gdimage==NULL) return false;

	// Write pixel data
	for(int y=0; y<height; y++)
		for(int x=0; x<width; x++)
			gdimage->tpixels[y][x]=
					(((*(char *)(pFrameRGB->data[0] + y * pFrameRGB->linesize[0] + x * 3)) << 16) & 0xff0000)
					| (((*(char *)(pFrameRGB->data[0] + y * pFrameRGB->linesize[0] + x * 3 + 1)) << 8) & 0xff00)
					| ((*(char *)(pFrameRGB->data[0] + y * pFrameRGB->linesize[0] + x * 3 + 2)) & 0xFF);

	return true;
}


void mediaFrame::saveJpegFrame(
		int dst_x, int dst_y, const char* filepath_out,
    const char* filename_out, const char* fileext_out){

  FILE *pFile;
  char szFilename[strlen(filename_out)+strlen(filepath_out)+strlen(fileext_out)+50];

  gdImagePtr gdOut= gdImageCreateTrueColor(dst_x, dst_y);

  // Open file
  sprintf(szFilename,
	  "%s%s_%09.3f.%s",
	  filepath_out,
	  filename_out,
	  t_sec,
	  fileext_out);
  if(debug) cout << endl << "OVSurGen debug: " << szFilename << endl << endl;
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL){
  	cerr << "OVSurGen error: ******** opening jpeg file failed ********" << endl;
    return;
  }

  gdImageCopyResized(gdOut, gdimage, 0, 0, 0, 0, dst_x, dst_y, width, height);
  gdImageJpeg(gdOut, pFile, -1);

  // Close file
  fclose(pFile);

  // delete local gd resource
  gdImageDestroy(gdOut);

  if(debug){
  	cout << "OVSurGen debug: JPEG " << dst_x << " " << dst_y << " " << width << " " <<  height << endl;
  }

}


void mediaFrame::savePngFrame(
		int dst_x, int dst_y, const char* filepath_out,
    const char* filename_out, const char* fileext_out){

  FILE *pFile;
  char szFilename[strlen(filename_out)+strlen(filepath_out)+strlen(fileext_out)+50];

  gdImagePtr gdOut= gdImageCreateTrueColor(dst_x, dst_y);

  // Open file
  sprintf(szFilename,
	  "%s%s_%09.3f.%s",
	  filepath_out,
	  filename_out,
	  t_sec,
	  fileext_out);
  if(debug) cout << endl << "OVSurGen debug: " << szFilename << endl << endl;
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;

  gdImageCopyResized(gdOut, gdimage, 0, 0, 0, 0, dst_x, dst_y, width, height);
  gdImagePng(gdOut, pFile);

  // Close file
  fclose(pFile);

  // delete local gd resource
  gdImageDestroy(gdOut);

  if(debug){
  	cout << "OVSurGen debug: PNG: " << dst_x << " " << dst_y << " " << width << " " <<  height << endl;
  }

}


void mediaFrame::saveGifFrame(
		int dst_x, int dst_y, const char* filepath_out,
    const char* filename_out, const char* fileext_out){

  FILE *pFile;
  char szFilename[strlen(filename_out)+strlen(filepath_out)+strlen(fileext_out)+50];

  gdImagePtr gdOut= gdImageCreateTrueColor(dst_x, dst_y);

  // Open file
  sprintf(szFilename,
	  "%s%s_%09.3f.%s",
	  filepath_out,
	  filename_out,
	  t_sec,
	  fileext_out);
  if(debug) cout << endl << "OVSurGen debug: " << szFilename << endl << endl;
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;

  gdImageCopyResized(gdOut, gdimage, 0, 0, 0, 0, dst_x, dst_y, width, height);

  //gdImageGifAnimBegin(gdOut, pFile, -1, 0);
  //gdImageGifAnimEnd(pFile);

  gdImageGif(gdOut, pFile);

  // Close file
  fclose(pFile);

  // delete local gd resource
  gdImageDestroy(gdOut);

  if(debug){
  	cout << "OVSurGen debug: GIF: " << dst_x << " " << dst_y << " " << width << " " <<  height << endl;
  }

}


void mediaFrame::initiateAGIF(int dst_x, int dst_y,
		//char* csize,
		const char* filepath_out, const char* filename_out){
  aGIFw= dst_x;
  aGIFh= dst_y;

  char szFilename[strlen(filename_out)+strlen(filepath_out)+50];

  // Open file
  sprintf(szFilename,
	  "%s%s_%09.3f._animated.gif",
	  filepath_out,
	  //csize,
	  filename_out, t_sec);
  pFileAGIF=fopen(szFilename, "wb");
  if(pFileAGIF==NULL){
    cerr << endl << "OVSurGen error: Failed to create " << szFilename << endl;
    exit(-1);
  }

  aGIFOut= gdImageCreateTrueColor(aGIFw, aGIFh);  // I may have to use gdImageCreate(aGIFw, aGIFh)
  gdImageGifAnimBegin(aGIFOut, pFileAGIF, -1, 0);
  gdImageDestroy(aGIFOut);  // I may have to do this at the end of closeAGIF
}


void mediaFrame::pushAGIF(gdImagePtr gdimage_src){

  gdImagePtr tim= gdImageCreateTrueColor(aGIFw, aGIFh);
  gdImageCopyResized(tim, gdimage_src, 0, 0, 0, 0,
		     aGIFw, aGIFh, width, height);
  gdImagePtr cim= gdImageCreatePaletteFromTrueColor(tim, 1, 256);
  gdImageGifAnimAdd(cim, pFileAGIF, 1, 0, 0, 20, gdDisposalNone, NULL);
  gdImageDestroy(cim);
  gdImageDestroy(tim);
}


void mediaFrame:: closeAGIF(){

  gdImagePtr eim= gdImageCreate(aGIFw, aGIFh);
  //int black = gdImageColorAllocate(eim, 0, 0, 0);
  gdImageGifAnimAdd(eim, pFileAGIF, 1, 0, 0, 20, gdDisposalNone, NULL);
  gdImageGifAnimEnd(pFileAGIF);
  
  // Close file
  fclose(pFileAGIF);

  // delete temp gd resource
  gdImageDestroy(eim);
  //gdImageDestroy(aGIFOut);  // can I move this to the end of initiateAGIF?

}
