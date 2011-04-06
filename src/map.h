#ifndef H_MAP
#define H_MAP


#include <array>
#include <list>
#include "types.h"

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

inline std::list<ChunkCoords> ambientChunks(const ChunkCoords & cc, size_t radius)
{
  std::list<ChunkCoords> res;
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
 
class Chunk
{
public:
  Chunk() : m_data() { }

  inline size_t index(size_t x, size_t y, size_t z) const { return y + (z * 128) + (x * 128 * 16); }
  inline size_t size() const { return m_data.size(); }

  enum { offsetBlockType = 0, offsetBlockMetaData = 32768, offsetBlockLight = 49152, offsetSkyLight = 65536 };

  inline       unsigned char & blockType    (size_t x, size_t y, size_t z)       { return m_data[offsetBlockType     + index(x, y, z)]; }
  inline const unsigned char & blockType    (size_t x, size_t y, size_t z) const { return m_data[offsetBlockType     + index(x, y, z)]; }

  inline       unsigned char & blockMetaData(size_t x, size_t y, size_t z)       { return m_data[offsetBlockMetaData + index(x, y, z) / 2]; }
  inline const unsigned char & blockMetaData(size_t x, size_t y, size_t z) const { return m_data[offsetBlockMetaData + index(x, y, z) / 2]; }

  inline       unsigned char & blockLight   (size_t x, size_t y, size_t z)       { return m_data[offsetBlockLight    + index(x, y, z) / 2]; }
  inline const unsigned char & blockLight   (size_t x, size_t y, size_t z) const { return m_data[offsetBlockLight    + index(x, y, z) / 2]; }

  inline       unsigned char & skyLight     (size_t x, size_t y, size_t z)       { return m_data[offsetSkyLight      + index(x, y, z) / 2]; }
  inline const unsigned char & skyLight     (size_t x, size_t y, size_t z) const { return m_data[offsetSkyLight      + index(x, y, z) / 2]; }

  std::string compress() const;

private:
  std::array<unsigned char, 81920> m_data;
};


class Map
{
public:
  Map();

  inline bool haveChunk(const ChunkCoords & cc) { return m_chunkMap.count(cc) > 0; }

  inline       Chunk & getChunk(const ChunkCoords & cc)       { return m_chunkMap.find(cc)->second; }
  inline const Chunk & getChunk(const ChunkCoords & cc) const { return m_chunkMap.find(cc)->second; }

  /* If no chunk exists at cc, create a random one. Always returns the chunk at cc. */
  const Chunk & getChunkOrGnerateNew(const ChunkCoords & cc);

private:
  ChunkMap m_chunkMap;
};


std::string makeRandomChunkPacket(int32_t X, int32_t Z);


#endif
