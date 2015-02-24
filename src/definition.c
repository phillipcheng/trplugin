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
int RSP_REASON_VAL_SUCCESS=0;
int RSP_REASON_VAL_NOREQHEAD=1;
int RSP_REASON_VAL_REQHEAD_MISSINGINFO=2;
int RSP_REASON_VAL_NOUSER=3;
int RSP_REASON_VAL_NOBAL=4;
int RSP_REASON_VAL_NOUSERSESSION=5;
int RSP_REASON_VAL_USERONLINE=6;

const int FLAG_AUTH_FAILED=1;//set the rsp_header_reason
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


