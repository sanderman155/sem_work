#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

char *get_word(char *end) {
    char *word = NULL;
    int i;
    for (i = 0; *end != ' ' && *end != '\n'; i++) {
        word = (char *)realloc(word, (i + 2) * sizeof(char));
        if (word == NULL) {
            perror("realloc");
            free(word);
            return NULL;
        }
        word[i] = *end;
        if (read(0, end, 1) < 0) {
            perror("read");
            free(word);
            return NULL;
        }
    }
    word[i] = '\0';
    return word;
}

char **get_list() {
    char end = ' ', **words = NULL;
    ssize_t err = 0;
    int i;
    for (i = 0; end != '\n'; i++) {
        words = (char **)realloc(words, (i + 2) * sizeof(char *));
        if (words == NULL) {
            perror("realloc");
            for (int j = 0; j < i; j++) {
                free(words[j]);
            }
            free(words);
            return NULL;
        }
        while (end == ' ') {
            err = read(0, &end, 1);
            if (err < 0) {
                perror("read");
                for (int j = 0; j < i; j++) {
                    free(words[j]);
                }
                free(words);
                return NULL;
            } else if (end == '\n') {
                words[i] = NULL;
                return words;
            }
        }
        words[i] = get_word(&end);
        if (words[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(words[j]);
            }
            free(words);
            return NULL;
        }
    }
    words[i] = NULL;
    return words;
}


void free_list(char **list) {
    for (int i = 0; list[i] != NULL; i++) {
        free(list[i]);
    }
    free(list);
}

int main(void) {
    char **cmd = NULL;
    while (1) {
        cmd = get_list();
        if (strcmp(cmd[0], "exit") == 0) {
            free_list(cmd);
            return 0;
        }
        if (cmd == NULL) {
            return 1;
        }
        if (fork() > 0) {
            wait(NULL);
            free_list(cmd);
        } else {
            if (execvp(cmd[0], cmd) < 0) {
                free_list(cmd);
                perror("exec failed");
                return 1;
            }
        }
    }
}
