#include "BinaryData.hh"
#include "ConfigFileReader.hh"

#include "iostream"

int main(int argc,char* argv[])
{
  if(argc != 3)
    {
      std::cout << "Usage: binToRoot binaryFile confFile" << std::endl;
      return 1;
    }

  ConfigFileReader* conf = new ConfigFileReader(argv[2]);

  BinaryData* binRead = new BinaryData(argv[1], conf);
  binRead->ReadFile();
  binRead->WriteEvts();

  delete binRead;
  delete conf;

  return 0;
}
