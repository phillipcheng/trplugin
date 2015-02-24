//
//  timeout_thread.c
//  trplugin
//
//  Created by Cheng Yi on 2/22/15.
//  Copyright (c) 2015 Cheng Yi. All rights reserved.
//

#include "trplugin.h"

const long us_timeout = 3*30; //seconds
const long us_timeout_check_interval=30;//seconds

void* checkSessionTimeout(void* ptr){
    time_t now;
    while(true){
        TSDebug(DEBUG_NAME, "check timeout session thread runs.\n");
        sleep(us_timeout_check_interval);
        now = time(NULL);
        UserSessionDL* elt, *tmp;
        DL_FOREACH_SAFE(user_session_dlinkedlist, elt, tmp) {
            if (elt!=NULL){
                if (elt->us->lastUpdateTime < now - us_timeout){
                    TSDebug(DEBUG_NAME, "Session timed out for user session: %s\n", elt->us->sid);
                    delete_user_session(elt->us->sid);
                }else{
                    TSDebug(DEBUG_NAME, "have session in queue id:%s, lastupdate till now: %ld\n", elt->us->sid, now-elt->us->lastUpdateTime);
                }
            }else{
                TSDebug(DEBUG_NAME, "null session found.\n");
            }
        }
    }
    return NULL;
}