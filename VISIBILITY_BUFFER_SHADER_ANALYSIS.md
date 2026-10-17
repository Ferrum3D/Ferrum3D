# Visibility/deferred work-group and scalarization analysis

2026-10-17. Baseline: `be594065578816c8a18be059121e5b78836774b1`.

## Implementation

All five material evaluation/shading kernels now use `EVALUATION_LANES * kSurfaceWavesPerGroup` threads, with `kSurfaceWavesPerGroup = 4`: 128 threads for 32-lane commands and 256 for 64-lane commands. Command emission dispatches `ceil(waveCount / 4)` groups. The common decoder selects command `groupID.x * 4 + groupThread / EVALUATION_LANES` and lane `groupThread % EVALUATION_LANES`. Unused commands in the last group and unused lanes in partial commands return before reading pixels.

The retained uniformity changes cover:

- Dispatch-wide shader ranges, camera constants, the mesh scene descriptor and scene table addresses.
- Wave-command index, tile, binding/group, pixel offset and valid-lane count. Tile/address calculations can then use scalar registers.
- The evaluation group from the command, used directly for mesh/material lookup. Group LOD metadata, geometry base address, material parameter pointer and shading route are shared within a command.
- Material texture descriptor indices and textured-material tint. Instance UV scale, transforms, meshlet/triangle selection, vertex data, reconstructed attributes and GBuffer values remain varying.

Material pipelines now request their configured 32/64 lane count through the existing wave-size control when supported. The guarded `SurfaceWaveUniform` helper also preserves correctness when a hardware wave contains multiple logical commands. Logical 64-lane commands remain supported on NVIDIA's 32-lane waves. Dispatch-wide broadcasts do not depend on the logical command size.

The classification count/scatter shaders already use wave reductions for bucket selection/counts and broadcast atomic reservations. Their per-pixel bucket selection varies. Command emission assigns a different command to each thread, so its bucket search and binding lookup also vary; those values were not broadcast.

## Method

Used the supplied RGA **2.14.0.14** installation and its bundled AMD compiler. SPIR-V was produced by the repository's `ThirdParty/dxc/bin/x64/dxc.exe`, with the engine's `cs_6_7`, HLSL 2021, Vulkan 1.3, DX layout, `-Od -Zi -Zpr`, interface/binding preservation and reflection options.

Compared three versions with identical compiler options:

1. Baseline, one logical wave per group.
2. Wider groups only, without uniformity hints or mesh/material lookup changes.
3. Final implementation, wider groups and the retained scalarization changes.

Each version was compiled for **gfx1100 (RDNA3)** and **gfx1201 (RDNA4)**, at native wave32 and wave64: 60 compiled cases. The bundled AMD compiler was given explicit `--native-wave-size` and `--subgroup-size` values. Without those settings it can select wave64 for widened groups, which would confound the comparison. Its binaries were analyzed with RGA `-s bin --isa ... -a ... --livereg ...`.

The bundled compiler rejects `gfx1030` as an invalid target, so these are not RDNA2 ISA measurements. No AMD hardware timing or NVIDIA speedup is claimed. Offline auto-layout results are compiler comparisons, not a complete runtime pipeline performance model.

For **EvaluateTextured only**, the bundled SPIR-V validator rejects the existing scalar-layout `float4 Tint` at offset 4. That shader was compiled with `--validate-spirv=false`; its layout was unchanged. The renderer enables scalar block layout, and runtime GPU validation passed. Other shaders retained the compiler's SPIR-V validation.

Raw sources, SPIR-V, binaries, ISA, register reports, logs and the comparison runner are under [the analysis directory](/C:/Users/user/TheCode/Ferrum3D/cmake-build/shader-wave-analysis). `summary.json` contains all 60 results. These intermediate artifacts are ignored by Git.

## RDNA3 wave32

VGPR/SGPR values are RGA's reported used registers, not rounded hardware allocation. Instruction counts are static ISA instructions; an encoded VOPD pair counts as one instruction. They do not represent dynamic loop iterations or execution time.

| Shader | Baseline VGPR | Wider-only VGPR | Final VGPR | SGPR baseline → final | Instructions baseline → final | ISA bytes baseline → final |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Bunny evaluation | 57 | 55 | 43 | 34 → 37 | 708 → 692 | 3968 → 3764 |
| Textured evaluation | 60 | 57 | 46 | 46 → 42 | 874 → 829 | 4860 → 4508 |
| PBR evaluation | 61 | 57 | 52 | 46 → 62 | 1190 → 1135 | 6592 → 6120 |
| Simple-lit shading | 16 | 16 | 16 | 22 → 26 | 189 → 196 | 988 → 1004 |
| PBR shading | 34 | 34 | 34 | 54 → 54 | 372 → 380 | 1992 → 2012 |

