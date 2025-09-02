include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

fn main() {
    unsafe {
        load_king_bb();
        load_knight_bb();
        load_bishop_bb();
        load_rook_bb();

        let cp: ChessPosition = init_position();

        let res = perft(cp, 5, 0);
        println!("perft(5) = {}", res);
    }
}
