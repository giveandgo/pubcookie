/*

    Copyright 1999, University of Washington.  All rights reserved.

     ____        _                     _    _
    |  _ \ _   _| |__   ___ ___   ___ | | _(_) ___
    | |_) | | | | '_ \ / __/ _ \ / _ \| |/ / |/ _ \
    |  __/| |_| | |_) | (_| (_) | (_) |   <| |  __/
    |_|    \__,_|_.__/ \___\___/ \___/|_|\_\_|\___|


    All comments and suggestions to pubcookie@cac.washington.edu
    More info: https:/www.washington.edu/pubcookie/
    Written by the Pubcookie Team

    this is the pubcookie login cgi, YEAH!

    this uses a modified version of the cgic library
    functions that are cgiSomething are from that library
 */

/*
    $Id: index.cgi.c,v 1.11 2000-08-22 19:30:45 willey Exp $
 */


/* LibC */
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
/* openssl */
#include <pem.h>
/* krb5  */
#include <com_err.h>
#include <krb5.h>
/* securid */
#include "securid.h"
/* pubcookie things */
#include "pubcookie.h"
#include "libpubcookie.h"
#include "pbc_config.h"
#include "pbc_version.h"
#include "index.cgi.h"
/* cgic */
#include <cgic.h>
/* meta-auth */
#include <authsrv.h>

#ifdef MAKE_MIRROR
/* the mirror file is a mirror of what gets written out of the cgi */
/* of course it is overwritten each time this runs                 */
FILE	*mirror;
#endif 

  /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ 
 /*	general utility thingies                                            */
/* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ 

/* this returns first cookie for a given name */
int get_cookie(char *name, char *result, int max)
{
    char	*s;
    char	*p;
    char	*target;
    char	*wkspc;

    if( !(target = malloc(PBC_20K)) ) {
        abend("out of memory");
    }

    /* get all the cookies */
    if( !(s = getenv("HTTP_COOKIE")) ){
        log_message("looking for %s cookie, but found no cookies", name);
        notok(notok_no_g_or_l);
        return(FAIL);
    }

    /* make us a local copy */
    strncpy( target, s, PBC_20K-1 );

    if( !(wkspc=strstr( target, name )) ) {
        log_message("looking for %s cookie, but didn't find it", name);
        return(FAIL);
    }

    /* get rid of the <name>= part from the cookie */
    p = wkspc = wkspc + strlen(name) + 1;
    while(*p) {
        if( *p == ';' ) {
            *p = '\0';
            break;
        }
        p++;
    }

    strncpy( result, wkspc, max );
    free( target );
    return( OK );

}

char *get_string_arg(char *name, cgiFormResultType (*f)())
{
    int			length;
    char		*s;
    cgiFormResultType 	res;

    cgiFormStringSpaceNeeded(name, &length);
    s = calloc(length+1, sizeof(char));

    if( (res=f(name, s, length+1)) != cgiFormSuccess ) {
        return(NULL);
    } 
    else {
        return(s);
    }

}

int get_int_arg(char *name)
{
    int		i;

    if( cgiFormInteger(name, &i, 0) != cgiFormSuccess ) {
        return(0);
    } 
    else {
        return(i);
    }

}

char *clean_username(char *in)
{
    char	*p;
    int		word_start = 0;

    p = in;
    while(*p) {
        /* no email addresses or full principals */
        if(*p == '@')
            *p = '\0';

        /* no spaces at the beginning of the username */
        if(*p == ' ' && !word_start)
            in = p + 1;
        else
            word_start = 1;

        /* no spaces at the end */
        if(*p == ' ' && word_start) {
            *p = '\0';
            break;
        }

        p++;
    }
 
    return(in);

}

login_rec *load_login_rec(login_rec *l) 
{

#ifdef DEBUG
    fprintf(stderr, "load_login_rec: hello\n");
#endif

    /* make sure the username is a uwnetid */
    if( (l->user=get_string_arg("user", NO_NEWLINES_FUNC)) )
        l->user = clean_username(l->user);

    l->pass 		= get_string_arg("pass", NO_NEWLINES_FUNC);
    l->pass2 		= get_string_arg("pass2", NO_NEWLINES_FUNC);

    l->args 		= get_string_arg("eight", YES_NEWLINES_FUNC);
    l->uri 		= get_string_arg("seven", NO_NEWLINES_FUNC);
    l->host 		= get_string_arg("six", NO_NEWLINES_FUNC);
    l->method 		= get_string_arg("five", NO_NEWLINES_FUNC);
    l->version 		= get_string_arg("four", NO_NEWLINES_FUNC);
    l->creds      	= get_int_arg("three") + 48;
    l->appid 		= get_string_arg("two", NO_NEWLINES_FUNC);
    l->appsrvid 	= get_string_arg("one", NO_NEWLINES_FUNC);
    l->fr 		= get_string_arg("fr", NO_NEWLINES_FUNC);

    l->real_hostname 	= get_string_arg("real_hostname", NO_NEWLINES_FUNC);
    l->appsrv_err 	= get_string_arg("appsrv_err", NO_NEWLINES_FUNC);
    l->file 		= get_string_arg("file", NO_NEWLINES_FUNC);
    l->flag 		= get_string_arg("flag", NO_NEWLINES_FUNC);
    l->referer 		= get_string_arg("referer", NO_NEWLINES_FUNC);
    l->next_securid     = get_int_arg("next_securid");

#ifdef DEBUG
    fprintf(stderr, "load_login_rec: bye\n");
#endif

    return(l);

}

