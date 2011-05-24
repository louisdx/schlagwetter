#ifndef H_GAMESTATEMANAGER
#define H_GAMESTATEMANAGER

#include <unordered_map>
#include <unordered_set>
#include "connection.h"
#include "types.h"
#include "constants.h"

class Map;

class PlayerState
{
public:
  enum EState { INVALID = 0, PRELOGIN, POSTLOGIN, READYTOSPAWN, SPAWNED, DEAD, TERMINATED };

  explicit PlayerState(EState s = INVALID);

  /// A simple global state flag
  EState state;

  /// The current position of the player
  RealCoords position;
  float stance;
  float pitch;
  float yaw;

  /// The chunks that we've sent to the player
  std::unordered_set<ChunkCoords> known_chunks;

  /// The last dig operation, which we must validate.
  struct DigStatus { WorldCoords wc; long long int start_time; } recent_dig;

  /// Meta-data information on the direction from the user to wc.
  Direction getRelativeXZDirection(const RealCoords & rc);

  /// The inventory.
  std::array< int16_t, 45> inventory_ids;
  std::array<uint16_t, 45> inventory_damage;
  std::array<uint16_t, 45> inventory_count;
  size_t                   holding; // 0-8, the inventory slot currently held in the hand

  inline void setInv(size_t slot, int16_t type, uint16_t count, uint16_t damage)
  {
    inventory_ids[slot]    = type;
    inventory_count[slot]  = count;
    inventory_damage[slot] = damage;
  }
};

class GameStateManager
{
public:
  GameStateManager(std::function<void(unsigned int)> sleep, ConnectionManager & connection_mananger, Map & map);

  void update(int32_t);

  /// Use with an std::bound-outgoing packet builder below to send a packet to all clients.
  void sendToAll(std::function<void(int32_t)> f);
  void sendToAllExceptOne(std::function<void(int32_t)> f, int32_t eid);

  /// Maybe sending raw data is more efficient, not invoking the packet builder each time.
  void sendRawToAll(const std::string & data);
  void sendRawToAllExceptOne(const std::string & data, int32_t eid);

  /* The following macros are useful for invoking sendToAll().
   * MAKE_CALLBACK is for most handlers
   * MAKE_EXPLICIT_CALLBACK is for overloaded ones where you have to provide an explicit function pointer.
   * MAKE_SIGNED_CALLBACK is for handlers with a specified signature
   *
   * Examples:
   *
   *   // Normal use
   *   sendToAll(MAKE_CALLBACK(packetSCKick, "Suckers!"));
   *
   *   // Request explicit overloads
   *   sendToAll(MAKE_SIGNED_CALLBACK(packetSCBlockChange, (int32_t, int32_t X, int8_t Y, int32_t Z, int8_t, int8_t), myX, myY, myZ, block_id, block_meta));
   *   sendToAll(MAKE_SIGNED_CALLBACK(packetSCBlockChange, (int32_t, const WorldCoords &, int8_t, int8_t), wc, block_id, block_meta));
   *
   * Note that the member function must not be const -- I did not bother implementing that
   * (cf. http://stackoverflow.com/questions/5608606/callback-with-variadic-template-to-ambiguous-overloads/5611367#5611367)
   *
   */
#define MAKE_CALLBACK(f, ...) std::bind(&GameStateManager::f, this, std::placeholders::_1, __VA_ARGS__)
#define MAKE_EXPLICIT_CALLBACK(f, ...) std::bind(f, this, std::placeholders::_1, __VA_ARGS__)
#define MAKE_SIGNED_CALLBACK(p, SIGNATURE, ...) MAKE_EXPLICIT_CALLBACK(  (void (GameStateManager::*)SIGNATURE)(&GameStateManager::p), __VA_ARGS__)

  /// Determine chunks the player might need and send.
  void sendMoreChunksToPlayer(int32_t eid);

  /// Retransmit the entire inventory to the player (45 packets).
  void sendInventoryToPlayer(int32_t eid);

  /// Add a single item to the player's inventory, optionally
  /// send the update to the player immediately. Returns the number
  /// of items that couldn't be allocated (if the inventory is full).
  uint16_t updatePlayerInventory(int32_t eid, int16_t type, uint16_t count, uint16_t damage, bool send_packets = true);

  enum EBlockPlacement { OK_NO_META, OK_WITH_META, CANNOT_PLACE, CANNOT_PLACE_AIRFORCE };
  EBlockPlacement blockPlacement(int32_t eid, const WorldCoords & wc, Direction dir, BlockItemInfoMap::const_iterator it, uint8_t & meta);
  void reactToSuccessfulDig(const WorldCoords & wc, EBlockItem block_type);
  void reactToToggle(const WorldCoords & wc, EBlockItem b);
  void reactToBlockDestruction(const WorldCoords & wc);

  void spawnSomething(uint16_t type, uint8_t number, uint8_t damage, const WorldCoords & wc);

  void makeItemsDrop(const WorldCoords & wc);
  bool fall(WorldCoords & wc);

  void handlePlayerMove(int32_t eid);

  std::function<void(unsigned int)> sleepMilli;

  /// Loading and saving player states to disk.
  void serializePlayer(int32_t eid);
  void deserializePlayer(int32_t eid);

