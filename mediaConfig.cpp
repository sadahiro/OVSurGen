
#include "mediaConfig.h"

using namespace std;


mediaConfig::mediaConfig(bool debug_flag):debug(debug_flag){

  if(debug){
    cout << "OVSurGen debug: object mediaConfig created" << endl;
  }
}

mediaConfig::~mediaConfig(){

  // delete filenames on heap

  if(debug){
    cout << "OVSurGen debug: object mediaConfig deleted" << endl;
  }
}

void mediaConfig::display_error(){

	cout << "OVSurGen error: parsing user option" << endl
	     << "OVSurGen error:     please use option -help for more detailed help" << endl;
}

void mediaConfig::display_help(){
  
  // display of basic usage
	// alignment is within:
	//      "88888888888888888888888888888888888888888888888888888888888888888888888888888888"

	cout << endl
       << "Usage: ovsurgen -version | -v | -help | --help                                  " << endl
       << "       ovsurgen [options] -if [path]filename_in.extension [options]             " << endl
       << "                          -of [path]filename_out.extension [options]            " << endl
       << "                                                                                " << endl
       << "mutually exclusive options:                                                     " << endl
       << "-version | -v               displays build version                              " << endl
       << "-help|--help                displays this help                                  " << endl
       << "                                                                                " << endl
       << "Simultaneous options:                                                           " << endl
       << "-verbose                                                                        " << endl
       << "-if filename.file_extension input filename                                      " << endl
       << "-of filename.file_extension output filename                                     " << endl
       << "                            supported output file types are:                    " << endl
       << "                             movie files-        mov, mpeg, mpg etc             " << endl
       << "                             image files-        gif|jpeg|jpg                   " << endl
       << "                                                 (gif not supported yet)        " << endl
       << "                             moving image file-  agif(animated gif)             " << endl
       << "-interval count             fast forward frame interval count                   " << endl
       << "                            default count is 64                                 " << endl
       << "-time_segment start end     specifies start and end of movie in seconds         " << endl
       << "-scene count                specifies maximum count for scene change            " << endl
       << "-threshold_d percentage     specifies threshold for frames too dark             " << endl
       << "                            percentage from complete black (less than 100.0)    " << endl
       << "                            good start point is 2.0                             " << endl
       << "-threshold_b percentage     specifies threshold for frames too bright           " << endl
       << "                            percentage from complete white (less than 100.0)    " << endl
       << "                            good start point is 2.0                             " << endl
       << "-scale_ratio ratio          output scale 0.01-1.00                              " << endl
       << "-scale_x width              output x-dimension by pixel count                   " << endl
       << "-scale_y height             output y-dimension by pixel count                   " << endl
       << "                            later scale factor in user  argument has higher     " << endl
       << "                            precedence, and aspect ratio is automatically       " << endl
       << "                            preserved unless both scale_x and scale_y are       " << endl
       << "                            specified simultaneously                            " << endl
       << "-force16                    forces scale to be divisible by 16                  " << endl
       << "                            use this option when you see shifted color          " << endl
       << endl;
}

void mediaConfig::display_summary(){

  cout << endl
  << " OVSurGen       --- argument summary ---" << endl
  << "    frame_skip_interval: " << frame_skip_interval << endl
  << "                verbose: " << verbose << " (0: off, 1: on)" << endl
  << "            filename in: " << filename_in << endl;
  if(mv_sf) cout
  << "           filename out: " << ofname << endl;
  else cout
  << "           filename out: " << filename_out << endl;
  cout
  //<< " scale ratio           : " << scale_ratio << endl
  //<< "  scale x, y           : " << scale_x << "," << scale_y << endl
  << "          output format: " << fileextOut_stem << endl
  << "      scene_count_limit: " << scene_count_limit << endl
  << "             spool type: ";
  if(mv_sf){
    if(frame_skip_interval==1)
    	cout << "subsegment_excerpt";
    else
    	cout << "fastforward";
    cout << endl;
  }
  if(kf_jpeg)
  	cout << "keyframe jpeg" << endl;
  if(kf_png)
  	cout << "keyframe png" << endl;
  if(kf_gif)
  	cout << "keyframe gif" << endl;
  if(kf_agif)
  	cout << "keyframe animated_gif" << endl;
  if(time_segment){
  	cout << "       sub segment time: " << time_in  << " - " << time_out << " sec" << endl;
  }
  cout << endl;
}

