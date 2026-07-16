#include <mram.h>
#include <alloc.h>
#include <defs.h>
#include <stdio.h>
#include "../common/graph_types.h"

// Helper macros
#define ROUND_UP_MULTIPLE_8(x) ((((x) + 7) / 8) * 8)

int main() {
    uint32_t tasklet_id = me();
    
    // Only tasklet 0 executes the verification to prevent cluttered log output
    if (tasklet_id != 0) {
        return 0;
    }
    
    printf("DPU (Tasklet 0) started. Reading graph parameters...\n");
    
    // Read the graph parameter structure from the beginning of the MRAM heap
    uint32_t params_m = (uint32_t)DPU_MRAM_HEAP_POINTER;
    graph_transfer_params_t params;
    mram_read((__mram_ptr void const *)params_m, &params, ROUND_UP_MULTIPLE_8(sizeof(graph_transfer_params_t)));
    
    printf("Graph parameters loaded:\n");
    printf("  Num nodes: %u\n", params.num_nodes);
    printf("  Seq pool len: %u\n", params.seq_pool_len);
    printf("  Edge pool len: %u\n", params.edge_pool_len);
    printf("  Nodes addr: %u\n", params.nodes_mram_addr);
    printf("  Seq pool addr: %u\n", params.seq_pool_mram_addr);
    printf("  Edge pool addr: %u\n", params.edge_pool_mram_addr);

    // Sanity checks on size
    if (params.num_nodes > 16) {
        printf("Error: Number of nodes (%u) exceeds tasklet WRAM buffer limit (16)\n", params.num_nodes);
        return -1;
    }
    if (params.seq_pool_len > 128) {
        printf("Error: Seq pool size (%u) exceeds tasklet WRAM buffer limit (128)\n", params.seq_pool_len);
        return -1;
    }
    if (params.edge_pool_len > 16) {
        printf("Error: Edge pool size (%u) exceeds tasklet WRAM buffer limit (16)\n", params.edge_pool_len);
        return -1;
    }

    // Allocate local WRAM buffers for validation
    dpu_node_t nodes[16];
    char seq_pool[128];
    uint32_t edge_pool[16];

    // Load nodes array from MRAM
    mram_read((__mram_ptr void const *)(params_m + params.nodes_mram_addr), 
              nodes, 
              ROUND_UP_MULTIPLE_8(params.num_nodes * sizeof(dpu_node_t)));

    // Load sequence pool from MRAM
    mram_read((__mram_ptr void const *)(params_m + params.seq_pool_mram_addr), 
              seq_pool, 
              ROUND_UP_MULTIPLE_8(params.seq_pool_len * sizeof(char)));

    // Load edge pool from MRAM
    if (params.edge_pool_len > 0) {
        mram_read((__mram_ptr void const *)(params_m + params.edge_pool_mram_addr), 
                  edge_pool, 
                  ROUND_UP_MULTIPLE_8(params.edge_pool_len * sizeof(uint32_t)));
    }

    printf("Verifying graph topology on DPU:\n");
    for (uint32_t i = 0; i < params.num_nodes; ++i) {
        printf("Node [%u]:\n", nodes[i].id);
        
        // Print node sequence
        printf("  Sequence: ");
        for (uint32_t j = 0; j < nodes[i].seq_len; ++j) {
            printf("%c", seq_pool[nodes[i].seq_offset + j]);
        }
        printf("\n");
        
        // Print node outgoing edges
        printf("  Outgoing Edges: [");
        for (uint32_t e = 0; e < nodes[i].out_degree; ++e) {
            uint32_t target_node_id = edge_pool[nodes[i].edges_offset + e];
            printf("%u", target_node_id);
            if (e + 1 < nodes[i].out_degree) {
                printf(", ");
            }
        }
        printf("]\n");
    }

    printf("Graph verification on DPU complete!\n");
    return 0;
}
