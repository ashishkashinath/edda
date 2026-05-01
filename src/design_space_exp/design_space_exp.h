#ifndef design_space_exp_h
#define design_space_exp_h

#include <flow_factory.h>
#include <topo_factory.h>
#include <util.h>
#include <eval_parameters.h>

class FlowGenerator;

class NetworkGenerator{
    public:
        //Network Topology
        std::vector<Node*> node_specifications;
        std::vector<Link*> link_specifications;
        std::vector<Link*> mesh_link_specifications;
        std::vector<std::vector<Link*>> possible_link_specifications;

        //Flow Specifications
        FlowGenerator* fg;

        uint num_nodes;
        uint num_links;
        uint num_topologies;

        //Latency Scheme
        latency_scheme delay_scheme;

        // Random Number Engine
        std::mt19937 rng;

        //Methods
        NetworkGenerator(uint nr_nodes, uint nr_links, FlowGenerator** fg, latency_scheme ds);
        void generateLinkSubsets(std::vector<Link *>& links, int k, int start, std::vector<Link *>& current, std::vector<std::vector<Link *>>& result);
        std::vector<std::vector<Link *>> allLinkSubsetsOfSizeK(std::vector<Link *>& total_links, int k);
        void generateAllRandomTopologies();
        void generateRandomTopology();

};
#endif
