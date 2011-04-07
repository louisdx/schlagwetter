#include <iostream>
#include <list>
#include "inputparser.h"
#include "packets.h"


/* The #defines are for reading from a std::vector,
 *  the read*() functions are for reversible reading from the deque.
 */

#define READ_INT8(data, i)   ((unsigned int)(data[i]))
#define READ_INT16(data, i)  (int)((unsigned int)(data[i+1]) | ((unsigned int)(data[i]) << 8))
#define READ_INT32(data, i)  (int)((unsigned int)(data[i+3]) | (unsigned int)(data[i+2]) << 8 | (unsigned int)(data[i+1]) << 16 | ((unsigned int)(data[i]) << 24))
#define READ_BOOL(data, i)   (data[i] != 0)

static inline double READ_DOUBLE(const std::vector<char> & data, size_t i)
{
  double y;
  char * c = reinterpret_cast<char*>(&y);
  c[7] = data[i]; c[6] = data[i+1]; c[5] = data[i+2]; c[4] = data[i+3]; c[3] = data[i+4]; c[2] = data[i+5]; c[1] = data[i+6]; c[0] = data[i+7];
  return y;
}

static inline float READ_FLOAT(const std::vector<char> & data, size_t i)
{
  float y;
  char * c = reinterpret_cast<char*>(&y);
  c[3] = data[i]; c[2] = data[i+1]; c[1] = data[i+2]; c[0] = data[i+3];
  return y;
}

static inline int8_t readInt8(std::deque<char> & q, std::list<char> journal)
{
  char c = q.front(); journal.push_back(c); q.pop_front();
  return c;
}

static inline int16_t readInt16(std::deque<char> & q, std::list<char> journal)
{
  char c;
  uint16_t r = 0;
  c = q.front(); journal.push_back(c); r |= ((uint16_t)(unsigned char)(c) <<  8); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint16_t)(unsigned char)(c) <<  0); q.pop_front();
  return int16_t(r);
}

static inline int32_t readInt32(std::deque<char> & q, std::list<char> journal)
{
  char c;
  uint32_t r = 0;
  c = q.front(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) << 24); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) << 16); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) <<  8); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) <<  0); q.pop_front();
  return int32_t(r);
}

static inline int64_t readInt64(std::deque<char> & q, std::list<char> journal)
{
  char c;
  uint64_t r = 0;
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 56); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 48); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 40); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 32); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 24); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 16); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) <<  8); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) <<  0); q.pop_front();
  return int64_t(r);
}

static inline std::string readString(std::deque<char> & q, std::list<char> journal, uint16_t len)
{
  std::string r(len, 0);
  for (size_t i = 0; i < len; ++i)
  {
    r[i] = q.front();
    journal.push_back(q.front());
    q.pop_front();
  }
  return r;
}

static inline void rewindJournal(std::deque<char> & q, std::list<char> journal)
{
  q.insert(q.begin(), journal.begin(), journal.end());
}


InputParser::InputParser(GameStateManager & gsm)
  : m_gsm(gsm)
{
}

