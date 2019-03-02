#include "tinyproxy.h"


void handler(int sig){
    pid_t pid;

    switch(sig) {

       case SIGINT :
           kill(0, SIGINT);
           exit(0);
           break;

      case SIGCHLD :
          if ((pid = waitpid(-1, NULL, 0)) < 0)
          	unix_error("waitpid error");
          break;

      default :
         printf("Erreur de signal");

	}

}

