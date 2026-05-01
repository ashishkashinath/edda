#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>

#include <netdesign_factory.h>
#include <flow_gen.h>
#include <topo_factory.h>
#include <eval_parameters.h>
#include <flow_factory.h>
#include <adaptive_routing.h>
#include <util.h>
#include <debug_config.h>

#include <pugixml/pugixml.hpp>
#include <pugixml/pugiconfig.hpp>

#include "fusion.h"
using namespace mosek::fusion;
using namespace monty;

/*
* User-defined constructor for FlowGenerator class
*/
FlowGenerator::FlowGenerator(std::vector<Flow*> flow_specifications, std::vector<AperiodicFlow*> aflow_specifications){
    generated_periodic_flows = flow_specifications;
    generated_aperiodic_flows = aflow_specifications;
}

/*
* User-defined constructor for FlowGenerator class
*/
FlowGenerator::FlowGenerator(int nr_flows, NetworkGenerator* ng){
    num_flows = nr_flows;
    NG = ng;
}

/*
* User-defined constructor for FlowGenerator class
*/
FlowGenerator::FlowGenerator(int nr_flows){
    num_flows = nr_flows;
}
/*
* Generate periodic flows across a deadline range [deadline_begin, deadline_end]
* This uses constructor that uses node references rather than node names 
* So, there is no need to use capture_all_interdependencies() after generating the topologies
*/
std::vector<Flow*> FlowGenerator::generate_periodic_flows_across_deadline_range_use_node_refs(double deadline_begin, double deadline_end){

    uint flow_id;
    Node* flow_source;
    Node* flow_destination;
    Node* x;


    std::string flow_name;
    std::string flow_source_name;
    std::string flow_destination_name;
    double packet_size_bytes;
    double flow_bw_cap_bps;
    double flow_period;
    double flow_deadline;
    uint flow_prio;
    flow_type ft;
    flow_safety_mode fsm;


    std::vector<Node *> all_nodes = NG->node_specifications;

    //Selecting from a list of possible packet sizes
    std::vector<double> pkt_sizes_bytes_lst = make_vector_with_step(PKT_SIZE_BYTES_MIN, DEADLINE_INCREMENT_SEC, PKT_SIZE_BYTES_MAX);

    //Selecting from a list of possible flow bandwidths
    std::vector<double> flow_bandwidth_lst = make_vector_with_step(MIN_FLOW_BW, STEP_SIZE_FLOW_BW, MAX_FLOW_BW);

    //Selecting from a list of possible flow periods
    std::vector<double> flow_period_lst = make_vector_with_step(MIN_FLOW_PERIOD_SEC, STEP_SIZE_FLOW_PERIOD_SEC, MAX_FLOW_PERIOD_SEC);

    //Selecting from a list of possible priorities
    std::vector<short> flow_priority_lst = make_short_vector_with_step(PRIORITY_MAX, STEP_SIZE_PRIORITY, PRIORITY_MIN);

    //Populate list of periodic flows
    for (uint i = 1; i <= num_flows; i++){

        flow_id = i;

        flow_name = std::string("Flow ") + std::to_string(flow_id);
        flow_deadline = generate_random_double(deadline_begin, deadline_end);

        flow_source = all_nodes[generate_random_int(0, all_nodes.size()-1)];
        flow_source_name = flow_source->get_name();

        x = all_nodes[generate_random_int(0, all_nodes.size()-1)];
        while(x == flow_source){
            x = all_nodes[generate_random_int(0, all_nodes.size()-1)];
        }

        flow_destination = x;
        flow_destination_name = flow_destination->get_name();
        packet_size_bytes = pkt_sizes_bytes_lst[pick_rand_index_from_a_list(pkt_sizes_bytes_lst)];
        flow_bw_cap_bps = flow_bandwidth_lst[pick_rand_index_from_a_list(flow_bandwidth_lst)];
        flow_period = flow_period_lst[pick_rand_index_from_a_list(flow_period_lst)];
        flow_prio = flow_priority_lst[pick_rand_index_from_a_short_list(flow_priority_lst)];
        ft = flow_type::PERIODIC;
        fsm = flow_safety_mode::PRIMARY;

        Flow *F_ptr = new Flow(flow_id, flow_name, *flow_source, *flow_destination, packet_size_bytes, flow_bw_cap_bps, flow_period, flow_deadline, flow_prio);

        //Flow* F_ptr = new Flow(flow_id, flow_name, flow_source_name, flow_destination_name, packet_size_bytes, flow_bw_cap_bps, flow_period, flow_deadline, flow_prio, ft, fsm);

        generated_periodic_flows.push_back(F_ptr);
    }

    //Sort the flows
    std::sort(generated_periodic_flows.begin(), generated_periodic_flows.end(), compareFlow);

    return generated_periodic_flows;

}

