#include <string.h>

#include "engine/debug.h"
#include "stdlib.h"

int main() {
    ChessPosition cp = alloca(sizeof(struct chess_position));
    init_position(cp);
    apply_move(cp, (Move){
                   .move_type = CAPTURE,
                   .capture = {
                       .from = from_position("E2"),
                       .to = from_position("E6")
                   }
               });
    print_position(cp);

    MoveQueue q = alloca(sizeof(struct move_queue));
    init_move_queue(q);

    cp->side_to_move = WHITE;
    gen_pseudo_legal_moves(cp, q);


    for (struct move_cel *p = q->sent->next; p != q->sent; p = p->next) {
        ChessPosition c_cp = alloca(sizeof(struct chess_position));
        memcpy(c_cp, cp, sizeof(struct chess_position));
        apply_move(c_cp, p->mv);
        print_position(c_cp);
    }
}
