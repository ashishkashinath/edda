#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>

#include <netdesign_factory.h>
#include <topo_factory.h>
#include <flow_factory.h>
#include "design_space_exp.h"
#include <adaptive_routing.h>
#include <flow_gen.h>
#include <util.h>
#include <debug_config.h>

#include <pugixml/pugixml.hpp>
#include <pugixml/pugiconfig.hpp>

#include <fusion.h>
using namespace mosek::fusion;
using namespace monty;

/*
* User-defined constructor for DesignSpaceExploration class
*/
NetworkGenerator::NetworkGenerator(uint nr_nodes, uint nr_links, FlowGenerator** FG, latency_scheme ds){
    num_nodes = nr_nodes;
    num_links = nr_links;
    fg = *FG;
    delay_scheme = ds;
}
/*
* Function to generate subsets of a set of links, i.e., std::vector<Link *>
*/
void NetworkGenerator::generateLinkSubsets(std::vector<Link *>& links, int k, int start, std::vector<Link *>& current, std::vector<std::vector<Link *>>& result) {
    if (current.size() == k) {
        result.push_back(current);
        return;
    }
    for (int i = start; i < links.size(); i++) {
        current.push_back(links[i]);
        generateLinkSubsets(links, k, i + 1, current, result);
        current.pop_back();
    }
}
/*
* Wrapper function that calls generateLinkSubsets
*/
std::vector<std::vector<Link*>> NetworkGenerator::allLinkSubsetsOfSizeK(std::vector<Link*>& nums, int k) {
    std::vector<std::vector<Link*>> result;
    std::vector<Link*> current;
    generateLinkSubsets(nums, k, 0, current, result);
    return result;
}
/*
* Function to generate a random topology, for a given number of nodes and number of links,
* the constraint is that the topology should contain the specified number of links and nodes,
*/
void NetworkGenerator::generateAllRandomTopologies(){

    //User Inputs are stored in numEdges and numVertices
    uint numEdges = num_links;
    uint numVertices = num_nodes;

    //Clearing node_specifications and link_specifications
    node_specifications.clear();
    link_specifications.clear();
    mesh_link_specifications.clear();

    //Nodes have a node-capacity, links have a link capacity
    //Different topologies have different nodes of different capacities
    //The links can also be between different start and end nodes - various shapes
    double min_node_cap_bps = MIN_NODE_CAP_BPS;
    double max_node_cap_bps = MAX_NODE_CAP_BPS;
    double step_size = STEP_SIZE_NODE_CAP_BPS;

    double min_link_cap_typical = MIN_LINK_CAP_TYPICAL;
    double max_link_cap_typical = MAX_LINK_CAP_TYPICAL;
    double min_link_cap_wc = MIN_LINK_CAP_WC;
    double max_link_cap_wc = MAX_LINK_CAP_WC;

    //Switch and Link IDs
    uint sw_id = 1;
    uint link_id = 1;

    //Count number of topologies
    uint count_topo = 1;

    //Random Device Engine
    auto rng = std::default_random_engine {};

    //Instance of the RTNetwork to check if the flows have paths
    //We should complete the layout of flows given a RTNetwork
    RTNetwork* rt_network_design;

    //The parameters of the switch and the link
    double sw_bw_cap_bps;
    double typical_link_bw_cap_bps;
    double worst_link_bw_cap_bps;

    //Selecting from a list of possible bandwidths
    std::vector<double> bandwidth_lst = make_vector_with_step(min_node_cap_bps, step_size, max_node_cap_bps);
    
    //Create 'num_nodes' number of nodes
    if (DBG_DSE)
        std::cout<<"==== CREATING NODES ===="<<std::endl;
    while(numVertices > 0){
        //Create a name for the switch
        std::string sw_name = "Switch " + std::to_string(sw_id);
        //std::cout<<"Creating Switch "<<sw_name<<std::endl;
        
        //Select a bandwidth for it from the list of possible bandwidths
        double sw_bw_cap_bps = bandwidth_lst[pick_rand_index_from_a_list(bandwidth_lst)];

        Node* N_ptr = new Node(sw_id, sw_name, sw_bw_cap_bps);
        
        if (DBG_DSE)
            std::cout<<"Node = "<<sw_name<<" Address = "<<N_ptr<<std::endl;

        node_specifications.push_back(N_ptr);
        
        sw_id += 1;
        numVertices -=1;
    }
    if (DBG_DSE)
        std::cout<<"==== DONE CREATING NODES ===="<<std::endl<<std::endl;
    
    //Add max possible number of edges i.e., numVertices*(numVertices-1)/2
    if (DBG_DSE)
        std::cout<<"==== CREATING EDGES ===="<<std::endl;
    for (Node* src_node:node_specifications){
        for (Node* dest_node:node_specifications){
            uint link_head = src_node->getid();
            uint link_tail = dest_node->getid();
            
            if(link_head < link_tail){
                //Connect them with a Link

                //Pick a typical cost
                typical_link_bw_cap_bps = generate_random_double(min_link_cap_typical, max_link_cap_typical);

                //Pick a worst-case cost
                worst_link_bw_cap_bps = generate_random_double(min_link_cap_wc, max_link_cap_wc);

                //Pick a link name
                std::string link_name = "Link " + std::to_string(link_id);
                std::string link_source = "Switch " + std::to_string(link_head);
                std::string link_destination = "Switch " + std::to_string(link_tail);
            
                //Add a edge
                Link* L_ptr = new Link(link_id, link_name, worst_link_bw_cap_bps, link_source, link_destination);
                
                if (DBG_DSE)
                    std::cout<<"Link = "<<link_name<<" Address = "<<L_ptr<<std::endl;
                
                mesh_link_specifications.push_back(L_ptr);
                
                // if (numEdges != 0)
                //     numEdges -= 1;
                //std::cout<<"Added link "<<link_id<<std::endl;
                link_id += 1;
            }
        }
    }
    if (DBG_DSE)
        std::cout<<"==== DONE CREATING MESH EDGES ===="<<std::endl<<std::endl;

    uint num_edges_in_mesh = mesh_link_specifications.size();
    
    if (DBG_DSE){
        std::cout<<"Number of links in Mesh Topology = "<<num_edges_in_mesh<<std::endl;
        std::cout<<"Desired Number of links in Topology = "<<num_links<<std::endl;
    }

    link_specifications = mesh_link_specifications;
    possible_link_specifications = allLinkSubsetsOfSizeK(link_specifications, num_links);

    std::shuffle(std::begin(possible_link_specifications), std::end(possible_link_specifications), rng);
    
    num_topologies = possible_link_specifications.size();
    
    std::cout<<"Total Number of Possible Topologies = "<<num_topologies<<std::endl;

    /*
    * Analyze inter-dependencies between nodes and links to capture inter-dependent specs
    * 
    */
    bool all_flows_have_paths;
    uint count_no_path = 0;
    count_topo = 1;

    std::cout<<"\n\n\n";
            
    // Each *L is a different topology
    // capture_all_interdependencies allows to fill in topology information

    for (std::vector<std::vector<Link*>>::iterator L = possible_link_specifications.begin(); L != possible_link_specifications.end(); ++L){
        
        //capture_node_link_interdependencies(node_specifications, *L);
        capture_all_interdependencies(node_specifications, *L, fg->generated_periodic_flows, fg->generated_aperiodic_flows);
        
        std::cout<<"Path Layout on Topology "<<count_topo<<" with links --> "<<"(";
        for (Link* x: *L){
                std::cout<<x->link_name<<" ";
        }
        std::cout<<")";
        std::cout<<"\n";

        //Create a RTNetwork with this specifications for design-time
        if (DBG_DSE){
            std::cout<<"Going to create an Real-Time Network with "<<num_nodes<<" nodes and "<<num_links<<" links"<<std::endl;
            std::cout<<"Number of generated periodic flows = "<<fg->generated_periodic_flows.size()<<std::endl;
            std::cout<<"Number of generated aperiodic flows = "<<fg->generated_aperiodic_flows.size()<<std::endl;
            std::cout<<"Number of links in the topology = "<<(*L).size()<<std::endl;
        }

        rt_network_design = new RTNetwork(num_nodes, num_links, node_specifications, *L, fg->generated_periodic_flows, fg->generated_aperiodic_flows, delay_scheme);

        //Check if all the flows have paths
        //Even if one of the flows don't have a path, we should ignore the topology
        all_flows_have_paths = true;
        for (auto f : fg->generated_periodic_flows){

            if (DBG_DSE)
                std::cout<<"Checking if a path exists for flow "<<f->flow_name<<" between "<<f->source->get_name()<<" and "<<f->destination->get_name()<<std::endl;
            
            if(!rt_network_design->check_if_connected(f->source, f->destination)){
                std::cout<<"Path doesn't exist between "<<f->source->node_name<<" and "<<f->destination->node_name<<std::endl;
                // std::remove and std::remove_if don’t actually erase elements; 
                // they shift the valid ones to the front and return a new logical end iterator;
                // That's why erase is needed.
                count_no_path += 1;
                auto noValEnd = std::remove(possible_link_specifications.begin(), possible_link_specifications.end(), *L);
                possible_link_specifications.erase(noValEnd, possible_link_specifications.end());
                all_flows_have_paths = false;
                break;
            }
        }
        
        if (!all_flows_have_paths){
            all_flows_have_paths = true;
            // we want to discard that set of links
            // delete references in the node
            // delete references in the RTNetwork
            std::cout<<"Path does not exist in Topology "<<count_topo<<std::endl;
            for (auto n: node_specifications){
                n->links.clear();
                rt_network_design->adjListOut[n].clear();
            }
        }
            
        else{

            /*We need to make sure that the length of the node_specifications and link_specifications is as asked */
            if (DBG_DSE){
                std::cout<<"\nDesired Number of Nodes = "<<num_nodes<<", Created Number of Nodes = "<<node_specifications.size()<<std::endl;
                std::cout<<"Desired Number of Links = "<<num_links<<", Created Mesh with Number of Links = "<<link_specifications.size()<<std::endl;
                std::cout<<"Number of Topologies with "<<num_links<<" links = "<<num_topologies<<std::endl<<std::endl<<std::endl;
            }

            assert (num_nodes == node_specifications.size());
            assert (num_links == (*L).size());

            // This is link combination guarantees paths for all flows in fg->generated_periodic_flows
            // capture_all_interdependencies will allow us to go from node_names to node_refs in flows
            // capture_all_interdependencies(node_specifications, *L, fg->generated_periodic_flows, fg->generated_aperiodic_flows);
            rt_network_design->printNetwork();

            std::cout<<"\\n\n=================================================================="<<std::endl;
            std::cout<<"********************PLACING PERIODIC FLOWS********************"<<std::endl;
            std::cout<<"=================================================================="<<std::endl;   
            rt_network_design->place_periodic_flows();
            std::cout<<"\n=================================================================="<<std::endl;
            std::cout<<"********************END OF PLACING PERIODIC FLOWS********************"<<std::endl;
            std::cout<<"=================================================================="<<std::endl;

            std::cout<<"\n\n=================================================================="<<std::endl;
            std::cout<<"********************PLACING APERIODIC FLOWS********************"<<std::endl;
            std::cout<<"=================================================================="<<std::endl;
            rt_network_design->place_aperiodic_flows();
            std::cout<<"\n=================================================================="<<std::endl;
            std::cout<<"********************END OF PLACING APERIODIC FLOWS********************"<<std::endl;
            std::cout<<"=================================================================="<<std::endl;
        }

        count_topo = count_topo + 1;

        rt_network_design->post_placement_performance_analysis();

        // Cleanup references updated by capture_all_dependencies()
        // for creating the next topology
        for (auto n: node_specifications){
            n->links.clear();
            rt_network_design->adjListOut[n].clear();
        }

        //Cleanup flow
        for (auto F: fg->generated_periodic_flows){
            F->assigned_path = NULL;
            F->actual_delay = 0;
            F->all_possible_paths.clear();
            F->expected_min_delay_rta = __DBL_MAX__;
            F->expected_min_delay_frc = __DBL_MAX__;
            F->placed = false;
        }
    }

    std::cout<<"Number of Topologies with no path for some flow = "<<count_no_path<<std::endl;
}
/*
* Function to generate a random topology, for a given number of nodes and number of links,
* the constraint is that the topology should contain the specified number of links and nodes,
*/
void NetworkGenerator::generateRandomTopology(){

    //User Inputs are stored in numEdges and numVertices
    uint numEdges = num_links;
    uint numVertices = num_nodes;

    std::cout<<"Num. of Switches"<<" "<<numVertices<<"\nNum. of Links"<<" "<<numEdges<<std::endl;

    //Clearing node_specifications and link_specifications
    node_specifications.clear();
    link_specifications.clear();
    mesh_link_specifications.clear();

    //Nodes have a node-capacity, links have a link capacity
    //Different topologies have different nodes of different capacities
    //The links can also be between different start and end nodes - various shapes
    double min_node_cap_bps = MIN_NODE_CAP_BPS;
    double max_node_cap_bps = MAX_NODE_CAP_BPS;
    double step_size = STEP_SIZE_NODE_CAP_BPS;

    double min_link_cap_typical = MIN_LINK_CAP_TYPICAL;
    double max_link_cap_typical = MAX_LINK_CAP_TYPICAL;
    double min_link_cap_wc = MIN_LINK_CAP_WC;
    double max_link_cap_wc = MAX_LINK_CAP_WC;

    //Switch and Link IDs
    uint sw_id = 1;
    uint link_id = 1;

    //Instance of the RTNetwork to check if the flows have paths
    //We should complete the layout of flows given a RTNetwork
    RTNetwork* rt_network_design;

    bool all_flows_have_paths;

    //The parameters of the switch and the link
    double sw_bw_cap_bps;
    double typical_link_bw_cap_bps;
    double worst_link_bw_cap_bps;

    //Selecting from a list of possible bandwidths
    std::vector<double> bandwidth_lst = make_vector_with_step(min_node_cap_bps, step_size, max_node_cap_bps);
    
    //std::cout<<"==== CREATING NODES ===="<<std::endl;

    //Create 'num_nodes' number of nodes
    while(numVertices > 0){
        //Create a name for the switch
        std::string sw_name = "Switch " + std::to_string(sw_id);
        //std::cout<<"Creating Switch "<<sw_name<<std::endl;
        
        //Select a bandwidth for it from the list of possible bandwidths
        double sw_bw_cap_bps = bandwidth_lst[pick_rand_index_from_a_list(bandwidth_lst)];

        Node* N_ptr = new Node(sw_id, sw_name, sw_bw_cap_bps);
        
        if (DBG_DSE)
            std::cout<<"Node = "<<sw_name<<" Address = "<<N_ptr<<std::endl;

        node_specifications.push_back(N_ptr);
        
        sw_id += 1;
        numVertices -=1;
    }

    //std::cout<<"==== DONE CREATING NODES ===="<<std::endl<<std::endl;

    // Add max possible number of edges i.e., numVertices*(numVertices-1)/2
    
    //std::cout<<"==== CREATING EDGES ===="<<std::endl;
    for (Node* src_node:node_specifications){
        for (Node* dest_node:node_specifications){
            uint link_head = src_node->getid();
            uint link_tail = dest_node->getid();
            
            if(link_head < link_tail){
                //Connect them with a Link

                //Pick a typical cost
                //typical_link_bw_cap_bps = generate_random_double(min_link_cap_typical, max_link_cap_typical);
                typical_link_bw_cap_bps = 100e6;

                //Pick a worst-case cost
                //worst_link_bw_cap_bps = generate_random_double(min_link_cap_wc, max_link_cap_wc);
                worst_link_bw_cap_bps = 100e6;

                //Pick a link name
                std::string link_name = "Link " + std::to_string(link_id);
                std::string link_source = "Switch " + std::to_string(link_head);
                std::string link_destination = "Switch " + std::to_string(link_tail);
            
                //Add a edge
                Link* L_ptr = new Link(link_id, link_name, worst_link_bw_cap_bps, link_source, link_destination);
                
                if (DBG_DSE)
                    std::cout<<"Link = "<<link_name<<" Address = "<<L_ptr<<std::endl;
                
                mesh_link_specifications.push_back(L_ptr);
                
                // if (numEdges != 0)
                //     numEdges -= 1;
                //std::cout<<"Added link "<<link_id<<std::endl;
                link_id += 1;
            }
        }
    }

    //std::cout<<"==== DONE CREATING MESH EDGES ===="<<std::endl<<std::endl;

    uint num_edges_in_mesh = mesh_link_specifications.size();
    std::cout<<"Number of links in mesh topology = "<<num_edges_in_mesh<<std::endl;
    std::cout<<"Desired Number of links in final topology = "<<num_links<<std::endl<<std::endl;

    //Remove some links from link_specifications until you have the target num_links

    // std::copy(mesh_link_specifications.begin(), mesh_link_specifications.end(), std::back_inserter(link_specifications));
    link_specifications = mesh_link_specifications;
    
    int removed_edge_index;
    std::vector<Link*>::iterator removed_link_it;
    
    //Create one configuration with required number of edges
    while (num_edges_in_mesh > numEdges){
        removed_edge_index = generate_random_int(0, link_specifications.size()-1);
        removed_link_it = link_specifications.begin() + removed_edge_index;
        auto noValEnd = std::remove(link_specifications.begin(), link_specifications.end(), *removed_link_it);
        link_specifications.erase(noValEnd, link_specifications.end());
        //link_specifications.erase(removed_link_it);        
        num_edges_in_mesh--;
    }

    /*Analyze inter-dependencies between nodes and links to capture inter-dependent specs*/
    //capture_node_link_interdependencies(node_specifications, link_specifications);
    capture_all_interdependencies(node_specifications, link_specifications, fg->generated_periodic_flows, fg->generated_aperiodic_flows);
    
    /*We need to make sure that the length of the node_specifications and link_specifications is as asked */
    if (DBG_DSE){
        std::cout<<num_nodes<<" "<<node_specifications.size()<<std::endl;
        std::cout<<num_links<<" "<<link_specifications.size()<<std::endl;
    }

    assert (num_nodes == node_specifications.size());
    assert (num_links == link_specifications.size());

    std::cout<<"==== PATH LAYOUT ====\n";
    rt_network_design = new RTNetwork(num_nodes, num_links, node_specifications, link_specifications, fg->generated_periodic_flows, fg->generated_aperiodic_flows, delay_scheme);

    //Check if all the flows have paths
    //Even if one of the flows don't have a path, we should ignore the topology

    all_flows_have_paths = true;
    for (auto f : fg->generated_periodic_flows){

        if (DBG_DSE)
            std::cout<<"Checking if a path exists for flow "<<f->flow_name<<" between "<<f->source->get_name()<<" and "<<f->destination->get_name()<<std::endl;
        
        if(!rt_network_design->check_if_connected(f->source, f->destination)){
            std::cout<<"Path doesn't exist between "<<f->source->node_name<<" and "<<f->destination->node_name<<std::endl;
            // std::remove and std::remove_if don’t actually erase elements; 
            // they shift the valid ones to the front and return a new logical end iterator;
            // That's why erase is needed.
            all_flows_have_paths = false;
            break;
        }
    }

    if (!all_flows_have_paths){
        all_flows_have_paths = true;
        // we want to discard that set of links
        // delete references in the node
        // delete references in the RTNetwork
        //std::cout<<"Path does not exist in Topology "<<count_topo<<std::endl;
        for (auto n: node_specifications){
            n->links.clear();
            rt_network_design->adjListOut[n].clear();
        }
    }
            
    else{

        // This is link combination guarantees paths for all flows in fg->generated_periodic_flows
        // capture_all_interdependencies will allow us to go from node_names to node_refs in flows
        // capture_all_interdependencies(node_specifications, *L, fg->generated_periodic_flows, fg->generated_aperiodic_flows);
        rt_network_design->printNetwork();

        std::cout<<"\t\t\t=============================================="<<std::endl;
        std::cout<<"\t\t\t****PLACING PERIODIC FLOWS****"<<std::endl;
        std::cout<<"\t\t\t=============================================="<<std::endl;   
        rt_network_design->place_periodic_flows();

    }
}

