#include "engine.h"

unsigned char is_square_attacked(ChessPosition cp, char position) {
    // TO DO
    return '0';
}

void apply_move(ChessPosition cp, Move m) {
    switch (m.move_type) {
        case CAPTURE: {
            PieceType moved = cp->piece_board[m.capture.from].piece;
            PieceType captured = cp->piece_board[m.capture.to].piece;

            // Remove previous location
            cp->board[cp->side_to_move][moved] &= ~(1UL << m.capture.from);
            cp->piece_board[m.capture.from].piece = NONE;

            // Add new location
            cp->board[cp->side_to_move][moved] |= 1UL << m.capture.to;
            cp->piece_board[m.capture.to].piece = moved;
            cp->piece_board[m.capture.to].color = cp->side_to_move;
            if (captured != NONE) {
                // Remove piece from board
                cp->board[ENEMY_COLOR(cp->side_to_move)][captured] &= ~(1UL << m.capture.to);
            }
            break;
        }
        case PROMOTION: {
            PieceType moved = cp->piece_board[m.promotion.from].piece;
            PieceType captured = cp->piece_board[m.promotion.to].piece;
            PromotionType promoted = m.promotion.promotion_type;

            // Remove previous location
            cp->board[cp->side_to_move][moved] &= ~(1UL << m.promotion.from);
            cp->piece_board[m.promotion.from].piece = NONE;

            if (captured != NONE) {
                // Remove piece from board
                cp->board[ENEMY_COLOR(cp->side_to_move)][captured] &= ~(1UL << m.promotion.to);
            }

            // Add promoted piece
            cp->board[cp->side_to_move][promoted] |= 1UL << m.promotion.to;
            cp->piece_board[m.promotion.to].piece = promoted;
            cp->piece_board[m.promotion.to].color = cp->side_to_move;
            break;
        }
        case CASTLE: {
            switch (m.castle) {
                case CASTLE_KING: {
                    //TO DO
                }
                case CASTLE_QUEEN: {
                    // TO DO
                }
            }
        }
    }
    cp->side_to_move = ENEMY_COLOR(cp->side_to_move);
}

void gen_pawn_moves(ChessPosition cp, MoveQueue mq, BitBoard all_pieces, BitBoard enemy_pieces) {
    BitBoard pawns = cp->board[cp->side_to_move][PAWN];
    BitBoard empty = ~all_pieces;

    BitBoard single_push = 0;
    BitBoard double_push = 0;

    // Pawn march
    if (cp->side_to_move == WHITE) {
        single_push = (pawns << 8) & empty;
        double_push = ((single_push & 0x0000000000FF0000ULL) << 8) & empty; // from rank 2
    } else {
        single_push = (pawns >> 8) & empty;
        double_push = ((single_push & 0x0000FF0000000000ULL) >> 8) & empty; // from rank 7
    }

    BitBoard tmp = single_push;
    while (tmp) {
        uint8_t to = __builtin_ctzll(tmp);
        uint8_t from = to + (cp->side_to_move == WHITE ? -8 : 8);
        tmp &= tmp - 1; // Clear lowest bit

        // Is promotion
        if (to <= 63 && to >= 54) {
            // No none promotion
            for (PromotionType pt = 0; pt < NR_PROMOTION_TYPE; pt++) {
                add_move_queue(mq, (Move){
                                   .move_type = PROMOTION,
                                   .promotion = (Promotion){
                                       .from = from,
                                       .to = to,
                                       .promotion_type = pt
                                   }
                               });
            }
        } else {
            add_move_queue(mq, (Move){
                               .move_type = CAPTURE,
                               .capture = (Capture){
                                   .from = from,
                                   .to = to,
                               }
                           });
        }
    }
    tmp = double_push;
    while (tmp) {
        uint8_t to = __builtin_ctzll(tmp);
        uint8_t from = to + (cp->side_to_move == WHITE ? -16 : 16);
        tmp &= tmp - 1; // Clear lowest bit

        add_move_queue(mq, (Move){
                           .move_type = CAPTURE,
                           .capture = (Capture){
                               .from = from,
                               .to = to,
                           }
                       });
    }

    //Pawn capture
    BitBoard capture_right;
    BitBoard capture_left;
    if (cp->side_to_move == WHITE) {
        BitBoard non_edge_pawns = pawns & 0x7e7e7e7e7e7e7e7e;
        BitBoard left_pawns = pawns & 0x101010101010101;
        BitBoard right_pawns = pawns & 0x8080808080808080;
        capture_right = (non_edge_pawns << 7 | left_pawns << 7) & enemy_pieces;
        capture_left = (non_edge_pawns << 9 | right_pawns << 9) & enemy_pieces;
    } else {
        BitBoard non_edge_pawns = pawns & 0x7e7e7e7e7e7e7e7e;
        BitBoard left_pawns = pawns & 0x8080808080808080;
        BitBoard right_pawns = pawns & 0x101010101010101;
        capture_right = (non_edge_pawns >> 7 | left_pawns >> 7) & enemy_pieces;
        capture_left = (non_edge_pawns >> 9 | right_pawns >> 9) & enemy_pieces;
    }
    tmp = capture_left;
    while (tmp) {
        uint8_t to = __builtin_ctzll(tmp);
        uint8_t from = to + (cp->side_to_move == WHITE ? -9 : 9);
        tmp &= tmp - 1; // Clear lowest bit

        add_move_queue(mq, (Move){
                           .move_type = CAPTURE,
                           .capture = (Capture){
                               .from = from,
                               .to = to,
                           }
                       });
    }
    tmp = capture_right;
    while (tmp) {
        uint8_t to = __builtin_ctzll(tmp);
        uint8_t from = to + (cp->side_to_move == WHITE ? -7 : 7);
        tmp &= tmp - 1; // Clear lowest bit

        add_move_queue(mq, (Move){
                           .move_type = CAPTURE,
                           .capture = (Capture){
                               .from = from,
                               .to = to,
                           }
                       });
    }
}

void gen_pseudo_legal_moves(ChessPosition cp, MoveQueue ml) {
    BitBoard all_pieces = 0;
    BitBoard enemy_pieces = 0;
    for (int i = 0; i < NR_PIECE_TYPES; i++) {
        all_pieces |= cp->board[WHITE][i];
        all_pieces |= cp->board[BLACK][i];
    }
    for (int i = 0; i < NR_PIECE_TYPES; i++) {
        enemy_pieces |= cp->board[WHITE][i];
    }
    gen_pawn_moves(cp, ml, all_pieces, enemy_pieces);
}
