#ifndef H_PACKETCRAFTER
#define H_PACKETCRAFTER


#include <vector>
#include <string>
#include "packets.h"

static inline void convertToJavaString(std::string & s)
{
  for (std::string::iterator i = s.begin(); i != s.end(); )
  {
    if (*i != 0) { ++i; continue; }

    i = s.erase(i);
    i = s.insert(i, char(0xC0)); ++i;
    i = s.insert(i, char(0x80)); ++i;
  }
}

class PacketCrafter
{
public:
  explicit PacketCrafter(uint8_t type = -1) : m_buffer(1, type) { m_buffer.reserve(512); }

  inline std::string craft() const { return std::string(m_buffer.begin(), m_buffer.end()); }

  inline void setType(int8_t type) { m_buffer[0] = (unsigned char)(type); }


  inline void addBool(bool b)
  {
    m_buffer.push_back(b ? 1 : 0);
  }

  inline void addInt8(uint8_t i)
  {
    m_buffer.push_back(i);
  }

  inline void addInt16(uint16_t i)
  {
    unsigned char b2 = (i >> 8), b1 = (i & 0xFF);
    m_buffer.push_back(b2);
    m_buffer.push_back(b1);
  }

  inline void addInt32(uint32_t i)
  {
    unsigned char b4 = ((i >> 24) & 0xFF), b3 = ((i >> 16) & 0xFF), b2 = ((i >> 8) & 0xFF), b1 = (i & 0xFF);
    m_buffer.push_back(b4);
    m_buffer.push_back(b3);
    m_buffer.push_back(b2);
    m_buffer.push_back(b1);
  }

  inline void addInt64(uint64_t i)
  {
    unsigned char b8 = ((i >> 56) & 0xFF), b7 = ((i >> 48) & 0xFF), b6 = ((i >> 40) & 0xFF), b5 = ((i >> 32) & 0xFF),
                  b4 = ((i >> 24) & 0xFF), b3 = ((i >> 16) & 0xFF), b2 = ((i >>  8) & 0xFF), b1 =  (i        & 0xFF);
    m_buffer.push_back(b8);
    m_buffer.push_back(b7);
    m_buffer.push_back(b6);
    m_buffer.push_back(b5);
    m_buffer.push_back(b4);
    m_buffer.push_back(b3);
    m_buffer.push_back(b2);
    m_buffer.push_back(b1);
  }

  inline void addJString(const std::string & s) { addInt16(s.length()); m_buffer.insert(m_buffer.end(), s.begin(), s.end()); }
  
  inline void addByteArray(const char * data, size_t length) { m_buffer.insert(m_buffer.end(), data, data + length); }

  inline void addDouble(double x)
  {
    char * c = reinterpret_cast<char*>(&x);
    std::vector<char> rev(c, c + 8);
    m_buffer.insert(m_buffer.end(), rev.rbegin(), rev.rend());
  }

  inline void addFloat(float x)
  {
    char * c = reinterpret_cast<char*>(&x);
    std::vector<char> rev(c, c + 4);
    m_buffer.insert(m_buffer.end(), rev.rbegin(), rev.rend());
  }


private:
  std::vector<unsigned char> m_buffer;
};


#endif
