#include <iostream>
#include "gamestatemanager.h"
#include "packetcrafter.h"
#include "cmdlineoptions.h"
#include "map.h"
#include "filereader.h"
#include "salt.h"


/******************                  ****************
 ******************  Packet Handlers ****************
 ******************                  ****************/



/**** Client to Server ****/


void GameStateManager::packetCSPlayerDigging(int32_t eid, int32_t X, uint8_t Y, int32_t Z, uint8_t status, uint8_t face)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerDigging from #" << std::dec << eid << ": [" << X << ", " << (unsigned int)(Y)
            << ", " << Z << ", " << (unsigned int)(status) << ", " << (unsigned int)(face) << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;

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
    unsigned char block = chunk.blockType(getLocalCoords(wc));

    const unsigned char block_properties = BLOCK_DIG_PROPERTIES[block];

    if (block_properties & LEFTCLICK_DIGGABLE)
    {
      m_states[eid]->recent_dig = { wc, clockTick() };
    }

    if (block_properties & LEFTCLICK_REMOVABLE)
    {
      sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc, BLOCK_Air, 0));
      chunk.blockType(getLocalCoords(wc)) = BLOCK_Air;
      chunk.taint();
      reactToSuccessfulDig(wc, EBlockItem(block));
    }

    if (block_properties & LEFTCLICK_TRIGGER)
    {
      // Trigger changes require meta data fiddling.
      reactToToggle(wc, EBlockItem(block));
    }
  }

  else if (status == 2)
  {
    const WorldCoords wc(X, Y, Z);
    Chunk & chunk = m_map.chunk(wc);
    unsigned char block = chunk.blockType(getLocalCoords(wc));
    const unsigned char block_properties = BLOCK_DIG_PROPERTIES[block];

    if (block_properties & LEFTCLICK_DIGGABLE)
    {
      if (m_states[eid]->recent_dig.wc == wc) // further validation needed
      {
        std::cout << "#" << eid << " spent " << (clockTick() - m_states[eid]->recent_dig.start_time) << "ms digging for "
                  << BLOCKITEM_INFO.find(EBlockItem(block))->second.name << "." << std::endl;

        sendToAll(MAKE_CALLBACK(packetSCBlockChange, wc, BLOCK_Air, 0));
        chunk.blockType(getLocalCoords(wc)) = BLOCK_Air;
        chunk.taint();
        makeItemsDrop(wc);
        reactToSuccessfulDig(wc, EBlockItem(block));
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
  std::cout << "Player position is " << m_states[eid]->position << std::endl;

  if (m_states.find(eid) == m_states.end()) return;

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

    if (chunk.blockType(getLocalCoords(wc)) == BLOCK_CraftingTable        ||
        chunk.blockType(getLocalCoords(wc)) == BLOCK_FurnaceBlock         ||
        chunk.blockType(getLocalCoords(wc)) == BLOCK_FurnaceBurningBlock  ||
        chunk.blockType(getLocalCoords(wc)) == BLOCK_ChestBlock           ||
        chunk.blockType(getLocalCoords(wc)) == BLOCK_DispenserBlock         )
     {
       uint8_t w = -1, slots = -1;
       std::string title = "";

       switch (chunk.blockType(getLocalCoords(wc)))
         {
         case BLOCK_CraftingTable:       w = WINDOW_CraftingTable; slots = 9;  title = "Make it so!"; break;
         case BLOCK_FurnaceBlock:        w = WINDOW_Furnace;       slots = 9;  title = "Furnace";     break;
         case BLOCK_FurnaceBurningBlock: w = WINDOW_Furnace;       slots = 9;  title = "Furnace";     break;
         case BLOCK_ChestBlock:          w = WINDOW_Chest;         slots = 60; title = "Myspace";     break;
         case BLOCK_DispenserBlock:      w = WINDOW_Dispenser;     slots = 9;  title = "Dispenser";   break;
         }

       packetSCOpenWindow(eid, 123 /* window ID? */, w, title, slots);
     }

    /// Stage 3a: Genuine target, but empty hands.

    else if (block_id < 0)
    {
      // empty-handed, not useful
    }

    /// Stage 3b: Genuine target, holding a placeable block or item.

    else if (it != BLOCKITEM_INFO.end())
    {
      wc += Direction(direction);

      if (!m_map.haveChunk(getChunkCoords(wc))) return;

      if (wc == getWorldCoords(m_states[eid]->position)                                  ||
          (wY(wc) != 0 && wc + BLOCK_YMINUS == getWorldCoords(m_states[eid]->position))   )
      {
        std::cout << "Player #" << eid << " tries to bury herself in " << it->second.name << "." << std::endl;
      }

      // Holding a building block
      if (BlockItemInfo::type(it->first) == BlockItemInfo::BLOCK)
      {
        Chunk & chunk = m_map.chunk(getChunkCoords(wc));

        // Before placing the block, we have to see if we need to set magic metadata (stair directions etc.)
        uint8_t meta;
        const auto bp_res = blockPlacement(eid, WorldCoords(X, Y, Z), Direction(direction), it, meta);

        if (bp_res != CANNOT_PLACE && bp_res != CANNOT_PLACE_AIRFORCE)
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

          if (block_id == BLOCK_FurnaceBlock || block_id == BLOCK_FurnaceBurningBlock ||
              block_id == BLOCK_ChestBlock || block_id == BLOCK_DispenserBlock)
          {
            m_map.addStorage(wc, block_id);
          }
        }
        else if (bp_res == CANNOT_PLACE)
        {
        }
        else if (bp_res == CANNOT_PLACE_AIRFORCE)
        {
          // Naughty client gets slapped on the fingers.
          // This means the server overrides the client decision, so we change the official behaviour.
          // Should not generally be needed for the official client.
          packetSCBlockChange(eid, wc, BLOCK_Air, 0);
        }
      }

      // Holding an item
      else if (BlockItemInfo::type(it->first) == BlockItemInfo::ITEM)
      {
        // check for seeds, sign, buckets, doors, saddles, minecarts, bed

        // For item placement we expect blockPlacement() to do the packet sending work.
        uint8_t meta;
        blockPlacement(eid, WorldCoords(X, Y, Z), Direction(direction), it, meta);
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

  if (m_states.find(eid) == m_states.end()) return;

  packetSCKeepAlive(eid);
}

void GameStateManager::packetCSChunkRequest(int32_t eid, int32_t X, int32_t Z, bool mode)
{
  //if (PROGRAM_OPTIONS.count("verbose"))
    std::cout << "GSM: Received ChunkRequest from #" << eid << ": [" << X << ", " << Z << ", " << mode << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSUseEntity(int32_t eid, int32_t e, int32_t target, bool leftclick)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received UseEntity from #" << eid << ": [" << e << ", " << target << ", " << leftclick << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSPlayer(int32_t eid, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerOnGround from #" << eid << ": " << ground << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSPlayerPosition(int32_t eid, double X, double Y, double Z, double stance, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerPosition from #" << eid << ": [" << X << ", " << Y << ", " << Z << ", " << stance << ", " << ground << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;

  const RealCoords rc(X, Y, Z);

  if (getChunkCoords(m_states[eid]->position) != getChunkCoords(rc))
  {
    sendMoreChunksToPlayer(eid);
  }

  m_states[eid]->position = rc;
  m_states[eid]->stance   = stance;

  handlePlayerMove(eid);
}

void GameStateManager::packetCSPlayerLook(int32_t eid, float yaw, float pitch, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerLook from #" << eid << ": [" << yaw << ", " << pitch << ", " << ground << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;

  m_states[eid]->pitch = pitch;
  m_states[eid]->yaw   = yaw;
}

void GameStateManager::packetCSPlayerPositionAndLook(int32_t eid, double X, double Y, double Z, double stance, float yaw, float pitch, bool ground)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PlayerPositionAndLook from #" << eid << ": [" << X << ", " << Y << ", " << Z << ", "
            << stance << ", " << yaw << ", " << pitch << ", " << ground << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;

  const RealCoords rc(X, Y, Z);
  m_states[eid]->position = rc;
  m_states[eid]->stance   = stance;
  m_states[eid]->pitch    = pitch;
  m_states[eid]->yaw      = yaw;

  handlePlayerMove(eid);

  if (m_states[eid]->state == PlayerState::READYTOSPAWN)
  {
    packetSCPlayerPositionAndLook(eid, X, Y, Z, stance, yaw, pitch, ground);
    m_states[eid]->state = PlayerState::SPAWNED;
  }
  else if (getChunkCoords(m_states[eid]->position) != getChunkCoords(rc))
  {
    sendMoreChunksToPlayer(eid);
  }

}

void GameStateManager::packetCSHoldingChange(int32_t eid, int16_t slot)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received HoldingChange from #" << std::dec << eid << ": " << slot << std::endl;

  if (m_states.find(eid) == m_states.end() || slot < 0 || slot > 8) return;

  m_states[eid]->holding = slot;
  sendRawToAllExceptOne(rawPacketSCHoldingChange(slot), eid);
}

void GameStateManager::packetCSArmAnimation(int32_t eid, int32_t e, int8_t animate)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Animation from #" << std::dec << eid << ": [" << e << ", " << (unsigned int)(animate) << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSEntityCrouchBed(int32_t eid, int32_t e, int8_t action)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received CrouchBed from #" << std::dec << eid << ": [" << e << ", " << (unsigned int)(action) << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSPickupSpawn(int32_t eid, int32_t e, int32_t X, int32_t Y, int32_t Z,
                                           double rot, double pitch, double roll, int8_t count, int16_t item, int16_t data)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received PickupSpawn from #" << std::dec << eid << ": [" << e << ", " << X << ", " << Y << ", " << Z << ", "
            << rot << ", " << pitch << ", " << roll << ", " << (unsigned int)(count) << ", " << item << ", " << data << ", "<< "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSRespawn(int32_t eid)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Respawn from #" << std::dec << eid << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSCloseWindow(int32_t eid, int8_t window_id)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received CloseWindow from #" << std::dec << eid << ": " << (unsigned int)(window_id) << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
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
    it->second->state = PlayerState::TERMINATED;
    return;
  }

  m_states.insert(std::make_pair(eid, std::make_shared<PlayerState>(PlayerState::PRELOGIN)));

  PacketCrafter p(PACKET_HANDSHAKE);
  p.addString("-");

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

  if (m_states.find(eid) == m_states.end()) return;

  const std::string name = m_connection_manager.getNickname(eid);

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
    p.addString("");
    if (protocol_version < 0x0B) p.addString("");
    p.addInt64(m_map.seed());
    p.addInt8(0); // 0: normal, -1: Nether
    m_connection_manager.sendDataToClient(eid, p.craft());
  }

  m_states[eid]->state = PlayerState::POSTLOGIN;

  deserializePlayer(eid);

  PlayerState & player = *m_states[eid];

  player.state = PlayerState::READYTOSPAWN;

  const WorldCoords start_pos = getWorldCoords(player.position);

  sendMoreChunksToPlayer(eid);

  packetSCSpawn(eid, start_pos);

  packetSCPlayerPositionAndLook(eid, wX(start_pos), wY(start_pos), wZ(start_pos), wY(start_pos) + 1.62, 0.0, 0.0, true);

  // Inform all others that this player has spawned.
  sendToAllExceptOne(MAKE_CALLBACK(packetSCSpawnEntity, eid, getFractionalCoords(player.position), 0, 0, 0), eid);

  // Inform this player of all the other players' positions. (Apparently one should only do this with players that are in range.)
  {
    std::lock_guard<std::recursive_mutex> lock(m_gs_mutex);
    for (auto it = m_states.cbegin(); it != m_states.cend(); ++it)
      if (it->first != eid)
        packetSCSpawnEntity(eid, it->first, getFractionalCoords(it->second->position), it->second->yaw, it->second->pitch, 0);
  }

  sendInventoryToPlayer(eid);
}

