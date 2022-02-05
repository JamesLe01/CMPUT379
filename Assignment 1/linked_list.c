#include "msh379.h"


// Initialize linked list
// head has no value
// The first element in the linked list starts at head->next
node_t *linked_list_init(void) {
    node_t *head = (node_t *) malloc(sizeof(node_t));
    head->next = NULL;
    return head;
}


// Append a node into linked list
void linked_list_append(node_t *head, char *string1, char *string2, char *string3, char *string4, char *string5, char *string6) {
    node_t *new_node = (node_t *) malloc(sizeof(node_t));  // allocate new node
    // populate new node
    strcpy(new_node->string1, string1);
    strcpy(new_node->string2, string2);
    strcpy(new_node->string3, string3);
    strcpy(new_node->string4, string4);
    strcpy(new_node->string5, string5);
    strcpy(new_node->string6, string6);
    new_node->next = NULL;

    // traverse to the end of linked list
    node_t *current = head;
    while (current->next)
        current = current->next;
    current->next = new_node; // append
}


// print all the info of processes in descendant trees root at target_id
// recursive function
void print_all_descendants(int target_id, node_t *head) {
    node_t *current = head;
    while (current != NULL) {
        if (atoi(current->string3) == target_id) {  // Check if the parent_id of every nodes in the linked list matches the target_id
            printf("%s %s %s %s %s %s\n", current->string1, current->string2, current->string3, current->string4, current->string5, current->string6);
            print_all_descendants(atoi(current->string2), head);
        }
        current = current->next;
    }
}


// free all the nodes in linked list
void free_linked_list(node_t *head) {
    node_t *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}

