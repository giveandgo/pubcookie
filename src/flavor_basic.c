/*

    Copyright 1999-2002, University of Washington.  All rights reserved.
    see doc/LICENSE.txt for copyright information

     ____        _                     _    _
    |  _ \ _   _| |__   ___ ___   ___ | | _(_) ___
    | |_) | | | | '_ \ / __/ _ \ / _ \| |/ / |/ _ \
    |  __/| |_| | |_) | (_| (_) | (_) |   <| |  __/
    |_|    \__,_|_.__/ \___\___/ \___/|_|\_\_|\___|

    All comments and suggestions to pubcookie@cac.washington.edu
    More information: http://www.pubcookie.org/
    Written by the Pubcookie Team

    the basic flavor of logins.  expect a username and a password and
    checks against one of the defined verifiers (see 'struct verifier'
    and verify_*.c for possible verifiers).
    
    will pass l->realm to the verifier and append it to the username when
    'append_realm' is set

 */

/*
    $Id: flavor_basic.c,v 1.22 2002-08-20 20:31:18 greenfld Exp $
 */
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
# include "pbc_path.h"
#endif

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

#include "flavor.h"
#include "verify.h"
#include "security.h"

#include "pbc_config.h"
#include "pbc_logging.h"
#include "libpubcookie.h"

static verifier *v = NULL;
extern int debug;


static int init_basic(void)
{
    const char *vname;
    
    /* find the verifier configured */
    vname = libpbc_config_getstring("basic_verifier", NULL);

    if (!vname) {
	pbc_log_activity(PBC_LOG_ERROR, 
			 "flavor_basic: no verifier configured");
	return -1;
    }

    v = get_verifier(vname);

    if (!v || !v->v) {
	pbc_log_activity(PBC_LOG_ERROR, 
			 "flavor_basic: verifier not found: %s", vname);
	v = NULL;
	return -1;
    }
    pbc_log_activity(PBC_LOG_DEBUG_LOW,
		     "init_basic: using %s verifier", vname);
    return 0;
}

