#include <iostream>
#include <cmath>
#include <algorithm>

#include "cmdlineoptions.h"
#include "gamestatemanager.h"
#include "packetcrafter.h"
#include "map.h"

GameStateManager::GameStateManager(ConnectionManager & connection_manager, Map & map)
  : m_connection_manager(connection_manager), m_map(map), m_states()
{
}

void GameStateManager::update(int32_t eid)
{
  if (m_connection_manager.findConnectionByEID(eid) == m_connection_manager.connections().end())
  {
    std::cout << "Client #" << eid << " no longer connected, cleaning up..." << std::endl;

    std::unique_lock<std::recursive_mutex> lock(m_connection_manager.m_cd_mutex);

    auto it = m_connection_manager.clientData().find(eid);
    if (it != m_connection_manager.clientData().end())
    {
      m_connection_manager.clientData().erase(it);
    }

    return;
  }

  auto it = m_states.find(eid);
  if (it != m_states.end() && it->second.state == GameState::TERMINATED)
  {
    std::cout << "Client #" << eid << " should leave, closing connection." << std::endl;
    m_connection_manager.stop(eid);
  }

}

struct L1DistanceFrom
{
  L1DistanceFrom(const ChunkCoords & cc) : cc(cc) { }
  inline bool operator()(const ChunkCoords & a, const ChunkCoords & b) const
  {
    return std::abs(cX(a) - cX(cc)) + std::abs(cZ(a) - cZ(cc)) < std::abs(cX(b) - cX(cc)) + std::abs(cZ(b) - cZ(cc));
  }
  ChunkCoords cc;
};

void GameStateManager::packetCSKeepAlive(int32_t eid)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received KeepAlive from #" << eid << std::endl;
  packetSCKeepAlive(eid);
}

void GameStateManager::packetCSUseEntity(int32_t eid, int32_t e, int32_t target, bool leftclick)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received UseEntity from #" << eid << ": [" << e << ", " << target << ", " << leftclick << "]" << std::endl;
}

void GameStateManager::packetCSPlayer(int32_t eid, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerOnGround from #" << eid << ": " << ground << std::endl;
}

void GameStateManager::packetCSPlayerPosition(int32_t eid, double X, double Y, double Z, double stance, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerPosition from #" << eid << ": [" << X << ", " << Y << ", " << Z << ", " << stance << ", " << ground << "]" << std::endl;
}

void GameStateManager::packetCSPlayerLook(int32_t eid, float yaw, float pitch, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerPosition from #" << eid << ": [" << yaw << ", " << pitch << ", " << ground << "]" << std::endl;
}

void GameStateManager::packetCSPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerPosition from #" << eid << ": [" << X << ", " << Y << ", " << Z << ", "
            << stance << ", " << yaw << ", " << pitch << ", " << ground << "]" << std::endl;

  if (m_states[eid].state == GameState::READYTOSPAWN)
  {
    PacketCrafter p(PACKET_PLAYER_POSITION_AND_LOOK);
    p.addDouble(X);      // X
    p.addDouble(Y);      // Y
    p.addDouble(stance); // stance
    p.addDouble(Z);      // Z
    p.addFloat(yaw);     // yaw
    p.addFloat(pitch);   // pitch
    p.addBool(ground);   // on ground
    m_connection_manager.sendDataToClient(eid, p.craft());

    m_states[eid].state = GameState::SPAWNED;
  }

}

void GameStateManager::packetCSPlayerDigging(int32_t eid, int32_t X, uint8_t Y, int32_t Z, uint8_t status, uint8_t face)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerDigging from #" << std::dec << eid << ": [" << X << ", " << (unsigned int)(Y)
            << ", " << Z << ", " << (unsigned int)(status) << ", " << (unsigned int)(face) << "]" << std::endl;
}

void GameStateManager::packetCSHoldingChange(int32_t eid, int16_t slot)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received HoldingChange from #" << std::dec << eid << ": " << slot << std::endl;
}

