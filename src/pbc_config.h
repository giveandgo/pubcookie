/*

    Copyright 1999, University of Washington.  All rights reserved.

     ____        _                     _    _
    |  _ \ _   _| |__   ___ ___   ___ | | _(_) ___
    | |_) | | | | '_ \ / __/ _ \ / _ \| |/ / |/ _ \
    |  __/| |_| | |_) | (_| (_) | (_) |   <| |  __/
    |_|    \__,_|_.__/ \___\___/ \___/|_|\_\_|\___|


    All comments and suggestions to pubcookie@cac.washington.edu
    More info: http://www.washington.edu/computing/pubcookie/
    Written by the Pubcookie Team

    this is a pubcookie include file for macros that define the 
    way the pubcookie module does stuff

    logic for how the pubcookie include files are devided up:
       libpubcookie.h: only stuff used in library
       pubcookie.h: stuff used in the module and library
       pbc_config.h: stuff used in the module and library that 
            people might want to change, as far a local configuration
       pbc_version.h: only version stuff

 */

/*
 *  $Revision: 1.52 $
 */

#ifndef PUBCOOKIE_CONFIG
#define PUBCOOKIE_CONFIG

#if defined (APACHE1_2) || defined (APACHE1_3)
#define APACHE
#endif

#include "pbc_myconfig.h" 

#ifndef PBC_PATH
#  if defined (WIN32)
#    define PBC_PATH "\\System32\\inetsrv\\pubcookie\\"
#  else 
#    define PBC_PATH "/usr/www/pubcookie/"
#  endif
#endif

/* where the runtime configuration file lives */
#define PBC_CONFIG (PBC_PATH "config")

/* names of the login servers */
#define PBC_CONFIGED
#define PBC_LOGIN_HOST (libpbc_config_getstring("login_host", "weblogin.washington.edu"))
#define PBC_LOGIN_URI (libpbc_config_getstring("login_uri", ""))
#define PBC_LOGIN_TEST_HOST (libpbc_config_getstring("login_test_host", "weblogintest.cac.washington.edu"))
#define PBC_LOGIN_PROD_TEST_HOST (libpbc_config_getstring("login_prod_test_host", "webloginprodtest.cac.washington.edu"))
#define PBC_LOGIN_TEST_PAGE (libpbc_config_getstring("login_test_page", "https://" PBC_LOGIN_TEST_HOST "/" PBC_LOGIN_URI))
#define PBC_ENTRPRS_DOMAIN (libpbc_config_getstring("enterprise_domain", ".washington.edu"))

/* lives only on application server */
extern const char *PBC_S_CERTFILE;
/* lives only on application server */
extern const char *PBC_S_KEYFILE;

/* lives everywhere (on application servers & login server) */
extern const char *PBC_G_CERTFILE;

/* lives only on login server */
extern const char *PBC_G_KEYFILE;

/* the login server builds it's key Filenames from the hostname     */
#define PBC_KEY_DIR (PBC_PATH "keys/")
#define PBC_CRYPT_KEY_PREFIX "c_key"
#define PBC_L_PUBKEY_FILE_PREFIX "pubcookie_login.cert"
#define PBC_L_PRIVKEY_FILE_PREFIX "pubcookie_login.key"
#define PBC_G_PRIVKEY_FILE_PREFIX "pubcookie_granting.key"

#define PBC_DEFAULT_AUTHTYPE "webiso-vanilla"

#define PBC_REFRESH_TIME 0
#define PBC_MIN_INACT_EXPIRE 	      ( 5 * 60 )
#define PBC_DEFAULT_INACT_EXPIRE     ( 30 * 60 )
#define PBC_UNSET_INACT_EXPIRE                 0
#define PBC_MIN_HARD_EXPIRE 	 ( 1 * 60 * 60 )
#define PBC_MAX_HARD_EXPIRE 	( 12 * 60 * 60 )
#define PBC_DEFAULT_HARD_EXPIRE  ( 8 * 60 * 60 )
#define PBC_UNSET_HARD_EXPIRE                  0
#define PBC_DEFAULT_EXPIRE_LOGIN ( 8 * 60 * 60 )
#define PBC_GRANTING_EXPIRE               ( 60 )
#define PBC_BAD_AUTH 1
#define PBC_BAD_USER 2
#define PBC_FORCE_REAUTH 3

#define PBC_DEFAULT_DIRDEPTH 0

