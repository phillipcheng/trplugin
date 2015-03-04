#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "ts/ts.h"
#include "trplugin.h"

void debugBytes(TSHttpTxn txnp, int event){
    long clientReqHdrBytes = TSHttpTxnClientReqHdrBytesGet(txnp);
    long clientReqBdyBytes = TSHttpTxnClientReqBodyBytesGet(txnp);
    long clientRspHdrBytes = TSHttpTxnClientRespHdrBytesGet(txnp);
    long clientRspBdyBytes = TSHttpTxnClientRespBodyBytesGet(txnp);
    long serverReqHdrBytes = TSHttpTxnServerReqHdrBytesGet(txnp);
    long serverReqBdyBytes = TSHttpTxnServerReqBodyBytesGet(txnp);
    long serverRspHdrBytes = TSHttpTxnServerRespHdrBytesGet(txnp);
    long serverRspBdyBytes = TSHttpTxnServerRespBodyBytesGet(txnp);
    TSDebug(DEBUG_NAME, "at event:%d, clientReqHdrBytes:%ld, clientReqBdyBytes:%ld, clientRspHdrBytes:%ld, clientRspBdyBytes:%ld, \
            serverReqHdrBytes:%ld, serverReqBdyBytes:%ld, serverRspHdrBytes:%ld, serverRspBdyBytes:%ld",
            event, clientReqHdrBytes, clientReqBdyBytes, clientRspHdrBytes, clientRspBdyBytes,
            serverReqHdrBytes, serverReqBdyBytes, serverRspHdrBytes, serverRspBdyBytes);
}

char* getHeaderStringAttr(TSMBuffer bufp, TSMLoc hdr_loc, const char* name, int length){
    TSMLoc cmdfield_loc = TSMimeHdrFieldFind(bufp, hdr_loc, name, length);
    const char *cmdval=NULL;
    int cmdval_length;
    if (cmdfield_loc){
        cmdval = TSMimeHdrFieldValueStringGet(bufp, hdr_loc, cmdfield_loc, -1, &cmdval_length);
        if (cmdval!=NULL){
            TSDebug(DEBUG_NAME, "get attribute:%s len:%d", name, cmdval_length);
            char* val = strndup(cmdval, cmdval_length);
            TSHandleMLocRelease(bufp, hdr_loc, cmdfield_loc);
            TSDebug(DEBUG_NAME, "get attribute:%s val:%s", name, val);
            return val;
        }else{
            return NULL;
        }
    }else{
        return NULL;
    }
}

void setHeaderIntAttr(TSMBuffer bufp, TSMLoc hdr_loc, const char* name, int val){
    //TSDebug(DEBUG_NAME, "set attribute:%s val:%s", name, val);
    TSMLoc field_loc;
    TSMimeHdrFieldCreate(bufp, hdr_loc, &field_loc);
    TSMimeHdrFieldNameSet(bufp, hdr_loc, field_loc, name, (int)strlen(name));
    TSMimeHdrFieldValueIntInsert(bufp, hdr_loc, field_loc, -1, val);
    TSMimeHdrFieldAppend(bufp, hdr_loc, field_loc);
    TSHandleMLocRelease(bufp, hdr_loc, field_loc);
}

void setHeaderStringAttr(TSMBuffer bufp, TSMLoc hdr_loc, const char* name, const char* val){
    //TSDebug(DEBUG_NAME, "set attribute:%s val:%s", name, val);
    TSMLoc field_loc;
    TSMimeHdrFieldCreate(bufp, hdr_loc, &field_loc);
    TSMimeHdrFieldNameSet(bufp, hdr_loc, field_loc, name, (int)strlen(name));
    TSMimeHdrFieldValueStringInsert(bufp, hdr_loc, field_loc, -1,  val, (int)strlen(val));
    TSMimeHdrFieldAppend(bufp, hdr_loc, field_loc);
    TSHandleMLocRelease(bufp, hdr_loc, field_loc);
}

