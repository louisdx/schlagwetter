#include <iostream>
#include <mutex>
#include <zlib.h>

#include "constants.h"
#include "map.h"
#include "generator.h"
#include "packetcrafter.h"

static std::array<unsigned char, 100000> zlib_buffer;
static std::mutex zlib_buffer_mutex;


/*  The amount of light from the sky depending on the daytime, tick = 0 .. 23999.
 *
 *      0 - 12000: bright day, 15
 *  12000 - 14000: dusk, -1 every 200 ticks
 *  14000 - 22000: night, 4
 *  22000 - 24000: dawn, +1 every 200 tick
 *
 */
int skySourceLight(unsigned long long ticks)
{
  if      (                  ticks < 12000) return 15;
  else if (12000 <= ticks && ticks < 14000) return 14 - (ticks - 12000) / 200;
  else if (14000 <= ticks && ticks < 22000) return  4;
  else if (22000 <= ticks && ticks < 24000) return  5 + (ticks - 22000) / 200;

  std::cout << "Call skySourceLigth() with a value mod 24000!" << std::endl;
  return 0;
}

Chunk::Chunk(const ChunkCoords & cc)
  :
  m_coords(cc),
  m_data(),
  m_heightmap(),
  m_zcache(m_coords)
{
}

Chunk::Chunk(const ChunkCoords & cc, const ChunkData & data, const ChunkHeightMap & hm)
  :
  m_coords(cc),
  m_data(data),
  m_heightmap(hm),
  m_zcache(m_coords)
{
}

/// This version uses global memory to hold the deflated data and returns a copy.
std::string Chunk::compress() const
{
  /// Our chunks are always 80KiB, so we assume that 100KB suffice for the output and skip the bound check.
  //unsigned long int outlength = compressBound(size());
  //unsigned char * buf = new unsigned char[outlength];
  //...
  //delete[] buf;

  std::string result;

  if (zlib_buffer_mutex.try_lock())
  {
    try
    {
      unsigned long int outlength = zlib_buffer.size();
      int zres = ::compress(zlib_buffer.data(), &outlength, m_data.data(), size());
      if (zres != Z_OK)
      {
        std::cerr << "Error during zlib deflate!" << std::endl;
      }
      else
      {
        std::cout << "zlib deflate: " << std::dec << outlength << std::endl;
        result = std::string(reinterpret_cast<char*>(zlib_buffer.data()), outlength);
      }
    }
    catch (...)
    {
      std::cout << "Exception caught during ZLIB compression!" << std::endl;
    }

    zlib_buffer_mutex.unlock();
  }
  else
  {
    std::cout << "PANIC: Access to static ZLIB buffer locked, you MUST rework the threading model." << std::endl;
  }

  return result;
}


/// This version uses a (rather small) per-chunk zip cache.
std::pair<const unsigned char *, size_t> Chunk::compress_beefedup()
{
  if (m_zcache.usable)
  {
    std::cout << "cached zlib deflate cache hit! (" << std::dec << m_zcache.length << " bytes)" << std::endl;
    return std::make_pair(m_zcache.cache.data(), m_zcache.length + 18);
  }

  unsigned long int outlength = m_zcache.cache.size() - 18;
  int zres = ::compress(m_zcache.cache.data() + 18, &outlength, m_data.data(), size());
  if (zres != Z_OK)
  {
    std::cerr << "Error during beefed-up zlib deflate!" << std::endl;
    outlength = 0;
  }
  else
  {
    std::cout << "caching zlib deflate: " << std::dec << outlength << std::endl;
    m_zcache.usable = true;
  }

  m_zcache.writeLength(outlength);
  m_zcache.length = outlength;

  return std::make_pair(m_zcache.cache.data(), m_zcache.length + 18);
}


Map::Map(unsigned long long int ticks)
  :
  tick_counter(ticks),
  m_chunkMap(),
  m_serializer()
{
}

void Map::ensureChunkIsLoaded(const ChunkCoords & cc)
{
  if (m_chunkMap.count(cc) == 0)
  {
    if (m_serializer.haveChunk(cc) == true)
    {
      m_chunkMap.insert(ChunkMap::value_type(cc, m_serializer.loadChunk(cc))).first;
    }
    else
    {
      auto ins = m_chunkMap.insert(ChunkMap::value_type(cc, std::make_shared<Chunk>(cc))).first;
      generateWithNoise(*ins->second, cc);
      ins->second->updateLightAndHeightMaps(tick_counter % 24000);
    }
  }
}

