/*
  OVSurGen

  author:
  Makoto Sadahiro
   makoto@tacc.utexas.edu

  This tool is developed for Open Video Digital Library Toolkit Project to parse input video files.
  Dr.Gary Geisler (Univ Texas at Austin, school of information) is the PI for the entire project.

  functionality:
  Scene change detection, meta image/movie files generation
  This tool has dependencies to ffmpeg and gd libraries

  disclaimer:
  As like any open source projects, I have learned and borrowed code segments from others in communities,
  especially from ffmpeg community.  I looked through as much as I can not recall whom I should give
  credit to.  I would like to express my thanks and give credit to the ffmpeg developers/users community.
*/

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "mediaConfig.h"
#include "mediaParse.h"
//already included by dependency files
// using system-wide ffmpeg install directory
extern "C"{
 #include <avformat.h>
 #include <swscale.h>
}
using namespace std;

/* constant for version, debug and etc */

// current version number;  change it here
const string OVSurGen_version= "Beta 1.0";
const string OVSurGen_build_date= __DATE__;
const string OVSurGen_build_time= __TIME__;

// Global debug flag: turn this on/off to enable/disable debug flag globally
// This can be overriden by run option "-debug"
bool g_debug= false;


int main(int argc, char **argv){

  // parsing user argument
  mediaConfig* user_mediaOption= new mediaConfig(g_debug);
  if(!(user_mediaOption->set_options(argc, argv))){

  	// display version number and exit
    if(user_mediaOption->version){
      cout << "Open Video Project" << endl
      		 << " OVSurGen (version "<< OVSurGen_version << " )" << endl
      		 << "build: " << OVSurGen_build_date << " " << OVSurGen_build_time << endl;
    	if(user_mediaOption!=NULL){
    		delete user_mediaOption;
    		user_mediaOption=NULL;
    	}
      return EXIT_FAILURE;
    }

  	if(user_mediaOption!=NULL){
  		delete user_mediaOption;
  		user_mediaOption=NULL;
  	}
    return EXIT_FAILURE;
  }

  // set user defined debug flag
  if(user_mediaOption->debug){
    g_debug=true;
  }
  
  // create parser
  mediaParse* pParser= new mediaParse(g_debug);
  
  // parsing
  if(!(pParser->parse(user_mediaOption))){
  	cerr << "OVSurGen error: parsing media file" << endl
  			 << endl;
  }
  
  if(pParser!=NULL){
  	delete pParser;
  	pParser= NULL;
  }

  if(user_mediaOption->verbose){
  	user_mediaOption->display_summary();
  }

  if(user_mediaOption!=NULL){
  	delete user_mediaOption;
  	user_mediaOption=NULL;
  }
  
  cerr << " OVSurGen: exiting normally" << endl;

  return EXIT_SUCCESS;
}
