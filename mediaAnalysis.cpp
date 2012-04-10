#include "mediaAnalysis.h"

using namespace std;

mediaAnalysis::mediaAnalysis(bool debug_flag,int dim_x, int dim_y, int fCount):
  debug(debug_flag), x(dim_x), y(dim_y), frame_count(fCount){
  
  if(debug){
    cout << "OVSurGen debug: x:" << x << " y:" << y << " frame_count:" << frame_count << endl;
  }
  frame_list= new vector<unsigned int>;
  if(debug) cout << "OVSurGen debug: max size is " << frame_list->max_size() << endl;
  if(debug){
    cout << "OVSurGen debug: mediaAnalysis constructor: frame_list count is "
	 << frame_list->size() << " for " << frame_count << " frames" << endl;
  }
}


mediaAnalysis::~mediaAnalysis(){
  
  if(debug){
    cout << "OVSurGen debug: mediaAnalysis destructor: frame list count is "
	 << frame_list->size() << " for " << frame_count << " frames" << endl;
    for(int i=0; i<frame_count; i++){
      if(debug) cout << "OVSurGen debug: hist_:" << i << "= " << frame_list->at(i) << endl;
    }
  }
  while((frame_list->size())>0){
    if(debug){
    	cout << "OVSurGen debug: frame_list->size()=" << frame_list->size() << " >> ";
    }

    frame_list->pop_back();

    if(debug){
    	cout << "OVSurGen debug: " << frame_list->size() << endl;
    }
  }
  //frame_list->clear();

  if(frame_list!=NULL) delete frame_list;
  if(temp_list!=NULL) delete temp_list;

  if(debug){ cout << "OVSurGen debug: leaving mediaAnalysis destructor" << endl;}
}


// testing only; not used in actual code
void mediaAnalysis::test_mem(){

  for(int idx=0; idx<frame_count; idx++){
    frame_list->push_back(idx+1000);
  }
}


void mediaAnalysis::create_deltagram(const vector<mediaFrame*>* pVMediaFrame_src,
																		 const unsigned long long int threshold_d,
																		 const unsigned long long int threshold_b){

	if(debug){
		cout << "OVSurGen debug: threshold_d " << threshold_d << endl
				 << "OVSurGen debug: threshold_b " << threshold_b << endl
				 << endl;
	}
  // create overflow prevention logic later

  if(debug)
    cout << "OVSurGen debug: create_deltagram frame_count:" << frame_count << endl;
  
  for(int idx=0; idx<frame_count; idx++){
    
    unsigned char *pChar_new=
      (unsigned char*)((((*pVMediaFrame_src)[idx])->get_pFrameRGB())->data[0]);
    //unsigned char tmp_frame[x*y*3];
    
    static unsigned int prev_hist[256*3];
    static unsigned int curr_hist[256*3];
    
    // copy current frame to tmp_frame
    //memcpy(tmp_frame, pChar_new, x*y*3);
    
    // initialize current histogram
    for(int i=0; i<256*3; i++){
      curr_hist[i]=0;
    }
    
    // capture current frame's histogram
		for(int j = 0; j < x * y; j++){
			for(int i = 0; i < 3; i++){ // rgbrgbrgb...
				//curr_hist[(int) (tmp_frame[j + i * 256])]++; // rrr...ggg...bbb...
				//curr_hist[(int) (tmp_frame[j*3 + i])]++; // testing other sort major rgbrgbrgb...
				curr_hist[(int)(pChar_new[j * 3 + i])]++; // testing other sort major + testing not using tmp_frame
				//curr_hist[(int)(i*256+tmp_frame[j])]++; // what is this?  does not seem to be right.
			}
		}

		// testing for frames that are too dark or bright
		if(debug){
			cout << "OVSurGen debug: *********************** " << endl
					 << "OVSurGen debug: numeric_limits<unsigned int>::max() = " << numeric_limits<unsigned int>::max() << endl
					 << "OVSurGen debug: numeric_limits<unsigned long int>::max() = " << numeric_limits<unsigned long int>::max() << endl
					 << "OVSurGen debug: numeric_limits<unsigned long long int>::max() = " << numeric_limits<unsigned long long int>::max() << endl
					 << "OVSurGen debug: uint max = " << UINT_MAX << endl
					 << "OVSurGen debug: ULLONG_MAX = " << ULLONG_MAX << endl
					 << "OVSurGen debug: *********************** " << endl
					 << endl;
		}
    // adds up darkness count till, in scale of 255
		bool tooDark = false;
		bool tooBright = false;
		//unsigned int prev_testDarkness = 0; // used to check on overflow
		unsigned long long int testDarkness = 0; // actual darkness tracker
		for(int i = 0; i < 256; i++){
			for(int j = 0; j < 3; j++){
				//prev_testDarkness = testDarkness;
				testDarkness += curr_hist[i * 3 + j] * i;
				//if(prev_testDarkness > testDarkness){
				//testDarkness = numeric_limits<unsigned int>::max();
				//cerr << "OVSurGen error: testDarkness Overflow - setting the value to numeric limit:: max()" << endl;
				//}
			}
		}

		if(testDarkness < threshold_d){
			tooDark = true;
			if(debug){ cout << "OVSurGen debug: Darkness= " << testDarkness << " is too dark!" << endl;}
		}

		if(testDarkness > (x*y*3*255 - threshold_b)){
			tooBright = true;
			if(debug){ cout << "OVSurGen debug: Brightness= " << testDarkness << " is too bright!" << endl;}
		}

		if (debug){ cout << "OVSurGen debug: created histogram.  Now comparing histogram with previous" << endl;}

		// take histogram difference and set the result
		unsigned int hist_diff = 0;
		for (int i = 0; i < 256 * 3; i++) {
			if (curr_hist[i] > prev_hist[i])
				hist_diff += curr_hist[i] - prev_hist[i];
			else
				hist_diff += prev_hist[i] - curr_hist[i];
		}

		if (idx == 0) { // delta-gram is still empty.  This is your first item.
			if (tooDark || tooBright) {
				frame_list->push_back(0);
				if(debug){ cout << "OVSurGen debug: initial frame with inappropriate saturation level removed" << endl;}
			} else {
				frame_list->push_back(numeric_limits<unsigned long long int>::max());  // forcing to be selected
			}
		} else {
			if (tooDark || tooBright) {
				frame_list->push_back(0);
				if(debug){ cout << "OVSurGen debug: a frame with inappropriate saturation level found" << endl;}
			} else {
				frame_list->push_back(hist_diff);
			}
		}

    if(debug) cout << "OVSurGen debug: pushed:"
		   << idx << ":" << (*pVMediaFrame_src)[idx]->get_id() << "= "
		   << frame_list->size() << " " << hist_diff << endl;
    
    // move current hist to prev hist for next round
    for(int i=0; i<256*3; i++){
      prev_hist[i]=curr_hist[i];
    }
  }
}


