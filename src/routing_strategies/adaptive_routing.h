/* Data structure associated with each node, comprising the shortest path to the destination node and the delay along such a path*/
#ifndef adaptive_routing_h
#define adaptive_routing_h

#include <util.h>
#include <delay_model.h>

class AdaptiveRouting;
class AdaptiveRoutingLabel;

class AdaptiveRoutingLabelComparator_Alpha;
class AdaptiveRoutingLabelComparator_Beta;

typedef std::unordered_map<Node*, AdaptiveRoutingLabel*, CustomNodePHash> NodeAdaptiveRoutingLabelDictCustom;
typedef std::priority_queue<AdaptiveRoutingLabel*, std::vector<AdaptiveRoutingLabel*>, AdaptiveRoutingLabelComparator_Alpha> PriorityQueueCustom_Alpha;
typedef std::priority_queue<AdaptiveRoutingLabel*, std::vector<AdaptiveRoutingLabel*>, AdaptiveRoutingLabelComparator_Beta> PriorityQueueCustom_Beta;

class AdaptiveRouting{

    // Removing cur_periodic_flow and cur_aperiodic_flow as members 
    // from the class as members since RTNetView can access them
    // Flow* cur_periodic_flow;
    // AperiodicFlow* cur_aperiodic_flow;
    
    RTNetView* rt_netview;

    // Cost metrics computed from the RTNetView using DelayModel
    DelayModel* DM;

    std::unordered_map<Node*, bool, CustomNodePHash> visited;

    // $\beta$ from the Adaptive routing paper - $\beta(v)$ is the minimum sum of typical-delay
    // costs of the edges on a path from s -> v in which all the edges are useful
    std::unordered_map<Node*, double, CustomNodePHash> beta;
    std::unordered_map<Node*, Path*, CustomNodePHash> T;

    // $\alpha$ from the Adaptive routing paper - $\alpha(v)$ is the minimum worst-case delay 
    // costs of the edges on a path from v -> t 
    std::unordered_map<Node*, double, CustomNodePHash> alpha;
    std::unordered_map<Node*, Path*, CustomNodePHash> W;

    // AdaptiveRoutingLabel Custom Dictionary
    // Maps from Node* to AdaptiveRoutingLabel*
    NodeAdaptiveRoutingLabelDictCustom ARL_dict;

    public:

        AdaptiveRouting(RTNetView* rtnv);
        //Preprocessing should be called as a part of the AdaptiveRouting constructor
        void preprocessing_via_dijkstra_rta();
        void preprocessing_via_dijkstra_dct();
        void preprocessing_via_dijkstra_nc();
        void print_routing_table();
        bool semi_adaptive_realtime_routing_rta(RTNetView* rt_nv, bool EMPTY_NETWORK_FLAG, bool IS_APERIODIC_FLAG);
        bool semi_adaptive_realtime_routing_dct(RTNetView* rt_nv, bool EMPTY_NETWORK_FLAG, bool IS_APERIODIC_FLAG);
        bool semi_adaptive_realtime_routing_nc(RTNetView* rt_nv, bool EMPTY_NETWORK_FLAG, bool IS_APERIODIC_FLAG);
        void fully_adaptive_realtime_routing(RTNetView* rt_nv, bool EMPTY_NETWORK_FLAG, bool IS_APERIODIC_FLAG);

};

class AdaptiveRoutingLabel{
    
    public:
        Flow* cur_flow;
        Node* cur_node;
        
        // $\alpha$ from the Adaptive routing paper - $\alpha(cur_node)$ tracks the minimum worst-case delay 
        // costs of the edges on a path from cur_node -> cur_flow->destination 
        Path* W;
        double alpha;

        // $\beta$ from the Adaptive routing paper - $\beta(v)$ is the minimum sum of typical-delay
        // costs of the edges on a path from s -> v in which all the edges are *useful*
        // A edge (u, v) is said to be useful if beta(u) + C_w(u, v) + alpha(v) <= D
        Path* T;
        double beta;

        AdaptiveRoutingLabel(Flow *cur_flow, Node* cur_node);
};

#endif