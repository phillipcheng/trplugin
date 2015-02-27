//
//  srv_resource.c
//  trplugin
//
//  Created by Cheng Yi on 2/17/15.
//  Copyright (c) 2015 Cheng Yi. All rights reserved.
//

#include "trplugin.h"

UserSessionHM* user_session_hashmap = NULL;
UserSessionDL* user_session_dlinkedlist = NULL;
int user_session_count_stat = 0;
const char id_sep = '|';

UserSession* user_session_alloc(char* userid, char* tenantId){
    //set the sid
    UserSession* us = malloc(sizeof(UserSession));
    us->sid = get_session_id(userid, tenantId);
    us->grantedQuota=0;
    us->leftQuota=0;
    us->dserver_error=false;
    us->errorUsed=0;
    us->pending_d_req=0;
    us->lastUpdateTime=time(NULL);
    pthread_mutex_init(&us->us_lock, NULL);
    return us;
}

char* get_session_id(char* userid, char* tenantId){
    char* sid = malloc(strlen(userid)+1+strlen(tenantId)+1);
    sprintf(sid, "%s%c%s", userid, id_sep, tenantId);
    return sid;
}

void add_user_session(UserSession* us){
    UserSessionHM* ushm = malloc(sizeof(UserSessionHM));
    ushm->sid = strdup(us->sid);
    UserSessionDL* usdl = malloc(sizeof(UserSessionDL));
    usdl->us = us;
    DL_APPEND(user_session_dlinkedlist, usdl);
    ushm->usdl = usdl;
    HASH_ADD_STR(user_session_hashmap, sid, ushm);
}

void user_session_free(UserSession *us){
    if(us!=NULL){
        if (us->d1sid!=NULL){
            free(us->d1sid);
        }
        if (us->sid!=NULL){
            free(us->sid);
        }
        pthread_mutex_destroy(&us->us_lock);
        free(us);
    }
}

void delete_user_session(char* sid){
    UserSessionHM* ushm;
    HASH_FIND_STR(user_session_hashmap, sid, ushm);
    if (ushm!=NULL){
        HASH_DEL(user_session_hashmap, ushm);
        if (ushm->sid!=NULL){
            free(ushm->sid);
        }
        UserSessionDL* usdl = ushm->usdl;
        if (usdl!=NULL){
            UserSession* us = usdl->us;
            user_session_free(us);
            DL_DELETE(user_session_dlinkedlist, usdl);
            free(usdl);
        }
        free(ushm);
    }
}

UserSession* find_user_session(char* sid){
    UserSessionHM* ushm;
    HASH_FIND_STR(user_session_hashmap, sid, ushm);
    if (ushm!=NULL){
        UserSessionDL* usdl = ushm->usdl;
        if (usdl!=NULL){
            return usdl->us;
        }else{
            return NULL;
        }
    }else{
        return NULL;
    }
}

void update_user_session(UserSession* us){
    UserSessionHM* ushm;
    HASH_FIND_STR(user_session_hashmap, us->sid, ushm);
    if (ushm!=NULL){
        UserSessionDL* usdl = ushm->usdl;
        if (usdl!=NULL){
            UserSession* us = usdl->us;
            //set the last update time
            us->lastUpdateTime = time(NULL);
            //move the session to the tail (newest)
            DL_DELETE(user_session_dlinkedlist, usdl);
            DL_APPEND(user_session_dlinkedlist, usdl);
        }
    }
}

//manage DiamTxnData
DiamTxnData* dtxn_alloc(TSHttpTxn txnp, TSCont contp, bool httpReq, d_req_type type){
    DiamTxnData* data = TSmalloc(sizeof(DiamTxnData));
    data->txnp=txnp;
    data->contp=contp;
    data->errid=RSP_REASON_VAL_SUCCESS;
    data->flag=FLAG_NORMAL;
    data->grantedQuota=0;
    data->requestQuota=0;
    data->thisTimeNeed=0;
    data->httpReq = httpReq;
    data->reqType = type;
    data->userId=NULL;
    data->tenantId = NULL;
    data->d1sid = NULL;
    data->dserver_error=false;
    return data;
}

void dtxn_free(DiamTxnData* data){
    if (data->tenantId!=NULL){
        TSfree(data->tenantId);
    }
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
    data->tenant = NULL;
    data->sessionid=NULL;
    return data;
}

void http_txn_data_free(HttpTxnData *data) {
    if (data->user!=NULL){
        TSfree(data->user);
    }
    if (data->tenant!=NULL){
        TSfree(data->tenant);
    }
    if (data->sessionid!=NULL){
        TSfree(data->sessionid);
    }
    TSfree(data);
}