char *url_encode(char *in)
{
    char	*out;
    char	*p;

    if( !(out = malloc(PBC_4K)) ) {
        abend("out of memory");
    }

    strncpy(out, in, PBC_4K);

    p = out;
    while( *p ) {
        switch(*p) {
        case ' ':
            *p = '+';
            break;
        case '!':
            *p = '%'; *(++p) = '2'; *(++p) = '1';
            break;
        case '"':
            *p = '%'; *(++p) = '2'; *(++p) = '2';
            break;
        case '#':
            *p = '%'; *(++p) = '2'; *(++p) = '3';
            break;
        case '$':
            *p = '%'; *(++p) = '2'; *(++p) = '4';
            break;
        case '%':
            *p = '%'; *(++p) = '2'; *(++p) = '5';
            break;
        case '&':
            *p = '%'; *(++p) = '2'; *(++p) = '6';
            break;
        case '+':
            *p = '%'; *(++p) = '2'; *(++p) = 'B';
            break;
        case ':':
            *p = '%'; *(++p) = '3'; *(++p) = 'A';
            break;
        case ';':
            *p = '%'; *(++p) = '3'; *(++p) = 'B';
            break;
        case '=':
            *p = '%'; *(++p) = '3'; *(++p) = 'D';
            break;
        case '?':
            *p = '%'; *(++p) = '3'; *(++p) = 'F';
            break;
        }
        p++;
    }

    return(out);

}

/* write a log message via whatever mechanism                                 */
void log_message(const char *format, ...) 
{
    va_list	args;
    char	new_format[PBC_4K];
    char	message[PBC_4K];

    bzero(new_format, PBC_4K);
    bzero(message, PBC_4K);

    snprintf(new_format, sizeof(new_format)-1, "%s: %s\n", 
			ANY_LOGINSRV_MESSAGE, format);
    va_start(args, format);
    vsnprintf(message, sizeof(message)-1, new_format, args);

    va_end(args);

    libpbc_debug(message);

}

void clear_error(const char *service, const char *message) 
{

}

/* send a message to pilot                                                    */
void send_pilot_message(int grade, const char *service, int self_clearing, char *message) 
{

    /* messages sent to pilot */

}

/* logs the message and forwards it on to pilot                               */
/*                                                                            */
/*   args: grade - same grades in pilot, 1 to 5, (5 is lowest)                */
/*         service - a name to id the weblogin service (see note)             */
/*         message - varg message                                             */
/*         self-clearing - does it clear itself (1) or not (0)                */
/*                                                                            */
/*   the trick to services is that there needs to be a TRIGGER event          */
/*      and then a CLEARING event.                                            */
/*                                                                            */
/* Service trigger/clear pairs (keep a log of messages here)                  */
/*   uwnetid-err         uwnetid timeout / o.k. uwnet auth                    */
/*   securid-err         securid timeout / o.k. securid auth                  */
/*   abend               abend / not self-clearing                            */
/*   system-problem      multiple / not self-clearing                         */
/*   version             wrong major version / not self-clearing              */
/*   misc                misc is misc        / not self-clearing              */
/*   auth-kdc            auth_kdc code       / not self-clearing              */
/*   auth-securid        auth securid code   / not self-clearing              */
/*   auth-securid        auth securid code   / not self-clearing              */
/*                                                                            */
/*                                                                            */
void log_error(int grade, const char *service, int self_clearing, const char *format,...)
{
    va_list	args;
    char	new_format[PBC_4K];
    char	message[PBC_4K];

    va_start(args, format);
    snprintf(new_format, sizeof(new_format)-1, "%s: %s", SYSERR_LOGINSRV_MESSAGE, format);
    vsnprintf(message, sizeof(message)-1, new_format, args);
    log_message(message);
    send_pilot_message(grade, service, self_clearing, message);
    va_end(args);

}

/* when things go wrong and you're not sure what else to do                   */
/* a polite bailing out                                                       */
void abend(char *message) 
{

    log_error(1, "abend", 0, message);
    notok(notok_generic);
    exit(0);

}

#ifdef MAKE_MIRROR
void init_mirror_file() 
{
    mirror = fopen("/tmp/mirror", "w");

}

void close_mirror_file() 
{
    fclose(mirror);

}
#endif 

void print_out(char *format,...)
{
    va_list	args;

    va_start(args, format);
    vprintf(format, args);
#ifdef DEBUG
    vfprintf(stderr, format, args);
#endif
#ifdef MAKE_MIRROR
    vfprintf(mirror, format, args);
#endif 
    va_end(args);

}

char *get_my_hostname() 
{
    struct utsname	myname;

    if ( uname(&myname) < 0 )
        log_error(2, "system-problem", 0, "problem doing uname lookup");

    return(strdup(myname.nodename));

}

char *get_domain_hostname() 
{
    char	host[PBC_1K];

    strncpy(host, getenv ("HTTP_HOST"), strlen(host));

    if( !host )
        return ("weblogin.cac.washington.edu");

    /* if this is a test server use the test name */
    if ( !strncmp(host,"pcookiel3",9) || !strncmp(host,"weblogintest",12) )
        return ("weblogintest.cac.washington.edu");
    else
        return ("weblogin.cac.washington.edu");

}

int has_login_cookie()
{
    if( getenv("HTTP_COOKIE") && strstr(getenv("HTTP_COOKIE"), PBC_L_COOKIENAME) )
        return(1);
    else
        return(0);

}

/* we've decided not to serialize cookies, but we'll use this field           */
/* for something else in the future                                           */
/* why 23?  consult the stars                                                 */
int get_next_serial()
{
    return(23);

}

char *get_granting_request() 
{
    char	*cookie;

    if( !(cookie = malloc(PBC_4K)) ) {
        abend("out of memory");
    }

    if( !get_cookie(PBC_G_REQ_COOKIENAME, cookie, PBC_4K-1) ) {
        return(NULL);
    }

    return( cookie );

}

char *decode_granting_request(char *in)
{
    char	*out;

    out = strdup(in);    
    base64_decode(in, out);
    return(out);

}


  /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ 
 /*	main line                                                           */
/* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ 

int cgiMain() 
{
    login_rec	*l;
    char	*res;
    char	message[PBC_4K];

#ifdef DEBUG
    fprintf(stderr, "cgiMain: hello\n");
#endif
#ifdef MAKE_MIRROR
    init_mirror_file();
#endif

    /* bail if not ssl */
    if( !getenv("HTTPS") || strcmp( getenv("HTTPS"), "on" ) ) { 
        notok(notok_need_ssl);
        exit(0);
    }

    /* check to see what cookies we have */
    /* if there is an error print the error page */
    if( !cookie_test() )
        exit(0);

    /* get the arguments to this cgi, whether they are from submitting */
    /* the login page or from from the granting request cookie         */
    /* you call tell the difference since the submitted one will have  */
    /* user and pass filled in                                         */
    /* malloc and populate login_rec                                   */
    l = get_query(); 

    /* log the arrival */
    log_message("%d Visit from user: %s client addr: %s app host: %s uri: %s", l->first_kiss, l->user, l->remote_host, l->host, l->uri);

