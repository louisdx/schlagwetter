#ifndef H_TYPES
#define H_TYPES

#include <tuple>
#include <unordered_map>
#include <cstdint>
#include <iomanip>
#include <iostream>

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

inline std::ostream & operator<<(std::ostream & o, const WorldCoords & wc) { return o << std::dec << "w[" << wX(wc) << ", " << wY(wc) << ", " << wZ(wc) << "]"; }
inline std::ostream & operator<<(std::ostream & o, const LocalCoords & lc) { return o << std::dec << "l[" << lX(lc) << ", " << lY(lc) << ", " << lZ(lc) << "]"; }
inline std::ostream & operator<<(std::ostream & o, const ChunkCoords & cc) { return o << std::dec << "c[" << cX(cc) << ", " << cZ(cc) << "]"; }


/* Good old C standard, never fixed the behaviour of signed division... */

inline unsigned int MyMod(int a, unsigned int b) { int c = a % b; while (c < 0) c += b; return (unsigned int)(c); }
inline unsigned int MyMod16(int a) { return MyMod(a, 16); }
inline   signed int MyDiv(int num, int den)
{
  if ((num < 0 && den < 0) || (num >= 0 && den >= 0)) return num / den;

  if (num < 0) return -((-num) / den + 1);

  return MyDiv(-num, -den);
}
inline   signed int MyDiv16(int num) { return MyDiv(num, 16); }



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


/* STL does not yet come with a hash_combine(), so I'm lifting this
   implementation from boost. It creates a hash function for every
   pair of hashable types. For further generalizations, see boost/functional/hash.
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
