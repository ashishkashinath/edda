#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <flow_factory.h>
#include <util.h>
#include <iomanip>
#include <debug_config.h>
#include <topo_factory.h>
#include <netdesign_factory.h>
#include <netview_factory.h>
#include <adaptive_routing.h>
#include <eval_parameters.h>


/* Class definitions for the RTNetwork class*/

/* Constructor v1 : with both network & flow info*/
RTNetwork::RTNetwork(uint num_Nodes, uint num_Links, 
    std::vector<Node*>& nodes_lst, std::vector<Link*>& links_lst,
    std::vector<Flow*>& fs, std::vector<AperiodicFlow*>& afs, latency_scheme ds){
        
        numNodes = num_Nodes;
        numLinks = num_Links;
        nodes = nodes_lst;
        links = links_lst;
        
        populate_adjacent_edges(nodes, links);
        
        flows = fs;
        aflows = afs;
        empty_network_flag = true;
        delay_scheme = ds;
}

/* Constructor v2 : with network info, but without flow info*/
RTNetwork::RTNetwork(uint num_Nodes, uint num_Links,
    std::vector<Node*>& nodes_lst, std::vector<Link*>& link_lst){
    
        numNodes = num_Nodes;
        numLinks = num_Links;
        nodes = nodes_lst;
        links = link_lst;
        
        populate_adjacent_edges(nodes, links);
        
        if(DBG_SYSTEM_FACTORY)
            std::cout<<"Populated edges and nodes"<<std::endl;
}

/* Returns link between source and destination node */
Link** RTNetwork::getLink(Node* source, Node* destination, Link** result){
    
    //std::cout<<"Number of links = "<<links.size()<<std::endl;
    for (auto l : links){
        if ((l->node1 == source) && (l->node2 == destination) || (l->node1 == destination) && (l->node2 == source)){
            //std::cout<<"MATCHED"<<l->node1<<" "<<source<<" "<<l->node2<<" "<<destination<<std::endl;
            *result = l;
            break;
        }
    }
    return result;  
}

/* Print the RTNetwork */
void RTNetwork::printNetwork(){

    std::cout<<"**********************************************"<<std::endl;
    std::cout<<"*************PRINTING THE NETWORK*************"<<std::endl;
    std::cout<<"**********************************************"<<std::endl;

    std::cout<<"Nodes : ";
    for (auto n:nodes)
        std::cout<<n->node_name<<" ";
    std::cout<<std::endl;

    std::cout<<"Links : ";
    for (auto l:links)
        std::cout<<l->link_name<<" ";
    std::cout<<std::endl;

    for (auto l:links)
        std::cout<<l->node1->node_name<<"----"<<l->link_name<<"----"<<l->node2->node_name<<" Bandwidth = "<<l->link_bw_capacity<<std::endl;

    std::cout<<"Regular Flows : ";
    for (auto f:flows)
        std::cout<<f->flow_name<<" : "<<f->source->node_name<<"---->"<<f->destination->node_name<<std::endl;

    std::cout<<"Aperiodic Flows : ";
    for (auto f:aflows)
        std::cout<<f->flow_name<<" : "<<f->source->node_name<<"---->"<<f->destination->node_name<<std::endl;
    
    std::cout<<std::endl;

    std::cout<<"**********************************************"<<std::endl;

}

/* Creates an adjacency list of the RTNetwork given the Nodes and Links */
void RTNetwork::populate_adjacent_edges(std::vector<Node*>& Nodes, 
    std::vector<Link*>& Links){

    for (auto n:Nodes){
        for (auto l:Links){
            if ((n->node_id == l->node1->node_id) or (n->node_id == l->node2->node_id))
                adjListOut[n].push_back(l);
        }
    }
    if (DBG_SYSTEM_FACTORY)
        std::cout<<"---- Done populating adjacency list"<<std::endl;
}