  /* Incoming packet handlers */

  void packetCSPlayerDigging(int32_t eid, int32_t X, uint8_t Y, int32_t Z, uint8_t status, uint8_t face);
  void packetCSBlockPlacement(int32_t eid, int32_t X, int8_t Y, int32_t Z, int8_t direction, int16_t block_id, int8_t amount, int16_t damage);

  void packetCSKeepAlive(int32_t eid);
  void packetCSHandshake(int32_t eid, const std::string & name);
  void packetCSLoginRequest(int32_t eid, int32_t protocol_version, const std::string & username, const std::string & password, int64_t map_seed, int8_t dimension);
  void packetCSDisconnect(int32_t eid, std::string message);

  void packetCSChunkRequest(int32_t eid, int32_t X, int32_t Z, bool mode);
  void packetCSUseEntity(int32_t eid, int32_t e, int32_t target, bool leftclick);
  void packetCSPlayer(int32_t eid, bool ground);
  void packetCSPlayerPosition(int32_t eid, double X, double Y, double Z, double stance, bool ground);
  void packetCSPlayerLook(int32_t eid, float yaw, float pitch, bool ground);
  void packetCSPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool ground);
  void packetCSHoldingChange(int32_t eid, int16_t slot);
  void packetCSArmAnimation(int32_t eid, int32_t e, int8_t animate);
  void packetCSEntityCrouchBed(int32_t eid, int32_t e, int8_t action);
  void packetCSPickupSpawn(int32_t eid, int32_t e, int32_t X, int32_t Y, int32_t Z, double rot, double pitch, double roll, int8_t count, int16_t item, int16_t data);
  void packetCSRespawn(int32_t eid);
  void packetCSCloseWindow(int32_t eid, int8_t window_id);
  void packetCSChatMessage(int32_t eid, std::string message);
  void packetCSWindowClick(int32_t eid, int8_t window_id, int16_t slot, int8_t right_click, int16_t action, int16_t item_id, int8_t item_count, int16_t item_uses);
  void packetCSSign(int32_t eid, int32_t X, int16_t Y, int32_t Z, std::string line1, std::string line2, std::string line3, std::string line4);


  /* Outgoing packet builders. The "raw" version does not post the packet, only returns its data. */

  void packetSCKick(int32_t eid, const std::string & message);
  void packetSCKeepAlive(int32_t eid);
  void packetSCSpawn(int32_t eid, const WorldCoords & wc);
  void packetSCPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool on_ground);
  std::string rawPacketSCPlayerPositionAndLook(const RealCoords & rc, double stance, float yaw, float pitch, bool on_ground);
  void packetSCSetSlot(int32_t eid, int8_t window, int16_t slot, int16_t item, int8_t count = 1, int16_t uses = 0);
  void packetSCHoldingChange(int32_t eid, int16_t slot);
  void packetSCBlockChange(int32_t eid, const WorldCoords & wc, int8_t block_type, int8_t block_md = 0);
  void packetSCTime(int32_t eid, int64_t ticks);
  void packetSCOpenWindow(int32_t eid, int8_t window_id, int8_t window_type, std::string title, int8_t slots);
  void packetSCPickupSpawn(int32_t eid, int32_t e, uint16_t type, uint8_t count, uint16_t da, const WorldCoords & wc);
  void packetSCPreChunk(int32_t eid, const ChunkCoords & cc, bool mode);
  void packetSCMapChunk(int32_t eid, int32_t X, int32_t Y, int32_t Z, const std::string & data, size_t sizeX = 15, size_t sizeY = 127, size_t sizeZ = 15);
  inline void packetSCMapChunk(int32_t eid, std::pair<const unsigned char *, size_t> d) { m_connection_manager.sendDataToClient(eid, d.first, d.second); }
  inline void packetSCMapChunk(int32_t eid, const ChunkCoords & cc, const std::string & data) { packetSCMapChunk(eid, 16 * cX(cc), 0, 16 * cZ(cc), data); }
  void packetSCCollectItem(int32_t eid, int32_t collectee_eid, int32_t collector_eid);
  void packetSCDestroyEntity(int32_t eid, int32_t e);
  void packetSCChatMessage(int32_t eid, std::string message);
  void packetSCSpawnEntity(int32_t eid, int32_t e, const FractionalCoords & fc, double rot, double pitch, uint16_t item_id);
  std::string rawPacketSCEntityTeleport(int32_t e, const FractionalCoords & fc, double yaw, double pitch);
  std::string rawPacketSCHoldingChange(int16_t slot);

private:
  ConnectionManager & m_connection_manager;
  Map & m_map;
 
  std::recursive_mutex m_gs_mutex;
  std::unordered_map<int32_t, std::shared_ptr<PlayerState>> m_states;
};


struct L1DistanceFrom
{
  L1DistanceFrom(const ChunkCoords & cc) : cc(cc) { }
  inline bool operator()(const ChunkCoords & a, const ChunkCoords & b) const
  {
    return std::abs(cX(a) - cX(cc)) + std::abs(cZ(a) - cZ(cc)) < std::abs(cX(b) - cX(cc)) + std::abs(cZ(b) - cZ(cc));
  }
  ChunkCoords cc;
};


#endif
