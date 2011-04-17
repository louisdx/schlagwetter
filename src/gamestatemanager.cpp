#include <iostream>
#include <cmath>
#include <algorithm>
#include <functional>

#include "cmdlineoptions.h"
#include "gamestatemanager.h"
#include "packetcrafter.h"
#include "map.h"
#include "filereader.h"
#include "lua.h"


int32_t EID_POOL = 0;

int32_t GenerateEID() { return ++EID_POOL; }


uint8_t PlayerState::getRelativeDirection(const RealCoords & rc)
{
  // We probably need fractional coordinates here for precision.

  enum { BLOCK_BOTTOM = 0, BLOCK_NORTH = 1, BLOCK_SOUTH = 2, BLOCK_EAST = 3, BLOCK_WEST = 4, BLOCK_TOP = 5 };

  const double diffX = rX(rc) - rX(position);
  const double diffZ = rZ(rc) - rZ(position);

  std::cout << "Relative: " << diffX << ", " << diffZ << std::endl;

  if (diffX > diffZ)
  {
    // We compare on the x axis
    if (diffX > 0)  return BLOCK_BOTTOM;
    else            return BLOCK_EAST;
  }
  else
  {
    // We compare on the z axis
    if (diffZ > 0) return BLOCK_SOUTH;
    else           return BLOCK_NORTH;
  }
}



GameStateManager::GameStateManager(std::function<void(unsigned int)> sleep, ConnectionManager & connection_manager, Map & map)
  : sleepMilli(sleep), m_connection_manager(connection_manager), m_map(map), m_states()
{
}

