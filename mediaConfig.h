#ifndef INC_MEDIACONFIG_H
#define INC_MEDIACONFIG_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
/*
extern "C"{ // using local ffmpeg directory
#include "avformat.h"
#include "swscale.h"
}
*/
//another possible option to parse argv
//#include <iomanip>

using namespace std;

class mediaConfig{

 public:
  
  /* set debug flag */
  bool debug;

  /* methods */
  mediaConfig(bool);
  ~mediaConfig();
  void display_error();
  void display_help();
  void display_summary();
  bool set_options(int, char**);
  void set_target_size(int, int);

  /* user defined options
     initialization to default values are done in set_options */
  bool version;
  bool verbose;
  bool frame_skip;
  int frame_skip_interval;
  char* filename_in;
  string filenameIn_stem;
  string filepathIn_stem;
  char* filename_out;
  string filenameOut_stem;
  string filepathOut_stem;
  string fileextOut_stem;
  char ofname[256];
  bool mv_sf, kf_jpeg, kf_png, kf_gif, kf_agif;
  float scale_ratio;
  int scale_x;
  int scale_y;
  bool force_16;
  float threshold_darkness, threshold_brightness;
  int scene_count_limit;
  bool time_segment;
  float time_in, time_out;

  /* keyframe sizings */
  int large;
  int medium;
  int small;


  /* auto-generated options */
  //int default_target_x;
  //int default_target_y;
  int target_x;
  int target_y;
  
 private:
  string argument;
};

#endif // INC_MEDIACONFIG_H
