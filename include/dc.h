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
#include <freeDiameter/extension.h>

#include <signal.h>
#include <getopt.h>
#include <locale.h>
#include "uthash.h"

typedef enum { false, true } bool;
typedef enum {d_start=1, d_update=2, d_stop=3} d_req_type;

extern const int DC_CLIENT;
extern const int DC_SERVER;

extern const unsigned long MIN_REQUEST_QUOTA;
extern const unsigned int MAX_USERID_LEN;
extern const unsigned int MAX_DSID_LEN;
extern const unsigned long DUMMY_INIT_QUOTA;

/* The module configuration */
struct ta_conf {
    uint32_t	vendor_id;	/* default 999999 */
    uint32_t	appli_id;	/* default 123456 */
    uint32_t	cmd_id;		/* default 234567 */
    uint32_t	avp_id;		/* default 345678 */
    uint32_t	long_avp_id;	/* default 0 */
    size_t		long_avp_len;	/* default 5000 */
    uint16_t    mode; //client=1, server=2
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

//user session structure, global user-sessions handler and methods
typedef struct {
    char    sid[40];//
    uint64_t grantedQuota;
    int64_t leftQuota; //can be less the zero for over-use
    char* d1sid;//diameter 1 session id
    UT_hash_handle hh; /* makes this structure hashable */
}UserSession;
extern UserSession* user_sessions;
UserSession * user_session_alloc(char* userid);
void uses_session_free(UserSession *data);
void add_user_session(UserSession* us);
UserSession* find_user_session(char* sid);
void delete_user_session(char* sid);

typedef struct {
    uint64_t leftQuota;
    char* sid;//diameter session id
}UsageServerSession;
UsageServerSession * usageserver_session_alloc();
void usageserver_session_free(UsageServerSession *data);

/* Parse the configuration file */
int ta_conf_handle(char * conffile);

int ta_cli_init(void);
void ta_cli_fini(void);
int ta_serv_init(void);
void ta_serv_fini(void);

/* Initialize dictionary definitions */
int ta_dict_init(void);

int dcinit(int argc, const char * argv[]);

int ta_entry(char * conffile);

extern const char* DEBUG_NAME;

/* Some global variables for dictionary */
extern struct dict_object * ta_vendor;
extern struct dict_object * ta_appli;
extern struct dict_object * ta_cmd_r;
extern struct dict_object * ta_cmd_a;
//
extern struct dict_object * ta_avp_optype; //start, stop, update
extern struct dict_object * ta_avp_userid; //
extern struct dict_object * ta_avp_requestQuota;
extern struct dict_object * ta_avp_usedQuota;
extern struct dict_object * ta_avp_grantedQuota;
extern struct dict_object * ta_avp;
extern struct dict_object * ta_avp_long;

extern struct dict_object * ta_sess_id;
extern struct dict_object * ta_origin_host;
extern struct dict_object * ta_origin_realm;
extern struct dict_object * ta_dest_host;
extern struct dict_object * ta_dest_realm;
extern struct dict_object * ta_user_name;
extern struct dict_object * ta_res_code;

extern struct session_handler * ta_cli_reg;

#endif
