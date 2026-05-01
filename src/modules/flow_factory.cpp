#include <iostream>
#include <vector>
#include <tuple>
#include <string>
#include <random>
#include <iomanip>
#include <flow_factory.h>
#include <topo_factory.h>
#include <debug_config.h>

/* Class Definitions of Flow Class*/

// Flow Class constructor with node objects
Flow::Flow(uint id, std::string name, Node& src, Node& dst, double pkt_size, double rate, double prd, double rel_deadline, uint prio){
    flow_id = id;
    flow_name = name;
    source = &src;
    destination = &dst;
    packet_size = pkt_size;
    bandwidth = rate;
    period = prd;
    relative_deadline = rel_deadline;
    priority = prio;
}

// Flow Class constructor with node names, and additional fields of flow types and flow modes
Flow::Flow(uint id, std::string name, std::string src_name, std::string dst_name, 
    double pkt_size, double bw, double time_period, double rel_ddl, uint prio, flow_type fl_type,
    flow_safety_mode fl_mode){
    flow_id = id;
    flow_name = name;
    src = src_name;
    dst = dst_name;
    packet_size = pkt_size;
    bandwidth = bw;
    period = time_period;
    relative_deadline = rel_ddl;
    priority = prio;
    ft = fl_type;
    fm = fl_mode;
    expected_min_delay_rta = __DBL_MAX__;
    expected_min_delay_frc = __DBL_MAX__;
    actual_delay = 0;
    placed = false;
    //std::cout<<"Done initializing flow with "<<"ID = "<<id<<std::endl;
}

// Given a switch, it calculates the processing time for a flow in terms of *per-job* self processing time
double Flow::getJobSelfProcessingTime(Node *N){

    auto processing_time = 0.0;
    double bytes_per_period = bandwidth * period  ; /* bytes per period 'seconds' */
    double switch_rate = N->node_bw_capacity;
    processing_time = bytes_per_period/switch_rate;
    return processing_time;

}

//Calculates per link delay as per Response-Time Analysis
double Flow::perLinkDelay(Link *L){
    
    auto link_delay = 0.0;
    auto forwarding_delay = 0.0;
    auto propagation_delay = 0.0;

    if (true){
        propagation_delay = PROPAGATION_DELAY;
        
       if(DBG_LINK_DELAY)
            std::cout<<"Packet Size = "<<packet_size<<" "<<"Link Bandwidth Capacity = "<<L->link_bw_capacity<<std::endl;
        
        forwarding_delay = (bandwidth * period)/(L->link_bw_capacity); //alternate expression: packet_size/(L->link_bw_capacity);

        link_delay = propagation_delay + forwarding_delay;
        
        if(DBG_LINK_DELAY)
            std::cout<<"\t\t\t\tLink Weight "<<"("<<L->link_name<<") --> "<<propagation_delay<<" + "<<forwarding_delay<<" = "<<link_delay<<std::endl;
    }

    else
        link_delay = L->cost;

    return link_delay;
}

//Calculates total per node delay as per response-time analysis
double Flow::perNodeDelay(Node *N){
    
    auto interference_delay = 0.0;
    auto queueing_delay = 0.0;
    auto self_processing = 0.0;

    interference_delay = perNodeInterferenceDelay(N);

    queueing_delay = perNodeQueuingDelay(N);

    self_processing = getJobSelfProcessingTime(N);

    auto node_delay = interference_delay + queueing_delay + self_processing;

    if(DBG_NODE_DELAY)
        std::cout<<"\t\t\t\tNode Weight "<<"("<<N->node_name<<") --> "<<interference_delay<<"(Interference Delay)"
        <<"+"<<queueing_delay<<"(Queueing Delay)"
        <<"+"<<self_processing<<"(Self Processing)"
        <<"="<<node_delay<<"(Total Node Delay)"<<std::endl;
    return node_delay;
}

