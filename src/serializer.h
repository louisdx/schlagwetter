#ifndef H_SERIALIZER
#define H_SERIALIZER


#include <string>
#include <boost/noncopyable.hpp>
#include "chunk.h"

class Map;

class Serializer : private boost::noncopyable
{
public:
  Serializer(ChunkMap & chunk_map, Map & map);

  bool haveChunk(const ChunkCoords & cc);
  ChunkMap::mapped_type loadChunk(const ChunkCoords & cc);
  void writeChunk(ChunkMap::mapped_type chunk);

  void serialize();
  void deserialize(const std::string & basename);

private:
  ChunkMap & m_chunk_map;
  Map      & m_map;
};


#endif
