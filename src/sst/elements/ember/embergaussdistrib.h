// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_EMBER_GAUSSIAN_COMPUTE_DISTRIBUTION
#define _H_SST_EMBER_GAUSSIAN_COMPUTE_DISTRIBUTION

#include "emberdistrib.h"
#include <sst/core/rng/gaussian.h>

namespace SST {
namespace Ember {

class EmberGaussianDistribution : public EmberComputeDistribution {

public:
	EmberGaussianDistribution(Component* owner, Params& params);
	~EmberGaussianDistribution();
	double sample(uint64_t now);

private:
	SSTGaussianDistribution* distrib;

};

}
}

#endif
