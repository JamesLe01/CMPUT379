#ifndef MSH379_H    /* This is an "include guard" */
#define MSH379_H     /* prevents the file from being included twice. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <signal.h>
#include <time.h>

#define MAXLINE 4096
#define NTASK 32

// node structure for "ps" linked list
typedef struct node {
    char string1[MAXLINE];
    char string2[MAXLINE];
    char string3[MAXLINE];
    char string4[MAXLINE];
    char string5[MAXLINE];
    char string6[MAXLINE];
    struct node *next;
} node_t;


node_t *linked_list_init(void);
void linked_list_append(node_t *head, char *string1, char *string2, char *string3, char *string4, char *string5, char *string6);
void print_all_descendants(int target_id, node_t *head);
void free_linked_list(node_t *head);



#endif