void setHttpHdrStatus(TSHttpTxn txnp, int status){
    TSMBuffer bufp;
    TSMLoc hdr_loc;
    if (TSHttpTxnClientRespGet(txnp, &bufp, &hdr_loc) == TS_SUCCESS) {
        TSHttpHdrStatusSet(bufp, hdr_loc, status);
        TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
    }
}

void http_req_shortcut_cb(TSHttpTxn txnp, TSCont contp, int flag,int errid){
    HttpTxnData* ctx = TSContDataGet(contp);
    ctx->errid = errid;
    ctx->flag = flag;
    TSHttpTxnHookAdd(txnp, TS_HTTP_SEND_RESPONSE_HDR_HOOK, contp);
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_ERROR);
}

void http_req_continue_cb(TSHttpTxn txnp, TSCont contp){
    TSHttpTxnHookAdd(txnp, TS_HTTP_SEND_RESPONSE_HDR_HOOK, contp);
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
}

void start_session_cb(DiamTxnData* dtdata){
    if (dtdata->flag==FLAG_AUTH_SUCC||dtdata->dserver_error){
		HttpTxnData* txnp = TSContDataGet(dtdata->contp);
        UserSession* us = user_session_alloc(txnp);
        us->d1sid =strdup(dtdata->d1sid);
        if (!dtdata->dserver_error){
            TSDebug(DEBUG_NAME, "start cmd: session created for user:%s and user-sid:%s and diameter1-sid:%s with quota %llu",
                    dtdata->userId, us->sid, us->d1sid, dtdata->grantedQuota);
            us->grantedQuota = dtdata->grantedQuota;
            us->leftQuota = dtdata->grantedQuota;
        }else{
            TSDebug(DEBUG_NAME, "start cmd: dserver has problem, shortcut session created for user:%s and user-sid:%s and diameter1-sid:%s",
                    dtdata->userId, us->sid, us->d1sid);
            us->dserver_error=true;
        }
        add_user_session(us);
        http_req_shortcut_cb(dtdata->txnp, dtdata->contp, FLAG_AUTH_SUCC, RSP_REASON_VAL_SUCCESS);
    }else{
        http_req_shortcut_cb(dtdata->txnp, dtdata->contp, dtdata->flag, dtdata->errid);
    }
    
    dtxn_free(dtdata);
}

void start_session(TSHttpTxn txnp, TSCont contp){
    //ask AAA server for quota and create the session in the call back if success
    DiamTxnData* dtxnData = dtxn_alloc(txnp, contp, true, d_start);
    HttpTxnData* txnData=TSContDataGet(contp);
    dtxnData->requestQuota = tr_conf->minRequestQuota;
    dtxnData->userId = strdup(txnData->user);
    dtxnData->tenantId = strdup(txnData->tenant);
    dtxnData->used=0;
    d_cli_send_msg(dtxnData);
}

void end_session_cb(DiamTxnData* dtxn_data){
    HttpTxnData* txnData=TSContDataGet(dtxn_data->contp);
    delete_user_session(txnData->sessionid);
    http_req_shortcut_cb(dtxn_data->txnp, dtxn_data->contp, FLAG_AUTH_SUCC, RSP_REASON_VAL_SUCCESS);
    dtxn_free(dtxn_data);
}

void end_session(TSHttpTxn txnp, TSCont contp){
    HttpTxnData* txnData=TSContDataGet(contp);
    UserSession * us = find_user_session(txnData->sessionid);
    if (us==NULL){
        TSDebug(DEBUG_NAME, "error: req: end session: session not found for id: %s", txnData->sessionid);
        http_req_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOUSERSESSION);
    }else{
        DiamTxnData* dtxn_data = dtxn_alloc(txnp, contp, true, d_stop);
        dtxn_data->used = us->grantedQuota-us->leftQuota;
        dtxn_data->d1sid = strdup(us->d1sid);
        TSDebug(DEBUG_NAME, "diameter session in user session is %s", dtxn_data->d1sid);
        d_cli_send_msg(dtxn_data);
    }
}

