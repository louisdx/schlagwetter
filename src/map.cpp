#include <iostream>

#include "map.h"
#include "generator.h"

uint32_t INVENTORY_UID_POOL = 2875; // let's start somewhere random

Map::Map(unsigned long long int ticks, int seed)
  :
  tick_counter(ticks),
  m_chunks(),
  m_items(),
  m_serializer(m_chunks, *this),
  m_seed(seed)
{
}

void Map::ensureChunkIsLoaded(const ChunkCoords & cc)
{
  if (m_chunks.count(cc) == 0)
  {
    if (m_serializer.haveChunk(cc) == true)
    {
      m_chunks.insert(ChunkMap::value_type(cc, m_serializer.loadChunk(cc))).first;
    }
    else
    {
      std::cout << "** generating chunk **" << std::endl;
      auto ins = m_chunks.insert(ChunkMap::value_type(cc, std::make_shared<Chunk>(cc))).first;
      generateWithNoise(*ins->second, cc);
    }
  }
}

void Map::addStorage(const WorldCoords & wc, uint8_t block_type)
{
  if (m_stridx.find(wc) != m_stridx.end())
  {
    std::cout << "Trying to add map storage unit at already occupied location" << wc << "." << std::endl;
    return;
  }

  switch (block_type)
  {
  case BLOCK_FurnaceBlock:
  case BLOCK_FurnaceBurningBlock:
    {
      const uint32_t uid = GenerateInventoryUID();
      m_stridx.insert(StorageIndex::value_type(wc, uid));
      m_storage[uid].type = FURNACE;
      m_storage[uid].inventory.clear();
      break;
    }
  case BLOCK_DispenserBlock:
    {
      const uint32_t uid = GenerateInventoryUID();
      m_stridx.insert(StorageIndex::value_type(wc, uid));
      m_storage[uid].type = DISPENSER;
      m_storage[uid].inventory.clear();
      break;
    }

  case BLOCK_ChestBlock:
    {
      const uint32_t uid = GenerateInventoryUID();

      // Must check if we have a new single chest or if we are converting a single to a double chest

      break;
    }
  }

}

void Map::addStorage(const WorldCoords & wc, EStorage type)
{
  if (m_stridx.find(wc) != m_stridx.end())
  {
    std::cout << "Trying to add map storage unit at already occupied location" << wc << "." << std::endl;
    return;
  }

  const uint32_t uid = GenerateInventoryUID();
  m_stridx.insert(StorageIndex::value_type(wc, uid));
  m_storage[uid].type = type;
  m_storage[uid].inventory.clear();
}


// TODO: have to decide who handles the inventory of the destroyed unit
void Map::removeStorage(const WorldCoords & wc)
{
  StorageIndex::iterator iit = m_stridx.find(wc);

  if (iit == m_stridx.end())
  {
    std::cout << "Trying to remove non-existing map storage unit at location " << wc << "." << std::endl;
    return;
  }

  const uint32_t uid = iit->second;

  MapStorage::iterator mit = m_storage.find(uid);

  if (mit == m_storage.end())
  {
    std::cout << "Inconsistent map storage data, cleaning up..." << std::endl;
    m_stridx.erase(iit);
    return;
  }

  if (mit->second.type == FURNACE || mit->second.type == DISPENSER  || mit->second.type == SINGLE_CHEST)
  {
    m_storage.erase(mit);
    m_stridx.erase(iit);
    return;
  }

  if (mit->second.type == DOUBLE_CHEST)
  {
    // need to add some code to create a suitable single chest
  }

}
