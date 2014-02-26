#include "ConfigFileReader.hh"

#include "iostream"

int main(int argc, char* argv[])
{
  if(argc != 2)
    {
      std::cout << "Usage: confTest confFile" << std::endl;
      return 1;
    }

  ConfigFileReader* conf = new ConfigFileReader(argv[1]);
  conf->DumpConfMap();

  std::cout << conf->GetValue("outPath") << std::endl;
  std::cout << conf->GetValue("pippo").size() << std::endl;

  delete conf;

  return 0;
}
