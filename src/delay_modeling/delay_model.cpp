#include "delay_model.h"
#include <flow_factory.h>
#include <topo_factory.h>
#include <netdesign_factory.h>
#include <netview_factory.h>
#include <util.h>
#include <debug_config.h>
#include <difflib.h>
#include <iostream>
#include <unordered_map>

/*
* User-defined constructor for DelayModel class
*/
DelayModel::DelayModel(RTNetView *RTNV){
   rt_netview = RTNV;
}

// The periodic flows already laid out in the RTNetView will be
// in rt_netview->planned_periodic_flows
bool DelayModel::isPlannedPeriodic(Flow* f){
   
    bool result = false;
    for (auto F: rt_netview->planned_periodic_flows){
        if (F->flow_id == f->flow_id){
                result = true;
                break;
        }
    }
    return result;
}

// The aperiodic flows already laid out in the RTNetView will be
// in rt_netview->planned_aperiodic_flows
bool DelayModel::isPlannedAperiodic(AperiodicFlow* AF){

    bool result = false;
    for (auto G: rt_netview->planned_aperiodic_flows){
        if (G->flow_id == AF->flow_id){
                result = true;
                break;
        }
    }
    return result;
}

/*
* Takes in a flow object and returns a list of all the periodic flow objects in the RTNetwork having higher priority
*/
std::vector<Flow*>& DelayModel::getHigherPriorityFlows(const Flow& F, std::vector<Flow*>& result){
    
    for (auto x: rt_netview->rt_network->flows){
        //If the flow is higher priority, we add to the list
        //std::cout<<"Considering flow "<<x->flow_name<<" with flow id "<<x->flow_id<<std::endl;
        if ((x->priority <= F.priority) && (x->flow_id != F.flow_id))
            result.push_back(x);
    }
    //std::cout<<"Done calculating higher priority flows than "<<F.flow_name<<std::endl;
    return result;
}

/*
* Takes in a flow object and returns a list of all the periodic flow objects in the RTNetwork that have been planned and have higher priority
*/
std::vector<Flow*>& DelayModel::getHigherPriorityPlannedFlows(const Flow& F, std::vector<Flow*>& result){
    
    for (auto x: rt_netview->rt_network->flows){
        //If the flow is higher priority, we add to the list
        //std::cout<<"Considering flow "<<x->flow_name<<" with flow id "<<x->flow_id<<std::endl;
        if ((x->priority <= F.priority) && (x->flow_id != F.flow_id) && isPlannedPeriodic(x))
            result.push_back(x);
    }
    //std::cout<<"Done calculating higher priority flows than "<<F.flow_name<<std::endl;
    return result;
}

/*
* Takes in a flow id and finds the flow having that id
*/
Flow* DelayModel::getFlowById(uint f_id, Flow* result){
    
    for (auto f:rt_netview->rt_network->flows){
        if (f->flow_id == f_id){
            result = f;
            break;
        }
    }
    //std::cout<<"\t\t\t Flow ID "<<f_id<<" matches with flow address = "<<result<<std::endl;
    return result;
}

