#include <iostream>
#include <cmath>
#include <algorithm>
#include <functional>

#include "gamestatemanager.h"
#include "map.h"


int32_t EID_POOL = 0;
int32_t GenerateEID() { return ++EID_POOL; }

PlayerState::PlayerState(EState s)
  :
  state(s),
  position(), stance(0), pitch(0), yaw(0),
  known_chunks(),
  inventory_ids(),
  inventory_damage(),
  inventory_count(),
  holding(0)
{
  std::fill(inventory_ids.begin(), inventory_ids.end(), -1);
  std::fill(inventory_damage.begin(), inventory_damage.end(), 0);
  std::fill(inventory_count.begin(), inventory_count.end(), 0);
}

/// Returns the horizontal direction of rc relative to the player's
/// position (this will usally be the negative of what you're thinking
/// about, but numerically more convenient for the block metadata values).

Direction PlayerState::getRelativeXZDirection(const RealCoords & rc)
{
  const double diffX = rX(rc) - rX(position);
  const double diffZ = rZ(rc) - rZ(position);

  if (std::abs(diffX) > std::abs(diffZ))
  {
    // We compare on the x axis
    if (diffX > 0)  return BLOCK_XPLUS;
    else            return BLOCK_XMINUS;
  }
  else
  {
    // We compare on the z axis
    if (diffZ > 0) return BLOCK_ZPLUS;
    else           return BLOCK_ZMINUS;
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
  if (it != m_states.end() && it->second->state == PlayerState::TERMINATED)
  {
    std::cout << "Client #" << eid << " should leave, closing connection." << std::endl;
    m_connection_manager.safeStop(eid);
    m_states.erase(it);
  }

}

void GameStateManager::sendToAll(std::function<void(int32_t)> f)
{
  std::list<int32_t> todo;

  {
    std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);
    for (auto it = m_states.cbegin(); it != m_states.cend(); ++it)
      todo.push_back(it->first);
  }

  for (auto it = todo.cbegin(); it != todo.cend(); ++it)
    f(*it);
}

void GameStateManager::sendToAllExceptOne(std::function<void(int32_t)> f, int32_t eid)
{
  std::list<int32_t> todo;

  {
    std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);
    for (auto it = m_states.cbegin(); it != m_states.cend(); ++it)
      if (it->first != eid)
        todo.push_back(it->first);
  }

  for (auto it = todo.cbegin(); it != todo.cend(); ++it)
    f(*it);
}

void GameStateManager::sendRawToAll(const std::string & data)
{
  std::list<int32_t> todo;

  {
    std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);
    for (auto it = m_states.cbegin(); it != m_states.cend(); ++it)
      todo.push_back(it->first);
  }

  for (auto it = todo.cbegin(); it != todo.cend(); ++it)
    m_connection_manager.sendDataToClient(*it, data);
}

void GameStateManager::sendRawToAllExceptOne(const std::string & data, int32_t eid)
{
  std::list<int32_t> todo;

  {
    std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);
    for (auto it = m_states.cbegin(); it != m_states.cend(); ++it)
      if (it->first != eid)
        todo.push_back(it->first);
  }

  for (auto it = todo.cbegin(); it != todo.cend(); ++it)
    m_connection_manager.sendDataToClient(*it, data);
}

