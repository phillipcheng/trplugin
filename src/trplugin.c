#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "ts/ts.h"
#include "dc.h"
#include "uthash.h"

const char* DEBUG_NAME="trafficreport";

const char* HEADER_CMD = "command";
const int HEADER_CMD_LEN= 7;
const char* HEADER_CMDVAL_START = "start";
const char* HEADER_CMDVAL_STOP = "stop";
const char* HEADER_CMDVAL_NO ="no";

const char* HEADER_USERID = "userid";
const int HEADER_USERID_LEN= 6;

const int FLAG_AUTH_FAILED=1;
const int FLAG_AUTH_SUCC=2;
const int FLAG_NORMAL=3;

const char* RSP_HEADER_REASON = "rejectreason";
const int RSP_HEADER_REASON_LEN=12;
const char* RSP_REASON_VAL_SUCCESS="command succeed";
const char* RSP_REASON_VAL_NOREQHEAD="no req head";
const char* RSP_REASON_VAL_REQHEAD_NOUSERIP="no user/ip in the start request header";
const char* RSP_REASON_VAL_NOUSER="no such user";
const char* RSP_REASON_VAL_NOBAL="no balance";
const char* RSP_REASON_VAL_NOIPSESSION="no ip session";
const char* RSP_REASON_VAL_USERONLINE="user already online";
const char* RSP_REASON_VAL_IPINUSE="ip already in use";

const long DUMMY_INITV = 1 * 1024 * 1024; //1Mbytes

struct user_ip {
    char user[20];
    char ip[20];
    UT_hash_handle hh;
};
struct user_ip *userips = NULL;

struct ip_volume {
    char ip[20];
    long volume;
    char user[20];
    UT_hash_handle hh;
};
struct ip_volume *ipvolumes = NULL;

void add_user_ip (char* user, char* ip){
    struct user_ip *s;
    s = malloc(sizeof(struct user_ip));
    strcpy(s->user, user);
    strcpy(s->ip, ip);
    HASH_ADD_STR(userips, user, s);
}
struct user_ip *find_user_ip(char* user){
    struct user_ip *s;
    HASH_FIND_STR(userips, user, s);
    return s;
}
void delete_user_ip(struct user_ip *s){
    HASH_DEL(userips, s);
    free(s);
}

void add_ip_volume (char* ip, char* user, long volume){
    struct ip_volume *s;
    s = malloc(sizeof(struct ip_volume));
    strcpy(s->ip, ip);
    strcpy(s->user, user);
    s->volume = volume;
    HASH_ADD_STR(ipvolumes, ip, s);
}
struct ip_volume *find_ip_volume(char* ip){
    struct ip_volume *s;
    HASH_FIND_STR(ipvolumes, ip, s);
    return s;
}
void delete_ip_volume(struct ip_volume *s){
    HASH_DEL(ipvolumes, s);
    free(s);
}

char* getHeaderAttr(TSMBuffer bufp, TSMLoc hdr_loc, const char* name, int length){
    TSMLoc cmdfield_loc = TSMimeHdrFieldFind(bufp, hdr_loc, name, length);
    const char *cmdval=NULL;
    int cmdval_length;
    if (cmdfield_loc){
        cmdval = TSMimeHdrFieldValueStringGet(bufp, hdr_loc, cmdfield_loc, -1, &cmdval_length);
        char* val = TSmalloc(cmdval_length+1);
        TSstrlcpy(val, cmdval, cmdval_length+1);
        *(val+cmdval_length+1)='\0';
        TSHandleMLocRelease(bufp, hdr_loc, cmdfield_loc);
        TSDebug(DEBUG_NAME, "get attribute:%s val:%s", name, val);
        return val;
    }else{
        return NULL;
    }
}

char* getClientIp(TSHttpTxn txnp){
    struct sockaddr const* incoming_addr = TSHttpTxnIncomingAddrGet(txnp);
    char* incoming_str = malloc(INET6_ADDRSTRLEN);
    const char* ptr = inet_ntop(incoming_addr->sa_family, incoming_addr, incoming_str, INET6_ADDRSTRLEN);
    if (ptr!=NULL){
        TSDebug(DEBUG_NAME, "src ip: after inet_ntop: %s", incoming_str);
    }else{
        TSDebug(DEBUG_NAME, "failed: src ip: after inet_ntop: %s, with reason %s", incoming_str, strerror(errno));
    }
    
    return incoming_str;
}

