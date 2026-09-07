# Ferrum artifact metadata

Metadata is UTF-8 JSON stored at `artifacts/metadata/pc/<asset[0:2]>/<asset[2:4]>/<asset>.meta` beneath the configured
store root. Default artifact data is stored at `artifacts/data/<artifact[0:2]>/<artifact[2:4]>/<artifact>.bin`.
It is the JSON serialization of `ArtifactRecord`, including the standard Core serialization envelope. The current generated
version is `0`; `$schema` is the generated serialization schema hash. UUID values remain readable strings, while
`DependencyKind` and `Compression::Method` are serialized as their numeric underlying values.

Each payload contains a `ResolvedDataSource`. Its path is either empty, selecting the artifact-addressed default data file,
or a safe store-relative path. Its byte offset and size bound the payload in that file. Chunks use offsets relative to the
payload and must contiguously cover the complete compressed source range. A chunk checksum covers its uncompressed bytes.

Metadata is limited to one MiB, 4,096 dependencies, 256 payloads, and 65,536 chunks per payload. A decoded payload is limited
to `UINT32_MAX` bytes. The simple fixture has a complete four-byte on-disk payload used by asynchronous read and checksum
tests. The remaining fixtures describe dependency graphs for later streaming stages.

```json
{
  "$type": "bfcd80e4-db85-41f2-b43d-f08ea62aa6ec",
  "$version": 0,
  "$schema": "0x4d3ae890a4a739c5",
  "$value": {
    "m_artifactId": "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
    "m_assetId": "11111111-1111-4111-8111-111111111111",
    "m_assetTypeId": "334f0750-1b4e-4f4c-ac6f-985382d4bd11",
    "m_payloads": [{
      "m_resolvedDataSource": { "m_filePath": "", "m_byteOffset": 0, "m_byteSize": 4 },
      "m_chunks": [{
        "m_offsetInPayload": 0,
        "m_compressedSize": 4,
        "m_uncompressedSize": 4,
        "m_compressionMethod": 0,
        "m_checksum": { "m_current": 3019424693 }
      }]
    }],
    "m_dependencies": []
  }
}
```
