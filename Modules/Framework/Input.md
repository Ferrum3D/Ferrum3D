# Input

`FeFramework` exposes device-independent actions under `FE::Input`. The application polls its desktop backend before `EntityWorld::BeginUpdate()`. Register `WorldTimeService` before `WorldInputService`.

For a sampled backend, each world consumes every published poll interval exactly once. The backend interval is the input clock, including held-action integration and camera rate movement. A repeated world update against the same poll publishes zero displacement, advances no timers, and keeps held values readable. Skipping a poll sequence is unsupported and asserted: this backend is a current snapshot, not a retained event timeline. Custom unsampled backends use world elapsed time.

Windows queued events retain `MSG.time`, converted from wrapping millisecond uptime to the monotonic clock. This assumes queued messages are younger than half the 32-bit uptime range. Synchronously generated messages use the current monotonic time. A negative event timestamp explicitly means an untimed synthetic event; it takes effect at poll end. The desktop backend has one explicit ProcessEvents(events, deltaSeconds, frameEndSeconds) entry point, called by platform polling. Unrelated platform events, repeated key states, and unchanged mouse positions do not split input intervals. Mouse positions are signed 32-bit screen coordinates.

World assets contain generic `WorldServiceDescriptor` values. `WorldInputService::Configuration` is `InputConfiguration`; its `Configure` and `CaptureConfiguration` methods use that concrete type. `WorldServiceDescriptor::Create<WorldInputService>(configuration)` checks the C++ type. Reflection performs the single erased configuration check when loading and captures current active contexts for snapshots. Context definitions, residency, and activation handles belong to the service; the world knows no input-specific fields.

```cpp
const auto gameplay = input.Activate(gameplayTable, 0);
const auto menu = input.Activate(menuTable, 100, true);
input.Deactivate(menu);
const Input::ActionReference<Vector2> move(moveActionLink);
const Vector2 value = input.GetFrame().Read(move);
```

Activation returns a unique context-instance handle. Higher priorities win; newer activations break ties. Context mutations apply on the next input update, leaving the current published frame unchanged. The stack owns the frame's values, event list, and frozen authored-ID index. Reads do not allocate, bind, touch the asset manager, or mutate the frame. Removed actions are compacted during preparation.

`ActionReference<T>` stores one authored `IO::Link<Action>` and carries its expected value type. Its serialization preserves Link's wire representation and hard dependencies. Cameras declare move, elevation, and look references once and read them directly; there is no entity-keyed binding cache. Null, absent, inactive, and incompatible values read as zero.

`Action` and `MappingTable` are source-less assets. Action assets define the authoritative value type. Each mapping has alternative bindings with an explicit variant payload: button, named negative/positive axis, named left/right/back/forward axis, mouse delta, wheel, or chord. Conditions are held-control gates. Chords require simultaneous held controls. Sum combines participating alternatives; Strongest uses the largest actuation magnitude and the first participating alternative on ties.

Direction normalization occurs before sensitivity and optional rate integration. Binding scales shape the direction; sensitivity converts it to the intended rate or displacement. Integrated and non-integrated bindings cannot share a row. Mouse displacement cannot use direction clamping. This keeps results invariant when an interval is subdivided. Mouse look sensitivity belongs to the authored binding.

Override, consumption, and modal blocking are separate policies. The highest context defining an action owns it even while inactive. A consuming mapping reserves participating physical controls from lower contexts, including opposed controls whose values cancel and held rate controls at zero elapsed time. Conditions consume only when their primary binding participates. Strongest consumes only its selected alternative. Mappings within one table share controls. A modal context blocks all lower contexts, including while loading. Replacement cancels the old interaction before starting the replacement.

Values and notifications are separate. Started, Triggered, Completed, and Canceled are ordered events; InProgress is internal state. Default processing triggers once on activation. Continuous processing publishes at most one Triggered notification per action per frame, while retaining activation and release edges. Hold processing triggers once at its threshold and cancels early releases. Short taps preserve events while final held values are zero.

Validation runs at asset building and generation preparation, using an optional generic reflected validator. Processors validate their own configuration. Stable updates reuse compiled mappings and processor classification.

Processors are immutable asset configuration; timers belong to each world resolver. Concrete processor classes override `DoRelease` using the engine deletion helper, including offset-base objects. Every concrete polymorphic serializable class must declare and register its own engine RTTI and be default constructible. An unregistered derived class inheriting a base's RTTI is unsupported programmer misuse; inherited metadata cannot detect that mistake without language RTTI. `Rc<T>` serializes inline polymorphic values and does not preserve shared identity or cycles.

Conditional writable queries explicitly return `Framework::QueryWriteResult::kChanged` or `kUnchanged`. The latter promises no writable data changed. Other callbacks retain unconditional write publication. Ordinary chunks aggregate changes locally; cascade trees synchronize publication for shared chunks. Read-only traversal performs no changed-state atomic operation. Idle cameras do not publish transform changes.

`FE_RTTI_SerializeFields()` generates field serialization and schema helpers for classes needing a manual outer contract, such as EntityReference remapping. It does not generate before/after serialization hooks. Inherited serializers remain usable through intermediary reflected bases.
