#ifndef H_GAMESTATEMANAGER
#define H_GAMESTATEMANAGER

#include <unordered_map>
#include <unordered_set>
#include "connection.h"
#include "types.h"
#include "constants.h"

class Map;

class GameState
{
public:
  enum EState { INVALID = 0, PRELOGIN, POSTLOGIN, READYTOSPAWN, SPAWNED, DEAD, TERMINATED };

  explicit GameState(EState s = INVALID) : state(s), position(), known_chunks() { }

  /// A simple global state flag
  EState state;

  /// The current position of the player
  WorldCoords position;

  /// The chunks that we've sent to the player
  std::unordered_set<ChunkCoords, PairHash<int32_t, int32_t>> known_chunks;

  /// The last dig operation, which we must validate.
  struct DigStatus { WorldCoords wc; long long int start_time; } recent_dig;

  /// Meta-data information on the direction from the user to wc.
  uint8_t getRelativeDirection(const WorldCoords & wc);

};

class GameStateManager
{
public:
  GameStateManager(std::function<void(unsigned int)> sleep, ConnectionManager & connection_mananger, Map & map);

  void update(int32_t);

  /// Use with an std::bound-outgoing packet builder below to send a packet to all clients.
  void sendToAll(std::function<void(int32_t)> f);

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

  enum EBlockPlacement { OK_NO_META, OK_WITH_META, CANNOT_PLACE };
  EBlockPlacement blockPlacement(int32_t eid, const WorldCoords & wc, Direction dir, BlockItemInfoMap::const_iterator it, uint8_t & meta);


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


  /* Outgoing packet builders */

  void packetSCKick(int32_t eid, const std::string & message);
  void packetSCKeepAlive(int32_t eid);
  void packetSCPreChunk(int32_t eid, const ChunkCoords & cc, bool mode);
  void packetSCMapChunk(int32_t eid, int32_t X, int32_t Y, int32_t Z, const std::string & data, size_t sizeX = 15, size_t sizeY = 127, size_t sizeZ = 15);
  inline void packetSCMapChunk(int32_t eid, std::pair<const unsigned char *, size_t> d) { m_connection_manager.sendDataToClient(eid, d.first, d.second); }
  inline void packetSCMapChunk(int32_t eid, const ChunkCoords & cc, const std::string & data) { packetSCMapChunk(eid, 16 * cX(cc), 0, 16 * cZ(cc), data); }
  void packetSCSpawn(int32_t eid, int32_t X, int32_t Y, int32_t Z);
  inline void packetSCSpawn(int32_t eid, const WorldCoords & wc) { packetSCSpawn(eid, wX(wc), wY(wc), wZ(wc)); }
  void packetSCPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool on_ground);
  void packetSCSetSlot(int32_t eid, int8_t window, int16_t slot, int16_t item, int8_t count = 1, int16_t uses = 0);
  void packetSCBlockChange(int32_t eid, int32_t X, int8_t Y, int32_t Z, int8_t block_type, int8_t block_md = 0);
  inline void packetSCBlockChange(int32_t eid, const WorldCoords & wc, int8_t block_type, int8_t block_md = 0) { packetSCBlockChange(eid, wX(wc), wY(wc), wZ(wc), block_type, block_md); }
  void packetSCTime(int32_t eid, int64_t ticks);
  void packetSCOpenWindow(int32_t eid, int8_t window_id, int8_t window_type, std::string title, int8_t slots);

  std::function<void(unsigned int)> sleepMilli;

private:
  ConnectionManager & m_connection_manager;
  Map & m_map;
 
  std::recursive_mutex m_gs_mutex;
  std::unordered_map<int32_t, GameState> m_states;
};


#endif
