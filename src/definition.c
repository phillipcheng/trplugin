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

const char* HEADER_CMD = "command";
const int HEADER_CMD_LEN= 7;
const char* HEADER_CMDVAL_START = "start";
const char* HEADER_CMDVAL_STOP = "stop";
const char* HEADER_CMDVAL_NO ="no";
const char* HEADER_USERID = "userid";
const int HEADER_USERID_LEN= 6;
const char* HEADER_SESSIONID = "dsessionid";
const int HEADER_SESSIONID_LEN= 10;

const int FLAG_AUTH_FAILED=1;
const int FLAG_AUTH_SUCC=2;
const int FLAG_NORMAL=3;

const char* RSP_HEADER_REASON = "rejectreason";
const char* RSP_REASON_VAL_SUCCESS="command succeed";
const char* RSP_REASON_VAL_NOREQHEAD="no req head";
const char* RSP_REASON_VAL_REQHEAD_NOUSERIP="no user/ip in the start request header";
const char* RSP_REASON_VAL_NOUSER="no such user";
const char* RSP_REASON_VAL_NOBAL="no balance";
const char* RSP_REASON_VAL_NOIPSESSION="no ip session";
const char* RSP_REASON_VAL_USERONLINE="user already online";
const char* RSP_REASON_VAL_IPINUSE="ip already in use";

const unsigned long MIN_REQUEST_QUOTA = 1*1024*1024;
const unsigned int MAX_USERID_LEN = 20;
const unsigned int MAX_DSID_LEN = 40;
const unsigned long DUMMY_INIT_QUOTA = 3*1024*1024;

struct session_handler * ta_cli_reg = NULL;

UserSession* user_sessions = NULL;

UserSession* user_session_alloc(char* userid){
    //now sid is userid
    UserSession* s = malloc(sizeof(UserSession));
    s->sid=strdup(userid);
    s->grantedQuota=0;
    s->leftQuota=0;
    s->dserver_error=false;
    s->errorUsed=0;
    s->pending_d_req=0;
    return s;
}

void user_session_free(UserSession *data){
    if (data->sid!=NULL){
        free(data->sid);
    }
    if (data->d1sid!=NULL){
        free(data->d1sid);
    }
    free(data);
}

void add_user_session(UserSession* us){
    HASH_ADD_STR(user_sessions, sid, us);
}

UserSession* find_user_session(char* sid){
    UserSession* s;
    HASH_FIND_STR(user_sessions, sid, s);
    return s;
}

void delete_user_session(char* sid){
    UserSession* s = find_user_session(sid);
    if (s!=NULL){
        HASH_DEL(user_sessions, s);
        user_session_free(s);
    }
}

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


