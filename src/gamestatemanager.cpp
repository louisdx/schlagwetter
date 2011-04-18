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
  state(s), position(), known_chunks(),
  inventory_ids(),
  inventory_damage(),
  inventory_count()
{
  std::fill(inventory_ids.begin(), inventory_ids.end(), -1);
  std::fill(inventory_damage.begin(), inventory_damage.end(), 0);
  std::fill(inventory_count.begin(), inventory_count.end(), 0);
}

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

  PlayerState & player = *it->second;

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


void GameStateManager::sendInventoryToPlayer(int32_t eid)
{
  const PlayerState & ps = *m_states[eid];
  for (size_t i = 0; i < ps.inventory_ids.size(); ++i)
  {
    // "no item" means type -1
    packetSCSetSlot(eid, 0, i, ps.inventory_ids[i], ps.inventory_count[i], ps.inventory_damage[i]);    
  }
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
  //std::cout << "Player #" << eid << " moves to " << m_states[eid].position << "/" << getWorldCoords(m_states[eid].position) << std::endl;

  auto interesting_blocks = m_map.blockAlerts().equal_range(getWorldCoords(m_states[eid]->position));

  for (auto it = interesting_blocks.first; it != interesting_blocks.second; ++it)
  {
    if (it->second.type == Map::BlockAlert::CONTAINS_SPAWN_ITEM)
    {
      // std::cout << "Player #" << eid << " interacts with object " << it->second.data << " at " << it->first << "." << std::endl;

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


// Returns false if the item dies.
bool fall(WorldCoords & wbelow, const Map & map)
{
  // This really shouldn't ever be able to happen...
  if (! map.haveChunk(getChunkCoords(wbelow))) return false;

  const Chunk & chunk = map.chunk(getChunkCoords(wbelow));

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

    if (chunk.blockType(getLocalCoords(wbelow)) != BLOCK_Air) break;
  }

  wbelow += BLOCK_YPLUS; // wbelow is now the last air block

  return true;
}

void GameStateManager::spawnSomething(uint16_t type, uint8_t number, uint8_t damage, const WorldCoords & wc)
{
  WorldCoords wbelow(wc);

  if (!fall(wbelow, m_map)) return;

  int32_t eid = GenerateEID();

  m_map.blockAlerts().insert(std::make_pair(wbelow, Map::BlockAlert(Map::BlockAlert::CONTAINS_SPAWN_ITEM, eid)));

  m_map.items().insert(std::make_pair(eid, type)); // stub

  sendToAll(MAKE_CALLBACK(packetSCPickupSpawn, eid, type, number, damage, wc));
}


/// This is the workhorse for digging (left-click) decisions.
/// It is called AFTER the dug block has been changed to Air,
/// but block_type is the value of the old, removed block
/// (maybe this can be structured better).

void GameStateManager::reactToSuccessfulDig(const WorldCoords & wc, EBlockItem block_type)
{
  // this is just temporary
  std::cout << "Successfully dug at " << wc << " for " << BLOCKITEM_INFO.find(block_type)->second.name << std::endl;

  if (block_type != 0)
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

    if (wY(wc) < 127)
    {
      Map::AlertMap new_alerts;
      interesting_blocks = m_map.blockAlerts().equal_range(wc + BLOCK_YPLUS);

      for (auto it = interesting_blocks.first; it != interesting_blocks.second; )
      {
        if (it->second.type == Map::BlockAlert::CONTAINS_SPAWN_ITEM)
        {
          WorldCoords wbelow(wc);

          if (fall(wbelow, m_map))
          {
            new_alerts.insert(std::make_pair(wbelow, it->second));
          }

          std::cout << "Item " << it->second.data << " must fall from " << wc << " to " << wbelow << "." << std::endl;

          m_map.blockAlerts().erase(it++);
        }
        else
        {
          ++it;
        }
      }

      m_map.blockAlerts().insert(new_alerts.begin(), new_alerts.end());
    }

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

      meta = m_states[eid]->getRelativeDirection(wc + dir);
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

      m_map.blockAlerts().insert(std::make_pair(wc, Map::BlockAlert(Map::BlockAlert::SUPPORTS_CANDLE)));

      return OK_WITH_META;
    }

  default:
    return OK_NO_META;
  }

  return OK_NO_META;
}
