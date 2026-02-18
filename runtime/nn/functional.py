import torch

from aisrt_cpu import (
    linear as linear_impl,
)

from aisrt_rtl import (
    vadd as vadd_impl,
)


def linear(
    input: torch.Tensor, weight: torch.Tensor, bias: torch.Tensor | None = None
) -> torch.Tensor:
    if bias is None:
        bias = torch.zeros_like(input)
    output = torch.zeros(input.shape[0], weight.shape[0])
    linear_impl(output.numpy(), input.numpy(), weight.numpy(), bias.numpy())
    return output


def vadd(input1: torch.Tensor, input2: torch.Tensor) -> torch.Tensor:
    output = vadd_impl(input1, input2)
    return output


__all__ = ["linear", "vadd"]