bool mediaConfig::set_options(int c, char** v){

  // set default values
  argument = "empty";
  version = false;
  verbose = false;
	filename_in = filename_out = NULL;
  mv_sf = kf_jpeg= kf_png= kf_gif = kf_agif = false;
	frame_skip = false;
  //default_target_x = 320;
  //default_target_y = 240;
  scale_ratio = 0.0;
  scale_x = scale_y = 0;
  target_x = target_y = 0;
  scene_count_limit = 24;
  force_16 = false;
  threshold_darkness = threshold_brightness = 0;
  time_segment = false;
  time_in= time_out = 0.0;

  /* local debug flag */
  //debug=true;

  while(--c){
    argument= *(++v);

    if((argument=="-help")||(argument=="--help")){
      display_help();
      return false;
    }
    else if((argument=="-version")||(argument=="-v")){
      version=true;
      return false;
    }
    else if(argument=="-debug"){
      debug=true;
    }
    else if(argument=="-verbose"){
      verbose= true;
    }
    else if(argument=="-if"){
      c--;
      if(c>0){
        filename_in= *(++v);
      }else{
        display_error();
        return false;
      }
    }
    else if(argument=="-of"){
      c--;
      if(c>0){
        filename_out= *(++v);
      }else{
        display_error();
        return false;
      }
    }
    else if(argument=="-interval"){
      c--;
      if(c>0){
      	frame_skip= true;
        frame_skip_interval = atoi(*(++v));
        if(frame_skip_interval < 1){
          cerr << "OVSurGen error: skip count has to be larger than 1" << endl;
          display_error();
          return false;
        }
      }else{
        display_error();
        return false;
      }
    }
    else if(argument=="-threshold_d"){
    	c--;
    	if(c>0){
    		threshold_darkness= (float)(atof(*(++v)));
    		if((threshold_darkness<=0.0)||(100.0<threshold_darkness)){
    			display_error();
    			return false;
    		}
    	}else{
    		display_error();
    		return false;
    	}
    }
    else if(argument=="-threshold_b"){
    	c--;
    	if(c>0){
    		threshold_brightness = (float)(atof(*(++v)));
				if((threshold_brightness <= 0.0) || (100.0 < threshold_brightness)){
					display_error();
					return false;
				}
			}else{
    		display_error();
    		return false;
    	}
    }
    else if(argument=="-scale_ratio"){
      c--;
      if(c>0){
        scale_ratio= atof(*(++v));
        if(scale_ratio==0.0){
        	cerr << "OVSurGen error: scale ratio has to be bigger than zero" << endl;
        	return false;
        }
      }else{
        display_error();
        return false;
      }
    }
    else if(argument=="-scale_x"){
      c--;
      if(c>0){
        scale_x= atoi(*(++v));
      }else{
        display_error();
        return false;
      }
    }
    else if(argument=="-scale_y"){
      c--;
      if(c>0){
        scale_y= atoi(*(++v));
      }else{
        display_error();
        return false;
      }
    }
    else if(argument=="-force16"){
      force_16= true;
    }
    else if(argument=="-scene"){
      c--;
      if(c>0){
        scene_count_limit= atoi(*(++v));
      }else{
        display_error();
        return false;
      }
    }
    else if(argument=="-time_segment"){
      c--;
      if(c>1){
        time_segment= true;
        time_in= atof(*(++v));
        c--;
        time_out= atof(*(++v));
      }
      else{
        if(debug) cout << "c=" << c << endl;
        cerr << "OVSurGen error: time segment is not set correctly" << endl;
        display_error();
        return false;
      }
    }

    // add "else if" here...
    
    else{
      cerr << "OVSurGen error: parsing argument list failed" << endl;
      display_error();
      return false;
    }
  }
  

  /* requirement checking */

  if(filename_in==NULL){
    cerr << "OVSurGen error: input file name needs to be specified." << endl;
    display_error();
    return false;
  }

  if(filename_out==NULL){
    cerr << "OVSurGen error: output file name needs to be specified." << endl;
    display_error();
    return false;
  }


  /* parameter parsing */

  // input file
  filepathIn_stem= filename_in;
  string::size_type ifIdx= filepathIn_stem.find_last_of(".");
  if ((ifIdx==string::npos)||(ifIdx==0)){
    cerr << "OVSurGen error: filename does not appear to be a valid one" << endl;
    return false;
  }
  else{
    filepathIn_stem.erase(ifIdx);
    if(debug){
    	cout << "file path+name stem:" << filepathIn_stem << endl;
    }
    string::size_type ifPath_idx= filepathIn_stem.find_last_of("/");
    if(ifPath_idx==string::npos){
      ifPath_idx= 0;
    }
    else{
      ifPath_idx+=1;
    }

    if(debug) cerr << "ifPath_idx:" << ifPath_idx << endl;
    filenameIn_stem= filepathIn_stem.substr(ifPath_idx,
        filepathIn_stem.length()-ifPath_idx);
    if(debug) cerr << "filenameIn_stem:" << filenameIn_stem << "," << endl;
    if(0!=ifPath_idx){
      filepathIn_stem.erase(filepathIn_stem.find_last_of("/")+1);
    }
    else{
      filepathIn_stem="";
    }
    if(debug) cerr << "filepathIn_stem:" << filepathIn_stem <<  "," << endl;
  }


  // output file

  filepathOut_stem= filename_out;
  if(debug){
    cout << "filepathOut_stem original:" << filepathOut_stem << endl;
  }
  string::size_type ofIdx= filepathOut_stem.find_last_of(".");
  if ((ofIdx==string::npos)||(ofIdx==0)){
    cerr << "Output filename does not appear to be a valid one" << endl;
    return false;
  }
  else{

    // file extension
    fileextOut_stem=filepathOut_stem.substr(ofIdx+1,
        filepathOut_stem.length()-ofIdx+1);
    if(debug){
      cout << "extension:" << fileextOut_stem << endl;
    }

    // path + name w/o file extension
    filepathOut_stem.erase(ofIdx);
    if (debug)
      cout << "file path+name stem:" << filepathOut_stem << endl;
    string::size_type ofPath_idx= filepathOut_stem.find_last_of("/");
    if(ofPath_idx==string::npos){
      ofPath_idx= 0;
    }
    else{
      ofPath_idx+=1;
    }
    if(debug) cerr << "ofPath_idx:" << ofPath_idx << endl;
    filenameOut_stem= filepathOut_stem.substr(ofPath_idx,
        filepathOut_stem.length()-ofPath_idx);
    if(debug) cerr << "filenameOut_stem:" << filenameOut_stem << "," << endl;
    if(0!=ofPath_idx){
      filepathOut_stem.erase(filepathOut_stem.find_last_of("/")+1);
    }
    else{
      filepathOut_stem="";
    }
    if(debug) cerr << "filepathOut_stem:" << filepathOut_stem <<  "," << endl;
  }


  // set task type and parameter from output file extension
  if((fileextOut_stem=="jpeg")||(fileextOut_stem=="jpg")){
    kf_jpeg= true;
  }
  else if(fileextOut_stem=="png"){
    kf_png= true;
  }
  else if(fileextOut_stem=="gif"){
  	kf_gif= true;
  }
  else if(fileextOut_stem=="agif"){
  	kf_agif= true;
  }
  else{
  	mv_sf= true;
  	if(!frame_skip)
  		frame_skip_interval = 64;
  }

  // create output filename
  string ofType_abbr;
  if(mv_sf){
    if(frame_skip_interval==1){
      ofType_abbr="ex";
    }
    else{
      ofType_abbr="ff";
    }
    sprintf(ofname,
        "%s%s-%s%d.%s",
        filepathOut_stem.c_str(),
        filenameOut_stem.c_str(),
        ofType_abbr.c_str(),
        frame_skip_interval,
        fileextOut_stem.c_str());

    if(debug){
    	cout << "OFNAME:" << ofname << endl;
    }
  }
  else if((fileextOut_stem!="jpg")
  		  &&(fileextOut_stem!="jpeg")
  		  &&(fileextOut_stem!="png")
  		  &&(fileextOut_stem!="gif")
  		  &&(fileextOut_stem!="agif")){
    cerr << "OVSurGen error: image file type not recognized"<< endl;
    display_error();
    return false;
  }

  /* user output */
  if(verbose){
  	display_summary();
  }

  // normal exit without error
  return true;
}

