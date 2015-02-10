
#include <dc.h>

#define TEST_APP_SID_OPT  "app_test"

bool quit=false;

struct test_session_state{
    int32_t		randval;	/* a random value to store in Test-AVP */
    struct timespec ts;		/* Time of sending the message */
};

void ta_rcv_ans(void * data, struct msg ** msg)
{
    struct test_session_state * mi = NULL;
    struct timespec ts;
    struct session * sess;
    struct avp * avp;
    struct avp_hdr * hdr;
    int error = 0;
    
    CHECK_SYS_DO( clock_gettime(CLOCK_REALTIME, &ts), return );
    
    /* Search the session, retrieve its data */
    {
        int new;
        CHECK_FCT_DO( fd_msg_sess_get(fd_g_config->cnf_dict, *msg, &sess, &new), return );
        ASSERT( new == 0 );
        
        CHECK_FCT_DO( fd_sess_state_retrieve( ta_cli_reg, sess, &mi ), return );
        TRACE_DEBUG( INFO, "%p", mi);
        
    }
    
    /* Now log content of the answer */
    fprintf(stderr, "RECV ");
    
    /* Value of Test-AVP */
    CHECK_FCT_DO( fd_msg_search_avp ( *msg, ta_avp, &avp), return );
    if (avp) {
        CHECK_FCT_DO( fd_msg_avp_hdr( avp, &hdr ), return );
        if (hdr->avp_value->i32 == mi->randval) {
            fprintf(stderr, "%x (%s) ", hdr->avp_value->i32, "Ok");
        } else {
            fprintf(stderr, "%x (%s) ", hdr->avp_value->i32, "PROBLEM");
            error++;
        }
    } else {
        fprintf(stderr, "no_Test-AVP ");
        error++;
    }
    
    /* Value of Result Code */
    CHECK_FCT_DO( fd_msg_search_avp ( *msg, ta_res_code, &avp), return );
    if (avp) {
        CHECK_FCT_DO( fd_msg_avp_hdr( avp, &hdr ), return );
        fprintf(stderr, "Status: %d ", hdr->avp_value->i32);
        if (hdr->avp_value->i32 != 2001)
            error++;
    } else {
        fprintf(stderr, "no_Result-Code ");
        error++;
    }
    
    /* Value of Origin-Host */
    CHECK_FCT_DO( fd_msg_search_avp ( *msg, ta_origin_host, &avp), return );
    if (avp) {
        CHECK_FCT_DO( fd_msg_avp_hdr( avp, &hdr ), return );
        fprintf(stderr, "From '%.*s' ", (int)hdr->avp_value->os.len, hdr->avp_value->os.data);
    } else {
        fprintf(stderr, "no_Origin-Host ");
        error++;
    }
    
    /* Value of Origin-Realm */
    CHECK_FCT_DO( fd_msg_search_avp ( *msg, ta_origin_realm, &avp), return );
    if (avp) {
        CHECK_FCT_DO( fd_msg_avp_hdr( avp, &hdr ), return );
        fprintf(stderr, "('%.*s') ", (int)hdr->avp_value->os.len, hdr->avp_value->os.data);
    } else {
        fprintf(stderr, "no_Origin-Realm ");
        error++;
    }
    
    quit = true;
    
    /* Free the message */
    CHECK_FCT_DO(fd_msg_free(*msg), return);
    *msg = NULL;
    free(mi);
    
    return;
}

int ta_send_req(){
    struct msg * req = NULL;
    struct avp * avp;
    union avp_value val;
    struct test_session_state * mi = NULL;
    struct session *sess = NULL;
    
    /* Create the request */
    CHECK_FCT_DO( fd_msg_new( ta_cmd_r, MSGFL_ALLOC_ETEID, &req ), goto out );
    
    /* Create a new session */
    CHECK_FCT_DO( fd_msg_new_session( req, (os0_t)TEST_APP_SID_OPT, CONSTSTRLEN(TEST_APP_SID_OPT) ), goto out );
    CHECK_FCT_DO( fd_msg_sess_get(fd_g_config->cnf_dict, req, &sess, NULL), goto out );
    os0_t sid;
    size_t sidlen;
    fd_sess_getsid(sess, &sid, &sidlen);
    os0_t dupsid = os0dup_int(sid, sidlen);
    fprintf(stderr, "sid: %s\n", dupsid);
    
    /* Create the random value to store with the session */
    mi = malloc(sizeof(struct test_session_state));
    if (mi == NULL) {
        fd_log_debug("malloc failed: %s", strerror(errno));
        goto out;
    }
    
    mi->randval = (int32_t)random();
    
    /* Now set all AVPs values */
    
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
    
    /* Set the User-Name AVP if needed*/
    if (ta_conf->user_name) {
        CHECK_FCT_DO( fd_msg_avp_new ( ta_user_name, 0, &avp ), goto out  );
        val.os.data = (unsigned char *)(ta_conf->user_name);
        val.os.len  = strlen(ta_conf->user_name);
        CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
        CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
    }
    
    /* Set the Test-AVP AVP */
    {
        CHECK_FCT_DO( fd_msg_avp_new ( ta_avp, 0, &avp ), goto out  );
        val.i32 = mi->randval;
        CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
        CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
    }
    
    /* Set the Test-Payload-AVP AVP */
    if (ta_conf->long_avp_id) {
        int l;
        CHECK_FCT_DO( fd_msg_avp_new ( ta_avp_long, 0, &avp ), goto out  );
        CHECK_MALLOC_DO( val.os.data = malloc(ta_conf->long_avp_len), goto out);
        val.os.len = ta_conf->long_avp_len;
        for (l=0; l < ta_conf->long_avp_len; l++)
            val.os.data[l]=l;
        CHECK_FCT_DO( fd_msg_avp_setvalue( avp, &val ), goto out  );
        free(val.os.data);
        CHECK_FCT_DO( fd_msg_avp_add( req, MSG_BRW_LAST_CHILD, avp ), goto out  );
    }
    
    CHECK_SYS_DO( clock_gettime(CLOCK_REALTIME, &mi->ts), goto out );
    
    /* Store this value in the session */
    CHECK_FCT_DO( fd_sess_state_store ( ta_cli_reg, sess, &mi ), goto out );
    
    /* Send the request */
    CHECK_FCT_DO( fd_msg_send( &req, ta_rcv_ans, NULL), goto out );
    
out:
    return 0;
}

int main(int argc, const char * argv[]) {
    printf("Hello, World!\n");
    
    dcinit(argc, argv);
    
    ta_entry(NULL);
    
    //wait for CEA
    sleep(2);
    
    ta_send_req();
    
    while(!quit){
        sleep(1);
    }
}
