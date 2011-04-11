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
  Map();

  inline bool haveChunk(const ChunkCoords & cc) const { return m_chunkMap.count(cc) > 0; }

  inline       Chunk & chunk(const ChunkCoords & cc)       { return *(m_chunkMap.find(cc)->second); }
  inline const Chunk & chunk(const ChunkCoords & cc) const { return *(m_chunkMap.find(cc)->second); }

  /// If no chunk exists at cc, create a random one. Always returns the chunk at cc.
  Chunk & getChunkOrGenerateNew(const ChunkCoords & cc);

  inline void insertChunk(std::shared_ptr<Chunk> chunk) { m_chunkMap.insert(ChunkMap::value_type(chunk->coords(), chunk)); }

private:
  ChunkMap m_chunkMap;
  Serializer m_serializer;
};

#endif
