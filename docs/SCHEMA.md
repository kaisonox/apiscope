# Event Schema

ApiScope renders each trace event as human-readable text or as one JSON object
per line (JSONL). Both formats are produced from the same parsed event and carry
the same information; only the presentation differs.

- JSON schema version: **1** (emitted as `schema_version` on every JSONL event)
- Binary event protocol version: **6** (`TRACE_EVENT_VERSION`)

## JSONL envelope

```json
{
  "schema_version": 1,
  "sequence": 3,
  "timestamp": "2026-06-16T07:28:26.4793503Z",
  "timestamp_100ns": 134260685064793503,
  "thread_id": 10064,
  "module": "ntdll.dll",
  "api": "NtWriteFile",
  "hook": "ntdll.dll!NtWriteFile",
  "truncated": false,
  "field_error": false,
  "fields": { }
}
```

`fields` holds the hook-local values. Each value is encoded by its type.

## Field value encodings

| Type | Text | JSON |
|------|------|------|
| uint32 / uint64 | decimal | number |
| int32 / int64 | decimal | number |
| boolean | `true` / `false` | `true` / `false` |
| pointer / handle | `0x` + 16 hex digits | string, `0x` + 16 hex digits |
| status (NTSTATUS) | `0x` + 8 hex digits | string, `0x` + 8 hex digits |
| wide string | UTF-8 text | string (UTF-8) |
| bytes | parallel lines (see below) | object (see below) |

Pointers and statuses are hex **strings** in JSON, not numbers, so 64-bit values
stay exact in parsers that use doubles and read the way Windows developers
expect (`0xC0000022`).

## Bytes fields

A captured buffer carries a bounded preview plus capture metadata. JSON nests
the parts under the field; text flattens them with matching suffixes.

JSON:

```json
"buffer": {
  "type": "bytes",
  "requested": 4096,
  "captured": 64,
  "status": "0x00000000",
  "hex": "48656C6C6F",
  "ascii": "Hello"
}
```

Text (suffixes mirror the JSON keys):

```text
    buffer_hex    : 48 65 6C 6C 6F ...
    buffer_ascii  : Hello...
    buffer_size   : 64 of 4096 bytes
    buffer_status : 0x00000000
```

- `requested` is the caller's length; `captured` is how many bytes were safely
  read (bounded by the preview size). When `captured < requested` the text view
  appends `...`; the `<name>_size` line always reports both counts.
- `ascii` is a lossy preview: bytes outside printable ASCII render as `.`.
- `status` is the NTSTATUS from reading the buffer (`0x00000000` is success).

## Notes

- The text format prints `truncated` / `field_error` lines only when those event
  flags are set; JSON always includes them as booleans.
- `path` on read and write events is added by handle correlation and may be
  stale if a handle was closed and reused (see the README limitations).
- Unknown future field types render as `type_<n> <hex>` (text) or
  `{"type":"unknown_<n>","hex":"..."}` (JSON), so older readers tolerate newer
  events.
