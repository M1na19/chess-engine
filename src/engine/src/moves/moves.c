#include "debug.h"
#include "engine.h"
#include "precompute/load.h"
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char is_square_attacked(ChessPosition cp, Color by, uint8_t position) {
  BitBoard location = 1ULL << position;
  // 1. Check if pawns attack
  BitBoard pawns = cp.board[by][PAWN];
  BitBoard capture;
  if (by == WHITE) {
    BitBoard non_edge_pawns = pawns & 0x7e7e7e7e7e7e7e7e;
    BitBoard left_pawns = pawns & 0x101010101010101;
    BitBoard right_pawns = pawns & 0x8080808080808080;
    capture = (non_edge_pawns << 9 | left_pawns << 9) |
              (non_edge_pawns << 7 | right_pawns << 7);

  } else {
    BitBoard non_edge_pawns = pawns & 0x7e7e7e7e7e7e7e7e;
    BitBoard left_pawns = pawns & 0x8080808080808080;
    BitBoard right_pawns = pawns & 0x101010101010101;
    capture = (non_edge_pawns >> 9 | left_pawns >> 9) |
              (non_edge_pawns >> 7 | right_pawns >> 7);
  }
  if (capture & location)
    return 1;

  // 2. Check if king attacks
  BitBoard king = cp.board[by][KING];
  if (king_look_up_table[__builtin_ctzll(king)] & location)
    return 1;

  // 3. Check if knights attack
  BitBoard knights = cp.board[by][KNIGHT];
  while (knights)
    if (knight_look_up_table[__builtin_ctzll(knights)] & location)
      return 1;
    else
      knights &= knights - 1;

  // Compute all pieces bitboard
  BitBoard all_pieces = 0;
  for (int i = 0; i < NR_PIECE_TYPES; i++) {
    all_pieces |= cp.board[WHITE][i] | cp.board[BLACK][i];
  }

  // 4. Check if bishops or queens attack
  BitBoard bishops = cp.board[by][BISHOP] | cp.board[by][QUEEN];
  while (bishops) {
    uint8_t from = __builtin_ctzll(bishops);
    bishops &= bishops - 1;
    if (get_attack_magic_vec(bishop_look_up_table[from], all_pieces) & location)
      return 1;
  }

  // 5.Check if rooks or queens attack
  BitBoard rooks = cp.board[by][ROOK] | cp.board[by][QUEEN];
  while (rooks) {
    uint8_t from = __builtin_ctzll(rooks);
    rooks &= rooks - 1;
    if (get_attack_magic_vec(rook_look_up_table[from], all_pieces) & location)
      return 1;
  }

  return 0;
}

void remove_piece(ChessPosition *cp, Color c, PieceType pt, uint8_t sq) {
  cp->board[c][pt] &= ~(1ULL << sq);
  cp->piece_board[sq].piece = NONE;
}

