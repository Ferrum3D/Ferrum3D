#pragma once
#include <Graphics/Base/DrawTag.h>

FE_DECLARE_DRAW_TAG(Visibility);
//! Batches with MeshMobility::kStatic, whose derived data may be cached across frames.
FE_DECLARE_DRAW_TAG(Static);
//! Batches with MeshMobility::kDynamic, for passes over moving objects only.
FE_DECLARE_DRAW_TAG(Dynamic);

FE_DECLARE_DRAW_TAG(Transparent);