void update_session_cb(DiamTxnData* dtxn_data){
    HttpTxnData* txnData = TSContDataGet(dtxn_data->contp);
    //get session data
    UserSession* us = find_user_session(txnData->sessionid);
    bool success = false;
    if (us!=NULL){
        pthread_mutex_lock(&us->us_lock);
        us->grantedQuota = dtxn_data->grantedQuota; //new grant
        us->leftQuota = us->grantedQuota-us->errorUsed;//deduct the volume used while waiting for response
        us->pending_d_req=0;//rsp comming back
        if (dtxn_data->flag==FLAG_AUTH_SUCC || dtxn_data->dserver_error){
            if (!dtxn_data->dserver_error){
                //allow
                us->leftQuota = us->grantedQuota - dtxn_data->thisTimeNeed;
                TSDebug(DEBUG_NAME, "req: %d: update session %s with address %p has quota: latest quota: %llu, quota left: %lld, this time usage: %llu, pending_d_request: %d",
                        dtxn_data->httpReq, us->sid, us, us->grantedQuota, us->leftQuota, dtxn_data->thisTimeNeed, us->pending_d_req);
            }else{
                us->dserver_error=true;
                us->errorUsed+=dtxn_data->thisTimeNeed;
                TSDebug(DEBUG_NAME, "req: %d: update dserver_error session: errorUsed %llu, this time usage: %llu",
                        dtxn_data->httpReq, us->errorUsed, dtxn_data->thisTimeNeed);
            }
            success = true;
        }else{
            success = false;
            //block
            TSDebug(DEBUG_NAME, "req: %d, use session: no quota: left:%lld, ask: %llu",
                    dtxn_data->httpReq, us->leftQuota, dtxn_data->thisTimeNeed);
        }
        pthread_mutex_unlock(&us->us_lock);
    }else{
        success = false;
        TSDebug(DEBUG_NAME, "fatal error, in update callback, user session %s not found.", txnData->sessionid);
    }
    if (dtxn_data->httpReq){
        if (success){
            http_req_continue_cb(dtxn_data->txnp, dtxn_data->contp);
        }else{
            http_req_shortcut_cb(dtxn_data->txnp, dtxn_data->contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOBAL);
        }
    }else{
        if (success){
            TSHttpTxnReenable(dtxn_data->txnp, TS_EVENT_HTTP_CONTINUE);
        }else{
            setHttpHdrStatus(dtxn_data->txnp, TS_HTTP_STATUS_UNAUTHORIZED);
            TSHttpTxnReenable(dtxn_data->txnp, TS_EVENT_HTTP_CONTINUE);
        }
    }
    dtxn_free(dtxn_data);
}