//Calculate per node interference delay as per response-time analysis
double Flow::perNodeInterferenceDelay(Node *N){

    auto interference_delay = 0.0;

    for (auto x: N->enterFlows){
        if (x->flow_id != flow_id){
            //Interference delay due to higher priority levels
            double processing_time =  (x->bandwidth * x->period)/N->node_bw_capacity;
            //lower number is higher priority
            if(priority > x->priority){
                //std::cout<<"Considering higher priority flow "<<x->flow_name<<std::endl;
                interference_delay = interference_delay + ceil(period/x->period) * processing_time;
            }
        }
    }
    return interference_delay;
}

//Calculate per node queuing delay as per response-time analysis
double Flow::perNodeQueuingDelay(Node *N){

    auto queuing_delay = 0.0;

    for (auto x: N->enterFlows){
        if (x->flow_id != flow_id){
            //Queueing delay due to same priority levels levels
            if (priority == x->priority)
                queuing_delay = queuing_delay + (x->bandwidth * x->period/N->node_bw_capacity) * ceil(period/x->period);
        }
    }
    return queuing_delay;
}

// Calculates the delay incurred by the flow along a given path as per Response-Time Analysis
double Flow::getPathDelay(Path* P){

    double path_delay = 0.0;
    double link_delay = 0.0;
    double node_delay = 0.0;

    for (auto x: P->links){
        double cur_link_delay = perLinkDelay(x);
        link_delay +=  cur_link_delay;
        if(DBG_RMA_DELAY)
            std::cout<<"\t\t\tLink ("<<x->link_name<<") = "<<format_time(cur_link_delay)<<std::endl;
    }

    if (!DBG_IGNORE_NODE_DELAY){
        //skip the destination at the end of the path
        for (size_t i = 0; i + 1 < P->nodes.size(); ++i){
            double cur_node_delay = perNodeDelay(P->nodes[i]);
            node_delay +=  cur_node_delay;
            if(DBG_RMA_DELAY)
                std::cout<<"\t\t\tNode ("<<(P->nodes[i])->node_name<<") = "<<format_time(cur_node_delay)<<std::endl; 
        }   
    }
    else
        node_delay = 0;

    path_delay += link_delay + node_delay;
    return path_delay;

}

// Show the flow parameters
void Flow::displayFlow(){
    std::cout<<"---- Flow Information ----"<<std::endl;
    std::cout << "\tFlow ID: "<<flow_id<<std::endl;
    std::cout << "\tSource: "<<src<<std::endl;
    std::cout << "\tDestn: "<<dst<<std::endl;
    std::cout << "\tPacket Size: "<<packet_size<<std::endl;
    std::cout << "\tRate: "<<bandwidth<<std::endl;
    std::cout << "\tPeriod: "<<period<<std::endl;
    std::cout << "\tRelative Deadline: "<<relative_deadline<<std::endl;
    std::cout << "\tPriority: "<<priority<<std::endl;
    std::cout<<std::endl<<std::endl<<std::endl;
}

// Show minimal flow parameters
void Flow::displayFlowMinimal(){
    std::cout<<std::setw(10)<<std::left<<flow_name
        <<std::setw(10)<<std::left<<src
        <<std::setw(10)<<std::left<<dst
        <<std::setw(15)<<std::left<<format_bandwidth(bandwidth)
        <<std::setw(15)<<std::left<<format_time(period)
        <<std::setw(15)<<std::left<<format_time(relative_deadline)
        <<std::setw(10)<<std::left<<priority<<std::endl;
}

// Given a switch, it calculates the processing time for a flow in terms of *per-job* self processing time
double AperiodicFlow::getJobSelfProcessingTime(Node *N){

    auto processing_time = 0.0;
    double bytes_per_period_min = bandwidth * period_min; /* bytes per period 'seconds' */
    double switch_rate = N->node_bw_capacity;
    processing_time = bytes_per_period_min/switch_rate;
    return processing_time;

}
