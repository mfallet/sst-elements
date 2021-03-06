// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_TIMING_DRAM_BACKEND
#define _H_SST_MEMH_TIMING_DRAM_BACKEND

#include "membackend/simpleMemBackend.h"
#include "membackend/timingAddrMapper.h"
#include "membackend/timingTransaction.h"
#include "membackend/timingPagePolicy.h"

namespace SST {
namespace MemHierarchy {

using namespace  TimingDRAM_NS;

class TimingDRAM : public SimpleMemBackend {

    const uint64_t DBG_MASK = 0x1;

    class Cmd;

    struct Bank {

        static bool m_printConfig;

      public:
        static const uint64_t DBG_MASK = (1 << 3); 
        Bank( Component*, Params&, unsigned mc, unsigned chan, unsigned rank, unsigned bank, Output* );

        void pushTrans( Transaction* trans ) {
            m_transQ->push(trans);
        }

        Cmd* popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle );

        void setLastCmd( Cmd* cmd ) {
            m_lastCmd = cmd;
        }

        void clearLastCmd() {
            m_lastCmd = NULL;
        }

        Cmd* getLastCmd() {
            return m_lastCmd;
        }

        void verbose( int line, const char* func, const char * format, ...) { 
        
            char buf[500];
            va_list arg;
            va_start( arg, format );
            vsnprintf( buf, 500, format, arg );
            va_end(arg);
            m_output->verbosePrefix( prefix(), line,"",func, 2, DBG_MASK, buf );
        }

        unsigned getRank() { return m_rank; }
        unsigned getBank() { return m_bank; }

      private:
        void update( SimTime_t );
        const char* prefix() { return m_pre.c_str(); }

        Output*             m_output;
        std::string         m_pre;

        unsigned            m_col_rd_lat;
        unsigned            m_col_wr_lat;
        unsigned            m_rcd_lat;
        unsigned            m_trp_lat;
        unsigned            m_data_lat;
        Cmd*                m_lastCmd;
//        bool                m_busy;
        unsigned            m_rank;
        unsigned            m_bank;
        unsigned            m_row;
        std::deque<Cmd*>    m_cmdQ;
        TransactionQ*       m_transQ;
        PagePolicy*         m_pagePolicy;
    };

    class Cmd {
      public:
        enum Op { PRE, ACT, COL } m_op;
        Cmd( Bank* bank, Op op, unsigned cycles, unsigned row = -1, unsigned dataCycles = 0, Transaction* trans  = NULL  ) : 
            m_bank(bank), m_op(op), m_cycles(cycles), m_row(row), m_dataCycles(dataCycles), m_trans(trans)
        { 
            switch( m_op ) {
              case PRE:
                m_name = "PRE";
                break;
              case ACT:
                m_name = "ACT";
                break;
              case COL:
                m_name = "COL";
                break;
            }
            m_bank->verbose(__LINE__,__FUNCTION__,"new %s for rank=%d bank=%d row=%d\n",
                    getName().c_str(), getRank(), getBank(), getRow());
        }

        ~Cmd() {
            if ( m_trans ) {
                m_trans->setRetired();
            }
            m_bank->clearLastCmd();
        }

        unsigned issue() {
            
            m_bank->setLastCmd(this);

            return m_dataBusAvailCycle;
        }

        bool canIssue( SimTime_t currentCycle, SimTime_t dataBusAvailCycle ) {
            bool ret=false;

            Cmd* lastCmd = m_bank->getLastCmd();

            if ( lastCmd ) {
#if 0 
                if ( lastCmd->m_trans && lastCmd->m_trans->req->isWrite_ && m_op == PRE ) {
                    printf( "WR to PRE\n" );
                }
#endif
                if ( m_op == COL && lastCmd->m_op == COL ) {
                    if ( lastCmd->m_trans->isWrite && ! m_trans->isWrite ) {
#if 0
                        printf("WR to RD\n");
                        assert(0);
#endif
                    }
                    if ( lastCmd->m_issueTime + m_dataCycles > currentCycle ) {
                        return false;
                    }
                } else {
                    return false;
                }
            }

            m_issueTime = currentCycle;
            m_finiTime = currentCycle + m_cycles;
            m_dataBusAvailCycle = dataBusAvailCycle;

            if ( m_finiTime >= dataBusAvailCycle ) { 
                m_finiTime += m_dataCycles;
                m_dataBusAvailCycle = m_finiTime;
                ret = true;
            } else {
                m_bank->verbose(__LINE__,__FUNCTION__,"bus not ready\n");
            } 

            return ret;
        }

