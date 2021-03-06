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

#ifndef C_USIMMTRACEREADER_H
#define C_USIMMTRACEREADER_H

#include <stdint.h>
#include <queue>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <string>
#include <fstream>
#include <unordered_map>

//SST includes
#include <sst/core/component.h>
#include <sst/core/link.h>

//local includes
#include "c_Transaction.hpp"

using namespace std;

namespace SST {
namespace n_Bank {
class c_USimmTraceReader: public SST::Component {

public:
	c_USimmTraceReader(SST::ComponentId_t x_id, SST::Params& x_params);
	~c_USimmTraceReader();

	void setup() {
	}
	void finish() {
		printf("TraceGen Queues:\n");
		printf("\t TraceGen Req Q:\n");
		for (unsigned l_i = 0; l_i != k_txnGenReqQEntries+1; ++l_i)
			printf("\t\t Size %u: Occured %u times\n", l_i, m_statsReqQ[l_i]);

		printf("\n\t TraceGen Res Q:\n");
		for (unsigned l_i = 0; l_i != k_txnGenResQEntries+1; ++l_i)
			printf("\t\t Size %u: Occured %u times\n", l_i, m_statsResQ[l_i]);

		printf("Total Read-Txns Requests send: %lu\n", m_reqReadCount);
		printf("Total Write-Txns Requests send: %lu\n", m_reqWriteCount);
		printf("Total Read-Txns Responses received: %lu\n", m_resReadCount);
		printf("Total Write-Txns Responses received: %lu\n", m_resWriteCount);
		printf("Total Cycles: %" PRIu64 "\n", Simulation::getSimulation()->getCurrentSimCycle());
		printf("Component Finished.\n");
	}

private:
	c_USimmTraceReader(); //for serialization only
	c_USimmTraceReader(const c_USimmTraceReader&); //do not implement
	void operator=(const c_USimmTraceReader&);

	c_Transaction* getNextTransaction(std::string x_txnType, unsigned x_addr, unsigned x_dataWidth);

	void createTxn();

	//txn to/from events
	void handleOutTxnGenReqPtrEvent(SST::Event *ev); // we do not need this function for functionality
	void handleInTxnUnitResPtrEvent(SST::Event *ev); //receive txn res ptr from Transaction unit

	//token chg to/from events
	void handleInTxnUnitReqQTokenChgEvent(SST::Event *ev); //receive change in tokens in txn unit req queue
	void handleOutTxnGenResQTokenChgEvent(SST::Event *ev); // we do not need this function for functionality

	void sendTokenChg(); //should happen at the end of every cycle
	void sendRequest(); //send out txn req ptr to Transaction unit
	void readResponse(); //read from res q to output

	virtual bool clockTic(SST::Cycle_t); //called every cycle

	//Transaction info
	uint64_t m_seqNum;

	//request-related links
	SST::Link* m_outTxnGenReqPtrLink; //outgoing txn gen req ptr
	SST::Link* m_inTxnUnitReqQTokenChgLink; //incoming change in txn unit req q tokens

	//response links
	SST::Link* m_inTxnUnitResPtrLink; //incoming txn unit res ptr
	SST::Link* m_outTxnGenResQTokenChgLink; //outgoing change in txn gen res q tokens

	//params for internal microarcitecture
	std::string m_traceFileName;
  std::ifstream m_traceFileStream;
	std::map<std::string,e_TransactionType> m_stringToTxnTypeMap = { {"READ",e_TransactionType::READ},
									 {"WRITE",e_TransactionType::WRITE} };

	int k_txnGenReqQEntries;
	int k_txnGenResQEntries;

	//param for receiver
	int k_txnUnitReqQEntries;

	// token change in this unit this cycle
	// beginning of every cycle this variable is reset
	// sendTokenChg() function sends the contents of this variable
	int m_thisCycleResQTknChg;

	//token changes from Txn unit
	int m_txnUnitReqQTokens;

	// used to keep track of the response types being received and send
	unsigned long m_reqReadCount;
	unsigned long m_reqWriteCount;
	unsigned long m_resReadCount;
	unsigned long m_resWriteCount;

	std::queue<std::pair<c_Transaction*, unsigned>> m_txnReqQ;
	std::queue<c_Transaction*> m_txnResQ; //incoming

	// FIXME: Delete. Used for debugging queue size issues
	unsigned* m_statsReqQ;
	unsigned* m_statsResQ;


};

} // namespace n_Bank
} // namespace SST

#endif  /* C_USIMMTRACEREADER_H */
