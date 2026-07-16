#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstring>
#include <dpu.h>
#include "../common/graph_types.h"
#include "../common/mram-management.h"

#ifndef DPU_BINARY
#define DPU_BINARY "build/test_transfer_dpu"
#endif

#define ROUND_UP_MULTIPLE_8(x) ((((x) + 7) / 8) * 8)

int main() {
    struct dpu_set_t dpu_set, dpu;
    uint32_t nr_of_dpus;

    std::cout << "Allocating 1 DPU..." << std::endl;
    DPU_ASSERT(dpu_alloc(1, NULL, &dpu_set));
    DPU_ASSERT(dpu_load(dpu_set, DPU_BINARY, NULL));
    DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &nr_of_dpus));
    std::cout << "Allocated " << nr_of_dpus << " DPU(s)." << std::endl;

    // Define a tiny graph:
    // Node 0: "ATCG", outgoing: [1, 2]
    // Node 1: "GG", outgoing: [3]
    // Node 2: "TT", outgoing: [3]
    // Node 3: "CA", outgoing: []
    
    std::vector<std::string> node_seqs = {"ATCG", "GG", "TT", "CA"};
    std::vector<std::vector<uint32_t>> outgoing_edges = {
        {1, 2}, // Node 0
        {3},    // Node 1
        {3},    // Node 2
        {}      // Node 3
    };

    uint32_t num_nodes = node_seqs.size();
    
    // Flatten / serialize the graph
    std::vector<dpu_node_t> dpu_nodes(num_nodes);
    std::vector<char> seq_pool;
    std::vector<uint32_t> edge_pool;

    for (uint32_t i = 0; i < num_nodes; ++i) {
        dpu_nodes[i].id = i;
        dpu_nodes[i].seq_offset = seq_pool.size();
        dpu_nodes[i].seq_len = node_seqs[i].length();
        
        // Append sequence characters to seq_pool
        for (char c : node_seqs[i]) {
            seq_pool.push_back(c);
        }

        dpu_nodes[i].edges_offset = edge_pool.size();
        dpu_nodes[i].out_degree = outgoing_edges[i].size();
        dpu_nodes[i].in_degree = 0; // Not used for now

        // Append outgoing edges to edge_pool
        for (uint32_t succ : outgoing_edges[i]) {
            edge_pool.push_back(succ);
        }
    }

    uint32_t seq_pool_len = seq_pool.size();
    uint32_t edge_pool_len = edge_pool.size();

    std::cout << "Graph serialized:" << std::endl;
    std::cout << "  Nodes count: " << num_nodes << std::endl;
    std::cout << "  Seq pool size: " << seq_pool_len << " bytes" << std::endl;
    std::cout << "  Edge pool size: " << edge_pool_len << " elements (" << edge_pool_len * sizeof(uint32_t) << " bytes)" << std::endl;

    // Allocate offsets in DPU MRAM heap
    struct mram_heap_allocator_t allocator;
    init_allocator(&allocator);

    uint32_t params_mram_addr = mram_heap_alloc(&allocator, sizeof(graph_transfer_params_t));
    uint32_t nodes_mram_addr = mram_heap_alloc(&allocator, num_nodes * sizeof(dpu_node_t));
    uint32_t seq_pool_mram_addr = mram_heap_alloc(&allocator, seq_pool_len * sizeof(char));
    uint32_t edge_pool_mram_addr = mram_heap_alloc(&allocator, edge_pool_len * sizeof(uint32_t));

    // Setup parameters to copy to DPU
    graph_transfer_params_t params;
    params.num_nodes = num_nodes;
    params.seq_pool_len = seq_pool_len;
    params.edge_pool_len = edge_pool_len;
    params.nodes_mram_addr = nodes_mram_addr;
    params.seq_pool_mram_addr = seq_pool_mram_addr;
    params.edge_pool_mram_addr = edge_pool_mram_addr;

    std::cout << "Pushing data to DPU MRAM..." << std::endl;

    // Transfer params
    DPU_FOREACH(dpu_set, dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &params));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, params_mram_addr, ROUND_UP_MULTIPLE_8(sizeof(graph_transfer_params_t)), DPU_XFER_DEFAULT));

    // Transfer nodes
    DPU_FOREACH(dpu_set, dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, dpu_nodes.data()));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, nodes_mram_addr, ROUND_UP_MULTIPLE_8(num_nodes * sizeof(dpu_node_t)), DPU_XFER_DEFAULT));

    // Transfer sequence pool
    DPU_FOREACH(dpu_set, dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, seq_pool.data()));
    }
    DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, seq_pool_mram_addr, ROUND_UP_MULTIPLE_8(seq_pool_len * sizeof(char)), DPU_XFER_DEFAULT));

    // Transfer edge pool (if not empty)
    if (edge_pool_len > 0) {
        DPU_FOREACH(dpu_set, dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, edge_pool.data()));
        }
        DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, edge_pool_mram_addr, ROUND_UP_MULTIPLE_8(edge_pool_len * sizeof(uint32_t)), DPU_XFER_DEFAULT));
    }

    std::cout << "Data transferred. Launching DPU..." << std::endl;
    DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
    std::cout << "DPU execution finished. Reading DPU log output:" << std::endl;
    std::cout << "========================================" << std::endl;

    // Display DPU logs
    uint32_t dpuIdx;
    DPU_FOREACH(dpu_set, dpu, dpuIdx) {
        std::cout << "--- DPU Log [" << dpuIdx << "] ---" << std::endl;
        DPU_ASSERT(dpu_log_read(dpu, stdout));
    }
    std::cout << "========================================" << std::endl;

    std::cout << "Freeing DPU resources..." << std::endl;
    DPU_ASSERT(dpu_free(dpu_set));
    std::cout << "Done!" << std::endl;

    return 0;
}