/* Checks if src and dst nodes are connected in the RTNetwork */
bool RTNetwork::check_if_connected(Node* src, Node* dst){

    if (DBG_EXPLORE){
        std::cout<<"***********************************************************************\n";
        std::cout<<"Checking if path exists between "<<src->node_name<<" and "<<dst->node_name<<std::endl;
        std::cout<<"***********************************************************************\n";
    }
    bool result = false;

    std::unordered_map<Node*, bool, CustomNodePHash> visited;
    
    result = explore(src, dst, visited);
    return result;
}

/* explore() method explores from the start node and returns true if finish can be reached */
bool RTNetwork::explore(Node* start, Node* finish,
    std::unordered_map<Node*, bool, CustomNodePHash>& visited){
    Node* discovered;

    if(DBG_EXPLORE)
        std::cout<<"Explore: Start = "<<start->node_name<<" "<<"Finish = "<<finish->node_name<<std::endl;
    
    if (start == finish){           
        if(DBG_EXPLORE)
            std::cout<<"---- ---- Discovered the end node "<<start->node_name<<" address "<<start<<std::endl;
        return true;
    }

    if (visited[start]){
        if(DBG_EXPLORE)
            std::cout<<"---- ---- Already visited node "<<start->node_name<<" address "<<start<<std::endl;   
        return false;
    }

    visited[start] = true;

    //Discover the edges of Node 'start'
    if(DBG_EXPLORE)
        std::cout<<"Exploring links at node "<<start->node_name<<std::endl;
    for (auto link: start->links){
        if(DBG_EXPLORE)
            std::cout<<"---- At "<<start->node_name<<" Link Name = "<<link->link_name<<" , "<<"Link ID = "<<link->link_id<<std::endl;
        discovered = (link->node1 == start)?link->node2:link->node1;
        if(explore(discovered, finish, visited))
            return true;
    }
    return false;
    
}

/* Approach:
* 1. Consider Periodic Flows one by one, starting at the highest priority periodic flow,
* 2. The topology view, 'RTNetView' contains a calculation of delays following the delay analysis scheme used viz., Response-Time Analysis or Delay Composition Calculus,
* 3. The routing scheme, then computes the best possible path for each of the periodic flows considered, using Baruah's Bellman Ford-based scheme
*/
void RTNetwork::place_periodic_flows(){

    bool schedulable;

    for (auto cur_periodic_flow: flows){
        
        schedulable = true;

        if (DBG_NW_DESIGN){
            std::cout<<"\n\n=============================================="<<std::endl;
            std::cout<<"****FLOW PLACEMENT FOR "<<cur_periodic_flow->flow_name<<"***"<<std::endl;
            std::cout<<"=============================================="<<std::endl;
        }
        //Compute the view of the topology as seen by the flow 'cur_periodic_flow'
        //The constructor invokes the compute_view() functionality
        if (DBG_NW_DESIGN){
            std::cout<<"\n\n===================STAGE 1===================="<<std::endl;
            std::cout<<"****************COMPUTING THE VIEW************"<<std::endl;
            std::cout<<"=============================================="<<std::endl;
        }
        RTNetView* topo_view = new RTNetView(cur_periodic_flow, existing_periodic_flows, existing_aperiodic_flows, this);
        if (DBG_VIEW)
            topo_view->printNetView();
        if (DBG_NW_DESIGN){
            std::cout<<"\n=============================================="<<std::endl;
            std::cout<<"----END OF COMPUTING THE VIEW----"<<std::endl;
            std::cout<<"=============================================="<<std::endl;
        }
        

        // Baruah's adaptive routing scheme

        AdaptiveRouting* A = new AdaptiveRouting(topo_view);

        //A->fully_adaptive_realtime_routing(topo_view, empty_network_flag, false);
        if (DBG_NW_DESIGN){
            std::cout<<"\n\n===================STAGE 3===================="<<std::endl;
            std::cout<<"**********ADAPTIVE ROUTING FOR FLOW "<<cur_periodic_flow->flow_id<<" *******"<<std::endl;
            std::cout<<"=============================================="<<std::endl;
        }
        switch(delay_scheme){
            case latency_scheme::RTA:
                schedulable = A->semi_adaptive_realtime_routing_rta(topo_view, empty_network_flag, false);
                if(empty_network_flag)
                    empty_network_flag = false;
                break;
            
            case latency_scheme::NC:
                schedulable = A->semi_adaptive_realtime_routing_nc(topo_view, empty_network_flag, false);
                if(empty_network_flag)
                    empty_network_flag = false;
                break;

            case latency_scheme::DCT:
                schedulable = A->semi_adaptive_realtime_routing_dct(topo_view, empty_network_flag, false);
                if(empty_network_flag)
                    empty_network_flag = false;
                break;                                

        }

        // schedulable = A->semi_adaptive_realtime_routing(topo_view, empty_network_flag, false);
        // empty_network_flag = false;        

        /*
        - Assign flows to path
        - Update enterFlows, exitFlows data structure for every node along the path
        - Update flowsDirect, flowsReverse data structure for every link along the path
        */
        
        //cur_flow->assigned_path->reversePath();
        if (schedulable){
            // std::cout<<"Path satisfies schedulability :";
            // topo_view->printPath(cur_periodic_flow->assigned_path);

            //Update existing_flows list
            existing_periodic_flows.push_back(cur_periodic_flow);

            for(auto x:cur_periodic_flow->assigned_path->nodes)
                x->enterFlows.push_back(cur_periodic_flow);

            for(auto y:cur_periodic_flow->assigned_path->links)
                y->flowsDirect.push_back(cur_periodic_flow);
            
            cur_periodic_flow->placed = true;
        }
        if (DBG_NW_DESIGN){
            std::cout<<"\n==============================================="<<std::endl;
            std::cout<<"********END OF ADAPTIVE ROUTING FOR FLOW "<<cur_periodic_flow->flow_id<<" *****"<<std::endl;
            std::cout<<"================================================"<<std::endl;
        }
    }
}

