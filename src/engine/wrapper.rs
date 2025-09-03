use std::{ffi::CString, fmt::Display, os::raw::c_void};

use crate::engine::wrapper::c_engine::{chess_position, engine_gen_legal_moves, Move};

mod c_engine{
    #![allow(non_upper_case_globals, unused, non_camel_case_types)]
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
#[repr(transparent)]
pub struct ChessPosition(c_engine::ChessPosition);
impl ChessPosition {
    #[inline]
    pub fn new() ->Self{
        unsafe {
            ChessPosition(c_engine::engine_init_position())
        }
    }
    #[inline]
    pub fn new_from_fen(fen: &str)->ChessPosition{
        unsafe {
            ChessPosition(c_engine::engine_init_position_from_fen(CString::new(fen).unwrap().as_ptr()))
        }
    }
    #[inline]
    pub fn is_king_attacked(&mut self)->bool{
        let res;
        unsafe {
            let chess_pos=&mut self.0;
            dbg!(chess_pos.side_to_move());
            let king_sq=chess_pos.board[chess_pos.side_to_move() as usize][c_engine::piece_type_KING as usize].trailing_zeros() as u8;
            res=c_engine::engine_is_square_attacked( chess_pos as *mut c_engine::chess_position, 1-chess_pos.side_to_move(),king_sq);
        }
        return res==1;
    }
    #[inline]
    pub fn perft(&mut self, depth:u32)->u64{
        unsafe {
            let chess_pos= &mut self.0;
            c_engine::engine_perft(chess_pos as *mut c_engine::chess_position, depth as i32, 0)
        }
    }
    #[inline]
    pub fn apply_move(&mut self, mv: c_engine::Move)-> c_engine::UndoMove{
        unsafe{
            let chess_pos= &mut self.0;
            return c_engine::engine_apply_move(chess_pos as *mut c_engine::chess_position, mv);
        }
    }
    #[inline]
    pub fn undo_move(&mut self, u_mv: c_engine::UndoMove){
        unsafe{
            let chess_pos= &mut self.0;
            c_engine::engine_undo_move(chess_pos as *mut c_engine::chess_position, u_mv);
        }
    }
    #[inline]
    pub fn gen_legal_moves(&mut self) ->Vec<c_engine::Move>{
        unsafe {
            let mut desc = c_engine::VectorMove {
                data: libc::malloc(size_of::<Move>()*256),                  
                capacity: 256,                       
                data_size: size_of::<c_engine::Move>(),
                count: 0,
            };
            c_engine::engine_gen_legal_moves(&mut self.0, &mut desc);
            let v=Vec::from_raw_parts(desc.data as *mut Move, desc.count, desc.capacity);
            return v;
        }
    }
    pub fn print(&self){
        unsafe {
            c_engine::print_position(self.0);
        }
    }
}
pub fn load_required_bitboards(){
    unsafe {
        c_engine::load_king_bb();
        c_engine::load_knight_bb();
        c_engine::load_bishop_bb();
        c_engine::load_rook_bb();
    }
}