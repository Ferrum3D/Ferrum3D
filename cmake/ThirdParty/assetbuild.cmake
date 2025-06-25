add_library(mikktspace STATIC ${FE_THIRD_PARTY_DIR}/MikkTSpace/mikktspace.c ${FE_THIRD_PARTY_DIR}/MikkTSpace/mikktspace.h)
target_include_directories(mikktspace PUBLIC ${FE_THIRD_PARTY_DIR}/MikkTSpace)
set_target_properties(mikktspace PROPERTIES FOLDER "ThirdParty")

add_library(tiny_gltf INTERFACE)
target_include_directories(tiny_gltf INTERFACE "${FE_THIRD_PARTY_DIR}/tiny_gltf")
set_target_properties(tiny_gltf PROPERTIES FOLDER "ThirdParty")

set(MESHOPTIMIZER_SOURCES
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/meshoptimizer.h
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/allocator.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/clusterizer.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/indexanalyzer.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/indexcodec.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/indexgenerator.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/overdrawoptimizer.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/partition.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/quantization.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/rasterizer.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/simplifier.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/spatialorder.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/stripifier.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/vcacheoptimizer.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/vertexcodec.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/vertexfilter.cpp
    ${FE_THIRD_PARTY_DIR}/meshoptimizer/src/vfetchoptimizer.cpp
)

add_library(meshoptimizer STATIC ${MESHOPTIMIZER_SOURCES})
target_include_directories(meshoptimizer PUBLIC ${FE_THIRD_PARTY_DIR}/meshoptimizer/src)
set_target_properties(meshoptimizer PROPERTIES FOLDER "ThirdParty")
