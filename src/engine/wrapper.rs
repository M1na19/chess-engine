use std::{ffi::CString, fmt::Display, hash::Hash, os::raw::c_void};

use crate::engine::wrapper::c_engine::{
    chess_position, color_BLACK, color_WHITE, engine_gen_legal_moves, move__CAPTURE, move__CASTLE,
    move__PROMOTION,
};

mod c_engine {
    #![allow(non_upper_case_globals, unused, non_camel_case_types)]
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
#[repr(transparent)]
pub struct ChessPosition(c_engine::ChessPosition);
#[derive(Clone)]
pub struct Move(c_engine::Move);
#[derive(Clone)]
pub struct UndoMove(c_engine::UndoMove);

pub fn load_required_bitboards() {
    unsafe {
        c_engine::load_king_bb();
        c_engine::load_knight_bb();
        c_engine::load_bishop_bb();
        c_engine::load_rook_bb();
    }
}

impl ChessPosition {
    #[inline]
    pub fn new() -> Self {
        unsafe { ChessPosition(c_engine::engine_init_position()) }
    }
    #[inline]
    pub fn new_from_fen(fen: &str) -> ChessPosition {
        unsafe {
            ChessPosition(c_engine::engine_init_position_from_fen(
                CString::new(fen).unwrap().as_ptr(),
            ))
        }
    }
    #[inline]
    pub fn is_king_attacked(&mut self) -> bool {
        let res;
        unsafe {
            let chess_pos = &mut self.0;
            dbg!(chess_pos.side_to_move);
            let king_sq = chess_pos.board[chess_pos.side_to_move as usize]
                [c_engine::piece_type_KING as usize]
                .trailing_zeros() as u8;
            res = c_engine::engine_is_square_attacked(
                chess_pos as *mut c_engine::chess_position,
                1 - chess_pos.side_to_move,
                king_sq,
            );
        }
        return res == 1;
    }
    #[inline]
    pub fn perft(&mut self, depth: u32) -> u64 {
        unsafe {
            let chess_pos = &mut self.0;
            c_engine::engine_perft(chess_pos as *mut c_engine::chess_position, depth as i32, 0)
        }
    }
    #[inline]
    pub fn apply_move(&mut self, mv: Move) -> UndoMove {
        unsafe {
            let chess_pos = &mut self.0;
            return UndoMove(c_engine::engine_apply_move(
                chess_pos as *mut c_engine::chess_position,
                mv.0,
            ));
        }
    }
    #[inline]
    pub fn undo_move(&mut self, u_mv: UndoMove) {
        unsafe {
            let chess_pos = &mut self.0;
            c_engine::engine_undo_move(chess_pos as *mut c_engine::chess_position, u_mv.0);
        }
    }
    #[inline]
    pub fn gen_legal_moves(&mut self) -> Vec<Move> {
        unsafe {
            let mut desc = c_engine::VectorMove {
                data: libc::malloc(size_of::<Move>() * 256),
                capacity: 256,
                data_size: size_of::<c_engine::Move>(),
                count: 0,
            };
            c_engine::engine_gen_legal_moves(&mut self.0, &mut desc);
            let v =
                Vec::from_raw_parts(desc.data as *mut c_engine::Move, desc.count, desc.capacity)
                    .into_iter()
                    .map(Move)
                    .collect();
            return v;
        }
    }
    pub fn print(&self) {
        unsafe {
            c_engine::print_position(self.0);
        }
    }
}
impl Hash for ChessPosition {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        // Hash bitboards
        self.0.board.iter().for_each(|color_bitboards| {
            color_bitboards
                .iter()
                .for_each(|bitboard| bitboard.hash(state))
        });

        // Hash side to move
        self.0.side_to_move.hash(state);

        // Hash castle laws
        self.0.castle_laws[color_WHITE as usize].hash(state);
        self.0.castle_laws[color_BLACK as usize].hash(state);

        // Hash en_passant
        self.0.en_passant.en_passant_status.hash(state);
        self.0.en_passant.en_passant_square.hash(state);
    }
}
impl PartialEq for ChessPosition {
    fn eq(&self, other: &Self) -> bool {
        // Compare bitboards
        self.0
            .board
            .iter()
            .zip(other.0.board)
            .all(|(color_bitbords_a, color_bitboards_b)| {
                color_bitbords_a
                    .iter()
                    .zip(color_bitboards_b.iter())
                    .all(|(a, b)| a == b)
            });

        // Compare side to move
        if self.0.side_to_move != other.0.side_to_move {
            return false;
        }

        if self.0.castle_laws[color_WHITE as usize] != other.0.castle_laws[color_WHITE as usize] {
            return false;
        }
        if self.0.castle_laws[color_BLACK as usize] != other.0.castle_laws[color_BLACK as usize] {
            return false;
        }

        if self.0.en_passant.en_passant_status != other.0.en_passant.en_passant_status {
            return false;
        }
        if self.0.en_passant.en_passant_square != other.0.en_passant.en_passant_square {
            return false;
        }
        true
    }
}
impl Hash for Move {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.0.move_type.hash(state);
        unsafe {
            match self.0.move_type {
                move__CASTLE => {
                    self.0.__bindgen_anon_1.castle.hash(state);
                }
                move__CAPTURE => {
                    self.0.__bindgen_anon_1.capture.from.hash(state);
                    self.0.__bindgen_anon_1.capture.to.hash(state);
                    self.0.__bindgen_anon_1.capture.is_en_passant.hash(state);
                }
                move__PROMOTION => {
                    self.0.__bindgen_anon_1.promotion.from.hash(state);
                    self.0.__bindgen_anon_1.promotion.to.hash(state);
                    self.0.__bindgen_anon_1.promotion.promotion_type.hash(state);
                }
                _ => {
                    panic!("Unexpected move type")
                }
            }
        }
    }
}
impl PartialEq for Move {
    fn eq(&self, other: &Self) -> bool {
        if self.0.move_type != other.0.move_type {
            return false;
        }
        unsafe {
            match self.0.move_type {
                move__CASTLE => {
                    if self.0.__bindgen_anon_1.castle != other.0.__bindgen_anon_1.castle {
                        return false;
                    }
                    true
                }
                move__CAPTURE => {
                    if self.0.__bindgen_anon_1.capture.from != other.0.__bindgen_anon_1.capture.from
                    {
                        return false;
                    }
                    if self.0.__bindgen_anon_1.capture.to != other.0.__bindgen_anon_1.capture.to {
                        return false;
                    }
                    if self.0.__bindgen_anon_1.capture.is_en_passant
                        != other.0.__bindgen_anon_1.capture.is_en_passant
                    {
                        return false;
                    }
                    true
                }
                move__PROMOTION => {
                    if self.0.__bindgen_anon_1.promotion.from
                        != other.0.__bindgen_anon_1.capture.from
                    {
                        return false;
                    }
                    if self.0.__bindgen_anon_1.promotion.to != other.0.__bindgen_anon_1.capture.to {
                        return false;
                    }
                    if self.0.__bindgen_anon_1.promotion.promotion_type
                        != other.0.__bindgen_anon_1.promotion.promotion_type
                    {
                        return false;
                    }
                    true
                }
                _ => {
                    panic!("Unexpected move type")
                }
            }
        }
    }
}
