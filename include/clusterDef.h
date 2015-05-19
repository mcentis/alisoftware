#ifndef CLUSTER_DEF_H
#define CLUSTER_DEF_H

#include "vector"

#include "Rtypes.h"

struct cluster
{
  double adcTot; // tot charge in adc
  double qTot; // tot charge in electrons

  std::vector<int> strips; // strips in the cluster
  std::vector<double> adcStrips; // strips charge in the cluster [adc]
  std::vector<double> qStrips; // strips charge in the cluster [e-]

  double posStrAdc; // position in strip number calculated from adc
  double posmmAdc; // position in mm calculated from adc

  double posStrQ; // position in strip number calculated from charge
  double posmmQ; // position in mm calculated from charge

  cluster()
    {
      adcTot = 0;
      qTot = 0;
      
      posStrAdc = -1;
      posmmAdc = -1;
      
      posStrQ = -1;
      posmmQ = -1;
    };

  virtual ~cluster()
  {
  };

  ClassDef(cluster, 1);
};

#endif // #ifndef CLUSTER_DEF_H
