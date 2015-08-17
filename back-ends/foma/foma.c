/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2014 Mans Hulden                                     */

/*     This file is part of foma.                                            */

/*     Foma is free software: you can redistribute it and/or modify          */
/*     it under the terms of the GNU General Public License version 2 as     */
/*     published by the Free Software Foundation.                            */

/*     Foma is distributed in the hope that it will be useful,               */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*     GNU General Public License for more details.                          */

/*     You should have received a copy of the GNU General Public License     */
/*     along with foma.  If not, see <http://www.gnu.org/licenses/>.         */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#if defined(ORIGINAL) || defined(HAVE_READLINE)
  #include <readline/readline.h>
#endif
#include "foma.h"

/* Front-end behavior variables */
int pipe_mode = 0;
int quiet_mode = 0;
#if defined(ORIGINAL) || defined(HAVE_READLINE)
  static int use_readline = 1;
#else
  static int use_readline = 0;
#endif

int promptmode = PROMPT_MAIN;
int apply_direction;

/* Variable to pass the position of rl completion to our completer */
#if defined(ORIGINAL) || defined(HAVE_READLINE)
  static int smatch;
#endif

char *usagestring = "Usage: foma [-e \"command\"] [-f run-once-script] [-l startupscript] [-p] [-q] [-s] [-v]\n";

#if defined(ORIGINAL) || defined(HAVE_READLINE)
static char** my_completion(const char*, int ,int);
char *my_generator(const char* , int);
char *cmd [] = {"ambiguous upper","apply down","apply med","apply up","apropos","assert-stack","clear stack","close sigma","compact sigma","complete net","compose net","concatenate net","crossproduct net","define","determinize net","echo","eliminate flags","eliminate flag","export cmatrix","extract ambiguous","extract unambiguous","factorize","help license","help warranty","ignore net","intersect net","invert net","label net","letter machine","load defined","lower-side net","minimize net","name net","negate net","one-plus net","pop stack","print defined","print dot","print lower-words","print cmatrix","print name","print net","print random-lower","print random-upper","print random-words","print sigma","print size","print shortest-string","print shortest-string-length","print words","print pairs","print random-pairs","print upper-words","prune net","push defined","quit","read att","read cmatrix","read prolog","read lexc","read regex","read spaced-text","read text","reverse net","rotate stack","save defined","save stack","sequentialize","set","show variables","show variable","shuffle net","sigma","sigma net","source","sort in","sort net","sort out","substitute defined","substitute symbol","system","test unambiguous","test star-free","test equivalent","test functional","test identity","test lower-universal","test upper-universal","test non-null","test null","test sequential","turn stack","twosided flag-diacritics","undefine","union net","upper-side net","view net","write att","write prolog","zero-plus net",NULL};

char *abbrvcmd [] = {"ambiguous","close","down","up","med","size","loadd","lower-words","upper-words","net","random-lower","random-upper","words","random-words","regex","rpl","au revoir","bye","exit","saved","seq","ss","stack","tunam","tid","tfu","tlu","tuu","tnu","tnn","tseq","tsf","equ","pss","psz","ratt","tfd","hyvästi","watt","wpl","examb","exunamb","pairs","random-pairs",NULL};
#endif

/* #include "yy.tab.h" */

int view_net(struct fsm *net);

extern int input_is_file;
extern int add_history (const char *);
extern int my_yyparse(char *my_string);
void print_help();
void xprintf(char *string) { return ; printf("%s",string); }
char disclaimer1[] = "Foma, version ";
char disclaimer2[] = "\nCopyright © 2008-2015 Mans Hulden\nThis is free software; see the source code for copying conditions.\nThere is ABSOLUTELY NO WARRANTY; for details, type \"help license\"\n\nType \"help\" to list all commands available.\nType \"help <topic>\" or help \"<operator>\" for further help.\n\n";

#ifndef SVN_REV
#define SVN_REV 0
#endif

/* A static variable for holding the line. */

static char *command = (char *)NULL;
char *flex_command = NULL;
static char *line_read = (char *)NULL;
char no_readline_line[512];

/* Read a string, and return a pointer to it.
   Returns NULL on EOF. */

char *rl_gets(char *prompt) {
    
    /* If the buffer has already been allocated,
       return the memory to the free pool. */
#if defined(ORIGINAL) || defined(HAVE_READLINE)
    if (use_readline == 1) {
        if (line_read) {
            free(line_read);
            line_read = (char *)NULL;
        }
    }
    if (use_readline == 0) {
#endif
        printf("%s",prompt);
        line_read = fgets(no_readline_line, 511, stdin);
        if (line_read != NULL) {
            strip_newline(line_read);
        }
#if defined(ORIGINAL) || defined(HAVE_READLINE)
    } else {
        line_read = readline(prompt);
    }
    
    /* If the line has any text in it,
       save it on the history. */
    if (use_readline == 1) {
        if (line_read && *line_read)
            add_history(line_read);        
    }
#endif
    return (line_read);
}