#ifdef DEBUG
    fprintf(stderr, "cgiMain: after get_query\n");
#endif

    /* check the user agent */
    if ( !check_user_agent() ) {
        log_message("%d bad agent: %s user: %s client_addr: %s",
        	l->first_kiss, 
        	cgiUserAgent, 
		l->user, 
		l->remote_host);
        notok(notok_bad_agent);
        exit(0);
    }

#ifdef DEBUG
    fprintf(stderr, "cgiMain: after user check_user_agent\n");
#endif

    /* allow for older versions that don't have froce_reauth */
    if ( !l->fr ) {
        l->fr = strdup("NFR");
    }

    /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ 
    /*                                                                   */
    /*                                                                   */
    /* four cases for the main thingie                                   */
    /*   - no prev login or creds include securid:                       */
    /*         in: no L cookie, bunch of GET data                        */
    /*               OR creds include securid info in g req              */
    /*         out: the login page (includes data from g req)            */
    /*                                                                   */
    /*   - not first time (have L cookie) but L cookie expired or invalid*/
    /*         in: expired or invalid L cookie, g req                    */
    /*         out: the login page (includes data from g req)            */
    /*                                                                   */
    /*   - not first time (have L cookie) L cookie not expired and valid */
    /*         in: valid L cookie, g req                                 */
    /*         out: L & G cookies redirect (username comes from L cookie)*/
    /*                                                                   */
    /*   - POST from login page                                          */
    /*         in: POST data that include creds                          */
    /*         process: validate creds                                   */
    /*         out: if successful L & G cookies redirect else login page */
    /*                                                                   */
    /*                                                                   */
    /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ 

    /* the main logic */
    if ( l->user ) {                           /* a reply from the login page */
#ifdef DEBUG
        fprintf(stderr, "wohooo!, an answer from the login page!\n");
#endif
        res = check_login(l);
        if( strcmp(res, CHECK_LOGIN_RET_SUCCESS) ) {
            log_message("Authentication failed: %s type: %c %s", l->user, l->creds, res);
            if( !strcmp(res, CHECK_LOGIN_RET_FAIL) ) {
                snprintf(message, sizeof(message)-1, "%s%s%s<P>%s",
                    PBC_EM1_START,
                    AUTH_FAILED_MESSAGE1,
                    PBC_EM1_END, 
                    AUTH_FAILED_MESSAGE2);
            }
            else {
                snprintf(message, sizeof(message)-1, "%s%s%s<P>",
                    PBC_EM1_START,
                    AUTH_TROUBLE,
                    PBC_EM1_END);
            }
            print_login_page(l, message, "bad auth", NO_CLEAR_LOGIN, NO_CLEAR_GREQ);
            exit(0);
        }
        log_message("Authentication success: %s type: %c", l->user, l->creds);
    }
    else if( l->creds == PBC_CREDS_UWNETID_SECURID ) {             /* securid */
        log_message("securid implies reauth by %s at %s", l->host, l->appid);
        print_login_page(l, PRINT_LOGIN_PLEASE, "securid requires reauth", YES_CLEAR_LOGIN, YES_CLEAR_GREQ);
        exit(0);
    }
    else if ( !has_login_cookie() ) {          /* no l cookie, must login */
        print_login_page(l, PRINT_LOGIN_PLEASE, "no L cookie yet", NO_CLEAR_LOGIN, YES_CLEAR_GREQ);
        exit(0);
    }
    else if ( (res=check_l_cookie(l)) ) {      /* problem w/ the l cookie*/
        log_message("%d Login cookie bad: %s", l->first_kiss, res);
        print_login_page(l, PRINT_LOGIN_PLEASE, res, YES_CLEAR_LOGIN, YES_CLEAR_GREQ);
        exit(0);
    }

    /* the reward for a hard days work                                        */
    log_message("Issuing cookies for %s at %s on %s at %s", 
 			l->user, cgiRemoteAddr, l->host, l->appid);

    /* generate the cookies and print the redirect page                       */
    print_redirect_page(l);

#ifdef MAKE_MIRROR
    close_mirror_file();
#endif

    return(0);  
}


void print_form_field(char *field, char *var) {
    char	*field_type;

    if( !strcmp(field, PROMPT_UWNETID) || !strcmp(field, PROMPT_SECURID) )
        field_type = strdup("text");
    else
        field_type = strdup("password");

    print_out("<P>\n");
    print_out("%s\n", field);
    print_out("<input type=\"%s\" ", field_type);
    print_out("name=\"%s\" SIZE=\"20\">\n", var);

}


void print_login_page(login_rec *l, char *message, char *reason, int need_clear_login, int need_clear_greq)
{
    char	*log_in_with = NULL;
    char	*field1 = NULL;
    char	*field2 = NULL;
    char	*field3 = NULL;
    char	*hostname = strdup(get_domain_hostname());

    switch (l->creds) {
    case '1':
        field1 = strdup(PROMPT_UWNETID);
        field2 = strdup(PROMPT_PASSWD);
        log_in_with = strdup("UW NetID and password");
        break;
    case '2':
        field1 = NULL;
        break;
    case '3':
        field1 = strdup(PROMPT_UWNETID);
        field2 = strdup(PROMPT_PASSWD);
        field3 = strdup(PROMPT_SECURID);
        log_in_with = strdup("UW NetID, password, and SecurID");
        break;
    default:
        field1 = NULL;
        break;
    }

    print_out("Content-Type: text/html\n");
    if( need_clear_login ) 
        print_out("Set-Cookie: %s=%s; domain=%s; path=%s; expires=%s; secure\n",
            PBC_L_COOKIENAME, 
            hostname, 
            LOGIN_DIR, 
            EARLIEST_EVER);
    if( need_clear_greq ) 
        print_out("Set-Cookie: %s=%s; domain=%s; path=/; secure\n",
            PBC_G_REQ_COOKIENAME, 
            G_REQ_RECIEVED,
            PBC_ENTRPRS_DOMAIN);
    print_out("\n");

    print_login_page_part1(YES_FOCUS);

    print_login_page_lhs1(message, reason, log_in_with);

    if( field1 ) print_form_field( field1, "user" );
    if( field2 ) print_form_field( field2, "pass" );
    if( field3 ) print_form_field( field3, "pass2" );

    print_login_page_lhs2(l);

    print_login_page_centre();

    print_login_page_rhs();

    print_login_page_expire_info();

    print_login_page_bottom();


}

