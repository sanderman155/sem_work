#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

#define ANSI_COLOR_RED     "\x1b[31;1m"
#define ANSI_COLOR_GREEN   "\x1b[32;1m"
#define ANSI_COLOR_YELLOW  "\x1b[33;1m"
#define ANSI_COLOR_BLUE    "\x1b[34;1m"
#define ANSI_COLOR_MAGENTA "\x1b[35;1m"
#define ANSI_COLOR_CYAN    "\x1b[36;1m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int bckrgnd = 0;
int bckrgnd_n = 0;
int pid = 0;
int nwline = 0;

void print() {
  char* pwd = getcwd(NULL, 256);
  char* user = getenv("USER");
  char host[256];
  gethostname(host, 256);
  printf(ANSI_COLOR_RED "%s@%s" ANSI_COLOR_RESET
        ":" ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET
        "$ ", user, host, pwd);
  fflush(stdout);
  free(pwd);
}

char *get_word(char *end) {
    char *word = NULL;
    int i = 0, br = 0;
    if (*end == '&') {
        bckrgnd = 1;
        read(0, end, 1);
        return word;
    }
    if (*end == '|') {
        return word;
    }
    if (*end == '>' || *end == '<') {
        word = (char *)realloc(word, 2);
        if (word == NULL) {
            perror("realloc");
            return NULL;
        }
        word[0] = *end;
        word[1] = '\0';
        if (read(0, end, 1) < 0) {
            perror("read");
            free(word);
            return NULL;
        }
        return word;
    }
    if (*end == '"') {
            br = 2;
            if (read(0, end, 1) < 0) {
                perror("read");
                free(word);
                return NULL;
            }
            if (*end == '"') {
                word = (char *)realloc(word, sizeof(char));
                if (word == NULL) {
                    perror("realloc");
                    return NULL;
                }
                *end = ' ';
                word[0] = '\0';
                return word;
            }
    }
    while (1) {
        if ((*end == ' ' || *end == '\n' || *end == '\t') && (br != 2)) {
            break;
        }
        if (*end == '"' && br == 2) {
            *end = ' ';
            break;
        }
        if ((*end == '<' || *end == '>' || *end == '|') && br != 2) {
            break;
        }
        word = (char *)realloc(word, (i + 2) * sizeof(char));
        if (word == NULL) {
            perror("realloc");
            return NULL;
        }
        word[i] = *end;
        if (read(0, end, 1) < 0) {
            perror("read");
            free(word);
            return NULL;
        }
        i++;
    }
    word[i] = '\0';
    return word;
}

void free_arr(char ***cmds) {
    if (cmds == NULL) {
        return;
    }
    for (int i = 0; cmds[i] != NULL; i++) {
        for (int j = 0; cmds[i][j] != NULL; j++) {
            free(cmds[i][j]);
        }
        free(cmds[i]);
    }
    free(cmds);
}

char **get_list(char *end) {
    char **words = NULL;
    ssize_t err = 0;
    int i;
    if (read(0, end, 1) < 0) {
        perror("read");
        return NULL;
    }
    for (i = 0; *end != '\n' && *end != '|'; i++) {
        words = (char **)realloc(words, (i + 2) * sizeof(char *));
        if (words == NULL) {
            perror("realloc");
            return NULL;
        }
        while (*end == ' ' || *end == '\t') {
            err = read(0, end, 1);
            if (err < 0) {
                perror("read");
                for (int j = 0; j < i; j++) {
                    free(words[j]);
                }
                free(words);
                return NULL;
            } else if (*end == '\n') {
                words[i] = NULL;
                return words;
            }
        }
        words[i] = get_word(end);
        if (bckrgnd == 1) {
            words[i + 1] = NULL;
            return words;
        }
        if (*end == '|') {
          words[i + 1] = NULL;
          return words;
        }
        if (words[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(words[j]);
            }
            free(words);
            return NULL;
        }
    }
    if (words != NULL) {
        words[i] = NULL;
    }
    return words;
}