        bool isDone( SimTime_t now ) { 

            m_bank->verbose(__LINE__,__FUNCTION__,"%lu %lu\n",now,m_finiTime);
            return ( now >= m_finiTime );
        }

        // these are used for debugging
        std::string& getName()  { return m_name; }
        unsigned getRank()      { return m_bank->getRank(); }
        unsigned getBank()      { return m_bank->getBank(); }
        unsigned getRow()       { return m_row; }

      private:

        Bank*           m_bank;
        std::string     m_name;
        unsigned        m_cycles;
        unsigned        m_row;
        unsigned        m_dataCycles;
        Transaction*    m_trans;

        SimTime_t       m_issueTime;
        SimTime_t       m_finiTime;
        SimTime_t       m_dataBusAvailCycle;
    };

    class Rank {

        static bool m_printConfig;

      public:
        static const uint64_t DBG_MASK = (1 << 2); 

        Rank( Component*, Params&, unsigned mc, unsigned chan, unsigned rank, Output*, AddrMapper* );
        
        Cmd* popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle );

        void pushTrans( Transaction* trans ) {
            unsigned bank = m_mapper->getBank( trans->addr);
            
            m_output->verbosePrefix(prefix(),CALL_INFO, 2, DBG_MASK,"bank=%d addr=%#lx\n",
                bank,trans->addr);

            m_banks[bank].pushTrans( trans );
        }

      private:

        const char* prefix() { return m_pre.c_str(); }
        Output*         m_output;
        AddrMapper*     m_mapper;
        std::string     m_pre;

        unsigned            m_nextBankUp;
        std::vector<Bank>   m_banks;
    };

    class Channel {

        static bool m_printConfig;

      public:
        static const uint64_t DBG_MASK = (1 << 1); 

        Channel( Component*, TimingDRAM* mem, Params&, unsigned mc, unsigned chan, Output*, AddrMapper* ); 

        bool issue( SimTime_t createTime, ReqId id, Addr addr, bool isWrite, unsigned numBytes ) {

            if ( m_maxPendingTrans == m_pendingTrans.size() ) {
                return false;
            } 

            unsigned rank = m_mapper->getRank( addr);

            m_output->verbosePrefix(prefix(),CALL_INFO, 3, DBG_MASK,"reqId=%lu rank=%d addr=%#lx\n", id, rank, addr );

            Transaction* trans = new Transaction( createTime, id, addr, isWrite, numBytes, m_mapper->getBank(addr),
                                                m_mapper->getRow(addr) );
            m_pendingTrans.push_back( trans );

            m_ranks[ rank ].pushTrans( trans );
            return true;
        }

        void clock(SimTime_t );

      private:
        Cmd* popCmd( SimTime_t cycle, SimTime_t dataBusAvailCycle );
        const char* prefix() { return m_pre.c_str(); }
        TimingDRAM*            m_mem;
        Output*             m_output;
        AddrMapper*         m_mapper;
        std::string         m_pre;

        unsigned            m_nextRankUp;
        std::vector<Rank>   m_ranks;

        unsigned            m_dataBusAvailCycle; 
        unsigned            m_maxPendingTrans;

        std::list<Cmd*>     m_issuedCmds;
        std::deque<Transaction*> m_pendingTrans;
    };

    static bool m_printConfig;

public:
    TimingDRAM();
    TimingDRAM(Component*, Params& );
    virtual bool issueRequest( ReqId, Addr, bool, unsigned );
    void handleResponse(ReqId  id ) {
        output->verbose(CALL_INFO, 2, DBG_MASK, "req=%p\n", id ); 
        handleMemResponse( id );
    }
    virtual void clock();
    virtual void finish() {}

private:

    std::vector<Channel> m_channels;
    AddrMapper* m_mapper;
    SimTime_t   m_cycle;

};

}
}

#endif