void update_session(TSHttpTxn txnp, TSCont contp, long len, bool req){
    HttpTxnData* txnData = TSContDataGet(contp);
    UserSession* us = find_user_session(txnData->sessionid);
    if (us==NULL){
        TSDebug(DEBUG_NAME, "rsp:%d, use session, session not found for id:%s", req, txnData->sessionid);
        if (req){
            http_req_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOUSERSESSION);
        }else{
            setHttpHdrStatus(txnp, TS_HTTP_STATUS_UNAUTHORIZED);
            TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
        }
    }else{
        TSDebug(DEBUG_NAME, "user session id:%s, is req: %d, usage:%ld", txnData->sessionid, req, len);
        pthread_mutex_lock(&us->us_lock);
        if (us->leftQuota>=len || us->dserver_error || us->pending_d_req>0){
            //let it pass under one of these conditions
            //1. has enough quota. 2.diameter server is not responding 3.there is pending d_request (under asking)
            TSDebug(DEBUG_NAME, "allow access for session %s with address %p:leftQuota:%llu, this len:%ld, dserver_error:%d, pending_d_req:%d",
                    us->sid, us, us->leftQuota, len, us->dserver_error, us->pending_d_req);
            if (us->leftQuota>=len){
                us->leftQuota-=len;
            }else{
                us->errorUsed+=len;
            }
			update_user_session(us);
            pthread_mutex_unlock(&us->us_lock);
            if (req){
                http_req_continue_cb(txnp, contp);
            }else{
                setHttpHdrStatus(txnp, TS_HTTP_STATUS_OK);
                TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
            }
        }else{
            pthread_mutex_unlock(&us->us_lock);
            //dserver normal and no left quota
            if (us->grantedQuota==0){
                //no more balance
                if (req){
                    http_req_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOBAL);
                }else{
                    setHttpHdrStatus(txnp, TS_HTTP_STATUS_UNAUTHORIZED);
                    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
                }
            }else{
                //last time server still grants, and I am the 1st to ask
                //send diameter update request asking quota again
                DiamTxnData* dtxn_data = dtxn_alloc(txnp, contp, req, d_update);
                dtxn_data->used = us->grantedQuota-us->leftQuota;
                dtxn_data->d1sid = strdup(us->d1sid);
                dtxn_data->thisTimeNeed = len;
                us->pending_d_req=1;//set this before send, if after, send's callback will set this to 0 and then this will reset it to 1
                d_cli_send_msg(dtxn_data);
            }
        }
    }
}

const char* FLAG_STR_SUCCESS="auth_success";
const char* FLAG_STR_FAILED="auth_failed";
const char* FLAG_STR_NORMAL="normal";

const char* getFlagString(int flag){
	if (flag==FLAG_AUTH_FAILED){
		return FLAG_STR_FAILED;
	}else if (flag==FLAG_AUTH_SUCC){
		return FLAG_STR_SUCCESS;
	}else {
		return FLAG_STR_NORMAL;
	}
}

static void handle_request(TSHttpTxn txnp, TSCont contp){
    uint32_t sesscnt=0;
    fd_sess_getcount(&sesscnt);
	
    TSDebug(DEBUG_NAME, "handle_request called, %d diameter sessions ......\n", sesscnt);
	
    TSMBuffer bufp=NULL;
    TSMLoc hdr_loc=NULL;
    HttpTxnData* httpTxnData = TSContDataGet(contp);
	httpTxnData->clientip = getClientIpStr(txnp);
    if (TSHttpTxnClientReqGet(txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
        TSError("fatal: couldn't retrieve client request header\n");
        http_req_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOREQHEAD);
        TSHandleMLocRelease(bufp, NULL, hdr_loc);
        return;
    }
    char* cmdval = getHeaderStringAttr(bufp, hdr_loc, HEADER_CMD, HEADER_CMD_LEN);
    if (cmdval==NULL){//this is normal request, interim
        httpTxnData->reqType = u_use;
        long l_req_len =TSHttpTxnClientReqHdrBytesGet(txnp)+TSHttpTxnClientReqBodyBytesGet(txnp);
		httpTxnData->sessionid = get_session_id(httpTxnData);
        if (httpTxnData->sessionid!=NULL){
            update_session(txnp, contp, l_req_len, true);
        }else{
            TSDebug(DEBUG_NAME, "req: normal usage request header has not sessionid attribute.");
            http_req_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_REQHEAD_MISSINGINFO);
        }
    }else if (strcmp(HEADER_CMDVAL_START, cmdval)==0){
        httpTxnData->reqType = u_start;
        httpTxnData->user = getHeaderStringAttr(bufp, hdr_loc, HEADER_USERID, HEADER_USERID_LEN);
        httpTxnData->tenant = getHeaderStringAttr(bufp, hdr_loc, HEADER_TENANTID, HEADER_TENANTID_LEN);
        if (httpTxnData->user!=NULL && httpTxnData->tenant!=NULL){
            //check whether session exists
            char* sid = get_session_id(httpTxnData);
            UserSession* us = find_user_session(sid);
            free(sid);
            if (us!=NULL){
                http_req_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_USERONLINE);
            }else{
                start_session(txnp, contp);
            }
        }else{
            TSDebug(DEBUG_NAME, "userid/tenantid header not found for session start request.");
            http_req_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_REQHEAD_MISSINGINFO);
        }
    }else if (strcmp(HEADER_CMDVAL_STOP, cmdval)==0){
        httpTxnData->reqType = u_stop;
        httpTxnData->sessionid = get_session_id(httpTxnData);
        if (httpTxnData->sessionid!=NULL){
            end_session(txnp, contp);
        }else{
            TSDebug(DEBUG_NAME, "sessionid header not found for end session request.");
            httpTxnData->flag = FLAG_AUTH_FAILED;//not normal anymore
            http_req_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_REQHEAD_MISSINGINFO);
        }
    }else{
        TSDebug(DEBUG_NAME, "command not supported: %s\n", cmdval);
    }
    if (cmdval!=NULL){
        free(cmdval);
    }
    TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
    return;
}