#define PBC_OK   1
#define PBC_FAIL 0
#define PBC_TRUE   1
#define PBC_FALSE  0

/* the cookies; l, g, and s have the same format g request and pre s
   are different internally
 */
/* the formmulti part will probably only hang around until will correctly
   handle form/multipart
 */
#define PBC_L_COOKIENAME "pubcookie_l"
#define PBC_G_COOKIENAME "pubcookie_g"
#define PBC_G_REQ_COOKIENAME "pubcookie_g_req"
#define PBC_S_COOKIENAME "pubcookie_s"
#define PBC_PRE_S_COOKIENAME "pubcookie_pre_s"
#define PBC_FORM_MP_COOKIENAME "pubcookie_formmultipart"

/* this apache module stuff should go into something like mod_pubcookie.h */
#define PBC_AUTH_FAILED_HANDLER "pubcookie-failed-handler"
#define PBC_BAD_USER_HANDLER "pubcookie-bad-user"
#define PBC_END_SESSION_REDIR_HANDLER "pubcookie-end-session-redir-handler"

/* set in apache config to clear session cookie and redirect to weblogin */
#define PBC_END_SESSION_ARG_REDIR   "redirect"
#define PBC_END_SESSION_ARG_CLEAR_L "clearLogin"
#define PBC_END_SESSION_ARG_ON      "On"
#define PBC_END_SESSION_ARG_OFF     "Off"

#define PBC_END_SESSION_NOPE          0
#define PBC_END_SESSION_MASK          1
#define PBC_END_SESSION_REDIR_MASK    2
#define PBC_END_SESSION_CLEAR_L_MASK  4

#define LOGOUT_ACTION_UNSET          -1
#define LOGOUT_ACTION_NOTHING        0
#define LOGOUT_ACTION_CLEAR_L        1
#define LOGOUT_ACTION_CLEAR_L_NO_APP 2

#define PBC_SESSION_REAUTH 1
#define PBC_SUPER_DEBUG 1
#define PBC_CLEAR_COOKIE "clear"
#define PBC_SET "set"

#define EARLIEST_EVER "Fri, 11-Jan-1990 00:00:01 GMT"

/* this is the content of the redirect page's body if there is a POST */

#define PBC_POST_NO_JS_HTML1 "<HTML><HEAD></HEAD>\n \
<BODY BGCOLOR=\"white\" onLoad=\"document.query.submit.click()\">\n \
<CENTER>\n \
<FORM METHOD=\"POST\" ACTION=\""
         /* url of login page */
#define PBC_POST_NO_JS_HTML2 "\" NAME=\"query\">\n \
<INPUT TYPE=\"hidden\" NAME=\"post_stuff\" VALUE=\""
         /* packages POST stuff */
#define PBC_POST_NO_JS_HTML3 "\">\n \
<TABLE CELLPADDING=0 CELLSPACING=0 BORDER=0 WIDTH=520><TR><TD WIDTH=300 VALIGN=\"MIDDLE\"> <IMG SRC=\""
         /* UWnetID logdo url */
#define PBC_POST_NO_JS_HTML4 "\" ALT=\"UW NetID Login\" HEIGHT=\"64\" WIDTH=\"208\"> \n \
<SCRIPT LANGUAGE=\"JavaScript\">\n\
document.write(\"<P>Your browser should move to the next page in a few seconds.  If it doesn't, please click the button to continue.<P>\")\n \
</SCRIPT> <NOSCRIPT> \
<P>You do not have Javascript turned on, please click the button to continue.<P>\n </NOSCRIPT>\n</TABLE>\n \
<INPUT TYPE=\"SUBMIT\" NAME=\"submit\" VALUE=\""
	/* button text (PBC_POST_NO_JS_BUTTON) */
#define PBC_POST_NO_JS_HTML5 "\">\n </FORM>\n"
	/* copyright (PBC_HTML_COPYRIGHT) */
#define PBC_POST_NO_JS_HTML6 "</CENTER>\n </BODY></HTML>\n"

#define PBC_HTML_COPYRIGHT "<P><address>&#169; 1999 University of Washington</address><P>\n" 
#define PBC_POST_NO_JS_BUTTON "Click here to continue"
#define PBC_WEBISO_LOGO "images/login.gif"

/* 
 for the GET line to the login server
 this is used in the login script too
 */
