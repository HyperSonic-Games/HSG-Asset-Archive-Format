
HyperSonic Games Asset Archive (HSGAA) API Documentation
========================================================

Overview
--------

The HyperSonic Games Asset Archive (HSGAA) provides a binary format for storing assets, such as images, with metadata. The format supports fast loading, compression, and a table of contents (TOC) for quick access to files.

C API
-----

### Constants

*   `MAGIC`: "AAF\\0" - The magic string that identifies the archive format.
*   `VERSION`: 1 - The version of the format.
*   `FLAG_ALL_COMPRESSED`: 0x01 - Flag indicating that all data in the archive is compressed.
*   `CHUNK_FLAG_COMPRESSED`: 0x01 - Flag indicating that an image chunk is compressed.

### Structures

#### HSGAAEntry

Defines an entry in the archive:

*   `name`: The name of the file in the archive.
*   `offset`: The byte offset of the image data in the archive.
*   `size`: The size of the compressed image data.

#### HSGAAArchive

Defines the archive structure:

*   `entries`: Array of file entries in the archive.
*   `entry_count`: The number of entries in the archive.
*   `image_chunks`: Array of pointers to compressed image data.
*   `image_chunk_sizes`: Array of sizes corresponding to each image chunk.
*   `image_chunk_count`: The number of image chunks in the archive.
*   `compress`: Flag to indicate if compression is enabled for this archive.

### Functions

#### hsgaa\_create\_archive

`hsgaa_create_archive(int compress)`

Creates a new empty archive.

**Parameters:**

*   `compress`: If 1, enables compression. If 0, no compression is applied.

**Returns:** A pointer to the newly created `HSGAAArchive`.

#### hsgaa\_add\_file

`hsgaa_add_file(HSGAAArchive* archive, const char* file_path)`

Adds a file to the archive.

**Parameters:**

*   `archive`: The archive to which the file will be added.
*   `file_path`: The path to the file to be added.

**Returns:** None. The file is compressed (if enabled) and added to the archive.

#### hsgaa\_write\_archive

`hsgaa_write_archive(HSGAAArchive* archive, const char* output_path)`

Writes the entire archive to a file.

**Parameters:**

*   `archive`: The archive to write.
*   `output_path`: The path where the archive will be written.

**Returns:** None.

#### hsgaa\_read\_archive

`hsgaa_read_archive(const char* archive_path)`

Reads an archive from a file.

**Parameters:**

*   `archive_path`: The path to the archive file.

**Returns:** A pointer to the `HSGAAArchive` read from the file.

#### hsgaa\_list\_entries

`hsgaa_list_entries(HSGAAArchive* archive)`

Lists all entries (files) in the archive.

**Parameters:**

*   `archive`: The archive from which to list the entries.

**Returns:** None. Prints the names of all files in the archive.

#### hsgaa\_extract\_file

`hsgaa_extract_file(HSGAAArchive* archive, const char* file_name, const char* output_path)`

Extracts a file from the archive.

**Parameters:**

*   `archive`: The archive from which to extract the file.
*   `file_name`: The name of the file to extract.
*   `output_path`: The path where the file will be saved.

**Returns:** None.

#### hsgaa\_free\_archive

`hsgaa_free_archive(HSGAAArchive* archive)`

Frees the memory used by the archive.

**Parameters:**

*   `archive`: The archive to free.

**Returns:** None.

Python API
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