 #ifndef BINARYDATA_HH
#define BINARYDATA_HH

/*
 * Class to read the binary data from the alibava
 * It calls the other run specific classes (or functions)
 * while reading the file
 */

#include "TFile.h"
#include "TTree.h"
#include "TH2I.h"

#include "fstream"
#include "string"
#include "stdint.h"

#include "constants.h"
#include "ConfigFileReader.hh"

class BinaryData
{
  // if there are problems with reading the file check these
  static const std::streamsize sizeInt32 = 4;
  static const std::streamsize sizeUint32 = 4;
  static const std::streamsize sizeInt16 = 2;
  static const std::streamsize sizeUint16 = 2;
  static const std::streamsize sizeChar = 1;
  static const std::streamsize sizeDouble = 8;
  std::streamsize sizeTime; // left as variable since is architecture dependent -> must be specified in the config file

  enum BlockType {NewFile = 0, StartOfRun, Data, CheckPoint, EndOfRun};

protected:
  std::string inFileName;
  std::string outFilePath;
  int runNumber;

  std::ifstream fileStr;

  std::string runHeader;
  int runType;

  TFile* outFile;
  TTree* rawEvtTree;

  Float_t injCharge; // charge injection for calibration
  Float_t delay; // delay for laser delay scan

  Float_t time; // event time (phase between trigger and clock)
  Float_t temp; // chip temperature
  UInt_t adcPH[nChannels];
  UInt_t headers[totHeadBits]; // headers of the chips

  TH2I* allEvents;

  ConfigFileReader* conf;

  Float_t convertTime(uint32_t rawT);
  Float_t convertTemp(uint16_t rawT);

  void ExtractRunNumber();

  // ============== functions for specific run types ==================
  virtual void doSpecificStuff(){return;};
  virtual void analyseRunHeader(){return;}; 
  int expectedEvts;
  std::vector<int> goodChannels;
  void readGoodChFile(const char* chFileName);

public:
  BinaryData(const char* InFileName, ConfigFileReader* Conf);
  virtual ~BinaryData();

  void ReadFile();
  void WriteEvts();

  std::string GetRunHeader(){return runHeader;};
  std::string GetInFileName(){return inFileName;};
  std::string GetOutFilePath(){return outFilePath;};
  int GetRunNumber(){return runNumber;};
  TFile* GetOutFile(){return outFile;};
};

#endif //#ifndef BINARYDATA_HH
