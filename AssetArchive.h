#ifndef HS_GAA_ARCHIVE_H
#define HS_GAA_ARCHIVE_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "zlib-1.3.1/zlib.h"

#define MAGIC "AAF\x00"
#define VERSION 1

// Flags
#define FLAG_ALL_COMPRESSED 0x01
#define CHUNK_FLAG_COMPRESSED 0x01

// CRC32 hash function (equivalent to hash32 in Python)
static inline uint32_t hash32(const char *name) {
    uint32_t crc = crc32(0, NULL, 0);  // Initialize CRC32
    return crc32(crc, (const uint8_t *)name, strlen(name));
}

// Entry structure for each file in the archive
typedef struct {
    char *name;        // File name
    uint64_t offset;   // Offset in the archive file
    uint64_t size;     // Size of the file data
} HSGAAEntry;

// Archive structure for managing the entire archive
typedef struct {
    HSGAAEntry *entries;      // List of file entries
    size_t entry_count;       // Number of file entries
    uint8_t **image_chunks;   // File data chunks
    size_t *image_chunk_sizes; // Size of each data chunk
    size_t image_chunk_count; // Number of chunks
    int compress;             // Flag indicating if compression is used
} HSGAAArchive;

// Create a new archive object
static inline HSGAAArchive* hsgaa_create_archive(int compress) {
    HSGAAArchive *archive = (HSGAAArchive *)malloc(sizeof(HSGAAArchive));
    archive->entries = NULL;
    archive->entry_count = 0;
    archive->image_chunks = NULL;
    archive->image_chunk_sizes = NULL;
    archive->image_chunk_count = 0;
    archive->compress = compress;

    return archive;
}

// Add a file to the archive, dynamically allocating memory for its data
static inline void hsgaa_add_file(HSGAAArchive *archive, const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    fseek(file, 0, SEEK_END);
    size_t raw_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Dynamically allocate memory for raw file data
    uint8_t *raw_data = (uint8_t *)malloc(raw_size);
    fread(raw_data, 1, raw_size, file);
    fclose(file);

    uint32_t signature = crc32(0, NULL, 0);
    signature = crc32(signature, raw_data, raw_size);

    uint8_t *compressed_data = NULL;
    size_t compressed_size = 0;

    if (archive->compress) {
        compressed_data = (uint8_t *)malloc(compressBound(raw_size));
        int ret = compress(compressed_data, &compressed_size, raw_data, raw_size);
        if (ret != Z_OK) {
            fprintf(stderr, "Compression failed\n");
            free(raw_data);
            free(compressed_data);
            return;
        }
    } else {
        compressed_data = raw_data;
        compressed_size = raw_size;
    }

    // Add the file entry to the archive
    archive->entries = realloc(archive->entries, sizeof(HSGAAEntry) * (archive->entry_count + 1));
    archive->image_chunks = realloc(archive->image_chunks, sizeof(uint8_t *) * (archive->image_chunk_count + 1));
    archive->image_chunk_sizes = realloc(archive->image_chunk_sizes, sizeof(size_t) * (archive->image_chunk_count + 1));

    HSGAAEntry *entry = &archive->entries[archive->entry_count];
    entry->name = strdup(file_path);  // Duplicate the file name
    entry->offset = archive->image_chunk_count ? archive->image_chunks[archive->image_chunk_count - 1] : 0;
    entry->size = compressed_size;

    archive->image_chunks[archive->image_chunk_count] = compressed_data;
    archive->image_chunk_sizes[archive->image_chunk_count] = compressed_size;
    archive->image_chunk_count++;

    archive->entry_count++;

    free(raw_data);  // Free raw data after compression or direct usage
}

