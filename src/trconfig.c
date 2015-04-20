//
//  trconfig.c
//

#include "trplugin.h"
#include "confuse.h"

TrConfig g_trconfig;
TrConfig* tr_conf;

/* forward declarations */
static int main_cmdline(int argc, char *argv[]);

static char *dc_conffile = NULL;
static char *tr_conffile = NULL;

static int tr_conf_init(void)
{
    tr_conf = &g_trconfig;
    memset(tr_conf, 0, sizeof(TrConfig));
    
    /* Set the default values */
    tr_conf->diameterVendorId  = 999999;		/* Dummy value */
    tr_conf->diameterAppId   = 0xffffff;	/* dummy value */
    tr_conf->diameterCmdId     = 0xfffffe;	/* Experimental */
    tr_conf->diameterDestRealm = strdup(fd_g_config->cnf_diamrlm);
    tr_conf->diameterDestHost  = NULL;
    tr_conf->minRequestQuota = 1*1024*1024;
    tr_conf->usTimeout = 3*60;
    tr_conf->usTimeoutCheckInterval=60;
    
    //read configuration from dc_conffile
    if (dc_conffile!=NULL){
        cfg_opt_t opts[] ={
            CFG_INT("diameterVendorId", 999999, CFGF_NONE),
            CFG_INT("diameterAppId", 16777215, CFGF_NONE),
			CFG_INT("diameterCmdId", 16777214, CFGF_NONE),
			CFG_STR("diameterDestHost", NULL, CFGF_NONE),
            CFG_INT("minRequestQuota", 1048576, CFGF_NONE),
            CFG_INT("usTimeout", 180, CFGF_NONE),
            CFG_INT("usTimeoutCheckInterval", 60, CFGF_NONE),
            CFG_END()
        };
        cfg_t *cfg;
        cfg = cfg_init(opts, CFGF_NONE);
        if(cfg_parse(cfg, tr_conffile) == CFG_PARSE_ERROR)
            return 1;
        tr_conf->diameterVendorId = (unsigned int)cfg_getint(cfg, "diameterVendorId");
        tr_conf->diameterAppId = (unsigned int)cfg_getint(cfg, "diameterAppId");
        tr_conf->diameterCmdId = (unsigned int)cfg_getint(cfg, "diameterCmdId");
		tr_conf->diameterDestHost = cfg_getstr(cfg, "diameterDestHost");
        tr_conf->minRequestQuota = cfg_getint(cfg, "minRequestQuota");
        tr_conf->usTimeout = cfg_getint(cfg, "usTimeout");
        tr_conf->usTimeoutCheckInterval = (unsigned int)cfg_getint(cfg, "usTimeoutCheckInterval");
        cfg_free(cfg);
    }
    
    return 0;
}

static void tr_conf_dump(void)
{
    TRACE_DEBUG( INFO, "------- tr configuration dump: ---------");
    TRACE_DEBUG( INFO, " Vendor Id .......... : %u", tr_conf->diameterVendorId);
    TRACE_DEBUG( INFO, " Application Id ..... : %u", tr_conf->diameterAppId);
    TRACE_DEBUG( INFO, " Command Id ......... : %u", tr_conf->diameterCmdId);
    TRACE_DEBUG( INFO, " Destination Realm .. : %s", tr_conf->diameterDestRealm ?: "- none -");
    TRACE_DEBUG( INFO, " Destination Host ... : %s", tr_conf->diameterDestHost ?: "- none -");
    TRACE_DEBUG( INFO, " minRequestQuota .. : %lu", tr_conf->minRequestQuota);
    TRACE_DEBUG( INFO, " minRequestQuota ... : %lu", tr_conf->minRequestQuota);
	TRACE_DEBUG( INFO, " usTimeoutCheckInterval ... : %d", tr_conf->usTimeoutCheckInterval);
	TRACE_DEBUG( INFO, " usTimeout ... : %lu", tr_conf->usTimeout);
    TRACE_DEBUG( INFO, "------- /tr configuration dump ---------");
}


/* entry point */
int ta_entry()
{
    tr_conf_init();
    
    tr_conf_dump();
    
    /* Install objects definitions for this test application */
    CHECK_FCT( ta_dict_init() );
    
    CHECK_FCT( ta_cli_init() );
    
    /* Advertise the support for the test application in the peer */
    CHECK_FCT( fd_disp_app_support ( ta_appli, ta_vendor, 1, 0 ) );
    
    return 0;
}

int dcinit(int argc, char * argv[])
{
    int ret;
    
    /* Parse the command-line */
    //this warning can be ignored
    ret = main_cmdline(argc, argv);
    if (ret != 0) {
        return ret;
    }
    
    /* Initialize the core library */
    ret = fd_core_initialize();
    if (ret != 0) {
        fprintf(stderr, "An error occurred during freeDiameter core library initialization.\n");
        return ret;
    }
    
    /* Parse the configuration file */
    CHECK_FCT_DO( fd_core_parseconf(dc_conffile), goto error );
    
    //here I can manually load my plugin
    ta_entry();
    
    /* Start the servers */
    CHECK_FCT_DO( fd_core_start(), goto error );
    
    TRACE_DEBUG(INFO, FD_PROJECT_BINARY " daemon initialized.");
    
    return 0;
error:
    CHECK_FCT_DO( fd_core_shutdown(),  );
    CHECK_FCT( fd_core_wait_shutdown_complete() );
    return -1;
}


