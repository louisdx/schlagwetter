#ifndef H_TYPES
#define H_TYPES

#include <tuple>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>
#include <iomanip>
#include <iostream>



/// A millisecond clock tick.

long long int clockTick();


/// Good old C standard, never fixed the behaviour of signed division...

inline unsigned int MyMod(int a, unsigned int b) { int c = a % b; while (c < 0) c += b; return (unsigned int)(c); }
inline unsigned int MyMod16(int a) { return MyMod(a, 16); }
inline unsigned int MyMod32(int a) { return MyMod(a, 32); }
inline   signed int MyDiv(int num, int den)
{
  if ((num < 0 && den < 0) || (num >= 0 && den >= 0)) return num / den;

  if (num < 0) return -((-num) / den + 1);

  return MyDiv(-num, -den);
}
inline   signed int MyDiv16(int num) { return MyDiv(num, 16); }
inline   signed int MyDiv32(int num) { return MyDiv(num, 32); }



/**

   World coordinates are (int32_t wX, int8_t wY, int32_t wZ),
   with 0 <= wY <= 127 (but ingame positions may exceed either bound).

   The world is divided into (16 x 128 x 16)-"chunks".

   The chunk grid is indexed by (int32_t cX, int32_t cZ), where:

       cX = wX / 16 = wX >> 4
       cZ = wZ / 16 = wZ >> 4

    Local coordinates on each chunk are (X, Y, Z). Thus:

       wX = 16 * cX + X
       wY =           Y
       wZ = 16 * cZ + Z


**/

typedef std::tuple<int32_t, int32_t, int32_t> WorldCoords;
typedef std::tuple<size_t, size_t, size_t>    LocalCoords;
typedef std::pair<int32_t, int32_t>           ChunkCoords; // Coordinates _of_ the chunk, not "within" the chunk.
typedef std::tuple<int64_t, int64_t, int64_t> FractionalCoords; // 32 units per block, fixed-point with 5 fractional bits.

inline int32_t wX(const WorldCoords & wc) { return std::get<0>(wc); }
inline int32_t & wX(WorldCoords & wc) { return std::get<0>(wc); }
inline int32_t wY(const WorldCoords & wc) { return std::get<1>(wc); }
inline int32_t & wY(WorldCoords & wc) { return std::get<1>(wc); }
inline int32_t wZ(const WorldCoords & wc) { return std::get<2>(wc); }
inline int32_t & wZ(WorldCoords & wc) { return std::get<2>(wc); }

inline size_t lX(const LocalCoords & lc) { return std::get<0>(lc); }
inline size_t & lX(LocalCoords & lc) { return std::get<0>(lc); }
inline size_t lY(const LocalCoords & lc) { return std::get<1>(lc); }
inline size_t & lY(LocalCoords & lc) { return std::get<1>(lc); }
inline size_t lZ(const LocalCoords & lc) { return std::get<2>(lc); }
inline size_t & lZ(LocalCoords & lc) { return std::get<2>(lc); }

inline int32_t cX(const ChunkCoords & cc) { return cc.first; }
inline int32_t & cX(ChunkCoords & cc) { return cc.first; }
inline int32_t cZ(const ChunkCoords & cc) { return cc.second; }
inline int32_t & cZ(ChunkCoords & cc) { return cc.second; }

inline int64_t fX(const FractionalCoords & fc) { return std::get<0>(fc); }
inline int64_t & fX(FractionalCoords & fc) { return std::get<0>(fc); }
inline int64_t fY(const FractionalCoords & fc) { return std::get<1>(fc); }
inline int64_t & fY(FractionalCoords & fc) { return std::get<1>(fc); }
inline int64_t fZ(const FractionalCoords & fc) { return std::get<2>(fc); }
inline int64_t & fZ(FractionalCoords & fc) { return std::get<2>(fc); }