static void handle_response(TSHttpTxn txnp, TSCont contp) {
	
    TSMBuffer bufp;
    TSMLoc hdr_loc;
    bool goon=true;
    if (TSHttpTxnClientRespGet(txnp, &bufp, &hdr_loc) == TS_SUCCESS) {
        HttpTxnData *txn_data = TSContDataGet(contp);
        if (txn_data!=NULL){
            TSDebug(DEBUG_NAME, "handle rsp: txn_data flag:%s", getFlagString(txn_data->flag));
            if (txn_data->flag == FLAG_AUTH_FAILED){//this can be start, normal or stop requests
                if (txn_data->reqType==u_stop){
                    TSHttpHdrStatusSet(bufp, hdr_loc, TS_HTTP_STATUS_OK);
                }else{
                    TSHttpHdrStatusSet(bufp, hdr_loc, TS_HTTP_STATUS_UNAUTHORIZED);
                    setHeaderIntAttr(bufp, hdr_loc, RSP_HEADER_REASON, txn_data->errid);
                }
            }else if (txn_data->flag==FLAG_AUTH_SUCC){
                TSHttpHdrStatusSet(bufp, hdr_loc, TS_HTTP_STATUS_OK);
            }else if (txn_data->flag==FLAG_NORMAL){
                goon = false; //continue will be re-enabled by callback
                long l_req_cnt_len = TSHttpTxnClientRespHdrBytesGet(txnp) + TSHttpTxnClientRespBodyBytesGet(txnp);
                update_session(txnp, contp, l_req_cnt_len, false);
            }
        }else{
            TSDebug(DEBUG_NAME, "error the txn_data must be set for all cont in all txn.");
        }
    }else{
        TSError("couldn't retrieve client response header\n");
    }
    TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
    if (goon){
        TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    }
}

//added the rsp length to used
static void postprocess_any_request(TSHttpTxn txnp, TSCont contp){
	
	long clientRspHdrBytes = TSHttpTxnClientRespHdrBytesGet(txnp);
	long clientRspBdyBytes = TSHttpTxnClientRespBodyBytesGet(txnp);
	long len = clientRspHdrBytes+clientRspBdyBytes;
	HttpTxnData* txnData = TSContDataGet(contp);
	if (txnData->reqType==u_start && txnData->flag==FLAG_AUTH_FAILED){
		//no op for start and failed
	}else{
		if (txnData->reqType==u_stop){
			//no op for stop
		}else{
			if (txnData->sessionid!=NULL){
				UserSession* us = find_user_session(txnData->sessionid);
				if (us!=NULL){
					pthread_mutex_lock(&us->us_lock);
					if (us->dserver_error){
						us->errorUsed+=len;
						TSDebug(DEBUG_NAME, "postprocess any request, session id:%s, usage this time:%ld, errorUsed:%llu",
								txnData->sessionid, len, us->errorUsed);
					}else{
						us->leftQuota-=len;
						TSDebug(DEBUG_NAME, "postprocess any request, session id:%s, usage this time:%ld, leftQuota:%lld",
								txnData->sessionid, len, us->leftQuota);
					}
					pthread_mutex_unlock(&us->us_lock);
				}else{
					TSDebug(DEBUG_NAME, "postprocess any request, session not found for %s, usage this time:%ld",
							txnData->sessionid, len);
				}
			}
		}
	}
}

