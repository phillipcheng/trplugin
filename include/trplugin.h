//
//  dc.h
//  trplugin
//
//  Created by Cheng Yi on 1/31/15.
//  Copyright (c) 2015 Cheng Yi. All rights reserved.
//

#ifndef trplugin_h
#define trplugin_h

#include "dc.h"

#include <signal.h>
#include <getopt.h>
#include <locale.h>
#include <ts/ts.h>


extern const int FLAG_AUTH_SUCC;
extern const int FLAG_AUTH_FAILED;
extern const int FLAG_NORMAL;

//context data for txn (req, rsp), put in the continuation for http response processor to get
typedef struct {
    int flag; //indicate whether the authentication succeed or not
    const char* errmsg; //the text reason of failure
    char* user;
    char* sessionid;
} TxnData;
//context data for diameter txn (req, rsp), put in the diameter session for diameter response processor to get
typedef struct {
    TSHttpTxn txnp;
    TSCont contp;
    int flag;//indicate whether the authentication succeed or not
    const char* errmsg;
    uint64_t grantedQuota;//to take from rsp
    uint64_t requestQuota;
    uint64_t thisTimeNeed;//volume this req/rsp need, check against the granted to decide whether allow or block
    uint64_t used;//volume used since last granted = session.granted - session.left
    bool httpReq; //this diameter msg is sent during http request or http response
    d_req_type reqType;//diameter req type: start, update, stop
    char* userId;//user id
    char* d1sid;//diameter session id for no.1 diameter server, can be multiple.
} DiamTxnData;

TxnData * txn_data_alloc();
void txn_data_free(TxnData *data);
DiamTxnData* dtxn_alloc(TSHttpTxn txnp, TSCont contp, bool req, d_req_type type);
void dtxn_free(DiamTxnData *data);

void d_cli_send_msg(DiamTxnData * dtdata);
void start_session_cb(DiamTxnData* dtdata);
void update_session_cb(DiamTxnData* dtxn_data);
void end_session_cb(DiamTxnData* dtxn_data);

//
extern const char* HEADER_CMD;
extern const int HEADER_CMD_LEN;
//
extern const char* HEADER_CMDVAL_START;
extern const char* HEADER_CMDVAL_STOP;
extern const char* HEADER_CMDVAL_NO;
//
extern const char* HEADER_USERID;
extern const int HEADER_USERID_LEN;
//
extern const char* HEADER_SESSIONID;
extern const int HEADER_SESSIONID_LEN;
//
extern const char* RSP_HEADER_REASON;
extern const char* RSP_REASON_VAL_SUCCESS;
extern const char* RSP_REASON_VAL_NOREQHEAD;
extern const char* RSP_REASON_VAL_REQHEAD_NOUSERIP;
extern const char* RSP_REASON_VAL_NOUSER;
extern const char* RSP_REASON_VAL_NOBAL;
extern const char* RSP_REASON_VAL_NOIPSESSION;
extern const char* RSP_REASON_VAL_USERONLINE;
extern const char* RSP_REASON_VAL_IPINUSE;

#endif