void GameStateManager::sendMoreChunksToPlayer(int32_t eid)
{
  std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);

  auto it = m_states.find(eid);
  if (it == m_states.end()) return;

  PlayerState & player = *it->second;

  // Someone can go and implement more overloads if this looks too icky.
  const ChunkCoords pc = getChunkCoords(getWorldCoords(getFractionalCoords(player.position)));

  std::vector<ChunkCoords> ac = ambientChunks(pc, PLAYER_CHUNK_HORIZON);
  std::sort(ac.begin(), ac.end(), L1DistanceFrom(pc));

  /// Here follows the typical chunk update acrobatics in three rounds.

  // Round 1: Load all relevant chunks to memory

  for (auto i = ac.cbegin(); i != ac.cend(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;

    //std::cout << "Player #" << std::dec << eid << " needs chunk " << *i << "." << std::endl;
    m_map.ensureChunkIsReadyForImmediateUse(*i);
  }

  // Round 2: Spread light to all chunks in memory. Light only spreads to loaded chunks.

  for (auto i = ac.cbegin(); i != ac.cend(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;

    m_map.chunk(*i).spreadAllLight(m_map);
    m_map.chunk(*i).spreadToNewNeighbours(m_map);
  }

  // Round 3: Send the fully updated chunks to the client.
  // 3a: Prechunks
  for (auto i = ac.cbegin(); i != ac.cend(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;

    Chunk & chunk = m_map.chunk(*i);
    packetSCPreChunk(eid, *i, true);
  }
  // 3b: Actual chunks
  for (auto i = ac.cbegin(); i != ac.cend(); ++i)
  {
    if (player.known_chunks.count(*i) > 0) continue;

    Chunk & chunk = m_map.chunk(*i);

    // Not sure if the client has a problem with data coming in too fast...
    //sleepMilli(5);

#define USE_ZCACHE 0   // The local chache doesn't seem to work reliably.
#if USE_ZCACHE > 0
    // This is using a chunk-local zip cache.
    std::pair<const unsigned char *, size_t> p = chunk.compress_beefedup();

    if (p.second > 18)
      packetSCMapChunk(eid, p);
    else
      packetSCMapChunk(eid, *i, chunk.compress());
#else
    // This is the safe way.
    packetSCMapChunk(eid, *i, chunk.compress());
#endif
#undef USE_ZCACHE

    player.known_chunks.insert(*i);
  }
}


void GameStateManager::sendInventoryToPlayer(int32_t eid)
{
  // I don't understand why we need this here (but we do), it should be possible to filter that already in the packet handlers.
  if (m_states.find(eid) == m_states.end())
  {
    std::cout << "Error, player state hasn't been constructed." << std::endl;
    packetSCKick(eid, "Server error.");
    m_connection_manager.safeStop(eid);
    return;
  }

  const PlayerState & ps = *m_states[eid];

  for (size_t i = 0; i < ps.inventory_ids.size(); ++i)
  {
    // "no item" means type -1
    packetSCSetSlot(eid, 0, i, ps.inventory_ids[i], ps.inventory_count[i], ps.inventory_damage[i]);    
  }

  //packetSCHoldingChange(eid, ps.holding); // this is not a valid Server-to-Client packet!
}


uint16_t GameStateManager::updatePlayerInventory(int32_t eid, int16_t type, uint16_t count, uint16_t damage, bool send_packets)
{
  PlayerState & ps = *m_states[eid];

  // 1. Try adding to an existing slot
  for (size_t i = 9; count > 0 && i < ps.inventory_ids.size(); ++i)
  {
    if (ps.inventory_ids[i] == type)
    {
      if (ps.inventory_count[i] < 10)
      {
        const uint16_t inc = std::min(int(count), 10 - ps.inventory_count[i]);
        ps.inventory_count[i] += inc;

        std::cout << "Sending inventory [" << ps.inventory_ids[i] << ", " << ps.inventory_count[i]
                  << ", " << ps.inventory_damage[i] << "] to player #" << eid << " slot " << i  << "." << std::endl;

        packetSCSetSlot(eid, 0, i, ps.inventory_ids[i], ps.inventory_count[i], ps.inventory_damage[i]);   
        count -= inc;
      }
    }
  }

  // 2. Try adding to an empty slot
  for (size_t i = 9; count > 0 && i < ps.inventory_ids.size(); ++i)
  {
    if (ps.inventory_ids[i] == -1)
    {
      const uint16_t inc = std::min(int(count), 10);
      ps.inventory_count[i] += inc;
      ps.inventory_ids[i] = type;

        std::cout << "Sending inventory [" << ps.inventory_ids[i] << ", " << ps.inventory_count[i]
                  << ", " << ps.inventory_damage[i] << "] to player #" << eid << " slot " << i  << "." << std::endl;

      packetSCSetSlot(eid, 0, i, ps.inventory_ids[i], ps.inventory_count[i], ps.inventory_damage[i]);   
      count -= inc;
    }
  }

  return count;
}

bool isBuildable(EBlockItem e)
{
  // Whether or not a new block may be placed in this block

  switch (e)
    {
    case BLOCK_Water:
    case BLOCK_StationaryWater:
    case BLOCK_Air:
      return true;

    default:
      return false;
    }
}

bool isPassable(EBlockItem e)
{
  // Whether or not things can pass through this block

  switch (e)
    {
    case BLOCK_Torch:
    case BLOCK_RedstoneTorchOff:
    case BLOCK_RedstoneTorchOn:
    case BLOCK_RedstoneWire:
    case BLOCK_Water:
    case BLOCK_StationaryWater:
    case BLOCK_Air:
    case BLOCK_Rails:
    case BLOCK_WoodenDoor:
    case BLOCK_IronDoor:
    case BLOCK_SignPost:
    case BLOCK_WallSign:
      return true;

    default:
      return false;
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


/// Things we have to do when a block gets toggled: swing doors, flip switches.

void GameStateManager::reactToToggle(const WorldCoords & wc, EBlockItem b)
{
  if (!m_map.haveChunk(getChunkCoords(wc))) return;

  Chunk & chunk = m_map.chunk(getChunkCoords(wc));

  switch (b)
  {
  case BLOCK_WoodenDoor:
  case BLOCK_IronDoor:
    {
      // We don't really care if the door is one, two or three blocks tall,
      // but we'll only treat at most one block above and below the clicked one.

      uint8_t meta = chunk.getBlockMetaData(getLocalCoords(wc));
      meta ^= 0x4;
      chunk.setBlockMetaData(getLocalCoords(wc), meta);
      sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc, b, meta));

      if (wY(wc) < 127 && chunk.blockType(getLocalCoords(wc + BLOCK_YPLUS)) == b)
      {
        uint8_t meta = chunk.getBlockMetaData(getLocalCoords(wc + BLOCK_YPLUS));
        meta ^= 0x4;
        chunk.setBlockMetaData(getLocalCoords(wc + BLOCK_YPLUS), meta);
        sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc + BLOCK_YPLUS, b, meta));
      }

      if (wY(wc) > 0 && chunk.blockType(getLocalCoords(wc + BLOCK_YMINUS)) == b)
      {
        uint8_t meta = chunk.getBlockMetaData(getLocalCoords(wc + BLOCK_YMINUS));
        meta ^= 0x4;
        chunk.setBlockMetaData(getLocalCoords(wc + BLOCK_YMINUS), meta);
        sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc + BLOCK_YMINUS, b, meta));
      }

      break;
    }
  default: break;
  }
}


