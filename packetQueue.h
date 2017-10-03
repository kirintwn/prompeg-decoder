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
    struct queueNode_ *next;
} queueNode_;

typedef struct queue_ {
    queueNode_ *head;
    queueNode_ *tail;
    int size;
} queue_;

queue_ *queue_construct();
void queue_destruct(queue_ *queue);
int queue_enqueue(queue_ *queue , queueNode_ *queueNode);
queueNode_ *queue_dequeue(queue_ *queue);
int isEmpty(queue_ *queue);

queue_ *queue_construct() {
    queue_ *queue = (queue_*) malloc(sizeof (queue_));
    if(queue == NULL)
        return NULL;
    else {
        queue -> head = NULL;
        queue -> tail = NULL;
        queue -> size = 0;
    }
    return queue;
}

void queue_destruct(queue_ *queue) {
    queueNode_ *recycleNode;

    while (!isEmpty(queue)) {
        recycleNode = queue_dequeue(queue);
        free(recycleNode);
    }
    free(queue);
}

int queue_enqueue(queue_ *queue , queueNode_ *queueNode) {
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

queueNode_ *queue_dequeue(queue_ *queue) {
    queueNode_ *queueNode;

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

int isEmpty(queue_ *queue) {
    if (queue == NULL) {
        return 0;
    }
    if (queue -> size == 0) {
        return 1;
    } else {
        return 0;
    }
}

int newNodeToEmptyQueue(queue_ *emptyQueue , int quantity) {
    if(emptyQueue == NULL) {
        exit(1);
    }
    else {
        for (int i = 0 ; i < quantity ; i++) {
            queueNode_ *queueNode = (queueNode_*) malloc(2048 * sizeof(uint8_t));
            queueNode -> data.size = 2048;
            queueNode -> data.used = 0;
            queueNode -> next = NULL;
            queue_enqueue(emptyQueue , queueNode);
        }
        return 0;
    }
}

int storePacketToQueue(queue_ *dstQueue , queue_ *emptyQueue , const void *buffer, size_t length) {
    if ((dstQueue == NULL) || (emptyQueue == NULL) || (buffer == NULL)) {
        exit(1);
    }
    else {
        queueNode_ *queueNode = queue_dequeue(emptyQueue);
        queueNode -> next = NULL;
        queueNode -> data.used = length;
        memcpy(queueNode -> data.packetData , buffer , length);
        queue_enqueue(dstQueue , queueNode);
        return 0;
    }
}

int freeNodeToEmptyQueue(queue_ *emptyQueue , queueNode_ *queueNode) {
    if ((emptyQueue == NULL) || (queueNode == NULL)) {
        exit(1);
    }
    else {
        queueNode -> next = NULL;
        queueNode -> data.used = 0;
        queue_enqueue(emptyQueue , queueNode);
        return 0;
    }
}
