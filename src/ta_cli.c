
/* Create and send a message, and receive it */

/* Note that we use both sessions and the argument to answer callback to pass the same value.
 * This is just for the purpose of checking everything went OK.
 */

#include "trplugin.h"

#define TEST_APP_SID_OPT  "app_test"

/* Cb called when an answer is received */
void ta_cb_ans(void * data, struct msg ** msg)
{
	struct timespec ts;
	struct avp * avp;
	struct avp_hdr * hdr;
	int error = 0;
    DiamTxnData* dtxn_data = data;
	
	CHECK_SYS_DO( clock_gettime(CLOCK_REALTIME, &ts), return );

	/* Now log content of the answer */
	fprintf(stderr, "RECV ");
	
	/* Value of Result Code */
    CHECK_FCT_DO( fd_msg_search_avp ( *msg, ta_res_code, &avp), return );
	if (avp) {
        CHECK_FCT_DO( fd_msg_avp_hdr( avp, &hdr ), return );
		fprintf(stderr, "Status: %d ", hdr->avp_value->i32);
		if (hdr->avp_value->i32 != 2001)
			error++;
	} else {
		fprintf(stderr, "no_Result-Code \n");
		error++;
	}
    
	/* Value of Origin-Host */
	CHECK_FCT_DO( fd_msg_search_avp ( *msg, ta_origin_host, &avp), return );
	if (avp) {
		CHECK_FCT_DO( fd_msg_avp_hdr( avp, &hdr ), return );
		fprintf(stderr, "From '%.*s' \n", (int)hdr->avp_value->os.len, hdr->avp_value->os.data);
	} else {
		fprintf(stderr, "no_Origin-Host \n");
		error++;
	}
    
	/* Value of Origin-Realm */
	CHECK_FCT_DO( fd_msg_search_avp ( *msg, ta_origin_realm, &avp), return );
	if (avp) {
		CHECK_FCT_DO( fd_msg_avp_hdr( avp, &hdr ), return );
		fprintf(stderr, "('%.*s') ", (int)hdr->avp_value->os.len, hdr->avp_value->os.data);
	} else {
		fprintf(stderr, "no_Origin-Realm \n");
		error++;
	}
    
    if (dtxn_data!=NULL){
        if (dtxn_data->reqType!=d_stop){
            struct avp * src = NULL;
            struct avp_hdr * hdr = NULL;
            
            //get the granted quota
            fd_msg_search_avp ( *msg, ta_avp_grantedQuota, &src);
            if (src){
                CHECK_FCT_DO( fd_msg_avp_hdr( src, &hdr ), return  );
                dtxn_data->grantedQuota = hdr->avp_value->u64;
                if (dtxn_data->grantedQuota<dtxn_data->thisTimeNeed){
                    dtxn_data->flag = FLAG_AUTH_FAILED;
                    //TODO, define/reuse the result type/code
                    dtxn_data->errmsg = RSP_REASON_VAL_NOBAL;
                }else{
                    dtxn_data->flag = FLAG_AUTH_SUCC;
                }
            }else{
                //only handle dserver error for start and update
                TSDebug(DEBUG_NAME, "fatal processing recieve msg. no granted quota AVP found in ans msg.");
                dtxn_data->dserver_error=true;
            }
            
            if (dtxn_data->reqType==d_start){//let it pass
                start_session_cb(dtxn_data);
            }else{
                update_session_cb(dtxn_data);
            }
        }else{
            end_session_cb(dtxn_data);
        }
    }else{
        TSDebug(DEBUG_NAME, "fatal processing recieve msg. dtxn_data is NULL.");
    }
	
    //Free the message, it will free the session associated with it
	CHECK_FCT_DO(fd_msg_free(*msg), return);
	*msg = NULL;

	return;
}

