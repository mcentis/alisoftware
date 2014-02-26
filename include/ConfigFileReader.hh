#ifndef CONFIGFILEREADER_H
#define CONFIGFILEREADER_H

#include "vector"
#include "string"
#include "fstream"
#include "map"

class ConfigFileReader
{
public:
  ConfigFileReader(const char* ConfFile);
  ~ConfigFileReader();

  std::string GetValue(std::string key) {return confMap[key];}; // get a value from configuration map
  std::string GetFileName() {return fileName;};
  void DumpConfMap();

private:
  std::string fileName;
  std::ifstream fileStr;
  std::map<std::string, std::string> confMap;

  void ReadFile();
};

#endif // #ifndef CONFIGFILEREADER_H
