#!/usr/bin/env python3
"""PS5-safe ONNX Runtime 1.18.2 MLAS intrinsic scoping patch.

The upstream x86 AVX common header defines AVX-512/VNNI helper functions
unconditionally, including when included by a translation unit compiled
with only AVX2. That fails on Clang targeting the PS5. Guard each helper
using its actual required compiler ISA; keep AVX2 kernels intact. Do not
enable AVX-512 globally or on AVX2 translation units.
"""
from __future__ import annotations
import argparse
from pathlib import Path

REL_HEADER = "onnxruntime/core/mlas/lib/sqnbitgemm_kernel_avx_common.h"
REL_CMAKE = "cmake/onnxruntime_mlas.cmake"

def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one original anchor, found {count}")
    return text.replace(old, new, 1)

def patch_text(header: str, cmake: str) -> tuple[str, str]:
    pairs = [
        (
            "static MLAS_FORCEINLINE __m256\ndot_quad_avx512vnni(",
            "static MLAS_FORCEINLINE __m256\ndot_quad_avx2(",
            "__AVX512VNNI__",
            "VNNI dot product",
        ),
        (
            "static MLAS_FORCEINLINE void\nload_and_mul_sum_s8_quads_with_zp_avx512vnni(",
            "static MLAS_FORCEINLINE void\nload_and_mul_sum_s8_quads_with_zp_avx2(",
            "__AVX512VNNI__",
            "VNNI multiply-accumulate",
        ),
        (
            "template <bool HasZeroPoint>\nstatic MLAS_FORCEINLINE void\naccumulate_mul_sum_avx512vnni(",
            "template <bool HasZeroPoint>\nstatic MLAS_FORCEINLINE void\naccumulate_mul_sum_avx2(",
            "__AVX512VNNI__",
            "VNNI accumulation",
        ),
        (
            "static MLAS_FORCEINLINE __m128\nFoldAccumulators(const __m512&",
            "}  // namespace",
            "__AVX512F__",
            "512-bit accumulator",
        ),
    ]
    for begin, end, isa, label in pairs:
        if header.count(begin) != 1 or header.count(end) != 1:
            raise RuntimeError(f"{label}: unexpected upstream header contents")
        start_pos = header.index(begin)
        end_pos = header.index(end, start_pos)
        header = (
            header[:start_pos] + f"#if defined({isa})  // PS5: exclude ISA-specific helpers from AVX2 TUs\n"
            + header[start_pos:end_pos] + f"#endif  // {isa}\n\n"
            + header[end_pos:]
        )
        print(f"Scoped {label} to {isa}")
    # AVX-512 core needs AVX512F (vector types) and FMA in addition to
    # BW/DQ/VL. These flags apply ONLY to the AVX-512 kernels, and the ORT
    # runtime CPUID dispatcher must still guard their execution on Zen 2.
    original = (
        "set_source_files_properties(${mlas_platform_srcs_avx512core} "
        'PROPERTIES COMPILE_FLAGS "-mavx512bw -mavx512dq -mavx512vl")'
    )
    replacement = (
        "set_source_files_properties(${mlas_platform_srcs_avx512core} "
        'PROPERTIES COMPILE_FLAGS "-mfma -mavx512f -mavx512bw -mavx512dq -mavx512vl")'
    )
    cmake = replace_once(cmake, original, replacement, "AVX512 core flags")
    if header.count("#if defined(__AVX512VNNI__)") != 3:
        raise RuntimeError("VNNI guard verification failed")
    if header.count("#if defined(__AVX512F__)") != 1:
        raise RuntimeError("AVX512F guard verification failed")
    if "static MLAS_FORCEINLINE __m256\ndot_quad_avx2(" not in header:
        raise RuntimeError("AVX2 fallback was accidentally removed")
    return header, cmake

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("ort_root", type=Path)
    args = parser.parse_args()
    header_path = args.ort_root / REL_HEADER
    cmake_path = args.ort_root / REL_CMAKE
    header = header_path.read_text()
    cmake = cmake_path.read_text()
    updated_header, updated_cmake = patch_text(header, cmake)
    header_path.write_text(updated_header)
    cmake_path.write_text(updated_cmake)
    print("PS5 MLAS intrinsic guards and per-file ISA flags patched safely")

if __name__ == "__main__":
    main()