void RTNetwork::place_aperiodic_flows(){

}

std::pair<double, double> RTNetwork::post_placement_performance_analysis_csv(std::string output_dir_fullpath, std::string lat_res_file, std::string acceptance_ratio_res_file){

    Link *L ;
    uint i;
    double dc_acceptance_ratio;
    double rta_acceptance_ratio;
    double cur_acceptance_ratio;
    uint schedulable_under_rta = 0;
    uint schedulable_under_dc = 0;
    uint schedulable_under_cur = 0;
    double rta_delay_val;

    // Latency Results csv file
    std::string latency_output_file_full_path = output_dir_fullpath + lat_res_file;
    std::ofstream csv(latency_output_file_full_path, std::ios::app);

    if (!csv.is_open()) {
        std::cerr << "Error opening latency_results.csv" << std::endl;
        return std::make_pair(-1.0, -1.0);
    }

    csv << "\n\nFlowName,Priority,Expected-Delay-RTA,Expected-Delay-FRC,"
       "Actual-Delay,Relative-Deadline,Path\n";
    
    for (auto f:flows){

        double delay_val = -1.0;
        FlowPath fp;
        auto it = std::find(existing_periodic_flows.begin(), existing_periodic_flows.end(), f);

        if (it != existing_periodic_flows.end()){
            // the flow is one which was schedulable
            delay_val = f->actual_delay;//cur_periodic_flow->getPathDelay(cur_periodic_flow->assigned_path);//C->getPathDelay(*cur_periodic_flow, *(cur_periodic_flow->assigned_path));
            fp = make_pair(f, f->assigned_path);
            latency_results[fp] = delay_val;
            csv<<fp.first->flow_name<< ","
                <<f->priority<< ","
                <<format_time(f->expected_min_delay_rta)<< ","
                <<format_time(f->expected_min_delay_frc)<< ","
                <<format_time(latency_results[fp])<< ","
                <<format_time(fp.first->relative_deadline)<< ",";
            //printPath(f->assigned_path);

            for (i = 0; i < (f->assigned_path->nodes).size()-1; i++){
                getLink((f->assigned_path)->nodes[i], (f->assigned_path)->nodes[i+1], &L);
                csv<<(f->assigned_path)->nodes[i]->node_name<<"--"<<(*L).link_name<<"--";
            }

            csv<<(f->assigned_path)->nodes[i]->node_name;
            csv<<std::endl;

            if (delay_val <= f->relative_deadline)
                schedulable_under_cur += 1;
        }
        else{
            // the flow is one which was not schedulable
            delay_val = f->expected_min_delay_frc;
            csv<<f->flow_name<<","
                <<f->priority<<","
                <<format_time(f->expected_min_delay_rta)<<","
                <<format_time(f->expected_min_delay_frc)<<","
                <<"NO PATH"<<","
                <<format_time(f->relative_deadline)<<",";
            csv<<std::endl;
        }

        if (f->expected_min_delay_rta <= f->relative_deadline)
            schedulable_under_rta += 1;
        
        if (f->expected_min_delay_frc <= f->relative_deadline)
            schedulable_under_dc += 1;

        //std::cout<<f->expected_min_delay_frc<<" "<<f->expected_min_delay_rta<<" "<<f->relative_deadline<<std::endl;
    }

    for (auto f : flows){
        std::string per_flow_results_filename = f->flow_name + "_" + lat_res_file;
        std::string per_flow_results_file_full_path = output_dir_fullpath + per_flow_results_filename;
        filter_flow_stats(f->flow_name, latency_output_file_full_path, per_flow_results_file_full_path);
    }

    csv.close();
    //std::cout<<"\nAshish ==> "<<schedulable_under_rta<<" "<<schedulable_under_dc<<" "<<(double) existing_periodic_flows.size()<<std::endl;

    // dc_acceptance_ratio = ((double) existing_periodic_flows.size())/flows.size();
    // rta_acceptance_ratio = ((double)schedulable_under_rta)/flows.size();

    //Acceptance Ratio Result file
    std::string acceptance_ratio_output_file_full_path = output_dir_fullpath + acceptance_ratio_res_file;
    std::ofstream ar_csv(acceptance_ratio_output_file_full_path, std::ios::app);

    dc_acceptance_ratio = ((double)schedulable_under_dc)/flows.size();
    rta_acceptance_ratio = ((double)schedulable_under_rta)/flows.size();
    cur_acceptance_ratio = ((double)schedulable_under_cur)/flows.size();

    if (DBG_NW_DESIGN){
        std::cout<<"Acceptance Ratio (DC)= "<<(double) schedulable_under_dc<<"/"<<flows.size()<<" = "<<dc_acceptance_ratio<<std::endl;
        std::cout<<"Acceptance Ratio (RTA)= "<<(double) schedulable_under_rta<<"/"<<flows.size()<<" = "<<rta_acceptance_ratio<<std::endl;
        std::cout<<"Acceptance Ratio (CUR)= "<<(double) schedulable_under_cur<<"/"<<flows.size()<<" = "<<cur_acceptance_ratio<<std::endl<<std::endl;
    }
    
    //ar_csv << "Deadline,Acceptance Ratio (DC),Acceptance Ratio (RTA),Acceptance Ratio (CUR)\n";
    ar_csv << format_time(flows[0]->relative_deadline)<<","<<dc_acceptance_ratio<<","<<rta_acceptance_ratio<<","<<cur_acceptance_ratio<<std::endl;
    ar_csv.close();

    //std::cout<<"File write to "<<latency_output_file_full_path<<" and "<<acceptance_ratio_output_file_full_path<<" done"<<std::endl;

    return std::make_pair(dc_acceptance_ratio, rta_acceptance_ratio);

}