void add_piece(ChessPosition *cp, Color c, PieceType pt, uint8_t sq) {
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

UndoMove apply_move(ChessPosition *cp, Move m) {
  UndoMove u;
  u.castle_laws[WHITE] = cp->castle_laws[WHITE];
  u.castle_laws[BLACK] = cp->castle_laws[BLACK];
  u.prev_en_passant = cp->en_passant;
  u.prev_half_move = cp->half_move_count;

  // Set default not possible
  cp->en_passant.en_passant_status = EN_PASSANT_NOT_POSSIBLE;

  switch (m.move_type) {
  case CAPTURE: {
    u.changes[0] = (struct undo_piece){.from.sq = m.capture.to,
                                       .to = m.capture.from,
                                       .is_promotion = 0,
                                       .on_table = 1};
    u.nr_changes = 0;

    PieceType moved = cp->piece_board[m.capture.from].piece;
    PieceType captured = cp->piece_board[m.capture.to].piece;

    // Remove previous location
    remove_piece(cp, cp->side_to_move, moved, m.capture.from);

    if (captured != NONE) {
      // Remove piece from board
      remove_piece(cp, ENEMY_COLOR(cp->side_to_move), captured, m.capture.to);
      u.changes[1] = (struct undo_piece){.from.piece = captured,
                                         .to = m.capture.to,
                                         .on_table = 0,
                                         .is_promotion = 0};
      u.nr_changes = 1;
    } else if (moved == PAWN && m.capture.is_en_passant == 1) {
      uint8_t target = m.capture.to + (cp->side_to_move == WHITE ? -8 : 8);
      remove_piece(cp, ENEMY_COLOR(cp->side_to_move), PAWN, target);
      u.changes[1] = (struct undo_piece){
          .from.piece = PAWN, .to = target, .on_table = 0, .is_promotion = 0};
      u.nr_changes = 1;
    }
    // Add new location
    add_piece(cp, cp->side_to_move, moved, m.capture.to);

    if (captured != NONE || moved == PAWN) {
      cp->half_move_count = 0;
    } else {
      cp->half_move_count++;
    }

    // Update castling rights

    // If rook is moved from initial square
    if ((moved == ROOK &&
         m.capture.from == (cp->side_to_move == WHITE ? 7 : 63)) ||
        moved == KING)
      cp->castle_laws[cp->side_to_move] &= 0b10;
    if ((moved == ROOK &&
         m.capture.from == (cp->side_to_move == WHITE ? 0 : 56)) ||
        moved == KING)
      cp->castle_laws[cp->side_to_move] &= 0b01;

    // If rook is captured on initial square
    if (captured == ROOK &&
        m.capture.to == (cp->side_to_move == WHITE ? 56 : 0)) {
      cp->castle_laws[ENEMY_COLOR(cp->side_to_move)] &= 0b01;
    }
    if (captured == ROOK &&
        m.capture.to == (cp->side_to_move == WHITE ? 63 : 7)) {
      cp->castle_laws[ENEMY_COLOR(cp->side_to_move)] &= 0b10;
    }

    // Update en_passant
    if (moved == PAWN && abs(m.capture.to - m.capture.from) == 16) {
      cp->en_passant.en_passant_status = EN_PASSANT_POSSIBLE;
      cp->en_passant.en_passant_square =
          (cp->side_to_move == WHITE ? 8 : -8) + m.capture.from;
    }
    break;
  }
  case PROMOTION: {
    u.changes[0] = (struct undo_piece){.from.sq = m.capture.to,
                                       .to = m.promotion.from,
                                       .is_promotion = 1,
                                       .on_table = 1};
    u.nr_changes = 0;

    PieceType moved = cp->piece_board[m.promotion.from].piece;
    PieceType captured = cp->piece_board[m.promotion.to].piece;
    PromotionType promoted = m.promotion.promotion_type;

    // Remove previous location
    remove_piece(cp, cp->side_to_move, moved, m.promotion.from);

    if (captured != NONE) {
      // Remove piece from board
      remove_piece(cp, ENEMY_COLOR(cp->side_to_move), captured, m.promotion.to);

      u.changes[1] = (struct undo_piece){.from.piece = captured,
                                         .to = m.promotion.to,
                                         .is_promotion = 0,
                                         .on_table = 0};
      u.nr_changes = 1;
    }

    // Add promoted piece
    add_piece(cp, cp->side_to_move, piece_from_promotion(promoted),
              m.promotion.to);
    cp->half_move_count = 0;
    break;
  }
  case CASTLE: {
    cp->half_move_count++;
    u.nr_changes = 1;

    switch (m.castle) {
    case CASTLE_KING: {
      if (cp->side_to_move == WHITE) {
        // Remove king
        remove_piece(cp, cp->side_to_move, KING, 4);

        // Add king
        add_piece(cp, cp->side_to_move, KING, 6);

        u.changes[0] = (struct undo_piece){
            .from.sq = 6, .to = 4, .on_table = 1, .is_promotion = 0};

        // Remove rook
        remove_piece(cp, cp->side_to_move, ROOK, 7);

        // Add rook
        add_piece(cp, cp->side_to_move, ROOK, 5);

        u.changes[1] = (struct undo_piece){
            .from.sq = 5, .to = 7, .on_table = 1, .is_promotion = 0};
      } else {
        // Remove king
        remove_piece(cp, cp->side_to_move, KING, 60);

        // Add king
        add_piece(cp, cp->side_to_move, KING, 62);

        u.changes[0] = (struct undo_piece){
            .from.sq = 62, .to = 60, .on_table = 1, .is_promotion = 0};

        // Remove rook
        remove_piece(cp, cp->side_to_move, ROOK, 63);

        // Add rook
        add_piece(cp, cp->side_to_move, ROOK, 61);

        u.changes[1] = (struct undo_piece){
            .from.sq = 61, .to = 63, .on_table = 1, .is_promotion = 0};
      }
      break;
    }
    case CASTLE_QUEEN: {
      if (cp->side_to_move == WHITE) {
        // Remove king
        remove_piece(cp, cp->side_to_move, KING, 4);

        // Add king
        add_piece(cp, cp->side_to_move, KING, 2);

        u.changes[0] = (struct undo_piece){
            .from.sq = 2, .to = 4, .on_table = 1, .is_promotion = 0};

        // Remove rook
        remove_piece(cp, cp->side_to_move, ROOK, 0);

        // Add rook
        add_piece(cp, cp->side_to_move, ROOK, 3);

        u.changes[1] = (struct undo_piece){
            .from.sq = 3, .to = 0, .on_table = 1, .is_promotion = 0};
      } else {
        // Remove king
        remove_piece(cp, cp->side_to_move, KING, 60);

        // Add king
        add_piece(cp, cp->side_to_move, KING, 58);

        u.changes[0] = (struct undo_piece){
            .from.sq = 58, .to = 60, .on_table = 1, .is_promotion = 0};

        // Remove rook
        remove_piece(cp, cp->side_to_move, ROOK, 56);

        // Add rook
        add_piece(cp, cp->side_to_move, ROOK, 59);

        u.changes[1] = (struct undo_piece){
            .from.sq = 59, .to = 56, .on_table = 1, .is_promotion = 0};
      }
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
  if (cp->side_to_move == BLACK) {
    cp->move_count++;
  }
  cp->side_to_move = ENEMY_COLOR(cp->side_to_move);
  return u;
}
void undo_move(ChessPosition *cp, UndoMove u) {
  if (cp->side_to_move == WHITE) {
    cp->move_count--;
  }
  cp->side_to_move = ENEMY_COLOR(cp->side_to_move);
  cp->half_move_count = u.prev_half_move;
  cp->en_passant = u.prev_en_passant;
  cp->castle_laws[WHITE] = u.castle_laws[WHITE];
  cp->castle_laws[BLACK] = u.castle_laws[BLACK];

  // Apply changes
  for (int i = 0; i <= u.nr_changes; i++) {
    struct undo_piece up = u.changes[i];
    if (up.is_promotion) {
      // Remove promoted piece
      remove_piece(cp, cp->side_to_move, cp->piece_board[up.from.sq].piece,
                   up.from.sq);

      // Add pawn that promoted
      add_piece(cp, cp->side_to_move, PAWN, up.to);
    } else {
      if (up.on_table == 0) {
        // Add piece that was captured
        add_piece(cp, ENEMY_COLOR(cp->side_to_move), up.from.piece, up.to);
      } else {
        PieceType pt = cp->piece_board[up.from.sq].piece;
        // Move piece back
        remove_piece(cp, cp->side_to_move, pt, up.from.sq);
        add_piece(cp, cp->side_to_move, pt, up.to);
      }
    }
  }
}
VectorMove gen_pawn_moves(ChessPosition cp, BitBoard all_pieces,
                          BitBoard enemy_pieces) {
  // Move vector
  VectorMove v = vector_init(sizeof(Move), 1);

  BitBoard pawns = cp.board[cp.side_to_move][PAWN];
  BitBoard empty = ~all_pieces;

  BitBoard single_push = 0;
  BitBoard double_push = 0;

  // Pawn march
  if (cp.side_to_move == WHITE) {
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
    uint8_t from = to + (cp.side_to_move == WHITE ? -8 : 8);
    tmp &= tmp - 1; // Clear lowest bit

    // Is promotion
    if (cp.side_to_move == WHITE ? to <= 63 && to >= 56 : to >= 0 && to <= 7) {
      // No none promotion
      for (PromotionType pt = 0; pt < NR_PROMOTION_TYPE; pt++) {
        vector_push(&v,
                    &(Move){.move_type = PROMOTION,
                            .promotion = (Promotion){
                                .from = from, .to = to, .promotion_type = pt}});
      }
    } else {
      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,
                              }});
    }
  }
  tmp = double_push;
  while (tmp) {
    uint8_t to = __builtin_ctzll(tmp);
    uint8_t from = to + (cp.side_to_move == WHITE ? -16 : 16);
    tmp &= tmp - 1; // Clear lowest bit

    vector_push(&v, &(Move){.move_type = CAPTURE,
                            .capture = (Capture){
                                .from = from,
                                .to = to,
                            }});
  }

  // Pawn capture
  BitBoard capture_right;
  BitBoard capture_left;
  if (cp.side_to_move == WHITE) {
    BitBoard non_edge_pawns = pawns & 0x7e7e7e7e7e7e7e7e;
    BitBoard left_pawns = pawns & 0x101010101010101;
    BitBoard right_pawns = pawns & 0x8080808080808080;
    capture_right =
        (non_edge_pawns << 9 | left_pawns << 9) &
        (enemy_pieces | (cp.en_passant.en_passant_status == EN_PASSANT_POSSIBLE
                             ? 1ULL << cp.en_passant.en_passant_square
                             : 0));
    capture_left =
        (non_edge_pawns << 7 | right_pawns << 7) &
        (enemy_pieces | (cp.en_passant.en_passant_status == EN_PASSANT_POSSIBLE
                             ? 1ULL << cp.en_passant.en_passant_square
                             : 0));
  } else {
    BitBoard non_edge_pawns = pawns & 0x7e7e7e7e7e7e7e7e;
    BitBoard left_pawns = pawns & 0x8080808080808080;
    BitBoard right_pawns = pawns & 0x101010101010101;
    capture_right =
        (non_edge_pawns >> 9 | left_pawns >> 9) &
        (enemy_pieces | (cp.en_passant.en_passant_status == EN_PASSANT_POSSIBLE
                             ? 1ULL << cp.en_passant.en_passant_square
                             : 0));
    capture_left =
        (non_edge_pawns >> 7 | right_pawns >> 7) &
        (enemy_pieces | (cp.en_passant.en_passant_status == EN_PASSANT_POSSIBLE
                             ? 1ULL << cp.en_passant.en_passant_square
                             : 0));
  }
  tmp = capture_left;
  while (tmp) {
    uint8_t to = __builtin_ctzll(tmp);
    uint8_t from = to + (cp.side_to_move == WHITE ? -7 : 7);
    tmp &= tmp - 1; // Clear lowest bit

    // Is promotion
    if (cp.side_to_move == WHITE ? to <= 63 && to >= 56 : to >= 0 && to <= 7) {
      // No none promotion
      for (PromotionType pt = 0; pt < NR_PROMOTION_TYPE; pt++) {
        vector_push(&v,
                    &(Move){.move_type = PROMOTION,
                            .promotion = (Promotion){
                                .from = from, .to = to, .promotion_type = pt}});
      }
    }
    // Is en_passant
    else if (cp.en_passant.en_passant_status == EN_PASSANT_POSSIBLE &&
             to == cp.en_passant.en_passant_square) {
      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,
                                  .is_en_passant = 1,
                              }});
    } else {
      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,

                              }});
    }
  }

  tmp = capture_right;
  while (tmp) {
    uint8_t to = __builtin_ctzll(tmp);
    uint8_t from = to + (cp.side_to_move == WHITE ? -9 : 9);
    tmp &= tmp - 1; // Clear lowest bit

    // Is promotion
    if (cp.side_to_move == WHITE ? to <= 63 && to >= 56 : to >= 0 && to <= 7) {
      // No none promotion
      for (PromotionType pt = 0; pt < NR_PROMOTION_TYPE; pt++) {
        vector_push(&v,
                    &(Move){.move_type = PROMOTION,
                            .promotion = (Promotion){
                                .from = from, .to = to, .promotion_type = pt}});
      }
    }
    // Is en_passant
    else if (cp.en_passant.en_passant_status == EN_PASSANT_POSSIBLE &&
             to == cp.en_passant.en_passant_square) {
      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,
                                  .is_en_passant = 1,
                              }});
    } else {
      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,
                              }});
    }
  }
  return v;
}

