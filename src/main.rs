use crate::engine::wrapper::{load_required_bitboards, ChessPosition};

mod engine;
fn main() {
    load_required_bitboards();
}
#[cfg(test)]
mod tests{
    use std::{fs::{self, File}, io::{BufRead, BufReader, Error}, time::Instant};

    use crate::engine::wrapper::{load_required_bitboards, ChessPosition};

    #[test]
    fn engine()->Result<(), Error>{
        load_required_bitboards();
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

                let depth:u32=depth[1..].parse().unwrap();
                let expected:u64=expected.parse().unwrap();

                

                let start = Instant::now();
                let mut cp=ChessPosition::new_from_fen(fen);
                let res=cp.perft(depth);
                let duration = start.elapsed();

                assert_eq!(res, expected, "perft mismatch for FEN: {fen} at depth {depth}");
                println!("Test: {} at depth {} in {} ms: {}", i, depth, duration.as_millis(), expected);
            }
        }

        Ok(())
    }
}