char *check_login_uwnetid(const char *user, const char *pass)
{
    char	*res;

#ifdef DEBUG
    fprintf(stderr, "check_login_uwnetid: hello\n");
#endif 

    if( auth_kdc(user, pass) == NULL ) {
#ifdef DEBUG
        fprintf(stderr, "check_login_uwnetid: auth_kdc say ok\n");
#endif 
        clear_error("uwnetid-fail", "uwnetid auth ok");
        return(CHECK_LOGIN_RET_SUCCESS);
    }
    else {
        /* now check the NDC passwd file */
        if( (res=auth_ndcpasswd(user, pass)) == NULL ) {
#ifdef DEBUG
            fprintf(stderr, "check_login_uwnetid: auth_ndcpasswd say ok\n");
#endif
            clear_error("uwnetid-fail", "uwnetid auth ok");
            return(CHECK_LOGIN_RET_SUCCESS);
        }
        else {
            return(CHECK_LOGIN_RET_FAIL);
        }
    }

}

char *check_login_securid(char *user, char *sid, int next, login_rec *l)
{
    if( auth_securid(user, sid, next, l) == NULL ) {
        clear_error("securid-fail", "securid auth ok");
        return(CHECK_LOGIN_RET_SUCCESS);
    }
    else {
        log_error(2, "securid-err", 1, "problem doing securid auth");
        return(CHECK_LOGIN_RET_FAIL);
    }

}

/* successful auth returns CHECK_LOGIN_RET_SUCCESS                            */
char *check_login(login_rec *l)
{
    char	*ret;

#ifdef DEBUG
    fprintf(stderr, "in check_login\n");
#endif

    if( !(ret = malloc(100)) ) {
        abend("out of memory");
    }

    strcpy(ret, CHECK_LOGIN_RET_BAD_CREDS);

    if( l->creds == PBC_CREDS_UWNETID ) {
        strcpy(ret, check_login_uwnetid(l->user, l->pass));
    }
    else if( l->creds == PBC_CREDS_UWNETID_SECURID ) {
        strcpy(ret, check_login_securid(l->user, l->pass2, l->next_securid, l));
        if( !strcmp(ret, CHECK_LOGIN_RET_SUCCESS) ) {
            strcpy(ret, check_login_uwnetid(l->user, l->pass));
        }
        else {
            return ret;
        }
    }

    return(ret);

}


/* returns NULL if o.k.                                                       */
/*   else a description of the failure                                        */
char *check_l_cookie(login_rec *l)
{
    char	*cookie;
    login_rec	*lc;
    time_t	t;
    char	*g_version;
    char	*l_version;

#ifdef DEBUG
    fprintf(stderr, "check_l_cookie: hello\n");
#endif

    if( !(cookie = malloc(PBC_4K)) ) {
        abend("out of memory");
    }

    /* get the login request cookie */
    if( !get_cookie(PBC_L_COOKIENAME, cookie, PBC_4K-1) ) {
        abend("no login cookies");
        return 0;
    }

    /* $verify_pgm takes arguments on the command line         */
    /* the arguments are <cookie type> <crypt key> <cert file> */
    /* and the cookie on stdin, it returns the information     */
    /* from teh cookie on stdout                               */

    lc = verify_login_cookie(cookie, l);

    if( !lc ) {
        log_message("couldn't deal with cookie %s", cookie);
        return("couldn't decode login cookie");
    }

#ifdef DEBUG
    fprintf(stderr, "in check_l_cookie ready to look at cookie contents %s\n", lc->user);
#endif

    /* look at what we got back from the cookie */
    if( ! lc->user ) {
        log_error(5, "system-problem", 0, "no user from L cookie? user from g_req: %s", l->user);
        return "malformed";
    }

    if( (lc->create_ts + EXPIRE_LOGIN) < (t=time(NULL)) ) {
        log_message("%d expired login cookie; created: %d timeout: %dsecs now: %d",
			l->first_kiss,
			lc->create_ts, 
                        EXPIRE_LOGIN, 
                        t);
        return "expired";
    }

#ifdef DEBUG
    fprintf(stderr, "in check_l_cookie ready to look at cookie creds %c\n", lc->creds);
#endif

    if( lc->creds != l->creds ) {
        if( l->creds == PBC_CREDS_UWNETID ) {
            if( lc->creds != PBC_CREDS_UWNETID_SECURID ) {
                log_message("wrong_creds: from login cookie: %s from request: %s", lc->creds, l->creds);
                return("wrong_creds");
            }
        }
        else {
            log_message("wrong_creds: from login cookie: %s from request: %s", lc->creds, l->creds);
            return("wrong_creds");
        }
    }

    l_version = lc->version; g_version = l->version;
    if( *l_version != *g_version ) {
        log_error(5, "version", 0, "wrong major version: from L cookie %s, from g_req %s for host %s", l_version, g_version, l->host);
        return("wrong major version");
    }
    if( *(l_version+1) != *(g_version+1) ) {
        log_message("warn: wrong minor version: from l cookie %s, from g_req %s for host %s", l_version, g_version, l->host);
    }

    l->user = lc->user;
    l->creds = lc->creds;
    free(cookie);
    return((char *)NULL);
}


  /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ 
 /*	functions                                                           */
/* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ /* */ 

void print_j_test() 
{

    print_out("%s", J_TEST_TEXT1);
    print_out("%s", J_TEST_TEXT2);
    print_out("%s", J_TEST_TEXT3);
    print_out("%s", J_TEST_TEXT4);
    print_out("%s", J_TEST_TEXT5);

}

void notok_no_g_or_l() 
{
    print_j_test();

    print_out("<NOSCRIPT>\n");

    print_out("%s\n", NOTOK_NO_G_OR_L_TEXT1);

    print_out("</NOSCRIPT>\n");

}

