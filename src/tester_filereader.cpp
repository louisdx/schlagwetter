#include <string>
#include <iostream>
#include <iomanip>

#include <zlib.h>
#include <nbt.h>
#include <cstring>

#include "filereader.h"
#include "chunk.h"


static inline void asciiprint(std::ostream & o, unsigned char c)
{
  if (c < 32 || c > 127) o << ".";
  else o << c;
}

static inline void hexdump(std::ostream & o, const unsigned char * buf, size_t length, const std::string & delim)
{
  for (size_t k = 0; 16*k < length; k++)
  {
    o << delim;

    for (int i = 0; i < 16 && 16*k + i < length; ++i)
      o << "0x" << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << (unsigned int)(buf[16*k + i]) << " ";

    if (16*(k+1) > length) for (size_t i = 0; i < 16*(k+1)-length; ++i) o << "     ";

    o << "    ";

    for (size_t i = 0; i < 16 && 16*k + i < length; ++i)  asciiprint(o, buf[16*k + i]);

    o << std::endl;
  }
}

static inline void hexdump(std::ostream & o, const std::string & data, const std::string & delim = "   ")
{
  hexdump(o, reinterpret_cast<const unsigned char*>(data.data()), data.length(), delim);
}

static std::array<unsigned char, 100000> xx;
static std::array<unsigned char, 81920> dump;

int main(int argc, char * argv[])
{
  if (argc < 2) { std::cout << "Usage: tester <filename>" << std::endl; return 0; }

  if (argc == 3)
  {
    std::string s(argv[2]);
    unsigned char buf[200], bufout[200];
    unsigned long int buflen = 200, bufoutlen = 200;
    int res = compress(buf, &buflen, reinterpret_cast<const unsigned char*>(s.data()), s.length());

    std::cout << "Compressing \"" << s << "\" from " << s.length() << "B to " << buflen << "B..." << std::endl;
    if (Z_OK == res) hexdump(std::cout, buf, buflen, "---> ");
    else std::cout << "FAILURE, " << res  << " (should be " << Z_OK << ")." << std::endl;

    std::cout << "Aaaand uncompressing again:" << std::endl;
    res = uncompress(bufout, &bufoutlen, buf, buflen);
    if (Z_OK == res) hexdump(std::cout, bufout, bufoutlen, "---> ");
    else std::cout << "FAILURE, " << res  << " (should be " << Z_OK << ")." << std::endl;
  }

  RegionFile f(argv[1]);
  f.parse(false);

  for (size_t x = 0; x < 32; ++x)
  {
    for (size_t z = 0; z < 32; ++z)
    {
      if (f.chunkSize(x, z) == 0) continue;

      std::string chuck = f.getCompressedChunk(x, z);

      //hexdump(std::cout, chuck);

      if (chuck == "") continue;

      {
        unsigned long int dest_len = xx.size();
        int k = ::uncompress(xx.data(), &dest_len, reinterpret_cast<const unsigned char *>(chuck.data()), chuck.length());
        if (k == Z_OK)
        {
          std::cout << "ZLIB::uncompress() successful, got " << std::dec << dest_len << " bytes:" << std::endl;
          //hexdump(std::cout, xx.data(), dest_len, "==> ");



          // Memory map this!

          nbt_node * root = nbt_parse_compressed(chuck.data(), chuck.length());

          nbt_node* node;

          node = nbt_find_by_path(root, ".Level.HeightMap");
          //memcpy(dump.data(), node->payload.tag_byte_array.data, 256);

          node = nbt_find_by_path(root, ".Level.Blocks");
          memcpy(dump.data() + 0,     node->payload.tag_byte_array.data, 32768);

          node = nbt_find_by_path(root, ".Level.Data");
          memcpy(dump.data() + 32768, node->payload.tag_byte_array.data, 16384);

          node = nbt_find_by_path(root, ".Level.BlockLight");
          memcpy(dump.data() + 49152, node->payload.tag_byte_array.data, 16384);

          node = nbt_find_by_path(root, ".Level.SkyLight");
          memcpy(dump.data() + 65536, node->payload.tag_byte_array.data, 16384);

          nbt_free(root);

          //hexdump(std::cout, dump.data(), dump.size(), "==> ");

          dest_len = xx.size();
          k = ::compress(xx.data(), &dest_len, dump.data(), dump.size());
          if (k == Z_OK) { std::cout << "Compressed into " << std::dec << dest_len << " bytes." << std::endl; }

        }
        else
        {
          std::cout << "ZLIB::uncompress() failed! Error: " << std::dec << k << std::endl;
        }
      }
    }
  }
}
