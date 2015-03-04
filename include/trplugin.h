//
//  trplugin.h
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
typedef enum {u_start=1, u_use=2, u_stop=3} u_req_type;

typedef struct {
    unsigned int diameterVendorId;//
    unsigned int diameterAppId;//
    unsigned int diameterCmdId;//
    char* diameterDestRealm;
    char* diameterDestHost;
    unsigned long minRequestQuota;//bytes
    unsigned long usTimeout; //seconds
    unsigned int usTimeoutCheckInterval;//seconds
}TrConfig;

extern TrConfig* tr_conf;

//context data for txn (req, rsp), put in the continuation for http response processor to get
typedef struct {
    int flag; //indicate whether the authentication succeed or not
    int errid; //the text reason of failure
    char* user;
    char* tenant;
	char* clientip;
    char* sessionid;
    u_req_type reqType;//user req type: start, use, stop
} HttpTxnData;

//context data for diameter txn (req, rsp), put in the diameter session for diameter response processor to get
typedef struct {
    TSHttpTxn txnp;
    TSCont contp;
    int flag;//indicate whether the authentication succeed or not
    int errid;
    uint64_t grantedQuota;//to take from rsp
    uint64_t requestQuota;
    uint64_t thisTimeNeed;//volume this req/rsp need, check against the granted to decide whether allow or block
    uint64_t used;//volume used since last granted = session.granted - session.left
    bool httpReq; //this diameter msg is sent during http request or http response
    d_req_type reqType;//diameter req type: start, update, stop
    char* userId;//user id
    char* tenantId;//tenant id
    char* d1sid;//diameter session id for no.1 diameter server, can be multiple.
    bool dserver_error;//when diameter server goes to error, let user use and record the usage, when the server recover, report all used.
} DiamTxnData;

//
HttpTxnData * http_txn_data_alloc();
void http_txn_data_free(HttpTxnData *data);
//
DiamTxnData* dtxn_alloc(TSHttpTxn txnp, TSCont contp, bool req, d_req_type type);
void dtxn_free(DiamTxnData *data);

//
void d_cli_send_msg(DiamTxnData * dtdata);
void start_session_cb(DiamTxnData* dtdata);
void update_session_cb(DiamTxnData* dtxn_data);
void end_session_cb(DiamTxnData* dtxn_data);

//user session structure
typedef struct {
    char*   sid;//
    uint64_t    grantedQuota;
    int64_t leftQuota; //can be less the zero for over-use
    char*   d1sid;//diameter 1 session id
    bool dserver_error;
    int64_t errorUsed;//used when the dserver is down
    pthread_mutex_t		us_lock;//lock for this user session
    uint32_t    pending_d_req;//the number of pending diameter req
    time_t lastUpdateTime; //
}UserSession;
//user session structure in a double linked list
typedef struct UserSessionDL{
    UserSession* us;
    struct UserSessionDL *next, *prev;
}UserSessionDL;
//user session structure in a hashmap
typedef struct{
    char* sid;
    UserSessionDL* usdl;
    UT_hash_handle hh; /* makes this structure hashable */
}UserSessionHM;

extern UserSessionHM* user_session_hashmap;
extern UserSessionDL* user_session_dlinkedlist;
extern int user_session_count_stat;//TSStat on user session

UserSession * user_session_alloc(HttpTxnData* txnp);
char* get_session_id(HttpTxnData* txnp);
void add_user_session(UserSession* us);
void delete_user_session(char* sid);
UserSession* find_user_session(char* sid);
void update_user_session(UserSession* ushm);

void* checkSessionTimeout(void* ptr);

char* getClientIpStr(TSHttpTxn txnp);

int dcinit(int argc, char * argv[]);

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
extern const char* HEADER_TENANTID;
extern const int HEADER_TENANTID_LEN;
//
extern const char* RSP_HEADER_REASON;//header field name
extern int RSP_REASON_VAL_SUCCESS;
extern int RSP_REASON_VAL_NOREQHEAD;
extern int RSP_REASON_VAL_REQHEAD_MISSINGINFO;
extern int RSP_REASON_VAL_NOUSER;
extern int RSP_REASON_VAL_NOBAL;
extern int RSP_REASON_VAL_NOUSERSESSION;
extern int RSP_REASON_VAL_USERONLINE;

#endif
