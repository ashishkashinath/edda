#ifndef netdesign_factory_h
#define netdesign_factory_h

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <math.h>
#include <util.h>
#include <topo_factory.h>

#define PROPAGATION_DELAY 0

typedef std::unordered_map<FlowPath, double, CustomFlowPathHash> latencyResultsMap;

typedef std::unordered_map<Node*, std::vector<Link*>, CustomNodePHash> adjacent_edges;

class RTNetwork{
    public:
    //Network Specifications
        uint numNodes;
        uint numLinks;
        std::vector<Node*> nodes;
        std::vector<Link*> links;
        adjacent_edges adjListOut;
        latencyResultsMap latency_results;
    
    // A parameter to say which latency scheme is used by the Adaptive Routing algorithm
        latency_scheme delay_scheme;

    //Network Status
        bool empty_network_flag;

    //Flow specifications
        std::vector<Flow*> flows;
        std::vector<AperiodicFlow*> aflows;
    
    /*
    * existing_periodic_flows and existing_aperiodic_flows denotes 
    * the list of periodic and aperidic flows that have been placed on the network so far
    */
        std::vector<Flow*> existing_periodic_flows {};
        std::vector<AperiodicFlow*> existing_aperiodic_flows {};

    //Methods    
        RTNetwork(uint numNodes, uint numLinks, std::vector<Node*>& nodes, 
            std::vector<Link*>& links, std::vector<Flow*>& flow_specifications, 
            std::vector<AperiodicFlow*>& aflow_specifications, latency_scheme delay_scheme);

        RTNetwork(uint numNodes, uint numLinks, std::vector<Node*>& nodes,
            std::vector<Link*>& links);

        Link** getLink(Node *source, Node* destination, Link** res);

        void printNetwork();

        void populate_adjacent_edges(std::vector<Node*>& nodes, 
            std::vector<Link*>& links);

        bool check_if_connected(Node* start, Node* finish);

        bool explore(Node* start, Node* finish, 
            std::unordered_map<Node*, bool, CustomNodePHash>& visited);

        void place_periodic_flows();

        void place_aperiodic_flows();

        std::pair<double, double> post_placement_performance_analysis_csv(std::string output_dir_fullpath, std::string lat_res_file, std::string acceptance_ratio_res_file);

        void post_placement_performance_analysis();

        void printPath(Path* P);

};

#endif
