#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include "serializer.h"

Serializer::Serializer(ChunkMap & chunk_map)
  : m_chunk_map(chunk_map)
{
}

bool Serializer::haveChunk(const ChunkCoords & cc)
{
  (void)cc;
  return false;
}

ChunkMap::mapped_type Serializer::loadChunk(const ChunkCoords & cc)
{
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

  std::set<ChunkCoords> s;
  for (auto i = m_chunk_map.begin(); i != m_chunk_map.end(); ++i)
    s.insert(i->first);

  for (size_t counter = 0; !s.empty(); ++counter)
  {
    const ChunkCoords & cc = *s.begin();
    int32_t X = cX(cc);
    int32_t Z = cZ(cc);

    std::copy(m_chunk_map[cc]->data().begin(), m_chunk_map[cc]->data().end(), std::ostream_iterator<unsigned char>(datfile));
    idxfile.write(reinterpret_cast<char*>(&X), 4);
    idxfile.write(reinterpret_cast<char*>(&Z), 4);
    idxfile.write(reinterpret_cast<char*>(&counter), 4);

    s.erase(s.begin());
  }

  std::cout << "saving ... done!" << std::endl;
}

void Serializer::deserialize()
{
 std::cout << "Load map..." << std::endl;

  std::ifstream idxfile("/tmp/mymap.idx", std::ios::binary);
  std::ifstream datfile("/tmp/mymap.dat", std::ios::binary);

  if (!idxfile || !datfile)
  {
    std::cerr << "Error while opening save files. Map was NOT loaded." << std::endl;
  }

  std::istream_iterator<unsigned char> iit(datfile);

  for (size_t counter = 0; !idxfile.eof() && !datfile.eof() ; ++counter)
  {
    int32_t X, Z, c;
    idxfile.read(reinterpret_cast<char*>(&X), 4);
    idxfile.read(reinterpret_cast<char*>(&Z), 4);
    idxfile.read(reinterpret_cast<char*>(&c), 4);

    if (c != int(counter)) { std::cerr << "Error while reading map. Everythis is now broken." << std::endl; return; }

    auto chunk = std::make_shared<Chunk>(ChunkCoords(X, Z));
    datfile.read(reinterpret_cast<char *>(chunk->data().data()), chunk->data().size());

    m_chunk_map.insert(std::make_pair(chunk->coords(), chunk));
  }

  std::cout << "loading ... done!" << std::endl;
}