void GameStateManager::packetCSChatMessage(int32_t eid, std::string message)
{
  //if (PROGRAM_OPTIONS.count("verbose"))
  std::cout << "GSM: Received ChatMessage from #" << std::dec << eid << ": \"" << message << "\"" << std::endl;

  sendToAllExceptOne(MAKE_CALLBACK(packetSCChatMessage, message), eid);

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSDisconnect(int32_t eid, std::string message)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Disconnect from #" << std::dec << eid << ": \"" << message << "\"" << std::endl;

  m_connection_manager.safeStop(eid);

  if (m_states.find(eid) == m_states.end()) return;

  m_states[eid]->state = PlayerState::TERMINATED;
}

void GameStateManager::packetCSWindowClick(int32_t eid, int8_t window_id, int16_t slot, int8_t right_click, int16_t action, int16_t item_id, int8_t item_count, int16_t item_uses)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received WindowClick from #" << std::dec << eid << ": [" << (unsigned int)(window_id) << ", " << slot << ", " << (unsigned int)(right_click) << ", "
            << action << ", " << item_id << ", " << (unsigned int)(item_count) << ", " << item_uses << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::packetCSSign(int32_t eid, int32_t X, int16_t Y, int32_t Z, std::string line1, std::string line2, std::string line3, std::string line4)
{
  if (PROGRAM_OPTIONS.count("verbose")) std::cout << "GSM: Received Sign from #" << std::dec << eid << ": [" << X << ", " << Y << ", " << Z << ", \"" << line1 << "\", " << line2 << "\", " << line3 << "\", " << line4 << "]" << std::endl;

  if (m_states.find(eid) == m_states.end()) return;
}