unsigned int mediaAnalysis::get_critical_pt(int sc_limit){
  
  //debug=true;
  
  if(debug){
    cout << "OVSurGen debug: frame_count: " << frame_count << endl;
    for(int i=0; i<frame_count; i++){
      cout << "OVSurGen debug: deltagram-" << i << "= " << (*frame_list)[i] << endl;
    }
  }

  temp_list= new vector<unsigned int>(*frame_list);
  sort(temp_list->begin(), temp_list->end(), greater<unsigned int>());

  if(debug){
    for(int i=0; i<frame_count; i++){
      cout << "OVSurGen debug: deltagram sorted-" << i << "= " << (*temp_list)[i] << endl;
    }
    cout << "OVSurGen debug: frame_count: " << frame_count << endl;
  }

  if(sc_limit<frame_count){
    if(((*temp_list)[sc_limit-1])==((*temp_list)[sc_limit]))
      return (*temp_list)[sc_limit-1]+1;
    else
      return (*temp_list)[sc_limit-1];
  }
  else{
    return (*temp_list)[frame_count-1];
  }
}


unsigned int mediaAnalysis::get_d_element(int idx){

  return (*frame_list)[idx];
}

/*
bool mediaAnalysis::sc_detect(unsigned int critical, AVFrame *pFrame_src, int x, int y){
  
  // check on the overflow of int later...

  unsigned char *pChar_new = (unsigned char*)(pFrame_src->data[0]);
  unsigned char tmp_frame[x*y*3];

  static int prev_hist[256*3];
  static int curr_hist[256*3];

  // copy current frame to tmp_frame
  memcpy(tmp_frame, pChar_new, x*y*3);

  // initialize current histgram
  for(int i=0; i<256*3; i++){
    curr_hist[i]=0;
  }
  
  // capture current frame's histogram
  cout << "histogram work" << endl;
  //int hv;
  for(int j=0; j<x*y; j++){
    for(int i=0; i<3; i++)
      //curr_hist[(int)(i*256+tmp_frame[j])]++;
      curr_hist[(int)(tmp_frame[i*256+j])]++;
    //hv= (int)(tmp_frame[i]);
    //curr_hist[hv]=curr_hist[hv]+1;
    //cout << i << ": " << (int)(tmp_frame[i]) << "  " << hv << " " << curr_hist[hv] << "  ";
  }
  //cout << endl;
  
  // take histogram difference and set the result
  unsigned int hist_diff=0;
  for(int i=0; i<256*3; i++){
    hist_diff+=abs(curr_hist[i]-prev_hist[i]);
  }

  // move current hist to prev hist for next round
  for(int i=0; i<256*3; i++){
    prev_hist[i]=curr_hist[i];
  }

  if(hist_diff<critical)
    return false;
  else{
    cout << "diff: " << hist_diff << endl;
    return true;
  }
}
*/
