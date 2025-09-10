use std::{hash::Hash, rc::Weak};

use crate::{
    engine::wrapper::{self, ChessPosition, Move},
    minimax::tree::State,
};

impl Eq for ChessPosition {}
impl Eq for Move {}
impl State for ChessPosition {
    type Move = wrapper::Move;
    fn apply(&mut self, mv: Self::Move) {
        todo!()
    }
    fn is_game_over(&self) -> bool {
        todo!()
    }
    fn children<S: State>(&self) -> std::collections::HashMap<S::Move, S> {
        todo!()
    }
    fn eval(&self) -> f32 {
        todo!()
    }
}
