#include "Morpher.h"
#include <cyassl/ctaocrypt/aes.h>
#include <boost/random/uniform_01.hpp>
#include <unistd.h>

// We need to replace this with boost PRNG
static double rand_double()
{
	//srand(time(NULL));
	double out;
	out = (double)rand()/RAND_MAX; //each iteration produces a number in [0, 1)
	out = (rand() + out)/RAND_MAX;
	out = (rand() + out)/RAND_MAX;
	out = (rand() + out)/RAND_MAX;
	out = (rand() + out)/RAND_MAX;
	out = (rand() + out)/RAND_MAX;

	return out;
}

// Given a distribution and a random r in [0,1], finds the inverse CDF
static size_t inverse_CDF(double* dist, size_t size_of_array, double rand) {
	size_t i = 1;
	for(; i<size_of_array; i++) {
		if(rand < *(dist+i)) break;
	}
	return i;
}

Bins::Bins() {
	x[0] = 0;                     // default value for the initial qureies
}

Bins::Bins(const Bins & rhs) {
	*this = rhs;
}

size_t Bins::push(size_t xn) {
	size_t x0 = x.front();
	x.pop_front();
	x.push_back(xn);
	return x0;
}

Bins & Bins::operator=(const Bins & rhs) {
	for (std::deque<size_t>::const_iterator i = rhs.x.begin();
			i != rhs.x.end(); ++i) {

		x.push_back(*i);
	}
	return *this;
}


bool Bins::operator==(const Bins & rhs) const {
	if (x.size() != rhs.x.size()) return false;

	std::deque<size_t>::const_iterator i = x.begin(), j = rhs.x.begin();

	while (i != x.end()) {
		if (*i != *j)
			return false;

		++i;
		++j;
	}
	return true;
}


size_t BinHash::operator()(const Bins & b) const {
	size_t ret = 0;

#ifdef HAVE_CXXTR1
	std::tr1::hash<size_t> hasher;
#else 
	std::hash<size_t>      hasher;
#endif

	for (std::deque<size_t>::const_iterator i = b.x.begin(); i != b.x.end(); ++i) {
		ret ^= hasher(*i);
	}
	return ret;
}


void Dist::push(double x) {
	cdf.push_back(x);
}

size_t Dist::draw(double r) const {
	size_t i = 1;
	for(; i < cdf.size(); i++) {
		if (r < cdf[i]) break;
	}
	return i;
}


//first is time, second is size.
//if you are using naive method just input 0 as pkt_size
std::pair<size_t,size_t> Morpher::get_next_time_len(size_t pkt_size) {
	std::pair<size_t,size_t> res;

	if (type == MP_TM) {
		double r = rand_double();
		res.first = inverse_CDF(time_dis, MAX_INTER_PKT_DELAY, r);

		r = rand_double();
		size_t len_real  = inverse_CDF(tor_size_dis, MAX_PKT_SIZE, r) + 1;

		r = rand_double();
		size_t len_morph = dream_get_target_length(morph_csc,len_real, r);


		res.second = std::min((size_t)MAX_PKT_SIZE,len_morph);
	}
	else if (type == MP_NAIVE) {
		if (order <= 1) {
			double r = rand_double();
			res.first = inverse_CDF(time_dis, MAX_INTER_PKT_DELAY, r);

			r = rand_double();
			res.second = inverse_CDF(size_dis, MAX_PKT_SIZE, r);
		}
		else {
			res = hquery();
		}
	}
	else {
		res.first = 100;          // FIXME: define a const
		res.second = 512;
	}
	return res;
}

std::pair<size_t, size_t> Morpher::hquery() {
	const Dist * dist = distmap[lastbin];
	double r = rand_double();
	size_t sz = dist->draw(r);

	lastbin.push(sz / SIZE_STEP);
	dist = distmap[lastbin];
	r = rand_double();

	size_t tm = dist->draw(r);

	return std::make_pair<size_t, size_t>(sz, tm);
}

static double* read_dist_from_file(const char* addr)
{
	FILE *f = fopen(addr, "r");
	if (f!=NULL)
	{
		char buf[1024];
		int length;
		if (!fgets(buf, 1024, f))
			return NULL;
		sscanf(buf, "%d", &length);

		double *value = new double[length];
		int i = 0;
		do
		{
			if (!fgets(buf, 1024, f)) break;
			sscanf(buf, "%lf", &value[i]);
			i++;
		}
		while (i < length);
		fclose(f);

		return value;
	}
	else
	{
		printf("Cannot find file %s specified in the config file\n", addr);
		exit(-1);
	}

}



// time and size distribution should be given as CDFs.
void Morpher::init(const char* time_dist_adr, 
		const char* size_dist_adr,
		const char* tor_size_dist_adr,
		const char* matrix_adr)
{
	time_dis = read_dist_from_file(time_dist_adr);
	size_dis = read_dist_from_file(size_dist_adr);
	tor_size_dis = read_dist_from_file(tor_size_dist_adr);
	srand(time(NULL));
	if (type == MP_TM) {
		morph_init(matrix_adr);
	}
}




void Morpher::morph_init(const char * fn_matrix) {
	if (!fn_matrix) {
		printf("please specify the matrix file\n");
		exit(-1);
	}

	FILE *f = fopen(fn_matrix, "r");

	enum mm_ret ret = dream_set_csc_from_mm(&morph_csc, f);
	switch (ret) {
	case DREAM_MM_OKAY:
		break;
	case DREAM_MM_CORRUPTED:
	case DREAM_MM_NOT_MORPHING:
	case DREAM_MM_INTERNAL:
	default:
		printf("invalid matrix\n");
		exit(-1);
	}
}

size_t  Morpher::morph( size_t len_real) {
	if (type == MP_TM) {
		double r = rand_double();
		size_t len_morph = (dream_get_target_length(morph_csc,len_real, r));

		return std::min((size_t)MAX_PKT_SIZE, len_morph);
	}
	else if (type == MP_NAIVE) {
		return inverse_CDF(size_dis, MAX_PKT_SIZE, rand_double());
	}
	else {
		return 512;                 // FIXME: define a const
	}
}

#ifdef UNITTEST 

#include <cstdio>

int main() {
	Morpher m(MP_NAIVE);

	m.init("/scratch/tor/skypemorph/data/skype_video_oneside/time_CDF",
			"/scratch/tor/skypemorph/data/skype_video_oneside/size_CDF",
			"/scratch/tor/skypemorph/data/matrix_tor_to_skype.mtx");

	for (int i = 0; i < 100; i++) {
		std::std::pair<size_t, size_t> query = m.get_next_time_len(512);
		printf("%d\t%d\n", query.first, query.second);
	}

	return 0;
}

#endif