VectorMove gen_king_moves(ChessPosition cp, BitBoard all_pieces,
                          BitBoard ally_pieces) {
  // Move vector
  VectorMove v = vector_init(sizeof(Move), 1);

  uint8_t king_location = __builtin_ctzll(cp.board[cp.side_to_move][KING]);
  BitBoard tmp = king_look_up_table[king_location] & ~(ally_pieces);
  while (tmp) {
    uint8_t to = __builtin_ctzll(tmp);
    tmp &= tmp - 1; // Clear lowest bit

    vector_push(&v, &(Move){.move_type = CAPTURE,
                            .capture = (Capture){
                                .from = king_location,
                                .to = to,
                            }});
  }
  // Check rights
  if ((cp.castle_laws[cp.side_to_move] & 1) != 0) {
    if (cp.side_to_move == WHITE) {
      // Check empty space and rook is in position
      if ((all_pieces & 0x60) == 0 && (cp.board[WHITE][ROOK] & 0x80) != 0)
        if (is_square_attacked(cp, BLACK, 4) == 0 &&
            is_square_attacked(cp, BLACK, 5) == 0 &&
            is_square_attacked(cp, BLACK, 6) == 0)
          vector_push(&v, &(Move){.move_type = CASTLE, .castle = CASTLE_KING});
    } else {
      // Check empty space and rook is in position
      if ((all_pieces & 0x6000000000000000) == 0 &&
          (cp.board[BLACK][ROOK] & 0x8000000000000000) != 0)
        if (is_square_attacked(cp, WHITE, 60) == 0 &&
            is_square_attacked(cp, WHITE, 61) == 0 &&
            is_square_attacked(cp, WHITE, 62) == 0)
          vector_push(&v, &(Move){.move_type = CASTLE, .castle = CASTLE_KING});
    }
  }
  if ((cp.castle_laws[cp.side_to_move] & 2) != 0) {
    if (cp.side_to_move == WHITE) {
      // Check empty space and rook is in position
      if ((all_pieces & 0xe) == 0 && (cp.board[WHITE][ROOK] & 0x1) != 0)
        if (is_square_attacked(cp, BLACK, 2) == 0 &&
            is_square_attacked(cp, BLACK, 3) == 0 &&
            is_square_attacked(cp, BLACK, 4) == 0)
          vector_push(&v, &(Move){.move_type = CASTLE, .castle = CASTLE_QUEEN});
    } else {
      // Check empty space and rook is in position
      if ((all_pieces & 0xe00000000000000) == 0 &&
          (cp.board[BLACK][ROOK] & 0x100000000000000) != 0)
        if (is_square_attacked(cp, WHITE, 58) == 0 &&
            is_square_attacked(cp, WHITE, 59) == 0 &&
            is_square_attacked(cp, WHITE, 60) == 0)
          vector_push(&v, &(Move){.move_type = CASTLE, .castle = CASTLE_QUEEN});
    }
  }
  return v;
}

