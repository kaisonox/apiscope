# Roadmap

## v0.1.0: Credible Demo

- Reproducible Windows x64 build
- Windows CI and runtime smoke test
- Bounded logging for `NtCreateFile`, `NtReadFile`, and `NtWriteFile`
- Conservative trampoline validation

## v0.2.0: Trace Pipeline

- Import-free injected DLL contract with build-time validation
- Nonblocking named-pipe event transport with drop counting
- Unpatched `NtWriteFile` and `NtReadVirtualMemory` transport bypass trampolines
- Self-describing binary TLV events for hook-local schemas
- Hook-entry timestamps and originating thread IDs
- Readable terminal rendering with optional text or JSONL file tee and `--quiet`
- Structured `run --hook ... -- <program> [args...]` CLI and `--hook all`
- Unicode target paths and forwarded target arguments

## v0.3.0: Attach Workflow

- Short CLI aliases and `--version`
- Attach to an existing PID with a suspended transactional setup window
- Real multithreaded launch and attach smoke coverage

## v0.4.0: ApiScope

- Hard rename to ApiScope
- Module-qualified, self-describing hook descriptors
- Unified debugger event loop for launch and attach
- Delayed DLL hook installation and unload tracking
- Clean Ctrl+C restoration and debugger detach
- Generic trace protocol v5
- `bcrypt.dll!BCryptOpenAlgorithmProvider` sample hook

## v0.5.0: Paths, Correlation, And Live Output

- File and registry path capture from `OBJECT_ATTRIBUTES`
- Handle-to-path correlation across reads, writes, registry access, and close
- `NtOpenFile`, `NtClose`, and registry key hooks (`NtOpenKey`, `NtSetValueKey`, `NtQueryValueKey`)
- Normalized text and JSON field encodings with a versioned JSON schema
- Human-readable NTSTATUS names, TTY color, a live status footer, and a session summary
- Larger trace ring to reduce dropped events under bursty load

## Next

- Path and API filters
- Interactive session console to add or remove hooks on a live target
- Configurable trace ring capacity or lossless mode to reduce dropped events
- Metadata-rich hook registry for categories and default filters
- Additional NT and Win32 APIs
- Ordinal forwarder support
- Relocation of RIP-relative and relative-control-flow trampoline instructions
