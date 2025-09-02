import glob
import os
import subprocess
import sys
import time

ENGINE_CMD = ['./build/chess']
TESTS_DIR = 'tests'
TIMEOUT_SEC = 30

def load_epd_tests(filename):
    tests = []
    with open(filename, "r") as f:
        for line in f:
            line = line.strip()
            parts = [p for p in line.split(";") if p.strip()]
            if not parts:
                continue
            fen = parts[0].strip()
            for field in parts[1:]:
                field = field.strip()
                if not field or not field[0].upper() == 'D':
                    continue
                tokens = field.split()
                if len(tokens) < 2:
                    continue
                try:
                    depth = int(tokens[0][1:])
                    nodes = int(tokens[1])
                    tests.append({"fen": fen, "depth": depth, "nodes": nodes})
                except ValueError:
                    continue
    return tests

def run_case(fen: str, depth: int, expected_nodes: int):
    input_data = f"{fen}\n{depth}\n"
    start_time = time.time()
    try:
        result = subprocess.run(
            ENGINE_CMD,
            input=input_data,
            capture_output=True,
            text=True,
            timeout=TIMEOUT_SEC
        )
    except subprocess.TimeoutExpired as e:
        return ("TIMEOUT", None, time.time() - start_time, str(e))

    elapsed = time.time() - start_time
    if result.returncode != 0:
        return ("ERROR", None, elapsed, result.stderr)

    try:
        actual = int(result.stdout.strip())
    except ValueError:
        return ("BAD-OUTPUT", None, elapsed, f"stdout: {result.stdout!r}")

    return ("PASS" if actual == expected_nodes else "FAIL", actual, elapsed, None)

def run_suite(epd_path: str):
    tests = load_epd_tests(epd_path)
    print(f"\n=== Suite: {epd_path} ({len(tests)} cases) ===")
    print(f"{'#':<4} {'Depth':<5} {'Expected':<12} {'Got':<12} {'Time(s)':<9} {'Status'}")
    print("-" * 64)

    passed = failed = errors = 0
    total_time = 0.0

    for i, t in enumerate(tests, 1):
        status, actual, t_sec, err = run_case(t['fen'], t['depth'], t['nodes'])
        total_time += t_sec if t_sec is not None else 0.0

        if status == "PASS":
            passed += 1
        elif status in ("FAIL",):
            failed += 1
        else:
            errors += 1

        actual_str = f"{actual}" if actual is not None else "—"
        print(f"{i:<4} {t['depth']:<5} {t['nodes']:<12} {actual_str:<12} {t_sec:<9.3f} {status}")
        if err and status != "PASS":
            for line in err.strip().splitlines():
                print(f"    {line}")

    print("-" * 64)
    print(f"Suite summary: {passed} passed, {failed} failed, {errors} errors | time {total_time:.3f}s")
    return passed, failed, errors, total_time, len(tests)

def find_epd_files():
    return sorted(glob.glob(os.path.join(TESTS_DIR, "**", "*.epd"), recursive=True))
    

def main():
    epd_files = find_epd_files()
    if not epd_files:
        sys.exit(2)

    grand_pass = grand_fail = grand_err = 0
    grand_time = 0.0
    grand_total = 0

    for epd in epd_files:
        p, f, e, t, n = run_suite(epd)
        grand_pass += p
        grand_fail += f
        grand_err += e
        grand_time += t
        grand_total += n

    print("\n=== Grand Total ===")
    print(f"Suites: {len(epd_files)} | Cases: {grand_total}")
    print(f"Passed: {grand_pass} | Failed: {grand_fail} | Errors: {grand_err} | Time: {grand_time:.3f}s")

    # Exit nonzero if any failure/error occurred (handy for CI)
    sys.exit(0 if (grand_fail == 0 and grand_err == 0) else 1)

if __name__ == "__main__":
    main()