VectorMove gen_knight_moves(ChessPosition cp, BitBoard ally_pieces) {
  // Move vector
  VectorMove v = vector_init(sizeof(Move), 1);

  BitBoard tmp = cp.board[cp.side_to_move][KNIGHT];
  while (tmp) {
    uint8_t from = __builtin_ctzll(tmp);
    tmp &= tmp - 1; // Clear lowest bit

    BitBoard attack = knight_look_up_table[from] & ~(ally_pieces);
    while (attack) {
      uint8_t to = __builtin_ctzll(attack);
      attack &= attack - 1;

      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,
                              }});
    }
  }
  return v;
}
VectorMove gen_bishop_moves(ChessPosition cp, BitBoard ally_pieces,
                            BitBoard enemy_pieces) {
  // Move vector
  VectorMove v = vector_init(sizeof(Move), 1);

  BitBoard tmp = cp.board[cp.side_to_move][BISHOP];
  while (tmp) {
    uint8_t from = __builtin_ctzll(tmp);
    tmp &= tmp - 1; // Clear lowest bit

    BitBoard attack = get_attack_magic_vec(bishop_look_up_table[from],
                                           (enemy_pieces | ally_pieces)) &
                      ~(ally_pieces);
    while (attack) {
      uint8_t to = __builtin_ctzll(attack);
      attack &= attack - 1;

      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,
                              }});
    }
  }
  return v;
}

