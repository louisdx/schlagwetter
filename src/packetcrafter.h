#ifndef H_PACKETCRAFTER
#define H_PACKETCRAFTER


#include <vector>
#include <string>
#include "constants.h"


/// This is now obsolete; strings are encoded as UTF16.

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

/// The new string converter. Evil hack, not real. Must use something better like iconv(). Only works for ASCII!

static inline uint16_t BE_UTF16FromChar(char c)
{
  return 0x0000 | ((unsigned int)(unsigned char)(c) & 0xFF);
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

  inline void addJString(const std::string & s)
  {
    addInt16(s.length());
    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it)
      addInt16(BE_UTF16FromChar(*it));
  }
  
  inline void addByteArray(const char * data, size_t length) { m_buffer.insert(m_buffer.end(), data, data + length); }

  inline void addDouble(double x)
  {
    unsigned char * c = reinterpret_cast<unsigned char*>(&x);
    std::vector<unsigned char> rev(c, c + 8);
    m_buffer.insert(m_buffer.end(), rev.rbegin(), rev.rend());
  }

  inline void addFloat(float x)
  {
    unsigned char * c = reinterpret_cast<unsigned char*>(&x);
    std::vector<unsigned char> rev(c, c + 4);
    m_buffer.insert(m_buffer.end(), rev.rbegin(), rev.rend());
  }

  inline void addAngleAsByte(double angle)
  {
    m_buffer.push_back((char)(int)(angle / 360. * 256.));
  }


private:
  std::vector<unsigned char> m_buffer;
};


#endif
