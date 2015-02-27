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
#include "utlist.h"

typedef enum { false, true } bool;
typedef enum {d_start=1, d_update=2, d_stop=3} d_req_type;//diameter request op type

extern const unsigned int DIAMETER_SUCCESS;
extern const unsigned int DIAMETER_UNKNOWN_SESSION_ID;//for update and stop
extern const unsigned int DIAMETER_AUTHORIZATION_REJECTED; //maps to user not found for start

int ta_cli_init(void);
void ta_cli_fini(void);

/* Initialize dictionary definitions */
int ta_dict_init(void);

extern const char* DEBUG_NAME;

/* Some global variables for dictionary */
extern struct dict_object * ta_vendor;
extern struct dict_object * ta_appli;
extern struct dict_object * ta_cmd_r;
extern struct dict_object * ta_cmd_a;
//
extern struct dict_object * ta_avp_optype; //start, stop, update
extern struct dict_object * ta_avp_userid; //
extern struct dict_object * ta_avp_tenantid;
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
