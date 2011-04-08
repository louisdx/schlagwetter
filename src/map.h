#ifndef H_MAP
#define H_MAP


#include <array>
#include <vector>
#include <memory>
#include <boost/noncopyable.hpp>
#include "types.h"

inline std::vector<ChunkCoords> ambientChunks(const ChunkCoords & cc, size_t radius)
{
  std::vector<ChunkCoords> res;
  res.reserve(4 * radius * radius);

  const int32_t r(radius);

  for (int32_t i = cX(cc) - r; i <= cX(cc) + r; ++i)
  {
    for (int32_t j = cX(cc) - r; j <= cX(cc) + r; ++j)
    {
      res.push_back(ChunkCoords(i, j));
    }
  }

  return res;
}


/* A complete chunk, 16 x 128 x 16.
 * It consists of 4 consecutive arrays of element sizes,
 * respectively, 1, 1/2, 1/2 and 1/2 byte.
 */
 
class Chunk : private boost::noncopyable
{
public:
  Chunk() : m_data() { }
  Chunk(Chunk && other) : m_data(std::move(other.m_data)) { }

  /// This is how the client expects the 3D data to be arranged.
  /// (Layers of (y,z)-slices indexed by x, consisting of y-columns indexed by z.)
  inline size_t index(size_t x, size_t y, size_t z) const { return y + (z * 128) + (x * 128 * 16); }

  /// Meta and light data is only 4 bytes, two consecutive fields share one byte.
  inline unsigned char getHalf(size_t y, unsigned char data) const
  {
    return (y % 2 == 0) ? data & 0x0F : data >> 4;
  }
  inline void setHalf(size_t y, unsigned char value, unsigned char & data)
  {
    if (y % 2 == 0)
    {
      // Set the lower 4 bit
      data |= 0xF0;
      data |= (value & 0x0F);
    }
    else
    {
      // Set the upper 4 bit
      data |= 0x0F;
      data |= (value << 4);
    }
  }

  inline size_t size() const { return m_data.size(); }

  enum { offsetBlockType = 0, offsetBlockMetaData = 32768, offsetBlockLight = 49152, offsetSkyLight = 65536 };

  inline       unsigned char & blockType    (size_t x, size_t y, size_t z)       { return m_data[offsetBlockType     + index(x, y, z)]; }
  inline const unsigned char & blockType    (size_t x, size_t y, size_t z) const { return m_data[offsetBlockType     + index(x, y, z)]; }

  inline void setBlockMetaData(size_t x, size_t y, size_t z, unsigned char val) { setHalf(y, val, m_data[offsetBlockMetaData + index(x, y, z) / 2]); }
  inline unsigned char getBlockMetaData(size_t x, size_t y, size_t z) const { return getHalf(y, m_data[offsetBlockMetaData + index(x, y, z) / 2]); }

  inline void setBlockLight(size_t x, size_t y, size_t z, unsigned char val) { setHalf(y, val, m_data[offsetBlockLight + index(x, y, z) / 2]); }
  inline unsigned char getBlockLight(size_t x, size_t y, size_t z) const { return getHalf(y, m_data[offsetBlockLight + index(x, y, z) / 2]); }

  inline void setSkyLight(size_t x, size_t y, size_t z, unsigned char val) { setHalf(y, val, m_data[offsetSkyLight + index(x, y, z) / 2]); }
  inline unsigned char getSkyLight(size_t x, size_t y, size_t z) const { return getHalf(y, m_data[offsetSkyLight + index(x, y, z) / 2]); }

  /// The client expects chunks to be deflate()ed. ZLIB to the rescue.
  std::string compress() const;

private:
  /// Every chunk is exactly 80KiB in size.
  std::array<unsigned char, 81920> m_data;
};


typedef std::unordered_map<ChunkCoords, std::shared_ptr<Chunk>, PairHash<int32_t, int32_t>> ChunkMap;

class Map
{
public:
  Map();

  inline bool haveChunk(const ChunkCoords & cc) { return m_chunkMap.count(cc) > 0; }

  inline       Chunk & getChunk(const ChunkCoords & cc)       { return *(m_chunkMap.find(cc)->second); }
  inline const Chunk & getChunk(const ChunkCoords & cc) const { return *(m_chunkMap.find(cc)->second); }

  /* If no chunk exists at cc, create a random one. Always returns the chunk at cc. */
  const Chunk & getChunkOrGnerateNew(const ChunkCoords & cc);

private:
  ChunkMap m_chunkMap;
};


std::string makeRandomChunkPacket(int32_t X, int32_t Z);


#endif
