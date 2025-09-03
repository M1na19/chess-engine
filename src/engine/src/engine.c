#include "engine.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
ChessPosition engine_init_position() {
  ChessPosition cp;
  // Clear everything
  memset(&cp, 0, sizeof(cp));
  for (int sq = 0; sq < 64; sq++) {
    cp.piece_board[sq].piece = NONE;
  }

  // White pieces
  cp.piece_board[0].color = WHITE;
  cp.piece_board[0].piece = ROOK;
  cp.piece_board[1].color = WHITE;
  cp.piece_board[1].piece = KNIGHT;
  cp.piece_board[2].color = WHITE;
  cp.piece_board[2].piece = BISHOP;
  cp.piece_board[3].color = WHITE;
  cp.piece_board[3].piece = QUEEN;
  cp.piece_board[4].color = WHITE;
  cp.piece_board[4].piece = KING;
  cp.piece_board[5].color = WHITE;
  cp.piece_board[5].piece = BISHOP;
  cp.piece_board[6].color = WHITE;
  cp.piece_board[6].piece = KNIGHT;
  cp.piece_board[7].color = WHITE;
  cp.piece_board[7].piece = ROOK;
  for (int i = 8; i < 16; i++) {
    cp.piece_board[i].color = WHITE;
    cp.piece_board[i].piece = PAWN;
  }

  // Black pieces
  cp.piece_board[56].color = BLACK;
  cp.piece_board[56].piece = ROOK;
  cp.piece_board[57].color = BLACK;
  cp.piece_board[57].piece = KNIGHT;
  cp.piece_board[58].color = BLACK;
  cp.piece_board[58].piece = BISHOP;
  cp.piece_board[59].color = BLACK;
  cp.piece_board[59].piece = QUEEN;
  cp.piece_board[60].color = BLACK;
  cp.piece_board[60].piece = KING;
  cp.piece_board[61].color = BLACK;
  cp.piece_board[61].piece = BISHOP;
  cp.piece_board[62].color = BLACK;
  cp.piece_board[62].piece = KNIGHT;
  cp.piece_board[63].color = BLACK;
  cp.piece_board[63].piece = ROOK;
  for (int i = 48; i < 56; i++) {
    cp.piece_board[i].color = BLACK;
    cp.piece_board[i].piece = PAWN;
  }

  // White pieces
  cp.board[WHITE][PAWN] = 0x000000000000FF00ULL;   // rank 2
  cp.board[WHITE][ROOK] = 0x0000000000000081ULL;   // a1, h1
  cp.board[WHITE][KNIGHT] = 0x0000000000000042ULL; // b1, g1
  cp.board[WHITE][BISHOP] = 0x0000000000000024ULL; // c1, f1
  cp.board[WHITE][QUEEN] = 0x0000000000000008ULL;  // d1
  cp.board[WHITE][KING] = 0x0000000000000010ULL;   // e1

  // Black pieces
  cp.board[BLACK][PAWN] = 0x00FF000000000000ULL;   // rank 7
  cp.board[BLACK][ROOK] = 0x8100000000000000ULL;   // a8, h8
  cp.board[BLACK][KNIGHT] = 0x4200000000000000ULL; // b8, g8
  cp.board[BLACK][BISHOP] = 0x2400000000000000ULL; // c8, f8
  cp.board[BLACK][QUEEN] = 0x0800000000000000ULL;  // d8
  cp.board[BLACK][KING] = 0x1000000000000000ULL;   // e8

  // Full castling rights for both sides
  cp.castle_laws[WHITE] = CAN_CASTLE_BOTH_WAYS;
  cp.castle_laws[BLACK] = CAN_CASTLE_BOTH_WAYS;

  // White to move first
  cp.side_to_move = WHITE;

  // No en passant target square at start
  cp.en_passant.en_passant_status = EN_PASSANT_NOT_POSSIBLE;

  // Clocks
  cp.half_move_count = 0;
  cp.move_count = 1;
  return cp;
}
ChessPosition engine_init_position_from_fen(const char *fen) {
  ChessPosition cp;
  // Clear everything
  memset(&cp, 0, sizeof(cp));
  for (int sq = 0; sq < 64; sq++) {
    cp.piece_board[sq].piece = NONE;
  }

  // 1. Parse piece placement
  int sq = 56; // starts from rank 8, file a (a8)
  while (*fen && *fen != ' ') {
    if (*fen == '/') {
      sq -= 16; // next rank
      fen++;
    } else if (*fen >= '1' && *fen <= '8') {
      sq += *fen - '0'; // skip empty squares
      fen++;
    } else {
      Color color = (*fen >= 'A' && *fen <= 'Z') ? WHITE : BLACK;
      PieceType piece;
      switch (tolower(*fen)) {
      case 'p':
        piece = PAWN;
        break;
      case 'r':
        piece = ROOK;
        break;
      case 'n':
        piece = KNIGHT;
        break;
      case 'b':
        piece = BISHOP;
        break;
      case 'q':
        piece = QUEEN;
        break;
      case 'k':
        piece = KING;
        break;
      default:
        piece = NONE;
        break;
      }

      cp.piece_board[sq].color = color;
      cp.piece_board[sq].piece = piece;
      cp.board[color][piece] |= 1ULL << sq;
      sq++;
      fen++;
    }
  }

  // 2. Skip space
  if (*fen == ' ')
    fen++;

  // 3. Parse side to move
  cp.side_to_move = (*fen == 'w') ? WHITE : BLACK;
  while (*fen && *fen != ' ')
    fen++;

  // 4. Parse castling rights
  if (*fen == ' ')
    fen++;
  cp.castle_laws[WHITE] = 0;
  cp.castle_laws[BLACK] = 0;
  while (*fen && *fen != ' ') {
    switch (*fen) {
    case 'K':
      cp.castle_laws[WHITE] |= CAN_KING_SIDE_CASTLE;
      break;
    case 'Q':
      cp.castle_laws[WHITE] |= CAN_QUEEN_SIDE_CASTLE;
      break;
    case 'k':
      cp.castle_laws[BLACK] |= CAN_KING_SIDE_CASTLE;
      break;
    case 'q':
      cp.castle_laws[BLACK] |= CAN_QUEEN_SIDE_CASTLE;
      break;
    }
    fen++;
  }

  // 5. Parse en passant square
  if (*fen == ' ')
    fen++;
  if (*fen != '-') {
    int file = fen[0] - 'a';
    int rank = fen[1] - '1';
    cp.en_passant.en_passant_status = EN_PASSANT_POSSIBLE;
    cp.en_passant.en_passant_square = rank * 8 + file;
  } else {
    cp.en_passant.en_passant_status = EN_PASSANT_NOT_POSSIBLE;
  }
  while (*fen && *fen != ' ')
    fen++;

  // 6. Halfmove clock
  if (*fen == ' ')
    fen++;
  cp.half_move_count = atoi(fen);
  while (*fen && *fen != ' ')
    fen++;

  // 7. Fullmove number
  if (*fen == ' ')
    fen++;
  cp.move_count = atoi(fen);
  return cp;
}