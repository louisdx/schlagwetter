#ifndef H_RANDOM_NUMBER_GENERATOR
#define H_RANDOM_NUMBER_GENERATOR

#include <random>

typedef std::mt19937 MyRNG;

extern MyRNG prng;
extern uint32_t seedval;

void initPRNG();

extern std::uniform_int<uint32_t> m_uniformUINT32;

inline uint32_t uniformUINT32() { return m_uniformUINT32(prng); }

#endif