void GameStateManager::update(int32_t eid)
{
  if (!m_connection_manager.hasConnection(eid))
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
    // of type std::unordered_map<int32_t, std::shared_ptr<PlayerState>>.
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
  if (it != m_states.end() && it->second.state == PlayerState::TERMINATED)
  {
    std::cout << "Client #" << eid << " should leave, closing connection." << std::endl;
    m_connection_manager.safeStop(eid);
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

  PlayerState & player = it->second;

  // Someone can go and implement more overloads if this looks too icky.
  const ChunkCoords pc = getChunkCoords(getWorldCoords(getFractionalCoords(player.position)));

  std::vector<ChunkCoords> ac = ambientChunks(pc, PLAYER_CHUNK_HORIZON);
  std::sort(ac.begin(), ac.end(), L1DistanceFrom(pc));

  /// Here follows the typical chunk update acrobatics in three rounds.

  // Round 1: Load all relevant chunks to memory

  for (auto i = ac.begin(); i != ac.end(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;

    //std::cout << "Player #" << std::dec << eid << " needs chunk " << *i << "." << std::endl;
    m_map.ensureChunkIsReadyForImmediateUse(*i);
  }

  // Round 2: Spread light to all chunks in memory. Light only spreads to loaded chunks.

  for (auto i = ac.begin(); i != ac.end(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;

    m_map.chunk(*i).spreadAllLight(m_map);
    m_map.chunk(*i).spreadToNewNeighbours(m_map);
  }

  // Round 3: Send the fully updated chunks to the client.

  for (auto i = ac.begin(); i != ac.end(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;

    Chunk & chunk = m_map.chunk(*i);

    // Not sure if the client has a problem with data coming in too fast...
    sleepMilli(5);

#define USE_ZCACHE 0   // The local chache doesn't seem to work reliably.
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



bool isStackable(EBlockItem e)
{
  // See below; I don't think this is an official feature.

  switch (e)
    {
    case BLOCK_CraftingTable:
    case BLOCK_ChestBlock:
    case BLOCK_Jukebox:
    case BLOCK_Torch:
    case BLOCK_RedstoneTorchOff:
    case BLOCK_RedstoneTorchOn:
    case BLOCK_RedstoneWire:
    case BLOCK_Water:
    case BLOCK_StationaryWater:
    case BLOCK_Lava:
    case BLOCK_StationaryLava:
    case BLOCK_Air:
    case BLOCK_Rails:
    case BLOCK_WoodenDoor:
    case BLOCK_IronDoor:
    case BLOCK_Ice:
    case BLOCK_Cake:
    case BLOCK_Bed:
      return false;

    default:
      return true;
    }
}


/// The following two work functions could possibly be relegated to an embedded scripting engine.

/// This is the workhorse for digging (left-click) decisions.

void GameStateManager::reactToSuccessfulDig(const WorldCoords & wc, EBlockItem block_type)
{
  // this is just temporary
  std::cout << "Successfully dug at " << wc << " for " << BLOCKITEM_INFO.find(block_type)->second.name << std::endl;

  if (block_type != 0)
  {
    sendToAll(MAKE_CALLBACK(packetSCPickupSpawn, GenerateEID(), uint16_t(block_type), 1, 0, wc));

#if USE_LUA > 0
    try
    {
      luabind::call_function<void>(LUA, "digHandler", wX(wc), wY(wc), wZ(wc), int(block_type));
    }
    catch (const luabind::error & e)
    {
      luabind::object error_msg(luabind::from_stack(e.state(), -1));
      std::cout << "Lua: luabind error: " << error_msg << std::endl;
    }
    catch (...)
    {
      std::cout << "Lua: generic error during invocation of digHandler()." << std::endl;
    }
#endif
  }
}

/// This is the workhorse for block placement (right-click) decisions.

GameStateManager::EBlockPlacement GameStateManager::blockPlacement(int32_t eid,
    const WorldCoords & wc, Direction dir, BlockItemInfoMap::const_iterator it, uint8_t & meta)
{
  // I believe we are never allowed to place anything on an already occupied block.
  // If that's false, we have to refactor this check.

  if (!m_map.haveChunk(getChunkCoords(wc + dir)) ||
      EBlockItem(m_map.chunk(getChunkCoords(wc + dir)).blockType(getLocalCoords(wc + dir))) != BLOCK_Air)
  {
    std::cout << "Sorry, cannot place object on occupied block at " << wc + dir << "." << std::endl;
    return CANNOT_PLACE;
  }

  // Likewise, we exclude globally right-clicks on banned block types, aka non-stackables.
  // Actually, I don't think the official server has such a rule. In fact, I think when the
  // client sends a PLACEMENT packet, it already expects the placement to be legal.
  // We're not expected to tell the client off. Oh well.

  if (!m_map.haveChunk(getChunkCoords(wc)) ||
      !isStackable(EBlockItem(m_map.chunk(getChunkCoords(wc)).blockType(getLocalCoords(wc)))) )
  {
    std::cout << "Sorry, cannot place object on non-stackable block at " << wc << "." << std::endl;
    return CANNOT_PLACE;
  }

  // Target block is clear and source block is stackable, let's get to work.

  switch(it->first) // block ID
  {
  case BLOCK_WoodenStairs:
  case BLOCK_CobblestoneStairs:
    {
      std::cout << "Special block: #" << eid << " is trying to place stairs." << std::endl;

      if (wY(wc) == 0 || dir == BLOCK_YMINUS) return CANNOT_PLACE;

      meta = m_states[eid].getRelativeDirection(wc + dir);
      return OK_WITH_META;
    }

  case BLOCK_Torch:
  case BLOCK_RedstoneTorchOff:
  case BLOCK_RedstoneTorchOn:
    {
      std::cout << "Special block: #" << eid << " is trying to place a torch." << std::endl;

      switch (int(dir))
      {
      case 2: meta = 4; break;
      case 3: meta = 3; break;
      case 4: meta = 2; break;
      case 5: meta = 1; break;
      default: meta = 5; break;
      }
                          
      return OK_WITH_META;
    }

  default:
    return OK_NO_META;
  }

  return OK_NO_META;
}



/******************                  ****************
 ******************  Packet Handlers ****************
 ******************                  ****************/


void GameStateManager::packetCSPlayerDigging(int32_t eid, int32_t X, uint8_t Y, int32_t Z, uint8_t status, uint8_t face)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerDigging from #" << std::dec << eid << ": [" << X << ", " << (unsigned int)(Y)
            << ", " << Z << ", " << (unsigned int)(status) << ", " << (unsigned int)(face) << "]" << std::endl;

  /*** Digging, aka "the left mouse button" ***

   We check have two states (apart from the third one): Start (0) and Stop (2).

   To determine the course of action, we look up the block type at the target (X,Y,Z)
   in the property table BLOCK_DIG_PROPERTIES. We recognise three properties:

   * LEFTCLICK_DIGGABLE:  Ordinary blocks that can be removed by finishing digging.
   * LEFTCLICK_REMOVABLE: Blocks that are immediately removed, like torches and wire.
   * LEFTCLICK_TRIGGER:   Blocks that change their status when clicked, like doors and buttons.

   A block can have several properties.

   A Start event needs to treat all three properties. A Stop event only needs to treat DIGGABLE blocks.

   The third status is Drop (4), which is something entirely different.

  ***/

  if (status == 0)
  {
    const WorldCoords wc(X, Y, Z);
    Chunk & chunk = m_map.chunk(wc);
    unsigned char & block =  chunk.blockType(getLocalCoords(wc));

    const unsigned char block_properties = BLOCK_DIG_PROPERTIES[block];

    if (block_properties & LEFTCLICK_DIGGABLE)
    {
      m_states[eid].recent_dig = { wc, clockTick() };
    }

    if (block_properties & LEFTCLICK_REMOVABLE)
    {
      sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc, BLOCK_Air, 0));
      reactToSuccessfulDig(wc, EBlockItem(block));
      block = BLOCK_Air;
      chunk.taint();
    }

    if (block_properties & LEFTCLICK_TRIGGER)
    {
      // Trigger changes require meta data fiddling.
    }
  }

  else if (status == 2)
  {
    const WorldCoords wc(X, Y, Z);
    Chunk & chunk = m_map.chunk(wc);
    unsigned char & block =  chunk.blockType(getLocalCoords(wc));
    const unsigned char block_properties = BLOCK_DIG_PROPERTIES[block];

    if (block_properties & LEFTCLICK_DIGGABLE)
    {
      if (m_states[eid].recent_dig.wc == wc) // further validation needed
      {
        std::cout << "#" << eid << " spent " << (clockTick() - m_states[eid].recent_dig.start_time) << "ms digging for "
                  << BLOCKITEM_INFO.find(EBlockItem(block))->second.name << "." << std::endl;
        sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc, BLOCK_Air, 0));
        reactToSuccessfulDig(wc, EBlockItem(block));
        block = BLOCK_Air;
        chunk.taint();
      }
      else
      {
        std::cout << "Bogus \"Digging Stop\" event received from #" << eid << " (consider kicking)." << std::endl;
      }
    }
  }

  else if (status == 4 && (X == 0 && Y == 0 && Z == 0 && face == 0))
  {
    std::cout << "Item drop stub. Please implement me." << std::endl;
  }

  else
  {
    std::cout << "Bogus PlayerDigging event received from #" << eid
              << ", status = " << (unsigned int)(status) << "." << std::endl;
  }
}

void GameStateManager::packetCSBlockPlacement(int32_t eid, int32_t X, int8_t Y, int32_t Z, int8_t direction, int16_t block_id, int8_t amount, int16_t damage)
{
  //if (PROGRAM_OPTIONS.count("verbose"))
  std::cout << "GSM: Received BlockPlacement from #" << std::dec << eid << ": [" << X << ", " << int(Y) << ", " << Z << ", "
            << Direction(direction) << ", " << block_id << ", " << int(amount) << ", " << damage << "]" << std::endl;
  std::cout << "Player position is " << m_states[eid].position << std::endl;


  /*** Placement, aka "the right mouse button" ***

   We have to tackle this event in stages.

   First, if the target is (-1, -1, -1), we have to handle our inventory action (e.g. eating).
   This may also be triggered when the right click faces no valid block in range ("thin air").

   Second, otherwise, if the target is an interactive item (Crafting Table, etc.),
   we open that and ignore all else.

   Third, otherwise, the target is a genuine placement target. If we hold no item,
   i.e. block_id < 0, there's nothing to do; otherwise we place the block. This is
   the most tricky operation, as we have to determine all sorts of special-needs blocks
   that require meta data to be set up.

   ***/

  auto it = BLOCKITEM_INFO.find(EBlockItem(block_id));

  /// Stage 1: Special inventory event. Eating and such like.
  if (X == -1 && Y == -1 && Z == -1)
  {
    std::cout << "   Player " << eid << " slashes thin air ";
    if (it != BLOCKITEM_INFO.end()) std::cout << "with " << it->second.name;
    std::cout << std::endl;
  }

  else
  {
    WorldCoords wc(X, Y, Z);

    if (!m_map.haveChunk(getChunkCoords(wc)))
    {
      std::cout << "Panic, right-click went into a non-existing chunk!" << std::endl;
      return;
    }

    Chunk & chunk = m_map.chunk(getChunkCoords(wc));

    /// Stage 2: Interactive block, open window.

    if (chunk.blockType(getLocalCoords(wc)) == BLOCK_CraftingTable ||
        chunk.blockType(getLocalCoords(wc)) == BLOCK_FurnaceBlock  ||
        chunk.blockType(getLocalCoords(wc)) == BLOCK_ChestBlock    ||
        chunk.blockType(getLocalCoords(wc)) == BLOCK_DispenserBlock  )
     {
       uint8_t w = -1, slots = -1;
       std::string title = "";

       switch (chunk.blockType(getLocalCoords(wc)))
         {
         case BLOCK_CraftingTable:  w = WINDOW_CraftingTable; slots = 9;  title = "Make it so!"; break;
         case BLOCK_FurnaceBlock:   w = WINDOW_Furnace;       slots = 9;  title = "Furnace";     break;
         case BLOCK_ChestBlock:     w = WINDOW_Chest;         slots = 60; title = "Myspace";     break;
         case BLOCK_DispenserBlock: w = WINDOW_Dispenser;     slots = 9;  title = "Dispenser";   break;
         }

       packetSCOpenWindow(eid, 123 /* window ID? */, w, title, slots);
     }

    /// Stage 3a: Genuine target, but empty hands.

    else if (block_id < 0)
    {
      // empty-handed, not useful
    }

    /// Stage 3b: Genuine target, holding a placeable block.

    else if (it != BLOCKITEM_INFO.end())
    {
      wc += Direction(direction);

      // Holding a building block
      if (BlockItemInfo::type(it->first) == BlockItemInfo::BLOCK)
      {
        if (wc == getWorldCoords(m_states[eid].position)                                  ||
            (wY(wc) != 0 && wc + BLOCK_YMINUS == getWorldCoords(m_states[eid].position))   )
        {
          std::cout << "Player #" << eid << " tries to bury herself in " << it->second.name << "." << std::endl;
        }

        else if (m_map.haveChunk(getChunkCoords(wc)))
        {
          Chunk & chunk = m_map.chunk(getChunkCoords(wc));

          // Before placing the block, we have to see if we need to set magic metadata (stair directions etc.)
          uint8_t meta;
          const auto bp_res = blockPlacement(eid, WorldCoords(X, Y, Z), Direction(direction), it, meta);

          if (bp_res != CANNOT_PLACE)
          {
            chunk.blockType(getLocalCoords(wc)) = block_id;
            chunk.taint();

            if (bp_res == OK_WITH_META)
            {
              chunk.setBlockMetaData(getLocalCoords(wc), meta);
              sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc, block_id, meta));
            }
            else // OK_NO_META
            {
              sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc, block_id, 0));
            }
          }
          else // CANNOT_PLACE
          {
            // Naughty client gets slapped on the fingers.
            packetSCBlockChange(eid, wc, BLOCK_Air, 0);
          }
        }
      }

      // Holding an item
      else if (BlockItemInfo::type(it->first) == BlockItemInfo::BLOCK)
      {
        // check for seeds, sign, buckets, doors, saddles, minecarts, bed
      }
    }

    else
    {
      std::cout << "Bogus BlockPlacement event received from #" << eid << "." << std::endl;
    }
  }
}

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

  const RealCoords rc(X, Y, Z);

  if (getChunkCoords(m_states[eid].position) != getChunkCoords(rc))
  {
    sendMoreChunksToPlayer(eid);
  }

  m_states[eid].position = rc;
}

