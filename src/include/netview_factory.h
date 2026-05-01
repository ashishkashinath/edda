#ifndef netview_factory_h
#define netview_factory_h

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <math.h>
#include <util.h>
#include <topo_factory.h>
#include <netdesign_factory.h>

class RTNetView{

    public:
        // Flow being laid out
        Flow* cur_periodic_flow;
        AperiodicFlow* cur_aperiodic_flow;
        Node* source;
        Node* destination;
        
        // Pre-existing/Planned flows in the RTNetView
        std::vector<Flow*> planned_periodic_flows;
        std::vector<AperiodicFlow*> planned_aperiodic_flows;

        // The complete RTNetwork from where the RTNetView is built
        RTNetwork* rt_network;
        
        // Dictionary to hold node and link weights as per RTA
        std::unordered_map<Node*, double, CustomNodePHash> node_view;
        std::unordered_map<Link*, double, CustomLinkPHash> link_view;

        // The set of Nodes and Links in the RTNetView
        std::vector<Node*> rtnetview_nodes;
        std::vector<Link*> rtnetview_links;

        // Paths in the RTNetView representing various interaction possibilities
        std::vector<Path*> all_possible_paths;
        Path* shortest_path;
        
    //Methods
        RTNetView (Flow* current_flow, std::vector<Flow*>& existing_periodic_flows, 
        std::vector<AperiodicFlow*>& existing_aperiodic_flows, RTNetwork* rtn);

        RTNetView (AperiodicFlow* current_aperiodic_flow, std::vector<Flow*>& existing_periodic_flows, 
        std::vector<AperiodicFlow*>& existing_aperiodic_flows, RTNetwork* rtn);

        void get_all_possible_paths(Node* src, Node* dst);
        
        void compute_view();

        void exploreDFS(Node* start, Node* finish, Path* visited,
            const int& min_hops, const int& max_hops);

        void explore(Node* start, Node* finish, Path* path_discovered,
            std::unordered_map<Node*, bool, CustomNodePHash>& visited);

        void printNetView();

        void printPath(Path* P);

        void printPathNodes(Path* P);

        Path* get_shortest_hoplength_path();
    
};

#endif
