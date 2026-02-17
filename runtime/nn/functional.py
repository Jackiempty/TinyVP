import torch

from aisrt_cpu import (
    linear as linear_impl,
)

from aisrt_cuda import (
    vadd as vadd_impl,
    qadd as qadd_impl,
    mmul as mmul_impl,
    mtrans as mtrans_impl,
)


def linear(
    input: torch.Tensor, weight: torch.Tensor, bias: torch.Tensor | None = None
) -> torch.Tensor:
    if bias is None:
        bias = torch.zeros_like(input)
    output = torch.zeros(input.shape[0], weight.shape[0])
    linear_impl(output.numpy(), input.numpy(), weight.numpy(), bias.numpy())
    return output


def add(input1: torch.Tensor, input2: torch.Tensor) -> torch.Tensor:
    output = vadd_impl(input1, input2)
    return output


def qadd(
    input1: torch.Tensor, input2: torch.Tensor, scale: float = 1.0, zero_point: int = 0
) -> torch.Tensor:
    output = qadd_impl(input1, input2, scale, zero_point)
    return output


def matmul(input1: torch.Tensor, input2: torch.Tensor) -> torch.Tensor:
    if input1.dim() != 2 or input2.dim() != 2:
        raise NotImplementedError(
            f"matmul only supports 2D matrix x 2D matrix. Got {input1.dim()}D x {input2.dim()}D."
        )
    if input1.size(1) != input2.size(0):
        raise ValueError(
            f"Incompatible shapes for matmul: {tuple(input1.shape)} and {tuple(input2.shape)}."
        )
    output = mmul_impl(input1, input2)
    return output


def transpose(input: torch.Tensor, dim0: int, dim1: int) -> torch.Tensor:
    if input.dim() != 2:
        raise NotImplementedError(
            f"transpose only supports 2D matrices. Got {input.dim()}D tensor."
        )
    output = mtrans_impl(input)
    return output


__all__ = ["linear", "add", "qadd", "matmul", "transpose"]
