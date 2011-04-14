#ifndef H_MAP
#define H_MAP


#include <array>
#include <vector>
#include <memory>
#include <boost/noncopyable.hpp>
#include "chunk.h"
#include "serializer.h"


typedef std::unordered_map<int32_t, int> ItemMap;

class Map
{
public:
  Map(unsigned long long int ticks);

  inline bool haveChunk(const ChunkCoords & cc) const { return m_chunks.count(cc) > 0; }

  inline       Chunk & chunk(const ChunkCoords & cc)       { return *(m_chunks.find(cc)->second); }
  inline const Chunk & chunk(const ChunkCoords & cc) const { return *(m_chunks.find(cc)->second); }
  inline       Chunk & chunk(const WorldCoords & wc)       { return *(m_chunks.find(getChunkCoords(wc))->second); }
  inline const Chunk & chunk(const WorldCoords & wc) const { return *(m_chunks.find(getChunkCoords(wc))->second); }

  /// If no chunk exists at cc, load from disk or create a random one if none exists.
  void ensureChunkIsLoaded(const ChunkCoords & cc);

  /// Call this only when about to send to a client. Don't forget to call "spreadAllLight()" on all chunks after this call.
  inline void ensureChunkIsReadyForImmediateUse(const ChunkCoords & cc)
  {
    ensureChunkIsLoaded(cc);
    chunk(cc).updateLightAndHeightMaps();
  }

  inline void insertChunk(std::shared_ptr<Chunk> chunk) { m_chunks.insert(ChunkMap::value_type(chunk->coords(), chunk)); }

  inline bool hasItem(int32_t eid) const { return m_items.count(eid) > 0; }

  inline void save() { m_serializer.serialize(); }
  inline void load(std::string basename) { m_serializer.deserialize(basename); }

  unsigned long long int tick_counter;

private:
  ChunkMap   m_chunks;
  ItemMap    m_items;
  Serializer m_serializer;
};

#endif
