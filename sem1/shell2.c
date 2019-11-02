#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

int fd[2];

char *get_word(char *end) {
    char *word = NULL;
    int i;
    for (i = 0; *end != ' ' && *end != '\n'; i++) {
        word = (char *)realloc(word, (i + 2) * sizeof(char));
    int i = 0, br = 0;
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
        if (*end == '<' || *end == '>' || *end == '|') {
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

char **get_list() {
    char end = ' ', **words = NULL;
void free_3_arr(char ***cmds) {
    for (int i = 0; cmds[i] != NULL; i++) {
        for (int j = 0;cmds[i][j] != NULL; j++) {
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
    for (i = 0; end != '\n'; i++) {
    if (read(1, end, 1) < 0) {
        perror("read");
        return NULL;
    }
    for (i = 0; *end != '\n' && *end != '|'; i++) {
        words = (char **)realloc(words, (i + 2) * sizeof(char *));
        if (words == NULL) {
            perror("realloc");
@@ -39,21 +102,24 @@ char **get_list() {
            free(words);
            return NULL;
        }
        while (end == ' ') {
            err = read(0, &end, 1);
        while (*end == ' ' || *end == '\t') {
            err = read(0, end, 1);
            if (err < 0) {
                perror("read");
                for (int j = 0; j < i; j++) {
                    free(words[j]);
                }
                free(words);
                return NULL;
            } else if (end == '\n') {
            } else if (*end == '\n') {
                words[i] = NULL;
                return words;
            }
        }
        words[i] = get_word(&end);
        words[i] = get_word(end);
        if (*end == '|') {
          return words;
        }
        if (words[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(words[j]);
@@ -62,10 +128,36 @@ char **get_list() {
            return NULL;
        }
    }
    words[i] = NULL;
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
        printf("%d\n", end);
        if (cmds[i] == NULL) {
            perror("get_list");
            free_3_arr(cmds);
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

void free_list(char **list) {
    for (int i = 0; list[i] != NULL; i++) {
@@ -74,26 +166,159 @@ void free_list(char **list) {
    free(list);
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

void do_cmd(char **cmd, char ***cmds) {
    int fd1, fd2;
    if(strcmp(cmd[0], "exit") == 0 || strcmp(cmd[0], "quit") == 0) {
        free_3_arr(cmds);
        exit(0);
    }
    if(cmd == NULL) {
        free_3_arr(cmds);
        exit(1);
    }
    if (fork() > 0) {
        wait(NULL);
        free_3_arr(cmds);
    } else {
        fd1 = redir(cmd);
        fd2 = redir(cmd);
        if (execvp(cmd[0], cmd) < 0) {
            free_3_arr(cmds);
            perror("exec failed");
            close(fd1);
            close(fd2);
            exit(1);
        }
        if (fd1 != 0) {
            close(fd1);
        }
        if (fd2 != 0) {
            close(fd2);
        }
        exit(0);
    }
}

void do_cmds(char ***cmds, int n) {
    int fd1, fd2;
    fd1 = redir(cmds[0]);
    fd2 = redir(cmds[n - 1]);
    //pipe(fd);
    printf("%d\n", n);
    int pipefd[n][2], pid;
    for(int i = 0; i < n; i++) {
        pipe(pipefd[i]);
        if ((pid = fork()) == 0) {
            if (n == 1) {
              if (execvp(cmds[0][0], cmds[0]) < 0) {
                  free_3_arr(cmds);
                  perror("exec failed");
                  close(fd1);
                  close(fd2);
                  exit(1);
              }
            }
            if(i != 0) {
              dup2(pipefd[i - 1][0], 0);
              close(pipefd[i - 1][0]);
              close(pipefd[i - 1][1]);
            }
            if (i != n - 1) {
                dup2(pipefd[i][1], 1);
            }
            close(pipefd[i][0]);
            close(pipefd[i][1]);
            execvp(cmds[i][0], cmds[i]);
            free_3_arr(cmds);
            exit(0);
        } else {
          wait(NULL);
        }
//        dup2(fd[0], 0);
        close(pipefd[i][0]);
        close(pipefd[i][1]);
    }
    if (fd1 != 0 && fd1 != 1) {
        close(fd1);
    }
    if (fd2 != 0 && fd2 != 1) {
        close(fd2);
    }
}

void print(char ***cmds) {
  for (int i = 0; cmds[i] != NULL; i++) {
    for (int j = 0; cmds[j] != NULL; j++){
        printf("%s", cmds[i][j]);
    }
    printf("\n");
  }
}
int main(void) {
    char **cmd = NULL;
    char ***cmds = NULL;
    int n = 0;
    while (1) {
        cmd = get_list();
        if (strcmp(cmd[0], "exit") == 0) {
            free_list(cmd);
            return 0;
        }
        if (cmd == NULL) {
        cmds = get_cmds(&n);
//        print(cmds);
        if (cmds == NULL) {
            perror("something went wrong");
            return 1;
        }
        if (fork() > 0) {
            wait(NULL);
            free_list(cmd);
        if (strcmp(cmds[0][0], "quit") == 0 || strcmp(cmds[0][0], "exit") == 0) {
            free_3_arr(cmds);
            break;
        }
        if (fork() == 0) {
            do_cmds(cmds, n);
            free_3_arr(cmds);
            return 0;
        } else {
            if (execvp(cmd[0], cmd) < 0) {
                free_list(cmd);
                perror("exec failed");
                return 1;
            }
            wait(NULL);
            free_3_arr(cmds);
        }
    }
    return 0;
}