// Write the archive to a file
static inline void hsgaa_write_archive(HSGAAArchive *archive, const char *output_path) {
    FILE *f = fopen(output_path, "wb");
    if (!f) {
        perror("Failed to open archive file for writing");
        return;
    }

    fwrite(MAGIC, 1, 4, f);
    uint8_t flags = (archive->compress) ? FLAG_ALL_COMPRESSED : 0;
    uint64_t toc_offset = 4 + 1 + 1 + 8 + (sizeof(HSGAAEntry) * archive->entry_count);
    fwrite(VERSION, sizeof(uint8_t), 1, f);
    fwrite(&flags, sizeof(uint8_t), 1, f);
    fwrite(&toc_offset, sizeof(uint64_t), 1, f);

    // Write all image chunks to the archive
    for (size_t i = 0; i < archive->image_chunk_count; ++i) {
        fwrite(archive->image_chunks[i], 1, archive->image_chunk_sizes[i], f);
    }

    // Write the Table of Contents (TOC)
    fwrite(&archive->entry_count, sizeof(uint32_t), 1, f);
    for (size_t i = 0; i < archive->entry_count; ++i) {
        HSGAAEntry *entry = &archive->entries[i];
        uint32_t name_len = strlen(entry->name);
        uint32_t name_hash = hash32(entry->name);
        fwrite(&name_hash, sizeof(uint32_t), 1, f);
        fwrite(&entry->offset, sizeof(uint64_t), 1, f);
        fwrite(&entry->size, sizeof(uint64_t), 1, f);
        fwrite(&name_len, sizeof(uint32_t), 1, f);
        fwrite(entry->name, 1, name_len, f);
    }

    fclose(f);
}

// Read an archive from a file
static inline HSGAAArchive* hsgaa_read_archive(const char *archive_path) {
    FILE *f = fopen(archive_path, "rb");
    if (!f) {
        perror("Failed to open archive file for reading");
        return NULL;
    }

    uint8_t magic[4];
    fread(magic, 1, 4, f);
    if (strncmp((const char *)magic, MAGIC, 4) != 0) {
        fclose(f);
        fprintf(stderr, "Invalid magic in archive\n");
        return NULL;
    }

    uint8_t version;
    uint8_t flags;
    uint64_t toc_offset;
    fread(&version, sizeof(uint8_t), 1, f);
    fread(&flags, sizeof(uint8_t), 1, f);
    fread(&toc_offset, sizeof(uint64_t), 1, f);

    fseek(f, toc_offset, SEEK_SET);

    uint32_t entry_count;
    fread(&entry_count, sizeof(uint32_t), 1, f);

    HSGAAArchive *archive = hsgaa_create_archive(flags & FLAG_ALL_COMPRESSED);
    archive->entries = malloc(sizeof(HSGAAEntry) * entry_count);
    archive->entry_count = entry_count;

    for (size_t i = 0; i < entry_count; ++i) {
        uint32_t name_hash;
        uint64_t offset, size;
        uint32_t name_len;
        fread(&name_hash, sizeof(uint32_t), 1, f);
        fread(&offset, sizeof(uint64_t), 1, f);
        fread(&size, sizeof(uint64_t), 1, f);
        fread(&name_len, sizeof(uint32_t), 1, f);

        char *name = malloc(name_len + 1);
        fread(name, 1, name_len, f);
        name[name_len] = '\0';

        archive->entries[i].name = name;
        archive->entries[i].offset = offset;
        archive->entries[i].size = size;
    }

    fclose(f);
    return archive;
}

// List the files in the archive
static inline void hsgaa_list_entries(HSGAAArchive *archive) {
    for (size_t i = 0; i < archive->entry_count; ++i) {
        printf("%s\n", archive->entries[i].name);
    }
}

// Extract a file from the archive
static inline void hsgaa_extract_file(HSGAAArchive *archive, const char *file_name, const char *output_path) {
    for (size_t i = 0; i < archive->entry_count; ++i) {
        if (strcmp(archive->entries[i].name, file_name) == 0) {
            FILE *out = fopen(output_path, "wb");
            if (!out) {
                perror("Failed to open file for extraction");
                return;
            }

            fwrite(archive->image_chunks[i], 1, archive->image_chunk_sizes[i], out);
            fclose(out);
            return;
        }
    }
    fprintf(stderr, "File not found in archive: %s\n", file_name);
}

// Free the memory used by the archive
static inline void hsgaa_free_archive(HSGAAArchive *archive) {
    for (size_t i = 0; i < archive->entry_count; ++i) {
        free(archive->entries[i].name);
    }
    free(archive->entries);
    for (size_t i = 0; i < archive->image_chunk_count; ++i) {
        free(archive->image_chunks[i]);
    }
    free(archive->image_chunks);
    free(archive->image_chunk_sizes);
    free(archive);
}

#endif // HS_GAA_ARCHIVE_H