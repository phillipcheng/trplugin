//
//  definition.c
//  trplugin
//
//  Created by Cheng Yi on 2/7/15.
//  Copyright (c) 2015 Cheng Yi. All rights reserved.
//

#include "dc.h"
#include <time.h>


const char* DEBUG_NAME="trafficreport";

//Http Custom Req Headers
const char* HEADER_CMD = "command";
const int HEADER_CMD_LEN= 7;
const char* HEADER_CMDVAL_START = "start";
const char* HEADER_CMDVAL_STOP = "stop";
const char* HEADER_CMDVAL_NO ="no";

const char* HEADER_USERID = "userid";
const int HEADER_USERID_LEN= 6;

const char* HEADER_TENANTID= "tenantid";
const int HEADER_TENANTID_LEN= 8;

const char* HEADER_SESSIONID = "dsessionid";
const int HEADER_SESSIONID_LEN= 10;

//Http Custom Rsp Headers
const char* RSP_HEADER_REASON = "rejectreason";
const char* RSP_REASON_VAL_SUCCESS="command succeed";
const char* RSP_REASON_VAL_NOREQHEAD="no req head";
const char* RSP_REASON_VAL_REQHEAD_NOUSERIP="no user/ip in the start request header";
const char* RSP_REASON_VAL_NOUSER="no such user";
const char* RSP_REASON_VAL_NOBAL="no balance";
const char* RSP_REASON_VAL_NOIPSESSION="no ip session";
const char* RSP_REASON_VAL_USERONLINE="user already online";
const char* RSP_REASON_VAL_IPINUSE="ip already in use";

const int FLAG_AUTH_FAILED=1;
const int FLAG_AUTH_SUCC=2;
const int FLAG_NORMAL=3;

const unsigned long MIN_REQUEST_QUOTA = 1*1024*1024;
const unsigned long DUMMY_INIT_QUOTA = 3*1024*1024;

const unsigned int DIAMETER_SUCCESS = 2001;
const unsigned int DIAMETER_UNKNOWN_SESSION_ID= 5002;//for update and stop
const unsigned int DIAMETER_AUTHORIZATION_REJECTED= 5003; //maps to user not found for start

struct session_handler * ta_cli_reg = NULL;

//for the simulated server side usage session
UsageServerSession * usageserver_session_alloc(){
    UsageServerSession* s = malloc(sizeof(UsageServerSession));
    s->leftQuota=0;
    return s;
}
void usageserver_session_free(UsageServerSession *data){
    if (data->sid!=NULL){
        free(data->sid);
    }
    free(data);
}


