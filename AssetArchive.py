import struct
import os
import binascii
import zlib

# Constants for the AAF format
MAGIC = b'AAF\x00'  # Magic identifier for the archive
VERSION = 1  # Version number of the archive format

# Flags for compression and chunk handling
FLAG_ALL_COMPRESSED = 0x01
CHUNK_FLAG_COMPRESSED = 0x01

def hash32(name: str) -> int:
    """
    Hashes the file name to a 32-bit integer using CRC32.

    Args:
        name (str): The file name to hash.

    Returns:
        int: The CRC32 hash of the file name.
    """
    return binascii.crc32(name.encode('utf-8')) & 0xFFFFFFFF

class HSGAAEntry:
    """
    Represents an entry in the HSGAA archive.

    Attributes:
        name (str): The name of the file.
        offset (int): The offset of the file data in the archive.
        size (int): The size of the file data in the archive.
    """
    def __init__(self, name: str, offset: int, size: int):
        self.name = name
        self.offset = offset
        self.size = size

class HSGAAArchive:
    """
    Represents an HSGAA archive that can store multiple files with compression.

    Attributes:
        entries (list): List of entries (files) in the archive.
        image_chunks (list): List of image chunks stored in the archive.
        compress (bool): Flag to indicate whether to compress files.
    """
    def __init__(self, compress: bool = True):
        """
        Initializes a new HSGAA archive.

        Args:
            compress (bool): Whether to use compression for the files. Defaults to True.
        """
        self.entries = []
        self.image_chunks = []
        self.compress = compress

    def add_file(self, file_path: str):
        """
        Adds a file to the archive. The file is optionally compressed.

        Args:
            file_path (str): The path to the file to add.
        """
        name = os.path.basename(file_path)
        with open(file_path, 'rb') as f:
            raw_data = f.read()

        chunk_flags = 0
        compressed_data = raw_data
        if self.compress:
            compressed_data = zlib.compress(raw_data, level=9)
            chunk_flags |= CHUNK_FLAG_COMPRESSED

        # Calculate the signature and chunk header
        signature = binascii.crc32(raw_data) & 0xFFFFFFFF
        chunk_header = struct.pack('<IBII', signature, chunk_flags, len(raw_data), len(compressed_data))
        chunk = chunk_header + compressed_data

        # Offset calculation: header + previous chunks
        offset = 4 + 1 + 1 + 8 + sum(len(c) for c in self.image_chunks)
        self.image_chunks.append(chunk)
        self.entries.append((hash32(name), offset, len(chunk), len(name), name.encode('utf-8')))

    def write(self, output_path: str):
        """
        Writes the archive to a file.

        Args:
            output_path (str): The file path to write the archive to.
        """
        toc_offset = 4 + 1 + 1 + 8 + sum(len(c) for c in self.image_chunks)
        with open(output_path, 'wb') as f:
            f.write(MAGIC)
            f.write(struct.pack('<BBQ', VERSION, FLAG_ALL_COMPRESSED if self.compress else 0, toc_offset))
            for chunk in self.image_chunks:
                f.write(chunk)
            f.write(struct.pack('<I', len(self.entries)))
            for entry in self.entries:
                f.write(struct.pack('<IQIB', *entry[:-1]))
                f.write(entry[-1])

    @staticmethod
    def read(archive_path: str):
        """
        Reads an archive file and returns a reader object to interact with it.

        Args:
            archive_path (str): The file path to read the archive from.

        Returns:
            HSGAAReader: A reader object for the archive.
        """
        with open(archive_path, 'rb') as f:
            if f.read(4) != MAGIC:
                raise ValueError("Invalid AAF file")
            version, flags, toc_offset = struct.unpack('<BBQ', f.read(10))
            f.seek(toc_offset)
            entry_count = struct.unpack('<I', f.read(4))[0]

            entries = []
            for _ in range(entry_count):
                name_hash, offset, size, name_len = struct.unpack('<IQIB', f.read(17))
                name = f.read(name_len).decode('utf-8')
                entries.append(HSGAAEntry(name, offset, size))

            return HSGAAReader(archive_path, entries)

class HSGAAReader:
    """
    Reader class for extracting files from an HSGAA archive.

    Attributes:
        archive_path (str): The path to the archive file.
        entries (dict): A dictionary of file entries with file names as keys.
    """
    def __init__(self, archive_path: str, entries):
        """
        Initializes the reader for the given archive.

        Args:
            archive_path (str): The path to the archive file.
            entries (list): The list of entries in the archive.
        """
        self.archive_path = archive_path
        self.entries = {e.name: e for e in entries}

    def list_entries(self):
        """
        Lists all the file names in the archive.

        Returns:
            list: List of file names in the archive.
        """
        return list(self.entries.keys())

    def extract(self, name: str, out_path: str = None) -> bytes:
        """
        Extracts a file from the archive.

        Args:
            name (str): The name of the file to extract.
            out_path (str): The output path to write the extracted file to.

        Returns:
            bytes: The extracted file data.

        Raises:
            KeyError: If the file is not found in the archive.
            ValueError: If the file's data is corrupted.
        """
        if name not in self.entries:
            raise KeyError(f"'{name}' not found in archive")

        entry = self.entries[name]
        with open(self.archive_path, 'rb') as f:
            f.seek(entry.offset)
            signature, chunk_flags, original_size, compressed_size = struct.unpack('<IBII', f.read(13))
            blob = f.read(compressed_size)

            # Check if the data is truncated
            if len(blob) != compressed_size:
                raise ValueError("Truncated chunk: expected {}, got {}".format(compressed_size, len(blob)))

            if chunk_flags & CHUNK_FLAG_COMPRESSED:
                blob = zlib.decompress(blob)

            # Verify the integrity of the extracted data
            if binascii.crc32(blob) & 0xFFFFFFFF != signature:
                raise ValueError(f"Corrupted blob for '{name}' (signature mismatch)")

            # If an output path is provided, write the file to disk
            if out_path:
                os.makedirs(os.path.dirname(out_path), exist_ok=True)
                with open(out_path, 'wb') as out:
                    out.write(blob)

            return blob
