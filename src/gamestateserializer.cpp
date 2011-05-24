#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>

#include <fstream>

#include "gamestatemanager.h"
#include "cmdlineoptions.h"
#include "sha1.h"

/// Serialization functions must store player's position and inventory (and perhaps non-inventory private property).

void GameStateManager::serializePlayer(int32_t eid)
{
  if (m_states.find(eid) == m_states.end()) return;
}

void GameStateManager::deserializePlayer(int32_t eid)
{
  const std::string name = m_connection_manager.getNickname(eid);
  const std::string nickhash = sha1::calcToString(name.data(), name.length());

  if (PROGRAM_OPTIONS.count("verbose")) 
  {
    std::cout << "Player nickname is \"" << name << "\", hash: 0x" << std::hex << std::uppercase;
    for (size_t i = 0; i < nickhash.length(); ++i)
      std::cout << std::setw(2) << std::setfill('0') << (unsigned int)(unsigned char)(nickhash[i]);
    std::cout << ". Attempting to load player data from disk...";
  }

  PlayerState & player = *m_states[eid];

  std::ifstream playerfile("/tmp/mymap.player." + nickhash, std::ios::binary);

  if (!playerfile)
  {
    if (PROGRAM_OPTIONS.count("verbose"))
    {
      std::cout << " failed. Setting default start data." << std::endl;
    }

    const WorldCoords start_pos(8, 80, 8);

    player.position = RealCoords(wX(start_pos) + 0.5, wY(start_pos) + 0.5, wZ(start_pos) + 0.5);
    player.stance   = wY(start_pos) + 0.5;

    player.setInv(37, ITEM_DiamondPickaxe, 1, 0);
    player.setInv(36, BLOCK_Torch, 50, 0);
    player.setInv(29, ITEM_Coal, 50, 0);
    player.setInv(21, BLOCK_Cobblestone, 60, 0);
    player.setInv(22, BLOCK_IronOre, 60, 0);
    player.setInv(30, BLOCK_Wood, 50, 0);
    player.setInv(38, ITEM_DiamondShovel, 1, 0);
    player.setInv(39, BLOCK_BrickBlock, 64, 0);
    player.setInv(40, BLOCK_Stone, 64, 0);
    player.setInv(41, BLOCK_Glass, 64, 0);
    player.setInv(42, BLOCK_WoodenPlank, 64, 0);
    player.setInv(44, BLOCK_Obsidian, 64, 0);
    player.setInv(43, ITEM_Bucket, 1, 0);

    player.holding = 4;
  }
  else
  {
    if (PROGRAM_OPTIONS.count("verbose"))
    {
      std::cout << " done!" << std::endl;
    }

    const WorldCoords start_pos(8, 80, 8);

    player.position = RealCoords(wX(start_pos) + 0.5, wY(start_pos) + 0.5, wZ(start_pos) + 0.5);
    player.stance   = wY(start_pos) + 0.5;
  }
    
}
