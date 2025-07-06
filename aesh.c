/*
 * Copyright (C) 2025 kurumihere(fuck)
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

int aesh_cd(char **args);
int aesh_help(char **args);
int aesh_exit(char **args);

char *aesh_str[] = {
  "cd",
  "help",
  "exit"
};

int 
(*aesh_func[]) (char **) = {
  &aesh_cd,
  &aesh_help,
  &aesh_exit
};

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

char 
*aesh_rl(void)
{
#ifdef AESH_USE_STD_GETLINE
  char *line = NULL;
  ssize_t bufs = 0;
  if (getline(&line, &bufs, stdin) == -1) {
    if (feof(stdin)) {
      exit(EXIT_SUCCESS);
    } else  {
      perror("aesh: getline\n");
      exit(EXIT_FAILURE);
    }
  }
  return line;
#else
#define AESH_RL_BUFS 1024
  int bufs = AESH_RL_BUFS;
  int pos = 0;
  char *buff = malloc(sizeof(char) * bufs);
  int c;

  if (!buff) {
    fprintf(stderr, "aesh: alloc err\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } else if (c == '\n') {
      buff[pos] = '\0';
      return buff;
    } else {
      buff[pos] = c;
    }
    pos++;

    if (pos >= bufs) {
      bufs += AESH_RL_BUFS;
      buff = realloc(buff, bufs);
      if (!buff) {
        fprintf(stderr, "aesh: alloc err\n");
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
}

#define AESH_TOK_BUFS 64
#define AESH_TOK_DELIM " \t\r\n\a"

char 
**aesh_sl(char *line)
{
  int bufs = AESH_TOK_BUFS, pos = 0;
  char **tkns = malloc(bufs * sizeof(char*));
  char *tkn, **tkns_bckp;

  if (!tkns) {
    fprintf(stderr, "aesh: alloc err\n");
    exit(EXIT_FAILURE);
  }

  tkn = strtok(line, AESH_TOK_DELIM);
  while (tkn != NULL) {
    tkns[pos] = tkn;
    pos++;

    if (pos >= bufs) {
      bufs += AESH_TOK_BUFS;
      tkns_bckp = tkns;
      tkns = realloc(tkns, bufs * sizeof(char*));
      if (!tkns) {
		free(tkns_bckp);
        fprintf(stderr, "aesh: alloc err\n");
        exit(EXIT_FAILURE);
      }
    }

    tkn = strtok(NULL, AESH_TOK_DELIM);
  }
  tkns[pos] = NULL;
  return tkns;
}

void 
aesh_loop(void)
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
