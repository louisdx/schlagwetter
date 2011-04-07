#ifndef H_INPUTPARSER
#define H_INPUTPARSER

#include "gamestatemanager.h"

class InputParser
{
public:
  InputParser(GameStateManager & gsm);

  /// Processing incoming packets.
  void immediateDispatch(int32_t eid, const std::vector<char> & data);

  /// Return true if a whole packet was extracted.
  bool dispatchIfEnoughData(int32_t eid, std::deque<char> & queue, std::recursive_mutex * ptr_mutex);

private:
  GameStateManager & m_gsm;
};


#endif
