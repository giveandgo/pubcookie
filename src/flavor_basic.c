/*
  Copyright (c) 1999-2003 University of Washington.  All rights reserved.
  For terms of use see doc/LICENSE.txt in this distribution.
 */

/** @file flavor_basic.c
 * The basic flavor of logins
 *
 *   expect a username and a password and
 *   checks against one of the defined verifiers (see 'struct verifier'
 *   and verify_*.c for possible verifiers).
 *   
 *   will pass l->realm to the verifier and append it to the username when
 *   'append_realm' is set
 *
 * $Id: flavor_basic.c,v 1.41 2003-07-02 23:27:04 willey Exp $
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
# include "pbc_path.h"
#endif

# include "apache.h"

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif /* HAVE_STDIO_H */

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
# include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif /* HAVE_ASSERT_H */

#include "snprintf.h"
#include "flavor.h"
#include "verify.h"
#include "security.h"

#include "pbc_config.h"
#include "pbc_logging.h"
#include "pbc_configure.h"
#include "libpubcookie.h"

#ifdef HAVE_DMALLOC_H
# if (!defined(APACHE) && !defined(APACHE1_3))
#  include <dmalloc.h>
# endif /* ! APACHE */
#endif /* HAVE_DMALLOC_H */

static verifier *v = NULL;
extern int debug;

/* The types of reasons for printing the login page.. 
 * Should this be in a header?  I don't think I need it outside this file.. */

#define FLB_BAD_AUTH          1
#define FLB_REAUTH            2
#define FLB_LCOOKIE_ERROR     3
#define FLB_CACHE_CREDS_WRONG 4
#define FLB_PINIT             5

/* The beginning size for the hidden fields */
#define INIT_HIDDEN_SIZE 2048
#define GETCRED_HIDDEN_MAX 512

static int init_basic()
{
    const char *vname;
    void *p;
    
    /* find the verifier configured */
    vname = libpbc_config_getstring(p, "basic_verifier", NULL);

    if (!vname) {
	pbc_log_activity(p, PBC_LOG_ERROR, 
			 "flavor_basic: no verifier configured");
	return -1;
    }

    v = get_verifier(vname);

    if (!v || !v->v) {
	pbc_log_activity(p, PBC_LOG_ERROR, 
			 "flavor_basic: verifier not found: %s", vname);
	v = NULL;
	return -1;
    }
    pbc_log_activity(p, PBC_LOG_DEBUG_LOW,
		     "init_basic: using %s verifier", vname);
    return 0;
}

/*
 * return the length of the passed file in bytes or 0 if we cant tell
 * resets the file postion to the start
 */
static long file_size(apr_pool_t *p, FILE *afile)
{
  long len;
  if (fseek(afile, 0, SEEK_END) != 0)
      return 0;
  len=ftell(afile);
  if (fseek(afile, 0, SEEK_SET) != 0)
      return 0;
  return len;
}

/* get the reason for our existing.  Returns NULL for an empty file. */

char * get_reason(apr_pool_t *p, const char * reasonpage ) {
    char * reasonfile;
    const char * reasonpath = TMPL_FNAME;
    int reasonfilelen;
    int reason_len;
    FILE *reason_file;
    char * reasonhtml;
    int readlen;

    pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE, "get_reason: hello");

    reasonfilelen = strlen(reasonpath) + strlen("/") + strlen(reasonpage) + 1;

    reasonfile = malloc( reasonfilelen * sizeof(char) );

    if ( snprintf( reasonfile, reasonfilelen, "%s%s%s",
                   reasonpath,
                   reasonpath[strlen(reasonpath) - 1 ] == '/' ? "" : "/",
                   reasonpage ) > reasonfilelen )  {
        /* Need to do something, we would have overflowed. */
        abend(p, "Reason filename overflow!\n");
    }

    reason_file = pbc_fopen(p, reasonfile, "r" );

    if (reason_file == NULL) {
        libpbc_abend(p, "Cannot open reasonfile %s", reasonfile );
    }

    reason_len = file_size(p, reason_file);

    if (reason_len == 0)
        return NULL;

    reasonhtml = malloc( (reason_len + 1) * sizeof( char ) );

    if ( reasonhtml == NULL ) {
        /* Out of memory! */
        libpbc_abend(p,  "Out of memory allocating to read reason file" );
    }

    readlen = fread( reasonhtml, 1, reason_len, reason_file );

    if (readlen != reason_len) {
        libpbc_abend(p,  "read %d when expecting %d on reason file read.",
                      readlen, reason_len );
    }

    reasonhtml[reason_len] = '\0';
    pbc_fclose(p, reason_file);
    free(reasonfile);

    pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE, "get_reason: goodbye");

    return reasonhtml;
}

