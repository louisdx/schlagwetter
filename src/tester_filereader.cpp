#include <string>
#include <iostream>
#include <iomanip>
#include <zlib.h>
#include "filereader.h"

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
  f.parse();

  for (size_t x = 0; x < 32; ++x)
  {
    for (size_t z = 0; z < 32; ++z)
    {
      if (f.chunkSize(x, z) == 0) continue;

      std::string chuck = f.getCompressedChunk(x, z);

      hexdump(std::cout, chuck);

      if (chuck == "") continue;

      {
        unsigned char * xx = new unsigned char[100000];
        unsigned long int dest_len = 100000;
        int k = ::uncompress(xx, &dest_len, reinterpret_cast<const unsigned char *>(chuck.data()), chuck.length());
        if (k == Z_OK)
        {
          std::cout << "ZLIB::uncompress() successful, got " << std::dec << dest_len << " bytes:" << std::endl;
          hexdump(std::cout, xx, dest_len, "==> ");
        }
        else
        {
          std::cout << "ZLIB::uncompress() failed! Error: " << std::dec << k << std::endl;
        }

        delete[] xx;
      }
    }
  }
}