void setHeaderAttr(TSMBuffer bufp, TSMLoc hdr_loc, const char* val, int length){
    TSMLoc field_loc;
    TSMimeHdrFieldCreate(bufp, hdr_loc, &field_loc);
    TSMimeHdrFieldNameSet(bufp, hdr_loc, field_loc, TS_MIME_FIELD_PROXY_AUTHENTICATE, TS_MIME_LEN_PROXY_AUTHENTICATE);
    TSMimeHdrFieldValueStringInsert(bufp, hdr_loc, field_loc, -1,  val, length);
    TSMimeHdrFieldAppend(bufp, hdr_loc, field_loc);
    TSHandleMLocRelease(bufp, hdr_loc, field_loc);
}

void startsession(char *user, char* ip, TSHttpTxn txnp, TSCont contp){
    //check
    struct user_ip* sui = find_user_ip(user);
    if (sui==NULL){
        //grant the DUMMY_INTV bytes to the user
        struct ip_volume * siv = find_ip_volume(ip);
        if (siv==NULL){
            //new session
            TSDebug(DEBUG_NAME, "req: start session: success adding session for user:%s and ip:%s with quota %lu", user, ip, DUMMY_INITV);
            add_user_ip(user, ip);
            add_ip_volume(ip, user, DUMMY_INITV);
            http_shortcut_cb(txnp, contp, FLAG_AUTH_SUCC, RSP_REASON_VAL_SUCCESS);
        }else{
            TSDebug(DEBUG_NAME, "req: start session: user:%s session not exist, but ip:%s session exists", user, ip);
            //new user, but the client ip exists
            http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_IPINUSE);
        }
    }else{
        TSDebug(DEBUG_NAME, "req: start session: user:%s is online with ip:%s", sui->user, sui->ip);
        //user session exists
        http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_USERONLINE);
    }
    /*
    struct da_cb * pdacb = malloc(sizeof(struct da_cb));
    pdacb->contp = contp;
    pdacb->txnp = txnp;
    TSDebug(DEBUG_NAME, "cmd is start, invoke dc send");
    ta_cli_test_message(pdacb);
     */
}

void endsession(char* user, TSHttpTxn txnp, TSCont contp){
    struct user_ip* sui = find_user_ip(user);
    if (sui==NULL){
        TSDebug(DEBUG_NAME, "req: end session: user:%s session not exist.", user);
        http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOUSER);
    }else{
        struct ip_volume * siv = find_ip_volume(sui->ip);
        if (siv==NULL){
            TSDebug(DEBUG_NAME, "req: end session: user:%s session exist, but ip:%s session not exist.", user, sui->ip);
            http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOIPSESSION);
        }else{
            TSDebug(DEBUG_NAME, "req: end session: success removing session for user:%s and ip:%s", user, sui->ip);
            delete_user_ip(sui);
            delete_ip_volume(siv);
            http_shortcut_cb(txnp, contp, FLAG_AUTH_SUCC, RSP_REASON_VAL_SUCCESS);
        }
    }
    /*
    struct da_cb * pdacb = malloc(sizeof(struct da_cb));
    pdacb->contp = contp;
    pdacb->txnp = txnp;
    TSDebug(DEBUG_NAME, "cmd is end, invoke dc send");
    ta_cli_test_message(pdacb);
     */
}

void use_session_req(char* ip, long len, TSHttpTxn txnp, TSCont contp){
    struct ip_volume * siv = find_ip_volume(ip);
    if (siv==NULL){
        TSDebug(DEBUG_NAME, "req: use session: session not found for ip:%s", ip);
        http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOIPSESSION);
    }else{
        if (siv->volume>len){
            siv->volume-=len;
            TSDebug(DEBUG_NAME, "req: use session: session usage old: %lu, session usage new: %lu", siv->volume+len, siv->volume);
            http_continue_cb(txnp, contp);
        }else{
            TSDebug(DEBUG_NAME, "req: use session: volume quota used up, left:%lu, ask: %lu", siv->volume, len);
            http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOBAL);
        }
    }
}

