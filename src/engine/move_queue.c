#include <stddef.h>
#include <stdlib.h>

#include "engine.h"

void init_move_queue(MoveQueue mq) {
    mq->sent=malloc(sizeof(struct move_cel));
    mq->sent->prev=mq->sent->next=mq->sent;
}
void add_move_queue(MoveQueue mq, Move to_add) {
    struct move_cel* aux=malloc(sizeof(struct move_cel));
    aux->mv=to_add;
    aux->next=mq->sent;
    aux->prev=mq->sent->prev;
    mq->sent->next=aux;
    mq->sent->prev->next=aux;
}
Move pop_back_move_queue(MoveQueue mq) {
    struct move_cel* aux=mq->sent->prev;
    aux->prev->next=aux->next;
    aux->next->prev=aux->prev;

    Move ret=aux->mv;
    free(aux);
    return ret;
}
Move pop_front_move_queue(MoveQueue mq) {
    struct move_cel* aux=mq->sent->next;
    aux->prev->next=aux->next;
    aux->next->prev=aux->prev;

    Move ret=aux->mv;
    free(aux);
    return ret;
}