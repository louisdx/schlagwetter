#ifndef H_UI
#define H_UI

#include <readline/readline.h>
#include <readline/history.h>

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


/* Pumping the UI lets the admin interact with the running server. */
class ConnectionManager;
bool pump(ConnectionManager & connection_manager, UI & ui);


#endif
