#ifndef INC_MEDIAANALYSIS_H
#define INC_MEDIAANALYSIS_H

#include <iostream>
#include <algorithm>
#include <vector>
#include <limits>

//#include "mediaConfig.h"
#include "mediaFrame.h"

using namespace std;

class mediaAnalysis{

 public:

  mediaAnalysis(bool, int, int, int);
  ~mediaAnalysis();
  void test_mem();
  void create_deltagram(const vector<mediaFrame*>*,
  		const unsigned long long int, const unsigned long long int);
  unsigned int get_critical_pt(int);
  unsigned int get_d_element(int);
  //bool sc_detect(unsigned int, AVFrame*, int, int);

 private:
  bool debug;

  int x, y, frame_count;
  vector<unsigned int>* frame_list;
  vector<unsigned int>* temp_list;

};

#endif // INC_MEDIAANALYSIS_H
