#include <iostream>
#include <cmath>
#include <algorithm>
#include <functional>

#include "cmdlineoptions.h"
#include "gamestatemanager.h"
#include "packetcrafter.h"
#include "map.h"
#include "filereader.h"
#include "constants.h"

#include <zlib.h>


GameStateManager::GameStateManager(std::function<void(unsigned int)> sleep, ConnectionManager & connection_manager, Map & map)
  : sleepMilli(sleep), m_connection_manager(connection_manager), m_map(map), m_states()
{
}

void GameStateManager::update(int32_t eid)
{
  if (m_connection_manager.findConnectionByEID(eid) == m_connection_manager.connections().end())
  {
    std::cout << "Client #" << eid << " no longer connected, cleaning up..." << std::endl;

    {
      std::lock_guard<std::recursive_mutex> lock(m_connection_manager.m_cd_mutex);
      m_connection_manager.clientData().erase(eid);
    }

    {
      std::lock_guard<std::recursive_mutex> lock(m_connection_manager.m_pending_mutex);
      auto it = std::find(m_connection_manager.pendingEIDs().begin(), m_connection_manager.pendingEIDs().end(), eid);
      if (it != m_connection_manager.pendingEIDs().end())
      {
        m_connection_manager.pendingEIDs().erase(it);
      }
    }

    // Note: We should really rework the m_states container. It should be
    // of type std::unordered_map<int32_t, std::shared_ptr<GameState>>.
    // That way, access to individual states can be done via copies of
    // shared pointers, and we don't need to hold the lock for the entirety
    // of a length operation like sendMoreChunksToPlayer().

    {
      std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);
      m_states.erase(eid);
    }

    return;
  }

  auto it = m_states.find(eid);
  if (it != m_states.end() && it->second.state == GameState::TERMINATED)
  {
    std::cout << "Client #" << eid << " should leave, closing connection." << std::endl;
    m_connection_manager.stop(eid);
    m_states.erase(it);
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

void GameStateManager::sendToAll(std::function<void(int32_t)> f)
{
  std::list<int32_t> todo;

  {
    std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);
    for (auto it = m_states.begin(); it != m_states.end(); ++it)
      todo.push_back(it->first);
  }

  for (auto it = todo.begin(); it != todo.end(); ++it)
    f(*it);
}

void GameStateManager::sendMoreChunksToPlayer(int32_t eid)
{
  std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);

  auto it = m_states.find(eid);
  if (it == m_states.end()) return;

  GameState & player = it->second;

  std::vector<ChunkCoords> ac = ambientChunks(getChunkCoords(player.position), PLAYER_CHUNK_HORIZON);
  std::sort(ac.begin(), ac.end(), L1DistanceFrom(getChunkCoords(player.position)));

  for (auto i = ac.begin(); i != ac.end(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;
    std::cout << "Player #" << std::dec << eid << " needs chunk " << *i << "." << std::endl;
    m_map.ensureChunkIsReadyForImmediateUse(*i);
  }
  for (auto i = ac.begin(); i != ac.end(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;
    std::cout << "Computing light spread... ";
    m_map.chunk(*i).spreadAllLight(m_map);
    std::cout << " ...done." << std::endl;
  }
  for (auto i = ac.begin(); i != ac.end(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;

    Chunk & chunk = m_map.chunk(*i);

    // Not sure if the client has a problem with data coming in too fast...
    sleepMilli(5);

#define USE_ZCACHE 1
#if USE_ZCACHE > 0
    // This is using a chunk-local zip cache.
    std::pair<const unsigned char *, size_t> p = chunk.compress_beefedup();
    packetSCPreChunk(eid, *i, true);

    if (p.second > 18)
      packetSCMapChunk(eid, p);
    else
      packetSCMapChunk(eid, *i, chunk.compress());
#else
    // This is the safe way.
    packetSCPreChunk(eid, *i, true);
    packetSCMapChunk(eid, *i, chunk.compress());
#endif
#undef USE_ZCACHE

    player.known_chunks.insert(*i);
  }
}



/******************                  ****************
 ******************  Packet Handlers ****************
 ******************                  ****************/


void GameStateManager::packetCSKeepAlive(int32_t eid)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received KeepAlive from #" << eid << std::endl;
  packetSCKeepAlive(eid);
}

void GameStateManager::packetCSChunkRequest(int32_t eid, int32_t X, int32_t Z, bool mode)
{
  //if (PROGRAM_OPTIONS.count("verbose"))
    std::cout << "GSM: Received ChunkRequest from #" << eid << ": [" << X << ", " << Z << ", " << mode << "]" << std::endl;
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
    m_states[eid].position = WorldCoords(X, Y, Z);
  }

  else
  {
    WorldCoords wc(X, Y, Z);
    if (getChunkCoords(m_states[eid].position) != getChunkCoords(wc))
    {
      m_states[eid].position = wc;
      sendMoreChunksToPlayer(eid);
    }
  }

}

