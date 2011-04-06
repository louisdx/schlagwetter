#ifndef H_GAMESTATEMANAGER
#define H_GAMESTATEMANAGER

#include "connection.h"

class Map;

class GameState
{
public:
  enum EState { INVALID = 0, PRELOGIN, POSTLOGIN, READYTOSPAWN, SPAWNED, DEAD, TERMINATED };

  explicit GameState(EState s = INVALID) : state(s) { }

  EState state; 
};

class GameStateManager
{
public:
  GameStateManager(ConnectionManager & connection_mananger, Map & map);

  void update(int32_t);

  void packetCSKeepAlive(int32_t eid);
  void packetCSUseEntity(int32_t eid, int32_t e, int32_t target, bool leftclick);
  void packetCSPlayer(int32_t eid, bool ground);
  void packetCSPlayerPosition(int32_t eid, double X, double Y, double Z, double stance, bool ground);
  void packetCSPlayerLook(int32_t eid, float yaw, float pitch, bool ground);
  void packetCSPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool ground);
  void packetCSPlayerDigging(int32_t eid, int32_t X, uint8_t Y, int32_t Z, uint8_t status, uint8_t face);
  void packetCSHoldingChange(int32_t eid, int16_t slot);
  void packetCSArmAnimation(int32_t eid, int32_t e, int8_t animate);
  void packetCSEntityCrouchBed(int32_t eid, int32_t e, int8_t action);
  void packetCSPickupSpawn(int32_t eid, int32_t e, int32_t X, int32_t Y, int32_t Z, int8_t rotp, int8_t pitchp, int8_t rollp, int8_t count, int16_t item, int16_t data);
  void packetCSRespawn(int32_t eid);
  void packetCSCloseWindow(int32_t eid, int8_t window_id);
  void packetCSHandshake(int32_t eid, const std::string & name);
  void packetCSLoginRequest(int32_t eid, int32_t protocol_version, const std::string & username, const std::string & password, int64_t map_seed, int8_t dimension);
  void packetCSBlockPlacement(int32_t eid, int32_t X, uint8_t Y, int32_t Z, int8_t direction, int16_t block_id, int8_t amount, int16_t damage);
  void packetCSChatMessage(int32_t eid, std::string message);
  void packetCSDisconnect(int32_t eid, std::string message);
  void packetCSWindowClick(int32_t eid, int8_t window_id, int16_t slot, int8_t right_click, int16_t action, int16_t item_id, int8_t item_count, int16_t item_uses);
  void packetCSSign(int32_t eid, int32_t X, int16_t Y, int32_t Z, std::string line1, std::string line2, std::string line3, std::string line4);

private:
  ConnectionManager & m_connection_manager;
  Map & m_map;

  std::map<int32_t, GameState> m_states;
};


#endif