void InputParser::immediateDispatch(int32_t eid, const std::vector<char> & data)
{
  const char type = data[0];

  switch (type)
  {

  case (PACKET_KEEP_ALIVE):
  {
    m_gsm.packetCSKeepAlive(eid);
    break;
  }

  case (PACKET_USE_ENTITY):
  {
    const int32_t e = READ_INT32(data, 1);
    const int32_t target = READ_INT32(data, 5);
    const bool leftclick = (data[9] != 0);
    m_gsm.packetCSUseEntity(eid, e, target, leftclick);
    break;
  }

  case (PACKET_PLAYER): // whether or not the player is on the ground
  {
    const bool b = READ_BOOL(data, 1);
    m_gsm.packetCSPlayer(eid, b);
    break;
  }

  case (PACKET_PLAYER_POSITION):
  {
    const double X = READ_DOUBLE(data, 1), Y = READ_DOUBLE(data, 9), Z = READ_DOUBLE(data, 25), stance = READ_DOUBLE(data, 17);
    const bool b = READ_BOOL(data, 33);
    m_gsm.packetCSPlayerPosition(eid, X, Y, Z, stance, b);
    break;
  }

  case (PACKET_PLAYER_LOOK):
  {
    const float yaw = READ_FLOAT(data, 1), pitch = READ_FLOAT(data, 5);
    const bool b = READ_BOOL(data, 9);
    m_gsm.packetCSPlayerLook(eid, yaw, pitch, b);
    break;
  }

  case (PACKET_PLAYER_POSITION_AND_LOOK):
  {
    const double X = READ_DOUBLE(data, 1), Y = READ_DOUBLE(data, 17), Z = READ_DOUBLE(data, 25), stance = READ_DOUBLE(data, 9);
    const float yaw = READ_FLOAT(data, 33), pitch = READ_FLOAT(data, 37);
    const bool b = READ_BOOL(data, 41);
    m_gsm.packetCSPlayerPositionAndLook(eid, X, Y, Z, stance, yaw, pitch, b);
    break;
  }

  case (PACKET_PLAYER_DIGGING):
  {
    const uint8_t status = READ_INT8(data, 1), Y = READ_INT8(data, 6), face = READ_INT8(data, 11);
    const int32_t X = READ_INT32(data, 2), Z = READ_INT32(data, 7);
    m_gsm.packetCSPlayerDigging(eid, X, Y, Z, status, face);
    break;
  }

  case (PACKET_HOLDING_CHANGE):
  {
    const int16_t slot = READ_INT16(data, 1);
    m_gsm.packetCSHoldingChange(eid, slot);
    break;
  }

  case (PACKET_ARM_ANIMATION):
  {
    const int32_t e = READ_INT32(data, 1);
    const int8_t animate = READ_INT8(data, 5);
    m_gsm.packetCSArmAnimation(eid, e, animate);
    break;
  }

  case (PACKET_ENTITY_CROUCH):
  {
    const int32_t e = READ_INT32(data, 1);
    const int8_t action = READ_INT8(data, 5);     // 1 = crouch, 2 = uncrouch, 3 = leave bed
    m_gsm.packetCSEntityCrouchBed(eid, e, action);
    break;
  }

  case (PACKET_PICKUP_SPAWN):
  {
    const int32_t e = READ_INT32(data, 1), X = READ_INT32(data, 10), Y = READ_INT32(data, 14), Z = READ_INT32(data, 18);
    const int16_t item = READ_INT16(data, 5), sdata = READ_INT16(data, 8);
    const int8_t count = READ_INT8(data, 7), rotp = READ_INT8(data, 22), pitchp = READ_INT8(data, 23), rollp = READ_INT8(data, 24);
    m_gsm.packetCSPickupSpawn(eid, e, X, Y, Z, rotp, pitchp, rollp, count, item, sdata);
    break;
  }

  case (PACKET_RESPAWN):
  {
    m_gsm.packetCSRespawn(eid);
    break;
  }

  case (PACKET_INVENTORY_CLOSE):
  {
    const int8_t window_id = READ_INT8(data, 1);
    m_gsm.packetCSCloseWindow(eid, window_id);
    break;
  }

  case (PACKET_TRANSACTION):
  {
    /*
    const int8_t window_id = READ_INT8(data, 1);
    const int16_t action_number = READ_INT16(data, 2);
    const bool accepted = READ_BOOL(data, 4);
    */
    
    /* We may ignore this client packet. */
    std::cout << "Transaction packet received from #" << eid << ", consider implementing." << std::endl;
    break;
  }

  default:
    std::cerr << "Unknown fixed-width data field: Type " << (unsigned int)(type) << ", length " << data.size() << std::endl;
  }
}

#define GUARDLOCK  std::lock_guard<std::recursive_mutex> lock(*ptr_mutex)

