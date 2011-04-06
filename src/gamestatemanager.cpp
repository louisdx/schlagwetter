#include <iostream>
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
    m_connection_manager.clientData().erase(eid);
    m_connection_manager.clientEgressQ().erase(eid);
    return;
  }

  auto it = m_states.find(eid);
  if (it != m_states.end() && it->second.state == GameState::TERMINATED)
  {
    std::cout << "Client #" << eid << " should leave, closing connection." << std::endl;
    m_connection_manager.stop(eid);
  }

}

void GameStateManager::packetCSKeepAlive(int32_t eid)
{
  std::cout << "GSM: Received KeepAlive from #" << eid << std::endl;
  PacketCrafter p(PACKET_KEEP_ALIVE);
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetCSUseEntity(int32_t eid, int32_t e, int32_t target, bool leftclick)
{
  std::cout << "GSM: Received UseEntity from #" << eid << ": [" << e << ", " << target << ", " << leftclick << "]" << std::endl;
}

void GameStateManager::packetCSPlayer(int32_t eid, bool ground)
{
  std::cout << "GSM: Received PlayerOnGround from #" << eid << ": " << ground << std::endl;
}

void GameStateManager::packetCSPlayerPosition(int32_t eid, double X, double Y, double Z, double stance, bool ground)
{
  std::cout << "GSM: Received PlayerPosition from #" << eid << ": [" << X << ", " << Y << ", " << Z << ", " << stance << ", " << ground << "]" << std::endl;
}

void GameStateManager::packetCSPlayerLook(int32_t eid, float yaw, float pitch, bool ground)
{
  std::cout << "GSM: Received PlayerPosition from #" << eid << ": [" << yaw << ", " << pitch << ", " << ground << "]" << std::endl;
}

void GameStateManager::packetCSPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool ground)
{
  std::cout << "GSM: Received PlayerPosition from #" << eid << ": [" << X << ", " << Y << ", " << Z << ", "
            << stance << ", " << yaw << ", " << pitch << ", " << ground << "]" << std::endl;
}

void GameStateManager::packetCSPlayerDigging(int32_t eid, int32_t X, uint8_t Y, int32_t Z, uint8_t status, uint8_t face)
{
  std::cout << "GSM: Received PlayerDigging from #" << eid << ": [" << X << ", " << (unsigned int)(Y) << ", " << Z << ", "
            << (unsigned int)(status) << ", " << (unsigned int)(face) << "]" << std::endl;
}

void GameStateManager::packetCSHoldingChange(int32_t eid, int16_t slot)
{

}

void GameStateManager::packetCSArmAnimation(int32_t eid, int32_t e, int8_t animate)
{
  std::cout << "GSM: Received Animation from #" << eid << ": [" << e << ", " << (unsigned int)(animate) << "]" << std::endl;
}

void GameStateManager::packetCSEntityCrouchBed(int32_t eid, int32_t e, int8_t action)
{

}

void GameStateManager::packetCSPickupSpawn(int32_t eid, int32_t e, int32_t X, int32_t Y, int32_t Z,
                                           int8_t rotp, int8_t pitchp, int8_t rollp, int8_t count, int16_t item, int16_t data)
{

}

void GameStateManager::packetCSRespawn(int32_t eid)
{

}

void GameStateManager::packetCSCloseWindow(int32_t eid, int8_t window_id)
{

}

void GameStateManager::packetCSHandshake(int32_t eid, const std::string & name)
{
  std::cout << "GSM: Received Handshake from #" << eid << ": " << name << std::endl;

  Connection * c = m_connection_manager.findConnectionByEIDwp(eid);
  if (c) c->nick() = name;

  auto it = m_states.find(eid);

  if (it != m_states.end())
  {
    std::cout << "GSM: Error, received handshake from a client that is already connected." << std::endl;
    PacketCrafter p(PACKET_DISCONNECT);
    p.addJString("Extraneous handshake received!");
    m_connection_manager.sendDataToClient(eid, p.craft());
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
  std::cout << "GSM: Received LoginRequest from #" << eid << ": ["
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
      PacketCrafter p(PACKET_DISCONNECT);
      p.addJString("Username mismatch!");
      m_connection_manager.sendDataToClient(eid, p.craft());
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
    
    {
      PacketCrafter p(PACKET_PRE_CHUNK);
      p.addInt32(0);    // X
      p.addInt32(0);    // Z
      p.addBool(true);  // Mode
      m_connection_manager.sendDataToClient(eid, p.craft());
    }

    std::list<ChunkCoords> ac = ambientChunks(ChunkCoords(0, 0), 4); // 9x9 around the current chunk
    for (auto i = ac.begin(); i != ac.end(); ++i)
      std::cout << "Need chunk [" << std::dec << cX(*i) << ", " << cZ(*i) << "]." << std::endl;

    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(0, 0));
    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(0, 16));
    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(16, 16));
    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(16, 0));
    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(16, -16));
    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(0, -16));
    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(-16, -16));
    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(-16, 0));
    m_connection_manager.sendDataToClient(eid, makeRandomChunkPacket(-16, 16));

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
      p.addBool(true);     // on ground
      m_connection_manager.sendDataToClient(eid, p.craft());
    }
  }
}

void GameStateManager::packetCSBlockPlacement(int32_t eid, int32_t X, uint8_t Y, int32_t Z, int8_t direction, int16_t block_id, int8_t amount, int16_t damage)
{
  std::cout << "GSM: Received BlockPlacement from #" << eid << ": [" << X << ", " << Y << ", " << Z << ", "
            << direction << ", " << block_id << ", " << amount << ", " << damage << "]" << std::endl;
}

void GameStateManager::packetCSChatMessage(int32_t eid, std::string message)
{
  std::cout << "GSM: Received ChatMessage from #" << eid << ": \"" << message << "\"" << std::endl;
}

void GameStateManager::packetCSDisconnect(int32_t eid, std::string message)
{
  std::cout << "GSM: Received Disconnect from #" << eid << ": \"" << message << "\"" << std::endl;
  m_connection_manager.stop(eid);
  m_states[eid].state = GameState::TERMINATED;
}

void GameStateManager::packetCSWindowClick(int32_t eid, int8_t window_id, int16_t slot, int8_t right_click, int16_t action, int16_t item_id, int8_t item_count, int16_t item_uses)
{

}

void GameStateManager::packetCSSign(int32_t eid, int32_t X, int16_t Y, int32_t Z, std::string line1, std::string line2, std::string line3, std::string line4)
{

}

