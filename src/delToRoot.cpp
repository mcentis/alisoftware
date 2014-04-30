#include "DelRun.hh"
#include "ConfigFileReader.hh"

#include "iostream"

int main(int argc, char* argv[])
{
  if(argc != 3)
    {
      std::cout << "Usage: delToRoot binaryFile confFile" << std::endl;
      return 1;
    }

  ConfigFileReader* conf = new ConfigFileReader(argv[2]);
  //conf->DumpConfMap();

  DelRun* del = new DelRun(argv[1], conf);
  del->ReadFile();
  del->AnalyseData();
  del->WriteEvts();

  delete del;

  return 0;
}
