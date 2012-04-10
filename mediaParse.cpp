#include "mediaParse.h"

using namespace std;

mediaParse::mediaParse(bool debug_flag) :
	debug(debug_flag), frame_list(new vector<mediaFrame*> ){

	if(debug){
		cout << "OVSurGen debug: frame_list size " << frame_list->size() << endl
		     << "OVSurGen debug: object mediaParse created" << endl
		     << "OVSurGen debug: debug option can be overwritten in mediaParse constructor init list"
		     << endl;
	}
}

mediaParse::~mediaParse(){

	if(debug){
		cout << "OVSurGen debug: frame_list size " << frame_list->size() << endl
		     << "OVSurGen debug: object mediaParse destructed" << endl;
	}

	// delete frame list
	while((frame_list->size()) > 0){
		delete frame_list->back();// deleting each pMediaFrame
		frame_list->pop_back();
		if(debug){
			cout << "OVSurGen debug: deleting frame list- " << frame_list->size()
			    << " left" << endl;
		}
	}
	delete frame_list;

	if(debug){
		cout << "OVSurGen debug: object mediaParse deleted" << endl
		     << "OVSurGen debug: exiting mediaParser.destructor" << endl;
	}
}

// video for output stream
AVStream *mediaParse::add_video_stream(AVFormatContext *oc, int codec_id,
    int target_video_width, int target_video_height){
	AVCodecContext *c;
	AVStream *st;

	st = av_new_stream(oc, 0);
	if(!st){
		fprintf(stderr, "Could not alloc stream\n");
		exit(1);
	}

	c = st->codec;
	c->codec_id = (CodecID)(codec_id);
	c->codec_type = CODEC_TYPE_VIDEO;

	/* put sample parameters */
	c->bit_rate = 400000; // 128-384Kb for video conf, 5Mb for DVD quality
	/* how do I decide on bit rate?
	 Can I automate/default optimal rate? */

	/* resolution must be a multiple of two or some player would not play*/
	c->width = target_video_width;
	c->height = target_video_height;
	/* time base: this is the fundamental unit of time (in seconds) in terms
	 of which frame time-stamps are represented. for fixed-fps content,
	 time-base should be 1/frame-rate and time-stamp increments should be
	 identically 1. */
	c->time_base.den = 30000;
	c->time_base.num = 1001;
	// adjust this fps by input file's fps
	// current above setup is about 30fps

	c->gop_size = 12; /* emit one intra frame every twelve frames at most */
	c->pix_fmt = PIX_FMT_YUV420P;
	if(c->codec_id == CODEC_ID_MPEG2VIDEO){
		/* just for testing, we also add B frames */
		c->max_b_frames = 2;
	}
	if(c->codec_id == CODEC_ID_MPEG1VIDEO){
		/* Needed to avoid using macroblocks in which some coeffs overflow.
		 This does not happen with normal video, it just happens here as
		 the motion of the chroma plane does not match the luma plane. */
		c->mb_decision = 2;
	}
	// some formats want stream headers to be separate
	//                                                     what about flv?
	if(!strcmp(oc->oformat->name, "mp4") || !strcmp(oc->oformat->name, "mov")
	    || !strcmp(oc->oformat->name, "3gp")){
		c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	return st;
}

AVFrame *mediaParse::alloc_picture(int pix_fmt, int width, int height){

	AVFrame *picture;
	uint8_t *picture_buf;
	int size;

	picture = avcodec_alloc_frame();
	if(!picture)
		return NULL;
	size = avpicture_get_size((PixelFormat)(pix_fmt), width, height);
	picture_buf = (uint8_t*)(av_malloc(size));
	if(!picture_buf){
		av_free(picture);
		return NULL;
	}
	avpicture_fill((AVPicture *)picture, picture_buf, (PixelFormat)(pix_fmt),
	    width, height);
	// 3rd item casting to PixelFormat.  Not sure needed.

	return picture;
}

void mediaParse::open_video(AVFormatContext *oc, AVStream *st){
	//fix: change void to returning bool for error check

	AVCodec *codec;
	AVCodecContext *c;

	c = st->codec;

	/* find the video encoder */
	codec = avcodec_find_encoder(c->codec_id);
	if(!codec){
		cerr << "OVSurGen error: video codec not found->  Codec_id: " << c->codec_id
		    << endl;
		exit(1);
	}

	/* open the codec */
	if(avcodec_open(c, codec) < 0){
		fprintf(stderr, "open_video: could not open codec\n");
		if(debug){
			cerr << "OVSurGen error: " << c->codec_id << endl;
		}
		exit(1);
	}

	video_outbuf = NULL;
	if(!(oc->oformat->flags & AVFMT_RAWPICTURE)){
		/* allocate output buffer */
		/* XXX: API change will be done */
		/* buffers passed into lav* can be allocated any way you prefer,
		 as long as they're aligned enough for the architecture, and
		 they're freed appropriately (such as using av_free for buffers
		 allocated with av_malloc) */
		video_outbuf_size = 200000;
		/* what is real good size for this?
		 find out how to optimize size */
		video_outbuf = (uint8_t*)(av_malloc(video_outbuf_size));
	}

	/* allocate the encoded raw picture */
	picture = alloc_picture(c->pix_fmt, c->width, c->height);
	if(debug){
		cout << "OVSurGen debug: mediaParse::open_video.allocpicture CW= "
		    << c->width << "  CH= " << c->height << endl;
	}
	if(!picture){
		fprintf(stderr, "Could not allocate picture\n");
		exit(1);
	}

	/* if the output format is not YUV420P, then a temporary YUV420P
	 picture is needed too. It is then converted to the required
	 output format */
	tmp_picture = NULL;
	if(c->pix_fmt != PIX_FMT_YUV420P){
		tmp_picture = alloc_picture(PIX_FMT_YUV420P, c->width, c->height);
		if(!tmp_picture){
			fprintf(stderr, "Could not allocate temporary picture\n");
			exit(1);
		}
	}
}

void mediaParse::write_video_frame(AVFormatContext *oc, AVStream *st,
    AVFrame* src){
	int out_size, ret;
	AVCodecContext *c;
	static struct SwsContext *img_convert_ctx;

	c = st->codec;

	if(img_convert_ctx == NULL){

		// src width and height should have been passed via parameter
		// AVFrame* src is wrong kind for this reason, but they happened
		// to be the same as c->width and c->height
		img_convert_ctx = sws_getContext(c->width, c->height, PIX_FMT_RGB24,
		    c->width, c->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
		if(img_convert_ctx == NULL){
			fprintf(stderr, "Cannot initialize the conversion context\n");
			exit(1);
		}
	}
	sws_scale(img_convert_ctx, src->data, src->linesize, 0, c->height,
	    picture->data, picture->linesize);

	if(oc->oformat->flags & AVFMT_RAWPICTURE){
		// raw video case
		// The API will change slightly in the near futur for that
		AVPacket pkt;
		av_init_packet(&pkt);

		pkt.flags |= PKT_FLAG_KEY;
		pkt.stream_index = st->index;
		pkt.data = (uint8_t *)picture;
		pkt.size = sizeof(AVPicture);

		ret = av_write_frame(oc, &pkt);
	}else{
		// encode the image
		out_size
		    = avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
		// if zero size, it means the image was buffered
		if(out_size > 0){
			AVPacket pkt;
			av_init_packet(&pkt);

			pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
			if(c->coded_frame->key_frame)
				pkt.flags |= PKT_FLAG_KEY;
			pkt.stream_index = st->index;
			pkt.data = video_outbuf;
			pkt.size = out_size;

			// write the compressed frame in the media file
			ret = av_write_frame(oc, &pkt);
		}else{
			ret = 0;
		}
	}
	if(ret != 0){
		fprintf(stderr, "Error while writing video frame\n");
		exit(1);
	}
	frame_count++;

}

void mediaParse::close_video(AVFormatContext *oc, AVStream *st){

	avcodec_close(st->codec);
	av_free(picture->data[0]);
	av_free(picture);
	if(tmp_picture){
		av_free(tmp_picture->data[0]);
		av_free(tmp_picture);
	}
	av_free(video_outbuf);
}

AVStream *mediaParse::add_audio_stream(AVFormatContext *oc, int codec_id){
	AVCodecContext *c;
	AVStream *st;

	st = av_new_stream(oc, 1);
	if(!st){
		fprintf(stderr, "Could not alloc stream\n");
		exit(1);
	}

	c = st->codec;
	c->codec_id = (CodecID)(codec_id);
	c->codec_type = CODEC_TYPE_AUDIO;

	/* put sample parameters */
	c->bit_rate = 128000; // 128K is typical for 10:1 encoding ratio, was 64K
	c->sample_rate = 44100; // 44.1KHz is standard for CD
	c->channels = 2;
	return st;
}

void mediaParse::open_audio(AVFormatContext *oc, AVStream *st){
	AVCodecContext *c;
	AVCodec *codec;

	c = st->codec;

	/* find the audio encoder */
	codec = avcodec_find_encoder(c->codec_id);
	if(!codec){
		fprintf(stderr, "audio codec not found\n");
		exit(1);
	}

	/* open it */
	if(avcodec_open(c, codec) < 0){
		fprintf(stderr, "could not open codec\n");
		exit(1);
	}

	/* init signal generator */
	t = 0;
	tincr = 2 * M_PI * 110.0 / c->sample_rate;
	/* increment frequency by 110 Hz per second */
	tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

	audio_outbuf_size = 10000;
	audio_outbuf = (uint8_t*)(av_malloc(audio_outbuf_size));

	/* ugly hack for PCM codecs (will be removed ASAP with new PCM
	 support to compute the input frame size in samples */
	if(c->frame_size <= 1){
		audio_input_frame_size = audio_outbuf_size / c->channels;
		switch(st->codec->codec_id){
		case CODEC_ID_PCM_S16LE:
		case CODEC_ID_PCM_S16BE:
		case CODEC_ID_PCM_U16LE:
		case CODEC_ID_PCM_U16BE:
			audio_input_frame_size >>= 1;
			break;
		default:
			break;
		}
	}else{
		audio_input_frame_size = c->frame_size;
	}
	samples = (int16_t*)(av_malloc(audio_input_frame_size * 2 * c->channels));
}

/* prepare a 16 bit dummy audio frame of 'frame_size' samples and
 'nb_channels' channels */
void mediaParse::get_audio_frame(int16_t *samples, int frame_size,
    int nb_channels){
	int j, i, v;
	int16_t *q;

	q = samples;
	for(j = 0; j < frame_size; j++){
		v = (int)(sin(t) * 10000);
		for(i = 0; i < nb_channels; i++)
			*q++ = v;
		t += tincr;
		tincr += tincr2;
	}
}

void mediaParse::write_audio_frame(AVFormatContext *oc, AVStream *st){
	AVCodecContext *c;
	AVPacket pkt;
	av_init_packet(&pkt);

	c = st->codec;

	get_audio_frame(samples, audio_input_frame_size, c->channels);

	pkt.size = avcodec_encode_audio(c, audio_outbuf, audio_outbuf_size, samples);

	pkt.pts = av_rescale_q(c->coded_frame->pts, c->time_base, st->time_base);
	pkt.flags |= PKT_FLAG_KEY;
	pkt.stream_index = st->index;
	pkt.data = audio_outbuf;

	/* write the compressed frame in the media file */
	if(av_interleaved_write_frame(oc, &pkt) != 0){
		fprintf(stderr, "Error while writing audio frame\n");
		exit(1);
	}
}

void mediaParse::close_audio(AVFormatContext *oc, AVStream *st){
	avcodec_close(st->codec);

	av_free(samples);
	av_free(audio_outbuf);
}

bool mediaParse::parse(mediaConfig* media_option){

	av_register_all();

	// open source video file
	if(0 != (av_open_input_file(&pFormatCtx, media_option->filename_in, NULL, 0,
	    NULL))){
		cerr << "OVSurGen error: opening source video file failed" << endl;
		return false;
	}

	// retrieve stream information
	if(av_find_stream_info(pFormatCtx) < 0){
		cerr << "OVSurGen error: no stream information found in source video"
		    << endl;
		return false;
	}

	// output the file info
	if(media_option->verbose){
		dump_format(pFormatCtx, 0, media_option->filename_in, false);
	}

	// set empty value for stream index before retrieving it
	int video_stream_index = -1;
	int audio_stream_index = -1;

	if((media_option->verbose) && (pFormatCtx->nb_streams > 1)){
		cout << "OVSurGen message: input file contains " << pFormatCtx->nb_streams
		    << " streams" << endl;
	}

	// Find the first video stream
	for(unsigned int index = 0; index < pFormatCtx->nb_streams; index++){

		if(pFormatCtx->streams[index]->codec->codec_type == CODEC_TYPE_VIDEO){
			if(-1 == video_stream_index){ // ensures to get FIRST stream ONLY
				video_stream_index = index;
			}else{
				if(media_option->verbose){
					cout << "OVSurGen warning: more than one video stream exists in this file" << endl
							 << "OVSurGen warning: using first stream" << endl;
				}
			}
		}

		if(pFormatCtx->streams[index]->codec->codec_type == CODEC_TYPE_AUDIO){
			if(-1 == audio_stream_index){ // ensures to get FIRST stream ONLY
				audio_stream_index = index;
			}else{
				if(media_option->verbose){
					cout
					    << "OVSurGen warning: more than one audio stream exists in this file" << endl
					    << "OVSurGen warning: using first stream" << endl;
				}
			}
		}

	}
	if(-1 == video_stream_index){
		cerr << "OVSurGen error: no video stream in the source video file is found"
		    << endl;
		return false;
	}
	if(debug){
		if(0 != video_stream_index)
			cerr << "OVSurGen debug: video stream_index(should be 0)="
			    << video_stream_index << endl;
	}
	if(-1 == audio_stream_index){
		cerr
		    << "OVSurGen error: no audio stream in the source video file is found- "
		    << audio_stream_index << endl;
	}
	if(debug){
		if(0 != audio_stream_index)
			cerr << "OVSurGen debug: audio stream_index(should be 0)="
			    << audio_stream_index << endl;
	}

	pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
	if(-1 < audio_stream_index){
		aCodecCtx = pFormatCtx->streams[audio_stream_index]->codec;
	}

	if(debug){
		cerr << "OVSurGen debug: source AVCodecCentext bitrate:= "
		    << pCodecCtx->bit_rate << endl
		    << "OVSurGen debug:       source AVFormat bitrate:= "
		    << pFormatCtx->bit_rate << endl
		    << "OVSurGen debug: !! fine out why different from bitrate from dump !!"
		    << endl << "OVSurGen debug:                      duration:= "
		    << pFormatCtx->duration / 1000000 << endl;
	}

	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(-1 < audio_stream_index)
		aCodec = avcodec_find_decoder(aCodecCtx->codec_id);

	if(debug)
		cout << "OVSurGen debug: Source CODEC: V." << pCodec << " : A." << aCodec
		    << endl;

	if(pCodec == NULL){
		cerr << "OVSurGen error: no appropriate codec for codec context was found"
		    << endl;
		return false;
	}

	// Open codec
	if(0 > avcodec_open(pCodecCtx, pCodec)){
		cerr << "OVSurGen error: opening codec for video codec context failed"
		    << endl;
		return false;
	}

	if(-1 < audio_stream_index){
		if(0 > avcodec_open(aCodecCtx, aCodec)){
			cerr << "OVSurGen error: opening codec for audio codec context failed"
			    << endl;
			return false;
		}
	}

	// Preparing for output frame
	// This is done here instead of mediaConfig because of dependency to the dimension of src file
	media_option->set_target_size(pCodecCtx->width, pCodecCtx->height);

	// Allocate pFrame, primary video frame storage from decoder
	pFrame = avcodec_alloc_frame();
	if(pFrame == NULL){
		cerr << "OVSurGen error: allocation for source frame failed" << endl;
		return false;
	}

	// output type is movie
	if(media_option->mv_sf){
		if(media_option->fileextOut_stem == "mpg")
			outputFormat = guess_format("mpeg", NULL, //media_option->filename_out,
			    NULL);
		else
			outputFormat = guess_format(media_option->fileextOut_stem.c_str(), NULL, //media_option->filename_out,
			    NULL);

		if(!outputFormat){
			cerr << "OVSurGen error: guess_format for "
			    << media_option->filenameOut_stem << " failed" << endl;
			return false;
		}

		outputContext = avformat_alloc_context(); //av_alloc_format_context() is deprecated
		if(!outputContext){
			cerr << "OVSurGen error: avformat_alloc_context failed" << endl;
			return false;
		}

		outputContext->oformat = outputFormat;
		snprintf(outputContext->filename, sizeof(outputContext->filename), "%s",
		    media_option->ofname);

		video_stream = NULL;
		if(outputFormat->video_codec != CODEC_ID_NONE){
			video_stream = add_video_stream(outputContext, outputFormat->video_codec,
			    media_option->target_x, media_option->target_y);
		}

		audio_stream = NULL;
		//    if(-1<audio_stream_index)
		//      if(outputFormat->audio_codec!=CODEC_ID_NONE){
		//	audio_stream= add_audio_stream(outputContext,
		//				       outputFormat->audio_codec);
		//      }

		if(av_set_parameters(outputContext, NULL) < 0){
			cerr << "OVSurGen error: mediaParse av_set_parameters returned error"
			    << endl;
			return false;
		}

		if(media_option->verbose){
			dump_format(outputContext, 0, media_option->ofname, 1);
		}

		if(video_stream){
			open_video(outputContext, video_stream);
		}
		if(audio_stream){
			open_audio(outputContext, audio_stream);
		}

		// open an output file
		if(!(outputFormat->flags & AVFMT_NOFILE)){
			if(url_fopen(&outputContext->pb, media_option->ofname, URL_WRONLY) < 0){
				cerr << "OVSurGen error: could not open " << media_option->ofname
				    << endl;
				return false;
			}
		}

		/* write the stream header, if any */
		av_write_header(outputContext);

	} // output type is movie


	// packet is main handle to a video frame
	AVPacket packet;
	//packet= avalloc_frame();

	// non-zero if read in avcodec_decode_video was successful
	int frameFinished;

	// Read frames and save every N-frames to disk
	int file_index = 0;
	int skip_counter = 0;
	bool spool_done = false;

	float fps, fps_f;
	fps_f = (float)(pFormatCtx->streams[video_stream_index]->r_frame_rate.num)
	    / (float)(pFormatCtx->streams[video_stream_index]->r_frame_rate.den);
	fps = (float)(pCodecCtx->time_base.den) / (float)(pCodecCtx->time_base.num);

	if(media_option->debug){
		cerr << "OVSurGen debug: fps_f= " << fps_f << "  fps:" << fps << endl;
	}

	if(fps_f != fps){
		cerr << "OVSurGen warning: stream fps:= " << fps
		    << " differs from container fps:= " << fps_f << endl;

		// use reasonable determination of which one to use
		if((fps > 3) && (fps <= 64)){
			cout << "OVSurGen warning: using stream's frame rate of " << fps << endl;
		}else{
			fps = fps_f;
			cout << "OVSurGen warning: using container's frame rate of " << fps
			    << endl;
		}
	}

	if((debug) || (media_option->verbose)){
		cout << "OVSurGen message: duration= " << (pFormatCtx->duration / 1000000)
		    << endl;
	}

	// modifying skip frame interval if output is not movie
	// this is to reduce memory consumption of scene change detection for long movie clips
	if((!(media_option->frame_skip)) && (!(media_option->mv_sf))){
		media_option->frame_skip_interval = ((pFormatCtx->duration) / 10000000);
		// 10000000 is a ball park figure to get interval to be about  64; Get a better method here
		if(media_option->frame_skip_interval < 64){
			(media_option->frame_skip_interval) = 64;
		}
		if(media_option->verbose){
			cout << "OVSurGen message: skip frame interval was not set by user" << endl
					 << "OVSurGen message: " << media_option->frame_skip_interval
			    << " is set for frame skip interval" << endl;
		}
	}

	// calculate time index for subsegment
	float tIn_index = fps * (media_option->time_in);
	float tOut_index = fps * (media_option->time_out);

	if((media_option->verbose) && (media_option->time_segment)){
		//cout.precision(2);
		cout << endl << "OVSurGen debug: extracting sub segment-" << endl << "  in:" << tIn_index
		    << " out:" << tOut_index << " fps:" << fps << endl << endl;
	}

	while((av_read_frame(pFormatCtx, &packet) >= 0) && (!spool_done)){

		// add gate for excerpt here

		if(packet.stream_index == video_stream_index){

			// Decode video frame
			// you need to do this due to the dependency to prev frames
			int adv_ret = avcodec_decode_video(pCodecCtx, pFrame, &frameFinished,
			    packet.data, packet.size);
			if(adv_ret < 0){
				cout << "OVSurGen error: decoding a frame failed" << endl;
				return false;
			}

			if(debug){
				if(adv_ret != (packet.size)){
					cout << "OVSurGen debug: avcodec_decode_video read: " << adv_ret
							 << "packet.size:" << packet.size << endl;
					if(adv_ret < ((packet.size) + FF_INPUT_BUFFER_PADDING_SIZE)){
						cout << "OVSurGen debug: packet.data, " << sizeof(packet.size)
						    << " must be " << FF_INPUT_BUFFER_PADDING_SIZE
						    << " bigger than the return_value of avcodec_decode_video:"
						    << adv_ret << " and packet.size: " << packet.size << endl
						    << " frameFinished:" << frameFinished << endl;
					}
					else{
						cerr << "OVSurGen error: avcodec_decode_video failure" << endl
								 << "OVSurGen error: FF_INPUT_BUFFER_PADDING_SIZE exceeded" << endl;
					}
				}
			}

			// Did we get a video frame?
			if(frameFinished < 0){
				cerr
				    << "OVSurGen error: avcodec_decode_video has returned non-positive frameFinished count "
				    << frameFinished << endl;
				return false;
			}

			if(frameFinished){

				if(0 == skip_counter){
					if(debug)
						cout << endl;

					if((media_option->time_segment) && (tOut_index < file_index))
						spool_done = true;

					if((!(media_option->time_segment)) || (tIn_index < file_index)){

						pMediaFrame = new mediaFrame(debug, pCodecCtx, file_index, pFrame,
						    media_option->target_x, media_option->target_y);

						if(media_option->mv_sf){
							pMediaFrame->initMV();
						}else{
							pMediaFrame->initKF();
						}

						/*
						 // use this section to extract jpeg for ALL keyframes
						 pMediaFrame->saveJpegFrame(media_option->target_x,
						 media_option->target_y,
						 (media_option->filepathOut_stem).c_str(),
						 (media_option->filenameOut_stem).c_str(),
						 (media_option->fileextOut_stem).c_str());
						 */

						if((media_option->kf_jpeg) || (media_option->kf_png)
						    || (media_option->kf_gif) || (media_option->kf_agif)){
							frame_list->push_back(pMediaFrame);
						}

						// Save a frame to output movie file
						// specification of new size is within AVStream video_stream
						else if(media_option->mv_sf){
							write_video_frame(outputContext, video_stream,
							    pMediaFrame->get_pFrameRGB_resized());
							delete pMediaFrame;
						}else{
							cout
							    << "OVSurGen error: mediaParse::parse; incorrect user defined media type"
							    << endl;
						}
					}
				}
				skip_counter++;
				file_index++;
				if(skip_counter >= (media_option->frame_skip_interval)){
					skip_counter = 0;
				}
			} // if(frameFinished)
		} // if(packet.stream_index==video_stream_index)


		/*
		 if(packet.stream_index==audio_stream_index){
		 if(av_write_frame(outputContext, &packet)!=0){
		 cerr << "OVSurGen error: writing audio frame failed" << endl;
		 exit(1);
		 }
		 */

		/*
		 avcodec_decode_audio2(pCodecCtx, pFrame, &frameFinished,
		 packet.data, packet.size);
		 if(frameFinished < 0){
		 cerr << "OVSurGen error: avcodec_decode_audio failed" << endl
		 << "OVSurGen error: frames finished= " << frameFinished << endl;
		 return false;
		 }
		 if(frameFinished){
		 write_audio_frame(outputContext,
		 audio_stream);

		 } // endif(frameFinished)
		 */

		//} // endif(packet.stream_index==audio_stream_index)

		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}

	// media spooling done.  Process the frames

	if((media_option->kf_jpeg) || (media_option->kf_png)
	    || (media_option->kf_gif) || (media_option->kf_agif)){
		int skip_frame_count = frame_list->size();

		if(debug)
			cout << "OVSurGen debug: frame_list->size()=" << frame_list->size()
			     << " ?= 1+file_index/media_option->frame_skip_interval: " << 1
			     + file_index / media_option->frame_skip_interval << endl;

		mediaAnalysis* me = new mediaAnalysis(debug, pCodecCtx->width,
		    pCodecCtx->height, skip_frame_count);

		float thresholdMUX= (float)((pCodecCtx->height)*(pCodecCtx->width)*3*255)/100.0;
		if(debug)
			cout << "OVSurGen debug: thresholdMUX for darkness test is " << thresholdMUX << endl
			     << " Darkness threshold = "
			     << (unsigned long long int)((media_option->threshold_darkness)*thresholdMUX) << endl
			     << " Brightness threshold = "
			     << (unsigned long long int)((media_option->threshold_brightness)*thresholdMUX) << endl
			     << endl;

		// create delta-gram
		me->create_deltagram(frame_list,
				(unsigned long long int)((media_option->threshold_darkness)*thresholdMUX),
				(unsigned long long int)((media_option->threshold_brightness)*thresholdMUX));
		if(debug)
			cout << "OVSurGen debug: deltagram list created" << endl
			     << endl;

		// creating rank
		if(debug)
			cout << "OVSurGen debug: ranking frame list with sc limit= "
			    << media_option->scene_count_limit << endl;
		unsigned int crit_hist = me->get_critical_pt(
		    media_option->scene_count_limit);
		if(debug){
			cout << "OVSurGen debug: skip_frame_count=" << skip_frame_count << endl;
			cout << "OVSurGen debug: critical delta point= " << crit_hist << endl;
		}

		// height[f(width)] = width * (height/width)ratio
		float i_ar_muxer = (float)(pCodecCtx->height) / (float)(pCodecCtx->width);

		mediaFrame* pCurrAGIF = NULL;
		bool status_aGIF = false;

		// test scene change by critical num on histogram, and export jpeg and aGIF

		if(media_option->verbose)
			cout << endl << "OVSurGen message: scene change list -" << endl;

		if(media_option->kf_jpeg){
			// ready the container
			if(media_option->verbose)
				cout << "OVSurGen message:     jpeg keyframe:" << endl
				     << "OVSurGen message: { ";
			for(int jp = 0; jp < skip_frame_count; jp++){

				if((me->get_d_element(jp)) >= crit_hist){

					if(debug){
						cerr << "OVSurGen debug: " << jp << ": " << me->get_d_element(jp)
						    << endl << "OVSurGen debug: parser- "
						    << (*frame_list)[jp]->get_pFrameRGB() << " invoking "
						    << (*frame_list)[jp] << endl
						    << "OVSurGen debug: " << i_ar_muxer << "  "
						    << media_option->large * i_ar_muxer << "  "
						    << (int)(media_option->large * i_ar_muxer) << endl;
					}

					if(media_option->verbose)
						cout << jp << " ";  // don't put "OVSurGen debug: "

					// extracting jpegs
					// create image in each frame and issue save jpeg command
					(*frame_list)[jp]->createGdImage();
					(*frame_list)[jp]->saveJpegFrame(media_option->target_x,
					    media_option->target_y, (media_option->filepathOut_stem).c_str(),
					    (media_option->filenameOut_stem).c_str(),
					    (media_option->fileextOut_stem).c_str());

				}
			} // "for(int jp = 0; jp < skip_frame_count; jp++){"

			if(media_option->verbose)
				cout << "}" << endl << "OVSurGen message: end of keyframe list." << endl;
		} // "if(media_option->kf_jpeg)"

		else if(media_option->kf_png){
			// ready the container
			if(media_option->verbose)
				cout << "OVSurGen message:     png keyframe:" << endl
				    << "OVSurGen message: { ";
			for(int jp = 0; jp < skip_frame_count; jp++){

				if((me->get_d_element(jp)) >= crit_hist){

					if(debug){
						cout << "OVSurGen debug: " << jp << ": " << me->get_d_element(jp)
						    << endl << "OVSurGen debug: parser- "
						    << (*frame_list)[jp]->get_pFrameRGB() << " invoking "
						    << (*frame_list)[jp] << endl << i_ar_muxer << "  "
						    << media_option->large * i_ar_muxer << "  "
						    << (int)(media_option->large * i_ar_muxer) << endl;
					}

					if(media_option->verbose)
						cout << jp << " ";

					// extracting pngs
					// create image in each frame and issue save png command
					(*frame_list)[jp]->createGdImage();
					(*frame_list)[jp]->savePngFrame(media_option->target_x,
					    media_option->target_y, (media_option->filepathOut_stem).c_str(),
					    (media_option->filenameOut_stem).c_str(),
					    (media_option->fileextOut_stem).c_str());
				}
			} // "for(int jp = 0; jp < skip_frame_count; jp++){"

			if(media_option->verbose)
				cout << "}" << endl << "OVSurGen message: end of keyframe list." << endl;
		} // "if(media_option->kf_png)"

		else if(media_option->kf_gif){
			// ready the container
			if(media_option->verbose)
				cout << "OVSurGen message:    gif keyframe:" << endl
				    << "OVSurGen message: { ";
			for(int jp = 0; jp < skip_frame_count; jp++){

				if((me->get_d_element(jp)) >= crit_hist){

					if(debug){
						cout << "OVSurGen debug: " << jp << ": " << me->get_d_element(jp)
						    << endl << "OVSurGen debug: parser- "
						    << (*frame_list)[jp]->get_pFrameRGB() << " invoking "
						    << (*frame_list)[jp] << endl << i_ar_muxer << "  "
						    << media_option->large * i_ar_muxer << "  "
						    << (int)(media_option->large * i_ar_muxer) << endl;
					}

					if(media_option->verbose)
						cout << jp << " ";

					// extracting gifs
					// create image in each frame and issue save gif command
					(*frame_list)[jp]->createGdImage();
					(*frame_list)[jp]->saveGifFrame(media_option->target_x,
					    media_option->target_y, (media_option->filepathOut_stem).c_str(),
					    (media_option->filenameOut_stem).c_str(),
					    (media_option->fileextOut_stem).c_str());
				}
			} // "for(int jp = 0; jp < skip_frame_count; jp++){"

			if(media_option->verbose)
				cout << "}" << endl << "OVSurGen message: end of keyframe list." << endl;
		} // "if(media_option->kf_gif)"

		// this is the section specific to agif frames
		// first Frame takes care of holding animGIF
		// anim GIF hold ptr to host frame to create aGIF
		// pass curr gdImage to host frame's push
		// close host frame's aGIF when done
		else if(media_option->kf_agif){
			for(int jp = 0; jp < skip_frame_count; jp++){

				// first create gdImage on this frame obj
				(*frame_list)[jp]->createGdImage();

				if((me->get_d_element(jp)) >= crit_hist){

					if(debug)
						cout << "OVSurGen debug: " << jp << me->get_d_element(jp) << endl;

					if(debug)
						cout << "OVSurGen debug: parser- "
						    << (*frame_list)[jp]->get_pFrameRGB() << " invoking "
						    << (*frame_list)[jp] << endl;

					if(debug)
						cout << "OVSurGen debug: " << i_ar_muxer << "  "
						    << media_option->large * i_ar_muxer << "  "
						    << (int)(media_option->large * i_ar_muxer) << endl;

					// most first agif frame
					if(!status_aGIF){
						// ready the container
						if(media_option->verbose)
							cout << "OVSurGen message:    animated gif:" << endl
							    << "OVSurGen message: {";
						pCurrAGIF = (*frame_list)[jp];
						pCurrAGIF->initiateAGIF(media_option->target_x,
						    media_option->target_y,
						    (media_option->filepathOut_stem).c_str(),
						    (media_option->filenameOut_stem).c_str());
						status_aGIF = true;
						// push first image
						if(media_option->verbose)
							cout << jp;
						pCurrAGIF->pushAGIF(pCurrAGIF->get_pGdImage());

						// last agif frame and following first agif frame
					}else{
						if(media_option->verbose)
							cout << "," << jp;
						pCurrAGIF->pushAGIF((*frame_list)[jp]->get_pGdImage());
						// close current container
						if(media_option->verbose)
							cout << "}" << endl;
						pCurrAGIF->closeAGIF();

						// then open new agif container with first frame
						if(media_option->verbose)
							cout << "OVSurGen message: {";
						pCurrAGIF = (*frame_list)[jp];
						pCurrAGIF->initiateAGIF(media_option->target_x,
						    media_option->target_y,
						    //(char*)("slideshow"),
						    (media_option->filepathOut_stem).c_str(),
						    (media_option->filenameOut_stem).c_str());
						// push first image
						if(media_option->verbose)
							cout << jp;
						pCurrAGIF->pushAGIF(pCurrAGIF->get_pGdImage());
					}
				} // crit histgram

				// middle agif frames
				else{

					// animGIF
					if(status_aGIF){
						// push mid image
						if(media_option->verbose)
							cout << "," << jp;
						pCurrAGIF->pushAGIF((*frame_list)[jp]->get_pGdImage());
					}
					if(jp == (skip_frame_count - 1)){
						// close current container
						if(media_option->verbose)
							cout << "}" << endl;
						pCurrAGIF->closeAGIF();
					}
				}// crit histogram else
			}// for loop with jp


			/*
			 // create animated GIF from ALL skip frames
			 *

			 //+strlen(filepath_out)+strlen(fileext_out)

			 char agFilename[strlen((media_option->filenameOut_stem).c_str()) + 50];
			 sprintf(agFilename, "%s%s_all.gif",
			 (media_option->filepathOut_stem).c_str(),
			 (media_option->filenameOut_stem).c_str());
			 FILE *out;
			 out = fopen(agFilename, "wb");

			 gdImagePtr im = gdImageCreate(media_option->target_x,
			 media_option->target_y);
			 gdImageGifAnimBegin(im, out, -1, 0);
			 gdImagePtr cim;
			 gdImagePtr tim;

			 for(int i = 0; (i < skip_frame_count); i++){
			 tim = gdImageCreateTrueColor(media_option->target_x,
			 media_option->target_y);
			 gdImageCopyResized(tim, (*frame_list)[i]->get_pGdImage(), 0, 0, 0, 0,
			 media_option->target_x, media_option->target_y, pCodecCtx->width,
			 pCodecCtx->height);
			 cim = gdImageCreatePaletteFromTrueColor(tim, 1, 256);
			 gdImageGifAnimAdd(cim, out, 1, 0, 0, 10, gdDisposalNone, NULL);
			 gdImageDestroy(cim);
			 gdImageDestroy(tim);
			 }
			 gdImagePtr eim = gdImageCreate(media_option->target_x,
			 media_option->target_y);
			 //int black = gdImageColorAllocate(eim, 0, 0, 0);
			 gdImageGifAnimAdd(eim, out, 1, 0, 0, 40, gdDisposalNone, NULL);
			 gdImageGifAnimEnd(out);
			 fclose(out);
			 gdImageDestroy(eim);
			 gdImageDestroy(im);
			 */

		} // if agif
		else{
			cerr
			    << "OVSurGen error: nothing is parsed - file format chosen is not supported"
			    << endl;
		}
		// clean up media analysis
		delete me;
	}// if keyframe or agif


	// close each codec
	if(video_stream){
		close_video(outputContext, video_stream);
		if(media_option->verbose)
			cout << "OVSurGen message: closing codec for video_stream" << endl;
	}
	if(audio_stream){
		close_audio(outputContext, audio_stream);
		if(media_option->verbose)
			cout << "OVSurGen message: closing codec for audio_stream" << endl;
	}

	if(media_option->mv_sf){

		// write the trailer, if any
		av_write_trailer(outputContext);

		// free the streams
		for(unsigned int i = 0; i < outputContext->nb_streams; i++){
			av_freep(&outputContext->streams[i]->codec);
			av_freep(&outputContext->streams[i]);
		}

		if(!(outputFormat->flags & AVFMT_NOFILE)){
			// close the output file
			url_fclose(outputContext->pb);
		}
		if(media_option->verbose)
			cout << "OVSurGen message: output file closed" << endl;

		// free output stream
		av_free(outputContext);
	}

	// input video frames
	av_free(pFrame);

	// src file
	avcodec_close(pCodecCtx);
	av_close_input_file(pFormatCtx);

	// change this to sc detect count later
	return true;
}