void GameStateManager::packetCSPlayerLook(int32_t eid, float yaw, float pitch, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerLook from #" << eid << ": [" << yaw << ", " << pitch << ", " << ground << "]" << std::endl;
}

void GameStateManager::packetCSPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerPositionAndLook from #" << eid << ": [" << X << ", " << Y << ", " << Z << ", "
            << stance << ", " << yaw << ", " << pitch << ", " << ground << "]" << std::endl;

  const WorldCoords wc(X, Y, Z);
  m_states[eid].position = wc;

  if (m_states[eid].state == PlayerState::READYTOSPAWN)
  {
    packetSCPlayerPositionAndLook(eid, X, Y, Z, stance, yaw, pitch, ground);
    m_states[eid].state = PlayerState::SPAWNED;
  }
  else if (getChunkCoords(m_states[eid].position) != getChunkCoords(wc))
  {
    sendMoreChunksToPlayer(eid);
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

  m_connection_manager.setNickname(eid, name);

  auto it = m_states.find(eid);

  if (it != m_states.end())
  {
    std::cout << "GSM: Error, received handshake from a client that is already connected." << std::endl;
    packetSCKick(eid, "Extraneous handshake received!");
    m_connection_manager.safeStop(eid);
    it->second.state = PlayerState::TERMINATED;
    return;
  }

  m_states.insert(std::pair<int32_t, PlayerState>(eid, PlayerState(PlayerState::PRELOGIN)));

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

  std::string name = m_connection_manager.getNickname(eid);

  if (name != username)
  {
    std::cout << "Username differs from the nickname we received earlier! Aborting." << std::endl;
    packetSCKick(eid, "Username mismatch!");
    m_connection_manager.safeStop(eid);
    return;
  }

  {
    PacketCrafter p(PACKET_LOGIN_REQUEST);
    p.addInt32(eid);
    p.addJString("");
    p.addJString("");
    p.addInt64(12345);
    p.addInt8(0); // 0: normal, -1: Nether
    m_connection_manager.sendDataToClient(eid, p.craft());
  }

  m_states[eid].state = PlayerState::POSTLOGIN;

  if (!PROGRAM_OPTIONS["testfile"].as<std::string>().empty())
  {
    RegionFile f(PROGRAM_OPTIONS["testfile"].as<std::string>());
    f.parse();

    std::vector<ChunkCoords> ac, bc;

    for (size_t x = 0; x < 32; ++x)
      for (size_t z = 0; z < 32; ++z)
        if (f.chunkSize(x, z) != 0) ac.push_back(ChunkCoords(x, z));

    /// Load all available chunks to memory, but only send the first 50 to the client.

    WorldCoords start_pos(100, 66, 78);

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

      std::pair<const unsigned char *, size_t> p = m_map.chunk(*i).compress_beefedup();
      packetSCPreChunk(eid, *i, true);

      if (p.second > 18)
        packetSCMapChunk(eid, p);
      else
        packetSCMapChunk(eid, *i, m_map.chunk(*i).compress());
    }

    m_states[eid].state = PlayerState::READYTOSPAWN;

    packetSCSpawn(eid, start_pos);
    packetSCPlayerPositionAndLook(eid, wX(start_pos), wY(start_pos), wZ(start_pos), wY(start_pos) + 1.6, 0.0, 0.0, false);
  }
  else
  {
    PlayerState & player = m_states[eid];

    const WorldCoords start_pos(8, 80, 8);
    //const WorldCoords start_pos(16, 66, -32);

    player.position = RealCoords(wX(start_pos) + 0.5, wZ(start_pos) + 0.5, wZ(start_pos) + 0.5);

    sendMoreChunksToPlayer(eid);

    m_states[eid].state = PlayerState::READYTOSPAWN;

    packetSCSpawn(eid, start_pos);
    packetSCPlayerPositionAndLook(eid, wX(start_pos), wY(start_pos), wZ(start_pos), wY(start_pos) + 1.6, 0.0, 0.0, true);

    packetSCSetSlot(eid, 0, 37, ITEM_DiamondPickaxe, 1, 0);
    packetSCSetSlot(eid, 0, 36, BLOCK_Torch, 50, 0);
    packetSCSetSlot(eid, 0, 29, ITEM_Coal, 50, 0);
    packetSCSetSlot(eid, 0, 21, BLOCK_Cobblestone, 60, 0);
    packetSCSetSlot(eid, 0, 22, BLOCK_IronOre, 60, 0);
    packetSCSetSlot(eid, 0, 30, BLOCK_Wood, 50, 0);
    packetSCSetSlot(eid, 0, 38, ITEM_DiamondShovel, 1, 0);
    packetSCSetSlot(eid, 0, 39, BLOCK_BrickBlock, 64, 0);
    packetSCSetSlot(eid, 0, 40, BLOCK_Stone, 64, 0);
    packetSCSetSlot(eid, 0, 41, BLOCK_Glass, 64, 0);
    packetSCSetSlot(eid, 0, 42, BLOCK_WoodenPlank, 64, 0);
    packetSCSetSlot(eid, 0, 43, ITEM_Bucket, 1, 0);
  }
}