void GameStateManager::packetCSPlayerDigging(int32_t eid, int32_t X, uint8_t Y, int32_t Z, uint8_t status, uint8_t face)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerDigging from #" << std::dec << eid << ": [" << X << ", " << (unsigned int)(Y)
            << ", " << Z << ", " << (unsigned int)(status) << ", " << (unsigned int)(face) << "]" << std::endl;

  if (status == 2)
  {
    const WorldCoords wc(X, Y, Z);
    Chunk & chunk = m_map.chunk(wc);
    chunk.blockType(getLocalCoords(wc)) = BLOCK_Air;
    chunk.taint();

    sendToAll(MAKE_SIGNED_CALLBACK(packetSCBlockChange, (int32_t, const WorldCoords &, int8_t, int8_t), wc, BLOCK_Air, 0));
  }
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

    if (!PROGRAM_OPTIONS["testfile"].as<std::string>().empty())
    {
      RegionFile f(PROGRAM_OPTIONS["testfile"].as<std::string>());
      f.parse();

      std::vector<ChunkCoords> ac, bc;

      for (size_t x = 0; x < 32; ++x)
        for (size_t z = 0; z < 32; ++z)
          if (f.chunkSize(x, z) != 0) ac.push_back(ChunkCoords(x, z));

      /// Load all available chunks to memory, but only send the first 50 to the client.

      WorldCoords start_pos(70, 70, 70);

      std::sort(ac.begin(), ac.end(), L1DistanceFrom(getChunkCoords(start_pos))); // L1-sorted by distance from centre.
      size_t counter = 0;

      for (auto i = ac.begin(); i != ac.end(); ++i, ++counter)
      {
        std::string chuck = f.getCompressedChunk(cX(*i), cZ(*i));
        if (chuck == "") continue;

        auto c = NBTExtract(reinterpret_cast<const unsigned char*>(chuck.data()), chuck.length(), *i);
        m_map.insertChunk(c);

        if (counter < 120)
        {
          bc.push_back(*i);
        }
      }
      for (auto i = bc.begin(); i != bc.end(); ++i, ++counter)
      {
        m_map.ensureChunkIsReadyForImmediateUse(*i);
      }
      for (auto i = bc.begin(); i != bc.end(); ++i, ++counter)
      {
        m_map.chunk(*i).spreadAllLight(m_map);
        // Not sure if the client has a problem with data coming in too fast...
        sleepMilli(10);

        std::pair<const unsigned char *, size_t> p = c->compress_beefedup();
        packetSCPreChunk(eid, *i, true);

        if (p.second > 18)
          packetSCMapChunk(eid, p);
        else
          packetSCMapChunk(eid, *i, c->compress());
      }

      m_states[eid].state = GameState::READYTOSPAWN;

      packetSCSpawn(eid, start_pos);
      packetSCPlayerPositionAndLook(eid, wX(start_pos), wY(start_pos), wZ(start_pos), wY(start_pos) + 1.6, 0.0, 0.0, false);
    }
    else
    {
      GameState & player = m_states[eid];

      WorldCoords start_pos(8, 80, 8);

      player.position = start_pos;

      sendMoreChunksToPlayer(eid);

      m_states[eid].state = GameState::READYTOSPAWN;

      packetSCSpawn(eid, start_pos);
      packetSCPlayerPositionAndLook(eid, wX(start_pos), wY(start_pos), wZ(start_pos), wY(start_pos) + 1.6, 0.0, 0.0, true);

      packetSCSetSlot(eid, 0, 37, ITEM_DiamondPickaxe, 1, 0);
      packetSCSetSlot(eid, 0, 36, BLOCK_Torch, 50, 0);
      packetSCSetSlot(eid, 0, 29, ITEM_Coal, 50, 0);
      packetSCSetSlot(eid, 0, 30, BLOCK_Wood, 50, 0);
      packetSCSetSlot(eid, 0, 38, ITEM_DiamondShovel, 1, 0);
      packetSCSetSlot(eid, 0, 39, BLOCK_BrickBlock, 64, 0);
      packetSCSetSlot(eid, 0, 40, BLOCK_Stone, 64, 0);
      packetSCSetSlot(eid, 0, 41, BLOCK_Glass, 64, 0);
      packetSCSetSlot(eid, 0, 42, BLOCK_WoodenPlank, 64, 0);
      packetSCSetSlot(eid, 0, 43, ITEM_Bucket, 1, 0);
    }
  }
}