/// Things we have to do when a block gets destroyed: Remove attached torches.

void GameStateManager::reactToBlockDestruction(const WorldCoords & wc)
{
  // Clear the block alerts. The only alerts that can possibly be
  // stored with a solid block are of the type that we process here,
  // so we clear the entire range.

  auto interesting_blocks = m_map.blockAlerts().equal_range(wc);
  m_map.blockAlerts().erase(interesting_blocks.first, interesting_blocks.second);


  // Remove attached torches.

  WorldCoords wn;

  for (size_t k = 1; k < 6; ++k)
  {
    const WorldCoords wn = wc + Direction(k);

    if (!m_map.haveChunk(getChunkCoords(wn))) continue;

    unsigned char & block =  m_map.chunk(getChunkCoords(wn)).blockType(getLocalCoords(wn));

    if (block == BLOCK_Torch)
    {
      sendToAll(MAKE_CALLBACK(packetSCBlockChange, wn, BLOCK_Air, 0));
      block = BLOCK_Air;
      m_map.chunk(getChunkCoords(wn)).taint();
      reactToSuccessfulDig(wn, EBlockItem(block));
    }
  }

}

void GameStateManager::handlePlayerMove(int32_t eid)
{
  const PlayerState & player = *m_states[eid];

  sendRawToAllExceptOne(rawPacketSCEntityTeleport(eid, getFractionalCoords(player.position), player.yaw, player.pitch), eid);

  auto interesting_blocks = m_map.blockAlerts().equal_range(getWorldCoords(player.position));

  for (auto it = interesting_blocks.first; it != interesting_blocks.second; ++it)
  {
    if (it->second.type == Map::BlockAlert::CONTAINS_SPAWN_ITEM)
    {
      const auto jt = m_map.items().find(it->second.data);
      if (jt != m_map.items().end())
      {
        sendToAll(MAKE_CALLBACK(packetSCCollectItem, jt->first, eid));
        sendToAll(MAKE_CALLBACK(packetSCDestroyEntity, jt->first));

        // Add 1 undamaged unit to the player's inventory.
        updatePlayerInventory(eid, jt->second, 1, 0);

        m_map.items().erase(jt);
      }
    }
  }
}