void GameStateManager::packetCSChatMessage(int32_t eid, std::string message)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received ChatMessage from #" << std::dec << eid << ": \"" << message << "\"" << std::endl;
}

void GameStateManager::packetCSDisconnect(int32_t eid, std::string message)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Disconnect from #" << std::dec << eid << ": \"" << message << "\"" << std::endl;
  m_connection_manager.safeStop(eid);
  m_states[eid].state = PlayerState::TERMINATED;
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

void GameStateManager::packetSCSpawn(int32_t eid, const WorldCoords & wc)
{
  PacketCrafter p(PACKET_SPAWN_POSITION);
  p.addInt32(wX(wc));  // X
  p.addInt32(wY(wc));  // Y
  p.addInt32(wZ(wc));  // Z
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

void GameStateManager::packetSCBlockChange(int32_t eid, const WorldCoords & wc, int8_t block_type, int8_t block_md)
{
  std::cout << "Sending BlockChange to #" << std::dec << eid << ": " << wc << ", block type " << int(block_type) << std::endl;

  PacketCrafter p(PACKET_BLOCK_CHANGE);
  p.addInt32(wX(wc));    // X
  p.addInt8 (wY(wc));    // Y
  p.addInt32(wZ(wc));    // Z
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

void GameStateManager::packetSCOpenWindow(int32_t eid, int8_t window_id, int8_t window_type, std::string title, int8_t slots)
{
  PacketCrafter p(PACKET_OPEN_WINDOW);
  p.addInt8(window_id);
  p.addInt8(window_type);
  p.addJString(title);
  p.addInt8(slots);
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCPickupSpawn(int32_t eid, int32_t e, uint16_t type, uint8_t count, uint16_t da, const WorldCoords & wc)
{
  // We ougth to randomise this a little

  PacketCrafter p(PACKET_PICKUP_SPAWN);
  p.addInt32(e);    // item EID
  p.addInt16(type);
  p.addInt8(count);
  p.addInt16(da);   // damage or metadata
  p.addInt32(wX(wc) * 32 + 16);
  p.addInt32(wY(wc) * 32 + 16);
  p.addInt32(wZ(wc) * 32 + 16);

  p.addAngleAsByte(0);
  p.addAngleAsByte(0);
  p.addAngleAsByte(0);

  m_connection_manager.sendDataToClient(eid, p.craft());

}