/**** Server to Client ****/


void GameStateManager::packetSCKeepAlive(int32_t eid)
{
  PacketCrafter p(PACKET_KEEP_ALIVE);
  m_connection_manager.sendDataToClient(eid, p.craft(), "<keep alive>");
}

void GameStateManager::packetSCKick(int32_t eid, const std::string & message)
{
  PacketCrafter p(PACKET_DISCONNECT);
  p.addString(message);
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

std::string GameStateManager::rawPacketSCPlayerPositionAndLook(const RealCoords & rc, double stance, float yaw, float pitch, bool on_ground)
{
  PacketCrafter p(PACKET_PLAYER_POSITION_AND_LOOK);
  p.addDouble(rX(rc));  // X
  p.addDouble(rY(rc));  // Y
  p.addDouble(stance);  // stance
  p.addDouble(rZ(rc));  // Z
  p.addFloat(yaw);      // yaw
  p.addFloat(pitch);    // pitch
  p.addBool(on_ground); // on ground
  return p.craft();
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

void GameStateManager::packetSCHoldingChange(int32_t eid, int16_t slot)
{
  PacketCrafter p(PACKET_HOLDING_CHANGE);
  p.addInt16(slot);
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

void GameStateManager::packetSCCollectItem(int32_t eid, int32_t collectee_eid, int32_t collector_eid)
{
  PacketCrafter p(PACKET_COLLECT_ITEM);
  p.addInt32(collectee_eid);    // item EID
  p.addInt32(collector_eid);    // collector EID
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCDestroyEntity(int32_t eid, int32_t e)
{
  PacketCrafter p(PACKET_DESTROY_ENTITY);
  p.addInt32(e);                // item EID
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCChatMessage(int32_t eid, std::string message)
{
  PacketCrafter p(PACKET_CHAT_MESSAGE);
  p.addString(message);
  m_connection_manager.sendDataToClient(eid, p.craft());
}

void GameStateManager::packetSCSpawnEntity(int32_t eid, int32_t e, const FractionalCoords & fc, double rot, double pitch, uint16_t item_id)
{
  PacketCrafter p(PACKET_NAMED_ENTITY_SPAWN);
  p.addInt32(e);
  p.addString(m_connection_manager.getNickname(e));
  p.addInt32(fX(fc));
  p.addInt32(fY(fc));
  p.addInt32(fZ(fc));
  p.addAngleAsByte(rot);
  p.addAngleAsByte(pitch);
  p.addInt16(item_id);
  m_connection_manager.sendDataToClient(eid, p.craft());
}

std::string GameStateManager::rawPacketSCEntityTeleport(int32_t e, const FractionalCoords & fc, double yaw, double pitch)
{
  PacketCrafter p(PACKET_ENTITY_TELEPORT);
  p.addInt32(e);
  p.addInt32(fX(fc));
  p.addInt32(fY(fc));
  p.addInt32(fZ(fc));
  p.addAngleAsByte(yaw);
  p.addAngleAsByte(pitch);
  return p.craft();
}

std::string GameStateManager::rawPacketSCHoldingChange(int16_t slot)
{
  PacketCrafter p(PACKET_HOLDING_CHANGE);
  p.addInt16(slot);
  return p.craft();
}

std::string GameStateManager::rawPacketSCDestroyEntity(int32_t e)
{
  PacketCrafter p(PACKET_DESTROY_ENTITY);
  p.addInt32(e);
  return p.craft();
}
