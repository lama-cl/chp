/*
 * htpasswd.c: simple program for manipulating password file for NCSA httpd
 * 
 * Rob McCool
 */

/* Modified 29aug97 by Jef Poskanzer to accept new password on stdin,
** if stdin is a pipe or file.  This is necessary for use from CGI.
*/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "checkedc_utils.h"

#define LF 10
#define CR 13

#define MAX_STRING_LEN 256

#pragma CHECKED_SCOPE on

int tfd;
char temp_template[16] : itype(char _Nt_checked[16]) = "/tmp/htp.XXXXXX";

void interrupted(int);

// Obviously this whole function could just be replaced with strdup, but I want
// to use it as a simple example for porting. ~ Matt
_Checked static char *strd(char *s : itype(_Nt_array_ptr<char>)) : itype(_Nt_array_ptr<char>) {
    size_t len = strlen(s);
    _Nt_array_ptr<char> d : count(len) = ((void *)0);

    d=((_Nt_array_ptr<char> )malloc_nt(len));
    xstrbcpy(d,s,len);
    return(d);
}

_Checked static void getword(char *word : itype(_Array_ptr<char>) count(255), char *line : itype(_Array_ptr<char>) count(255), char stop) {
    int x = 0,y;

    for(x=0;((line[x]) && (line[x] != stop));x++)
        word[x] = line[x];

    word[x] = '\0';
    if(line[x]) ++x;
    y=0;

    while((line[y++] = line[x++]));
}

_Checked static int my_getline(char *s : itype(_Array_ptr<char>) count(n), int n, FILE *f : itype(_Ptr<FILE>)) {
    int i=0;

    while(1) {
        s[i] = (char)fgetc(f);

        if(s[i] == CR)
            s[i] = fgetc(f);

        if((s[i] == 0x4) || (s[i] == LF) || (i == (n-1))) {
            s[i] = '\0';
            return (feof(f) ? 1 : 0);
        }
        ++i;
    }
}

_Checked static void putline(FILE *f : itype(_Ptr<FILE>), char *l : itype(_Array_ptr<char>) count(255)) {
    int x;

    for(x=0;l[x];x++) fputc(l[x],f);
    fputc('\n',f);
}


/* From local_passwd.c (C) Regents of Univ. of California blah blah */
static unsigned char itoa64[65] : itype(unsigned char _Checked[65]) =         /* 0 ... 63 => ascii - 64 */
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

_Checked static void to64(char *__3c_tmp_s : itype(_Array_ptr<char>) count(__tmp_n), long v, int __tmp_n) {
    int n = __tmp_n;
    _Array_ptr<char> s : bounds(__3c_tmp_s, __3c_tmp_s + __tmp_n) =  __3c_tmp_s;
    while (--n >= 0) {
        *s++ = itoa64[v&0x3f];
        v >>= 6;
    }
}

#ifdef MPE
/* MPE lacks getpass() and a way to suppress stdin echo.  So for now, just
issue the prompt and read the results with echo.  (Ugh). */

char *getpass(const char *prompt) {

static char password[81];

fputs(prompt,stderr);
gets((char *)&password);

if (strlen((char *)&password) > 8) {
  password[8]='\0';
}

return (char *)&password;
}
#endif

_Checked static void
add_password(char *user : itype(_Nt_array_ptr<char>), FILE *f : itype(_Ptr<FILE>))
    {
    char pass _Nt_checked[100];
    _Nt_array_ptr<char> pw = ((void *)0);
    _Nt_array_ptr<char> cpw = ((void *)0);
    char salt _Nt_checked[3];

    if ( ! isatty( fileno( stdin ) ) )
	{
	(void) fgets( pass, sizeof(pass) - 1, stdin );
	if ( pass[strlen(pass) - 1] == '\n' )
	    pass[strlen(pass) - 1] = '\0';
	pw = pass;
	}
    else
	{
	pw = strd( (_Nt_array_ptr<char>) getpass( "New password:" ) );
	if ( strcmp( pw, (_Nt_array_ptr<char>) getpass( "Re-type new password:" ) ) != 0 )
	    {
	    (void) fprintf( stderr, "They don't match, sorry.\n" );
	    if ( tfd != -1 ) {
                _Unchecked { unlink( temp_template ); }
            }
	    exit( 1 );
	    }
	}
    (void) srandom( (int) time( 0 ) );
    to64( salt, random(), 2 );
    cpw = ((_Nt_array_ptr<char> )crypt( pw, salt ));
    (void) fprintf( f, "%s:%s\n", user, cpw );
    }

_Checked static void usage(void) {
    fprintf(stderr,"Usage: htpasswd [-c] passwordfile username\n");
    fprintf(stderr,"The -c flag creates a new file.\n");
    exit(1);
}

_Checked void interrupted(int signo) {
    fprintf(stderr,"Interrupted.\n");
    if(tfd != -1) _Unchecked { unlink(temp_template); }
    exit(1);
}

_Checked int main(int argc, char **argv : itype(_Array_ptr<_Nt_array_ptr<char>>) count(argc)) {
    _Ptr<FILE> tfp = ((void *)0);
_Ptr<FILE> f = ((void *)0);

    char user _Nt_checked[MAX_STRING_LEN];
    char line _Nt_checked[MAX_STRING_LEN];
    char l _Nt_checked[MAX_STRING_LEN];
    char w _Nt_checked[MAX_STRING_LEN];
    char command _Nt_checked[MAX_STRING_LEN];
    int found;

    tfd = -1;
    signal(SIGINT,(_Ptr<void (int)>)interrupted);
    if(argc == 4) {
        if(strcmp(argv[1],"-c"))
            usage();
        if(!(tfp = fopen(argv[2],"w"))) {
            fprintf(stderr,"Could not open passwd file %s for writing.\n",
                    argv[2]);
            perror("fopen");
            exit(1);
        }
        printf("Adding password for %s.\n",argv[3]);
        add_password(argv[3],tfp);
        fclose(tfp);
        exit(0);
    } else if(argc != 3) usage();

    tfd = mkstemp(temp_template);
    if(!(tfp = fdopen(tfd,"w"))) {
        fprintf(stderr,"Could not open temp file.\n");
        exit(1);
    }

    if(!(f = fopen(argv[1],"r"))) {
        fprintf(stderr,
                "Could not open passwd file %s for reading.\n",argv[1]);
        fprintf(stderr,"Use -c option to create new one.\n");
        exit(1);
    }
    xstrbcpy(user,argv[2],sizeof(user)-1);
    user[sizeof(user)-1] = '\0';

    found = 0;
    while(!(my_getline(line,MAX_STRING_LEN - 1,f))) {
        if(found || (line[0] == '#') || (!line[0])) {
            putline(tfp,line);
            continue;
        }
        xstrbcpy(l, line, sizeof(line) - 1);
        getword(w,l,':');
        if(strcmp(user,w)) {
            putline(tfp,line);
            continue;
        }
        else {
            printf("Changing password for user %s\n",user);
            add_password(user,tfp);
            found = 1;
        }
    }
    if(!found) {
        printf("Adding user %s\n",user);
        add_password(user,tfp);
    }
    fclose(f);
    fclose(tfp);
    xsbprintf(command, sizeof(command) - 1, "cp %s %s",temp_template,argv[1]);
    system(command);
    _Unchecked { unlink(temp_template); }
    exit(0);
}
