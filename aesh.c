/*  Copyright (C) 2025 kurumihere(fuck)
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 * 
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>

#define HIST_SIZE 100
#define RL_BUFS 1024
#define TOK_BUFS 64
#define TOK_DELIM " \t\r\n\a"

/* master get array length (num elements) */
#define ARRLEN(x) (sizeof(x)/sizeof((x)[0]))

int aesh_cd(char **);
int aesh_help(char **);
int aesh_exit(char **);
int aesh_set(char **);
int aesh_get(char **);
int aesh_clear(char **);

char* home;

char *aesh_str[] = {
  "cd",
  "help",
  "exit",
  "set",
  "get",
  "clear"
};

char *aesh_str_desc[] = {
  "Change directory",
  "Show help",
  "Exit from the shell",
  "Set env var",
  "Get env var",
  "Clear the terminal (because of aeterm bah)"
};

int (*aesh_func[]) (char **) = {
  &aesh_cd,
  &aesh_help,
  &aesh_exit,
  &aesh_set,
  &aesh_get,
  &aesh_clear
};

char *mesg[] = {
  0,
  "operation not permitted",
  "no such file or directory",
  "no such process",
  "interrupted syscall",
  "input/output error",
  "no such device or address",
  "argument list exceeds limit",
  "file format is not executable",
  "bad file descriptor",
  "no child processes are running",
  "try again later",
  "no free memory left",
  "permission denied",
  "bad address",
  "block device required",
  "device is busy",
  "file already exists",
  "symlink across multiple devices",
  "no such device",
  "not a directory",
  "is a directory",
  "invalid argument",
  "file table overflow",
  "file descriptor count overflow (> 255)",
  "not a tty",
  "the file is busy",
  "the file is too large",
  "no space left on disk",
  "illegal file seek",
  "read-only filesystem",
  "too many symlinks",
  "broken pipe",
  "math domain error",
  "math result not representable"
};

char *hist[HIST_SIZE];
int hist_c = 0;
int hist_p = -1;

char* Prompt = "# ";

int 
aesh_cd(char **args) 
{
  if (args[1] == NULL) {
    if (chdir(home) != 0) {
      printf("aesh: %s: %s\n", home, mesg[errno]);
    }
  } else {
    if (chdir(args[1]) != 0) {
      printf("aesh: %s: %s\n", args[1], mesg[errno]);
    }
  }
  return EXIT_FAILURE;
}

int 
aesh_help(char **args) 
{
  int i;
  puts("aesh - ae shell written in c");
  puts("commands:");

  for (i = 0; i < ARRLEN(aesh_str); i++) {
    printf("  %s\t\t%s\n", aesh_str[i], aesh_str_desc[i]);
  }

  return EXIT_FAILURE;
}

int 
aesh_exit(char **args) 
{
  puts("AEEEE! ae ae AEEE");
  puts("aee");
  return EXIT_SUCCESS;
}

int 
aesh_clear(char **args) 
{
  fputs("\033[H\033[2J", stdout);
  return EXIT_FAILURE;
}

int 
aesh_set(char **args) 
{
  if (!args[2]) {
    fputs("set: incorrect usage\nusage: get <var> <value>\n", stderr);
    return EXIT_FAILURE;
  }
  setenv(args[1], args[2], 1);
  return EXIT_FAILURE;
}

int 
aesh_get(char **args) 
{
  if (!args[1]) {
    fputs("get: incorrect usage\nusage: get <var>\n", stderr);
    return EXIT_FAILURE;
  }
  char* v = getenv(args[1]);
  if (v) puts(v);
  return EXIT_FAILURE;
}

int 
aesh_launch(char **args) 
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      printf("aesh: %s: %s\n", args[0], mesg[errno]);
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    printf("aesh: %s\n", mesg[errno]);
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

  for (i = 0; i < ARRLEN(aesh_str); i++) {
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
    int i;
    for (i = 0; i < HIST_SIZE-1; i++) {
      hist[i] = hist[i+1];
    }
    hist_c--;
  }
  hist[hist_c++] = strdup(line);
  hist_p = -1;
}

char
*aesh_rl() 
{
  int bufs = RL_BUFS;
  int pos = 0;
  char *buff = malloc(bufs);
  int c;
  char *c_hist = NULL;

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
      hist_p = -1; // reset
      return buff;
    } else if (c == 27) { // ESC
      if (getchar() == '[') {
        int arrow = getchar();
        if (arrow == 'A') { // UP
          if (hist_p < hist_c - 1) {
            hist_p++;
            c_hist = hist[hist_c - 1 - hist_p];
            strncpy(buff, c_hist, bufs-1);
            buff[bufs-1] = '\0';
            pos = strlen(buff);
            printf("\r%s%s", Prompt, buff);
            fflush(stdout);
          }
        } else if (arrow == 'B') { // DOWN
          if (hist_p > 0) {
            hist_p--;
            c_hist = hist[hist_c - 1 - hist_p];
            strncpy(buff, c_hist, bufs-1);
            buff[bufs-1] = '\0';
            pos = strlen(buff);
            printf("\r%s%s", Prompt, buff);
            fflush(stdout);
          } else if (hist_p == 0) {
            hist_p = -1;
            pos = 0;
            buff[0] = '\0';
            printf("\r%s", Prompt);
            fflush(stdout);
          }
        }
      }
    } else if (c >= 32 && c < 255) {
      buff[pos++] = c;
      putchar(c);
      fflush(stdout);
    } else if (c == 4) {
      buff[0] = '\0';
      tcsetattr(0, TCSANOW, &old);
      puts("^D");
      hist_p = -1;
      return buff;
    }

    if (pos >= bufs-1) {
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
    fputs(Prompt, stdout);
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
  if (argc > 1) {
    if (!strcmp(argv[1], "--version")) {
      puts("aesh 1.0");
      return 1;
    }
  }

  setenv("SHELL", "aesh", 1);

  home = getenv("HOME");
  char rcfile[64];
  snprintf(rcfile, 64, "%s/.aeshrc", home);
  FILE* rc = fopen(rcfile, "r");
  if (!rc) {
    printf("aesh: ~/.aeshrc: %s\n", mesg[errno]);
    return EXIT_FAILURE;
  }
  char rcline[512];
  char **rcargs;
  int rcstat;

  while (fgets(rcline, 512, rc)) {
    rcargs = aesh_sl(rcline);
    rcstat = aesh_exec(rcargs);
 }
  fclose(rc);

  aesh_loop();
  return EXIT_SUCCESS;
}