/*
* Generate periodic flows across a deadline range [deadline_begin, deadline_end]
* This uses constructor that uses node references rather than node names 
* So, we need to use capture_all_interdependencies() after generating the topologies
*/
std::vector<Flow*> FlowGenerator::generate_periodic_flows_across_deadline_range_use_node_names(double deadline_begin, double deadline_end, int num_nodes){

    uint flow_id;

    int src_num;
    int dst_num;

    std::string flow_name;
    std::string flow_source_name;
    std::string flow_destination_name;
    std::string x;
    double packet_size_bytes;
    double flow_bw_cap_bps;
    double flow_period;
    double flow_deadline;
    uint flow_prio;
    flow_type ft;
    flow_safety_mode fsm;


    std::vector<int> all_nodes(num_nodes);

    std::iota(all_nodes.begin(), all_nodes.end(), 1);

    //Selecting from a list of possible packet sizes
    std::vector<double> pkt_sizes_bytes_lst = make_vector_with_step(PKT_SIZE_BYTES_MIN, PKT_SIZE_BYTES_INCREMENT, PKT_SIZE_BYTES_MAX);

    //Selecting from a list of possible flow bandwidths
    std::vector<double> flow_bandwidth_lst = make_vector_with_step(MIN_FLOW_BW, STEP_SIZE_FLOW_BW, MAX_FLOW_BW);

    //Selecting from a list of possible flow periods
    //std::vector<double> flow_period_lst = {10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000};; 
    std::vector<double> flow_period_lst = make_vector_with_step(MIN_FLOW_PERIOD_SEC, STEP_SIZE_FLOW_PERIOD_SEC, MAX_FLOW_PERIOD_SEC);

    //Selecting from a list of possible flow deadlines
    //std::vector<double> flow_deadline_lst = {10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000};
    std::vector<double> flow_deadline_lst = make_vector_with_step(deadline_begin, DEADLINE_INCREMENT_SEC, deadline_end);

    //Selecting from a list of possible priorities
    std::vector<short> flow_priority_lst = make_short_vector_with_step(PRIORITY_MAX, STEP_SIZE_PRIORITY, PRIORITY_MIN);

    //Populate list of periodic flows
    for (uint i = 1; i <= num_flows; i++){

        flow_id = i;

        flow_name = std::string("Flow ") + std::to_string(flow_id);
        
        src_num = generate_random_int(1, num_nodes);
        dst_num = generate_random_int(1, num_nodes);

        while (src_num >= dst_num){
            src_num = generate_random_int(1, num_nodes);
            dst_num = generate_random_int(1, num_nodes);
        }

        flow_source_name = std::string("Switch ") + to_string(src_num);
        flow_destination_name = std::string("Switch ") + to_string(dst_num);
        
        packet_size_bytes = pkt_sizes_bytes_lst[pick_rand_index_from_a_list(pkt_sizes_bytes_lst)];
        flow_bw_cap_bps = flow_bandwidth_lst[pick_rand_index_from_a_list(flow_bandwidth_lst)];
        flow_period = flow_period_lst[pick_rand_index_from_a_list(flow_period_lst)];
        flow_deadline = flow_deadline_lst[pick_rand_index_from_a_list(flow_deadline_lst)]; //flow_period;
        flow_prio = flow_priority_lst[(flow_id-1) % flow_priority_lst.size()];
        // flow_prio = flow_priority_lst[pick_rand_index_from_a_short_list(flow_priority_lst)];
        ft = flow_type::PERIODIC;
        fsm = flow_safety_mode::PRIMARY;

        Flow *F_ptr = new Flow(flow_id, flow_name, flow_source_name, flow_destination_name, packet_size_bytes, flow_bw_cap_bps, flow_period, flow_deadline, flow_prio, ft, fsm); 

        if (DBG_DSE)
            F_ptr->displayFlow();
        
        generated_periodic_flows.push_back(F_ptr);
        
    }

    //Sort the flows
    std::sort(generated_periodic_flows.begin(), generated_periodic_flows.end(), compareFlow);

    if (DBG_UTIL)
        std::cout<<"---- Number of Periodic Flows = "<<generated_periodic_flows.size()<<std::endl;

    return generated_periodic_flows;

}

