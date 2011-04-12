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

  inline bool haveChunk(const ChunkCoords & cc) const { return m_chunk_map.count(cc) > 0; }

  inline       Chunk & chunk(const ChunkCoords & cc)       { return *(m_chunk_map.find(cc)->second); }
  inline const Chunk & chunk(const ChunkCoords & cc) const { return *(m_chunk_map.find(cc)->second); }
  inline       Chunk & chunk(const WorldCoords & wc)       { return *(m_chunk_map.find(getChunkCoords(wc))->second); }
  inline const Chunk & chunk(const WorldCoords & wc) const { return *(m_chunk_map.find(getChunkCoords(wc))->second); }

  /// If no chunk exists at cc, load from disk or create a random one if none exists.
  void ensureChunkIsLoaded(const ChunkCoords & cc);

  inline void insertChunk(std::shared_ptr<Chunk> chunk) { m_chunk_map.insert(ChunkMap::value_type(chunk->coords(), chunk)); }

  inline void save() { m_serializer.serialize(); }
  inline void load() { m_serializer.deserialize(); }

  unsigned long long int tick_counter;

private:
  ChunkMap m_chunk_map;
  Serializer m_serializer;
};

#endif
