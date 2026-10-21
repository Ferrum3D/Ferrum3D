# Jolt Physics

Library sources copied from the local JoltPhysics 5.6.0 checkout, together with the upstream build entry point and MIT license. No upstream sample, test, viewer, or asset directories are included. The engine exposes only primitive rigid bodies.

cmake/ThirdParty/jolt.cmake disables compute backends, object streams, debug rendering, profiling, installation, and upstream compiler-flag overrides. Jolt uses the same dynamic MSVC runtime as the engine.

The upstream build file has one compatibility fix: quote CMAKE_CXX_FLAGS in its exception-flag replacement so configuring the codegen preset with an empty flags string succeeds.

WorldPhysicsService installs engine allocation callbacks before constructing Jolt objects. Small scenes use the upstream inline JobSystemSingleThreaded on the calling engine fiber. Larger scenes use a custom JobSystem with pooled JobNodes dispatched directly through engine job fibers and reusable engine WaitGroups, including draining queued engine jobs before destroying physics state. Jolt's own std::function job callable implementation remains upstream code; its internal allocations are outside the allocator callback contract.
