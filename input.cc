#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
  // input stream
  std::istream *ifs;

  // file input
  std::ifstream fileStream;

  try {
    po::options_description generic("Generic options");
    std::string config_file;
    std::string filename;

    generic.add_options()
    ("help", "produce help message")
    ("file", po::value<std::string>(&filename), "input filename")
    ("config,c", po::value<std::string>(&config_file)->default_value(".inputrc"),
     "name of a file of a configuration.")
    ;

    po::positional_options_description p;
    p.add("file", -1);

    po::options_description config("Configuration");
    config.add_options()
      ("include-path,I", po::value< std::vector<std::string> >()->composing(), "include-path")
    ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
      ("input-file", po::value< std::vector<std::string> >(), "input file")
      ;

    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config).add(hidden);

    po::options_description config_file_options;
    config_file_options.add(config).add(hidden);

    po::options_description visible("Allowed options");

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
    po::notify(vm);

    if ( vm.count("help")) {
      std::cout << generic << "\n";
      return 0;
    }

    if ( vm.count("config") ) {
      std::ifstream configStream(config_file.c_str());
      if ( configStream ) {
        po::store(po::parse_config_file(configStream, config_file_options), vm);
        notify(vm);
      }
      else {
        std::cerr << "can not open config file: " << config_file << "\n";
        return 1;
      }
    }
    else {
      std::cerr << "can not open config file: " << config_file << "\n";
      return 1;
    }

    if ( vm.count("file")) {
      fileStream.open(filename.c_str(), std::ifstream::in);
      if ( !fileStream.is_open() ) {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
        return 1;
      }
      ifs = &fileStream;
    } else {
      ifs = &std::cin;
    }

  } catch(std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  } catch(...) {
    std::cerr << "Exception of unknown type!\n";
    return 1;
  }

  // Max buffer size
  const int cMaxLine = 1;

  // input buffer
  char line[cMaxLine];

  // parsing buffer
  std::string buffer;

  while (!ifs->eof()) {
    ifs->read(line, cMaxLine);

    if ( *ifs ) {
      buffer.append(line, ifs->gcount());

      // process commands
      size_t end = 0;
      while ((end = buffer.find("\n")) != std::string::npos) {
        // copy command
        std::string command(buffer, 0, end);
        std::size_t comment = command.find_first_of("#");
        if ( comment != std::string::npos ) {
          command.erase(comment);
        }

        // remove command from input
        buffer.erase(0, end+1);

        if ( ! command.empty() ) {
          std::cout << "Command: " << command << std::endl;
        }
      }
    }
  }
  return 0;
}