void RTNetwork::post_placement_performance_analysis(){
    
    std::cout<<std::endl<<std::endl;

    std::cout<<"\n\n"<<std::string(240, '=')<<std::endl;
    std::cout<<"Flow\t\t"
        <<std::setw(10)<<std::left<<"Priority "
        <<std::setw(30)<<std::left<<"Exp. Min Delay (RTA) "
        <<std::setw(30)<<std::left<<"Exp. Min Delay (FRC) "
        <<std::setw(30)<<std::left<<"Act. Delay "
        <<std::setw(30)<<std::left<<"Deadline"
        <<std::setw(30)<<std::left<<"Assigned Path "<<std::endl;
    std::cout<<std::string(240, '=')<<std::endl;
    
    /*
    for (auto cur_periodic_flow: existing_periodic_flows){
        // Create a view for each flow
        // The constructor invokes the compute_view() functionality
        // RTNetView* topo_view = new RTNetView(cur_periodic_flow, existing_periodic_flows, existing_aperiodic_flows, this);
        //DelayModel *C = new DelayModel(topo_view);

        double delay_val = cur_periodic_flow->actual_delay;//cur_periodic_flow->getPathDelay(cur_periodic_flow->assigned_path);//C->getPathDelay(*cur_periodic_flow, *(cur_periodic_flow->assigned_path));
        FlowPath fp = make_pair(cur_periodic_flow, cur_periodic_flow->assigned_path);
        latency_results[fp] = delay_val;
        std::cout<<fp.first->flow_name<<"\t\t"
            <<std::setw(20)<<std::left<<cur_periodic_flow->expected_min_delay_rta
            <<std::setw(20)<<std::left<<cur_periodic_flow->expected_min_delay_frc
            <<std::setw(20)<<std::left<<format_time(latency_results[fp])
            <<std::setw(20)<<std::left<<format_time(fp.first->relative_deadline);
        printPath(cur_periodic_flow->assigned_path);
    }
    */

    for (auto f: flows){

        double delay_val;
        FlowPath fp;
        auto it = std::find(existing_periodic_flows.begin(), existing_periodic_flows.end(), f);

        if (it != existing_periodic_flows.end()){
            // the flow is one which was schedulable
            delay_val = f->actual_delay;//cur_periodic_flow->getPathDelay(cur_periodic_flow->assigned_path);//C->getPathDelay(*cur_periodic_flow, *(cur_periodic_flow->assigned_path));
            fp = make_pair(f, f->assigned_path);
            latency_results[fp] = delay_val;
            std::cout<<fp.first->flow_name<<"\t\t"
                <<std::setw(10)<<std::left<<f->priority
                <<std::setw(30)<<std::left<<format_time(f->expected_min_delay_rta)
                <<std::setw(30)<<std::left<<format_time(f->expected_min_delay_frc)
                <<std::setw(30)<<std::left<<format_time(latency_results[fp])
                <<std::setw(30)<<std::left<<format_time(fp.first->relative_deadline);
            printPath(f->assigned_path);
        }
        else{
            // the flow is one which was not schedulable
            delay_val = f->expected_min_delay_frc;
            std::cout<<f->flow_name<<"\t\t"
                <<std::setw(10)<<std::left<<f->priority
                <<std::setw(30)<<std::left<<format_time(f->expected_min_delay_rta)
                <<std::setw(30)<<std::left<<format_time(f->expected_min_delay_frc)
                <<std::setw(30)<<std::left<<""
                <<std::setw(30)<<std::left<<format_time(f->relative_deadline);
            std::cout<<std::endl;
        } 
    }

    std::cout<<std::string(240, '=')<<std::endl;

}

void RTNetwork::printPath(Path* P){

    std::vector<Node*>::iterator node_it1, node_it2;
    std::vector<Link*>::iterator link_it;
    Link *L ;
    uint i;

    for (i = 0; i < (P->nodes).size()-1; i++){
            getLink(P->nodes[i], P->nodes[i+1], &L);
            std::cout<<P->nodes[i]->node_name<<"--"<<(*L).link_name<<"--";
    }
    std::cout<<P->nodes[i]->node_name;

    //std::cout<<std::endl;

}
