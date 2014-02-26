#include "DataRun.hh"
#include "ConfigFileReader.hh"

#include "iostream"

int main(int argc, char* argv[])
{
  if(argc != 3)
    {
      std::cout << "Usage: dataToRoot binaryFile confFile" << std::endl;
      return 1;
    }

  ConfigFileReader* conf = new ConfigFileReader(argv[2]);
  //conf->DumpConfMap();

  DataRun* data = new DataRun(argv[1], conf);
  data->ReadFile();
  data->WriteCookedTree();
  data->WriteEvts();
  data->AnalyseData();

  delete data;

  return 0;
}