/* Create a test message */
void d_cli_send_msg(DiamTxnData * cbdata)
{
	struct msg * req = NULL;
	struct avp * avp;
	union avp_value val;
	struct session *sess = NULL;
    
	//create the request
	CHECK_FCT_DO( fd_msg_new( ta_cmd_r, MSGFL_ALLOC_ETEID, &req ), goto out );

    TSDebug(DEBUG_NAME, "Creating a new message for sending.\n");
    if (cbdata->reqType==d_start){
        // Create a new request-session
        CHECK_FCT_DO( fd_msg_new_session( req, (os0_t)TEST_APP_SID_OPT, CONSTSTRLEN(TEST_APP_SID_OPT) ), goto out );
        //set diameter session id in the DiamTxnData
        CHECK_FCT_DO( fd_msg_sess_get(fd_g_config->cnf_dict, req, &sess, NULL), goto out );
        os0_t sid;
        size_t len;
        fd_sess_getsid(sess, &sid, &len);
        cbdata->d1sid=os0dup_int(sid, len);
    }else{
        //init the message with given session id
        fd_msg_avp_new( ta_sess_id, 0, &avp);
        memset(&val, 0, sizeof(val));
        val.os.data = (os0_t)cbdata->d1sid;
        val.os.len  = strlen(cbdata->d1sid);
        fd_msg_avp_setvalue( avp, &val );
        fd_msg_avp_add( req, MSG_BRW_FIRST_CHILD, avp );
    }
	/* Set the Destination-Realm AVP */
	{
		CHECK_FCT_DO( fd_msg_avp_new ( ta_dest_realm, 0, &avp ), goto out  );
		val.os.data = (unsigned char *)(ta_conf->dest_realm);
		val.os.len  = strlen(ta_conf->dest_realm);
		CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
		CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
	}
	/* Set the Destination-Host AVP if needed*/
	if (ta_conf->dest_host) {
		CHECK_FCT_DO( fd_msg_avp_new ( ta_dest_host, 0, &avp ), goto out  );
		val.os.data = (unsigned char *)(ta_conf->dest_host);
		val.os.len  = strlen(ta_conf->dest_host);
		CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
		CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
	}
	/* Set Origin-Host & Origin-Realm */
	CHECK_FCT_DO( fd_msg_add_origin ( req, 0 ), goto out  );
	
    //set optype
    {
        CHECK_FCT_DO( fd_msg_avp_new ( ta_avp_optype, 0, &avp ), goto out  );
        val.i32 = cbdata->reqType;
        CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
        CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
    }
    if (cbdata->reqType==d_start){
        //Set userid
		CHECK_FCT_DO( fd_msg_avp_new ( ta_avp_userid, 0, &avp ), goto out  );
        val.os.len = strlen(cbdata->userId);
        val.os.data = (unsigned char *)cbdata->userId;
		CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
		CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
	}
    //set requestQuota
    {
        CHECK_FCT_DO( fd_msg_avp_new ( ta_avp_requestQuota, 0, &avp ), goto out  );
        uint64_t reqSize = cbdata->requestQuota > MIN_REQUEST_QUOTA? cbdata->requestQuota:MIN_REQUEST_QUOTA;
        val.u64 = reqSize;
        CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
        CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
    }
    //set usedQuota
    {
        CHECK_FCT_DO( fd_msg_avp_new ( ta_avp_usedQuota, 0, &avp ), goto out  );
        val.u64 = cbdata->used;
        CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
        CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
    }
	
	/* Log sending the message */
	TSDebug(DEBUG_NAME, "SEND %s,%llu to '%s' (%s)\n", cbdata->userId, cbdata->requestQuota,
            ta_conf->dest_realm, ta_conf->dest_host?:"-" );
		
	/* Send the request */
	CHECK_FCT_DO( fd_msg_send( &req, ta_cb_ans, cbdata), goto out );

	/* Increment the counter */
	CHECK_POSIX_DO( pthread_mutex_lock(&ta_conf->stats_lock), );
	ta_conf->stats.nb_sent++;
	CHECK_POSIX_DO( pthread_mutex_unlock(&ta_conf->stats_lock), );

out:
	return;
}

int ta_cli_init(void)
{    
	CHECK_FCT( fd_sess_handler_create(&ta_cli_reg, (void *)free, NULL, NULL) );
	
	return 0;
}

void ta_cli_fini(void)
{
	CHECK_FCT_DO( fd_sess_handler_destroy(&ta_cli_reg, NULL), /* continue */ );
	
	return;
};
