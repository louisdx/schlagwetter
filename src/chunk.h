#ifndef H_CHUNK
#define H_CHUNK


#include <unordered_map>
#include <vector>
#include <array>
#include <memory>
#include <boost/noncopyable.hpp>
#include "types.h"

class Map;

/* A complete chunk, 16 x 128 x 16.
 * It consists of 4 consecutive arrays of element sizes,
 * respectively, 1, 1/2, 1/2 and 1/2 byte.
 */
 
class Chunk : private boost::noncopyable
{
public:

  typedef std::array<unsigned char, 81920> ChunkData;
  typedef std::array<unsigned char, 256>   ChunkHeightMap;

  Chunk(const ChunkCoords & cc) : m_coords(cc), m_data(), m_heightmap(), m_zcache(m_coords) { }
  Chunk(const ChunkCoords & cc, const ChunkData & data, const ChunkHeightMap & hm) : m_coords(cc), m_data(data), m_heightmap(hm), m_zcache(m_coords) { }
  Chunk(ChunkCoords && cc, ChunkData && data, ChunkHeightMap && hm) : m_coords(std::move(cc)), m_data(std::move(data)), m_heightmap(std::move(hm)), m_zcache(m_coords) { }

  Chunk(Chunk && other) : m_data(std::move(other.m_data)), m_heightmap(std::move(other.m_heightmap)), m_zcache(std::move(other.m_zcache)) { }

  /// For all sorts of purposes, we need to know if the chunk has been modified.
  /// We won't do it automatically at every access, please remember to taint your chunk.
  inline void taint()
  {
    m_zcache.usable = false;
  }

  /// This is how the client expects the 3D data to be arranged.
  /// (Layers of (y,z)-slices indexed by x, consisting of y-columns indexed by z.)
  inline size_t index(size_t x, size_t y, size_t z) const { return y + (z * 128) + (x * 128 * 16); }

  /// Meta and light data is only 4 bits, two consecutive fields share one byte.
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
  inline const ChunkCoords & coords() const { return m_coords; }

  enum { offsetBlockType = 0, offsetBlockMetaData = 32768, offsetBlockLight = 49152, offsetSkyLight = 65536,
         sizeBlockType = 32768, sizeBlockMetaData = 16384, sizeBlockLight = 16384, sizeSkyLight = 16384 };

  inline       unsigned char & height(size_t x, size_t z)       { return m_heightmap[z + 16 * x]; }
  inline const unsigned char & height(size_t x, size_t z) const { return m_heightmap[z + 16 * x]; }

  inline       unsigned char & blockType(size_t x, size_t y, size_t z)       { return m_data[offsetBlockType + index(x, y, z)]; }
  inline const unsigned char & blockType(size_t x, size_t y, size_t z) const { return m_data[offsetBlockType + index(x, y, z)]; }
  inline       unsigned char & blockType(const LocalCoords& lc)              { return blockType(lX(lc), lY(lc), lZ(lc)); }
  inline const unsigned char & blockType(const LocalCoords& lc)        const { return blockType(lX(lc), lY(lc), lZ(lc)); }

  inline void setBlockMetaData(size_t x, size_t y, size_t z, unsigned char val) { setHalf(y, val, m_data[offsetBlockMetaData + index(x, y, z) / 2]); }
  inline unsigned char getBlockMetaData(size_t x, size_t y, size_t z) const { return getHalf(y, m_data[offsetBlockMetaData + index(x, y, z) / 2]); }

  inline void setBlockLight(size_t x, size_t y, size_t z, unsigned char val) { setHalf(y, val, m_data[offsetBlockLight + index(x, y, z) / 2]); }
  inline void setBlockLight(const LocalCoords & lc, unsigned char val) { setBlockLight(lX(lc), lY(lc), lZ(lc), val); }
  inline unsigned char getBlockLight(size_t x, size_t y, size_t z) const { return getHalf(y, m_data[offsetBlockLight + index(x, y, z) / 2]); }
  inline unsigned char getBlockLight(const LocalCoords & lc) const { return getBlockLight(lX(lc), lY(lc), lZ(lc)); }

  inline void setSkyLight(size_t x, size_t y, size_t z, unsigned char val) { setHalf(y, val, m_data[offsetSkyLight + index(x, y, z) / 2]); }
  inline void setSkyLight(const LocalCoords & lc, unsigned char val) { setSkyLight(lX(lc), lY(lc), lZ(lc), val); }
  inline unsigned char getSkyLight(size_t x, size_t y, size_t z) const { return getHalf(y, m_data[offsetSkyLight + index(x, y, z) / 2]); }
  inline unsigned char getSkyLight(const LocalCoords & lc) const { return getSkyLight(lX(lc), lY(lc), lZ(lc)); }

  /// Compute the chunk's light map and height map.
  void updateLightAndHeightMaps(Map & map);
  void spreadLight(const WorldCoords & wc, unsigned char sky_light, unsigned char block_light, Map & map);

  /// The client expects chunks to be deflate()ed. ZLIB to the rescue.
  /// The beefed-up version uses a local cache if possible.
  std::string compress() const;
  std::pair<const unsigned char *, size_t> compress_beefedup();

private:
  // Own coordinates.
  ChunkCoords m_coords;

  /// Every chunk is exactly 80KiB in size.
  ChunkData m_data;

  /// The height map isn't stored, but only used by us in private.
  /// The value at (x, z) is the height of the last air block reachable by going down from height 127, or 127 if there is no air.
  ChunkHeightMap m_heightmap;

  struct ZCache
  {
    /* First 18 bytes:
       uint8_t type = PACKET_MAP_CHUNK (0x33)
       int32_t    X
       int16_t    Y
       int32_t    Z
       uint8_t    size_X = 15, size_Y = 127, size_Z = 15.
       int32_t    lenght of the subsequent data
       -------
       unsigned char data[len]
    */

    std::array<unsigned char, 10000> cache;
    size_t length;
    bool usable;

    ZCache(const ChunkCoords & cc)
      :
      cache(),
      length(0),
      usable(false)
    {
      cache[0] = 0x33;
      cache[11] = 0x0F; // 15
      cache[12] = 0x7F; // 127
      cache[13] = 0x0F; // 15

      WorldCoords wc = getWorldCoords(LocalCoords(0, 0), cc);
      writeInt32(wX(wc), cache.data() + 1);
      writeInt16(wY(wc), cache.data() + 5);
      writeInt32(wZ(wc), cache.data() + 7);
    }

    static inline void writeInt16(uint16_t x, unsigned char * p) { *p++ = (x >> 8) & 0xFF; *p = x & 0xFF; }
    static inline void writeInt32(uint32_t x, unsigned char * p) { *p++ = (x >> 24); *p++ = (x >> 16) & 0xFF; *p++ = (x >> 8) & 0xFF; *p = x & 0xFF; }
    inline void writeLength(size_t len) { writeInt32(len, cache.data() + 14); }
  } m_zcache;
};


typedef std::unordered_map<ChunkCoords, std::shared_ptr<Chunk>, PairHash<int32_t, int32_t>> ChunkMap;


#endif
