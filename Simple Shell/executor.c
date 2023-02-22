/* Maverick Kieu
   115028852
   kieu92
   May 7, 2019 */

#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <err.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include "command.h"
#include "executor.h"

#define FILE_PERMISSIONS 0664

/* static void print_tree(struct tree *t); */
int execute(struct tree *t);
int execute_aux(struct tree *t, int input_fd, int output_fd);

int execute(struct tree *t) {
   /* print_tree(t); */
   execute_aux(t, STDIN_FILENO, STDOUT_FILENO);

   return 0;
}

int execute_aux(struct tree *t, int input_fd, int output_fd) {
   int fd;
   pid_t pid = 0;

   if(!t) {
      return 0;
   }

   /* If the conjunction is NONE */
   if (t->conjunction == NONE) {
      if (!strcmp(t->argv[0], "cd")) {
         if (t->argv[1]) {
            if(chdir(t->argv[1]) < 0) {
               perror(t->argv[1]);
            }
         }
         else {
            getenv("HOME");
         }
      }
      else if (!strcmp(t->argv[0], "exit")) {
         exit(EXIT_SUCCESS);
      }

      else {
         /* Forking */
         pid = 0;

         if ((pid = fork()) < 0) {
            err(EX_OSERR, "fork error");
         }
         /* Parent process */
         if (pid) {
            int num;

            wait(&num); /* waiting for child to finish */
            if (WIFEXITED(num)) {
               if (WEXITSTATUS(num) == 0) {
                  return 0;
               } else {
                  return 1;
               }
            }
         }
         /* Child process */
         else {
            /* Input redirection */
            if (t->input) {
               if ((fd = open(t->input, O_RDONLY)) < 0) {
                  err(EX_OSERR, "File opening (read) failed");
               }

               if (dup2(fd, STDIN_FILENO) < 0) {
                  err(EX_OSERR, "dup2 (read) failed");
               }

               close(fd); /* Releasing resource */
            }

            /* Output redirection */
            if (t->output) {
               if ((fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS)) < 0) {
                  err(EX_OSERR, "File opening (write) failed");
               }

               if (dup2(fd, STDOUT_FILENO) < 0) {
                  err(EX_OSERR, "dup2 (write) failed");
               }

               close(fd); /* Releasing resource */
            }

            /* Child code */
            execvp(t->argv[0], t->argv);
            fprintf(stderr, "Failed to execute %s\n", t->argv[0]);
            fflush(stdout);
            exit(EX_OSERR);
         }
      }
   }

   /* If the conjunction is AND */
   else if (t->conjunction == AND) {
      if(execute_aux(t->left, input_fd, output_fd) == 0) {
         if(execute_aux(t->right, input_fd, output_fd != 0)) {
            return 1;
         }
      }
   }

   /* If the conjunction is PIPE */
   else if (t->conjunction == PIPE) {
      pid_t child_pid_one, child_pid_two;
      int pipe_fd[2]; /* pipe */

      if(t->right->input) {
         printf("Ambiguous input redirect.\n");
         fflush(stdout);
      }
      if(t->left->output) {
         printf("Ambiguous output redirect.\n");
         fflush(stdout);
      }

      /* Before the fork, we need to create the pipe */ 
      /* (otherwise no sharing of pipe) */
      if (pipe(pipe_fd) < 0) {
         err(EX_OSERR, "pipe error");
      }

      if ((child_pid_one = fork()) < 0) {
         err(EX_OSERR, "fork error");
      }

      if (child_pid_one == 0)  { /* CHILD #1's code */
         close(pipe_fd[0]); /* we don't need pipe's read end */

         /* Redirecting standard output to pipe write end */
         if (dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
            err(EX_OSERR, "dup2 error");
         }
         /* Releasing resource */     
         close(pipe_fd[1]);
   
         execute_aux(t->left, input_fd, pipe_fd[1]);
         exit(EX_OSERR);
      }  else { /* parent's code */
   
         /* Creating second child */
         if ((child_pid_two = fork()) < 0) {
            err(EX_OSERR, "fork error");
         } 

         if (child_pid_two == 0)  { /* CHILD #2's code */
            close(pipe_fd[1]); /* we don't need pipe's write end */

            /* Redirecting standard input to pipe read end */
            if (dup2(pipe_fd[0], STDIN_FILENO) < 0) {
               err(EX_OSERR, "dup2 error");
            }
            /* Releasing resource */
            close(pipe_fd[0]);

            execute_aux(t->right, pipe_fd[0], output_fd);
            exit(EX_OSERR);
         } else {
            /* Parent has no need for the pipe */
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            /* Reaping each child */
            wait(NULL); 
            wait(NULL);
         }
      }
   }

   else if(t->conjunction == SUBSHELL) {
      pid_t pid2 = 0;

      if ((pid2 = fork()) < 0) {
         err(EX_OSERR, "fork error");
      }
      if (pid2) { /* parent code */
         wait(NULL); /* waiting for child to finish */
      }
      else {
         /* Input redirection */
         if (t->input) {
            if ((fd = open(t->input, O_RDONLY)) < 0) {
               err(EX_OSERR, "File opening (read) failed");
            }

            if (dup2(fd, STDIN_FILENO) < 0) {
               err(EX_OSERR, "dup2 (read) failed");
            }

               close(fd); /* Releasing resource */
         }

         /* Output redirection */
         if (t->output) {
            if ((fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS)) < 0) {
               err(EX_OSERR, "File opening (write) failed");
            }

            if (dup2(fd, STDOUT_FILENO) < 0) {
               err(EX_OSERR, "dup2 (write) failed");
            }

            close(fd); /* Releasing resource */
         }

         if(execute_aux(t->left, input_fd, output_fd) == 0) {
            exit(EXIT_SUCCESS);
         }  
         else {
            err(EX_OSERR, "exec error");
         }
      }
   }

   return 0;
}

/* static void print_tree(struct tree *t) {
   if (t != NULL) {
      print_tree(t->left);

      if (t->conjunction == NONE) {
         printf("NONE: %s, ", t->argv[0]);
      } else {
         printf("%s, ", conj[t->conjunction]);
      }
      printf("IR: %s, ", t->input);
      printf("OR: %s\n", t->output);

      print_tree(t->right);
   }
} */
