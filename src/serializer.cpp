#include <fstream>
#include <iostream>
#include <iterator>
#include <set>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>

#include "serializer.h"
#include "cmdlineoptions.h"
#include "map.h"
#include "constants.h"


Serializer::Serializer(ChunkMap & chunk_map, Map & map)
  : m_chunk_map(chunk_map), m_map(map)
{
}

bool Serializer::haveChunk(const ChunkCoords & cc)
{
  // this should say "true" if the chunk is available on the disk
  // but not loaded to memory.
  (void)cc;
  return false;
}

ChunkMap::mapped_type Serializer::loadChunk(const ChunkCoords & cc)
{
  // in the above situation, this should retrieve an individual chunk from the disk.
  return std::make_shared<Chunk>(cc);
}

void Serializer::serialize()
{
  std::cout << "Saving map..." << std::endl;

  std::ofstream idxfile("/tmp/mymap.idx", std::ios::binary);
  std::ofstream datfile("/tmp/mymap.dat", std::ios::binary);
  std::ofstream metfile("/tmp/mymap.meta", std::ios::binary);

  if (!idxfile || !datfile || !metfile)
  {
    std::cerr << "Error while opening save files. Map was NOT saved." << std::endl;
    return;
  }

  boost::iostreams::filtering_ostreambuf zidx;
  boost::iostreams::filtering_ostreambuf zdat;
  boost::iostreams::filtering_ostreambuf zmet;

  zidx.push(boost::iostreams::zlib_compressor());
  zdat.push(boost::iostreams::zlib_compressor());
  zmet.push(boost::iostreams::zlib_compressor());

  zidx.push(idxfile);
  zdat.push(datfile);
  zmet.push(metfile);

  std::set<ChunkCoords> s;
  for (auto i = m_chunk_map.cbegin(); i != m_chunk_map.cend(); ++i)
    s.insert(i->first);

  /* Save map chunk data */
  for (size_t counter = 0; !s.empty(); ++counter)
  {
    const ChunkCoords & cc = *s.begin();
    int32_t X = cX(cc);
    int32_t Z = cZ(cc);

    boost::iostreams::write(zdat, reinterpret_cast<const char*>(m_chunk_map[cc]->data().data()), Chunk::sizeBlockType + Chunk::sizeBlockMetaData);

    boost::iostreams::write(zidx, reinterpret_cast<const char*>(&X), 4);
    boost::iostreams::write(zidx, reinterpret_cast<const char*>(&Z), 4);
    boost::iostreams::write(zidx, reinterpret_cast<const char*>(&counter), 4);

    s.erase(s.begin());
  }

  /* Save map metadata */

  uint32_t tmp = m_map.seed();
  boost::iostreams::write(zmet, reinterpret_cast<const char*>(&tmp), 4);

  tmp = m_map.m_storage.size();
  boost::iostreams::write(zmet, reinterpret_cast<const char*>(&tmp), 4);

  for (auto it = m_map.m_stridx.cbegin(); it != m_map.m_stridx.cend(); ++it)
  {
    uint32_t t = it->second; // UID
    boost::iostreams::write(zmet, reinterpret_cast<const char*>(&t), 4);

    const StorageUnit & su = m_map.m_storage.find(t)->second;

    t = su.type & 0x1F;      // Storage Type
    // later we'll add a privacy flag and double-chestness here:
    // Lower 4 bit: Storage type; Bits 5-7: double-chest orientation; Bit 8: Privacy flag
    boost::iostreams::write(zmet, reinterpret_cast<const char*>(&t), 4);

    t = wX(it->first);
    boost::iostreams::write(zmet, reinterpret_cast<const char*>(&t), 4);

    t = wY(it->first);
    boost::iostreams::write(zmet, reinterpret_cast<const char*>(&t), 4);

    t = wZ(it->first);
    boost::iostreams::write(zmet, reinterpret_cast<const char*>(&t), 4);

    boost::iostreams::write(zmet, reinterpret_cast<const char*>(su.nickhash.data()), su.nickhash.size());
  }

  std::cout << "saving ... done!" << std::endl;
}

