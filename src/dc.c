//
//  dc.c
//  trplugin
//

#include "dc.h"

/* forward declarations */
static int main_cmdline(int argc, char *argv[]);

static char *conffile = NULL;

int dcinit(int argc, const char * argv[])
{
    fprintf(stderr, "in dcinit.");
    
    int ret;
    
    /* Parse the command-line */
    ret = main_cmdline(argc, argv);
    if (ret != 0) {
        return ret;
    }
    
    TSDebug("debug-dcinit", "dc init!\n");
    
    /* Initialize the core library */
    ret = fd_core_initialize();
    if (ret != 0) {
        fprintf(stderr, "An error occurred during freeDiameter core library initialization.\n");
        return ret;
    }
    
    /* Parse the configuration file */
    CHECK_FCT_DO( fd_core_parseconf(conffile), goto error );
    
    //here I can manually load my plugin
    ta_entry(NULL);
    
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
           "  -c, --config=filename  Read configuration from this file instead of the \n"
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
        { "config",	required_argument, 	NULL, 'c' },
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
        c = getopt_long (argc, argv, "hc:dql:M:", long_options, &option_index);
        if (c == -1)
            break;	/* Exit from the loop.  */
        
        switch (c) {
            case 'h':	/* Print help and exit.  */
                main_help();
                exit(0);
                
            case 'c':	/* Read configuration from this file instead of the default location..  */
                if (optarg == NULL ) {
                    fprintf(stderr, "Missing argument with --config directive\n");
                    return EINVAL;
                }
                conffile = optarg;
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
