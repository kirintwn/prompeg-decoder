#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


typedef struct PacketContainer_ {
    int size;
    int used;
    unsigned char packetData[0];
} __attribute__((scalar_storage_order("big-endian"))) PacketContainer_;

typedef struct queueNode_ {
    PacketContainer_ data;
    struct queueNode_* next;
} queueNode_;

typedef struct queue_ {
    queueNode_ *head;
    queueNode_ *tail;
    int size;
} queue_;

queue_* queue_construct();
void queue_destruct(queue_* queue);
int queue_enqueue(queue_* queue , queueNode_* queueNode);
queueNode_* queue_dequeue(queue_* queue);
int isEmpty(queue_* queue);

queue_* queue_construct() {
    queue_* queue = (queue_*) malloc(sizeof (queue_));
    if(queue == NULL)
        return NULL;
    else {
        queue -> head = NULL;
        queue -> tail = NULL;
        queue -> size = 0;
    }
    return queue;
}

void queue_destruct(queue_* queue) {
    queueNode_* recycleNode;

    while (!isEmpty(queue)) {
        recycleNode = queue_dequeue(queue);
        free(recycleNode);
    }
    free(queue);
}

int queue_enqueue(queue_* queue , queueNode_* queueNode) {
    if ((queue == NULL) || (queueNode == NULL)) {
        return -1;
    }

    if (isEmpty(queue)) {
        queue -> head = queueNode;
    }
    else {
        queue -> tail -> next = queueNode;
    }

    queue -> tail = queueNode;
    queue -> size++;

    return 0;
}

queueNode_* queue_dequeue(queue_* queue) {
    queueNode_* queueNode;

    if (isEmpty(queue)) {
        return NULL;
    }
    else {
        queueNode = queue -> head;
        queue -> head = queue -> head -> next;
        queue -> size--;
        return queueNode;
    }
}

int isEmpty(queue_* queue) {
    if (queue == NULL) {
        return 0;
    }
    if (queue -> size == 0) {
        return 1;
    } else {
        return 0;
    }
}