#define PBC_GETVAR_APPSRVID "one"
#define PBC_GETVAR_APPID "two"
#define PBC_GETVAR_CREDS "three"
#define PBC_GETVAR_VERSION "four"
#define PBC_GETVAR_METHOD "five"
#define PBC_GETVAR_HOST "six"    /* host portion of url, could be host:port */
#define PBC_GETVAR_URI "seven"
#define PBC_GETVAR_ARGS "eight"
#define PBC_GETVAR_FR "fr"
/* new in dec 1999 */
#define PBC_GETVAR_REAL_HOST "hostname"  /* machine's hostname         */
#define PBC_GETVAR_APPSRV_ERR "nine"  /* let the login server know why */
#define PBC_GETVAR_FILE_UPLD "file"   /* for form multipart testing    */
#define PBC_GETVAR_FLAG "flag"        /* not currently used            */
#define PBC_GETVAR_REFERER "referer"  /* to knit together the referer  */
#define PBC_GETVAR_POST_STUFF "post_stuff"  /* post args               */
/* new in Aug 2001 */
#define PBC_GETVAR_SESSION_REAUTH "sess_re" /* session delta force reauth */
#define PBC_GETVAR_REPLY "reply"            /* tags a reply from the form */
/* new in oct 2001 */
#define PBC_GETVAR_DURATION "duration" 
/* new in March 2002 to support short term logout */
#define PBC_GETVAR_LOGOUT_ACTION "logout_action"
/* added previously but only now as defines March 2002 */
#define PBC_GETVAR_FIRST_KISS "first_kiss"
#define PBC_GETVAR_NEXT_SECURID "next_securid"
#define PBC_GETVAR_USER "user"
#define PBC_GETVAR_REALM "realm"
#define PBC_GETVAR_PASS "pass"
#define PBC_GETVAR_PASS2 "pass2"
#define PBC_GETVAR_GREQ_CREDS "creds_from_greq"
/* added May 2002 */
#define PBC_GETVAR_PINIT "pinit"

/* 
 things that are used both places (module and the library)
 */
#define PBC_SIG_LEN 128
#define PBC_CREDS_NONE    '0'
#define PBC_CREDS_DEFAULT '1'

#define PBC_COOKIE_TYPE_NONE  '0'
#define PBC_COOKIE_TYPE_G     '1'
#define PBC_COOKIE_TYPE_S     '2'
#define PBC_COOKIE_TYPE_L     '3'
#define PBC_COOKIE_TYPE_PRE_S '4'


/* macros to support older version of apache */

#ifdef APACHE1_3
#define pbc_malloc(x) ap_palloc(p, x)
#define pbc_free(x) libpbc_void(x)
#define pbc_strdup(x) ap_pstrdup(p, x)
#define pbc_strndup(s, n) ap_pstrdup(p, s, n)
#define pbc_fopen(x, y) ap_pfopen(p, x, y)
#define pbc_fclose(x) ap_pfclose(p, x)
#endif

#ifndef pbc_malloc
#define pbc_malloc(x) malloc(x)
#endif
#ifndef pbc_free
#define pbc_free(x) free(x)
#endif
#ifndef pbc_strdup
#define pbc_strdup(x) strdup(x)
#endif
#ifndef pbc_strndup
#define pbc_strndup(s, n) (char *)strncpy(calloc(n+1, sizeof(char)), s, n)
#endif
#ifndef pbc_fopen
#define pbc_fopen(x, y) fopen(x, y)
#endif
#ifndef pbc_fclose
#define pbc_fclose(x) fclose(x)
#endif

/* 
   macros to support passing extra args when compiling w/ apache
 */

/* p is the memory pool in apache */

#if defined (APACHE1_2) || defined (APACHE1_3)
#define libpbc_gen_granting_req(a,b,c,d,e,f,g,h,i,j,k) \
		libpbc_gen_granting_req_p(p, a,b,c,d,e,f,g,h,i,j,k,l)
#define libpbc_get_cookie(a,b,c,d,e,f,g,h) \
		libpbc_get_cookie_p(p, a,b,c,d,e,f,g,h)
#define libpbc_get_cookie_with_expire(a,b,c,d,e,f,g,h,i) \
		libpbc_get_cookie_with_expire_p(p, a,b,c,d,e,f,g,h,i)
