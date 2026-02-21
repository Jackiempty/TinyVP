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
    if input1.dtype != torch.int32 or input2.dtype != torch.int32:
        raise TypeError(
            "Hardware accelerator currently only supports torch.int32 tensors."
        )
    if input1.size() != input2.size():
        raise ValueError(
            f"Tensor sizes do not match: {input1.size()} vs {input2.size()}"
        )
    if input1.numel() > 512:
        raise ValueError(
            f"Tensor size {input1.numel()} exceeds hardware MAX_VEC_SIZE (512)."
        )

    input1_contig = input1.contiguous()
    input2_contig = input2.contiguous()
    output = vadd_impl(input1_contig, input2_contig)
    return output


__all__ = ["linear", "vadd"]