/*
* Takes in a flow and finds the bottleneck switch over a common segment
*/
double DelayModel::getMaxSingleSwitchProcessingTimeOverCommonSegment(uint flow_id, 
            std::vector<Node*>& common_segment){
    
    double max_single_switch_processing_time_over_common_segment = 0.0, per_job_processing_time;
    
    if (DBG_FRC)
        std::cout<<"\t\t\t\t\t\tCalculating max. single switch processing time over a common segment for flow with ID = "<<flow_id<<std::endl;

    for (auto sw: common_segment){
        Flow* f;
        Flow* g;
        g = getFlowById(flow_id, f);
        //std::cout<<"Switch name = "<<sw->node_name<<" "<<"Switch Bandwidth Capacity = "<<sw->node_bw_capacity<<std::endl;
        //std::cout<<"Packet size = "<<g->packet_size<<std::endl;
        // if (g->destination == rt_netview->cur_periodic_flow->destination)
        //     per_job_processing_time = 0;
        // else
        per_job_processing_time = ((g->bandwidth * g->period)/sw->node_bw_capacity);
        

        //std::cout<<"Processing Time for "<<g<<"("<<g->flow_name<<")"<<" over switch "<<sw->node_name<<" = "<<processing_time<<std::endl;
        if (per_job_processing_time >= max_single_switch_processing_time_over_common_segment)
            max_single_switch_processing_time_over_common_segment = per_job_processing_time;
    }

    if (DBG_FRC)
        std::cout<<"\t\t\t\t\t\tDone calculating max. single switch processing time over a common segment for flow with ID = "<<flow_id<<" = "<<format_time(max_single_switch_processing_time_over_common_segment)<<std::endl;
    
    return max_single_switch_processing_time_over_common_segment;
}

/*
* Takes in a flow id and finds the folds by the flow id
*/
void DelayModel::getFoldsbyFlowId(uint flow_id, std::vector<std::vector<Node*>>& folds){

    //std::vector<std::vector<Node*>> folds;
    std::vector<Path*> all_simple_paths;
    Flow* f; 
    Flow* g;
    
    //std::cout<<"\t\t\tIn "<<__func__<<"for Flow ="<<flow_id<<std::endl;
    g = getFlowById(flow_id, f);
    std::unordered_map<Path*, bool, CustomPathHash> visited;

    Node* src = g->source;
    Node* dst = g->destination;

    if(DBG_FRC){
        std::cout<<"\t\t\tSource Node = "<<g->source<<" "<<"Destination Node = "<<g->destination<<std::endl;
        std::cout<<"\t\t\tSource Node = "<<src->get_name()<<" "<<"Destination Node = "<<dst->get_name()<<std::endl;
    }

    //Older Methods of getting folds -- seems incorrect to consider folds across all possible paths
    /*all_simple_paths = cps_network_topo_view->all_possible_paths;//is a vector of Path*
    for (auto P: all_simple_paths)
        P->get_folds(folds);
    */

    g->assigned_path->get_folds(folds);

    if (DBG_FRC)
        std::cout<<"\t\t\tDone getting folds by the higher flow id for flow with ID = "<<flow_id<<std::endl;
}

class SegmentHashFunction {

    public:
        //Simple XOR-based hash - not best practices as far as security is concerned
        size_t operator()(const std::tuple<size_t, size_t, size_t>& p) const{
            return (hash<double>()(get<0>(p))) ^ 
                (hash<double>()(get<1>(p))) ^
                (hash<double>()(get<2>(p)));
        }
};
    
/*
* Takes in a low priority flow's Path, a fold from a high priority flow 
* and finds the common segments
* hp_fold -- is the higher priority fold
* hp_folds -- is the set of folds from the higher priority flow
* lp_path -- subpath/path of lower priority flow we are considering
* common_segments -- list of common segments that are output by the function
*/


