#ifndef H_SERIALIZER
#define H_SERIALIZER


#include <boost/noncopyable.hpp>
#include "chunk.h"

class Serializer : private boost::noncopyable
{
public:
  Serializer(ChunkMap & chunk_map);

  bool haveChunk(const ChunkCoords & cc);
  ChunkMap::mapped_type loadChunk(const ChunkCoords & cc);
  void writeChunk(ChunkMap::mapped_type chunk);

  void serialize();
  void deserialize();

private:
  ChunkMap & m_chunk_map;
};


#endif
