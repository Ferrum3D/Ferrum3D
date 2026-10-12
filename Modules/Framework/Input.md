# Input

`FeFramework` exposes device-independent actions under `FE::Input`. The platform application captures keyboard, mouse, and ordered timestamped transitions once per poll. Every world reads the same application snapshot, or supplies its own `Core::Backend` for testing or other devices. Poll before `EntityWorld::BeginUpdate()`.

Register `Framework::WorldTimeService` before `Input::WorldInputService`. The input service owns context residency, priorities, interaction state, and one immutable `InputFrame` per update. Actions resolve even if no gameplay system reads them.

World assets describe services generically with `WorldServiceDescriptor { m_type, m_configuration }`. Input contexts live in the input service's `InputConfiguration`, stored in the descriptor's `Rtti::Any`. The world default-constructs each service, calls `Configure`, and then `Init`; it has no knowledge of input. Snapshot capture calls each service's `CaptureConfiguration` and preserves active input contexts without serializing held keys or interaction timers.

```cpp
const auto gameplay = input.Activate(gameplayTable, 0);
const auto menu = input.Activate(menuTable, 100, true); // modal
input.Deactivate(menu);
const auto move = input.Bind<Vector2>(moveActionLink);
const Vector2 value = input.GetFrame().Read(move);
```

Activation is enabled by default and returns a unique context-instance handle. Higher priorities win; newest activation breaks ties. Mutations become visible on the next input update. The current frame stays immutable. Residency requests belong to the service, so callers do not need to retain a separate request.

Action override, consumption, and modal blocking are separate policies. The highest context defining an action owns it even while inactive. A mapping's `m_consume` reserves its active controls from lower contexts; mappings in one table share controls. A modal context blocks every lower context, including while it is loading. Replacement cancels the old interaction before the new mapping may start in the same frame.

`Action` and `MappingTable` are source-less assets. Each table has one row per action, with an expected value type and alternative bindings. Bindings describe `Button`, `Axis1D`, `Axis2D`, `MouseDelta`, `MouseWheel`, or `Chord` composites. `m_conditions` applies reusable held-control gates, e.g. Ctrl or RMB. `(Ctrl+K) OR F1` is a conditioned button plus an alternative F1 button. Chords require overlapping held states rather than remembered press edges.

Axis1D controls are negative/positive. Axis2D controls are left/right/back/forward. `Binding::WASD()` and `ArrowKeys()` provide common presets. Alternative values sum or select the strongest magnitude; `m_clampMagnitude` limits direction length to one. Scale, sensitivity, and optional held-axis rate integration live in bindings. Mouse look in the sample produces angular displacement in radians, so cameras have no mouse sensitivity or device assumptions. Held and displacement bindings cannot be mixed in one action row.

Asset building validates duplicate rows, controls, composite/value compatibility, and finite parameters. Mapping/action type mismatches and typed-handle mismatches receive runtime binding diagnostics; incompatible or missing actions read as zero. Mappings compile to dense action indices when table generations change. Typed runtime handles read dense slots in constant time and reject handles from another world or an expired service lifetime.

Current values are separate from ordered `Started`, `Triggered`, `Completed`, and `Canceled` events. A null processor triggers once on activation; continuous input triggers while active and completes on release; a hold triggers once at its duration, completes on release after triggering, or cancels on an early release. Several transitions may occur in one frame. `InProgress` is internal interaction state, not a notification. Short taps preserve events while final held values are zero. Timestamped samples limit hold timing to the actual activation interval and apply mouse gates to each motion sample. Synthetic events without timestamps occur at frame end, conservatively adding no invented hold duration.

Custom polymorphic processors remain supported as immutable asset configuration; runtime timers belong to the world resolver. Processor release destroys through the registered dynamic type and frees the complete object address, including offset bases. `Rc<T>` remains inline polymorphic serialization, without preserving shared identity or cycles.

Camera movement components retain authored `IO::Link<Action>` fields. The system caches typed handles per camera, reads three values, and reports whether it changed the transform. Query callbacks returning `bool` opt into conditional change publication: returning false must mean no writable data was changed. Void callbacks retain unconditional write publication for matched entities. Idle cameras therefore avoid downstream transform propagation and graphics extraction.