void DelayModel::getCommonSegments(std::vector<Node*>& hp_fold, 
    Path& lp_path, std::vector<std::vector<Node*>>& forward_segments, 
    std::vector<std::vector<Node*>>& reverse_segments, 
    std::vector<std::vector<Node*>>& cross_segments ){

    double path_delay = 0.0;

    bool reverse_match = false;
    bool cross_match = false;

    std::unordered_map<std::tuple<size_t, size_t, size_t>, bool, SegmentHashFunction> added;

    if (DBG_DBG)
        DBG_PRINT

    std::vector<Node*> lp_path_nodes_rev;
    std::vector<Node*> single_common_segs;

    auto seq_forward = difflib::MakeSequenceMatcher(hp_fold, lp_path.nodes);
    
    lp_path_nodes_rev.resize(lp_path.nodes.size());
    std::reverse_copy(lp_path.nodes.begin(), lp_path.nodes.end(), lp_path_nodes_rev.begin());
    auto seq_reverse = difflib::MakeSequenceMatcher(hp_fold, lp_path_nodes_rev);

    size_t start1, start2, length;
    size_t start1_rev, start2_rev, length_rev;

    for(difflib::match_t const& m : seq_forward.get_matching_blocks()){
        if (DBG_FRC)
            std::cout<<"\t\t\t\tTesting for Forward & Cross Matching"<<std::endl;
      
        std::tie(start1, start2, length) = m;
        //std::cout << start1 << " " << start2 << " " << length << "\n";

        if ((length > 1) && (!added[make_tuple(start1, start2, length)])){
            // x[start1:start1+length] is the common
            std::vector<Node*>::const_iterator first = hp_fold.begin() + start1;
            std::vector<Node*>::const_iterator last = hp_fold.begin() + start1 + length;
            std::vector<Node*> val1(first, last);                
            forward_segments.push_back(val1);
            added[make_tuple(start1, start2, length)] = true;
            if (DBG_FRC)
                std::cout<<"\t\t\t\tForward Matching Satisfied"<<std::endl;
        }
        else if((length == 1) && (!added[make_tuple(start1, start2, length)])){
            std::vector<Node*>::const_iterator cross_segment_common = hp_fold.begin() + start1;
            std::vector<Node*> val2{*cross_segment_common};
            cross_segments.push_back(val2);
            added[make_tuple(start1, start2, length)] = true;
            if (DBG_FRC)
                std::cout<<"\t\t\t\tCross Matching Satisfied"<<std::endl;
        }        
    }

    for(difflib::match_t const& m : seq_reverse.get_matching_blocks()){
        if (DBG_FRC)
            std::cout<<"\t\t\t\tTesting for Reverse Matching"<<std::endl;

        std::tie(start1_rev, start2_rev, length_rev) = m;
        //std::cout << start1 << " " << start2 << " " << length << "\n";

        if ((length_rev > 1) && (!added[make_tuple(start1_rev, start2_rev, length_rev)])){
            
            // x[start1_rev:start1+length_rev] is the common
            std::vector<Node*>::const_iterator first = hp_fold.begin() + start1_rev;
            std::vector<Node*>::const_iterator last = hp_fold.begin() + start1_rev + length_rev;
            std::vector<Node*> val3(first, last);                
            reverse_segments.push_back(val3);
            added[make_tuple(start1_rev, start2_rev, length_rev)] = true;
            if (DBG_FRC)
                std::cout<<"\t\t\t\tReverse Matching Satisfied"<<std::endl;
        }
    }

    if (DBG_FRC)
        std::cout<<"\t\t\tDone getting segments by the flow id for flow with ID = "<<rt_netview->cur_periodic_flow->flow_name<<std::endl;

}

/* 
RETURNS THE DELAY COMPUTED BY DELAY COMPOSITION THEOREM
Steps:
1) Find set of flows $F_j$ having priority higher than the tagged flow
2) For every flow, $f_k$ in $F_j$, find all the folds 
3) For every flow, $f_k$ in $F_j$, find segments that intersects with the tagged flow. For every *common segment*, 
find the maximum single stage execution time for that flow. Call it $C_{k, max}$. 
Add $C_{k,max}$ for every *common segment* for every *fold* for every *higher-priority flow* than the tagged flow
4) For switch in the path of tagged flow, find the maximum stage execution time of all the job segments on the node 
*/
 
