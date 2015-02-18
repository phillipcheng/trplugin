//
//  srv_resource.c
//  trplugin
//
//  Created by Cheng Yi on 2/17/15.
//  Copyright (c) 2015 Cheng Yi. All rights reserved.
//

#include "trplugin.h"

UserSession* user_sessions = NULL;
int user_session_count_stat = 0;

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
    //TSStatIntIncrement(user_session_count_stat, 1l);
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
        //TSStatIntDecrement(user_session_count_stat, 1l);
    }
}

//manage DiamTxnData
DiamTxnData* dtxn_alloc(TSHttpTxn txnp, TSCont contp, bool httpReq, d_req_type type){
    DiamTxnData* data = TSmalloc(sizeof(DiamTxnData));
    data->txnp=txnp;
    data->contp=contp;
    data->errmsg=NULL;
    data->flag=FLAG_NORMAL;
    data->grantedQuota=0;
    data->requestQuota=0;
    data->thisTimeNeed=0;
    data->httpReq = httpReq;
    data->reqType = type;
    data->userId=NULL;
    data->d1sid = NULL;
    data->dserver_error=false;
    return data;
}

void dtxn_free(DiamTxnData* data){
    if (data->userId!=NULL){
        TSfree(data->userId);
    }
    if (data->d1sid!=NULL){
        TSfree(data->d1sid);
    }
    TSfree(data);
}

//Manager HttpTxnData
HttpTxnData * http_txn_data_alloc() {
    HttpTxnData *data;
    data = TSmalloc(sizeof(HttpTxnData));
    data->flag = FLAG_NORMAL;
    data->user=NULL;
    data->sessionid=NULL;
    return data;
}

void http_txn_data_free(HttpTxnData *data) {
    if (data->user!=NULL){
        TSfree(data->user);
    }
    if (data->sessionid!=NULL){
        TSfree(data->sessionid);
    }
    TSfree(data);
}