static int tr_plugin(TSCont txn_contp, TSEvent event, void *edata)
{
    TSHttpTxn txnp = (TSHttpTxn) edata;
    HttpTxnData *txn_data = TSContDataGet(txn_contp);
    
    //debugBytes(txnp, event);
    switch (event) {
        case TS_EVENT_HTTP_READ_REQUEST_HDR:
            handle_request(txnp, txn_contp);
			
			//will be removed, for analyzing
			//TSHttpTxnHookAdd(txnp, TS_HTTP_REQUEST_TRANSFORM_HOOK, txn_contp);
			//TSHttpTxnHookAdd(txnp, TS_HTTP_RESPONSE_TRANSFORM_HOOK, txn_contp);
			//TSHttpTxnHookAdd(txnp, TS_HTTP_RESPONSE_CLIENT_HOOK, txn_contp);
			
            return 0;
        case TS_EVENT_HTTP_SEND_RESPONSE_HDR:
            handle_response(txnp, txn_contp);
            return 0;
        case TS_EVENT_HTTP_TXN_CLOSE:
            postprocess_any_request(txnp, txn_contp);
            http_txn_data_free(txn_data);
            TSContDestroy(txn_contp);
            TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
            return 0;
		default:
			TSDebug(DEBUG_NAME, "error unexpected event %d handling..\n", event);
			//
			TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
			return 0;
			break;
    }
    
    return 0;
}

static int tr_global_plugin(TSCont contp, TSEvent event, void *edata){
    TSHttpTxn txnp = (TSHttpTxn) edata;
    TSCont txn_contp;
    HttpTxnData *txn_data;
    
    switch (event) {
        case TS_EVENT_HTTP_TXN_START:
            /* Create a new continuation for this txn and associate data to it */
            txn_contp = TSContCreate(tr_plugin, TSMutexCreate());
            txn_data = http_txn_data_alloc();
            TSContDataSet(txn_contp, txn_data);
            
            /* Registers locally to hook READ_REQUEST and TXN_CLOSE */
            TSHttpTxnHookAdd(txnp, TS_HTTP_READ_REQUEST_HDR_HOOK, txn_contp);
            TSHttpTxnHookAdd(txnp, TS_HTTP_TXN_CLOSE_HOOK, txn_contp);

            break;
            
		default:
			TSDebug(DEBUG_NAME, "error unexpected event %d handling..\n", event);
            break;
    }
    
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    return 1;
}

const char* user_session_count_name = "user.session.count";

void TSPluginInit(int argc, const char *argv[])
{
    TSDebug(DEBUG_NAME, "Load TRPlugin......\n");
    
    //warning is ok
    dcinit(argc, argv);
    
    TSCont    cont = TSContCreate(tr_global_plugin, NULL);
    TSHttpHookAdd(TS_HTTP_TXN_START_HOOK, cont);
    
    pthread_t sessionTimeoutThread;
    //start session timeout thread
    if(pthread_create(&sessionTimeoutThread, NULL, checkSessionTimeout, NULL)) {
        TSDebug(DEBUG_NAME, "Error creating Session Timeout Thread.\n");
    }else{
        TSDebug(DEBUG_NAME, "Session Timeout Thread Created.\n");
    }
    //define stat
    user_session_count_stat = TSStatCreate(user_session_count_name, TS_RECORDDATATYPE_INT, TS_STAT_NON_PERSISTENT, TS_STAT_SYNC_COUNT);
    
    TSDebug(DEBUG_NAME, "user_session_count_stat:%d", user_session_count_stat);
}

