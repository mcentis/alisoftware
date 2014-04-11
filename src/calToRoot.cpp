#include "CalRun.hh"
#include "ConfigFileReader.hh"

#include "iostream"

int main(int argc, char* argv[])
{
  if(argc != 3)
    {
      std::cout << "Usage: calToRoot binaryFile confFile" << std::endl;
      return 1;
    }

  ConfigFileReader* conf = new ConfigFileReader(argv[2]);
  //conf->DumpConfMap();

  CalRun* cal = new CalRun(argv[1], conf);
  cal->ReadFile();
  cal->AnalyseData();
  cal->WriteEvts();

  delete cal;

  return 0;
}
