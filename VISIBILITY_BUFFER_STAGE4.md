# Stage 4: independent loose-tiled deferred shading

Select `--visibility-debug deferred` in GameSample or RendererSample. This mode runs depth/visibility, full-view material evaluation, a fresh GBuffer classifier and command-driven lighting, followed by the existing tonemap. Forward remains the default for Stage 4 comparisons; changing the default and completing residency/footprint tuning belong to Stage 5. Existing visibility and surface debug modes remain available.

## Shading classification

The second classifier reads only G0's shader-derived route. It receives neither visibility nor geometry database descriptors. Routes zero and 255 mean background and unsupported respectively; out-of-registry routes are diagnostic. Each shading shader has one binding per tile, so pixels from different original mesh/material groups can share a logical shading wave. Evaluation and shading registries remain independent.

The full-view count/prefix/scatter builder is shared with material evaluation. The shading mode preserves G0 and clears only its small counter/cursor arrays. It generates a separate pixel list, command stream, ranges and indirect arguments. Each registered shading shader consumes one indirect dispatch across the view; empty ranges dispatch zero. No radix sort, CPU visibility readback, tile-window loop or geometry reconstruction occurs during shading classification.

Material evaluation and shading retain separate routing/count/scan/range/argument metadata. Their large coordinate and wave buffers are physically shared, sized to the larger capacity and prepared before pass descriptors capture them. Frame-graph dependencies retire evaluation reads before shading scatter/emission overwrites the shared arrays. Routing and persistent allocations are reused across unchanged frames. Per-work scratch charges include the shared buffers; combined physical accounting must subtract their sizes once rather than double-counting them.

## Lighting and HDR

SimpleLit reads only color and normal and preserves the existing directional/ambient equation without applying PBR exposure. StandardPBR reads the packed surface, reconstructs world position from reverse-Z depth and a supplied inverse view-projection matrix, then calls the existing BRDF/light function. Viewport origin and size are included in the view buffer. Emissive remains scene-linear in G3 and is pre-exposed exactly once in lighting; AO remains reserved for indirect illumination.

Both kernels consume logical wave commands with valid-count tails and 32-bit tile-local coordinate words. Current runtime shading pipelines use 32 lanes; GPU tests also exercise 64-lane variants. Commands remain correct independently of the physical hardware wave width. HDR initialization writes background and unsupported diagnostics only; covered supported pixels are written by their shading kernel. It does not read uninitialized G1/G2/G3 background data, and G3 is loaded only for the extended-payload flag.

Format capabilities are cached per renderer. Visibility/deferred views use packed B10G11R11 HDR when storage writes and sampling are supported, otherwise RGBA16F. The fallback logs its eight-byte-per-pixel footprint versus the packed four-byte target. Forward views retain their authored packed render-target format. Sequential view/clear behavior and graphics-queue scheduling are preserved.

For the normal one-binding GameSample view, both classifiers fit one prefix block. Stable frames have five evaluation-builder dispatches, one evaluation dispatch, one background initializer, five shading-builder dispatches and one shading dispatch: 13 compute dispatches, plus the two small view-buffer uploads and existing raster/culling/tonemap work. Additional shaders add evaluation/shading dispatches and larger metadata can add scan levels. No GPU-time estimate is claimed before a new capture.

## Validation and remaining comparisons

The graphics GPU probes exercise independent G0-only classification with mixed shader routes, original material-group mixing, partial tiles, 32/64 lanes, unsupported routes, interior empty buckets and exactly-once command visits. Lighting probes use both packed HDR and RGBA16F, compare all pixels against the existing lighting equations after surface quantization, verify nonzero viewport origin and homogeneous world-position reconstruction, cover emissive on/off with poisoned unused payload, and test an empty view after populated views.

Validated on 2026-10-14: all seven graphics tests pass with API-only and GPU-assisted validation, including the expanded classification and lighting probes. Packed-HDR comparisons allow 3.2% relative storage-conversion error for the five-bit blue mantissa; RGBA16F comparisons use 0.12%. Both Debug samples pass deferred smoke runs with API validation. GameSample forward and surface-normal smoke runs also pass. No validation errors were reported. Configure, reflection generation, executable builds and manual builds of the three material depots complete.

Manual comparison remains silhouettes/material boundaries, PBR highlights and exposure, helmet emissive, warm/textured/cool bunnies, camera motion, reload/streaming, resize and multi-view captures. Use no validation flags for timings. Use `--api-validation` or `--gpu-validation` explicitly for correctness checks. Shader compilation still uses the repository's existing debug compilation policy; timings should be interpreted with that configuration in mind.

The camera-angle black-mesh regression was caused by `Math::Invert` dividing matrix cofactors by different SIMD lanes of its determinant reduction. Broadcasting the valid low lane fixes the inverse view-projection used by deferred world-position reconstruction. A rotated, translated perspective camera now has a CPU inverse regression test, and the GPU lighting probe compares reconstruction against an independent analytic perspective/camera transform rather than using the uploaded inverse for its expected result.