void use_session_rsp(char* ip, long len, TSMBuffer bufp, TSMLoc hdr_loc){
    struct ip_volume * siv = find_ip_volume(ip);
    if (siv==NULL){
        TSDebug(DEBUG_NAME, "rsp: use session: session not found for ip:%s", ip);
        TSHttpHdrStatusSet(bufp, hdr_loc, TS_HTTP_STATUS_UNAUTHORIZED);
    }else{
        if (siv->volume>len){
            siv->volume-=len;
            TSDebug(DEBUG_NAME, "rsp: use session: session usage old: %lu, session usage new: %lu", siv->volume+len, siv->volume);
        }else{
            TSDebug(DEBUG_NAME, "rsp: use session: volume quota used up, left:%lu, ask: %lu", siv->volume, len);
            TSHttpHdrStatusSet(bufp, hdr_loc, TS_HTTP_STATUS_UNAUTHORIZED);
        }
    }
}

void http_shortcut_cb(TSHttpTxn txnp, TSCont contp, int flag, const char* errormsg){
    TxnData* ctx = TSContDataGet(contp);
    ctx->errmsg = errormsg;
    ctx->flag = flag;
    TSHttpTxnHookAdd(txnp, TS_HTTP_SEND_RESPONSE_HDR_HOOK, contp);
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_ERROR);
}

void http_continue_cb(TSHttpTxn txnp, TSCont contp){
    TSHttpTxnHookAdd(txnp, TS_HTTP_SEND_RESPONSE_HDR_HOOK, contp);
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
}

static void handle_request(TSHttpTxn txnp, TSCont contp){
    TSDebug(DEBUG_NAME, "handle_request called ......\n");
    TSMBuffer bufp=NULL;
    TSMLoc hdr_loc=NULL;
    
    if (TSHttpTxnClientReqGet(txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
        TSError("couldn't retrieve client request header\n");
        http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_NOREQHEAD);
        TSHandleMLocRelease(bufp, NULL, hdr_loc);
        return;
    }
    const char* cmdval = getHeaderAttr(bufp, hdr_loc, HEADER_CMD, HEADER_CMD_LEN);
    TSDebug(DEBUG_NAME, "cmd value:%s", cmdval);
    if (cmdval==NULL){//this is normal request, interim
        int clientReqHdrBytes = TSHttpTxnClientReqHdrBytesGet(txnp);
        int64_t clientReqBodyBytes = TSHttpTxnClientReqBodyBytesGet(txnp);
        long l_req_len =clientReqHdrBytes+clientReqBodyBytes;
        char* ip = getClientIp(txnp);
        if (ip!=NULL){
            use_session_req(ip, l_req_len, txnp, contp);
            TSfree(ip);
        }else{
            TSDebug(DEBUG_NAME, "req: request header has not client ip attribute.");
            http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_REQHEAD_NOUSERIP);
        }
    }else if (strcmp(HEADER_CMDVAL_START, cmdval)==0){
        char* user = getHeaderAttr(bufp, hdr_loc, HEADER_USERID, HEADER_USERID_LEN);
        char* ip = getClientIp(txnp);
        if (user!=NULL && ip!=NULL){
            startsession(user, ip, txnp, contp);
            TSfree(user);
            TSfree(ip);
        }else{
            TSDebug(DEBUG_NAME, "userid/ip header not found for session start request.");
            http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_REQHEAD_NOUSERIP);
        }
    }else if (strcmp(HEADER_CMDVAL_STOP, cmdval)==0){
        char* user = getHeaderAttr(bufp, hdr_loc, HEADER_USERID, HEADER_USERID_LEN);
        if (user!=NULL){
            endsession(user, txnp, contp);
            TSfree(user);
        }else{
            TSDebug(DEBUG_NAME, "userid header not found for end session request.");
            http_shortcut_cb(txnp, contp, FLAG_AUTH_FAILED, RSP_REASON_VAL_REQHEAD_NOUSERIP);
        }
    }
    TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
    return;
}

