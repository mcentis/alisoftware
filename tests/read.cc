/**
 * print on screen the info of a alibava run
 */

#include "fstream"
#include "iostream"
#include "time.h"
#include "stdlib.h"
#include "stdint.h"

const int nChannels = 256;

enum BlockType { NewFile = 0, StartOfRun, Data, CheckPoint, EndOfRun};

int main(int argc, char* argv[])
{
  // all the sizes
  // std::cout << "int " << sizeof(int) << std::endl;
  // std::cout << "int32_t " << sizeof(int32_t) << std::endl;
  // std::cout << "unsigned int " << sizeof(unsigned int) << std::endl;
  // std::cout << "uint32_t " << sizeof(uint32_t) << std::endl;
  // std::cout << "int16_t " << sizeof(int16_t) << std::endl;
  // std::cout << "uint16_t " << sizeof(uint16_t) << std::endl;
  // std::cout << "double " << sizeof(double) << std::endl;
  // std::cout << "char " << sizeof(char) << std::endl;
  // std::cout << "time_t " << sizeof(time_t) << std::endl;
  // std::cout << "float " << sizeof(float) << std::endl;
  // return 0;

  if(argc != 2)
    {
      std::cout << "Usage: read \"filename\"" << std::endl;
      return 1;
    }

  // if the machine that produced the data has a different architecture these numbers and the types used for reading must be changed
  // first candidates are double, time and floats
  const std::streamsize sizeInt32 = 4;
  const std::streamsize sizeUint32 = 4;
  const std::streamsize sizeInt16 = 2;
  const std::streamsize sizeUint16 = 2;
  const std::streamsize sizeChar = 1;
  const std::streamsize sizeTime = 8;
  const std::streamsize sizeDouble = 8;

  std::ifstream fileStr;
  fileStr.open(argv[1], std::ifstream::in);

  if(fileStr.is_open() == false)
    {
      std::cout << "Impossible to open the file " << argv[1] << std::endl;
      return 1;
    }

  // =================================== file header =======================================
  time_t date;
  int32_t type;
  uint32_t headerLength;
  char* header;
  int version;

  fileStr.read((char*) &date, sizeTime);
  fileStr.read((char*) &type, sizeInt32);
  fileStr.read((char*) &headerLength, sizeInt32);

  header = new char[headerLength];
  fileStr.read(header, headerLength);

  if(header[0] != 'v' && header[0] != 'V')
    version = 0;
  else 
    version = atoi(&header[1]);

  std::cout << "##################### File header ######################" << std::endl;

  std::cout <<  "Time = " << ctime(&date);// << std::endl;
  std::cout <<  "Type = " << type << " -> ";//std::endl;
  switch(type)
    {
    case 1:
      std::cout << "calibration run" << std::endl;
      break;
    case 2:
      std::cout << "laser sync. run" << std::endl;
      break;
    case 3:
      std::cout << "laser run" << std::endl;
      break;
    case 4:
      std::cout << "radioactive source run" << std::endl;
      break;
    case 5:
      std::cout << "pedestal run" << std::endl;
      break;
    default:
      std::cout << "unknown run type" << std::endl;
    }

  std::cout <<  "Header = " << header << std::endl;
  std::cout <<  "Version = " << version << std::endl;

  // =============================== pedestals and noise =============================================
  double ped[nChannels];
  double noise[nChannels];

  fileStr.read((char*) &ped, sizeDouble * nChannels);
  fileStr.read((char*) &noise, sizeDouble * nChannels);

  // std::cout << "##################### Pedestals and Noise######################" << std::endl;
  // for(int i = 0; i < nChannels; ++i)
  //   {
  //     std::cout << "Channel " << i << "\tped\t" << ped[i] << "\tnoise\t" << noise[i] << std::endl;
  //   }

  // ============================= data blocks ===============================================================

  uint32_t dbHead = 0;
  uint16_t dbInfo = 0;
  unsigned int dbType;
  bool isUserDb;
  uint32_t dbSize;
  char* dataBlock;

  uint8_t rea = 0;

  int evtCount = 0;

  if(version == 0)
    {
      std::cout << "Version is 0 => Data are not going to be readen (so it is in the alibava code)" << std::endl;
      fileStr.close();
      return 1;
    }

  do // loop to read the file to the end
  {
    try
      {
	do // find the data block header
	  {
	    fileStr.read((char*) &dbHead, sizeUint32);
	    if(!fileStr.good())
	      {
		std::cout << "File stream not good while looking for data block header" << std::endl;
		throw 1;
	      }
	  } while((dbHead & 0xffff0000) != 0xcafe0000); // find the data block header
      }

    catch(int a)
      {
	std::cout << "Catched exception: " << a << std::endl;
	break;
      }

    //    fileStr.read((char*) &dbInfo, sizeUint16);

    // dbInfo = dbHead& 0xffff;

    //std::cout << std::hex << dbHead << "    " << dbInfo << std::dec << std::endl;
    // std::cout << std::hex << dbInfo << std::dec << std::endl;

    dbType = dbHead & 0xfff;
    isUserDb = dbHead & 0x1000;

    // std::cout << dbType << std::endl;
    // std::cout << isUserDb << std::endl;

    fileStr.read((char*) &dbSize, sizeUint32);

    //std::cout << dbSize << std::endl;

    switch(dbType)
      {
      case NewFile:
	break;
      case StartOfRun:
	break;
      case Data:
	evtCount++;
	//std::cout << std::hex;
	for(int i = 0; i < dbSize + 2; ++i) // the size of the data block is not so a good quantity? !!!
	  {
	    fileStr.read((char*) &rea, 1);
	    //std::cout << rea;
	  }
	//std::cout << std::dec << std::endl;
	break;
      case CheckPoint:
	break;
      case EndOfRun:
	break;
      default:
	std::cout << "Data block type unknown" << std::endl;
      }
    continue;

    // if(dbType == 2 && isUserDb == false) continue; for testing only

    std::cout << "############# Data block #####################" << std::endl;
    std::cout << "Header = " << std::hex << dbHead << std::dec << std::endl;
    std::cout << "Type = " << std::hex << dbType << std::dec << " -> ";// << std::endl;

    switch(dbType)
      {
      case NewFile:
	std::cout << "New file" << std::endl;
	break;
      case StartOfRun:
	std::cout << "Run start" << std::endl;
	break;
      case Data:
	std::cout << "Data!!!" << std::endl;
	break;
      case CheckPoint:
	std::cout << "Check point" << std::endl;
	break;
      case EndOfRun:
	std::cout << "Run end" << std::endl;
	break;
      default:
	std::cout << "Data block type unknown" << std::endl;
      }

    std::cout << "Is user data = " << isUserDb << std::endl;
    std::cout << "Data block size = " << dbSize << " bytes" << std::endl;

    // std::cout << std::hex << dbType << std::dec << std::endl;
    //std::cout << "found" << std::endl;

  } while(fileStr.good());  // loop to read the file to the end

  fileStr.close();

  std::cout << "==================================================" << std::endl;
  std::cout << "Total events = " << evtCount << std::endl;

  return 0;
}
