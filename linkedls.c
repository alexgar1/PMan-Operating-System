#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "linkedls.h"

 
Node * newNode(Node* head, pid_t new_pid, char * new_path) {
    // Create a new node
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Malloc failed");
        return head;
    }

    new_node->pid = new_pid;
    new_node->path = strdup(new_path);
    new_node->next = NULL;

    if (head == NULL) {
        return new_node;
    }

    Node *tmp = head;
    while (tmp->next != NULL) {
        tmp = tmp->next;
    }
    tmp->next = new_node;

    return head;
}

Node * deleteNode(Node *head, pid_t pid) {
    Node *tmp = head;
    Node *prev = NULL;

    if (tmp == NULL) {
        // printf("Process %d not found\n", pid);
        return head;
    }

    // if pid in head
    else if (tmp->pid == pid) {
        head = tmp->next;
        free(tmp->path);
        free(tmp);
        return head;
    }

    // otherwise search
    while (tmp != NULL && tmp->pid != pid) {
        prev = tmp;
        tmp = tmp->next;
    }

    prev->next = tmp->next;

    free(tmp->path);
    free(tmp);

    return head;
}

void printList(Node *node) {
    // print linked list contents and the total number of contents
    int jobs = 0;
    while (node != NULL) {
        jobs++;
        printf("%d: %s\n", node->pid, node->path);
        node = node->next;
    }
    printf("Total background jobs: %d\n", jobs);
}

int pidExist(Node *node, pid_t pid) {
    // search for pid
    while (node != NULL) {
        if (node->pid == pid) {
            return 1;
        }
        node = node->next;
    }
    printf("Process %d not found\n", pid);
    return 0;
}



