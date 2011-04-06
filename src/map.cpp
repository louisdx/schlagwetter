#include <iostream>
#include <zlib.h>
#include "constants.h"

#include "map.h"
#include "generator.h"
#include "packetcrafter.h"

std::string Chunk::compress() const
{
  unsigned long int outlength = compressBound(size());
  unsigned char * buf = new unsigned char[outlength];
  int zres = ::compress(buf, &outlength, m_data.data(), size());
  if (zres != Z_OK)
  {
    std::cerr << "Error during zlib deflate!" << std::endl;
    delete[] buf;
    return "";
  }
  else
  {
    std::cout << "zlib deflate: " << std::dec << outlength << std::endl;
    std::string r(reinterpret_cast<char*>(buf), outlength);
    delete[] buf;
    return r;
  }
}

Map::Map()
  :
  m_chunkMap()
{
}

const Chunk & Map::generateRandomChunk(const ChunkCoords & cc)
{
  auto ins = m_chunkMap.insert(ChunkMap::value_type(cc, Chunk()));

  if (ins.second) generateWithNoise(ins.first->second, cc);

  return ins.first->second;
}


std::string makeRandomChunkPacket(int32_t X, int32_t Z)
{
  Chunk chuck;
  generateWithNoise(chuck, ChunkCoords(X, Z));
  const std::string zhuck = chuck.compress();

  PacketCrafter p(PACKET_MAP_CHUNK);
  p.addInt32(X);    // X
  p.addInt16(0);    // Y
  p.addInt32(Z);    // Z
  p.addInt8(15);
  p.addInt8(127);
  p.addInt8(15);
  p.addInt32(zhuck.length());
  p.addByteArray(zhuck.data(), zhuck.length());
  return p.craft();
}
