
HyperSonic Games Asset Archive (HSGAA) API Documentation
========================================================

Overview
--------

The HyperSonic Games Asset Archive (HSGAA) provides a binary format for storing assets, such as images, with metadata. The format supports fast loading, compression, and a table of contents (TOC) for quick access to files.

API
----------

### Constants

*   `MAGIC`: "AAF\\0" - The magic string identifying the archive format.
*   `VERSION`: 1 - The version number of the archive format.
*   `FLAG_ALL_COMPRESSED`: 0x01 - Flag indicating all data in the archive is compressed.
*   `CHUNK_FLAG_COMPRESSED`: 0x01 - Flag indicating an image chunk is compressed.

### Classes

#### HSGAAEntry

Represents an entry for a file in the archive:

*   `name`: The name of the file.
*   `offset`: The offset of the file data in the archive.

#### HSGAAArchive

Represents the archive structure:

*   `entries`: List of `HSGAAEntry` objects.
*   `compress`: Flag to indicate whether compression is enabled for the archive.

### Methods

#### HSGAAArchive.add\_file(file\_path)

Adds a file to the archive.

**Parameters:**

*   `file_path`: The path to the file to be added.

#### HSGAAArchive.write(output\_path)

Writes the archive to a file.

**Parameters:**

*   `output_path`: The path where the archive will be saved.

#### HSGAAArchive.list\_entries()

Lists all entries in the archive.

**Returns:** A list of file names in the archive.

#### HSGAAArchive.extract(name, out\_path=None)

Extracts a file from the archive.

**Parameters:**

*   `name`: The name of the file to extract.
*   `out_path`: Optional path where the file will be saved. If not specified, the file is returned as bytes.

# Format Spec
## VERSION: 1
```
+--------------------+  <-- Header
| Magic: "AAF\0"     |  // 4 bytes
| Version (u8)       |  // 1 byte, e.g., 1
| Flags (u8)         |  // Global flags (bitfield)
| TOC Offset (u64)   |
+--------------------+

+---------------------------+  <-- Raw Image Blobs
| [Image Chunk 0]           |
| [Image Chunk 1]           |
| ...                       |
+---------------------------+

Each Image Chunk:
+----------------------------+
| Signature (u32)           | // CRC32 or Adler32
| Flags (u8)                | // per-image: compressed, mipmapped, etc.
| Original Size (u32)       |
| Compressed Size (u32)     | // 0 if uncompressed
| Blob Data [...]           |
+----------------------------+

+---------------------------+  <-- TOC (Table of Contents)
| Entry Count (u32)         |
| [TOC Entry 0]             |
| [TOC Entry 1]             |
| ...                       |
+---------------------------+

Each TOC Entry:
+-----------------------------+
| NameHash (u32)             |
| Offset (u64)               |
| Size (u32)                 | // Size of the chunk (not raw image size)
| NameLen (u8)               |
| Name (UTF-8, variable)     |
+-----------------------------+
```
