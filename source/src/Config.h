#ifndef CONFIG_H
#define CONFIG_H
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
using namespace std;

class Config
{
	struct classcomp {
		bool operator() (const char* lhs, const char* rhs) const
		{return strcmp(lhs,rhs)<0;}
	};
public:
	map<const char*,const char*,classcomp> options;
	void parse_config_line_from_str(char**, char**, char**);
	int load_cfg_file(const char *file_adr);
	Config();
};
#endif
