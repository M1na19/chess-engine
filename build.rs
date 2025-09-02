use std::{env, path::PathBuf};

fn main() {
    // Rebuild if headers or sources change
    println!("cargo:rerun-if-changed=src/engine/include");
    println!("cargo:rerun-if-changed=src/engine/src");

    let mut build = cc::Build::new();
    build
        .include("src/engine/include")
        .flag_if_supported("-std=c99")
        .flag_if_supported("-Ofast")
        .flag_if_supported("-march=native")
        .flag_if_supported("-Wall")
        .flag_if_supported("-Wextra")
        .files([
            "src/engine/src/utils/utils.c",
            "src/engine/src/vector/vector.c",
            "src/engine/src/debug/debug.c",
            "src/engine/src/moves/moves.c",
            "src/engine/src/precompute/load.c",
            "src/engine/src/engine.c",
        ]);
    build.compile("engine_c");


    let bindings = bindgen::Builder::default()
        .header("src/engine/include/rust_ffi.h")
        .clang_arg("-std=c99")
        .clang_arg("-Isrc/engine/include")
        .allowlist_type("ChessPosition")
        .allowlist_type("Move")
        .allowlist_type("UndoMove")
        .allowlist_type("Vector")
        .allowlist_type("EnPassant")
        .allowlist_type("Promotion")
        .allowlist_type("Capture")
        .allowlist_type("Color")
        .allowlist_type("PieceType")
        .allowlist_function("init_position")
        .allowlist_function("init_position_from_fen")
        .allowlist_function("gen_legal_moves")
        .allowlist_function("apply_move")
        .allowlist_function("undo_move")
        .allowlist_function("perft")
        .allowlist_function("load_knight_bb")
        .allowlist_function("load_king_bb")
        .allowlist_function("load_bishop_bb")
        .allowlist_function("load_rook_bb")
        .generate()
        .expect("Unable to generate bindings for engine API");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
