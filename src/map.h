#ifndef H_MAP
#define H_MAP


#include <array>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <boost/noncopyable.hpp>

#include "chunk.h"
#include "serializer.h"


struct InventoryItem
{
  int16_t  id;
  uint16_t count;
  uint16_t damage;
};

typedef std::map<size_t, InventoryItem> InventoryCollection; // maps slot number to inventory item

enum EStorage { ERROR = 0x00, SINGLE_CHEST = 0x01, DOUBLE_CHEST = 0x02, FURNACE = 0x03, DISPENSER = 0x04 };

struct StorageUnit
{
  StorageUnit()
  :
  type(ERROR),
  nickhash(),
  inventory()
  {
    std::fill(nickhash.begin(), nickhash.end(), 0);
  }

  EStorage type; // single chest, double chest, furnace, dispenser

  std::array<unsigned char, 20> nickhash;

  InventoryCollection inventory;
};

/// Every world storage object (chests, furnaces, dispensers) gets a unique ID.
/// and to each ID we associate an inventory collection.
typedef std::unordered_map<uint32_t, StorageUnit> MapStorage;

/// Each storage unit is stored sorted by its ID.
typedef std::unordered_map<WorldCoords, uint32_t> StorageIndex;

/// Open windows need a temporarily unique ID, but we only have 8 bits for that.
typedef std::unordered_map<uint32_t, int8_t> WindowIDs;

extern uint32_t INVENTORY_UID_POOL;

inline uint32_t GenerateInventoryUID() { return ++INVENTORY_UID_POOL; }


class Server;
class UI;

class Map
{
  friend bool pump(Server &, UI &);
  friend class Serializer;

public:

  Map(unsigned long long int ticks, int seed);

  struct BlockAlert
  {
    enum EAlert { SUPPORTS_CANDLE, CONTAINS_SPAWN_ITEM } type;

    int32_t data;

    explicit BlockAlert(EAlert type, int32_t data = -1) : type(type), data(data) { }
  };

  typedef std::unordered_map<int32_t, int> ItemMap;
  typedef std::unordered_multimap<WorldCoords, BlockAlert> AlertMap;

  inline bool haveChunk(const ChunkCoords & cc) const { return m_chunks.count(cc) > 0; }

  inline       Chunk & chunk(const ChunkCoords & cc)       { return *(m_chunks.find(cc)->second); }
  inline const Chunk & chunk(const ChunkCoords & cc) const { return *(m_chunks.find(cc)->second); }
  inline       Chunk & chunk(const WorldCoords & wc)       { return *(m_chunks.find(getChunkCoords(wc))->second); }
  inline const Chunk & chunk(const WorldCoords & wc) const { return *(m_chunks.find(getChunkCoords(wc))->second); }

  inline const ItemMap  & items()       const { return m_items; }
  inline       ItemMap  & items()             { return m_items; }

  inline const AlertMap & blockAlerts() const { return m_block_alerts; }
  inline       AlertMap & blockAlerts()       { return m_block_alerts; }

  inline int seed() const { return m_seed; }
  inline int & seed()     { return m_seed; }

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

  void addStorage(const WorldCoords & wc, uint8_t block_type);
  void addStorage(const WorldCoords & wc, EStorage type);
  void removeStorage(const WorldCoords & wc);
  inline uint32_t storageIndex(const WorldCoords & wc) const
  {
    auto it = m_stridx.find(wc);
    return it == m_stridx.end() ? 0 : it->second;
  }

  inline void save() { m_serializer.serialize(); }

  inline void load(const std::string & basename) { m_serializer.deserialize(basename); }

  unsigned long long int tick_counter;

private:
  ChunkMap   m_chunks;
  ItemMap    m_items;
  AlertMap   m_block_alerts;
  Serializer m_serializer;

  int        m_seed;

  MapStorage m_storage;
  StorageIndex m_stridx;
  WindowIDs  m_window_ids;
};

#endif
