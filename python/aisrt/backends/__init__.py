try:
    from . import aisrt_cpu
except ImportError as e:
    aisrt_cpu = None
    import warnings

    warnings.warn(f"Failed to load aisrt_cpu backend: {e}")

try:
    from . import aisrt_rtl
except ImportError as e:
    aisrt_rtl = None
    import warnings

    warnings.warn(f"Failed to load aisrt_rtl backend: {e}")

__all__ = ["aisrt_cpu", "aisrt_rtl"]
