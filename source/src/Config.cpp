#include "Config.h"
#include <iostream>

Config::Config()
{
	options["morphingmatrix"] = "data/matrix_tor_sc_to_skype_video.mtx";
	options["socksport"] = "22222";
	options["udpport"] = "11111";
	options["morphingmethod"] = "1";
	options["skypesizecdf"] = "data/size_CDF";
	options["skypetimecdf"] = "data/time_CDF";
	options["torsizecdf"] = "data/tor_sc_CDF";
	options["ipadr"] = "127.0.0.1";
	//options["skypeport"] = "33332";
	options["serverid"] = "";

	options["skypeid"] = "";
	options["skypepwd"] = "";
	options["skrtip"] = "127.0.0.1";
	options["skrtport"] = "20000";
	options["skrtpath"] = "";
	options["keyfilename"] = "tkclient.pem";
	options["torport"] = "9006";
	options["toraddr"] = "127.0.0.1";
	options["skypekitadr"] = "";
	options["routerip"] = "";

}


//clears leading and trailing spaces
static void *trimwhitespace(char **str)
{
	char* val;
	val = *str;
	char *end;

	// Trim leading space
	while(isspace(*val)) val++;

	if(*val == 0)  // All spaces?
		return val;

	// Trim trailing space
	end = val + strlen(val) - 1;
	while(end > val && isspace(*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	strcpy(*str,val);
}

void toLowerCase(char** str)
{
	size_t s = strlen(*str);
	for(int i = 0; i < s ; i++)
		*(*(str)+i) = tolower(*(*(str)+i));

}

//Returns 1 on success, 0 otherwise
static int getline(FILE* f, char** str)
{
	char* line = new char[1024];
	if(fgets(line,1024,f) != NULL )
	{
		trimwhitespace(&line);
		//toLowerCase(&line);
		strcpy(*str,line);
		return 1;
	}
	else
		return 0;
}

//Returns 1 on success, 0 otherwise
static int parse_value(const char* line, char** key, char** value)
{
	char *k,*v;
	k = v = NULL;
	if(*line=='#')
		return 0;
	int i = 0;
	size_t s = strlen(line);
	while(i<s)
	{
		if(isspace(line[i]))
		{
			k = new char[i+1];
			strncpy(k,line,i);
			k[i] = '\0';
			toLowerCase(&k);
			*key = k;
			break;
		}
		i++;
	}
	while(i<s)
	{
		if(!isspace(line[i]))
			break;
		i++;
	}
	int j = i;
	i = 0;
	while(i+j<s)
	{
		if(isspace(line[i+j]) || i+j+1==s)
		{
			v = new char[i+2];
			strncpy(v, line+j,i+1);
			v[i+1] = '\0';
			*value = v;
			return 1;
		}
		i++;
	}
	return 0;
}

int Config::load_cfg_file(const char *file_adr)
{
	char *line = new char[BUFSIZ];
	char *k,*v;
	FILE* f;
	f = fopen(file_adr, "r");

	if(f!=NULL)
	{

		map<const char*,const char*>::iterator it;
		while(getline(f,&line))
		{
			k = v = NULL;
			if(parse_value(line,&k,&v))
			{

				it = options.find(k);
				if(it!=options.end())
				{
					options.erase (it);
					options.insert(pair<const char*,const char*>(k,v));
					//printf("%s:%s\n", k,v);
					//fflush(stdout);
				}
				else
				{
					printf("Property %s does not exist.\n",k);
				}
			}
		}
		return 1;
	}
	else
	{
		return 0;
	}

}