#define libpbc_unbundle_cookie(a,b,c)      libpbc_unbundle_cookie_p(p, a,b,c)
#define libpbc_update_lastts(a,b,c)        libpbc_update_lastts_p(p, a,b,c)
#define libpbc_sign_init(a) 		   libpbc_sign_init_p(p, a)
#define libpbc_verify_init(a) 	   	   libpbc_verify_init_p(p, a)
#define libpbc_pubcookie_init() 	   libpbc_pubcookie_init_p(p)
#define libpbc_alloc_init(a) 		   libpbc_alloc_init_p(p, a)
#define libpbc_gethostip() 		   libpbc_gethostip_p(p)
#define libpbc_init_crypt(a) 		   libpbc_init_crypt_p(p, a)
#define libpbc_rand_malloc() 		   libpbc_rand_malloc_p(p)
#define libpbc_get_private_key(a,b) 	   libpbc_get_private_key_p(p, a,b)
#define libpbc_get_public_key(a,b) 	   libpbc_get_public_key_p(p, a,b)
#define libpbc_init_cookie_data() 	   libpbc_init_cookie_data_p(p)
#define libpbc_init_md_context_plus() 	   libpbc_init_md_context_plus_p(p)
#define libpbc_get_crypt_key(a,b) 	   libpbc_get_crypt_key_p(p, a,b)
#define libpbc_sign_cookie(a,b) 	   libpbc_sign_cookie_p(p, a,b)
#define libpbc_sign_bundle_cookie(a,b,c)   libpbc_sign_bundle_cookie_p(p, a,b,c)
#define libpbc_stringify_cookie_data(a)    libpbc_stringify_cookie_data_p(p, a)
#define libpbc_free_md_context_plus(a)     libpbc_free_md_context_plus_p(p, a)
#define libpbc_free_crypt(a)               libpbc_free_crypt_p(p, a)
#define libpbc_generate_crypt_key(a)       libpbc_generate_crypt_key_p(p, a)
#define libpbc_set_crypt_key(a,b)          libpbc_set_crypt_key_p(p,a,b)

#else

#define libpbc_gen_granting_req(a,b,c,d,e,f,g,h,i,j,k) \
		libpbc_gen_granting_req_np(a,b,c,d,e,f,g,h,i,j,k)
#define libpbc_get_cookie(a,b,c,d,e,f,g,h) \
		libpbc_get_cookie_np(a,b,c,d,e,f,g,h)
#define libpbc_get_cookie_with_expire(a,b,c,d,e,f,g,h,i) \
		libpbc_get_cookie_with_expire_np(a,b,c,d,e,f,g,h,i)
#define libpbc_unbundle_cookie(a,b,c)    libpbc_unbundle_cookie_np(a,b,c)
#define libpbc_update_lastts(a,b,c)      libpbc_update_lastts_np(a,b,c)
#define libpbc_sign_init(a) 		 libpbc_sign_init_np(a)
#define libpbc_verify_init(a) 	   	 libpbc_verify_init_np(a)
#define libpbc_pubcookie_init	 	 libpbc_pubcookie_init_np
#define libpbc_alloc_init(a) 		 libpbc_alloc_init_np(a)
#define libpbc_gethostip   		 libpbc_gethostip_np
#define libpbc_init_crypt(a) 		 libpbc_init_crypt_np(a)
#define libpbc_rand_malloc 	   	 libpbc_rand_malloc_np
#define libpbc_get_private_key(a,b) 	 libpbc_get_private_key_np(a,b)
#define libpbc_get_public_key(a,b) 	 libpbc_get_public_key_np(a,b)
#define libpbc_init_cookie_data 	 libpbc_init_cookie_data_np
#define libpbc_init_md_context_plus 	 libpbc_init_md_context_plus_np
#define libpbc_get_crypt_key(a,b) 	 libpbc_get_crypt_key_np(a,b)
#define libpbc_sign_cookie(a,b) 	 libpbc_sign_cookie_np(a,b)
#define libpbc_sign_bundle_cookie(a,b,c) libpbc_sign_bundle_cookie_np(a,b,c)
#define libpbc_stringify_cookie_data(a)  libpbc_stringify_cookie_data_np(a)
#define libpbc_free_md_context_plus(a)   libpbc_free_md_context_plus_np(a)
#define libpbc_free_crypt(a)             libpbc_free_crypt_np(a)
#define libpbc_generate_crypt_key(a)     libpbc_generate_crypt_key_np(a)
#define libpbc_set_crypt_key(a,b)        libpbc_set_crypt_key_np(a,b)

#endif 

#endif /* !PUBCOOKIE_CONFIG */