void Chunk::updateLightAndHeightMaps(unsigned long long ticks)
{
  // Clear lightmaps
  std::fill(m_data.data() + offsetBlockLight, m_data.data() + offsetBlockLight + sizeBlockLight + sizeSkyLight, 0);

  for (int x = 0; x < 16; ++x)
  {
    for (int z = 0; z < 16; ++z)
    {
      // Sky light and height map

      int  light = skySourceLight(ticks);
      int  first_nonbright_y = 0;
      height(x, z) = 0;

      for (int y = 127; y >= 0; --y)
      {
        const unsigned char & block = blockType(x, y, z);

        light -= int(STOP_LIGHT[block]);
        light = std::max(light, 0);

        if (light) setSkyLight(x, y, z, light);

        if (height(x, z) == 0 && (block != BLOCK_Air))
        {
          height(x, z) = y + 1;
        }

        // at non-day, computations are harder because we must potentially illuminate the entire night sky :-(
        if (first_nonbright_y == 0 && light < 15) // <---- 15 will be only in the bright sun
        {
          first_nonbright_y = y;
        }

        // We can stop when we got the height map and there's no more light to go around.
        if (light == 0 && height(x, z) != 0) break;
      }

      // Block emissive light

      for (int y = first_nonbright_y; y >= 0; --y)  // we don't need to bother if the skylight is already at max
        if (EMIT_LIGHT[blockType(x, y, z)] > 0)
        {
          setBlockLight(x, y, z, EMIT_LIGHT[blockType(x, y, z)]);
        }

    }   // z
  }     // x
}

void Chunk::spreadAllLight(Map & map)
{
  // Spread light no neighbouring blocks

  for (int x = 0; x < 16; ++x)
    for (int z = 0; z < 16; ++z)
      // only non-air blocks can emit, and for passive blocks, only the layer immediately above matters.
      for (int y = std::min(127, int(height(x, z))); y >= 0; --y)
        {
          if (getSkyLight(x, y, z))
            spreadLightRecursively(getWorldCoords(LocalCoords(x, y, z), coords()), getSkyLight(x, y, z), map, 0 /* == sky */);

          if (getBlockLight(x, y, z))
            spreadLightRecursively(getWorldCoords(LocalCoords(x, y, z), coords()), getBlockLight(x, y, z), map, 1 /* == block */);
        }
}

void Chunk::spreadLightRecursively(const WorldCoords & wc, unsigned char value, Map & map, uint8_t type, size_t recursion_depth)
{
  if (recursion_depth > 16)
  {
    std::cout << "Panic, spreadLight() [Type = " << (unsigned int)(type) << "] recursed unexpectedly!" << std::endl;
    return;
  }

  // If no light, stop!
  if (value == 0) return;

  for (size_t direction = 0; direction < 6; ++direction)
  {
    if      ((lY(wc) == 127) && (direction == BLOCK_YPLUS))  ++direction;     // Going too high
    else if ((lY(wc) ==   0) && (direction == BLOCK_YMINUS)) ++direction;     // Going too low

    WorldCoords to_set = wc + Direction(direction);

    if (!map.haveChunk(getChunkCoords(to_set))) continue; // Only spread to chunks that exist.

    Chunk & chunk = map.chunk(getChunkCoords(to_set)); // Most times chunk == *this.
    const unsigned char & block = chunk.blockType(getLocalCoords(to_set));

    const unsigned char value_new = std::max(0, int(value) - int(STOP_LIGHT[block]) - 1);

    if (value_new > ( type == 0 ? chunk.getSkyLight(getLocalCoords(to_set)) : chunk.getBlockLight(getLocalCoords(to_set)) ))
    {
      if      (type == 0) chunk.setSkyLight  (getLocalCoords(to_set), value_new);
      else if (type == 1) chunk.setBlockLight(getLocalCoords(to_set), value_new);

      chunk.spreadLightRecursively(to_set, value_new, map, type, recursion_depth + 1);
    }
  }
}
