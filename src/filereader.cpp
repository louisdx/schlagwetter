#include <iostream>
#include <iomanip>
#include <mutex>
#include <cstring>

#include <zlib.h>
#include <nbt.h>

#include "filereader.h"
#include "map.h"


static Chunk::ChunkData      chunk_dump;
static Chunk::ChunkHeightMap chunk_heightmap;
static std::mutex            chunk_mutex;


RegionFile::RegionFile()
  :
  m_file()
{
}

RegionFile::RegionFile(const std::string & s)
 :
  m_file(s, std::ios_base::in | std::ios_base::binary)
{
  if (!m_file) std::cerr << "Error: Could not open file \"" << s << "\"." << std::endl;
}


bool RegionFile::open(const std::string & filename)
{
  m_file.open(filename, std::ios_base::in | std::ios_base::binary);

  return m_file;
}

void RegionFile::parse(bool print)
{
  if (!m_file) return;

  m_file.seekg(0, std::ios::end);
  m_size = m_file.tellg();
  m_file.seekg(0, std::ios::beg);

  if (m_size < m_header.size())
  {
    std::cerr << "Error: File is not good." << std::endl;
    return;
  }

  m_file.read(reinterpret_cast<char*>(m_header.data()), m_header.size());

  if (print)
    for (size_t z = 0; z < 32; ++z)
      for (size_t x = 0; x < 32; ++x)
        std::cout << "Chunk [" << x << ", " << z << "]: Offset = " << chunkOffset(x, z) << " (at " << chunkOffsetOffset(x, z)
                  << "), max chunk size = " << chunkSize(x, z) << ", timestamp = " << chunkTimestamp(x, z) << std::endl;

}

std::string RegionFile::getCompressedChunk(size_t x, size_t z)
{
  unsigned char b[4], c;
  m_file.seekg(chunkOffset(x, z), std::ios::beg);
  m_file.read(reinterpret_cast<char*>(b), 4);
  m_file.read(reinterpret_cast<char*>(&c), 1);

  uint32_t size = READ_UINT32(b);

  std::cout << "[" << x << ", " << z << "] About to read a chunk of size " << size
            << " at offset " << chunkOffset(x, z) << ", compression type is " << (unsigned int)(c) << std::endl;

  if (chunkOffset(x, z) + 4 + size >= m_size || size > 1000000)
  {
    std::cerr << "Error: File is not good." << std::endl;
    return "";
  }

  char * buf = new char[size - 1];

  m_file.read(buf, size - 1);

  if (c != 2)
  {
    std::cout << "Warning: Compression format " << std::dec << (unsigned int)(unsigned char)(buf[0]) << " not implemented." << std::endl;
  }

  std::string res(buf, size - 1);

  delete[] buf;

  return res;
}

std::shared_ptr<Chunk> NBTExtract(const unsigned char * buf, size_t len, const ChunkCoords & cc)
{
  std::shared_ptr<Chunk> result;

  if (chunk_mutex.try_lock())
  {
    try
    {
      nbt_node * node, * root = nbt_parse_compressed(buf, len);

      node = nbt_find_by_path(root, ".Level.HeightMap");
      memcpy(chunk_heightmap.data(), node->payload.tag_byte_array.data, 256);

      node = nbt_find_by_path(root, ".Level.Blocks");
      memcpy(chunk_dump.data() + 0,     node->payload.tag_byte_array.data, 32768);

      node = nbt_find_by_path(root, ".Level.Data");
      memcpy(chunk_dump.data() + 32768, node->payload.tag_byte_array.data, 16384);

      node = nbt_find_by_path(root, ".Level.BlockLight");
      memcpy(chunk_dump.data() + 49152, node->payload.tag_byte_array.data, 16384);

      node = nbt_find_by_path(root, ".Level.SkyLight");
      memcpy(chunk_dump.data() + 65536, node->payload.tag_byte_array.data, 16384);

      nbt_free(root);

      result = std::make_shared<Chunk>(cc, chunk_dump, chunk_heightmap);
    }
    catch (...)
    {
      std::cout << "Exception caught during NBT parsing!" << std::endl;
    }

    chunk_mutex.unlock();
  }
  else
  {
    std::cout << "PANIC: Access to static chunk buffer locked, you MUST rework the threading model." << std::endl;
  }

  return result;
}
