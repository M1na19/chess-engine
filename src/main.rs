use std::time::Instant;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

fn main() {
    let res;
    let depth=6;
    unsafe {
        load_king_bb();
        load_knight_bb();
        load_bishop_bb();
        load_rook_bb();

        let start = Instant::now();
        let mut cp: ChessPosition = init_position();
        res = perft(&mut cp as *mut ChessPosition, depth, 0);
        let duration = start.elapsed();
        println!("perft({}) = {} in {} ms", depth, res, duration.as_millis());
    }
    
}
#[cfg(test)]
mod tests{
    use std::{ffi::CString, fs::{self, File}, io::{BufRead, BufReader, Error}, time::Instant};

    use super::*;
    #[test]
    fn engine()->Result<(), Error>{
        unsafe {
            load_king_bb();
            load_knight_bb();
            load_bishop_bb();
            load_rook_bb();
        }
        for entry in fs::read_dir("tests/perft")?{
            let entry=entry?;
            let file=File::open(entry.path())?;
            let reader=BufReader::new(file);
            for (i,line) in reader.lines().enumerate(){
                let line=line?;
                let line=line.trim();

                let (fen,config) = line.split_once(';').unwrap();

                let config=config.trim();
                let (depth, expected)=config.split_once(' ').unwrap();

                let fen_c=CString::new(fen).unwrap();
                let depth:u32=depth[1..].parse().unwrap();
                let expected:u64=expected.parse().unwrap();

                let res;

                let start = Instant::now();
                unsafe {
                    let mut cp: ChessPosition = init_position_from_fen(fen_c.as_ptr());
                    res = perft(&mut cp as *mut ChessPosition, depth as i32, 0);
                }
                let duration = start.elapsed();

                assert_eq!(res, expected, "perft mismatch for FEN: {fen} at depth {depth}");
                println!("Test: {} at depth {} in {} ms: {}", i, depth, duration.as_millis(), expected);
            }
        }

        Ok(())
    }
}