/* Display package version */
static void main_version_core(void)
{
    printf("%s, version %s\n", FD_PROJECT_NAME, fd_core_version);
}


/* Print command-line options */
static void main_help( void )
{
    main_version_core();
    printf(	"  This daemon is an implementation of the Diameter protocol\n"
           "  used for Authentication, Authorization, and Accounting (AAA).\n");
    printf("\nUsage:  " FD_PROJECT_BINARY " [OPTIONS]...\n");
    printf( "  -h, --help             Print help and exit\n"
           "  -V, --version          Print version and exit\n"
           "  -c, --dcconfig=filename  Read diameter configuration from this file instead of the \n"
           "  -a, --trconfig=filename Read tr configuration from this file\n"
           "                           default location (" DEFAULT_CONF_PATH "/" FD_DEFAULT_CONF_FILENAME ").\n");
    printf( "\nDebug:\n"
           "  These options are mostly useful for developers\n"
           "  -l, --dbglocale         Set the locale for error messages\n"
           "  -d, --debug             Increase verbosity of debug messages if default logger is used\n"
           "  -q, --quiet             Decrease verbosity if default logger is used\n"
           "  -f, --dbg_func <func>   Enable all traces within the function <func>\n"
           "  -F, --dbg_file <file.c> Enable all traces within the file <file.c> (basename match)\n"
           "  --dbg_gnutls <int>      Enable GNU TLS debug at level <int>\n"
           );
}

/* Parse the command-line */
static int main_cmdline(int argc, char *argv[])
{
    int c;
    int option_index = 0;
    char * locale;
    
    struct option long_options[] = {
        { "help",	no_argument, 		NULL, 'h' },
        { "version",	no_argument, 		NULL, 'V' },
        { "dcconfig",	required_argument, 	NULL, 'c' },
        { "trconfig",	required_argument, 	NULL, 'a' },
        { "debug",	no_argument, 		NULL, 'd' },
        { "quiet",	no_argument, 		NULL, 'q' },
        { "dbglocale",	optional_argument, 	NULL, 'l' },
        { "dbg_func",	required_argument, 	NULL, 'f' },
        { "dbg_file",	required_argument, 	NULL, 'F' },
        { "dbg_gnutls",	required_argument, 	NULL, 'g' },
        { NULL,		0, 			NULL, 0 }
    };
    
    /* Loop on arguments */
    while (1) {
        c = getopt_long (argc, argv, "hF:f:c:dql:M:a:", long_options, &option_index);
        if (c == -1)
            break;	/* Exit from the loop.  */
        
        switch (c) {
            case 'h':	/* Print help and exit.  */
                main_help();
                exit(0);
                
            case 'c':	/* Read diameter configuration from this file instead of the default location..  */
                if (optarg == NULL ) {
                    fprintf(stderr, "Missing argument with --dcconfig directive\n");
                    return EINVAL;
                }
                dc_conffile = optarg;
                break;
            case 'a':
                if (optarg == NULL ) {
                    fprintf(stderr, "Missing argument with --trconfig directive\n");
                    return EINVAL;
                }
                tr_conffile = optarg;
                break;
            case 'l':	/* Change the locale.  */
                locale = setlocale(LC_ALL, optarg?:"");
                if (!locale) {
                    fprintf(stderr, "Unable to set locale (%s)\n", optarg);
                    return EINVAL;
                }
                break;
                
            case 'd':	/* Increase verbosity of debug messages.  */
                fd_g_debug_lvl--;
                break;
                
            case 'f':	/* Full debug for the function with this name.  */
#ifdef DEBUG
                fd_debug_one_function = optarg;
#else /* DEBUG */
                fprintf(stderr, "Error: must compile with DEBUG support to use --dbg_func feature!\n");
                return EINVAL;
#endif /* DEBUG */
                break;
                
            case 'F':	/* Full debug for the file with this name.  */
#ifdef DEBUG
                fd_debug_one_file = basename(optarg);
#else /* DEBUG */
                fprintf(stderr, "Error: must compile with DEBUG support to use --dbg_file feature!\n");
                return EINVAL;
#endif /* DEBUG */
                break;
                
            case 'q':	/* Decrease verbosity then remove debug messages.  */
                fd_g_debug_lvl++;
                break;
                
            case '?':	/* Invalid option.  */
                /* `getopt_long' already printed an error message.  */
                fprintf(stderr, "getopt_long found an invalid character\n");
                return EINVAL;
                
            default:	/* bug: option not considered.  */
                fprintf(stderr, "A command-line option is missing in parser: %c\n", c);
                ASSERT(0);
                return EINVAL;
        }
    }
    
    return 0;
}
