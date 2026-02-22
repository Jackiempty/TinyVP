# examples/test_vadd.py
import time
import torch
import aisrt.nn.functional as F

GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"


def main():
    print(f"\n{GREEN}=== Starting RTL Vector Addition Test ==={RESET}")

    size = 10
    vec_a = torch.arange(1, size + 1, dtype=torch.int32)
    vec_b = torch.arange(10, (size + 1) * 10, step=10, dtype=torch.int32)

    print(f"[Host] Input A: {vec_a.tolist()}")
    print(f"[Host] Input B: {vec_b.tolist()}")

    print("[Host] Dispatching computation to Hardware via SHM...")

    start_time = time.time()
    vec_c = F.vadd(vec_a, vec_b)
    end_time = time.time()

    print(
        f"[Host] Hardware computation finished in {(end_time - start_time) * 1000:.2f} ms"
    )
    print(f"[Host] Output C (from RTL): {vec_c.tolist()}")

    expected = vec_a + vec_b
    if torch.equal(vec_c, expected):
        print(
            f"\n{GREEN}Test Passed! Hardware computation perfectly matches PyTorch.{RESET}"
        )
    else:
        print(f"\n{RED}Test Failed! Results do not match.{RESET}")
        print(f"Expected: {expected.tolist()}")
        print(f"Got     : {vec_c.tolist()}")


if __name__ == "__main__":
    main()