double DelayModel::getPathDelay(Flow& F, Path& P){

    double delay = 0.0;
    double path_independent_delay = 0.0;
    double path_dependent_delay = 0.0;
    double link_delay = 0.0;

    double max_delay_at_switch, processingTime;

    std::vector<Flow*> hp_flows;
    std::vector<std::vector<Node*>> hp_folds;
    std::vector<std::vector<Node*>> forward_segments;
    std::vector<std::vector<Node*>> reverse_segments;
    std::vector<std::vector<Node*>> cross_segments;

    std::vector<Flow*> flows_sharing_switch;

    std::vector<Flow*>* result;

    if(DBG_FRC){
        std::cout<<"\nComputing Path Delay for Flow "<<F.flow_id<<" over :";
        P.printPath();
    }

    /*
    * Path Independent Delay depends on flows having higher priority than F
    */

    getHigherPriorityPlannedFlows(F, hp_flows);
    
    if(DBG_FRC)
        std::cout<<"\t\t\tNumber of existing higher priority flows than Flow "<<F.flow_id<<" = "<<hp_flows.size()<<std::endl;
    
    for (auto flow_hp: hp_flows){
        
        hp_folds.clear();
        
        if(DBG_FRC){
            std::cout<<"\n\n\t\t\tConsidering existing higher priority flow "<<flow_hp->flow_id<<" with path ";
            printPath(flow_hp->assigned_path->nodes);
            std::cout<<std::endl;
        }
            
        getFoldsbyFlowId(flow_hp->flow_id, hp_folds);
        
        if (DBG_FRC){
            std::cout<<"\t\t\tFolds : ";
            printPathSet(hp_folds);
        }

        for (auto hp_fold: hp_folds){

            forward_segments.clear();
            reverse_segments.clear();
            cross_segments.clear();
            
            if (DBG_FRC){
                std::cout<<"\t\t\t\tConsidering Fold : ";
                printPath(hp_fold);
                std::cout<<"\n";
            }
            
            //F.assigned_path->printPath();

            getCommonSegments(hp_fold, P, forward_segments, reverse_segments, cross_segments);
            //getCommonSegments(fold, F.flow_id, F.assigned_path, folds, common_segments);
            
            if (DBG_FRC){
                std::cout<<"\t\t\t\t\tCommon Forward Segments : ";
                printPathSet(forward_segments);
                std::cout<<"\t\t\t\t\tCommon Reverse Segments : ";
                printPathSet(reverse_segments);
                std::cout<<"\t\t\t\t\tCommon Cross Segments : ";
                printPathSet(cross_segments);
            }
            
            //std::cout<<"\t\t\t\t\t\tGoing over common segments----"<<std::endl;
            auto i = 0;
            if(!forward_segments.empty()){
                for (auto forward_segment: forward_segments){

                    if(!forward_segment.empty()){
                    
                        if (DBG_FRC){
                            std::cout<<"\t\t\t\t\t\t\tConsidering common forward segment : ";
                            printPath(forward_segment);
                            std::cout<<"\t\t\t\t\t\t\t\t"<<i++<<std::endl;
                        }

                        auto val = getMaxSingleSwitchProcessingTimeOverCommonSegment(flow_hp->flow_id, forward_segment);
                        
                        if (DBG_FRC)
                            std::cout<<"\t\t\t\t\t\t\t Max. single switch processing time over the common forward segment with flow with ID = "<<flow_hp->flow_id<<" = "<<format_time(val)<<std::endl;

                        path_independent_delay += val;
                        //std::cout<<"Still in the common segment loop\n";
                    }
                }
            }

            if(!reverse_segments.empty()){
                for (auto reverse_segment: reverse_segments){

                    if(!reverse_segment.empty()){
                    
                        if (DBG_FRC){
                            std::cout<<"\t\t\t\t\t\t\tConsidering common reverse segment : ";
                            printPath(reverse_segment);
                            std::cout<<"\t\t\t\t\t\t\t\t"<<i++<<std::endl;
                        }

                        auto val = getMaxSingleSwitchProcessingTimeOverCommonSegment(flow_hp->flow_id, reverse_segment);
                        
                        if (DBG_FRC)
                            std::cout<<"\t\t\t\t\t\t\t Max. single switch processing time over the common reverse segment with flow with ID = "<<flow_hp->flow_id<<" = "<<format_time(val)<<std::endl;

                        path_independent_delay += val;
                        //std::cout<<"Still in the common segment loop\n";
                    }
                }
            }

            if(!cross_segments.empty()){
                for (auto cross_segment: cross_segments){

                    if(!cross_segment.empty()){
                    
                        if (DBG_FRC){
                            std::cout<<"\t\t\t\t\t\t\tConsidering common cross segment : ";
                            printPath(cross_segment);
                            std::cout<<"\t\t\t\t\t\t\t\t"<<i++<<std::endl;
                        }

                        auto val = getMaxSingleSwitchProcessingTimeOverCommonSegment(flow_hp->flow_id, cross_segment);
                        
                        if (DBG_FRC)
                            std::cout<<"\t\t\t\t\t\t\t Max. single switch processing time over the common reverse segment with flow with ID = "<<flow_hp->flow_id<<" = "<<format_time(val)<<std::endl;

                        path_independent_delay += val;
                        //std::cout<<"Still in the common segment loop\n";
                    }
                }
            }
        }
    }
    
    if (DBG_FRC)
        std::cout<<"\t\t\tComputed Path Independent Delay = "<<format_time(path_independent_delay)<<std::endl;

    /*
    * Path Dependent Delay depends on flows sharing the same switch as F
    */
    
    //for (auto sw: F.assigned_path->nodes){
    for (auto sw: P.nodes){
        //std::cout<<"In "<<__FILE__<<":"<<__LINE__<<" "<<"for Flow "<<F.flow_name<<std::endl;
        // TODO : Changed on prev. day of NSDI before avionics evaluation
        if (sw != P.nodes[P.nodes.size()-1]){
        //if (sw != F.destination){
            flows_sharing_switch = getFlowsSharingSwitch(sw, flows_sharing_switch);
            max_delay_at_switch = 0.0;
            for (auto f_sharing:flows_sharing_switch){
                processingTime = (f_sharing->bandwidth * f_sharing->period)/sw->node_bw_capacity;//f_sharing->packet_size/sw->node_bw_capacity;
                if(processingTime >= max_delay_at_switch)
                    max_delay_at_switch = processingTime;
            }
            path_dependent_delay += std::max((F.bandwidth * F.period)/sw->node_bw_capacity, max_delay_at_switch);
        }
    }
    
    if (DBG_FRC)
        std::cout<<"\t\t\tComputed Path Dependent Delay = "<<format_time(path_dependent_delay)<<std::endl;
    
    /*
    * Link delay depends on the propagation delay as well as the forwarding delay
    */
    for (auto l: P.links)
        link_delay += F.perLinkDelay(l);
    
    delay = path_independent_delay + path_dependent_delay + link_delay;
    
    if (DBG_FRC)
        std::cout<<"\t\t\tDelay for "<<F.flow_name<<" is "<<format_time(delay)<<std::endl;
    
    return delay;
}

//Returns the list of Flow* objects sharing a switch 
std::vector<Flow*>& DelayModel::getFlowsSharingSwitch(Node* N, std::vector<Flow*>& flows_sharing_switch){
    flows_sharing_switch = N->enterFlows;
    return flows_sharing_switch;
}

//Helper Function to print a list of paths, where each path is represented as a list of Node*
void DelayModel::printPathSet(std::vector<std::vector<Node*>>& x){
    if(!x.empty()){
        for (auto node_vec: x){
            if(!node_vec.empty()){
                std::cout<<"[";
                printPath(node_vec);
                std::cout<<"],";
            }
        }
        std::cout<<std::endl;
    }
}

//Helper Function to print a path, represented as a list of Node*
void DelayModel::printPath(std::vector<Node*>& x){
    std::cout<<"[";
    for (auto n: x)
        std::cout<<n->node_name<<" ";
    std::cout<<"]";
}