inline std::ostream & operator<<(std::ostream & o, const WorldCoords & wc)
{
  return o << std::dec << "w[" << wX(wc) << ", " << wY(wc) << ", " << wZ(wc) << "]";
}
inline std::ostream & operator<<(std::ostream & o, const LocalCoords & lc)
{
  return o << std::dec << "l[" << lX(lc) << ", " << lY(lc) << ", " << lZ(lc) << "]";
}
inline std::ostream & operator<<(std::ostream & o, const ChunkCoords & cc)
{
  return o << std::dec << "c[" << cX(cc) << ", " << cZ(cc) << "]";
}
inline std::ostream & operator<<(std::ostream & o, const FractionalCoords & fc)
{
  return o << std::dec << "c[" << MyDiv32(fX(fc)) << " " << MyMod32(fX(fc)) << "/32, "
           << MyDiv32(fY(fc)) << " " << MyMod32(fY(fc)) << "/32, "
           << MyDiv32(fZ(fc)) << " " << MyMod32(fZ(fc)) << "/32]";
}

/* Transform between the various coordinate types. We COULD actually
   implement most of those as cast operators. */

inline LocalCoords getLocalCoords(const WorldCoords & wc)
{
  return LocalCoords(MyMod16(wX(wc)), wY(wc), MyMod16(wZ(wc)));
}
inline ChunkCoords getChunkCoords(const WorldCoords & wc)
{
  return ChunkCoords(MyDiv16(wX(wc)), MyDiv16(wZ(wc)));
}
inline WorldCoords getWorldCoords(const LocalCoords & lc, const ChunkCoords & cc)
{
  return WorldCoords(cX(cc) * 16 + int32_t(lX(lc)), int32_t(lY(lc)), cZ(cc) * 16 + int32_t(lZ(lc)));
}
inline WorldCoords getWorldCoords(const FractionalCoords & fc)
{
  return WorldCoords(MyDiv32(fX(fc)), MyDiv32(fY(fc)), MyDiv32(fZ(fc)));
}
inline LocalCoords getLocalCoords(const FractionalCoords & fc)
{
  return getLocalCoords(getWorldCoords(fc));
}
inline ChunkCoords getChunkCoords(const FractionalCoords & fc)
{
  return getChunkCoords(getWorldCoords(fc));
}

/* Direction. We allow adding those to WorldCoords. */

enum Direction
{
  BLOCK_YMINUS = 0,
  BLOCK_YPLUS  = 1,
  BLOCK_ZMINUS = 2,
  BLOCK_ZPLUS  = 3,
  BLOCK_XMINUS = 4,
  BLOCK_XPLUS  = 5
};

inline WorldCoords & operator+=(WorldCoords & wc, Direction dir)
{
  switch (dir)
  {
  case BLOCK_XMINUS: { --wX(wc); break; }
  case BLOCK_XPLUS:  { ++wX(wc); break; }
  case BLOCK_YMINUS: { --wY(wc); break; }
  case BLOCK_YPLUS:  { ++wY(wc); break; }
  case BLOCK_ZMINUS: { --wZ(wc); break; }
  case BLOCK_ZPLUS:  { ++wZ(wc); break; }
  default: { std::cout << "Invalid direction received!" << std::endl; }
  }
  return wc;
}

inline const WorldCoords operator+(const WorldCoords & wc, Direction dir)
{
  WorldCoords wcnew(wc);
  return wcnew += dir;
}


/*  A little helper function that creates a collection
 *  of chunks in a (2*radius+1)^2 square around a given chunk.
 */
inline std::vector<ChunkCoords> ambientChunks(const ChunkCoords & cc, size_t radius)
{
  std::vector<ChunkCoords> res;
  res.reserve(4 * radius * radius);

  const int32_t r(radius);

  for (int32_t i = cX(cc) - r; i <= cX(cc) + r; ++i)
  {
    for (int32_t j = cZ(cc) - r; j <= cZ(cc) + r; ++j)
    {
      res.push_back(ChunkCoords(i, j));
    }
  }

  return res;
}


/*  STL does not yet come with a hash_combine(), so I'm lifting this
 *  implementation from boost. It creates a hash function for every
 *  pair of hashable types. For further generalizations, see boost/functional/hash.
 */

template <class T>
inline void hash_combine(std::size_t & seed, T const & v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template<typename S, typename T>
struct PairHash : public std::unary_function<std::pair<S, T>, size_t>
{
  inline size_t operator()(const std::pair<S, T> & v) const
  {
    std::size_t seed = 0;
    hash_combine(seed, v.first);
    hash_combine(seed, v.second);
    return seed;
  }
};

#endif