char ***get_cmds(int *n) {
    char  ***cmds = NULL, end = '\0';
    int i = 0;
    while (end != '\n') {
        cmds = (char ***)realloc(cmds, (i + 2) * sizeof(char **));
        if (cmds == NULL) {
            perror("realloc char***");
            return NULL;
        }
        cmds[i] = get_list(&end);
        if (cmds[i] == NULL && end == '\n') {
            free_arr(cmds);
            *n = -1;
            return NULL;
        }
        if (cmds[i] == NULL) {
            perror("get_list");
            free_arr(cmds);
            return NULL;
        }
        i++;
    }
    *n = i;
    if (cmds[i] != NULL) {
        cmds[i] = NULL;
    }
    return cmds;
}

void del_w(char **cmd, int n) {
    char *word;
    while (cmd[n + 1] != NULL) {
        word = cmd[n];
        cmd[n] = cmd[n + 1];
        cmd[n + 1] = word;
        n++;
    }
    free(cmd[n]);
    free(cmd[n + 1]);
    cmd[n] = NULL;
}

int redir(char **cmd) {
    int fd, i = 0;
    while (cmd[i] != NULL) {
        if (strcmp(cmd[i], ">") == 0) {
            fd = open(cmd[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("failed to open file after <");
                exit(1);
            }
            dup2(fd, 1);
            break;
        } else if (strcmp(cmd[i], "<") == 0) {
            fd = open(cmd[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("failed to open file after >");
                exit(1);
            }
            dup2(fd, 0);
            break;
        }
        i++;
    }
    if (cmd[i] != NULL) {
        del_w(cmd, i);
        del_w(cmd, i);
    }
    return fd;
}

void do_cmds(char ***cmds, int n) {
    int fd1, fd2;
    if (strcmp(cmds[0][0], "cd") == 0) {
        char *home = getenv("HOME");
        if (cmds[0][1] == NULL || strcmp(cmds[0][1] , "~") == 0) {
            chdir(home);
        } else {
            chdir(cmds[0][1]);
        }
        return;
    }
    if (n == 1) {
        if (fork() == 0) {
            fd1 = redir(cmds[0]);
            fd2 = redir(cmds[0]);
            if (execvp(cmds[0][0], cmds[0]) < 0) {
                bckrgnd = 0;
                free_arr(cmds);
                perror("exec failed");
                if (fd1 != 0 && fd1 != 1) {
                    close(fd1);
                }
                if (fd2 != 0 && fd2 != 1) {
                    close(fd2);
                }
                exit(1);
            }
        } else {
            if (bckrgnd == 0) {
                wait(NULL);
            }
            bckrgnd = 0;
            return;
        }
    }
    int pipefd[n - 1][2];
    for (int i = 0; i < n; i++) {
        if (i != n - 1) {
            pipe(pipefd[i]);
        }
        if ((pid = fork()) == 0) {
            if (i == 0) {
                fd1 = redir(cmds[i]);
            } else if (i == n - 1) {
                fd2 = redir(cmds[i]);
            }
            if (i != 0) {
                dup2(pipefd[i - 1][0], 0);
            }
            if (i != n - 1) {
                dup2(pipefd[i][1], 1);
            }
            for (int j = 0; j < i + 1; j++) {
                if (j == n - 1) {
                    break;
                }
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }
            if (execvp(cmds[i][0], cmds[i]) < 0) {
                free_arr(cmds);
                perror("exec failed");
                      close(fd1);
                      close(fd2);
                exit(1);
            }
        }
    }
    if (bckrgnd) {
        n--;
        bckrgnd = 0;
    }
    for (int i = 0; i < n; i ++) {
        if (i != n - 1) {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
        }
        wait(NULL);
    }
    if (fd1 != 0 && fd1 != 1) {
        close(fd1);
    }
    if (fd2 != 0 && fd2 != 1) {
        close(fd2);
    }
}

void sl1() {
    if (pid != 0) {
        kill(SIGINT, pid);
        wait(NULL);
    }
    putchar('\n');
    print();
    pid = 0;
}

int main(void) {
    char ***cmds = NULL;
    int n = 0;
    signal(SIGINT, sl1);
    while (1) {
        print();
        cmds = get_cmds(&n);
        if (n == -1) {
            free(cmds);
            continue;
        }
        if (cmds == NULL || cmds[0] == NULL || cmds[0][0] == NULL) {
            free_arr(cmds);
            continue;
        }
        if (strcmp(cmds[0][0], "quit") == 0 ||
            strcmp(cmds[0][0], "exit") == 0) {
            free_arr(cmds);
            break;
        }
        do_cmds(cmds, n);
        free_arr(cmds);
    }
    return 0;
}
