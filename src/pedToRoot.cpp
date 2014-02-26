#include "PedRun.hh"
#include "ConfigFileReader.hh"

#include "iostream"

int main(int argc, char* argv[])
{
  if(argc != 3)
    {
      std::cout << "Usage: pedToRoot binaryFile confFile" << std::endl;
      return 1;
    }

  ConfigFileReader* conf = new ConfigFileReader(argv[2]);
  //conf->DumpConfMap();

  PedRun* ped = new PedRun(argv[1], conf);
  ped->ReadFile();
  ped->AnalyseData();
  ped->WriteEvts();

  delete ped;

  return 0;
}
