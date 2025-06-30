#include <stdio.h>
#include <stdlib.h>

#include "vector.h"
#include "engine/engine.h"
#include "engine/precompute/load.h"

unsigned char is_square_attacked(ChessPosition cp, uint8_t position) {
    // TO DO
    return '0';
}

void remove_piece(ChessPosition cp, Color c, PieceType pt, uint8_t sq) {
    cp->board[c][pt] &= ~(1ULL << sq);
    cp->piece_board[sq].piece = NONE;
}

void add_piece(ChessPosition cp, Color c, PieceType pt, uint8_t sq) {
    cp->board[c][pt] |= 1ULL << sq;
    cp->piece_board[sq].piece = pt;
    cp->piece_board[sq].color = c;
}

PieceType piece_from_promotion(PromotionType pt) {
    switch (pt) {
        case PROMOTE_KNIGHT:
            return KNIGHT;
        case PROMOTE_BISHOP:
            return BISHOP;
        case PROMOTE_ROOK:
            return ROOK;
        case PROMOTE_QUEEN:
            return QUEEN;
        default:
            exit(-1);
    }
}

void apply_move(ChessPosition cp, Move m) {
    switch (m.move_type) {
        case CAPTURE: {
            PieceType moved = cp->piece_board[m.capture.from].piece;
            PieceType captured = cp->piece_board[m.capture.to].piece;
            // Remove previous location
            remove_piece(cp, cp->side_to_move, moved, m.capture.from);

            if (captured != NONE) {
                // Remove piece from board
                remove_piece(cp, ENEMY_COLOR(cp->side_to_move), captured, m.capture.to);
            }

            // Add new location
            add_piece(cp, cp->side_to_move, moved, m.capture.to);

            // Update castling rights
            if (moved == ROOK &&
                m.capture.from == (cp->side_to_move == WHITE ? 7 : 63) ||
                moved == KING)
                cp->castle_laws[cp->side_to_move] &= 0b10;
            if (moved == ROOK &&
                m.capture.from == (cp->side_to_move == WHITE ? 0 : 56) ||
                moved == KING)
                cp->castle_laws[cp->side_to_move] &= 0b01;

            // Update en_passant
            if (moved == PAWN && abs(m.capture.to - m.capture.from) == 16) {
                cp->en_passant_square =
                        1ULL << ((cp->side_to_move == WHITE ? 8 : -8) + m.capture.from);
            } else {
                cp->en_passant_square = 0ULL;
            }
            break;
        }
        case PROMOTION: {
            PieceType moved = cp->piece_board[m.promotion.from].piece;
            PieceType captured = cp->piece_board[m.promotion.to].piece;
            PromotionType promoted = m.promotion.promotion_type;

            // Remove previous location
            remove_piece(cp, cp->side_to_move, moved, m.promotion.from);

            if (captured != NONE) {
                // Remove piece from board
                remove_piece(cp, ENEMY_COLOR(cp->side_to_move), captured, m.promotion.to);
            }

            // Add promoted piece
            add_piece(cp, cp->side_to_move, piece_from_promotion(promoted),
                      m.promotion.to);
            break;
        }
        case CASTLE: {
            switch (m.castle) {
                case CASTLE_KING: {
                    // Remove king
                    remove_piece(cp, cp->side_to_move, KING, 4);

                    // Add king
                    add_piece(cp, cp->side_to_move, KING, 6);

                    // Remove rook
                    remove_piece(cp, cp->side_to_move, ROOK, 7);

                    // Add rook
                    add_piece(cp, cp->side_to_move, ROOK, 5);

                    break;
                }
                case CASTLE_QUEEN: {
                    // Remove king
                    remove_piece(cp, cp->side_to_move, KING, 4);

                    // Add king
                    add_piece(cp, cp->side_to_move, KING, 2);

                    // Remove rook
                    remove_piece(cp, cp->side_to_move, ROOK, 0);

                    // Add rook
                    add_piece(cp, cp->side_to_move, ROOK, 3);

                    break;
                }
            }
            cp->castle_laws[cp->side_to_move] = CANNOT_CASTLE;
            break;
        }
        default: {
            exit(-1);
        }
    }
    cp->side_to_move = ENEMY_COLOR(cp->side_to_move);
}

