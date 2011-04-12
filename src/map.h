#ifndef H_MAP
#define H_MAP


#include <array>
#include <vector>
#include <memory>
#include <boost/noncopyable.hpp>
#include "chunk.h"
#include "serializer.h"

class Map
{
public:
  Map(unsigned long long int ticks);

  inline bool haveChunk(const ChunkCoords & cc) const { return m_chunkMap.count(cc) > 0; }

  inline       Chunk & chunk(const ChunkCoords & cc)       { return *(m_chunkMap.find(cc)->second); }
  inline const Chunk & chunk(const ChunkCoords & cc) const { return *(m_chunkMap.find(cc)->second); }
  inline       Chunk & chunk(const WorldCoords & wc)       { return *(m_chunkMap.find(getChunkCoords(wc))->second); }
  inline const Chunk & chunk(const WorldCoords & wc) const { return *(m_chunkMap.find(getChunkCoords(wc))->second); }

  /// If no chunk exists at cc, load from disk or create a random one if none exists.
  void ensureChunkIsLoaded(const ChunkCoords & cc);

  inline void insertChunk(std::shared_ptr<Chunk> chunk) { m_chunkMap.insert(ChunkMap::value_type(chunk->coords(), chunk)); }

  unsigned long long int tick_counter;

private:
  ChunkMap m_chunkMap;
  Serializer m_serializer;
};

#endif