VectorMove gen_rook_moves(ChessPosition cp, BitBoard ally_pieces,
                          BitBoard enemy_pieces) {
  // Move vector
  VectorMove v = vector_init(sizeof(Move), 1);

  BitBoard tmp = cp.board[cp.side_to_move][ROOK];
  while (tmp) {
    uint8_t from = __builtin_ctzll(tmp);
    tmp &= tmp - 1; // Clear lowest bit

    BitBoard attack = get_attack_magic_vec(rook_look_up_table[from],
                                           (enemy_pieces | ally_pieces)) &
                      ~(ally_pieces);
    while (attack) {
      uint8_t to = __builtin_ctzll(attack);
      attack &= attack - 1;

      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,
                              }});
    }
  }
  return v;
}

VectorMove gen_queen_moves(ChessPosition cp, BitBoard ally_pieces,
                           BitBoard enemy_pieces) {
  // Move vector
  VectorMove v = vector_init(sizeof(Move), 1);

  BitBoard tmp = cp.board[cp.side_to_move][QUEEN];
  while (tmp) {
    uint8_t from = __builtin_ctzll(tmp);
    tmp &= tmp - 1; // Clear lowest bit

    BitBoard attack_bishop =
        get_attack_magic_vec(bishop_look_up_table[from],
                             (enemy_pieces | ally_pieces)) &
        ~(ally_pieces);
    BitBoard attack_rook = get_attack_magic_vec(rook_look_up_table[from],
                                                (enemy_pieces | ally_pieces)) &
                           ~(ally_pieces);

    BitBoard attack = attack_rook | attack_bishop;
    while (attack) {
      uint8_t to = __builtin_ctzll(attack);
      attack &= attack - 1;

      vector_push(&v, &(Move){.move_type = CAPTURE,
                              .capture = (Capture){
                                  .from = from,
                                  .to = to,
                              }});
    }
  }
  return v;
}