std::vector<Flow*> FlowGenerator::generate_periodic_flows_specific_deadline_use_node_names(double given_deadline, int num_nodes){

    uint flow_id;

    int src_num;
    int dst_num;

    std::string flow_name;
    std::string flow_source_name;
    std::string flow_destination_name;
    std::string x;
    double packet_size_bytes;
    double flow_bw_cap_bps;
    double flow_period;
    double flow_deadline;
    uint flow_prio;
    flow_type ft;
    flow_safety_mode fsm;

    std::vector<int> all_nodes(num_nodes);

    std::iota(all_nodes.begin(), all_nodes.end(), 1);

    //Selecting from a list of possible packet sizes
    std::vector<double> pkt_sizes_bytes_lst = make_vector_with_step(PKT_SIZE_BYTES_MIN, PKT_SIZE_BYTES_INCREMENT, PKT_SIZE_BYTES_MAX);

    //Selecting from a list of possible flow bandwidths
    std::vector<double> flow_bandwidth_lst = make_vector_with_step(MIN_FLOW_BW, STEP_SIZE_FLOW_BW, MAX_FLOW_BW);

    //Selecting from a list of possible flow periods
    //std::vector<double> flow_period_lst = {10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000};; 
    std::vector<double> flow_period_lst = make_vector_with_step(MIN_FLOW_PERIOD_SEC, STEP_SIZE_FLOW_PERIOD_SEC, MAX_FLOW_PERIOD_SEC);

    //Selecting from a list of possible priorities
    std::vector<short> flow_priority_lst = make_short_vector_with_step(PRIORITY_MAX, STEP_SIZE_PRIORITY, PRIORITY_MIN);

    //Populate list of periodic flows
    for (uint i = 1; i <= num_flows; i++){

        flow_id = i;

        flow_name = std::string("Flow ") + std::to_string(flow_id);
        
        src_num = generate_random_seeded(1, num_nodes);
        dst_num = generate_random_seeded(1, num_nodes);

        while (src_num >= dst_num){
            src_num = generate_random_seeded(1, num_nodes);
            dst_num = generate_random_seeded(1, num_nodes);
        }

        flow_source_name = std::string("Switch ") + to_string(src_num);
        flow_destination_name = std::string("Switch ") + to_string(dst_num);
        
        packet_size_bytes = pkt_sizes_bytes_lst[pick_rand_index_from_a_list(pkt_sizes_bytes_lst)];
        flow_bw_cap_bps = flow_bandwidth_lst[pick_rand_index_from_a_list(flow_bandwidth_lst)];
        flow_period = flow_period_lst[pick_rand_index_from_a_list(flow_period_lst)];
        flow_deadline = given_deadline;
        flow_prio = flow_priority_lst[(flow_id-1) % flow_priority_lst.size()];
        // flow_prio = flow_priority_lst[pick_rand_index_from_a_short_list(flow_priority_lst)];
        ft = flow_type::PERIODIC;
        fsm = flow_safety_mode::PRIMARY;

        Flow *F_ptr = new Flow(flow_id, flow_name, flow_source_name, flow_destination_name, packet_size_bytes, flow_bw_cap_bps, flow_period, flow_deadline, flow_prio, ft, fsm); 

        if (DBG_DSE)
            F_ptr->displayFlow();
        
        generated_periodic_flows.push_back(F_ptr);
        
    }

    //Sort the flows
    std::sort(generated_periodic_flows.begin(), generated_periodic_flows.end(), compareFlow);

    if (DBG_UTIL)
        std::cout<<"---- Number of Periodic Flows = "<<generated_periodic_flows.size()<<std::endl;

    return generated_periodic_flows;

}

void FlowGenerator::printGeneratedFlows(){
    std::cout<<std::setw(10)<<std::left<<"Flow"
        <<std::setw(10)<<std::left<<"Source"
        <<std::setw(10)<<std::left<<"Destn"
        <<std::setw(15)<<std::left<<"Rate"
        <<std::setw(15)<<std::left<<"Period"
        <<std::setw(15)<<std::left<<"Deadline"
        <<std::setw(10)<<std::left<<"Priority"<<std::endl;
    for (auto F: generated_periodic_flows){
        F->displayFlowMinimal();
    }
}