void notok_no_g() 
{
    print_out("%s", NOTOK_NO_G_TEXT1);

}

void notok_formmultipart() 
{
    print_out("%s", NOTOK_FORMMULTIPART_TEXT1);

}

void notok_need_ssl() 
{
    print_out("%s", NOTOK_NEEDSSL_TEXT1);
    log_message("host %s came in on a non-ssl port, why?", cgiRemoteAddr);
}

void notok_bad_agent() 
{
    print_out("%s", NOTOK_BAD_AGENT_TEXT1);
    print_out("The browser you are using identifies itself as:<P><TT></TT>",
                 cgiUserAgent);
    print_out("%s", NOTOK_BAD_AGENT_TEXT2);

}

void notok_generic() 
{
    print_out("%s", NOTOK_GENERIC_TEXT1);

}

void notok ( void (*notok_f)() )
{
    print_out("Content-Type: text/html\n");

    /* if we got a form multipart cookie, reset it */
    if ( getenv("HTTP_COOKIE") && strstr(getenv("HTTP_COOKIE"), PBC_FORM_MP_COOKIENAME) ) {
        print_out("Set-Cookie: %s=done; domain=%s; path=/; expires=%s\n", 
            PBC_FORM_MP_COOKIENAME, 
            PBC_ENTRPRS_DOMAIN, 
            EARLIEST_EVER);
    }

    /* newline that ends the header info */
    print_out("\n");

    print_login_page_part1(NO_FOCUS);
    print_out("<td valign=\"middle\">\n");
    print_uwnetid_logo();

    notok_f();

    print_out("</td>\n</tr>\n");
    print_login_page_bottom();

}


int cookie_test() 
{
    char        *cookies;

    /* get the cookies */
    if ( !(cookies = getenv("HTTP_COOKIE")) ){
        notok(notok_no_g_or_l);
        return(0);
    }
    
    /* we don't currently handle form-multipart */
    /* the formmultipart cookie is set by the module */
    if ( strstr(cookies, PBC_FORM_MP_COOKIENAME) ) {
        notok(notok_formmultipart);
        return(0);
    }

    if ( !strstr(cookies, PBC_G_REQ_COOKIENAME) ) {

        if ( !strstr(cookies, PBC_L_COOKIENAME) ) {
            log_message("no granting req or login cookie from %s", getenv("REMOTE_ADDR"));
            notok(notok_no_g_or_l);
            return(0);
        }
        else {
            log_message("no granting req, connection from %s", getenv("REMOTE_ADDR"));
            notok(notok_no_g);
            return(0);
        }
    }
    
    free(cookies);
    return(1);
}

/*	################################### The beginning of the table        */
void print_table_start()
{
    print_out("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\" width=\"580\">\n");

}

/*	################################### da copyright, it's ours!          */
void print_copyright()
{
    print_out("<address>&#169; 2000 University of Washington</address>\n");

}

/*	################################### UWNetID Logo                      */
void print_uwnetid_logo()
{
    print_out("<img src=\"/images/login.gif\" alt=\"\" height=\"64\" width=\"208\">\n");

}

