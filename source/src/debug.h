#ifndef _DEBUG_H_
#define _DEBUG_H_

/* 
 * Debugging utilities
 */

#include <cstring>
#include <cstdarg>
#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>

class Debug_Helper {
  static int n; 
 public: 
  Debug_Helper(const char * str) {
    std::string sp = ""; 
    for (int i = 0; i < n; i++) sp = sp + " "; 
    std::cerr << sp << str << std::endl;
    n++; 
  }
  ~Debug_Helper() {
    n--;
  }
};

#define DEBUG_FUNC \
  Debug_Helper(__PRETTY_FUNCTION__)


bool log_init(const char * filename); 

void log(char * cat, char * format, ...); 


#endif
