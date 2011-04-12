#ifndef H_UI
#define H_UI

#include <iostream>

/* Abstract UI interface. */

class UI
{
public:
  UI() { }
  virtual ~UI() { }
  virtual std::string readline(const std::string & prompt) = 0;
};


/* Simple implementation, standard C++ only. */

class SimpleUI : public UI
{
  std::string readline(const std::string & prompt);
};



#ifdef HAVE_GNUREADLINE
#  include <readline/readline.h>
#  include <readline/history.h>

/* GNU Readline power! */

class GNUReadlineUI : public UI
{
private:
  std::string m_histfile;
  char      * m_line;

public:
  GNUReadlineUI(const std::string & histfile);
  ~GNUReadlineUI();
  std::string readline(const std::string & prompt);
};

#endif // HAVE_GNUREADLINE


/* Pumping the UI lets the admin interact with the running server. */
class Server;
bool pump(Server & server, UI & ui);


#endif
