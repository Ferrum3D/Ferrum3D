# CPU mesh culling and updates

The spatial index lives in `Core/Math/Octree.h` and has no graphics dependencies.
Mutation is single threaded; queries read an immutable tree. Entries are caller-owned
and must keep stable addresses while inserted. Remove entries before releasing them.

## Storage and allocation

Each node owns its own AABB. On x64 the node is 64 bytes, down from 224 bytes with
eight child AABBs embedded in every node, including leaves. Children remain an
eight-node pooled allocation: sibling locality and a single allocation per split
are useful, while allocating each child separately adds pointers and allocator work.
Empty nodes allocate no entry storage.

Entries use pooled blocks of eight bounds in AVX SoA form, with eight stable entry
pointers. The SoA bounds are authoritative; entries carry only a node pointer,
block pointer, lane and user index (24 bytes), avoiding a second persistent AABB.
Blocks are packed except for the head. Removal swaps the head's last entry into
the removed lane, fixes its location, and returns empty blocks to the pool.
This replaces the previous power-of-two pointer arrays and eight size-class pools
with two pools: child groups and entry blocks. Pool pages are reused until teardown.

SIMD storage is a deliberate tradeoff: a full block is 288 bytes, or 36 bytes per
entry including pointers and block metadata. Together with the 24-byte entry this
is 60 bytes, versus the previous 48-byte entry plus an 8-byte pointer. Node storage
is substantially smaller; total memory still depends on occupancy and tree shape.

Leaves target 64 entries, merge below 32, and stop subdividing at depth 12. Boxes
crossing split planes and coincident boxes use overflow blocks. A strict total
entry limit per node cannot represent these cases without duplication or an
additional overflow structure. The eight-entry block limit bounds SIMD work and
allocation size instead. Split/merge hysteresis avoids oscillating around one
threshold. Updates retain valid placement or reinsert from the root so entries
can descend again after moving. Root overflow is tested independently of nominal
root bounds, including queries entirely outside the root.

## Culling and upload

`Frustum` extracts six inward clip planes once, using row-vector transforms and
depth in [0,w]. Support-point AABB tests handle reversed Z and zero-normal infinite
planes without inverse projection or corner transforms. This is conservative
plane culling, not a full frustum/AABB separating-axis test. SoA blocks test eight
entry bounds at once; child nodes use scalar tests against their own bounds.

Parallel traversal builds a bounded frontier of at most 64 disjoint ranges and
combines small subtrees into batches targeting 64 entries. Subtree counts are
maintained on mutation, and nodes remain 64 bytes using compact block pointers.
Small scenes stay inline. Traversal joins before returning. Large local overflow lists are also partitioned, so
split-plane crossings do not force all culling onto one worker. Task-indexed
output vectors retain capacity between views and frames. No worker appends to
shared output or mutates the render graph. Small single-task queries run inline.

A prefix sum assigns each output list a disjoint range in one mapped uploader
reservation. Workers copy larger outputs directly into that reservation; small
outputs copy inline. Reservation, flush and graph registration run on the render
fiber. There is one dispatch-buffer upload and no intermediate concatenated array.

## Transform extraction

MeshSystem records a changed main-thread model/material query, followed by a
dependent parallel transform query with its own change cursor. Lifecycle work
and membership changes complete before any transform jobs. Workers update
distinct preallocated database transform rows through stable cached addresses
and set per-instance dirty bytes; they never touch shared database or batch bits.
Concurrent writes to the same mesh handle are unsupported.

After ECS work joins, MeshSceneModule publishes dirty rows/batches serially,
calculates dirty batch bounds in up to eight jobs, and mutates the octree serially.
Batch bounds are rebuilt from invalid bounds, allowing them to shrink. Mesh-group
local bounds are cached and refreshed with asset generations. Dirty lists,
temporary bounds and culling output reuse their capacity. No component addresses
are retained across ECS scheduling.

The split thresholds, frontier size and job grains are initial tuning choices,
not measured performance claims. GameSample's five levels contain 333 helmets
and 167 bunnies with three material variants, for 500 rotating mesh entities.

## Query granularity and profiler interpretation

Parallel ECS queries divide each storage chunk into ranges of at most 32 rows.
Chunk size remains a storage choice and no longer sets the minimum parallel task
size. Range jobs publish a shared chunk's changed flag atomically, and the query
publishes versions and its change cursor only after joining all ranges.

Parallel cascades process at most 16 entities per task within one hierarchy level,
then join that level before visiting children. This allows a single broad tree to
use several workers. A single ancestor chain retains its required serial order.
Sequential/main-thread cascades preserve their depth-first walk. Breadth-first
entity order and level offsets are cached until hierarchy or storage structure
changes; concurrent queries share that immutable plan. Small single-range work
runs on the coordinator without another allocation or dispatch.

`ExecuteTraversal` is a coordinator zone and includes preparation and join time.
It cannot show worker utilization by itself. `QueryRows` and `CascadeRows` zones
show callback work on each executing worker, with the system name and range in
their text. `JoinQueryJobs` and `JoinCascadeLevel` show coordination separately.
`OctreeTraversalTask` and `MeshWorkRange` expose the culling/bounds worker work.
GameSample's smoke completion message reports scheduler job, cascade batch and
callback counts for its final frame. Compare these worker zones in a fresh
capture; the previous trace did not instrument the subdivision jobs.

## Materialization budget

Runtime construction and initial loading share a quota of 32 entity steps per
frame; initialization and activation share a separate quota of 32 distinct
entities. Validation and UUID/schema discovery run once per operation. Large
collections retain construction/loading cursors and stay pending until complete.
An entity initialized and activated in the same frame consumes one lifecycle
admission. Already completed work is retained, and a pending activation is distinct
from failure. A new subtree is published only after all its activations succeed.
Cancellation and failure unwind partial membership and prepared graphics objects.

`EntityWorld::BeginFrame` opens these quotas for all world updates/commits in one
rendered frame; Application calls it once in its frame loop. Standalone worlds
that omit the frame hook reset quotas at each update. Explicit synchronous
bootstrap commits and snapshot row restoration retain their existing immediate
construction contract. Pending operations rotate after exhausting the construction
quota so a large collection cannot monopolize construction indefinitely.
