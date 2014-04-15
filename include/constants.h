#ifndef CONSTANTS_H
#define CONSTANTS_H

// modify the construction of the branches when these cont are changed
const int nChannels = 256; // number of chennels
const int nParameters = 2; // paramters to fit the calibration
const char* const fitCal = "pol1"; // fit the calibration
const char* const convertToe = "(x - [0]) / [1]"; // convert to alibava e-

// i do not fully understand the second const for the char*, but it is necessary to get the code to compile

#endif // #ifndef CONSTANTS_H