void GameStateManager::packetCSArmAnimation(int32_t eid, int32_t e, int8_t animate)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Animation from #" << std::dec << eid << ": [" << e << ", " << (unsigned int)(animate) << "]" << std::endl;
}

void GameStateManager::packetCSEntityCrouchBed(int32_t eid, int32_t e, int8_t action)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received CrouchBed from #" << std::dec << eid << ": [" << e << ", " << (unsigned int)(action) << "]" << std::endl;
}

void GameStateManager::packetCSPickupSpawn(int32_t eid, int32_t e, int32_t X, int32_t Y, int32_t Z,
                                           double rot, double pitch, double roll, int8_t count, int16_t item, int16_t data)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PickupSpawn from #" << std::dec << eid << ": [" << e << ", " << X << ", " << Y << ", " << Z << ", "
            << rot << ", " << pitch << ", " << roll << ", " << (unsigned int)(count) << ", " << item << ", " << data << ", "<< "]" << std::endl;
}

void GameStateManager::packetCSRespawn(int32_t eid)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Respawn from #" << std::dec << eid << std::endl;
}

void GameStateManager::packetCSCloseWindow(int32_t eid, int8_t window_id)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received CloseWindow from #" << std::dec << eid << ": " << (unsigned int)(window_id) << std::endl;
}

void GameStateManager::packetCSHandshake(int32_t eid, const std::string & name)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Handshake from #" << std::dec << eid << ": " << name << std::endl;

  Connection * c = m_connection_manager.findConnectionByEIDwp(eid);
  if (c) c->nick() = name;

  auto it = m_states.find(eid);

  if (it != m_states.end())
  {
    std::cout << "GSM: Error, received handshake from a client that is already connected." << std::endl;
    packetSCKick(eid, "Extraneous handshake received!");
    m_connection_manager.stop(eid);
    it->second.state = GameState::TERMINATED;
    return;
  }

  m_states.insert(std::pair<int32_t, GameState>(eid, GameState(GameState::PRELOGIN)));

  PacketCrafter p(PACKET_HANDSHAKE);
  p.addJString("-");

  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetCSLoginRequest(int32_t eid, int32_t protocol_version, const std::string & username, const std::string & password, int64_t map_seed, int8_t dimension)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received LoginRequest from #" << std::dec << eid << ": ["
            << std::dec << protocol_version << ", "
            << username<< ", "
            << password << ", "
            << map_seed << ", "
            << int(dimension)
            << "]" << std::endl;

  Connection * c = m_connection_manager.findConnectionByEIDwp(eid);
  if (c)
  {
    if (c->nick() != username)
    {
      std::cout << "Username differs from the nickname we received earlier! Aborting." << std::endl;
      packetSCKick(eid, "Username mismatch!");
      m_connection_manager.stop(eid);
      return;
    }

    {
      PacketCrafter p(PACKET_LOGIN_REQUEST);
      p.addInt32(eid);
      p.addJString("");
      p.addJString("");
      p.addInt64(12345);
      p.addInt8(0);
      m_connection_manager.sendDataToClient(eid, p.craft());
    }

    m_states[eid].state = GameState::POSTLOGIN;
    
    std::vector<ChunkCoords> ac = ambientChunks(ChunkCoords(0, 0), 4);    // 9x9 around the current chunk
    std::sort(ac.begin(), ac.end(), L1DistanceFrom(ChunkCoords(0, 0))); // L1-sorted by distance from centre.

    for (auto i = ac.begin(); i != ac.end(); ++i)
    {
      std::cout << "Need chunk [" << std::dec << cX(*i) << ", " << cZ(*i) << "]." << std::endl;
      const Chunk & c = m_map.getChunkOrGnerateNew(*i);

      packetSCPreChunk(eid, *i, true);
      packetSCMapChunk(eid, *i, c.compress());
    }

    m_states[eid].state = GameState::READYTOSPAWN;

    {
      PacketCrafter p(PACKET_SPAWN_POSITION);
      p.addInt32(8);    // X
      p.addInt32(100);  // Y
      p.addInt32(8);    // Z
      m_connection_manager.sendDataToClient(eid, p.craft());
    }

    {
      PacketCrafter p(PACKET_PLAYER_POSITION_AND_LOOK);
      p.addDouble(8.0);    // X
      p.addDouble(100.0);  // Y
      p.addDouble(101.6);  // stance
      p.addDouble(8.0);    // Z
      p.addFloat(0.0);     // yaw
      p.addFloat(0.0);     // pitch
      p.addBool(false);     // on ground
      m_connection_manager.sendDataToClient(eid, p.craft());
    }

  }
}

