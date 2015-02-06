//
//  dc.h
//  trplugin
//
//  Created by Cheng Yi on 1/31/15.
//  Copyright (c) 2015 Cheng Yi. All rights reserved.
//

#ifndef trplugin_dc_h
#define trplugin_dc_h

#include <freeDiameter/freeDiameter-host.h>
#include <freeDiameter/libfdcore.h>

#include <signal.h>
#include <getopt.h>
#include <locale.h>
#include <ts/ts.h>

extern const int FLAG_AUTH_SUCC;
extern const int FLAG_AUTH_FAILED;
extern const int FLAG_NORMAL;
//context data for txn (req, rsp), put in the continuation for http response processor to get
typedef struct {
    int flag; //indicate whether the authentication succeed or not
    const char* errmsg; //the text reason of failure
} TxnData;

//context data for diameter txn (req, rsp), put in the diameter session for diameter response processor to get
typedef struct {
    TSHttpTxn txnp;
    TSCont contp;
} DiamTxnData;

#define	MODE_CLI	0x2
/* The module configuration */
struct ta_conf {
    uint32_t	vendor_id;	/* default 999999 */
    uint32_t	appli_id;	/* default 123456 */
    uint32_t	cmd_id;		/* default 234567 */
    uint32_t	avp_id;		/* default 345678 */
    uint32_t	long_avp_id;	/* default 0 */
    size_t		long_avp_len;	/* default 5000 */
    char 	*	dest_realm;	/* default local realm */
    char 	*	dest_host;	/* default NULL */
    char 	*	user_name;	/* default NULL */
    struct ta_stats {
        unsigned long long	nb_echoed; /* server */
        unsigned long long	nb_sent;   /* client */
        unsigned long long	nb_recv;   /* client */
        unsigned long long	nb_errs;   /* client */
        unsigned long		shortest;  /* fastest answer, in microseconds */
        unsigned long		longest;   /* slowest answer, in microseconds */
        unsigned long		avg;       /* average answer time, in microseconds */
    } stats;
    pthread_mutex_t		stats_lock;
};
extern struct ta_conf * ta_conf;

TxnData * txn_data_alloc();
void txn_data_free(TxnData *data);

/* Parse the configuration file */
int ta_conf_handle(char * conffile);

/* Create outgoing message (client) */
int ta_cli_init(void);
void ta_cli_fini(void);

/* Initialize dictionary definitions */
int ta_dict_init(void);

int dcinit(int argc, const char * argv[]);

void ta_cli_test_message(DiamTxnData * cbdata);

void http_shortcut_cb(TSHttpTxn txnp, TSCont contp, int flag, const char* errormsg);

void http_continue_cb(TSHttpTxn txnp, TSCont contp);

int ta_entry(char * conffile);

extern const char* DEBUG_NAME;

/* Some global variables for dictionary */
extern struct dict_object * ta_vendor;
extern struct dict_object * ta_appli;
extern struct dict_object * ta_cmd_r;
extern struct dict_object * ta_cmd_a;
extern struct dict_object * ta_avp;
extern struct dict_object * ta_avp_long;

extern struct dict_object * ta_sess_id;
extern struct dict_object * ta_origin_host;
extern struct dict_object * ta_origin_realm;
extern struct dict_object * ta_dest_host;
extern struct dict_object * ta_dest_realm;
extern struct dict_object * ta_user_name;
extern struct dict_object * ta_res_code;

extern const char* RSP_HEADER_REASON;
extern const int RSP_HEADER_REASON_LEN;
extern const char* RSP_REASON_VAL_SUCCESS;
extern const char* RSP_REASON_VAL_NOREQHEAD;
extern const char* RSP_REASON_VAL_REQHEAD_NOUSERIP;
extern const char* RSP_REASON_VAL_NOUSER;
extern const char* RSP_REASON_VAL_NOBAL;
extern const char* RSP_REASON_VAL_NOIPSESSION;
extern const char* RSP_REASON_VAL_USERONLINE;
extern const char* RSP_REASON_VAL_IPINUSE;

#endif