void mediaConfig::set_target_size(int src_x, int src_y){
  
  // set target size from default and user input
  // set original size equals to new size as default
  if((scale_ratio==0)&&(scale_x==0)&&(scale_y==0)){
    //scale_x= default_target_x;
    //target_x= default_target_x;//320
    //target_y= default_target_y;//240
  	scale_x= src_x;
  	scale_y= src_y;
  }
  // set new size as ratio if ratio is specified
  if((scale_ratio)!=0){
    target_x= (int)(scale_ratio*src_x);
    target_y= (int)(scale_ratio*src_y);
  }
  // set new size explicitly
  if((scale_x>0)&&(scale_y)>0){
    target_x= scale_x;
    target_y= scale_y;
  }
  // set y dimension according to x dimension
  if((scale_x>0)&&(scale_y==0)){
    target_x= scale_x;
    target_y= (int)((float)(src_y)
		    *((float)(scale_x)/(float)(src_x)));
  }
  // set x dimension according to y dimension
  if((scale_x==0)&&(scale_y>0)){
    target_x= (int)((float)(src_x)
		    *((float)(scale_y)/(float)(src_y)));
    target_y= scale_y;
  }

  // force sizings to multiple of 16
  if (force_16) {
    float scale_mod_x = target_x % 16;
    if(debug) cerr << "scale_mod_x: " << scale_mod_x << endl;
    if (scale_mod_x < 5) {
    	if(debug) cerr << "target_x: " << target_x << " -> ";
      target_x = target_x / 16 * 16;
      if(debug) cerr << target_x << endl;
    }
    else {
    	if(debug) cerr << "target_x: " << target_x << " -> ";
      target_x = (target_x / 16 + 1) * 16;
      if(debug) cerr << target_x << endl;
    }

    float scale_mod_y = target_y % 16;
    if(debug) cerr << "scale_mod_y: " << scale_mod_y << endl;
    if (scale_mod_y < 5) {
    	if(debug) cerr << "target_y: " << target_y << " -> ";
      target_y = target_y / 16 * 16;
      if(debug) cerr << target_y << endl;
    }
    else {
    	if(debug) cerr << "target_y: " << target_y << " -> ";
      target_y = (target_y / 16 + 1) * 16;
      if(debug) cerr << target_y << endl;
    }

    if (debug) {
    	cerr << "debug: target dimension= " << target_x << "x" << target_y
          << endl;
    }
  }
}
