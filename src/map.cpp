#include <iostream>

#include "constants.h"
#include "map.h"
#include "generator.h"
#include "packetcrafter.h"


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

Map::Map(unsigned long long int ticks)
  :
  tick_counter(ticks),
  m_chunk_map(),
  m_serializer(m_chunk_map)
{
}

void Map::ensureChunkIsLoaded(const ChunkCoords & cc)
{
  if (m_chunk_map.count(cc) == 0)
  {
    if (m_serializer.haveChunk(cc) == true)
    {
      m_chunk_map.insert(ChunkMap::value_type(cc, m_serializer.loadChunk(cc))).first;
    }
    else
    {
      std::cout << "** generating chunk **" << std::endl;
      auto ins = m_chunk_map.insert(ChunkMap::value_type(cc, std::make_shared<Chunk>(cc))).first;
      generateWithNoise(*ins->second, cc);
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

    if (value_new > ( (type == 0) ? chunk.getSkyLight(getLocalCoords(to_set)) : chunk.getBlockLight(getLocalCoords(to_set)) ))
    {
      if      (type == 0) chunk.setSkyLight  (getLocalCoords(to_set), value_new);
      else if (type == 1) chunk.setBlockLight(getLocalCoords(to_set), value_new);

      chunk.spreadLightRecursively(to_set, value_new, map, type, recursion_depth + 1);
    }
  }
}