static void print_login_page(login_rec *l, login_rec *c, const char **errstr)
{
    /* currently, we never clear the login cookie
       we always clear the greq cookie */
    int need_clear_login = 0;
    int need_clear_greq = 1;
    char message_out[1024];

    pbc_log_activity(PBC_LOG_DEBUG_VERBOSE, "print_login_page: hello");
    assert(errstr);

    /* set the cookies */
    if (need_clear_login) {
        print_header("Set-Cookie: %s=%s; domain=%s; path=%s; expires=%s; secure\n",
                     PBC_L_COOKIENAME, 
                     PBC_CLEAR_COOKIE,
                     PBC_LOGIN_HOST,
                     LOGIN_DIR, 
                     EARLIEST_EVER);
    }

    if (need_clear_greq) {
        print_header("Set-Cookie: %s=%s; domain=%s; path=/; secure\n",
                     PBC_G_REQ_COOKIENAME, 
                     PBC_CLEAR_COOKIE,
                     PBC_ENTRPRS_DOMAIN);

    }

    /* text before the form fields */
    snprintf(message_out, sizeof(message_out), 
	     "<p>The resource you requested requires you to authenticate."
	     "  %s</p>\n", *errstr ? *errstr : "");
    
    tmpl_print_html(TMPL_FNAME "login_part1", 
		    "", "this reason not implemented", 
		    PBC_LOGIN_URI,
		    message_out,
                    l->user ? l->user : "");

    /* keep all of the state around we need */
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n", 
		PBC_GETVAR_APPSRVID, (l->appsrvid ? l->appsrvid : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_APPID, (l->appid ? l->appid : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%c\">\n", 
                "creds_from_greq", l->creds_from_greq);
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%c\">\n", 
                PBC_GETVAR_CREDS, l->creds);
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_VERSION, (l->version ? l->version : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_METHOD, (l->method ? l->method : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_HOST, (l->host ? l->host : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_URI, (l->uri ? l->uri : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_ARGS, (l->args ? l->args : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_FR, (l->fr ? l->fr : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_REAL_HOST, (l->real_hostname?l->real_hostname:"") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_APPSRV_ERR, (l->appsrv_err ? l->appsrv_err : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_FILE_UPLD, (l->file ? l->file : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_FLAG, (l->flag ? l->flag : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_REFERER, (l->referer ? l->referer : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		PBC_GETVAR_POST_STUFF, (l->post_stuff ? l->post_stuff : "") );
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%d\">\n",
		PBC_GETVAR_SESSION_REAUTH, l->session_reauth);
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%d\">\n",
		PBC_GETVAR_PRE_SESS_TOK, l->pre_sess_tok);
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		"first_kiss", (l->first_kiss ? l->first_kiss : "") );

    /* xxx save add'l requests */
    {
	/* xxx sigh, i have to explicitly save this */
	char *target = get_string_arg(PBC_GETVAR_CRED_TARGET,
                                      NO_NEWLINES_FUNC);
	if (target) {
	    print_html("<input type=\"hidden\" name=\"%s\" value=\"%s\">\n",
		       PBC_GETVAR_CRED_TARGET, target);
	}
    }

    /* this tags the incoming request as a form reply */
    print_html("<input type=\"hidden\" name=\"%s\" value=\"%d\">\n",
		PBC_GETVAR_REPLY, FORM_REPLY);

    print_html("\n");

    /* finish off the customized login page */
    tmpl_print_html(TMPL_FNAME "login_part2", 
		    message_out,
		    "this reason not implemented");

    pbc_log_activity(PBC_LOG_DEBUG_VERBOSE, "print_login_page: goodbye");
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
static login_result process_basic(login_rec *l, login_rec *c,
				  const char **errstr)
{
    struct credentials *creds = NULL;
    struct credentials **credsp = NULL;

    /* make sure we're initialized */
    assert(v != NULL);
    assert(l != NULL);
    /* c seems to always be null here. */
    /* XXX need to re-examine exactly what l and c should contain here */
    /* assert(c != NULL); */
    assert(errstr);

    *errstr = NULL;

    if (!v) {
        pbc_log_activity(PBC_LOG_ERROR,
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
        if (libpbc_config_getswitch("save_credentials", 0)) {
            credsp = &creds;
        }

        if (v->v(l->user, l->pass, NULL, l->realm, credsp, errstr) == 0) {
            if (debug) {
                /* xxx log realm */
                pbc_log_activity( PBC_LOG_AUDIT,
                                  "authentication successful for %s\n", l->user );
            }

            /* authn succeeded! */

            /* xxx modify 'l' accordingly ? */

            /* optionally stick @REALM into the username */
            if (l->user && l->realm &&
                libpbc_config_getswitch("append_realm", 0)) {
                /* append @REALM onto the username */
                char * tmp;
                tmp = calloc(strlen(l->user)+strlen(l->realm)+1, 1);
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

                if (!libpbc_mk_priv(NULL, creds->str, creds->sz,
                                    &outbuf, &outlen)) {
                    /* save for later */
                    out64 = malloc(outlen * 4 / 3 + 20);
                    libpbc_base64_encode( (unsigned char *) outbuf,
                                          (unsigned char *) out64,
                                          outlen );

                    print_header("Set-Cookie: %s=%s; domain=%s; secure\n",
                                 PBC_CRED_COOKIENAME, out64, PBC_LOGIN_HOST);

                    /* free buffer */
                    free(outbuf);
                    free(out64);
                } else {
                    pbc_log_activity(PBC_LOG_ERROR, 
                                     "libpbc_mk_priv failed: can't save credentials");
                }

                /* xxx save creds for later just in case we're
                   really flavor_getcred. this leaks. */
                l->flavor_extension = creds;

                creds = NULL;
            }

            return LOGIN_OK;
        } else {
            /* authn failed! */
            if (!*errstr) {
                *errstr = "authentication failed";
            }
            pbc_log_activity(PBC_LOG_AUDIT,
                             "flavor_basic: login failed for %s: %s", l->user,
                             *errstr);

            /* make sure 'l' reflects that */
            l->user = NULL;	/* in case wrong username */
            print_login_page(l, c, errstr);
            return LOGIN_INPROGRESS;
        }
    } else if (l->session_reauth) {
        *errstr = "reauthentication required";
        pbc_log_activity(PBC_LOG_AUDIT, "flavor_basic: %s: %s", l->user, *errstr);

        print_login_page(l, c, errstr);
        return LOGIN_INPROGRESS;

        /* l->check_error will be set whenever we couldn't decode the
           login cookie, including (for example) when the login cookie
           has expired. */
    } else if (l->check_error) {
        *errstr = l->check_error;
        pbc_log_activity(PBC_LOG_ERROR, "flavor_basic: %s", *errstr);

        print_login_page(l, c, errstr);
        return LOGIN_INPROGRESS;

        /* if l->check_error is NULL, then 'c' must be set and must
           contain the login cookie information */
    } else if (!c) {
        pbc_log_activity(PBC_LOG_ERROR,
                         "flavor_basic: check_error/c invariant violated");
        abort();

        /* make sure the login cookie represents credentials for this flavor */
    } else if (c->creds != PBC_BASIC_CRED_ID) {
        *errstr = "cached credentials wrong flavor";
        pbc_log_activity(PBC_LOG_ERROR, "flavor_basic: %s", *errstr);

        print_login_page(l, c, errstr);
        return LOGIN_INPROGRESS;

    } else { /* valid login cookie */
        pbc_log_activity(PBC_LOG_AUDIT,
                         "flavor_basic: free ride", l->user);
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
