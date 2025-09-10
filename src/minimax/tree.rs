use std::{collections::HashMap, hash::Hash};

pub trait State: Eq + Hash {
    type Move: Eq + Hash + Clone;
    fn eval(&self) -> f32;
    fn is_game_over(&self) -> bool;

    fn apply(&mut self, mv: Self::Move);
    fn children<S: State>(&self) -> HashMap<S::Move, S>;
}

struct MiniMaxSearchTree<S> {
    root: S,
    transposition_table: HashMap<S, f32>,
}
struct MiniMaxEvaluation<S: State> {
    mv: Option<S::Move>,
    eval: f32,
}
impl<S: State> MiniMaxSearchTree<S> {
    fn new(root_state: S) -> Self {
        MiniMaxSearchTree {
            root: root_state,
            transposition_table: HashMap::new(),
        }
    }
    fn find_best_move(&self, max_depth: u32) -> MiniMaxEvaluation<S> {
        fn alpha_beta_prune<S: State>(
            state: &S,
            mv: Option<S::Move>,
            depth: u32,
            max_depth: u32,
            isMaximizingPlayer: bool,
            mut alpha: f32,
            mut beta: f32,
        ) -> MiniMaxEvaluation<S> {
            if depth == max_depth || state.is_game_over() {
                return MiniMaxEvaluation {
                    mv: mv,
                    eval: state.eval(),
                };
            }
            if isMaximizingPlayer {
                let mut best = MiniMaxEvaluation {
                    mv: None,
                    eval: f32::NEG_INFINITY,
                };
                let children = state.children::<S>();
                for (m, child) in children {
                    let child_eval = alpha_beta_prune(
                        &child,
                        Some(m),
                        depth + 1,
                        max_depth,
                        !isMaximizingPlayer,
                        alpha,
                        beta,
                    );
                    alpha = alpha.max(child_eval.eval);
                    if best.eval < child_eval.eval {
                        best = child_eval;
                    }
                    if beta <= alpha {
                        break;
                    }
                }
                return best;
            } else {
                let mut best = MiniMaxEvaluation {
                    mv: None,
                    eval: f32::INFINITY,
                };
                let children = state.children::<S>();
                for (m, child) in children {
                    let child_eval = alpha_beta_prune(
                        &child,
                        Some(m),
                        depth + 1,
                        max_depth,
                        !isMaximizingPlayer,
                        alpha,
                        beta,
                    );
                    beta = beta.min(child_eval.eval);
                    if best.eval > child_eval.eval {
                        best = child_eval;
                    }
                    if beta <= alpha {
                        break;
                    }
                }
                return best;
            }
        }
        return alpha_beta_prune(
            &self.root,
            None,
            0,
            max_depth,
            true,
            f32::NEG_INFINITY,
            f32::INFINITY,
        );
    }
}