int main(int argc, char *argv[]) {
    int opt;

    char *scriptfile, prompt[50];
    extern void my_interfaceparse(char *my_string);
    /*  YY_BUFFER_STATE flex_command; */
    stack_init();
    srand ((unsigned int)time(NULL));
    /* Init defined_networks structures */
    g_defines = defined_networks_init();
    g_defines_f = defined_functions_init();

    while ((opt = getopt(argc, argv, "e:f:hl:pqrsv")) != -1) {
        switch(opt) {
        case 'e':
            my_interfaceparse(optarg);
            break;
        case 'f':
            scriptfile = file_to_mem(optarg);
            if (scriptfile != NULL) {
                input_is_file = 1;
                my_interfaceparse(scriptfile);
            }
            exit(0);
        case 'h':
            print_help();
            exit(0);
        case 'l':
            scriptfile = file_to_mem(optarg);
            if (scriptfile != NULL) {
                input_is_file = 1;
                my_interfaceparse(scriptfile);
		xxfree(scriptfile);
            }
            break;
        case 'p':
            pipe_mode = 1;
            break;
        case 'q':
            quiet_mode = 1;
            break;
        case 'r':
            use_readline = 0;
            break;
        case 's':
	  exit(0);
        case 'v':
            printf("%s %i.%i.%i%s\n",argv[0],MAJOR_VERSION,MINOR_VERSION,BUILD_VERSION,STATUS_VERSION);
            exit(0);
        default:
            fprintf(stderr, "%s", usagestring);
            exit(EXIT_FAILURE);
        }
    }

    if (!pipe_mode && !quiet_mode) 
      printf("%s%i.%i.%i%s (svn r%i)%s",disclaimer1,MAJOR_VERSION,MINOR_VERSION,BUILD_VERSION,STATUS_VERSION,SVN_REV,disclaimer2);
#if defined(ORIGINAL) || defined(HAVE_READLINE)
    rl_basic_word_break_characters = " >";

    rl_attempted_completion_function = my_completion;
#endif
    for(;;) {
        if (promptmode == PROMPT_MAIN)
            sprintf(prompt, "foma[%i]: ",stack_size());
        if (promptmode == PROMPT_A && apply_direction == AP_D)
            sprintf(prompt, "apply down> ");
        if (promptmode == PROMPT_A && apply_direction == AP_U)
            sprintf(prompt, "apply up> ");
        if (promptmode == PROMPT_A && apply_direction == AP_M)
            sprintf(prompt, "apply med> ");
        if (pipe_mode || quiet_mode)
	    prompt[0] = '\0';

	fflush(stdout);
	
        command = rl_gets(prompt);

        if (command == NULL && promptmode == PROMPT_MAIN) {
            printf("\n");
            exit(0);
        }
        if (command == NULL && promptmode == PROMPT_A) {
            /* apply_clear(); */
            promptmode = PROMPT_MAIN;
            printf("\n");
            continue;
        }
        input_is_file = 0;
        my_interfaceparse(command);
    }
}

void print_help() {
    printf("%s",usagestring);
    printf("Options:\n");
    printf("-e \"command\"\texecute a command on startup (-e can be invoked several times)\n");
    printf("-f scriptfile\tread commands from scriptfile on startup, and quit\n");
    printf("-l scriptfile\tread commands from scriptfile on startup\n");
    printf("-p\t\tpipe-mode\n");
    printf("-q\t\tquiet mode (more quiet than pipe-mode)\n");
    printf("-r\t\tdon't use readline library for input\n");
    printf("-s\t\tstop execution and exit\n");
    printf("-v\t\tprint version number\n");
}

#if defined(ORIGINAL) || defined(HAVE_READLINE)
static char **my_completion(const char *text, int start, int end) {
    char **matches;

    matches = (char **)NULL;
    smatch = start;
    matches = rl_completion_matches ((char*)text, &my_generator);
    
    return (matches);    
}

char *my_generator(const char *text, int state) {
    static int list_index, list_index2, len, nummatches;
    char *name;
    text = rl_line_buffer;
    if (!state) {
        list_index = 0;
        list_index2 = 0;
        nummatches = 0;
        len = strlen(text);
    }
    
    while ((name = cmd[list_index])) {
        list_index++;

        if (strncmp (name, text, len) == 0) {
            nummatches++;            
            /* Can't use xxstrdup here */
            return(strdup(name+smatch));
        }
    }
    
    if (rl_point > 0) {
        while ((name = abbrvcmd[list_index2])) {
            list_index2++;
            
            /* Can't use xxstrdup here */
            if (strncmp (name, text, len) == 0)
                return(strdup(name+smatch));
        }        
    }
    
    /* If no names matched, then return NULL. */
    return ((char *)NULL);
}
#endif