void Serializer::deserialize(const std::string & basename)
{
 std::cout << "Load map..." << std::endl;

  std::ifstream idxfile(basename + ".idx", std::ios::binary);
  std::ifstream datfile(basename + ".dat", std::ios::binary);
  std::ifstream metfile(basename + ".meta", std::ios::binary);

  if (!idxfile || !datfile || !metfile)
  {
    std::cerr << "Error while opening save files. Map was NOT loaded." << std::endl;
    return;
  }

  boost::iostreams::filtering_istreambuf zidx;
  boost::iostreams::filtering_istreambuf zdat;
  boost::iostreams::filtering_istreambuf zmet;

  zidx.push(boost::iostreams::zlib_decompressor());
  zdat.push(boost::iostreams::zlib_decompressor());
  zmet.push(boost::iostreams::zlib_decompressor());

  zidx.push(idxfile);
  zdat.push(datfile);
  zmet.push(metfile);

  for (size_t counter = 0; ; ++counter)
  {
    int32_t X, Z, c;
    if (boost::iostreams::read(zidx, reinterpret_cast<char*>(&X), 4) == -1) break;
    if (boost::iostreams::read(zidx, reinterpret_cast<char*>(&Z), 4) == -1) break;
    if (boost::iostreams::read(zidx, reinterpret_cast<char*>(&c), 4) == -1) break;

    auto chunk = std::make_shared<Chunk>(ChunkCoords(X, Z));

    if (boost::iostreams::read(zdat, reinterpret_cast<char *>(chunk->data().data()), Chunk::sizeBlockType + Chunk::sizeBlockMetaData) == -1)
    {
      break;
    }

    if (c != int(counter))
    {
      std::cerr << "Warning: Something unexpected when reading the map. (Read: " << c << ", Expected: " << counter << ")" << std::endl;
    }

    m_chunk_map.insert(std::make_pair(chunk->coords(), chunk));

    /*
    for (size_t x = 0; x < 16; ++x)
    {
      for (size_t z = 0; z < 16; ++z)
      {
        for (size_t y = 0; y < 128; ++y)
        {
          const LocalCoords lc(x, y, z);
          const unsigned char block = chunk->blockType(lc);

          if (block == BLOCK_FurnaceBlock || block == BLOCK_FurnaceBurningBlock)
          {
            m_map.addStorage(getWorldCoords(lc, chunk->coords()), FURNACE);

            std::cout << "Furnace at " << getWorldCoords(lc, chunk->coords()) << ", assigning UID "
                      << m_map.storageIndex(getWorldCoords(lc, chunk->coords())) << "." << std::endl;
          }
          else if (block == BLOCK_DispenserBlock)
          {
            std::cout << "Dispenser at " << getWorldCoords(lc, chunk->coords()) << std::endl;
          }
          else if (block == BLOCK_ChestBlock)
          {
            ChunkMap::const_iterator it;

            if (x > 0 && chunk->blockType(LocalCoords(x-1, y, z)) == BLOCK_ChestBlock)
            {
              std::cout << "Double chest at " << getWorldCoords(lc, chunk->coords())
                        << "/" << getWorldCoords(LocalCoords(x-1, y, z), chunk->coords()) << std::endl;
            }

            else
            {
              std::cout << "Single chest at " << getWorldCoords(lc, chunk->coords()) << std::endl;
            }
          }

        }
      }
    }
    */
  }

  uint32_t tmp, uid, st, x, y, z;
  unsigned char n[20];

  boost::iostreams::read(zmet, reinterpret_cast<char*>(&tmp), 4);
  m_map.seed() = tmp;
  std::cout << "Reading map seed from file: " << std::dec << tmp << std::endl;

  boost::iostreams::read(zmet, reinterpret_cast<char*>(&tmp), 4);

  std::cout << "Reading " << std::dec << tmp << " storage units." << std::endl;
  for (uint32_t i = 0; i < tmp; ++i)
  {
    boost::iostreams::read(zmet, reinterpret_cast<char*>(&uid), 4);
    boost::iostreams::read(zmet, reinterpret_cast<char*>(&st), 4);
    boost::iostreams::read(zmet, reinterpret_cast<char*>(&x), 4);
    boost::iostreams::read(zmet, reinterpret_cast<char*>(&y), 4);
    boost::iostreams::read(zmet, reinterpret_cast<char*>(&z), 4);
    boost::iostreams::read(zmet, reinterpret_cast<char*>(n), 20);

    const WorldCoords wc(x, y, z);

    std::cout << "UID " << uid << " at " << wc << " of type " << st << std::endl;
    m_map.m_stridx[wc] = uid;
    m_map.m_storage[uid].type = EStorage(st);
    std::copy(n, n + 20, m_map.m_storage[uid].nickhash.begin());
  }

  std::cout << "loading ... done!" << std::endl;
}
