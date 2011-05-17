#ifndef H_MAP
#define H_MAP


#include <array>
#include <vector>
#include <memory>
#include <unordered_map>
#include <boost/noncopyable.hpp>

#include "chunk.h"
#include "serializer.h"


class Map
{
public:

  Map(unsigned long long int ticks);

  struct BlockAlert
  {
    enum EAlert { SUPPORTS_CANDLE, CONTAINS_SPAWN_ITEM } type;

    int32_t data;

    explicit BlockAlert(EAlert type, int32_t data = -1) : type(type), data(data) { }
  };

  typedef std::unordered_map<int32_t, int> ItemMap;
  typedef std::unordered_multimap<WorldCoords, BlockAlert, TripleHash<int32_t, int32_t, int32_t>> AlertMap;

  inline bool haveChunk(const ChunkCoords & cc) const { return m_chunks.count(cc) > 0; }

  inline       Chunk & chunk(const ChunkCoords & cc)       { return *(m_chunks.find(cc)->second); }
  inline const Chunk & chunk(const ChunkCoords & cc) const { return *(m_chunks.find(cc)->second); }
  inline       Chunk & chunk(const WorldCoords & wc)       { return *(m_chunks.find(getChunkCoords(wc))->second); }
  inline const Chunk & chunk(const WorldCoords & wc) const { return *(m_chunks.find(getChunkCoords(wc))->second); }

  inline const ItemMap  & items()       const { return m_items; }
  inline       ItemMap  & items()             { return m_items; }

  inline const AlertMap & blockAlerts() const { return m_block_alerts; }
  inline       AlertMap & blockAlerts()       { return m_block_alerts; }

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

  inline void load(const std::string & basename) { m_serializer.deserialize(basename); }

  unsigned long long int tick_counter;

private:
  ChunkMap   m_chunks;
  ItemMap    m_items;
  AlertMap   m_block_alerts;
  Serializer m_serializer;
};

#endif
