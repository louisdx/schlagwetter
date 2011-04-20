#ifndef H_INPUTHELPER
#define H_INPUTHELPER


#include <list>
#include <deque>
#include <vector>
#include <string>
#include <algorithm>

#include "syncqueue.h"



typedef struct _twobytestring_t
{
  unsigned char byte1;
  unsigned char byte2;
} twobytestring_t;

typedef union t_codepoint_tmp
{
  char c[7];
  unsigned char u[7];
} t_codepoint;

/** Creates a UTF-8 representation of a single unicode codepoint.
 *  Implements the Prosser-Thompson scheme, szOut.c is always null-terminated.
 */
void codepointToUTF8(unsigned int cp, t_codepoint * szOut)
{
  std::fill(szOut->u, szOut->u + 7, 0);

  // We don't need this. It's just for information.
  /*
  if      (cp < 0x00000080) len = 1;
  else if (cp < 0x00000800) len = 2;
  else if (cp < 0x00010000) len = 3;
  else if (cp < 0x04000000) len = 4;
  else if (cp < 0x80000000) len = 5;
  else                      len = 6;
  */

  size_t i = 0;

  if (cp < 0x80) szOut->u[i] = cp & 0x7F;

  else if (cp < 0x800)
  {
    szOut->u[i++] = 0xc0 | (cp >> 6);
    szOut->u[i++] = 0x80 | (cp & 0x3F);
  }

  else if (cp < 0x10000)
  {
    szOut->u[i++] = 0xE0 | ( cp >> 12         );
    szOut->u[i++] = 0x80 | ((cp >>  6) & 0x3F );
    szOut->u[i++] = 0x80 | ( cp        & 0x3F );
  }

  else if (cp < 0x4000000)
  {
    szOut->u[i++] = 0xF0 | ( cp >> 18         );
    szOut->u[i++] = 0x80 | ((cp >> 12) & 0x3F );
    szOut->u[i++] = 0x80 | ((cp >>  6) & 0x3F );
    szOut->u[i++] = 0x80 | ( cp        & 0x3F );
  }

  else if (cp < 0x80000000)
  {
    szOut->u[i++] = 0xF8 | ( cp >> 24         );
    szOut->u[i++] = 0x80 | ((cp >> 18) & 0x3F );
    szOut->u[i++] = 0x80 | ((cp >> 12) & 0x3F );
    szOut->u[i++] = 0x80 | ((cp >>  6) & 0x3F );
    szOut->u[i++] = 0x80 | ( cp        & 0x3F );
  }

  else
  {
    szOut->u[i++] = 0xFC | ( cp >> 30         );
    szOut->u[i++] = 0x80 | ((cp >> 24) & 0x3F );
    szOut->u[i++] = 0x80 | ((cp >> 18) & 0x3F );
    szOut->u[i++] = 0x80 | ((cp >> 12) & 0x3F );
    szOut->u[i++] = 0x80 | ((cp >>  6) & 0x3F );
    szOut->u[i++] = 0x80 | ( cp        & 0x3F );
  }
}



/* The #defines are for reading from a std::vector,
 *  the read*() functions are for reversible reading from the deque.
 */

#define READ_INT8(data, i)   ((unsigned int)(data[i]))
#define READ_INT16(data, i)  (int)((unsigned int)(data[i+1]) | ((unsigned int)(data[i]) << 8))
#define READ_INT32(data, i)  (int)((unsigned int)(data[i+3]) | (unsigned int)(data[i+2]) << 8 | (unsigned int)(data[i+1]) << 16 | ((unsigned int)(data[i]) << 24))
#define READ_BOOL(data, i)   (data[i] != 0)
#define READ_ANGLEFROMBYTE(data, i)   (double(data[i]) / 256. * 360.)

static inline double READ_DOUBLE(const std::vector<unsigned char> & data, size_t i)
{
  double y;
  char * c = reinterpret_cast<char*>(&y);
  c[7] = data[i]; c[6] = data[i+1]; c[5] = data[i+2]; c[4] = data[i+3]; c[3] = data[i+4]; c[2] = data[i+5]; c[1] = data[i+6]; c[0] = data[i+7];
  return y;
}

static inline float READ_FLOAT(const std::vector<unsigned char> & data, size_t i)
{
  float y;
  char * c = reinterpret_cast<char*>(&y);
  c[3] = data[i]; c[2] = data[i+1]; c[1] = data[i+2]; c[0] = data[i+3];
  return y;
}

static inline int8_t readInt8(std::shared_ptr<SyncQueue> q, std::list<unsigned char> & journal)
{
  char c = q->pop_unsafe(); journal.push_back(c);
  return c;
}

static inline int16_t readInt16(std::shared_ptr<SyncQueue> q, std::list<unsigned char> & journal)
{
  char c;
  uint16_t r = 0;
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint16_t)(unsigned char)(c) <<  8);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint16_t)(unsigned char)(c) <<  0);
  return int16_t(r);
}

static inline int32_t readInt32(std::shared_ptr<SyncQueue> q, std::list<unsigned char> & journal)
{
  char c;
  uint32_t r = 0;
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) << 24);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) << 16);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) <<  8);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) <<  0);
  return int32_t(r);
}

static inline int64_t readInt64(std::shared_ptr<SyncQueue> q, std::list<unsigned char> & journal)
{
  char c;
  uint64_t r = 0;
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 56);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 48);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 40);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 32);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 24);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 16);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) <<  8);
  c = q->pop_unsafe(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) <<  0);
  return int64_t(r);
}

/// This does not actually convert the Java null-byte surrogate back to 0, it's just
/// a function to deal with pre-1.5 strings (ordinary 8-bit strings, modified-UTF8-encoded).
/// Since we use UTF8 internally, there's nothing to do.
static inline std::string readJString(std::shared_ptr<SyncQueue> q, std::list<unsigned char> & journal, uint16_t len)
{
  std::string r(len, 0);
  for (size_t i = 0; i < len; ++i)
  {
    r[i] = q->pop_unsafe();
    journal.push_back(r[i]);
  }
  return r;
}

static inline std::string readString(std::shared_ptr<SyncQueue> q, std::list<unsigned char> & journal, uint16_t len)
{
  t_codepoint     ccp;
  twobytestring_t cbuf;
  std::string     s;
  size_t          n = len;

  while (n--)
  {
    cbuf.byte1 = q->pop_unsafe();
    cbuf.byte2 = q->pop_unsafe();

    journal.push_back(cbuf.byte1);
    journal.push_back(cbuf.byte2);

    codepointToUTF8(((unsigned int)(cbuf.byte1)<<8) | ((unsigned int)(cbuf.byte2)), &ccp);
    s += std::string(ccp.c);
  }

  return s;
}

static inline void rewindJournal(std::shared_ptr<SyncQueue> q, std::list<unsigned char> & journal)
{
  q->pushFrontUnsafe(journal.begin(), journal.end());
}


#endif