/*       ################################### part 1                           */
void print_login_page_part1(int focus)
{
    print_out("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
    print_out("<html>\n");
    print_out("<head>\n");
    print_out("<title>UW NetID Login</title>\n");
    print_out("</head>\n");

    if( focus ) {
        print_out("<body bgcolor=\"#FFFFFF\" onLoad=\"document.query.user.focus()\">\n");
    }
    else {
        print_out("<body bgcolor=\"#FFFFFF\">\n");
    }

    print_out("<center>\n");

    print_table_start();
    
    print_out("<tr>\n");
}

/*	################################### left hand side of big table       */
void print_login_page_lhs1(char *message, char *reason, char *log_in_with)
{
    print_out("<td width=\"310\" valign=\"middle\">\n");

    print_uwnetid_logo();

    /* any additional messages and hints from the cgi */
    if( reason != NULL ) 
        print_out("<!-- %s -->\n\n", reason);

    /* open the form */
    print_out("\n<form method=\"POST\" action=\"/\" enctype=\"application/x-www-form-urlencoded\" name=\"query\">\n");

    /* text before the for fields */
    if( message != NULL && strcmp(message, PRINT_LOGIN_PLEASE) ) {
        print_out("%s", message);
    }
    else {
        print_out("<P>The resource you requested requires you to log in ");
        print_out(" with your %s.</P>\n", log_in_with);
    }

}

/*	################################### more, left hand side of big table */
void print_login_page_lhs2(login_rec *l)
{
    print_out("<p><strong><input type=\"SUBMIT\" name=\"submit\" value=\"Login\">");
    print_out("</strong>\n");
    print_login_page_hidden_stuff(l);
    print_out("</form>\n");
    print_out("</td>\n");
    print_out("<td width=\"9\">&nbsp;</td>\n");

}

/*	################################### centre of the page                */
void print_login_page_centre()
{
    print_out("<td width=\"2\" bgcolor=\"#000000\">\n");
    print_out("<img src=\"/images/1pixffcc33iystpiwfy.gif\" width=\"1\" height=\"1\" align=\"BOTTOM\" alt=\"\"></td>\n");
    print_out("<td width=\"9\">&nbsp;</td>\n");

}

/*	################################### right hand side                   */
void print_login_page_rhs()
{

    print_out("<td width=\"250\" valign=\"MIDDLE\">\n");
    print_out("<dl>\n");
    print_out("<dt>Need a UW NetID?</dt>\n");
    print_out("<dd><a href=\"https://accounts.washington.edu/new/new\">Students</a></dd>\n");
    print_out("<dd><a href=\"https://accounts.washington.edu/new/new?type=staff\">Faculty or staff</a></dd>\n");
    print_out("</dl>\n");
    print_out("<dl>\n");
    print_out("<dt>Forget your password?</dt>\n");
    print_out("<dd><a href=\"https://accounts.washington.edu/renew/renew\">Students</a></dd>\n");
    print_out("<dd><a href=\"http://www.washington.edu/computing/uwnetid/password/forget.html#FAC\">Faculty or staff</a></dd>\n");
    print_out("</dl>\n");
    print_out("<dl>\n");
    print_out("<dt>Have a question?</dt>\n");
    print_out("<dd><a href=\"mailto:help@cac.washington.edu\">help@cac.washington.edu</a></dd>\n");
    print_out("</dl>\n");
    print_out("<dl>\n");
    print_out("<dt>Want to know more?</dt>\n");
    print_out("<dd><a href=\"http://www.washington.edu/computing/uwnetid/\">About UW NetIDs</a></dd>\n");
    print_out("</dl>\n");
    print_out("</td>\n");

}

/*	################################### hidden stuff                      */
void print_login_page_hidden_stuff(login_rec *l)
{

    print_out("\n");
    print_out("<input type=\"hidden\" name=\"one\" value=\"%s\">\n", 
		(l->appsrvid ? l->appsrvid : "") );
    print_out("<input type=\"hidden\" name=\"two\" value=\"%s\">\n",
		(l->appid ? l->appid : "") );
    print_out("<input type=\"hidden\" name=\"three\" value=\"%c\">\n", l->creds);
    print_out("<input type=\"hidden\" name=\"four\" value=\"%s\">\n",
		(l->version ? l->version : "") );
    print_out("<input type=\"hidden\" name=\"five\" value=\"%s\">\n",
		(l->method ? l->method : "") );
    print_out("<input type=\"hidden\" name=\"six\" value=\"%s\">\n",
		(l->host ? l->host : "") );
    print_out("<input type=\"hidden\" name=\"seven\" value=\"%s\">\n",
		(l->uri ? l->uri : "") );
    print_out("<input type=\"hidden\" name=\"eight\" value=\"%s\">\n",
		(l->args ? l->args : "") );
    print_out("<input type=\"hidden\" name=\"fr\" value=\"%s\">\n",
		(l->fr ? l->fr : "") );

    print_out("<input type=\"hidden\" name=\"real_hostname\" value=\"%s\">\n",
		(l->real_hostname ? l->real_hostname : "") );
    print_out("<input type=\"hidden\" name=\"appsrv_err\" value=\"%s\">\n",
		(l->appsrv_err ? l->appsrv_err : "") );
    print_out("<input type=\"hidden\" name=\"file\" value=\"%s\">\n",
		(l->file ? l->file : "") );
    print_out("<input type=\"hidden\" name=\"flag\" value=\"%s\">\n",
		(l->flag ? l->flag : "") );
    print_out("<input type=\"hidden\" name=\"next_securid\" value=\"%d\">\n",
		(l->next_securid ? l->next_securid : 0) );
    print_out("<input type=\"hidden\" name=\"referer\" value=\"%s\">\n",
		(l->referer ? l->referer : "") );

    print_out("<input type=\"hidden\" name=\"post_stuff\" value=\"%s\">\n",
		(l->post_stuff ? l->post_stuff : "") );

}

/*	################################### part 5                            */
void print_login_page_bottom() 
{

    print_out("<tr>\n");
    print_out("<td colspan=\"5\" align=\"center\">\n");
    print_copyright();
    print_out("</td>\n");
    print_out("</tr>\n");
    print_out("</table>\n");
    print_out("</center>\n");
    print_out("</body>\n");
    print_out("</html>\n");

}

/*	################################### part expire_info                  */
void print_login_page_expire_info()
{

    print_out("</tr>\n");
    print_out("\n");
    print_out("<tr>\n");
    print_out("<td colspan=\"5\" align=\"center\">\n");
    print_out("<p>Login gives you 8-hour access without repeat login to UW NetID-protected Web resources.</p>\n");
    print_out("<p><strong>WARNING</strong>: Protect your privacy! Prevent unauthorized use! Close all Web browser windows and Web-enabled applications when you are finished.</p>\n");
    print_out("</td>\n");
    print_out("</tr>\n");

}

char *to_lower(char *in)
{
    char	*p;

    for(p = in; *p; p++)
        *p = tolower(*p);

    return(in);

}

void clean_ok_browsers_line(char *in)
{
    char	*p;

    for(p = in; *p; p++) {
        *p = tolower(*p);
        if( *p == '\n' ) 
            *p-- = '\0';
    }

}

int check_user_agent()
{
    char        line[PBC_1K];
    char        agent_clean[PBC_1K];
    FILE	*ifp;

    if ( !(ifp = fopen(OK_BROWSERS_FILE, "r")) ) {
        log_error(2, "system-problem", 0, "can't open ok browsers file: %s, continuing", OK_BROWSERS_FILE);
        return(0);
    }

    /* make the user agent lower case */
    strncpy( agent_clean, cgiUserAgent, sizeof(agent_clean) );
    clean_ok_browsers_line(agent_clean);

    while( fgets(line, sizeof(line), ifp) != NULL ) {
        clean_ok_browsers_line(line);
        if( line[0] == '#' )
            continue;
        if( strstr( agent_clean, line ) ) {
            return(1);
        }
    }

    return(0);

}


void print_redirect_page(login_rec *l)
{
    int			serial = 0;
    char		*g_cookie;
    char		*l_cookie;
    char		*redirect_uri;
    char		*message;
    char		*args_enc = NULL; 
    char		*redirect_dest_tmp = NULL;
    char		*redirect_dest = NULL;
    char		g_set_cookie[PBC_1K];
    char		l_set_cookie[PBC_1K];
    char		clear_g_req_cookie[PBC_1K];
    char		*post_stuff_lower = NULL;
    char		*p = NULL;
    int			g_res, l_res;
    int			limitations_mentioned = 0;
    char		*submit_value = NULL;
    cgiFormEntry	*c;
    cgiFormEntry	*n;

    if( !(redirect_dest_tmp = malloc(PBC_4K)) ) {
        abend("out of memory");
    }
    if( !(redirect_dest = malloc(PBC_4K)) ) {
        abend("out of memory");
    }
    if( !(message = malloc(PBC_4K)) ) {
        abend("out of memory");
    }
    if( !(g_cookie = malloc(PBC_4K)) ) {
        abend("out of memory");
    }
    if( !(l_cookie = malloc(PBC_4K)) ) {
        abend("out of memory");
    }
    serial = get_next_serial();

    /* cook up them cookies */
    l_res = create_cookie(url_encode(l->user),
                          url_encode(l->appsrvid),
                          url_encode(l->appid),
                          PBC_COOKIE_TYPE_L,
                          l->creds,
                          serial,
                          l_cookie,
                          PBC_4K);
    g_res = create_cookie(url_encode(l->user),
                          url_encode(l->appsrvid),
                          url_encode(l->appid),
                          PBC_COOKIE_TYPE_G,
                          l->creds,
                          serial,
                          g_cookie,
                          PBC_4K);

    /* if we have a problem then bail with a nice message */
    if ( !l_res || !g_res ) {
          sprintf( message, "%s%s%s%s%s%s",
		PBC_EM1_START,
		TROUBLE_CREATING_COOKIE,
		PBC_EM1_END,
      		PBC_EM2_START,
		PROBLEMS_PERSIST,
         	PBC_EM2_END);
          print_login_page(l, message, "cookie create failed", NO_CLEAR_LOGIN, NO_CLEAR_GREQ);
          log_error(1, "system-problem", 0, "Not able to create cookie for user %s at %s-%s", l->user, l->appsrvid, l->appid);
          free(message);
          return;
    }

    /* create the http header line with the cookie */
    snprintf( g_set_cookie, sizeof(g_set_cookie)-1, 
		"Set-Cookie: %s=%s; domain=%s; path=/; secure", 
		PBC_G_COOKIENAME,
                g_cookie,
                PBC_ENTRPRS_DOMAIN);
    snprintf( l_set_cookie, sizeof(l_set_cookie)-1, 
		"Set-Cookie: %s=%s; domain=%s; path=%s; secure", 
		PBC_L_COOKIENAME,
                l_cookie,
                get_domain_hostname(),
                LOGIN_DIR);
    snprintf( clear_g_req_cookie, sizeof(l_set_cookie)-1, 
		"Set-Cookie: %s=clear; domain=%s; path=/; secure", 
		PBC_G_REQ_COOKIENAME,
                PBC_ENTRPRS_DOMAIN);

    /* whip up the url to send the browser back to */
    if( !strcmp(l->fr, "NFR") )
        redirect_uri = l->uri;
    else
        redirect_uri = l->fr;
    snprintf(redirect_dest, PBC_4K-1, "https://%s%s%s", 
		l->host, (*redirect_uri == '/' ? "" : "/"), redirect_uri);

    if( l->args ) {
        args_enc = strdup(l->args);    
	base64_decode(l->args, args_enc);
        snprintf( redirect_dest, PBC_4K-1, "%s?%s", redirect_dest, args_enc );
    } 

    /* we don't use the fab log_message funct here because the url encoding */
    /* will look like format chars in future *printf's */
    fprintf(stderr, "about to do redirect of %s for host %s, redirect is: %s\n",
				l->user, l->host, redirect_dest);

    /* now blat out the redirect page */
    print_out("%s\n", g_set_cookie);
    print_out("%s\n", l_set_cookie);
    print_out("%s\n", clear_g_req_cookie);

    /* incase we have a post */
    if ( l->post_stuff ) {
        /* cgiParseFormInput will extract the arguments from the post */
        /* make them available to subsequent cgic calls */
        if( cgiParseFormInput(l->post_stuff, strlen(l->post_stuff))
                   != cgiParseSuccess ) {
            log_error(5, "misc", 0, "couldn't parse the decoded granting request cookie");
            notok(notok_generic);
            exit(0);
        }

        print_out("Pragma: No-Cache\n");
        print_out("Content-Type: text/html\n\n\n");
	print_out("<HTML>");
	/* when the page loads click on the last element */
        /* (which will always be the submit) in the array */
        /* of elements in the first, and only, form. */
	print_out("<BODY BGCOLOR=\"white\" onLoad=\"");

        /* depending on whether-or-not there is a SUBMIT field in the form */
        /* use the correct javascript to autosubmit the POST */
        post_stuff_lower = strdup(l->post_stuff);
        for(p=post_stuff_lower; *p != '\0'; p++)
            *p = tolower(*p);
        if( strstr(post_stuff_lower, "submit") != NULL )
            print_out("document.query.submit.click()");
        else
            print_out("document.query.submit");

        print_out("\">\n");

	print_out("<center>");
        print_table_start();
	print_out("<tr><td align=\"LEFT\">\n");

	print_out("<form method=\"POST\" action=\"%s\" ", redirect_dest);
        print_out("enctype=\"application/x-www-form-urlencoded\" ");
        print_out("name=\"query\">\n");

        c = cgiFormEntryFirst;
        while (c) {
            // in the perl version we had to make sure we were getting
            // rid of this header line
            //        c->attr =~ s%^\s*HTTP/1.1 100 Continue\s*%%mi;

            /* if there is a " in the value string we have to put */
            /* in a TEXTAREA object that will be visible          */
            if( strstr(c->value, "\"") ) {
                if( ! limitations_mentioned ) {
                    print_out("Certain limitations require that this be shown, please ignore it<BR>\n");
                    limitations_mentioned++;
                }
                print_out("<textarea cols=\"0\" rows=\"0\" name=\"%s\">\n", c->attr);
                print_out("%s</textarea>", c->value);
                print_out("<P>\n");
            }
            else {
                /* we don't want to cover other people's submits */
                if ( !strcmp(c->attr, "submit") )  {
                    submit_value = c->value;
                }
                else {
                    print_out("<input type=\"hidden\" ");
		    print_out("name=\"%s\" value='%s'>\n", c->attr, c->value);
                }
    	    }

            /* move onto the next attr/value pair */
            n = c->next;
            c = n;
        } /* while c */


        print_out("</td></tr>\n");
        print_uwnetid_logo();
        print_out("<P>");
        print_out("%s\n", PBC_POST_NO_JS_TEXT);
        print_out("</td></tr></table>\n");

        /* put submit at the bottom so it looks better and */
        if( submit_value )
            print_out("<input type=\"submit\" name=\"submit\" value=\'%s\'>\n", submit_value);
        else
            print_out("<input type=\"submit\" value=\"%s\">\n", PBC_POST_NO_JS_BUTTON);

        print_out("</form>\n");
        print_copyright();
        print_out("</center>");
        print_out("</BODY></HTML>\n");
    }
    else {
        /*                                                               */
        /* non-post redirect area                 non-post redirect area */
        /*                                                               */
        print_out("Content-Type: text/html\n\n\n");
        print_out("<html><head>\n");
        print_out("<meta http-equiv=\"Refresh\" content=\"%s;URL=%s\">\n", REFRESH, redirect_dest);
        print_out("<BODY BGCOLOR=\"white\">");
        print_out("<!--redirecting to %s-->", redirect_dest);
        print_out("</BODY></HTML>\n");
    } /* end if post_stuff */

    free(g_cookie);
    free(l_cookie);
    free(message);
    free(redirect_dest);

}

login_rec *get_query() 
{
    login_rec		*l = malloc(sizeof(login_rec));
    char		*g_req;
    char		*g_req_clear;

    /* even if we hav a granting request post stuff will be in the request */
    l->post_stuff	= get_string_arg("post_stuff", YES_NEWLINES_FUNC);

    /* take everything out of the environment */
    l = load_login_rec(l);

    /* cgiParseFormInput will extract the arguments from the granting         */
    /* cookie string and make them available to subsequent cgic calls         */

    /* if there is a user field there it is a submit from a login */
    if( !l->user ) {
        if( !(g_req = get_granting_request()) ) {
            log_message("No granting request cookie.  remote addr %s", getenv("REMOTE_ADDR"));
            notok(notok_no_g_or_l);
            return(NULL);
        }
        g_req_clear = decode_granting_request(g_req);

        if( cgiParseFormInput(g_req_clear, strlen(g_req_clear)) 
                   != cgiParseSuccess ) {
            log_error(5, "misc", 0, "couldn't parse the decoded granting request cookie");
            notok(notok_generic);
            return(NULL);
        }
        l = load_login_rec(l);
        free( g_req );
        free( g_req_clear );
    }

    /* we should always have apphost, cry if we don't */
    if( !(l->appid) ) {
        abend("submit from login page problem or granting request mangled");
    }

    /* because it's convenient we add some info that will follow the req */
    l->first_kiss = time(NULL);
    l->remote_host = strdup( getenv("REMOTE_ADDR") );

#ifdef DEBUG 
    fprintf(stderr, "from login user: %s\n", l->user);
    fprintf(stderr, "from login version: %s\n", l->version);
    fprintf(stderr, "from login creds: %c\n", l->creds);
    fprintf(stderr, "from login appid: %s\n", l->appid);
    fprintf(stderr, "from login host: %s\n", l->host);
    fprintf(stderr, "from login appsrvid: %s\n", l->appsrvid);
    fprintf(stderr, "from login next_securid: %d\n", l->next_securid);
    fprintf(stderr, "from login post_stuff: %s\n", l->post_stuff);
#endif

    return(l);

}

login_rec *verify_login_cookie (char *cookie, login_rec *l)
{
    md_context_plus     *ctx_plus;
    crypt_stuff         *c_stuff;
    pbc_cookie_data     *cookie_data;
    char		crypt_keyfile[PBC_4K];
    char		sign_keyfile[PBC_4K];
    login_rec		*new;

    new = malloc(sizeof(new));

    snprintf(crypt_keyfile, sizeof(crypt_keyfile)-1, "%s%s.%s", 
			KEY_DIR, CRYPT_KEY_FILE, get_my_hostname()); 
    c_stuff = libpbc_init_crypt(crypt_keyfile);

    snprintf(sign_keyfile, sizeof(sign_keyfile)-1, "%s%s", 
			KEY_DIR, CERT_FILE); 
    ctx_plus = libpbc_verify_init(sign_keyfile);

    if( ! (cookie_data = libpbc_unbundle_cookie(cookie, ctx_plus, c_stuff)) )
        return((login_rec *)NULL);

    new->user = (*cookie_data).broken.user;
    new->version = (*cookie_data).broken.version;
    new->type = (*cookie_data).broken.type;
    new->creds = (*cookie_data).broken.creds;
    new->serial = (*cookie_data).broken.serial;
    new->appsrvid = (*cookie_data).broken.appsrv_id;
    new->appid = (*cookie_data).broken.app_id;
    new->create_ts = (*cookie_data).broken.create_ts;
    new->last_ts = (*cookie_data).broken.last_ts;

    return(new);

}

int create_cookie(char *user_buf,
                  char *appsrv_id_buf,
                  char *app_id_buf,
                  char type,
                  char creds,
                  int serial,
                  char *cookie,
 	          int max)
{
    /* special data structs for the crypt stuff */
    md_context_plus 	*ctx_plus;
    crypt_stuff         *c_stuff;
    unsigned char	crypt_keyfile[PBC_1K];
    unsigned char	cert_keyfile[PBC_1K];

    /* measured quantities */
    unsigned char 	user[PBC_USER_LEN];
    unsigned char 	appsrv_id[PBC_APPSRV_ID_LEN];
    unsigned char 	app_id[PBC_APP_ID_LEN];

    /* local junk */
    char		*cookie_local;

    /* right size the args */
    strncpy(user, user_buf, sizeof(user));
    user[sizeof(user)-1] = '\0';
    strncpy(appsrv_id, appsrv_id_buf, sizeof(appsrv_id));
    appsrv_id[sizeof(appsrv_id)-1] = '\0';
    strncpy(app_id, app_id_buf, sizeof(app_id));
    appsrv_id[sizeof(app_id)-1] = '\0';

    /* load up the encryption key stuff */
    snprintf(crypt_keyfile, sizeof(crypt_keyfile)-1, "%s%s.%s", 
			KEY_DIR, CRYPT_KEY_FILE, get_my_hostname()); 
    c_stuff = libpbc_init_crypt(crypt_keyfile);

    /* load up the certificate context */
    snprintf(cert_keyfile, sizeof(cert_keyfile)-1, "%s%s", 
			KEY_DIR, CERT_KEY_FILE); 
    ctx_plus = libpbc_sign_init(cert_keyfile);

    /* go get the cookie */
    cookie_local = libpbc_get_cookie(user, type, creds, serial, appsrv_id, app_id, ctx_plus, c_stuff);

    strncpy( cookie, cookie_local, max );
    return(OK);

}