/* get the html for user field, static or dynamic */
/* this really needs to be replaced by something from the template system */
char * get_user_field(apr_pool_t *p, const char * user_field_page, const char * user ){
    char *userfieldfile;
    const char *user_field_path = TMPL_FNAME;
    int userfilelen;
    int user_field_len;
    FILE * user_field_file;
    char * user_field_html;
    int readlen;
    char buf[PBC_1K];
    char * tok1;
    char * tok2;
    int user_len = ( user != NULL ? strlen(user) : 0 );

    pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE, "get_user_field: hello");

    userfilelen = strlen(user_field_path) + strlen("/") + strlen(user_field_page) + 1;

    userfieldfile = malloc( userfilelen * sizeof(char) );

    if ( snprintf( userfieldfile, userfilelen, "%s%s%s",
                   user_field_path,
                   user_field_path[strlen(user_field_path) - 1 ] == '/' ? "" : "/",
                   user_field_page ) > userfilelen )  {
        /* Need to do something, we would have overflowed. */
        abend(p, "user field filename overflow!\n");
    }

    user_field_file = pbc_fopen(p, userfieldfile, "r" );

    if (user_field_file == NULL) {
        libpbc_abend(p, "Cannot open user field file %s", userfieldfile );
    }

    user_field_len = file_size(p, user_field_file);

    if (user_field_len == 0)
        return NULL;

    if ( user_field_len >= sizeof(buf) ) {
        libpbc_abend(p,  "Need bigger buffer for reading user form field file, %D not big enough", sizeof(buf) );
    }

    user_field_html = malloc( (user_field_len + 1) * sizeof( char ) + user_len );

    if ( user_field_html == NULL ) {
        /* Out of memory! */
        libpbc_abend(p,  "Out of memory allocating to user field file" );
    }

    readlen = fread( buf, 1, user_field_len, user_field_file );

    if (readlen != user_field_len) {
        libpbc_abend(p,  "read %d when expecting %d on user field file read.",
                      readlen, user_field_len );
    }

    pbc_fclose(p, user_field_file);
    free(userfieldfile);

    buf[user_field_len] = '\0';
    strcpy(user_field_html, buf);

    /* cheesy non-generic substitution for user field */
    /* chop up the strings */
    tok2 = strstr(strstr(buf, "%user%")+1, "%");
    tok1 = strstr(user_field_html, "%user%");

    /* piece them back together */
    strcpy(tok1, (user != NULL ? user : ""));
    strcpy(tok1+user_len, tok2+1);

    pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE, "get_user_field: goodbye: %s", user_field_html);

    return user_field_html;

}

