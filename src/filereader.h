#ifndef H_FILEREADER
#define H_FILEREADER


#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <cstdint>
#include <boost/noncopyable.hpp>

#include "types.h"

#define READ_UINT32(data) (((unsigned int)(*(data+3))) | ((unsigned int)(*(data+2)) << 8) | ((unsigned int)(*(data+1)) << 16) | ((unsigned int)(*(data)) << 24))
#define READ_UINT24(data) (((unsigned int)(*(data+2))) | ((unsigned int)(*(data+1)) << 8) | ((unsigned int)(*(data)) << 16))

class Chunk;

template <class T>
inline std::string to_string (const T & t)
{
  std::stringstream ss;
  ss << t;
  return ss.str();
}

std::shared_ptr<Chunk> NBTExtract(const unsigned char * buf, size_t len, const ChunkCoords & cc);

class RegionFile : private boost::noncopyable
{
public:
  RegionFile();
  RegionFile(const std::string & filename);

  bool open(const std::string & filename);

  void parse(bool print = false);
  std::string getCompressedChunk(size_t x, size_t z);

  /// Local coordiates, 0..31.
  uint32_t chunkOffsetOffset(size_t x, size_t z) const { return 4 * (x + 32 * z); }
  uint32_t chunkTimestampOffset(size_t x, size_t z) const { return 4 * (x + 32 * z) + 4096; }
  uint32_t chunkOffset(size_t x, size_t z) const { return READ_UINT24(&m_header[chunkOffsetOffset(x, z)]) * 4096; }
  uint32_t chunkSize(size_t x, size_t z) const { return (unsigned int)(m_header[chunkOffsetOffset(x, z) + 3]) * 4096; }
  uint32_t chunkTimestamp(size_t x, size_t z) const { return READ_UINT32(m_header.data() + chunkTimestampOffset(x, z)); }

private:
  std::ifstream m_file;
  std::array<unsigned char, 8192> m_header;
  size_t m_size;
};



#endif
