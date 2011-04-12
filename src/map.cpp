#include <iostream>

#include "map.h"
#include "generator.h"


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
