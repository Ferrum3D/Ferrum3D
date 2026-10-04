# Renderer sample

Build `RendererSample` and launch it from `cmake-build/windows-debug-msvc/Samples/Renderer`.
The sample displays a static scene with warm bunnies on the left, textured bunnies in the center, and cool bunnies on the right using the default depth and opaque passes.

GPU validation, lifecycle stress modes, and benchmarks live in `Samples/GpuDrivenTestApp`.
Each application has its own asset depot. Renderer uses `StanfordBunny`, `Textured`, `texture`, `BunnyMaterial`, `BunnyVariants`, `BunnyWarm`, and `BunnyCool`.
Rebuild these with FerrumCli using `Samples/Renderer/Assets/source` as the source root and `Samples/Renderer/Assets` as the output root.
