# ShyGuy Roadmap & Task Breakdown

This document tracks current state, gaps, and an actionable plan to evolve ShyGuy (the server-side component) toward a DAG orchestrator in C++.

## Gaps & Notable Issues
- Networking server not implemented: no ZMQ router/dealer in `shyguy` for receiving requests or responding.
- JSON adapter bug: `from_json` for `shyguy_request` reads from the whole object instead of `j["value"]`.
- Task/DAG execution path incomplete:
  - `concurrent_shyguy::execute(dag)` enqueues task runners but returns `not_currently_supported` and uses a placeholder command.
  - `concurrent_shyguy::execute(task)` unimplemented behavior.
  - `shy_executioner` constructor is private by default; make it constructible as needed when integrating.
- Scheduling stubbed: `next_scheduled_dag()` returns a constant placeholder; no cron parsing or wakeup thread.
- Content paths: `prefix_folder()` uses `getenv("/opt")` on non-Windows (invalid). Should use a sensible base (e.g., XDG dirs or `/var/lib/cosmos/shyguy`).
- Name uniqueness check for tasks likely inverted: `has_unique_name(shyguy_task)` returns `dag.contains(name)` instead of `!contains(name)`.
- TUI wiring: `interactive_shyguy` enqueue/actions are placeholders and not connected to actual create/run/remove requests.
- Persistence on startup: no manifest scanning to rebuild DAG/task registry.
- Tests: no unit/integration tests yet.

## Milestones & Tasks (Prioritized)

### M0: Quick Stabilization (Bugs/Fixes)
- [ ] Replace non-Windows `prefix_folder()` with a correct directory strategy 
  - Choose storage roots:
    - Windows: `%APPDATA%/cosmos/shyguy`.
    - Linux/macOS: `$XDG_DATA_HOME/cosmos/shyguy` or fallback to `~/.local/share/cosmos/shyguy`.
    - Run folders under `.../runs/<dag>/<date>/<uuid>`; content under `.../content/...`.

### M1: Command Routing & Networking
- [ ] Define wire format (JSON) for commands: `{ command: "create|remove|execute|snapshot", type: "dag|task", value: {...} }`.
- [ ] Implement ZMQ Router in `shyguy::run()` to accept requests and return results.
- [ ] Add request parsing → `shyguy_request` + `command_enum` mapping.
- [ ] Connect to `concurrent_shyguy::process()` and return `command_result_type` to client.
- [ ] Implement a minimal client in `src/babyLuigi` (optional now; CLI or example script).

### M2: Execution Flow Integration
- [ ] Implement `concurrent_shyguy::execute(shyguy_dag)` fully:
  - [ ] Load task content for each task, set `task_runner::task_function` based on `task.type` (python/shell).
  - [ ] For python/shell: write content to a temp file in the run folder; execute via `system_execution::execute_command` with the right interpreter; capture output.
  - [ ] Enqueue `{runners, dag}` for `shy_executioner`.
- [ ] Start a dedicated executioner thread in `shyguy::run()` that:
  - [ ] Owns a `shy_executioner` with a `running` flag and the shared `request_queue`.
  - [ ] Calls `dispatcher()` until shutdown.
- [ ] Ensure logging around start/finish and error conditions per task.

### M3: DAG/Task CRUD Completeness
- [ ] Validate DAG/task uniqueness and error codes consistently.
- [ ] On task creation with `file_content`/`filename`, persist under content folder and record in dag manifest.
- [ ] Implement `remove(task)` to also delete stored task content file if present.
- [ ] Implement `snapshot(dag|task)` stubs to return `not_currently_supported` with a clear message (or initial basic behavior).

### M4: Scheduling
- [ ] Parse cron expressions using existing dependency; validate on DAG creation.
- [ ] Maintain `schedules` map and implement next- [ ]run calculation.
- [ ] Implement a scheduler thread with `notify_updater` to wake at next scheduled times and enqueue `execute(dag)`.

### M5: Interactive TUI (ftxui)
- [ ] Wire create DAG/task flows to enqueue appropriate requests.
- [ ] Show DAG list, task lists, and latest run results/status per task.
- [ ] Add triggers for run/remove and show logs (tail).

### M6: Persistence & Recovery
- [ ] On startup, scan `content` for `dag_manifest.json` and task files to reconstruct in- [ ]memory DAGs and tasks.
- [ ] Validate manifests, handle missing content gracefully, log warnings.
- [ ] Provide a reindex command.

### M7: Testing & Tooling
- [ ] Unit tests: graph (cycles, topological order), request parsing/serialization, `concurrent_shyguy` CRUD, scheduling next- [ ]run calc.
- [ ] Integration: small DAG (2–3 tasks) end-to-end run via queue without networking.
- [ ] Optional: basic network integration test with inproc ZMQ.
- [ ] Add CI (GitHub Actions) with matrix for compilers.

### M8: Docs & Examples
- [ ] README: architecture, folder layout, how to run server and send requests.
- [ ] Example DAGs and tasks (python/shell) with scripts.
- [ ] Troubleshooting guide (permissions, paths, ZMQ issues).

## Nice-to-Haves
- [ ] Pluggable executors (local process, container, remote worker via ZMQ).
- [ ] Task caching and outputs/artifacts tracking.
- [ ] Retries, timeouts, SLAs.
- [ ] Metrics and health endpoints.
- [ ] Authentication/authorization for network commands.

## Suggested Next Steps (this week)
1) M0 fixes: JSON adapter, name uniqueness, path base.
2) M2 execution flow: run a single DAG end-to-end without networking (manual enqueue) to prove engine.
3) M1 networking: add ZMQ Router and parse/dispatch commands.

## Notes
- Code references of interest:
  - `include/graph/graph.hpp:1`
  - `include/shyguy_request.hpp:1`
  - `src/shyGuy/concurrent_shyguy.hpp:1`
  - `src/shyGuy/concurrent_shyguy.cc:1`
  - `src/shyGuy/shyexecutioner.hpp:1`
  - `src/shyGuy/shyexecutioner.cc:1`
  - `src/shyGuy/shyguy.hpp:1`
  - `src/shyGuy/shyguy.cc:1`

