/*  Copyright (C) 2025 kurumihere(fuck)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>

#define HIST_SIZE 100
#define RL_BUFS 1024
#define TOK_BUFS 64
#define TOK_DELIM " \t\r\n\a"

int aesh_cd(char **);
int aesh_help(char **);
int aesh_exit(char **);

char *aesh_str[] = {
  "cd",
  "help",
  "exit"
};

int (*aesh_func[]) (char **) = {
  &aesh_cd,
  &aesh_help,
  &aesh_exit
};

char *hist[HIST_SIZE];
int hist_c = 0;

int 
aesh_num_all() 
{
  return sizeof(aesh_str) / sizeof(char *);
}

int 
aesh_cd(char **args) 
{
  if (args[1] == NULL) {
    fprintf(stderr, "aesh: error\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("aesh");
    }
  }
  return EXIT_FAILURE;
}

int 
aesh_help(char **args) 
{
  int i;
  puts("aesh - ae shell written on c");
  puts("commands:");

  for (i = 0; i < aesh_num_all(); i++) {
    printf("  %s\n", aesh_str[i]);
  }

  return EXIT_FAILURE;
}

int 
aesh_exit(char **args) 
{
  return EXIT_SUCCESS;
}

int 
aesh_launch(char **args) 
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("aesh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("aesh");
  } else {
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return EXIT_FAILURE;
}

int 
aesh_exec(char **args) 
{
  int i;

  if (args[0] == NULL) {
    return EXIT_FAILURE;
  }

  for (i = 0; i < aesh_num_all(); i++) {
    if (strcmp(args[0], aesh_str[i]) == 0) {
      return (*aesh_func[i])(args);
    }
  }

  return aesh_launch(args);
}

void 
ath(char *line) 
{
  if (hist_c >= HIST_SIZE) {
    free(hist[0]);
    for (int i = 0; i < HIST_SIZE-1; i++) {
      hist[i] = hist[i+1];
    }
    hist_c--;
  }
  hist[hist_c++] = strdup(line);
}


char 
*aesh_rl() 
{
  int bufs = RL_BUFS;
  int pos = 0;
  char *buff = malloc(bufs);
  int c;

  if (!buff) {
    fprintf(stderr, "aesh: alloc err\n");
    exit(1);
  }

  struct termios old, new;
  tcgetattr(0, &old);
  new = old;
  new.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &new);

  while (1) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buff[pos] = '\0';
      tcsetattr(0, TCSANOW, &old);
      putchar('\n');
      if (pos > 0) ath(buff);
      return buff;
    } else if (c == 27) { // ESC
      if (getchar() == '[') {
        if (getchar() == 'A' && hist_c > 0) { // UP
          strncpy(buff, hist[hist_c-1], bufs);
          pos = strlen(buff);
          printf("\r# %s", buff);
          fflush(stdout);
        }
      }
    } else if (c >= 32 && c < 127) {
      buff[pos++] = c;
      putchar(c);
    }

    if (pos >= bufs) {
      bufs += RL_BUFS;
      buff = realloc(buff, bufs);
      if (!buff) {
        fprintf(stderr, "aesh: alloc err\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

char 
**aesh_sl(char *line) 
{
  int bufs = TOK_BUFS, pos = 0;
  char **tkns = malloc(bufs * sizeof(char*));
  char *tkn;

  if (!tkns) {
    fprintf(stderr, "aesh: alloc err\n");
    exit(EXIT_FAILURE);
  }

  tkn = strtok(line, TOK_DELIM);
  while (tkn != NULL) {
    tkns[pos] = tkn;
    pos++;

    if (pos >= bufs) {
      bufs += TOK_BUFS;
      tkns = realloc(tkns, bufs * sizeof(char*));
      if (!tkns) {
        fprintf(stderr, "aesh: alloc err\n");
        exit(EXIT_FAILURE);
      }
    }

    tkn = strtok(NULL, TOK_DELIM);
  }
  tkns[pos] = NULL;
  return tkns;
}

void 
aesh_loop() 
{
  char *line;
  char **args;
  int status;

  do {
    printf("# ");
    line = aesh_rl();
    args = aesh_sl(line);
    status = aesh_exec(args);

    free(line);
    free(args);
  } while (status);
}

int 
main(int argc, char **argv) 
{
  aesh_loop();
  return EXIT_SUCCESS;
}
