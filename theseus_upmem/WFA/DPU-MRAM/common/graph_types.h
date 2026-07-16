#ifndef GRAPH_TYPES_H
#define GRAPH_TYPES_H

#include <stdint.h>

// A node representation that the DPU can read
typedef struct {
    uint32_t id;
    uint32_t seq_offset;    // Byte offset of the sequence in the sequence pool
    uint32_t seq_len;       // Length of the sequence (number of characters)
    uint32_t edges_offset;  // index offset in the edge pool
    uint32_t out_degree;    // Number of outgoing edges
    uint32_t in_degree;     // Number of incoming edges
} dpu_node_t;

// Parameters structure sent to the DPU
typedef struct {
    uint32_t num_nodes;
    uint32_t seq_pool_len;
    uint32_t edge_pool_len;
    
    // Addresses in the DPU MRAM heap (allocated by host)
    uint32_t nodes_mram_addr;
    uint32_t seq_pool_mram_addr;
    uint32_t edge_pool_mram_addr;
} graph_transfer_params_t;

#endif
