#pragma once
#define NR_PROMOTION_TYPE 4

typedef enum piece_type {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
    NONE = 6,
} PieceType;


typedef struct capture {
    unsigned from: 6;
    unsigned to: 6;
} Capture;

typedef enum promotion_type {
    PROMOTE_BISHOP = 2,
    PROMOTE_KNIGHT = 1,
    PROMOTE_ROOK = 3,
    PROMOTE_QUEEN = 4
} PromotionType;

typedef struct promotion {
    unsigned from: 6;
    unsigned to: 6;
    PromotionType promotion_type: 3;
} Promotion;

typedef enum castle {
    CASTLE_KING,
    CASTLE_QUEEN
} Castle;

typedef struct move {
    enum {
        CASTLE,
        PROMOTION,
        CAPTURE
    } move_type: 2;

    union {
        Castle castle;
        Promotion promotion;
        Capture capture;
    };
} Move;


struct move_cel {
    Move mv;
    struct move_cel *next, *prev;
};

typedef struct move_queue {
    struct move_cel *sent;
} *MoveQueue;

void init_move_queue(MoveQueue mq);

void add_move_queue(MoveQueue mq, Move to_add);

Move pop_back_move_queue(MoveQueue mq);

Move pop_back_move_queue(MoveQueue mq);