void gen_pawn_moves(ChessPosition cp, Vector v, BitBoard all_pieces,
                    BitBoard enemy_pieces) {
    BitBoard pawns = cp->board[cp->side_to_move][PAWN];
    BitBoard empty = ~all_pieces;

    BitBoard single_push = 0;
    BitBoard double_push = 0;

    // Pawn march
    if (cp->side_to_move == WHITE) {
        single_push = (pawns << 8) & empty;
        double_push =
                ((single_push & 0x0000000000FF0000ULL) << 8) & empty; // from rank 2
    } else {
        single_push = (pawns >> 8) & empty;
        double_push =
                ((single_push & 0x0000FF0000000000ULL) >> 8) & empty; // from rank 7
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
                push_vector(
                    v, &(Move){
                        .move_type = PROMOTION,
                        .promotion = (Promotion){
                            .from = from, .to = to, .promotion_type = pt
                        }
                    });
            }
        } else {
            push_vector(v, &(Move){
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

        push_vector(v, &(Move){
                        .move_type = CAPTURE,
                        .capture = (Capture){
                            .from = from,
                            .to = to,
                        }
                    });
    }

    // Pawn capture
    BitBoard capture_right;
    BitBoard capture_left;
    if (cp->side_to_move == WHITE) {
        BitBoard non_edge_pawns = pawns & 0x7e7e7e7e7e7e7e7e;
        BitBoard left_pawns = pawns & 0x101010101010101;
        BitBoard right_pawns = pawns & 0x8080808080808080;
        capture_right = (non_edge_pawns << 9 | left_pawns << 9) &
                        (enemy_pieces | cp->en_passant_square);
        capture_left = (non_edge_pawns << 7 | right_pawns << 7) &
                       (enemy_pieces | cp->en_passant_square);
    } else {
        BitBoard non_edge_pawns = pawns & 0x7e7e7e7e7e7e7e7e;
        BitBoard left_pawns = pawns & 0x8080808080808080;
        BitBoard right_pawns = pawns & 0x101010101010101;
        capture_right = (non_edge_pawns >> 9 | left_pawns >> 9) &
                        (enemy_pieces | cp->en_passant_square);
        capture_left = (non_edge_pawns >> 7 | right_pawns >> 7) &
                       (enemy_pieces | cp->en_passant_square);
    }
    tmp = capture_left;
    while (tmp) {
        uint8_t to = __builtin_ctzll(tmp);
        uint8_t from = to + (cp->side_to_move == WHITE ? -7 : 7);
        tmp &= tmp - 1; // Clear lowest bit

        // Is promotion
        if (to <= 63 && to >= 54) {
            // No none promotion
            for (PromotionType pt = 0; pt < NR_PROMOTION_TYPE; pt++) {
                push_vector(
                    v, &(Move){
                        .move_type = PROMOTION,
                        .promotion = (Promotion){
                            .from = from, .to = to, .promotion_type = pt
                        }
                    });
            }
        } else {
            push_vector(v, &(Move){
                            .move_type = CAPTURE,
                            .capture = (Capture){
                                .from = from,
                                .to = to,
                            }
                        });
        }
    }

    tmp = capture_right;
    while (tmp) {
        uint8_t to = __builtin_ctzll(tmp);
        uint8_t from = to + (cp->side_to_move == WHITE ? -9 : 9);
        tmp &= tmp - 1; // Clear lowest bit

        // Is promotion
        if (to <= 63 && to >= 54) {
            // No none promotion
            for (PromotionType pt = 0; pt < NR_PROMOTION_TYPE; pt++) {
                push_vector(
                    v, &(Move){
                        .move_type = PROMOTION,
                        .promotion = (Promotion){
                            .from = from, .to = to, .promotion_type = pt
                        }
                    });
            }
        } else {
            push_vector(v, &(Move){
                            .move_type = CAPTURE,
                            .capture = (Capture){
                                .from = from,
                                .to = to,
                            }
                        });
        }
    }
}

void gen_king_moves(ChessPosition cp, Vector v, BitBoard all_pieces,
                    BitBoard ally_pieces) {
    uint8_t king_location = __builtin_ctzll(cp->board[cp->side_to_move][KING]);
    BitBoard tmp = king_look_up_table[king_location] & ~(ally_pieces);
    while (tmp) {
        uint8_t to = __builtin_ctzll(tmp);
        tmp &= tmp - 1; // Clear lowest bit

        push_vector(v, &(Move){
                        .move_type = CAPTURE,
                        .capture = (Capture){
                            .from = king_location,
                            .to = to,
                        }
                    });
    }
    // Check rights
    if ((cp->castle_laws[cp->side_to_move] & 1) != 0) {
        if (cp->side_to_move == WHITE) {
            // Check empty space and rook is in position
            if ((all_pieces & 0x60) == 0 && (cp->board[WHITE][ROOK] & 0x80) != 0)
                if (is_square_attacked(cp, 4) == 0 && is_square_attacked(cp, 5) == 0 &&
                    is_square_attacked(cp, 6) == 0)
                    push_vector(v,
                                &(Move){.move_type = CASTLE, .castle = CASTLE_KING});
        } else {
            // Check empty space and rook is in position
            if ((all_pieces & 0x6000000000000000) == 0 &&
                (cp->board[BLACK][ROOK] & 0x8000000000000000) != 0)
                if (is_square_attacked(cp, 60) == 0 &&
                    is_square_attacked(cp, 61) == 0 && is_square_attacked(cp, 62) == 0)
                    push_vector(v,
                                &(Move){.move_type = CASTLE, .castle = CASTLE_KING});
        }
    }
    if ((cp->castle_laws[cp->side_to_move] & 2) != 0) {
        if (cp->side_to_move == WHITE) {
            // Check empty space and rook is in position
            if ((all_pieces & 0xc) == 0 && (cp->board[WHITE][ROOK] & 0x1) != 0)
                if (is_square_attacked(cp, 2) == 0 && is_square_attacked(cp, 3) == 0 &&
                    is_square_attacked(cp, 4) == 0)
                    push_vector(v,
                                &(Move){.move_type = CASTLE, .castle = CASTLE_QUEEN});
        } else {
            // Check empty space and rook is in position
            if ((all_pieces & 0xc00000000000000) == 0 &&
                (cp->board[BLACK][ROOK] & 0x100000000000000) != 0)
                if (is_square_attacked(cp, 58) == 0 &&
                    is_square_attacked(cp, 59) == 0 && is_square_attacked(cp, 60) == 0)
                    push_vector(v,
                                &(Move){.move_type = CASTLE, .castle = CASTLE_QUEEN});
        }
    }
}

void gen_knight_moves(ChessPosition cp, Vector v, BitBoard ally_pieces) {
    BitBoard tmp = cp->board[cp->side_to_move][KNIGHT];
    while (tmp) {
        uint8_t from = __builtin_ctzll(tmp);
        tmp &= tmp - 1; // Clear lowest bit

        BitBoard attack = knight_look_up_table[from] & ~(ally_pieces);
        while (attack) {
            uint8_t to = __builtin_ctzll(attack);
            attack &= attack - 1;

            push_vector(v, &(Move){
                            .move_type = CAPTURE,
                            .capture = (Capture){
                                .from = from,
                                .to = to,
                            }
                        });
        }
    }
}

void gen_bishop_moves() {
}

void gen_rook_moves() {
}

void gen_queen_moves() {
}

void gen_pseudo_legal_moves(ChessPosition cp, Vector v) {
    BitBoard all_pieces = 0;
    BitBoard enemy_pieces = 0;
    for (int i = 0; i < NR_PIECE_TYPES; i++) {
        all_pieces |= cp->board[WHITE][i];
        all_pieces |= cp->board[BLACK][i];
    }
    for (int i = 0; i < NR_PIECE_TYPES; i++) {
        enemy_pieces |= cp->board[ENEMY_COLOR(cp->side_to_move)][i];
    }
    gen_pawn_moves(cp, v, all_pieces, enemy_pieces);
    gen_king_moves(cp, v, all_pieces, all_pieces & ~(enemy_pieces));
    gen_knight_moves(cp, v, all_pieces & ~(enemy_pieces));
}
