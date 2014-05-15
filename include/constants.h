#ifndef CONSTANTS_H
#define CONSTANTS_H

// modify the construction of the branches when these cont are changed
const int nChips = 2;
const int nChChip = 128;
const int nChannels = nChChip * nChips; // number of chennels
const int nParameters = 2; // paramters to fit the calibration
const char* const fitCal = "pol1"; // fit the calibration
const char* const convertToe = "(x - [0]) / [1]"; // convert to alibava e-

// first const: the pointed value is constant, second const: the pointer is constant
// this syntax is needed to compile the code

#endif // #ifndef CONSTANTS_H
