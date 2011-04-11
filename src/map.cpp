#include <iostream>
#include <mutex>
#include <zlib.h>

#include "constants.h"
#include "map.h"
#include "generator.h"
#include "packetcrafter.h"

static std::array<unsigned char, 100000> zlib_buffer;
static std::mutex zlib_buffer_mutex;


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


Map::Map()
  :
  m_chunkMap(),
  m_serializer()
{
}

Chunk & Map::getChunkOrGenerateNew(const ChunkCoords & cc)
{
  ChunkMap::iterator ins = m_chunkMap.find(cc);

  if (ins == m_chunkMap.end())
  {
    if (m_serializer.haveChunk(cc) == true)
    {
      ins = m_chunkMap.insert(ChunkMap::value_type(cc, m_serializer.loadChunk(cc))).first;
    }
    else
    {
      ins = m_chunkMap.insert(ChunkMap::value_type(cc, std::make_shared<Chunk>(cc))).first;
      generateWithNoise(*ins->second, cc);
      (*ins->second).updateLightAndHeightMaps(*this); // Off for now so you can marvel at the Cave :-)
    }
  }

  return *ins->second;
}

void Chunk::updateLightAndHeightMaps(Map & map)
{
  // Clear lightmaps
  std::fill(m_data.data() + offsetBlockLight, m_data.data() + offsetBlockLight + sizeBlockLight + sizeSkyLight, 0);

  // The highest y coordinate which is completely dark
  int highest_y = 0;

  // Sky light
  int light;
  bool foundheight = false;

  for (int x = 0; x < 16; ++x)
  {
    for (int z = 0; z < 16; ++z)
    {
      light = 15;
      foundheight = false;

      for (int y = 127; y > 0; --y)
      {
        const unsigned char & block = blockType(x, y, z);

        light -= STOP_LIGHT[block];
        light = std::max(light, 0);

        // Calculate height map while looping this
        if (!foundheight && (block != BLOCK_Air))
        {
          height(x, z) = (y == 127 ? y : y + 1);
          foundheight = true;
        }

        // Update highest_y
        if (light == 0)
        {
          highest_y = std::max(highest_y, y);
          break;
        }

        setSkyLight(x, y, z, light);
      }
    }
  }

  // Block light

  for (int x = 0; x < 16; ++x)
    for (int z = 0; z < 16; ++z)
      for (int y = highest_y; y >= 0; --y)
        if (EMIT_LIGHT[blockType(x, y, z)] > 0)
          setBlockLight(x, y, z, EMIT_LIGHT[blockType(x, y, z)]);

  // Spread light
  for (int x = 0; x < 16; ++x)
    for (int z = 0; z < 16; ++z)
      for (int y = height(x, z); y >= 0; --y)
        if (getSkyLight(x, y, z) || getBlockLight(x, y, z))
          spreadLight(getWorldCoords(LocalCoords(x, y, z), coords()), getSkyLight(x, y, z), getBlockLight(x, y, z), map);
}


void Chunk::spreadLight(const WorldCoords & wc, unsigned char sky_light, unsigned char block_light, Map & map)
{
  // If no light, stop!
  if (sky_light == 0 && block_light == 0) return;

  for (int direction = 0; direction < 6; ++direction)
  {
    // Going too high
    if ((lY(wc) == 127) && (direction == 2))
    {
      //Skip this direction
      ++direction;
    }
    // Going too low
    else if ((lY(wc) == 0) && (direction == 3))
    {
      //Skip this direction
      ++direction;
    }

    WorldCoords to_set(wc);

    switch (direction)
    {
    case 0:
      ++wX(to_set);
      break;
    case 1:
      --wX(to_set);
      break;
    case 2:
      ++wY(to_set);
      break;
    case 3:
      --wY(to_set);
      break;
    case 4:
      ++wZ(to_set);
      break;
    case 5:
      --wZ(to_set);
      break;
    };

    if (map.haveChunk(getChunkCoords(to_set)))
    {
      Chunk & chunk = map.chunk(getChunkCoords(to_set)); // Most times chunk == *this.
      const unsigned char & block = chunk.blockType(getLocalCoords(to_set));

      const unsigned char sky_light_new   = std::max(0, sky_light   - STOP_LIGHT[block] - 1);
      const unsigned char block_light_new = std::max(0, block_light - STOP_LIGHT[block] - 1);

      bool spread = false;

      if (sky_light_new > chunk.getSkyLight(getLocalCoords(to_set)))
      {
        chunk.setSkyLight(getLocalCoords(to_set), sky_light_new);
        spread = true;
      }

      if (block_light_new > chunk.getBlockLight(getLocalCoords(to_set)))
      {
        chunk.setBlockLight(getLocalCoords(to_set), block_light_new);
        spread = true;
      }

      if (spread) 
        chunk.spreadLight(to_set, chunk.getSkyLight(getLocalCoords(to_set)), chunk.getBlockLight(getLocalCoords(to_set)), map);
    }
  }
}
