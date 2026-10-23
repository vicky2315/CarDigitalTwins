# Review checklist

Every review / re-audit = `code-reviewer` agent (Opus, read-only), which applies this list verbatim; never a general-purpose agent
with a hand-written checklist. Brief adds scope only (commits, files, spec sections). Add a rule here, never only in a brief.

Sources of truth: `docs/SPEC.md` (schema §2, thresholds §3, connection states §4, MVVM §5–§6, budget §7), `docs/TESTS.md`
(test conventions), `docs/PROGRESS.md` (decisions, current checkpoint), [engineering-standards.md](../rules/engineering-standards.md).

| # | Rule | Check |
|---|---|---|
| 1 | Existing pattern first | Each new class/mechanism: project precedent exists? Copied (file:symbol named) or invented without saying so. Widgets reading ViewModels derive from `UTwinViewModelWidget`; colours/formatting from `TwinHmiColors` / `TwinHmiFormat` |
| 2 | Dependency direction | SPEC §6: receiver → `UTelemetrySubsystem` → ViewModels → widgets; car actor subscribes to `OnTelemetryUpdated`. Subsystems never know UI classes or call controller/widget methods; widgets never hold actor pointers (go through the player controller or a ViewModel). Commands go down (widget → ViewModel → subsystem), state comes back up; no optimistic widget display |
| 3 | Event-driven over polling | Every Tick / per-frame / timer / poll site reading another system's state or recomputing event-only data: justified (no event exists, or inherently per-frame: camera projection, car interpolation, fades, marquee scroll) or → ViewModel subscription / delegate. Dashboard values update per sample (10 Hz, SPEC §5), not per frame. Ticks enabled only while there is work |
| 4 | Optimization | Best algorithm on hot paths; no per-frame or per-sample allocations, `FString`/`FText` formatting or `FindComponentByTag`-style searches that could be cached; text set only when the value changed. Rendering budget SPEC §7 respected (no extra draw calls / materials per frame). Threads: none custom; engine async only |
| 5 | Camera never snaps | Every view change, follow, hand-off interpolates (`SetViewTargetWithBlend`, settings blend time; 0 = explicit opt-out) |
| 6 | Scripts | `Tools/UnrealEditor/*.py`: idempotent, check every save/load result, never claim a save that failed, correct asset paths, MANUAL fallback lines; Blueprint CDO / instance edits = MANUAL step. `Tools/Relay`, `Tools/TripGenerator`: errors handled, no blocking calls in async code, CLI flags documented |
| 7 | Docs | Touched docs: state once + link (SPEC owns contracts, PROGRESS owns status, TESTS owns test lists); no stale claims (built/passes/seen) contradicting git or code; no AI attribution |
| 8 | Parity (Python ↔ C++) | One contract, two languages: schema fields + `schemaVersion` (SPEC §2) in `trip_generator.py` / `relay.py` vs `FVehicleTelemetry` + receivers; thresholds + hysteresis (SPEC §3) vs `FVehicleStatusEvaluator` and any Python copy; commands (SPEC §2.5) vs `VehicleCommand.h` and relay; connection states (SPEC §4) vs `ETelemetryConnectionState` + WebSocket receiver |
| 9 | TEMPORARY marked | Stopgaps tagged TEMPORARY in code + listed in PROGRESS; nothing temporary passed off as final; dead code / unused members flagged |
| 10 | Standards | [engineering-standards.md](../rules/engineering-standards.md): file header comment, comment per member, `.h` order mirrored in `.cpp`, descriptive names, final newline, Unity-build name clashes (file-scope `static`/namespaces), null/config misses loud (`UE_LOG`), BindWidget vs BindWidgetOptional matches the WBP |
| 11 | Lifetime + threading | Subscribe/bind in `NativeConstruct` ↔ unsubscribe in `NativeDestruct` (pairing); subsystem delegates removed in `Deinitialize`; lambdas capturing `this` across async boundaries use weak pointers; timers cleared; socket callbacks only queue, game thread does parsing + state (SPEC §4); `TObjectPtr`/`UPROPERTY` on every `UObject*` member that must survive GC |
| 12 | Tests | TESTS.md conventions: `CarDigitalTwins.` names, no Jeep assets, temp files cleaned, assert only own setup, restore global state (settings, CVars), no order dependence, claims in TESTS.md match assertions; headless-only gaps named as PIE checks |
| 13 | Spec + decision fidelity | Matches SPEC / PROGRESS decisions / UI mock + build page; deviations flagged, not silently chosen |

## Output contract
- One section per rule number; a rule with no finding states "Rule N: checked, nothing found" (silence = skipped rule).
- Finding: severity (Must-fix / Should-fix / Nit), `file:line`, issue, precedent `file:line` (or "no precedent"), concrete fix.
- Rule 3: full per-frame site table (site | reads | justified? | fix).
- End: prioritized fix list (a) fold into current change, (b) separate refactor step, (c) backlog; then a one-line verdict.