void GameStateManager::packetCSBlockPlacement(int32_t eid, int32_t X, int8_t Y, int32_t Z, int8_t direction, int16_t block_id, int8_t amount, int16_t damage)
{
  //if (PROGRAM_OPTIONS.count("verbose"))
  std::cout << "GSM: Received BlockPlacement from #" << std::dec << eid << ": [" << X << ", " << int(Y) << ", " << Z << ", "
            << Direction(direction) << ", " << block_id << ", " << int(amount) << ", " << damage << "]" << std::endl;

  auto it = std::find(BLOCKITEM_INFO.begin(), BLOCKITEM_INFO.end(), block_id);

  if (X == -1 && Y == -1 && Z == -1)
  {
    std::cout << "   Player " << eid << " slashes thin air ";
    if (it != BLOCKITEM_INFO.end()) std::cout << "with " << it->name;
    std::cout << std::endl;
  }

  else if (block_id < 0) return /* empty-handed */ ;

  else if (it != BLOCKITEM_INFO.end() && it->type() == BlockItemInfo::BLOCK)
  {
    WorldCoords wc(X, Y, Z);
    wc += Direction(direction);
    if (m_map.haveChunk(getChunkCoords(wc)))
    {
      Chunk & chunk = m_map.chunk(getChunkCoords(wc));
      chunk.blockType(getLocalCoords(wc)) = block_id;
      chunk.taint();

      sendToAll(MAKE_SIGNED_CALLBACK(packetSCBlockChange, (int32_t, const WorldCoords &, int8_t, int8_t), wc, block_id, 0));

      /*
        I used to have this:

  template<typename ... Args>
  void GameStateManager::sendToAll(std::function<void(int32_t, Args ...)> f, Args && ... args)
  {
    // [... loop ...]
    f(*it, std::forward<Args>(args)...);
  }

        Used like this:

      using namespace std::placeholders;
      //void (GameStateManager::*f)(int32_t, const WorldCoords &, int8_t, int8_t) = &GameStateManager::packetSCBlockChange;
      //sendToAll(std::function<void(int32_t, const WorldCoords &, int8_t, int8_t)>(std::bind(f, this, _1, _2, _3, _4)), (const WorldCoords &)(wc), int8_t(block_id), int8_t(0));

        Or even (jikes!) this:


      /// I don't know if I should feel victorious about this construct.
      sendToAll(std::function<void(int32_t, const WorldCoords &, int8_t, int8_t)>(
          std::bind((void(GameStateManager::*)(int32_t, const WorldCoords &, int8_t, int8_t))(&GameStateManager::packetSCBlockChange), this, _1, _2, _3, _4)
        ), (const WorldCoords &)(wc), int8_t(block_id), int8_t(0));
      */


    }
  }
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
  m_connection_manager.sendDataToClient(eid, p.craft(), "<keep alive>");
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

void GameStateManager::packetSCSpawn(int32_t eid, int32_t X, int32_t Y, int32_t Z)
{
  PacketCrafter p(PACKET_SPAWN_POSITION);
  p.addInt32(X);    // X
  p.addInt32(Y);    // Y
  p.addInt32(Z);    // Z
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool on_ground)
{
  PacketCrafter p(PACKET_PLAYER_POSITION_AND_LOOK);
  p.addDouble(X);       // X
  p.addDouble(Y);       // Y
  p.addDouble(stance);  // stance
  p.addDouble(Z);       // Z
  p.addFloat(yaw);      // yaw
  p.addFloat(pitch);    // pitch
  p.addBool(on_ground); // on ground
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCSetSlot(int32_t eid, int8_t window, int16_t slot, int16_t item, int8_t count, int16_t uses)
{
  PacketCrafter p(PACKET_SET_SLOT);
  p.addInt8(window);  // window #, 0 = inventory
  p.addInt16(slot);   // slot #
  p.addInt16(item);   // item id
  p.addInt8(count);   // item count
  p.addInt16(uses);   // uses
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCBlockChange(int32_t eid, int32_t X, int8_t Y, int32_t Z, int8_t block_type, int8_t block_md)
{
  std::cout << "Sending BlockChange to #" << std::dec << eid << ": [" << X << ", " << int(Y) << ", "
            << Z << ", block type " << int(block_type) << ", block md " << int(block_md) << "]" << std::endl;

  PacketCrafter p(PACKET_BLOCK_CHANGE);
  p.addInt32(X);         // X
  p.addInt8(Y);          // Y
  p.addInt32(Z);         // Z
  p.addInt8(block_type); // block type
  p.addInt8(block_md);   // block metadata
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCTime(int32_t eid, int64_t ticks)
{
  PacketCrafter p(PACKET_TIME_UPDATE);
  p.addInt64(ticks);
  m_connection_manager.sendDataToClient(eid, p.craft());
}
