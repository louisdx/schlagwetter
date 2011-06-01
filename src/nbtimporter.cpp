#include <iostream>
#include <string>
#include <algorithm>
#include <list>
#include <boost/regex.hpp>   // std version not ready yet, will be similar
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/write.hpp>
#include <nbt.h>

#include "filereader.h"
#include "chunk.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

po::variables_map PROGRAM_OPTIONS;
bool parseOptions(int argc, char * argv[], po::variables_map & options);



int main(int argc, char * argv[])
{
  /// Stage I: Preparation

  if (!parseOptions(argc, argv, PROGRAM_OPTIONS)) return 0;

  const std::string dir   = PROGRAM_OPTIONS["directory"].as<std::string>();
  const std::string outfn = PROGRAM_OPTIONS["outfile"].as<std::string>();
  const bool verbose      = PROGRAM_OPTIONS.count("verbose") > 0;

  if (dir.empty() || !fs::exists(dir) || !fs::is_directory(dir))
  {
    std::cerr << "Please specify the region file directory. Type '-h' for help." << std::endl;
    return 0;
  }
  if (outfn.empty())
  {
    std::cerr << "Please specify the base name of the output file. Type '-h' for help." << std::endl;
    return 0;
  }
  else if (fs::exists(outfn + ".idx") || fs::exists(outfn + ".dat"))
  {
    std::cerr << "One of the output files already exists. To be safe, we abort." << std::endl;
    return 0;
  }


  /// Stage II: Open output files

  std::ofstream idxfile(outfn + ".idx", std::ios::binary);
  std::ofstream datfile(outfn + ".dat", std::ios::binary);

  if (!idxfile || !datfile)
  {
    std::cerr << "Error while opening the output files. Aborting." << std::endl;
    return 0;
  }

  boost::iostreams::filtering_ostreambuf zidx;
  boost::iostreams::filtering_ostreambuf zdat;

  zidx.push(boost::iostreams::zlib_compressor());
  zdat.push(boost::iostreams::zlib_compressor());

  zidx.push(idxfile);
  zdat.push(datfile);

  size_t counter = 0;


  /// Stage III: Loop over input files

  std::list<fs::path> filenames;
  std::copy(fs::directory_iterator(dir), fs::directory_iterator(), std::back_inserter(filenames));

  std::cout << "The directory \"" << dir << "\" contains the following files:" << std::endl;
  for (auto it = filenames.cbegin(); it != filenames.cend(); ++it)
  {
    std::string name = fs::basename(*it);
    std::string ext = fs::extension(*it);
    std::string fullname = it->string();

    if (verbose) std::cout << "Examining file " << name << ext << std::endl;

    if (ext != ".mcr")
    {
      std::cerr << "Skipping unexpected file " << *it << "." << std::endl;
      continue;
    }

    boost::regex rx("^r\\.(-?[0-9]+)\\.(-?[0-9]+)$");
    boost::cmatch rxmatch;

    if (!boost::regex_search(name.c_str(), rxmatch, rx))
    {
      std::cerr << "Skipping unexpected file " << *it << "." << std::endl;
      continue;
    }

    int X = boost::lexical_cast<int>(rxmatch[1]);
    int Z = boost::lexical_cast<int>(rxmatch[2]);

    std::cout << "Found coordinates: " << X << ", " << Z << " (= " << 32*X << ", " << 32*Z << "). " << std::endl;

    RegionFile f(fullname);
    f.parse();

    for (size_t x = 0; x < 32; ++x)
      for (size_t z = 0; z < 32; ++z)
        if (f.chunkSize(x, z) != 0)
        {
          const int32_t cx = 32 * X + int(x);
          const int32_t cz = 32 * Z + int(z);

          const ChunkCoords cc(cx, cz);

          if (verbose) std::cout << "Found chunk at " << cc << ", raw size " << f.chunkSize(x, z) << std::endl;

          std::string raw = f.getCompressedChunk(x, z);

          if (raw == "") { std::cerr << "Error extracting chunk " << cc << ", skipping." << std::endl; continue; }

          auto chunk = NBTExtract(reinterpret_cast<const unsigned char*>(raw.data()), raw.length(), cc);

          boost::iostreams::write(zdat, reinterpret_cast<const char*>(chunk->data().data()), Chunk::sizeBlockType + Chunk::sizeBlockMetaData);
          boost::iostreams::write(zidx, reinterpret_cast<const char*>(&cx), 4);
          boost::iostreams::write(zidx, reinterpret_cast<const char*>(&cz), 4);
          boost::iostreams::write(zidx, reinterpret_cast<const char*>(&counter), 4);

          ++counter;

          if (!verbose) { std::cout << "*"; std::cout.flush(); }
        }

    std::cout << std::endl;

  }  // files in directory ...


  std::cout << "All done!" << std::endl;
}


bool parseOptions(int argc, char * argv[], po::variables_map & options)
{
  po::options_description desc("Schlagwetter NBT Map Importer");
  desc.add_options()
    ("help,h", "Print usage information")
    ("verbose,v", "Print lots of run-time information")
    ("outfile,o", po::value<std::string>()->default_value(""), "The basename of the output file (\"myfile\" will create \"myfile.idx\" and \"myfile.dat\")")
    ("directory,d", po::value<std::string>()->default_value(""), "The directory that contains your region files")
    ;

  try
  {
    po::store(po::parse_command_line(argc, argv, desc), options);
    po::notify(options);
  }
  catch (const std::exception & e)
  {
    std::cerr << "Error during command line parsing: " << e.what() << std::endl;
    return false;
  }
  catch (...)
  {
    std::cerr << "Unknown error during command line parsing." << std::endl;
    return false;
  }

  if (options.count("help"))
  {
    std::cout << desc << std::endl;
      return false;
  }

  return true;
}
