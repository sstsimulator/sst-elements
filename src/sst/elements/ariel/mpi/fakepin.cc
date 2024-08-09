#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[])
{
    if (argc < 3) {
        printf("Usage: ./fakepin -- <program> [program args...]\n");
        exit(1);
    }

    int prog_idx = 1;

    while (strcmp("--", argv[prog_idx])) {
        prog_idx++;
    }
    prog_idx++;

    printf("prog_name: %s\n", argv[prog_idx]);

    // Make a copy of envp so we can add FAKEPIN=1
    char **envp_copy;

    int envp_len = 0;
    while(envp[envp_len]!=NULL) {
        envp_len++;
    }

    envp_copy = (char**) malloc(sizeof(char*) * (envp_len + 1));
    for (int i = 0; i < envp_len - 1; i++) {
        envp_copy[i] = (char*) malloc(sizeof(char) * (strlen(envp[i])+1));
        strcpy(envp_copy[i], envp[i]);
    }
    envp_copy[envp_len-1] = (char*) malloc(sizeof(char) * 10);
    strcpy(envp_copy[envp_len-1], "FAKEPIN=1");
    envp_copy[envp_len] = NULL;


    // Launch the program
    char* _argv[] = {NULL};
    printf("Fakepin launching [%s]\n", argv[prog_idx]);

    if (execve(argv[prog_idx], &argv[prog_idx], envp_copy) == -1) {
        perror("Could not execve");
    }
}
