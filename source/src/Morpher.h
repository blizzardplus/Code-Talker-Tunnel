#ifndef MORPHER_H
#define MORPHER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <vector>
#include <deque>
#include <utility>

extern "C" {

#include "dream/dream.h"

}

#include "Packet.h"
#define MAX_PKT_SIZE        MAX_RAW_MSG_LEN
#define MAX_INTER_PKT_DELAY 1000  // this is in millisecond
#define SIZE_STEP           50
#define TIME_STEP           20

enum mm_type {
  MP_NONE  = 0,                 // no morphing; fixed size and timing
  MP_TM,                        // traffic morphing
  MP_NAIVE,                     // naive morphing
  NUM_MP_METHODS
}; 

class BinHash; 

class Bins {
private: 
  std::deque<size_t> x; 
public: 
  Bins(); 
  Bins(const Bins & rhs); 

  size_t push(size_t xn); 
  Bins & operator=(const Bins & rhs); 
  bool   operator==(const Bins & rhs) const; 

  friend class BinHash; 
}; 


struct BinHash {
  size_t operator()(const Bins & b) const; 
}; 

class Dist {

private: 
  std::vector<double> cdf; 
public: 
  void push(double x); 

  size_t draw(double r) const; 
}; 

#if defined(HAVE_CXXTR1)
#include <tr1/unordered_map>
typedef std::tr1::unordered_map<Bins, const Dist *, BinHash> DistMap; 

#elif defined(HAVE_CXX11)
#include <unordered_map>
typedef std::unordered_map<Bins, const Dist *, BinHash> DistMap; 

#else
#error unordered_map required
#endif

class Morpher {
public:

  Morpher(int t) : type(t), order(1) { }  
  Morpher(int t, int o) : type(t), order(o) { }


  void                      init(const char*, const char*, 
                                 const char*, const char*);
  std::pair<size_t, size_t> get_next_time_len(size_t);

private:
  double *time_dis, *size_dis, *tor_size_dis; 
  csc_t* morph_csc;
  int type; 

  int order; 
  Bins    lastbin; 
  DistMap distmap; 

  void morph_init(const char * fn_matrix);

  size_t morph(size_t);

  std::pair<size_t, size_t> hquery(); 
};

#endif

