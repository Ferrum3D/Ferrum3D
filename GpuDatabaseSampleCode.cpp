// === Declaration (C++) ===

struct MeshLodInfo final
{
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    uint32_t m_meshletCount;
    uint32_t m_primitiveCount;
};

struct MeshLodInfoTable final
{
	// We AoS layout here.
	MeshLodInfo m_info;
};

struct MeshGroupTable final
{
	// Here all geometry pointers (i.e. the first column) must be stored in a contiguous array followed by all the lod info slices (second column).
	// The code generator must calculate how many elements will fit in one page.
	// Each page must contain the same amount of rows for each column.
	BufferPointer m_geometry;
	DB::Slice<MeshLodInfoTable> m_lods;
};

struct MeshInstanceTable final
{
	DB::Ref<MeshGroupTable> m_meshGroup;
	float4x4 m_transform;
};

FE_RTTI_Reflect(MeshLodInfo, /* UUID here... */);
FE_DECLARE_SCENE_DATABASE_TABLE(MeshLodInfoTable);
// And others too...

// === Host Setup (C++) ===

// Include generated C++ header files (there are also corresponding .cpp files with implementations):
#include <Graphics/Tables/MeshLodInfoTable.h>
#include <Graphics/Tables/MeshGroupTable.h>
#include <Graphics/Tables/MeshInstanceTable.h>

struct MeshSceneModule final
{
	// Table instances. They are declared in the generated headers and inherit from DB::TableBase.
	// Instance uses base class methods to reallocate pages and update data based on the knowledge of its own memory layout.
	DB::TableInstance<MeshGroupTable> m_meshGroups;
	DB::TableInstance<MeshInstanceTable> m_meshInstances;

	void AddMesh(/* ... */)
	{
		m_meshGroups.Resize(1);
		m_meshInstances.Resize(1);

		// Writing sets dirty flags for the replication manager.
		const DB::TableRowWriteHandle<MeshGroupTable> groupHandle = m_meshGroups.WriteRow(0);
		const DB::TableRowWriteHandle<MeshInstanceTable> instanceHandle = m_meshInstances.WriteRow(0);

		groupHandle.m_geometry = GetGeometryBufferPointer(/* ... */);
		groupHandle.m_lods = Slice<MeshLodInfoTable>(0, 4);
		instanceHandle.m_meshGroup = Ref<MeshGroupTable>{ 0 };
		instanceHandle.m_transform = /* ... */;
	}
};

// === Usage in Shaders (HLSL) ===

// The declarations are not included in shaders directly, so MeshGroupTable etc. here came from generated HLSL code.

// Include generated shader files:
#include <Shaders/Tables/MeshLodInfoTable.hlsli>
#include <Shaders/Tables/MeshGroupTable.hlsli>
#include <Shaders/Tables/MeshInstanceTable.hlsli>

struct Constants
{
	uint32_t m_instanceIndex;

	// These are just buffer device address wrappers (uint64_t) pointing to page tables.
	MeshGroupTable::Instance m_meshGroups;
	MeshInstanceTable::Instance m_meshInstances;
};

[[vk::push_constant]] Constants GConstants;

void main()
{
	// Shaders are not permitted to write to tables.
	const uint32_t instanceIndex = GConstants.m_instanceIndex;

	// This operation calculates the page index based on the provided row index and the number of elements in page.
	// Then reads the corresponding buffer pointer from the page table.
	// Each field in RowHandle is a combination of the page pointer and the element index in that page.
	// We rely on compiler optimizations to avoid actually copying this data for each field.
	// I can't think of any other solution here since HLSL doesn't support references to local variables.
	const MeshInstanceTable::RowHandle instanceHandle = GConstants.m_meshInstances.ReadRow(instanceIndex);

	// Get method here uses RawBufferLoad to read the actual data.
	const float4x4 transform = instanceHandle.m_transform.Get();
	const DB::Ref<MeshGroupTable> meshGroupRef = instanceHandle.m_meshGroup.Get();

	// We can also index into tables using Ref<T> since it's just a uint32_t.
	// MeshGroupTable's ReadRow should only accept Ref<MeshGroupTable> for type-safety.
	const MeshGroupTable::RowHandle groupHandle = GConstants.m_meshGroups.ReadRow(meshGroupRef);
}
