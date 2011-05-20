#ifndef H_RANDOM_NUMBER_GENERATOR
#define H_RANDOM_NUMBER_GENERATOR

#include <random>


/** We have two fundamental random generators:
 *
 *  1. The map generator (using libnoise), seeded with a specified value
 *     (either on the command line or in a stored map file).
 *     It is imperative that we keep track of this seed value permanently.
 *
 *  2. An auxiliary random number generator (provided by the STL) for local
 *     random features, like spawn item position fluctuations or mob movement.
 *     This is seeded with 'seedval', a disposable number (we try to take it
 *     from /dev/urandom if available).
 *
 */


typedef std::mt19937 MyRNG;

extern MyRNG prng;
extern uint32_t seedval;

void initPRNG(int mapseed = -1);

extern std::uniform_int<uint32_t> m_uniformUINT32;

inline uint32_t uniformUINT32() { return m_uniformUINT32(prng); }

#endif
