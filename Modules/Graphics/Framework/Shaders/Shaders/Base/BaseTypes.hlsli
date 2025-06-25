#pragma once

#define FE_HOST_BEGIN_NAMESPACE(name)
#define FE_HOST_END_NAMESPACE

#if defined(__JETBRAINS_IDE__) || defined(__INTELLISENSE__)
typedef uint uint32_t;
#    define fe_globallycoherent

#    define fe_mesh_payload
#    define fe_mesh_vertices
#    define fe_mesh_indices

#    define FE_CONST const
#else
#    define fe_globallycoherent globallycoherent

#    define fe_mesh_payload payload
#    define fe_mesh_vertices vertices
#    define fe_mesh_indices indices

#    define FE_CONST
#endif

typedef min16float fehalf;
typedef min16float2 fehalf2;
typedef min16float3 fehalf3;
typedef min16float4 fehalf4;

#define FE_NUM_THREADS(x, y, z) [numthreads(x, y, z)]
#define FE_OUTPUT_TOPOLOGY(topology) [outputtopology(topology)]

static const uint32_t kInvalidIndex = 0xffffffff;
static const uint kLanesPerWave = 32;