VectorMove gen_pseudo_legal_moves(ChessPosition cp) {
  BitBoard ally_pieces = 0;
  BitBoard enemy_pieces = 0;
  for (int i = 0; i < NR_PIECE_TYPES; i++) {
    ally_pieces |= cp.board[cp.side_to_move][i];
    enemy_pieces |= cp.board[ENEMY_COLOR(cp.side_to_move)][i];
  }
  VectorMove v = gen_pawn_moves(cp, ally_pieces | enemy_pieces, enemy_pieces);

  vector_consume(&v,
                 gen_king_moves(cp, ally_pieces | enemy_pieces, ally_pieces));

  vector_consume(&v, gen_knight_moves(cp, ally_pieces));

  vector_consume(&v, gen_bishop_moves(cp, ally_pieces, enemy_pieces));

  vector_consume(&v, gen_rook_moves(cp, ally_pieces, enemy_pieces));

  vector_consume(&v, gen_queen_moves(cp, ally_pieces, enemy_pieces));
  return v;
}

VectorMove gen_legal_moves(ChessPosition cp) {
  VectorMove v = gen_pseudo_legal_moves(cp);

  VectorMove legal = vector_init(sizeof(Move), v.count);

  for (size_t i = 0; i < v.count; i++) {
    UndoMove um = apply_move(&cp, VALUE(Move, vector_get(v, i)));
    if (!is_square_attacked(
            cp, cp.side_to_move,
            __builtin_ctzll(cp.board[ENEMY_COLOR(cp.side_to_move)][KING]))) {
      vector_push(&legal, vector_get(v, i));
    }
    undo_move(&cp, um);
  }
  vector_free(v);
  return legal;
}
uint64_t perft(ChessPosition cp, int max_depth, int depth) {
  Vector moves = gen_legal_moves(cp);

  if (depth == max_depth - 1) {
    int nr = moves.count;
    vector_free(moves);
    return nr;
  }
  uint64_t total = 0;
  for (size_t i = 0; i < moves.count; i++) {
    UndoMove um = apply_move(&cp, VALUE(Move, vector_get(moves, i)));
    // print_position(cp);
    total += perft(cp, max_depth, depth + 1);

    undo_move(&cp, um);
  }

  vector_free(moves);
  return total;
}
