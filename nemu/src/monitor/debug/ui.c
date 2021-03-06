#include <isa.h>
#include "expr.h"
#include "watchpoint.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
int is_batch_mode();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_si( char *args ) {
  	char *arg = strtok( NULL, " " );
	int step = 0;
	if( arg == NULL ) {
		cpu_exec(1);	
	}
	else {
		sscanf( arg ,"%d", &step );
		if( step <= 0 )
			cpu_exec(-1);
		else
			cpu_exec(step);	
	}
	return 0;		
}

static int cmd_info( char *args ) {

	char *arg = strtok( NULL, " " );
	if( arg == NULL )
		printf("agument default\n");
	else {
	
		if( strcmp( arg, "r") == 0 )
			isa_reg_display();
		else if( strcmp( arg, "w" ) == 0 ) {
			display_wp();	
		} else 	
		
			printf("Unkown command: %s\n", arg);
	}
	
	return 0;
}  
static int cmd_q(char *args) {
  return -1;
}
 
static int cmd_x( char *args ) {

	if( !args ) {
	
		printf("agument default\n");
		return 0;
	} else {
		
		int nums = 0;
		nums = atoi( strtok( NULL, " ") );
		if( nums == 0 ) {
			printf("error: 6 isn't agument\n");
			return 0;
		}
		char *dummy = strtok( NULL, " " );
		
		if( !dummy ) {
			printf("agument default\n");
			return 0;
		}
		else {
			extern word_t paddr_read( paddr_t, int );
			unsigned int addr = 0;
			sscanf( dummy, "%x", &addr );
			for( int i=0; i<nums; i++ )
				if( i == nums-1 )
					printf("%02x\n", paddr_read( addr+i, 1 ) );
				else
					printf("%02x ",   paddr_read( addr+i, 1) );
		}
		
	}

	return 0;
}
static int  cmd_p ( char *args ) {
	if( args == NULL ) {
		printf("too few agument\n");
		return 0;
	}
	bool success = true;
	word_t res;
	res = expr( args, &success );
	if( success )
		printf("Dec:%u, Hex: %08x\n",res, res );
	else 
		printf("faild\n");
	return 0;

}

static int cmd_w( char *args ) {

	if( !args ) { 
		printf("Please input agument\n");
		return 0;
	} 
	int NO;
	NO = set_wp( args );
	if( NO != -1 )
		printf("watchpoint %d:%s\n", NO, args );
	else 
		printf("set watchpoint faild\n");
 	return 0;	

}
static int cmd_d( char *args ) {

	if( args == NULL ) { 
		while(1) {
		printf("you want to delete all watchpoints?, Please y ? n\n");
		char c;
		scanf( "%c", &c );
		switch( c ) {
			case 'y': de_all(); return 0; 
			case 'n': return 0;
			default : printf("Please input y ? n\n"); 

			}
	   	}

	}
	int NO;
	if( sscanf( args ,"%d", &NO ) == 1 ) {
	       	if( de_wp( NO ) )
			printf("watchpoint %d:successly\n",NO);
		return 0;	
		
	}
	printf("agument:%serror\n",args);
	return 0;	

}
static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "step to instructs", cmd_si },
  { "info", "printf reg", cmd_info },
  { "x", "printf to mem", cmd_x },
  { "p", "printf expr", cmd_p}, 
  { "w", "set watchpoint", cmd_w},
  { "d", "delete watchpoint", cmd_d }
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop() {
  if (is_batch_mode()) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
