#include "ConfigFileReader.hh"

#include "iostream"
#include "stdlib.h"
#include "algorithm"


ConfigFileReader::ConfigFileReader(const char* ConfFile):
  fileName(ConfFile), fileStr()
{
  fileStr.open(fileName.c_str(), std::ifstream::in);

  if(!fileStr.is_open())
    {
      std::cout << "[ConfigFileReader::ConfigFileReader] Error opening file " << fileName << std::endl;
      exit(1);
    }

  ReadFile();
}

ConfigFileReader::~ConfigFileReader()
{
  fileStr.close();
}

void ConfigFileReader::ReadFile()
{
  std::string line;
  std::string name;
  std::string value;
  unsigned int eqPos; // position of the =
  std::string::iterator newEnd; // end of the string after remove
  unsigned int sharpPos; // position of the #
  std::string previousReserved;

  while(!fileStr.eof())
    {
      line.clear();

      std::getline(fileStr, line);

      // std::cout << "Read line: |" << line << "|" << std::endl; // for testing only

      if(line.length() == 0) continue; // empty line

      sharpPos = line.find('#');
      if(sharpPos == 0) continue; // full comment line, skip

      if(sharpPos < line.size()) // sharp found
      line.resize(sharpPos); // ignore what comes after the #

      // removes all the spaces from the string, remove does not change the length of the string, it is necessary to resize the string
      newEnd = std::remove(line.begin(), line.end(), ' ');
      if(newEnd == line.begin()) continue; // string of spaces and comments
      line.resize(newEnd - line.begin()); // resize the string to its new size

      // same treatment for the \t
      newEnd = std::remove(line.begin(), line.end(), '\t');
      if(newEnd == line.begin()) continue; // string of spaces, tabs and comments
      line.resize(newEnd - line.begin()); // resize the string to its new size

      // std::cout << "After selection: |" << line << "|\n-----------------------" << std::endl; // for testing only

      eqPos = line.find('=');

      if(eqPos == std::string::npos) // no = sign found
	{
	  std::cout << "[ConfigFileReader::ReadFile]: Error no = sign found in line: " << std::endl;
	  std::cout << "Check the config file for errors.\nI trusted you and this is what you did to me!" << std::endl;
	  continue;
	}

      name = line.substr(0,eqPos);
      value = line.substr(eqPos + 1);

      confMap[name] = value;
    }

  return;
}

void ConfigFileReader::DumpConfMap()
{
  std::map<std::string, std::string>::iterator it;
  for(it = confMap.begin(); it != confMap.end(); ++it)
    std::cout << (*it).first << " => " << (*it).second << std::endl;

  return;
}