The evaluation shaders benefit beyond the wider-group control. Shading register usage is unchanged, with a small instruction increase versus the old single-wave groups. Relative to wider groups alone, the final simple-lit and PBR shading instruction counts fall from 204 to 196 and 382 to 380 respectively.

For PBR evaluation, scalar load instruction count stays at **16**, while vector buffer/global load instructions fall from **52 to 46**, and `v_readfirstlane_b32` instructions increase from **16 to 56**. The compiler keeps many loads in VMEM and moves their shared results/address calculations into SGPRs; this is not a wholesale conversion to scalar memory loads. Command addressing uses `s_add_i32` / `s_lshl_b32` after the broadcast. Register usage and total instructions improve despite the additional broadcasts.

Compare [baseline PBR ISA](/C:/Users/user/TheCode/Ferrum3D/cmake-build/shader-wave-analysis/baseline/gfx1100-wave32/gfx1100_EvaluateStandardPBR_comp.isa) with [final PBR ISA](/C:/Users/user/TheCode/Ferrum3D/cmake-build/shader-wave-analysis/current/gfx1100-wave32/gfx1100_EvaluateStandardPBR_comp.isa).

## RDNA3 wave64

| Shader | Baseline VGPR | Wider-only VGPR | Final VGPR | SGPR baseline → final | Instructions baseline → final |
| --- | ---: | ---: | ---: | ---: | ---: |
| Bunny evaluation | 55 | 55 | 43 | 50 → 46 | 746 → 724 |
| Textured evaluation | 57 | 57 | 46 | 58 → 54 | 909 → 863 |
| PBR evaluation | 57 | 57 | 47 | 62 → 74 | 1221 → 1185 |
| Simple-lit shading | 16 | 16 | 16 | 26 → 26 | 199 → 206 |
| PBR shading | 34 | 34 | 34 | 54 → 54 | 377 → 384 |

## RDNA4 cross-check

| Shader | Wave32 VGPR baseline → final | Wave64 VGPR baseline → final | Wave32 instructions baseline → final | Wave64 instructions baseline → final |
| --- | ---: | ---: | ---: | ---: |
| Bunny evaluation | 57 → 43 | 55 → 43 | 748 → 727 | 769 → 773 |
| Textured evaluation | 60 → 46 | 57 → 45 | 924 → 877 | 943 → 924 |
| PBR evaluation | 61 → 52 | 57 → 47 | 1266 → 1192 | 1281 → 1258 |
| Simple-lit shading | 16 → 16 | 16 → 16 | 203 → 208 | 214 → 218 |
| PBR shading | 35 → 35 | 35 → 35 | 396 → 402 | 403 → 408 |

There were no reported VGPR spills or scratch usage in any of the 60 cases. Lower VGPR use does not by itself establish an occupancy or timing improvement; SGPR use and work-group residency also matter.

## Experiments not retained

- Broadcasting the deferred PBR camera matrices increased gfx1100 wave32 usage from 34 to 38 VGPRs and enlarged its ISA. The original shading camera load remains. Camera broadcasts were retained for mesh evaluation, where they reduced VGPR pressure.
- Removing the raw physical-address `NonUniformResourceIndex` annotation produced byte-identical gfx1100 wave32 ISA for all five shaders. That generic helper was left unchanged. Bindless descriptor accesses also retain their existing non-uniform annotations.
- DXC warns that `[WaveSize]` is unsupported for Vulkan SPIR-V, so that attribute was not retained; the existing pipeline wave-size control is used instead.

## Runtime verification

- Reflection generation and root configuration completed; Debug builds passed for `FeGraphicsTests`, `GameSample`, `RendererSample` and `GpuDrivenTestApp`.
- All seven graphics tests passed with GPU validation. Coverage includes 32/64-lane material evaluation and shading, non-symmetric camera reconstruction, multiple commands per group, partial waves, a new four-pixel tail case, and indirect evaluation dispatches above 65,535 groups. Shader ranges and rounded dispatch arguments are checked independently by readback.
- GameSample smoke runs passed in deferred, surface-normal and evaluation-group modes with GPU validation.
- RendererSample smoke and the 15-frame GPU flight stress run passed with GPU validation. No Vulkan validation errors were reported.
- No assets were recooked. Changes remain uncommitted.