static void print_login_page(apr_pool_t *p, login_rec *l, login_rec *c, int reason)
{
    /* currently, we never clear the login cookie
       we always clear the greq cookie */
    int need_clear_login = 0;
    int need_clear_greq = 1;
    char message_out[1024];
    const char * reasonpage = NULL;

    char * hidden_fields = NULL;
    int hidden_len = 0;
    int hidden_needed_len = INIT_HIDDEN_SIZE;
    char * getcred_hidden = NULL;

    char * reason_html = NULL;
    char * user_field = NULL;
    char now[64];
    
    pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE, "print_login_page: hello");

    /* set the cookies */
    if (need_clear_login) {
        print_header(p, "Set-Cookie: %s=%s; domain=%s; path=%s; expires=%s; secure\n",
                     PBC_L_COOKIENAME, 
                     PBC_CLEAR_COOKIE,
                     PBC_LOGIN_HOST,
                     LOGIN_DIR, 
                     EARLIEST_EVER);
    }

    if (need_clear_greq) {
        print_header(p, "Set-Cookie: %s=%s; domain=%s; path=/; secure\n",
                     PBC_G_REQ_COOKIENAME, 
                     PBC_CLEAR_COOKIE,
                     PBC_ENTRPRS_DOMAIN);

    }

    switch (reason) {
        case FLB_BAD_AUTH:
            reasonpage = libpbc_config_getstring(p,  "tmpl_login_bad_auth",
                                                  "login_bad_auth" );
            break;
        case FLB_REAUTH:
            reasonpage = libpbc_config_getstring(p,  "tmpl_login_reauth",
                                                  "login_reauth" );
            break;
        case FLB_CACHE_CREDS_WRONG:
            reasonpage = libpbc_config_getstring(p,  "tmpl_login_cache_creds_wrong",
                                                  "login_cache_creds_wrong" );
            break;
        case FLB_PINIT:
            reasonpage = libpbc_config_getstring(p,  "tmpl_login_pinit",
                                                  "login_pinit" );
            break;
        case FLB_LCOOKIE_ERROR:
        default:
            reasonpage = libpbc_config_getstring(p,  "tmpl_login_nolcookie",
                                                  "login_nolcookie" );
            break;
    }

    if (reasonpage == NULL) {
        /* We shouldn't be here, but handle it anyway, of course. */
        libpbc_abend(p,  "Reasonpage is null, this is impossible." );
    }
    
    /* Get the HTML for the error reason */
    
    reason_html = get_reason(p, reasonpage);

    while (hidden_needed_len > hidden_len) {

        /* Just in case there's a bad implementation of realloc() .. */
        if (hidden_fields == NULL) {
            hidden_fields = malloc( hidden_needed_len * sizeof(char) );
        } else {
            hidden_fields = realloc( hidden_fields, hidden_needed_len * sizeof(char) );
        }

        if (hidden_fields == NULL) {
            /* Out of memory, ooops. */
            libpbc_abend(p,  "Out of memory allocating for hidden fields!" );
        }
        
        hidden_len = hidden_needed_len;

        /* Yeah, this sucks, but I don't know a better way. 
         * That doesn't mean there isn't one. */

        hidden_needed_len = snprintf( hidden_fields, hidden_len,
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%c\">\n" 
                                      "<input type=\"hidden\" name=\"%s\" value=\"%c\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%d\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%d\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%d\">\n"
                                      "<input type=\"hidden\" name=\"%s\" value=\"%d\">\n",
                                      PBC_GETVAR_APPSRVID, (l->appsrvid ? l->appsrvid : ""),
                                      PBC_GETVAR_APPID, (l->appid ? l->appid : ""),
                                      "creds_from_greq", l->creds_from_greq,
                                      PBC_GETVAR_CREDS, l->creds,
                                      PBC_GETVAR_VERSION, (l->version ? l->version : ""),
                                      PBC_GETVAR_METHOD, (l->method ? l->method : ""),
                                      PBC_GETVAR_HOST, (l->host ? l->host : ""),
                                      PBC_GETVAR_URI, (l->uri ? l->uri : ""),
                                      PBC_GETVAR_ARGS, (l->args ? l->args : ""),
                                      PBC_GETVAR_FR, (l->fr ? l->fr : ""),
                                      PBC_GETVAR_REAL_HOST, (l->real_hostname?l->real_hostname:""),
                                      PBC_GETVAR_APPSRV_ERR, (l->appsrv_err ? l->appsrv_err : ""),
                                      PBC_GETVAR_FILE_UPLD, (l->file ? l->file : ""),
                                      PBC_GETVAR_FLAG, (l->flag ? l->flag : ""),
                                      PBC_GETVAR_REFERER, (l->referer ? l->referer : ""),
                                      PBC_GETVAR_POST_STUFF, (l->post_stuff ? l->post_stuff : ""),
                                      PBC_GETVAR_SESSION_REAUTH, l->session_reauth,
                                      PBC_GETVAR_PRE_SESS_TOK, l->pre_sess_tok,
                                      "first_kiss", (l->first_kiss ? l->first_kiss : ""),
                                      PBC_GETVAR_PINIT, l->pinit,
                                      PBC_GETVAR_REPLY, FORM_REPLY
                                    );
    }

    /* xxx save add'l requests */
    {
        /* xxx sigh, i have to explicitly save this */
        char *target = get_string_arg(p, PBC_GETVAR_CRED_TARGET,
                                      NO_NEWLINES_FUNC);

        if (target) {
            int needed_len;

            getcred_hidden = malloc( GETCRED_HIDDEN_MAX * sizeof(char) );

            if (getcred_hidden == NULL) {
                /* Out of memory */
                libpbc_abend(p,  "Out of memory allocating for GetCred" );
            }

            needed_len = snprintf( getcred_hidden, GETCRED_HIDDEN_MAX, 
                                   "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
                                   PBC_GETVAR_CRED_TARGET, target );

            if ( needed_len > GETCRED_HIDDEN_MAX ) {
                /* We were going to overflow, oops. */
                libpbc_abend(p,  "Almost overflowed writing GetCred" );
            }
        } 
    }

    snprintf(now, sizeof(now), "%d", time(NULL));

    /* if it's a reauth then the user field can't be changed */
    if ( reason == FLB_REAUTH && l->user != NULL )
        user_field = get_user_field(p, 
		libpbc_config_getstring(p,  "tmpl_login_user_static",
                                                  "login_user_static" ), 
		l->user);
    else
        user_field = get_user_field(p,
		libpbc_config_getstring(p,  "tmpl_login_user_form_field",
                                                  "login_user_form_field" ), 
		l->user != NULL ? l->user : (c != NULL ? c->user : NULL));

    /* Display the login form. */
    ntmpl_print_html(p, TMPL_FNAME,
                     libpbc_config_getstring(p, "tmpl_login", "login"),
                    "loginuri", PBC_LOGIN_URI,
                    "message", reason_html != NULL ? reason_html : "",
                    "curtime", now, 
                    "hiddenfields", hidden_fields,
                    "user_field", user_field != NULL ? user_field : "",
                    "getcredhidden", getcred_hidden != NULL ? getcred_hidden : "",
                    NULL
                   );

    /* this tags the incoming request as a form reply */

    print_html(p, "\n");

    if (user_field != NULL)
        free( user_field );

    if (reason_html != NULL)
        free( reason_html );

    if (hidden_fields != NULL)
        free( hidden_fields );

    if (getcred_hidden != NULL)
        free( getcred_hidden );

    pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE, "print_login_page: goodbye");
}

