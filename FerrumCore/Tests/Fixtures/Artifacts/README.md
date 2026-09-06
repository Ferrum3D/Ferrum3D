# Ferrum artifact metadata schema, version 1

Metadata is UTF-8 JSON stored at `artifacts/<asset[0:2]>/<asset[2:4]>/<asset>.meta.json` beneath the configured store root.
The store snapshots the configured root as an absolute path. Configuration must remain fixed while loads are in flight.
Metadata uses Core `JsonFormat`: its top-level `$version` is `1`, while `$schema` is the stable schema hash shown below. The
`$value.schema` discriminator is `ferrum-artifact`; `platform` must match the configured platform. `asset`,
`artifact`, `type`, and dependency `asset`/`type` values are canonical hyphenated UUIDs. `type` values name registered RTTI IDs.

`dependencies` contains `{ "asset", "type", "kind" }` objects, where `kind` is `hard` or `soft`. `payloads[0]` is the
required serialized-object payload. Each payload names an optional safe store-relative `source`; omitting it selects the
artifact-addressed default data path. `offset` and `size` bound the payload in that file. Each chunk contains `offset`
relative to the payload, `compressedSize`, `uncompressedSize`, `compression` (`none`, `deflate`, or `zstd`), and a numeric
CRC-32 `checksum` covering the uncompressed chunk bytes. Chunks are concatenated in array order and must contiguously cover
the complete compressed payload range.

Metadata is limited to one MiB, 4,096 dependencies, 256 payloads, and 65,536 chunks per payload. A decoded payload is limited
to `UINT32_MAX` bytes to match the current Core vector and initial-read interfaces. Core `JsonFormat` ignores unknown object
fields for forward compatibility. The simple fixture has a complete four-byte on-disk payload used by asynchronous read and
checksum tests. The chain, diamond, cycle, and soft-link fixtures are metadata-only graph fixtures for later discovery tests;
their described payloads are not yet generic-object serialization output.

```json
{
  "$type": "00000000-0000-0000-0000-000000000000",
  "$version": 1,
  "$schema": "0x6172746966616374",
  "$value": {
    "schema": "ferrum-artifact",
    "platform": "windows-x64",
    "asset": "11111111-1111-4111-8111-111111111111",
    "artifact": "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
    "type": "334f0750-1b4e-4f4c-ac6f-985382d4bd11",
    "dependencies": [],
    "payloads": [{
      "offset": 0,
      "size": 4,
      "chunks": [{ "offset": 0, "compressedSize": 4, "uncompressedSize": 4, "compression": "none", "checksum": 3019424693 }]
    }]
  }
}
```
