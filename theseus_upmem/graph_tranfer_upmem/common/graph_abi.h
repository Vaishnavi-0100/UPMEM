#ifndef GRAPH_ABI_H
#define GRAPH_ABI_H

#include <stdint.h>

#define GRAPH_INVALID_NODE UINT32_MAX

typedef struct {
    uint32_t sequence_offset;
    uint32_t sequence_length;
    uint32_t edge_offset;
    uint32_t edge_count;
} graph_node_t;

typedef struct {
    uint32_t num_nodes;
    uint32_t num_edges;
    uint32_t sequence_bytes;

    /* Offsets relative to DPU_MRAM_HEAP_POINTER. */
    uint32_t nodes_m;
    uint32_t edges_m;
    uint32_t sequences_m;

    uint32_t padding0;
    uint32_t padding1;
} graph_descriptor_t;

#endif