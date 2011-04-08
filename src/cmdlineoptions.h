#ifndef H_CMDLINEOPTIONS
#define H_CMDLINEOPTIONS


#include <boost/program_options.hpp>

namespace po = boost::program_options;

/* Main command line parser */

bool parseOptions(int argc, char * argv[], po::variables_map & options);

/* Global object, visible for everyone who cares. */

extern po::variables_map PROGRAM_OPTIONS;


#endif
