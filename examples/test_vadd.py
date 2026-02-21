# examples/test_vadd.py
import torch
import runtime.nn.functional as F

GREEN = "\033[92m"
RED = "\033[91m"
RESET = "\033[0m"


def main():
    print("=== Starting RTL Vector Addition Test ===")

    size = 10
    vec_a = torch.arange(1, size + 1, dtype=torch.int32)  # [1, 2, 3, ..., 10]
    vec_b = torch.arange(
        10, (size + 1) * 10, step=10, dtype=torch.int32
    )  # [10, 20, 30, ..., 100]

    print(f"[Host] Input A: {vec_a.tolist()}")
    print(f"[Host] Input B: {vec_b.tolist()}")

    print("\n[Host] Dispatching computation to Hardware via SHM...")
    vec_c = F.vadd(vec_a, vec_b)

    print(f"[Host] Output C (from RTL): {vec_c.tolist()}")

    expected = vec_a + vec_b
    if torch.equal(vec_c, expected):
        print(
            f"\n{GREEN}Test Passed! Hardware computation perfectly matches PyTorch.{RESET}"
        )
    else:
        print(f"\n{RED}Test Failed! Results do not match.{RESET}")
        print(f"Expected: {expected.tolist()}")


if __name__ == "__main__":
    main()
