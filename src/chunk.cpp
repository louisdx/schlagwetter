#include <iostream>
#include <mutex>
#include <zlib.h>
#include "chunk.h"

static std::array<unsigned char, 100000> zlib_buffer;
static std::mutex zlib_buffer_mutex;


Chunk::Chunk(const ChunkCoords & cc)
  :
  m_coords(cc),
  m_data(),
  m_heightmap(),
  m_zcache(m_coords)
{
}

Chunk::Chunk(const ChunkCoords & cc, const ChunkData & data, const ChunkHeightMap & hm)
  :
  m_coords(cc),
  m_data(data),
  m_heightmap(hm),
  m_zcache(m_coords)
{
}

/// This version uses global memory to hold the deflated data and returns a copy.
std::string Chunk::compress() const
{
  /// Our chunks are always 80KiB, so we assume that 100KB suffice for the output and skip the bound check.
  //unsigned long int outlength = compressBound(size());
  //unsigned char * buf = new unsigned char[outlength];
  //...
  //delete[] buf;

  std::string result;

  if (zlib_buffer_mutex.try_lock())
  {
    try
    {
      unsigned long int outlength = zlib_buffer.size();
      int zres = ::compress(zlib_buffer.data(), &outlength, m_data.data(), size());
      if (zres != Z_OK)
      {
        std::cerr << "Error during zlib deflate!" << std::endl;
      }
      else
      {
        std::cout << "zlib deflate: " << std::dec << outlength << std::endl;
        result = std::string(reinterpret_cast<char*>(zlib_buffer.data()), outlength);
      }
    }
    catch (...)
    {
      std::cout << "Exception caught during ZLIB compression!" << std::endl;
    }

    zlib_buffer_mutex.unlock();
  }
  else
  {
    std::cout << "PANIC: Access to static ZLIB buffer locked, you MUST rework the threading model." << std::endl;
  }

  return result;
}


/// This version uses a (rather small) per-chunk zip cache.
std::pair<const unsigned char *, size_t> Chunk::compress_beefedup()
{
  if (m_zcache.usable)
  {
    std::cout << "cached zlib deflate cache hit! (" << std::dec << m_zcache.length << " bytes)" << std::endl;
    return std::make_pair(m_zcache.cache.data(), m_zcache.length + 18);
  }

  unsigned long int outlength = m_zcache.cache.size() - 18;
  int zres = ::compress(m_zcache.cache.data() + 18, &outlength, m_data.data(), size());
  if (zres != Z_OK)
  {
    std::cerr << "Error during beefed-up zlib deflate!" << std::endl;
    outlength = 0;
  }
  else
  {
    std::cout << "caching zlib deflate: " << std::dec << outlength << std::endl;
    m_zcache.usable = true;
  }

  m_zcache.writeLength(outlength);
  m_zcache.length = outlength;

  return std::make_pair(m_zcache.cache.data(), m_zcache.length + 18);
}
