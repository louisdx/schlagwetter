#include <fstream>
#include <iostream>
#include <iterator>
#include <set>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/write.hpp>
#include <boost/iostreams/read.hpp>

#include "serializer.h"


Serializer::Serializer(ChunkMap & chunk_map)
  : m_chunk_map(chunk_map)
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

  if (!idxfile || !datfile)
  {
    std::cerr << "Error while opening save files. Map was NOT saved." << std::endl;
  }

  boost::iostreams::filtering_ostreambuf zidx;
  boost::iostreams::filtering_ostreambuf zdat;

  zidx.push(boost::iostreams::zlib_compressor());
  zdat.push(boost::iostreams::zlib_compressor());

  zidx.push(idxfile);
  zdat.push(datfile);

  std::set<ChunkCoords> s;
  for (auto i = m_chunk_map.begin(); i != m_chunk_map.end(); ++i)
    s.insert(i->first);

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

  std::cout << "saving ... done!" << std::endl;
}

void Serializer::deserialize(const std::string & basename)
{
 std::cout << "Load map..." << std::endl;

  std::ifstream idxfile(basename + ".idx", std::ios::binary);
  std::ifstream datfile(basename + ".dat", std::ios::binary);

  if (!idxfile || !datfile)
  {
    std::cerr << "Error while opening save files. Map was NOT loaded." << std::endl;
  }

  boost::iostreams::filtering_istreambuf zidx;
  boost::iostreams::filtering_istreambuf zdat;

  zidx.push(boost::iostreams::zlib_decompressor());
  zdat.push(boost::iostreams::zlib_decompressor());

  zidx.push(idxfile);
  zdat.push(datfile);

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
  }

  std::cout << "loading ... done!" << std::endl;
}