/// A helper function to establish the nearest non-passable block below. Returns false if the item dies.
/// i.e. by falling out of the world or into fire or a cactus. The argument is expected to be the
/// initial position and is modified to the final resting position.

/// All this isn't quite right; in the real game it's possible to catch a falling item in mid-air
/// even if it would otherwise fall to its death.

bool GameStateManager::fall(WorldCoords & wbelow)
{
  // This really shouldn't ever be able to happen...
  if (!m_map.haveChunk(getChunkCoords(wbelow))) return false;

  const Chunk & chunk = m_map.chunk(getChunkCoords(wbelow));

  for ( ; ; wbelow += BLOCK_YMINUS)
  {
    // An item that drops on something hot or out of the world dies.
    if (chunk.blockType(getLocalCoords(wbelow)) == BLOCK_Lava           ||
        chunk.blockType(getLocalCoords(wbelow)) == BLOCK_StationaryLava ||
        chunk.blockType(getLocalCoords(wbelow)) == BLOCK_Fire           ||
        wY(wbelow) < 0 )
    {
      return false; // won't even spawn an item that's died.
    }

    if (!isPassable(EBlockItem(chunk.blockType(getLocalCoords(wbelow))))) break;
  }

  wbelow += BLOCK_YPLUS; // wbelow is now the last passable block

  return true;
}

void GameStateManager::spawnSomething(uint16_t type, uint8_t number, uint8_t damage, const WorldCoords & wc)
{
  WorldCoords wbelow(wc);

  if (!fall(wbelow)) return;

  int32_t eid = GenerateEID();

  m_map.blockAlerts().insert(std::make_pair(wbelow, Map::BlockAlert(Map::BlockAlert::CONTAINS_SPAWN_ITEM, eid)));

  m_map.items().insert(std::make_pair(eid, type)); // stub

  sendToAll(MAKE_CALLBACK(packetSCPickupSpawn, eid, type, number, damage, wc));
}


/// Items that rest on a block that gets destroyed fall; this is a client-preempted
/// reaction that we must track.

void GameStateManager::makeItemsDrop(const WorldCoords & wc)
{
  Map::AlertMap new_alerts;
  auto interesting_blocks = m_map.blockAlerts().equal_range(wc + BLOCK_YPLUS); // if y == 127, we won't find anything.

  for (auto it = interesting_blocks.first; it != interesting_blocks.second; )
  {
    if (it->second.type == Map::BlockAlert::CONTAINS_SPAWN_ITEM)
    {
      WorldCoords wbelow(wc);

      if (fall(wbelow))
      {
        new_alerts.insert(std::make_pair(wbelow, it->second));
        std::cout << "Item " << it->second.data << " must fall from " << wc << " to " << wbelow << "." << std::endl;
      }
      else
      {
        std::cout << "Item " << it->second.data << " falls from " << wc << " to its death." << std::endl;
        sendRawToAll(rawPacketSCDestroyEntity(it->second.data));
      }
  
      m_map.blockAlerts().erase(it++);
    }
    else
    {
      ++it;
    }
  }

  m_map.blockAlerts().insert(new_alerts.begin(), new_alerts.end());
}


/// This is the workhorse for digging (left-click) decisions.
/// It is called AFTER the dug block has been changed to Air,
/// but block_type is the value of the old, removed block
/// (maybe this can be structured better).