void GameStateManager::packetCSBlockPlacement(int32_t eid, int32_t X, uint8_t Y, int32_t Z, int8_t direction, int16_t block_id, int8_t amount, int16_t damage)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received BlockPlacement from #" << std::dec << eid << ": [" << X << ", " << Y << ", " << Z << ", "
            << direction << ", " << block_id << ", " << amount << ", " << damage << "]" << std::endl;
}

void GameStateManager::packetCSChatMessage(int32_t eid, std::string message)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received ChatMessage from #" << std::dec << eid << ": \"" << message << "\"" << std::endl;
}

void GameStateManager::packetCSDisconnect(int32_t eid, std::string message)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Disconnect from #" << std::dec << eid << ": \"" << message << "\"" << std::endl;
  m_connection_manager.stop(eid);
  m_states[eid].state = GameState::TERMINATED;
}

void GameStateManager::packetCSWindowClick(int32_t eid, int8_t window_id, int16_t slot, int8_t right_click, int16_t action, int16_t item_id, int8_t item_count, int16_t item_uses)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received WindowClick from #" << std::dec << eid << ": [" << (unsigned int)(window_id) << ", " << slot << ", " << (unsigned int)(right_click) << ", "
            << action << ", " << item_id << ", " << (unsigned int)(item_count) << ", " << item_uses << "]" << std::endl;
}

void GameStateManager::packetCSSign(int32_t eid, int32_t X, int16_t Y, int32_t Z, std::string line1, std::string line2, std::string line3, std::string line4)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Sign from #" << std::dec << eid << ": [" << X << ", " << Y << ", " << Z << ", \"" << line1 << "\", " << line2 << "\", " << line3 << "\", " << line4 << "]" << std::endl;
}


void GameStateManager::packetSCKeepAlive(int32_t eid)
{
  PacketCrafter p(PACKET_KEEP_ALIVE);
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCKick(int32_t eid, const std::string & message)
{
  PacketCrafter p(PACKET_DISCONNECT);
  p.addJString(message);
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCPreChunk(int32_t eid, const ChunkCoords & cc, bool mode)
{
  PacketCrafter q(PACKET_PRE_CHUNK);
  q.addInt32(cX(cc)); // cX
  q.addInt32(cZ(cc)); // cZ
  q.addBool(mode);    // Mode (true = initialize, false = unload)
  m_connection_manager.sendDataToClient(eid, q.craft());
}

void GameStateManager::packetSCMapChunk(int32_t eid, int32_t X, int32_t Y, int32_t Z, const std::string & data, size_t sizeX, size_t sizeY, size_t sizeZ)
{
  PacketCrafter p(PACKET_MAP_CHUNK);
  p.addInt32(X);    // wX
  p.addInt16(Y);    // wY
  p.addInt32(Z);    // wZ
  p.addInt8(sizeX);
  p.addInt8(sizeY);
  p.addInt8(sizeZ);
  p.addInt32(data.length());
  p.addByteArray(data.data(), data.length());
  m_connection_manager.sendDataToClient(eid, p.craft());
}