static void handle_response(TSHttpTxn txnp, TSCont contp) {
    TSMBuffer bufp;
    TSMLoc hdr_loc;
    if (TSHttpTxnClientRespGet(txnp, &bufp, &hdr_loc) == TS_SUCCESS) {
        TxnData *txn_data = TSContDataGet(contp);
        if (txn_data!=NULL){
            TSDebug(DEBUG_NAME, "txn_data flag:%d", txn_data->flag);
            if (txn_data->flag == FLAG_AUTH_FAILED){
                TSHttpHdrStatusSet(bufp, hdr_loc, TS_HTTP_STATUS_UNAUTHORIZED);
                setHeaderAttr(bufp, hdr_loc, txn_data->errmsg, (int)strlen(txn_data->errmsg));
            }else if (txn_data->flag==FLAG_AUTH_SUCC){
                TSHttpHdrStatusSet(bufp, hdr_loc, TS_HTTP_STATUS_OK);
            }else if (txn_data->flag==FLAG_NORMAL){
                char* ip = getClientIp(txnp);
                if (ip!=NULL){
                    long l_req_cnt_len = TSHttpTxnServerRespHdrBytesGet(txnp) + TSHttpTxnServerRespBodyBytesGet(txnp);
                    use_session_rsp(ip, l_req_cnt_len, bufp, hdr_loc);
                    TSfree(ip);
                }else{
                    TSDebug(DEBUG_NAME, "error rsp header has no client ip attribute.");
                }
            }
        }else{
            TSDebug(DEBUG_NAME, "error the txn_data must be set for all cont in all txn.");
        }
    }else{
        TSError("couldn't retrieve client response header\n");
    }
    TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
}

TxnData * txn_data_alloc() {
    TxnData *data;
    data = TSmalloc(sizeof(TxnData));
    data->flag = FLAG_NORMAL;
    return data;
}

void txn_data_free(TxnData *data) {
    TSfree(data);
}

static int tr_plugin(TSCont contp, TSEvent event, void *edata)
{
    TSHttpTxn txnp = (TSHttpTxn) edata;
    TxnData *txn_data = TSContDataGet(contp);
    
    switch (event) {
        case TS_EVENT_HTTP_READ_REQUEST_HDR:
            handle_request(txnp, contp);
            return 0;
        case TS_EVENT_HTTP_SEND_RESPONSE_HDR:
            handle_response(txnp, contp);
            return 0;
        case TS_EVENT_HTTP_TXN_CLOSE:
            txn_data_free(txn_data);
            TSContDestroy(contp);
            TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
            return 0;
        default:
            TSAssert(!"Unexpected event");
            break;
    }
    
    return 0;
}

static int tr_global_plugin(TSCont contp, TSEvent event, void *edata){
    TSHttpTxn txnp = (TSHttpTxn) edata;
    TSCont txn_contp;
    TxnData *txn_data;
    
    switch (event) {
        case TS_EVENT_HTTP_TXN_START:
            /* Create a new continuation for this txn and associate data to it */
            txn_contp = TSContCreate(tr_plugin, TSMutexCreate());
            txn_data = txn_data_alloc();
            TSContDataSet(txn_contp, txn_data);
            
            /* Registers locally to hook READ_REQUEST and TXN_CLOSE */
            TSHttpTxnHookAdd(txnp, TS_HTTP_READ_REQUEST_HDR_HOOK, txn_contp);
            TSHttpTxnHookAdd(txnp, TS_HTTP_TXN_CLOSE_HOOK, txn_contp);
            break;
            
        default:
            TSAssert(!"Unexpected event");
            break;
    }
    
    TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
    return 1;
}

void TSPluginInit(int argc, const char *argv[])
{
    TSDebug(DEBUG_NAME, "Load TRPlugin......\n");
    
    dcinit(argc, argv);
    
    TSCont    cont = TSContCreate(tr_global_plugin, NULL);
    TSHttpHookAdd(TS_HTTP_TXN_START_HOOK, cont);
}