void GameStateManager::reactToSuccessfulDig(const WorldCoords & wc, EBlockItem block_type)
{
  // this is just temporary
  std::cout << "Successfully dug at " << wc << " for " << BLOCKITEM_INFO.find(block_type)->second.name << std::endl;

  if (block_type == BLOCK_WoodenDoor || block_type == BLOCK_IronDoor)
  {
    Chunk & chunk = m_map.chunk(getChunkCoords(wc));

    if (wY(wc) < 127 && chunk.blockType(getLocalCoords(wc + BLOCK_YPLUS)) == block_type)
    {
      sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc + BLOCK_YPLUS, BLOCK_Air, 0));
      chunk.blockType(getLocalCoords(wc + BLOCK_YPLUS)) = BLOCK_Air;
    }

    if (wY(wc) > 0 && chunk.blockType(getLocalCoords(wc + BLOCK_YMINUS)) == block_type)
    {
      sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc + BLOCK_YMINUS, BLOCK_Air, 0));
      chunk.blockType(getLocalCoords(wc + BLOCK_YMINUS)) = BLOCK_Air;
    }

    spawnSomething(block_type == BLOCK_WoodenDoor ? ITEM_WoodenDoor : ITEM_IronDoor, 1, 0, wc);
  }

  else if (block_type != 0)
  {
    spawnSomething(uint16_t(block_type), 1, 0, wc);

    auto interesting_blocks = m_map.blockAlerts().equal_range(wc);

    for (auto it = interesting_blocks.first; it != interesting_blocks.second; ++it)
    {
      if (it->second.type == Map::BlockAlert::SUPPORTS_CANDLE)
      {
        reactToBlockDestruction(wc);
        break;
      }
    }
  }
}

/// This is the workhorse for block placement (right-click) decisions.

