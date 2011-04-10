#include "serializer.h"

Serializer::Serializer()
{
}

bool Serializer::haveChunk(const ChunkCoords & cc)
{
  (void)cc;
  return false;
}

ChunkMap::mapped_type Serializer::loadChunk(const ChunkCoords & cc)
{
  return std::make_shared<Chunk>(cc);
}