/* process_basic():
   this routine is responsible for authenticating the user.
   if authentication is not possible (either the user hasn't logged in
   or the password was incorrect) it displays the login page and returns
   LOGIN_INPROGRESS.

   if authentication for this user will never succeed, it returns LOGIN_ERR.

   if authentication has succeeded, no output is generated and it returns
   LOGIN_OK.
 */
static login_result process_basic(apr_pool_t *p, login_rec *l, login_rec *c,
				  const char **errstr)
{
    struct credentials *creds = NULL;
    struct credentials **credsp = NULL;

    pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE, "process_basic: hello\n" );

    /* make sure we're initialized */
    assert(v != NULL);
    assert(l != NULL);
    /* c seems to always be null here. */
    /* XXX need to re-examine exactly what l and c should contain here */
    /* assert(c != NULL); */
    assert(errstr);

    *errstr = NULL;

    if (!v) {
        pbc_log_activity(p, PBC_LOG_ERROR,
                         "flavor_basic: flavor not correctly configured");
        return LOGIN_ERR;
    }

    /* choices, choices */

    /* index.cgi is responsible for extracting replies to the prompts
       that I printed into 'l'.  I'm responsible for modifying 'l' for
       later free rides.

       so, some possibilities:
       . reply from login page
       'l' is unauthed but has a username/pass that i should
       verify.  if yes, modify login cookie accordingly and return
       LOGIN_OK.  if no, print out the page and return
       LOGIN_INPROGRESS.

       . expired login cookie
       i should print out the page and return LOGIN_INPROGRESS.

       . valid login cookie
       i should return LOGIN_OK.
     */

    if (l->reply == FORM_REPLY) {
        if (libpbc_config_getswitch(p, "save_credentials", 0)) {
            credsp = &creds;
        }

        if (v->v(p, l->user, l->pass, NULL, l->realm, credsp, errstr) == 0) {
            if (debug) {
                /* xxx log realm */
                pbc_log_activity(p,  PBC_LOG_AUDIT,
                                  "authentication successful for %s\n", l->user );
            }

            /* authn succeeded! */

            /* xxx modify 'l' accordingly ? */

            /* optionally stick @REALM into the username */
            if (l->user && l->realm &&
                libpbc_config_getswitch(p, "append_realm", 0)) {
                /* append @REALM onto the username */
                char * tmp;
                tmp = pbc_malloc(p, strlen(l->user)+strlen(l->realm)+1);
                memset(tmp, 0, strlen(l->user)+strlen(l->realm)+1);
                if (tmp) {
                    strncat(tmp, l->user, strlen(l->user));
                    strncat(tmp, "@", 1);
                    strncat(tmp, l->realm, strlen(l->realm));
                    free (l->user);
                    l->user = tmp;
                }
            }

            /* if we got some long-term credentials, save 'em for later */
            if (creds != NULL) {
                char *outbuf;
                int outlen;
                char *out64;

                if (!libpbc_mk_priv(p, NULL, creds->str, creds->sz,
                                    &outbuf, &outlen)) {
                    /* save for later */
                    out64 = malloc(outlen * 4 / 3 + 20);
                    libpbc_base64_encode(p, (unsigned char *) outbuf,
                                          (unsigned char *) out64,
                                          outlen );

                    print_header(p, "Set-Cookie: %s=%s; domain=%s; secure\n",
                                 PBC_CRED_COOKIENAME, out64, PBC_LOGIN_HOST);

                    /* free buffer */
                    free(outbuf);
                    free(out64);
                } else {
                    pbc_log_activity(p, PBC_LOG_ERROR, 
                                     "libpbc_mk_priv failed: can't save credentials");
                }

                /* xxx save creds for later just in case we're
                   really flavor_getcred. this leaks. */
                l->flavor_extension = creds;

                creds = NULL;
            }

            pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE,
                             "process_basic: good login, goodbye\n" );

            return LOGIN_OK;
        } else {
            /* authn failed! */
            if (!*errstr) {
                *errstr = "authentication failed";
            }
            pbc_log_activity(p, PBC_LOG_AUDIT,
                             "flavor_basic: login failed for %s: %s", 
                             l->user == NULL ? "(null)" : l->user,
                             *errstr);

            /* make sure 'l' reflects that */
            l->user = NULL;	/* in case wrong username */
            print_login_page(p, l, c, FLB_BAD_AUTH);

            pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE,
                             "process_basic: login in progress, goodbye\n" );
            return LOGIN_INPROGRESS;
        }
    } else if (l->session_reauth) {
        *errstr = "reauthentication required";
        pbc_log_activity(p, PBC_LOG_AUDIT, "flavor_basic: %s: %s", l->user, *errstr);

        print_login_page(p, l, c, FLB_REAUTH);
        pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE,
                         "process_basic: login in progress, goodbye\n" );
        return LOGIN_INPROGRESS;

        /* If the pinit flag is set, show a pinit login page */
    } else if (l->pinit == PBC_TRUE) {
        pbc_log_activity(p, PBC_LOG_ERROR, "flavor_basic: pinit");

        print_login_page(p, l, c, FLB_PINIT);
        pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE,
                         "process_basic: login in progress, goodbye\n" );
        return LOGIN_INPROGRESS;

        /* l->check_error will be set whenever we couldn't decode the
           login cookie, including (for example) when the login cookie
           has expired. */
    } else if (l->check_error) {
        *errstr = l->check_error;
        pbc_log_activity(p, PBC_LOG_ERROR, "flavor_basic: %s", *errstr);

        print_login_page(p, l, c, FLB_LCOOKIE_ERROR);
        pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE,
                         "process_basic: login in progress, goodbye\n" );
        return LOGIN_INPROGRESS;

        /* if l->check_error is NULL, then 'c' must be set and must
           contain the login cookie information */
    } else if (!c) {
        pbc_log_activity(p, PBC_LOG_ERROR,
                         "flavor_basic: check_error/c invariant violated");
        abort();

        /* make sure the login cookie represents credentials for this flavor */
    } else if (c->creds != PBC_BASIC_CRED_ID) {
        *errstr = "cached credentials wrong flavor";
        pbc_log_activity(p, PBC_LOG_ERROR, "flavor_basic: %s", *errstr);

        print_login_page(p, l, c, FLB_CACHE_CREDS_WRONG);
        pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE,
                         "process_basic: login in progress, goodbye\n" );
        return LOGIN_INPROGRESS;

    } else { /* valid login cookie */
        pbc_log_activity(p, PBC_LOG_AUDIT,
                         "flavor_basic: free ride user: %s", l->user);
        pbc_log_activity(p, PBC_LOG_DEBUG_VERBOSE,
                         "process_basic: free ride, goodbye\n" );
        return LOGIN_OK;
    }
}

struct login_flavor login_flavor_basic =
{
    "basic", /* name */
    PBC_BASIC_CRED_ID, /* id; see libpbc_get_credential_id() */
    &init_basic, /* init_flavor() */
    &process_basic /* process_request() */
};