GameStateManager::EBlockPlacement GameStateManager::blockPlacement(int32_t eid,
    const WorldCoords & wc, Direction dir, BlockItemInfoMap::const_iterator it, uint8_t & meta)
{
  // I believe we are never allowed to place anything on an already occupied block.
  // If that's false, we have to refactor this check. Water counts as unoccupied.

  if (!m_map.haveChunk(getChunkCoords(wc + dir)) || !isBuildable(EBlockItem(m_map.chunk(getChunkCoords(wc + dir)).blockType(getLocalCoords(wc + dir)))))
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

      // Stairs are oriented to have their low end pointing toward the player.
      meta = 5 - int(m_states[eid]->getRelativeXZDirection(midpointRealCoords(wc + dir)));
      return OK_WITH_META;
    }

  case BLOCK_FurnaceBlock:
    {
      std::cout << "Special block: #" << eid << " is trying to place a furnace." << std::endl;

      if (wY(wc) == 0 || dir == BLOCK_YMINUS) return CANNOT_PLACE_AIRFORCE;

      const int d = int(m_states[eid]->getRelativeXZDirection(midpointRealCoords(wc + dir)));

      if      (d == 2) meta = 3;
      else if (d == 3) meta = 2;
      else if (d == 4) meta = 5;
      else if (d == 5) meta = 4;

      return OK_WITH_META;
    }
  case BLOCK_Torch:
  case BLOCK_RedstoneTorchOff:
  case BLOCK_RedstoneTorchOn:
    {
      std::cout << "Special block: #" << eid << " is trying to place a torch." << std::endl;

      // Torches are oriented simply to attach to the face which the user clicked.
      switch (int(dir))
      {
      case 2: meta = 4; break;
      case 3: meta = 3; break;
      case 4: meta = 2; break;
      case 5: meta = 1; break;
      default: meta = 5; break;
      }

      m_map.blockAlerts().insert(std::make_pair(wc, Map::BlockAlert(Map::BlockAlert::SUPPORTS_CANDLE)));

      return OK_WITH_META;
    }

  case ITEM_WoodenDoor:
  case ITEM_IronDoor:
    {
      // Doors can only be placed from above.
      if (dir != BLOCK_YPLUS)
        return CANNOT_PLACE;

      // Doors cannot be placed on glass, it seems.
      if (m_map.chunk(getChunkCoords(wc)).blockType(getLocalCoords(wc)) == BLOCK_Glass)
        return CANNOT_PLACE;

      const auto d = m_states[eid]->getRelativeXZDirection(midpointRealCoords(wc + dir));
      const unsigned char b = it->first == ITEM_WoodenDoor ? BLOCK_WoodenDoor : BLOCK_IronDoor;

      uint8_t meta;

      enum { HINGE_NE = 0, HINGE_SE = 1, HINGE_SW = 2, HINGE_NW = 3, SWUNG = 4 };

      switch (d)
      {
      case BLOCK_XPLUS:
        {
          if (m_map.haveChunk(getChunkCoords(wc + dir + BLOCK_ZPLUS)) &&
              !isBuildable(EBlockItem(m_map.chunk(getChunkCoords(wc + dir + BLOCK_ZPLUS)).blockType(getLocalCoords(wc + dir + BLOCK_ZPLUS)))))
            meta = HINGE_NW | SWUNG;
          else
            meta = HINGE_NE;
          break;
        }
      case BLOCK_XMINUS:
        {
          if (m_map.haveChunk(getChunkCoords(wc + dir + BLOCK_ZMINUS)) &&
              !isBuildable(EBlockItem(m_map.chunk(getChunkCoords(wc + dir + BLOCK_ZMINUS)).blockType(getLocalCoords(wc + dir + BLOCK_ZMINUS)))))
            meta = HINGE_SE | SWUNG;
          else
            meta = HINGE_SW;
          break;
        }
      case BLOCK_ZPLUS:
        {
          if (m_map.haveChunk(getChunkCoords(wc + dir + BLOCK_XMINUS)) &&
              !isBuildable(EBlockItem(m_map.chunk(getChunkCoords(wc + dir + BLOCK_XMINUS)).blockType(getLocalCoords(wc + dir + BLOCK_XMINUS)))))
            meta = HINGE_NE | SWUNG;
          else
            meta = HINGE_SE;
          break;
        }
      case BLOCK_ZMINUS:
        {
          if (m_map.haveChunk(getChunkCoords(wc + dir + BLOCK_XPLUS)) &&
              !isBuildable(EBlockItem(m_map.chunk(getChunkCoords(wc + dir + BLOCK_XPLUS)).blockType(getLocalCoords(wc + dir + BLOCK_XPLUS)))))
            meta = HINGE_SW | SWUNG;
          else
            meta = HINGE_NW;
          break;
        }
      default: return CANNOT_PLACE;
      }

      // Double-door algorithm: only check for a door on the left (apparently that's what the client does).
      if      (d == BLOCK_XMINUS && m_map.chunk(getChunkCoords(wc + dir + BLOCK_ZPLUS)) .blockType(getLocalCoords(wc + dir + BLOCK_ZPLUS))  == b)
      {
        meta = HINGE_SE | SWUNG;
      }
      else if (d == BLOCK_XPLUS  && m_map.chunk(getChunkCoords(wc + dir + BLOCK_ZMINUS)).blockType(getLocalCoords(wc + dir + BLOCK_ZMINUS)) == b)
      {
        meta = HINGE_NW | SWUNG;
      }
      else if (d == BLOCK_ZMINUS && m_map.chunk(getChunkCoords(wc + dir + BLOCK_XMINUS)).blockType(getLocalCoords(wc + dir + BLOCK_XMINUS)) == b)
      {
        meta = HINGE_SW | SWUNG;
      }
      else if (d == BLOCK_ZPLUS  && m_map.chunk(getChunkCoords(wc + dir + BLOCK_XPLUS)) .blockType(getLocalCoords(wc + dir + BLOCK_XPLUS))  == b)
      {
        meta = HINGE_NE | SWUNG;
      }

      sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc + dir, b, meta));
      sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc + dir + BLOCK_YPLUS, b, meta | 0x8));

      Chunk & chunk = m_map.chunk(getChunkCoords(wc + dir));
      chunk.blockType(getLocalCoords(wc + dir)) = b;
      chunk.setBlockMetaData(getLocalCoords(wc + dir), meta);
      chunk.blockType(getLocalCoords(wc + dir + BLOCK_YPLUS)) = b;
      chunk.setBlockMetaData(getLocalCoords(wc + dir + BLOCK_YPLUS), meta | 0x8);

      return OK_NO_META;
    }

  default:
    return OK_NO_META;
  }

  return OK_NO_META;
}