bool InputParser::dispatchIfEnoughData(int32_t eid, std::deque<char> & queue, std::recursive_mutex * ptr_mutex)
{
  const unsigned int type = (unsigned int)(unsigned char)(queue.front());
  std::list<char> tmp;

  switch (type)
  {

  case (PACKET_LOGIN_REQUEST):
  {
    if (queue.size() < 7) return false;

    int32_t     protocol_version;
    int16_t     username_len;
    std::string username;
    int16_t     password_len;
    std::string password;
    int64_t     map_seed;
    int8_t      dimension;

    {
      GUARDLOCK;

      readInt8(queue, tmp);
      protocol_version = readInt32(queue, tmp);
      username_len     = readInt16(queue, tmp);
      if (int(queue.size()) < username_len) { rewindJournal(queue, tmp); break; }
      username         = readString(queue, tmp, username_len);
      password_len     = readInt16(queue, tmp);
      if (int(queue.size()) < password_len) { rewindJournal(queue, tmp); break; }
      password         = readString(queue, tmp, password_len);
      if (int(queue.size()) < 9) { rewindJournal(queue, tmp); break; }
      map_seed         = readInt64(queue, tmp);
      dimension        = readInt8(queue, tmp);
    }

    m_gsm.packetCSLoginRequest(eid, protocol_version, username, password, map_seed, dimension);

    return true;
  }

  case (PACKET_HANDSHAKE):
  case (PACKET_CHAT_MESSAGE):
  case (PACKET_DISCONNECT):
  {
    if (queue.size() < 3) return false;

    int16_t str_len;
    std::string str;

    {
      GUARDLOCK;

      readInt8(queue, tmp);
      str_len = readInt16(queue, tmp);
      if (int(queue.size()) < str_len) { rewindJournal(queue, tmp); break; }
      str = readString(queue, tmp, str_len);
    }

    switch (type)
    {
    case (PACKET_HANDSHAKE):    { m_gsm.packetCSHandshake(eid, str); return true; }
    case (PACKET_CHAT_MESSAGE): { m_gsm.packetCSChatMessage(eid, str); return true; }
    case (PACKET_DISCONNECT):   { m_gsm.packetCSDisconnect(eid, str); return true; }
    }   

    break;
  }

  case (PACKET_PLAYER_BLOCK_PLACEMENT):
  {
    if (queue.size() < 13) return false;

    int32_t X, Z;
    int8_t Y, direction, amount = 0;
    int16_t block_id, damage = 0;

    {
      GUARDLOCK;

      readInt8(queue, tmp);
      X = readInt32(queue, tmp);
      Y = readInt8 (queue, tmp);
      Z = readInt32(queue, tmp);
      direction = readInt8(queue, tmp);
      block_id = readInt16(queue, tmp);
      if (block_id >= 0)
      {
        if (queue.size() < 16) { rewindJournal(queue, tmp); break; }
        amount = readInt8(queue, tmp);
        damage = readInt16(queue, tmp);
      }
    }

    m_gsm.packetCSBlockPlacement(eid, X, Y, Z, direction, block_id, amount, damage);

    return true;
  }

  case (PACKET_INVENTORY_CHANGE):
  {
    if (queue.size() < 9) return false;

    int8_t window_id, right_click, item_count = 0;
    int16_t slot, action, item_id, item_uses = 0;

    {
      GUARDLOCK;

      readInt8(queue, tmp);
      window_id = readInt8(queue, tmp);
      slot = readInt16(queue, tmp);
      right_click = readInt8(queue, tmp);
      action = readInt16(queue, tmp);
      item_id = readInt16(queue, tmp);
      if (item_id != -1)
      {
        if (queue.size() < 3) { rewindJournal(queue, tmp); break; }
        item_count = readInt8(queue, tmp);
        item_uses = readInt16(queue, tmp);
      }
    }

    m_gsm.packetCSWindowClick(eid, window_id, slot, right_click, action, item_id, item_count, item_uses);

    return true;
  }

  case (PACKET_SIGN):
  {
    if (queue.size() < 13) return false;

    int32_t X, Z;
    int16_t Y, len1, len2, len3, len4;
    std::string line1, line2, line3, line4;
    
    {
      GUARDLOCK;

      readInt8(queue, tmp);
      X = readInt32(queue, tmp);
      Y = readInt16(queue, tmp);
      Z = readInt32(queue, tmp);

      len1 = readInt16(queue, tmp);
      if (int(queue.size()) < len1) { rewindJournal(queue, tmp); break; }
      line1 = readString(queue, tmp, len1);

      len2 = readInt16(queue, tmp);
      if (int(queue.size()) < len2) { rewindJournal(queue, tmp); break; }
      line2 = readString(queue, tmp, len2);

      len3 = readInt16(queue, tmp);
      if (int(queue.size()) < len3) { rewindJournal(queue, tmp); break; }
      line3 = readString(queue, tmp, len3);

      len4 = readInt16(queue, tmp);
      if (int(queue.size()) < len4) { rewindJournal(queue, tmp); break; }
      line4 = readString(queue, tmp, len4);
    }

    m_gsm.packetCSSign(eid, X, Y, Z, line1, line2, line3, line4);

    return true;
  }

  default:
    std::cerr << "Unknown variable-width data field: Type " << (unsigned int)(type) << std::endl;
  }
  return false;
}
