#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <util.h>
#include <netdesign_factory.h>
#include <topo_factory.h>
#include <flow_factory.h>
#include <flow_gen.h>
#include <debug_config.h>
#include <unordered_map>

#include <algorithm>
#include <mosek.h>
#include <iomanip>
#include <delay_model.h>
//for timestamp
#include <chrono>
#include <iomanip>
#include <sstream>

/* Header files to parse XML file */
#include <pugixml.hpp>
#include <pugiconfig.hpp>

#include "fusion.h"
using namespace mosek::fusion;
using namespace monty;



int generate_random_seeded(int x, int y)
{
    //std::random_device dev;
    static std::mt19937 rng_seed(1000);
    std::uniform_int_distribution<std::mt19937::result_type> dist_xy(x,y);

    return dist_xy(rng_seed);
}

int generate_random_int(int x, int y)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist_xy(x,y);

    return dist_xy(rng);
}

double generate_random_double(double x, double y)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<> dist_xy(x,y);

    return dist_xy(rng);
}

double generate_random_double_seeded(double x, double y)
{
    //std::random_device dev;
    std::mt19937 rng(55);
    std::uniform_real_distribution<> dist_xy(x,y);

    return dist_xy(rng);
}

int pick_rand_index_from_a_list(std::vector<double>& lst)
{
	int rand_index = generate_random_seeded(0, lst.size()-1);
	return rand_index;
}

int pick_rand_index_from_a_short_list(std::vector<short>& lst)
{
	int rand_index = generate_random_int(0, lst.size()-1);
	return rand_index;
}

std::vector<double> make_vector_with_step(double beg, double step, double end) 
{
    std::vector<double> vec;
    vec.reserve((end - beg) / step + 1);
    while (beg <= end) {
        vec.push_back(beg);
        beg += step;
    }
    return vec;
}

std::vector<short> make_short_vector_with_step(short beg, short step, short end) 
{
    std::vector<short> vec;
    uint x = (end - beg) / step + 1;
    // std::cout<<"Reserving "<<x<<std::endl;
    vec.reserve((end - beg) / step + 1);
    while (beg <= end) {
        vec.push_back(beg);
        beg += step;
    }
    return vec;
}

bool compareFlow(Flow *lhs, Flow *rhs){
    //Sort by priority if its given, if not then sort by deadline
    if (lhs->priority == rhs->priority)
        return (lhs->flow_id < rhs->flow_id);
    else
        return (lhs->priority < rhs->priority);
}

uint get_candidate_path_dict_size(std::unordered_map<Flow, std::vector<Link> > candidate_path_dict){
    return candidate_path_dict.size();
}

//returns the euclidean distance
double compute_eta(std::vector<AperiodicFlow*> event_driven_flows, 
    std::vector<double> Tstar){
    double Nr, Dr;
    double euclidean_distance = 0.0;
    for (int i = 0; i != Tstar.size(); i++){
        Nr = (Tstar[i] - event_driven_flows[i]->period_min)*
			(Tstar[i] - event_driven_flows[i]->period_min);
        Dr = (event_driven_flows[i]->period_max - event_driven_flows[i]->period_min)*
			(event_driven_flows[i]->period_max - event_driven_flows[i]->period_min);
        euclidean_distance = Nr/Dr;
    }

    return euclidean_distance;
}

/* Converts a string like 5 Gbps into 5X10^9 bps*/
double parse(std::string capacity){

    double result; 
    double multiplier;
    std::stringstream ss;

    int pos;
    std::string::iterator it;
    std::string result_str;

    if (capacity.find("Gbps") != std::string::npos){
        multiplier = 1e9;
        pos = capacity.find("Gbps");
        it = capacity.begin() + pos - 1;
        result_str =  std::string(capacity.begin(), it);
        ss << result_str;
        ss >> result;
        result = result * multiplier;
    }

    else if (capacity.find("Mbps") != std::string::npos){
        multiplier = 1e6;
        pos = capacity.find("Mbps");
        it = capacity.begin() + pos - 1;
        result_str =  std::string(capacity.begin(), it);
        ss << result_str;
        ss >> result;
        result = result * multiplier;
    }

    else if (capacity.find("Kbps") != std::string::npos){
        multiplier = 1e3;
        pos = capacity.find("Kbps");
        it = capacity.begin() + pos - 1;
        result_str =  std::string(capacity.begin(), it);
        ss << result_str;
        ss >> result;
        result = result * multiplier;
    }

    else
        result = -1.0;

    return result;
}

/* Formatting time for easy readability */
std::string format_time(double delay) {

    if (delay == __DBL_MAX__) {
        return "UNREACHABLE";
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3); // Set precision for consistent formatting

    if (delay >= 1){
        oss << delay << "s";
    }
    else if (delay >= 1e-3) {
        oss << delay / 1e-3 << "ms";
    }
    else if (delay >= 1e-6) {
        oss << delay / 1e-6 << "us";
    }
    else if (delay >= 1e-9) {
        oss << delay / 1e-9 << "ns";
    }
    else {
        oss << delay << "s";  // fallback if < 1 ns
    }

    return oss.str();
}

/* Formatting bandwidth for easy readability */
std::string format_bandwidth(double bandwidth) {

    if (bandwidth == 0) {
        return "NO CAPACITY";
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3); // Set precision for consistent formatting

    if (bandwidth >= 1e9){
        oss << bandwidth / 1e9 << "Gbps";
    }
    else if (bandwidth >= 1e6) {
        oss << bandwidth / 1e6 << "Mbps";
    }
    else if (bandwidth >= 1e3) {
        oss << bandwidth / 1e3 << "Kbps";
    }
    else {
        oss << bandwidth << "bps";  // fallback if < 1 ns
    }

    return oss.str();
}

/* Converts a string like 5 ms into 5X10^-3 s*/
double parse_time(std::string time_val){

    double result; 
    double multiplier;
    std::stringstream ss;

    int pos;
    std::string::iterator it;
    std::string result_str;

    if (time_val.find("ms") != std::string::npos){
        multiplier = 1e-3;
        pos = time_val.find("ms");
        it = time_val.begin() + pos - 1;
        result_str =  std::string(time_val.begin(), it);
        ss << result_str;
        ss >> result;
        result = result * multiplier;
    }

    else if (time_val.find("us") != std::string::npos){
        multiplier = 1e-6;
        pos = time_val.find("us");
        it = time_val.begin() + pos - 1;
        result_str =  std::string(time_val.begin(), it);
        ss << result_str;
        ss >> result;
        result = result * multiplier;
    }

    else if (time_val.find("ns") != std::string::npos){
        multiplier = 1e-9;
        pos = time_val.find("ns");
        it = time_val.begin() + pos - 1;
        result_str =  std::string(time_val.begin(), it);
        ss << result_str;
        ss >> result;
        result = result * multiplier;
    }

    else{
        multiplier = 1;
        pos = time_val.find("s");
        it = time_val.begin() + pos - 1;
        result_str =  std::string(time_val.begin(), it);
        ss << result_str;
        ss >> result;
        result = result * multiplier;
    }
    
    return result;
}


void capture_all_interdependencies(std::vector<Node*>& node_spec,
    std::vector<Link*>& link_spec, std::vector<Flow*>& flow_spec,
    std::vector<AperiodicFlow*>& aflow_spec){

    //Capture Node references in Flow
    if (DBG_UTIL)
        std::cout<<"Capturing Node references in Periodic Flow"<<std::endl;
    for (auto f: flow_spec){
        for (auto n: node_spec){
            if (DBG_UTIL)
                std::cout<<"Flow ID = "<<f->flow_name<<" "<<"Source = "<<f->src<<" "<<"Destination = "<<f->dst<<std::endl;
            if (f->src == n->node_name)
                f->source = n;
            if (f->dst == n->node_name)
                f->destination = n;
        }
        
    }

    //Capture Node references in AperiodicFlow
    if (DBG_UTIL)
        std::cout<<"Capturing Node references in Aperiodic Flow"<<std::endl;
    for (auto af: aflow_spec){
        for (auto n: node_spec){
            if (DBG_UTIL)
                std::cout<<"AFlow ID = "<<af->flow_name<<" "<<"Source = "<<af->src<<" "<<"Destination = "<<af->dst<<std::endl;
            if (af->src == n->node_name)
                af->source = n;
            if (af->dst == n->node_name)
                af->destination = n;
        }
        
    }

    //Capture Link references in Node
    //Capture Node references in Link
    if (DBG_UTIL)
        std::cout<<"==== Capturing Link and Node Interdependencies ===="<<std::endl;
    for (auto l:link_spec){
        for (auto n: node_spec){
            if (l->src == n->node_name){
                if (DBG_UTIL)
                    std::cout<<"---- Node name "<<n<<" match with source of "<<l<<std::endl;
                l->node1 = n;
                if (std::find(n->links.begin(), n->links.end(), l) == n->links.end()){
                    if (DBG_UTIL)
                        std::cout<<"---- Capturing "<<l->link_name<<" "<<"to"<<" "<<n->node_name<<std::endl;
                    n->links.push_back(l);
                }    
            }
            if (l->dst == n->node_name){
                if (DBG_UTIL)
                    std::cout<<"---- Node name "<<n<<" match with destination of "<<l<<std::endl;
                l->node2 = n;
                if (std::find(n->links.begin(), n->links.end(), l) == n->links.end()){
                    if (DBG_UTIL)
                        std::cout<<"---- Capturing "<<l->link_name<<" "<<"to"<<" "<<n->node_name<<std::endl;
                    n->links.push_back(l);
                }
                    
            }
        }
        if (DBG_UTIL)
            std::cout<<"---- Link ID = "<<l->link_name<<" "<<"Source = "<<l->node1<<" "<<"Destination = "<<l->node2<<std::endl;
    }
    if (DBG_UTIL)
        std::cout<<"==== Link and Node Interdependencies Captured ===="<<std::endl<<std::endl;

    //Debug prints to check if Link references have been captured in Node
    if(DBG_NW_CAPTURE_SPECS){
        for (auto n:node_spec){
            std::cout<<"Node ID = "<<n->node_name<<" "<<"Links connected = ";
            for (auto l : n->links)
                std::cout<<l->link_name<<" ";

            std::cout<<std::endl;
        }
    }
    
}

void capture_node_link_interdependencies(std::vector<Node*>& node_spec,
    std::vector<Link*>& link_spec){

    //Capture Link references in Node
    //Capture Node references in Link
    //std::cout<<"==== CAPTURING INTERDEPENDENCIES ===="<<std::endl;
    for (auto l:link_spec){
        for (auto n: node_spec){
            if (l->src == n->node_name){
                if (DBG_NW_CAPTURE_SPECS)
                    std::cout<<"---- Node name "<<n<<" match with source of "<<l<<std::endl;
                l->node1 = n;
                if (std::find(n->links.begin(), n->links.end(), l) == n->links.end()){
                    if (DBG_NW_CAPTURE_SPECS)
                        std::cout<<"---- Capturing "<<l->link_name<<" "<<"to"<<" "<<n->node_name<<std::endl;
                    n->links.push_back(l);
                }    
            }
            if (l->dst == n->node_name){
                if (DBG_NW_CAPTURE_SPECS)
                    std::cout<<"---- Node name "<<n<<" match with destination of "<<l<<std::endl;
                l->node2 = n;
                if (std::find(n->links.begin(), n->links.end(), l) == n->links.end()){
                    if (DBG_NW_CAPTURE_SPECS)
                        std::cout<<"---- Capturing "<<l->link_name<<" "<<"to"<<" "<<n->node_name<<std::endl;
                    n->links.push_back(l);
                }
                    
            }
        }
        if (DBG_NW_CAPTURE_SPECS)
            std::cout<<"---- Link ID = "<<l->link_name<<" "<<"Source = "<<l->node1<<" "<<"Destination = "<<l->node2<<std::endl;
    }
    //std::cout<<"==== INTERDEPENDENCIES CAPTURED ===="<<std::endl<<std::endl;

}

void addNode(Node *x, Path* P){ 
    P->nodes.push_back(x);
    //return Path(nodes, links);
}

void addLink(Link *y, Path* P){ 
    P->links.push_back(y);
    //return Path(nodes, links);
}

/*
* Parsing given XML file to node_specs and link_specs
*/
void parseNetworkSpecifications(const char* network_file_path, std::vector<Node*>& node_specs, 
        std::vector<Link*>& link_specs){
    
    uint num_nodes, num_links;

    pugi::xml_document rt_network_design;
    pugi::xml_parse_result analyzed_rt_network_design = rt_network_design.load_file(network_file_path);

    if (!rt_network_design){
        std::cout<<"Reading Network XML Failure\n";
    }

    for (pugi::xml_node parent: rt_network_design.child("Profile").children("elements")){
        
        //Populate list of switches
        if (DBG_UTIL)
            std::cout<<"Populating switches"<<std::endl;
        for (pugi::xml_node sw: parent.children("switch")){
            uint sw_id = sw.attribute("id").as_uint();
            std::string sw_name = sw.attribute("name").as_string();
            std::string sw_bw_cap_raw = sw.attribute("switch-bw-capacity").as_string();
            double sw_bw_cap_bps = parse(sw_bw_cap_raw);
            Node* N_ptr = new Node(sw_id, sw_name, sw_bw_cap_bps);
            if (DBG_UTIL){
                std::cout<<"Node = "<<sw_name<<" Address = "<<N_ptr;
                std::cout<< " Switch Bandwidth capacity = "<<sw_bw_cap_bps<<std::endl;
            }
            node_specs.push_back(N_ptr);
        }

        //Populate list of links
        if (DBG_UTIL)
            std::cout<<"Populating links"<<std::endl;
        for (pugi::xml_node link: parent.children("link")){
            uint link_id = link.attribute("id").as_uint();
            std::string link_name = link.attribute("name").as_string();
            std::string link_bw_cap_raw = link.attribute("link-bw-capacity").as_string();
            double link_bw_cap_bps = parse(link_bw_cap_raw);
            std::string link_source = link.attribute("src").as_string();
            std::string link_destination = link.attribute("dst").as_string();
            if (link.attribute("cost")){
                float link_cost = link.attribute("cost").as_float();
                Link* L_ptr = new Link(link_id, link_name, link_bw_cap_bps, link_source, link_destination, link_cost);
                link_specs.push_back(L_ptr);
            }
            else{
                Link* L_ptr = new Link(link_id, link_name, link_bw_cap_bps, link_source, link_destination);
                if (DBG_UTIL){
                    std::cout<<"Link = "<<link_name<<" Address = "<<L_ptr;
                    std::cout<<" Link Bandwidth capacity = "<<link_bw_cap_bps<<std::endl;
                }
                link_specs.push_back(L_ptr);
            }
        }
       
        /*  AperiodicServer SS(0.0,0.0);

            N.init(sw_bw_cap_bps, links, enterFlows, exitFlows, &SS);
            node_specifications.push_back(&N);
        */
        num_nodes = node_specs.size();
        num_links = link_specs.size();

    }
    if (DBG_UTIL){
        std::cout<<"Finished reading the topology\nNo. of Switches = "<<num_nodes<<"\t";
        std::cout<<"No. of Links = "<<num_links<<"\t\n";
    }

    if (DBG_UTIL){
        std::cout<<"**********************************************"<<std::endl;
        std::cout<<"Nodes: "<<std::endl;
        for (auto N: node_specs)
            std::cout<<N<<" ";
        std::cout<<std::endl;
        std::cout<<"**********************************************"<<std::endl;
        
        std::cout<<"Links: "<<std::endl;
        for (auto L: link_specs)
            std::cout<<L<<" ";
        std::cout<<std::endl;
        std::cout<<"**********************************************"<<std::endl;
    }

}

/*
* Parsing given XML file to flow_specs and aflow_specs
*/
void parseFlowSpecifications(const char* flow_file_path, std::vector<Flow*>& flow_specs, std::vector<AperiodicFlow*>& aflow_specs){

    pugi::xml_document flow_spec;
    pugi::xml_parse_result analyzed_flow_spec = flow_spec.load_file(flow_file_path);

    if (!analyzed_flow_spec){
        std::cout<<"Reading Flow XML Failure\n";
    }

    for (pugi::xml_node parent: flow_spec.child("Profile").children("elements")){

        //Populate list of periodic flows
        std::cout<<"Populating Periodic flows"<<std::endl;
        for (pugi::xml_node flow: parent.children("flow")){
            
            uint flow_id = flow.attribute("id").as_uint();
            std::string flow_name = flow.attribute("name").as_string();

            std::string flow_source = flow.attribute("source").as_string();
            std::string flow_destination = flow.attribute("destination").as_string();
            double packet_size_bytes = flow.attribute("packet-size-bytes").as_double();
            std::string flow_bw_cap_raw = flow.attribute("bandwidth").as_string();
            double flow_bw_cap_bps = parse(flow_bw_cap_raw);
            //std::cout << "Flow Bandwidth requirements = "<<flow_bw_cap_bps<<std::endl;

            std::string flow_period_raw = flow.attribute("period").as_string();
            double flow_period = parse_time(flow_period_raw);
            //std::cout << "Flow Period = "<<flow_period<<std::endl;

            std::string flow_deadline_raw = flow.attribute("deadline").as_string();
            double flow_deadline = parse_time(flow_deadline_raw);
            //std::cout << "Flow Deadline = "<<flow_deadline<<std::endl;

            uint flow_prio = flow.attribute("priority").as_uint();
            flow_type ft = static_cast<flow_type> (flow.attribute("flow-type").as_uint());
            flow_safety_mode fsm = static_cast<flow_safety_mode> (flow.attribute("flow-safety-mode").as_uint());
            
            Flow* F_ptr = new Flow(flow_id, flow_name, flow_source, flow_destination,
                packet_size_bytes, flow_bw_cap_bps, flow_period, flow_deadline, flow_prio,
                ft, fsm);
            
            //std::cout<<"Flow = "<<flow_name<<" Address = "<<F_ptr<<std::endl;
            flow_specs.push_back(F_ptr);
        }

        //Populate list of aperiodic flows
        std::cout<<"Populating Aperiodic flows"<<std::endl;
        for (pugi::xml_node flow: parent.children("aflow")){
            
            uint flow_id = flow.attribute("id").as_uint();
            std::string flow_name = flow.attribute("name").as_string();

            std::string flow_source = flow.attribute("source").as_string();
            std::string flow_destination = flow.attribute("destination").as_string();
            double packet_size_bytes = flow.attribute("packet-size-bytes").as_double();
            std::string flow_bw_cap_raw = flow.attribute("bandwidth").as_string();
            double flow_bw_cap_bps = parse(flow_bw_cap_raw);
            //std::cout << "Flow Bandwidth requirements = "<<flow_bw_cap_bps<<std::endl;

            std::string flow_period_raw = flow.attribute("period").as_string();
            double flow_period = parse_time(flow_period_raw);
            //std::cout << "Flow Period = "<<flow_period<<std::endl;

            std::string flow_period_min_raw = flow.attribute("period-des").as_string();
            double flow_period_min = parse_time(flow_period_min_raw);

            std::string flow_period_max_raw = flow.attribute("period-max").as_string();
            double flow_period_max = parse_time(flow_period_max_raw);

            std::string flow_deadline_raw = flow.attribute("deadline").as_string();
            double flow_deadline = parse_time(flow_deadline_raw);
            //std::cout << "Flow Deadline = "<<flow_deadline<<std::endl;

            uint flow_prio = flow.attribute("priority").as_uint();
            flow_type ft = static_cast<flow_type> (flow.attribute("flow-type").as_uint());
            flow_safety_mode fsm = static_cast<flow_safety_mode> (flow.attribute("flow-safety-mode").as_uint());
            
            AperiodicFlow* aF_ptr = new AperiodicFlow(flow_id, flow_name, flow_source, flow_destination,
                packet_size_bytes, flow_bw_cap_bps, flow_period, flow_deadline, flow_prio,
                ft, fsm, flow_period_min, flow_period_max);
            
            //std::cout<<"Flow = "<<flow_name<<" Address = "<<F_ptr<<std::endl;
            aflow_specs.push_back(aF_ptr);
        }
    }
}

/*
* Parsing given XML file to node_specs, link_specs, flow_specs and aflow_specs
*/
void parseNetworkFlowSpecifications(const char* network_flow_file_path, std::vector<Node*>& node_specs, 
        std::vector<Link*>& link_specs, std::vector<Flow*>& flow_specs, std::vector<AperiodicFlow*>& aflow_specs){
    
    uint num_nodes, num_links, num_periodic_flows, num_aperiodic_flows;

    pugi::xml_document rt_network_design;
    pugi::xml_parse_result analyzed_rt_network_design = rt_network_design.load_file(network_flow_file_path);

    if (!rt_network_design){
        std::cout<<"Reading Network XML Failure\n";
    }

    for (pugi::xml_node parent: rt_network_design.child("Profile").children("elements")){
        
        //Populate list of switches
        if (DBG_UTIL)
            std::cout<<"Populating switches"<<std::endl;
        for (pugi::xml_node sw: parent.children("switch")){
            uint sw_id = sw.attribute("id").as_uint();
            std::string sw_name = sw.attribute("name").as_string();
            std::string sw_bw_cap_raw = sw.attribute("switch-bw-capacity").as_string();
            double sw_bw_cap_bps = parse(sw_bw_cap_raw);
            Node* N_ptr = new Node(sw_id, sw_name, sw_bw_cap_bps);
            if (DBG_UTIL){
                std::cout<<"Node = "<<sw_name<<" Address = "<<N_ptr;
                std::cout<< " Switch Bandwidth capacity = "<<sw_bw_cap_bps<<std::endl;
            }
            node_specs.push_back(N_ptr);
        }

        //Populate list of links
        if (DBG_UTIL)
            std::cout<<"Populating links"<<std::endl;
        for (pugi::xml_node link: parent.children("link")){
            uint link_id = link.attribute("id").as_uint();
            std::string link_name = link.attribute("name").as_string();
            std::string link_bw_cap_raw = link.attribute("link-bw-capacity").as_string();
            double link_bw_cap_bps = parse(link_bw_cap_raw);
            std::string link_source = link.attribute("src").as_string();
            std::string link_destination = link.attribute("dst").as_string();
            if (link.attribute("cost")){
                float link_cost = link.attribute("cost").as_float();
                Link* L_ptr = new Link(link_id, link_name, link_bw_cap_bps, link_source, link_destination, link_cost);
                link_specs.push_back(L_ptr);
            }
            else{
                Link* L_ptr = new Link(link_id, link_name, link_bw_cap_bps, link_source, link_destination);
                if (DBG_UTIL){
                    std::cout<<"Link = "<<link_name<<" Address = "<<L_ptr;
                    std::cout<<" Link Bandwidth capacity = "<<link_bw_cap_bps<<std::endl;
                }
                link_specs.push_back(L_ptr);
            }
        }

        //Populate list of periodic flows
        if (DBG_UTIL)
            std::cout<<"Populating Periodic flows"<<std::endl;
        for (pugi::xml_node flow: parent.children("flow")){
            
            uint flow_id = flow.attribute("id").as_uint();
            std::string flow_name = flow.attribute("name").as_string();

            std::string flow_source = flow.attribute("source").as_string();
            std::string flow_destination = flow.attribute("destination").as_string();
            double packet_size_bytes = flow.attribute("packet-size-bytes").as_double();
            std::string flow_bw_cap_raw = flow.attribute("bandwidth").as_string();
            double flow_bw_cap_bps = parse(flow_bw_cap_raw);
            std::string flow_period_raw = flow.attribute("period").as_string();
            double flow_period = parse_time(flow_period_raw);
            std::string flow_deadline_raw = flow.attribute("deadline").as_string();
            double flow_deadline = parse_time(flow_deadline_raw);
            uint flow_prio = flow.attribute("priority").as_uint();
            flow_type ft = static_cast<flow_type> (flow.attribute("flow-type").as_uint());
            flow_safety_mode fsm = static_cast<flow_safety_mode> (flow.attribute("flow-safety-mode").as_uint());
            
            Flow* F_ptr = new Flow(flow_id, flow_name, flow_source, flow_destination,
                packet_size_bytes, flow_bw_cap_bps, flow_period, flow_deadline, flow_prio,
                ft, fsm);
            if (DBG_UTIL){
                std::cout<<"Flow = "<<flow_name<<" Address = "<<F_ptr;
                std::cout<<" Flow Bandwidth requirements = "<<flow_bw_cap_bps;
                std::cout<<" Flow Period = "<<flow_period;
                std::cout<<" Flow Deadline = "<<flow_deadline<<std::endl;
            }
            flow_specs.push_back(F_ptr);
        }

        //Populate list of aperiodic flows
        if (DBG_UTIL)
            std::cout<<"Populating Aperiodic flows"<<std::endl;
        for (pugi::xml_node flow: parent.children("aflow")){
            
            uint flow_id = flow.attribute("id").as_uint();
            std::string flow_name = flow.attribute("name").as_string();

            std::string flow_source = flow.attribute("source").as_string();
            std::string flow_destination = flow.attribute("destination").as_string();
            double packet_size_bytes = flow.attribute("packet-size-bytes").as_double();
            std::string flow_bw_cap_raw = flow.attribute("bandwidth").as_string();
            double flow_bw_cap_bps = parse(flow_bw_cap_raw);

            std::string flow_period_raw = flow.attribute("period").as_string();
            double flow_period = parse_time(flow_period_raw);

            std::string flow_period_min_raw = flow.attribute("period-des").as_string();
            double flow_period_min = parse_time(flow_period_min_raw);

            std::string flow_period_max_raw = flow.attribute("period-max").as_string();
            double flow_period_max = parse_time(flow_period_max_raw);

            std::string flow_deadline_raw = flow.attribute("deadline").as_string();
            double flow_deadline = parse_time(flow_deadline_raw);

            uint flow_prio = flow.attribute("priority").as_uint();
            flow_type ft = static_cast<flow_type> (flow.attribute("flow-type").as_uint());
            flow_safety_mode fsm = static_cast<flow_safety_mode> (flow.attribute("flow-safety-mode").as_uint());
            
            AperiodicFlow* aF_ptr = new AperiodicFlow(flow_id, flow_name, flow_source, flow_destination,
                packet_size_bytes, flow_bw_cap_bps, flow_period, flow_deadline, flow_prio,
                ft, fsm, flow_period_min, flow_period_max);
            
            if (DBG_UTIL){
                std::cout<<"Aperiodic Flow = "<<flow_name<<" Address = "<<aF_ptr;
                std::cout<<" Flow Bandwidth requirements = "<<flow_bw_cap_bps;
                std::cout<<" Flow Period Min = "<<flow_period_min<<" Flow Period Max = "<<flow_period_max;
                std::cout<<" Flow Deadline = "<<flow_deadline<<std::endl;
            }
            aflow_specs.push_back(aF_ptr);
        }
        
        /*  AperiodicServer SS(0.0,0.0);

            N.init(sw_bw_cap_bps, links, enterFlows, exitFlows, &SS);
            node_specifications.push_back(&N);
        */
        num_nodes = node_specs.size();
        num_links = link_specs.size();
        num_periodic_flows = flow_specs.size();
        num_aperiodic_flows = aflow_specs.size();

    }

    std::cout<<"Finished reading the topology\nNo. of Switches="<<num_nodes<<"\t";
    std::cout<<"No. of Links="<<num_links<<"\t";
    std::cout<<"No. of Periodic Flows="<<num_periodic_flows<<"\t";
    std::cout<<"No. of Aperiodic Flows="<<num_aperiodic_flows<<std::endl<<std::endl;

    if (DBG_UTIL){
        std::cout<<"**********************************************"<<std::endl;
        std::cout<<"Nodes: "<<std::endl;
        for (auto N: node_specs)
            std::cout<<N<<" ";
        std::cout<<std::endl;
        std::cout<<"**********************************************"<<std::endl;
        
        std::cout<<"Links: "<<std::endl;
        for (auto L: link_specs)
            std::cout<<L<<" ";
        std::cout<<std::endl;
        std::cout<<"**********************************************"<<std::endl;
    }
    
    /*Analyze inter-dependencies between nodes, links and flows to capture inter-dependent specs
    * fill in data structures
    */
    capture_all_interdependencies(node_specs, link_specs, flow_specs, aflow_specs);

}

bool isNumber(const char number[])
{
    int i = 0;
    
    //checking for negative numbers
    if (number[0] == '-')
        i = 1;
    for (; number[i] != 0; i++)
    {
        if (!isdigit(number[i]))
            return false;
    }
    return true;
}

/*
* Design when both the topology and flow specifications in a single XML file
*/
std::pair<double, double> design_given_topo_and_flow_specs(const char* network_flow_file_path, std::string output_dirname_fullpath, latency_scheme ds){

    uint num_nodes, num_links, num_flows, aperiodic_num_flows;

    std::pair<double, double> acceptance_ratio;

    std::vector<Node*> node_specifications;
    std::vector<Link*> link_specifications;
    std::vector<Flow*> flow_specifications;
    std::vector<AperiodicFlow*> aflow_specifications;

    parseNetworkFlowSpecifications(network_flow_file_path, node_specifications, link_specifications, flow_specifications, aflow_specifications);

    //Sort the flows
    std::sort(flow_specifications.begin(), flow_specifications.end(), compareFlow);

    //Debug print to view the sorted order
    if (DBG_SORTED_FLOWS){
        std::cout<<"Sorted Flows: "<<std::endl;
        for (auto F: flow_specifications)
            std::cout<<F<<" "<<F->flow_name<<"(Priority="<<F->priority<<","<<"Deadline="<<F->relative_deadline<<")"<<std::endl;
        std::cout<<std::endl<<std::endl;  
    }

    num_nodes = node_specifications.size();
    num_links = link_specifications.size();
    num_flows = flow_specifications.size();
    aperiodic_num_flows = aflow_specifications.size();

    // std::cout<<num_nodes<<"\t"<<num_links<<"\t"<<num_flows<<"\t"<<aperiodic_num_flows<<std::endl;

    //Create an instance of the RT Network Design Problem
    RTNetwork* rt_network_design = new RTNetwork(num_nodes, num_links, node_specifications, link_specifications, flow_specifications, aflow_specifications, ds);

    if (DBG_MAIN)
        rt_network_design->printNetwork();
    
    if (DBG_MAIN){
        std::cout<<"\n\n=================================================================="<<std::endl;
        std::cout<<"********************PLACING PERIODIC FLOWS********************"<<std::endl;
        std::cout<<"=================================================================="<<std::endl;
    }

    rt_network_design->place_periodic_flows();

    if (DBG_MAIN){
        std::cout<<"\n=================================================================="<<std::endl;
        std::cout<<"********************END OF PLACING PERIODIC FLOWS********************"<<std::endl;
        std::cout<<"=================================================================="<<std::endl;
    }

    if (DBG_MAIN){
        std::cout<<"\n\n=================================================================="<<std::endl;
        std::cout<<"********************PLACING APERIODIC FLOWS********************"<<std::endl;
        std::cout<<"=================================================================="<<std::endl;
    }
    
    rt_network_design->place_aperiodic_flows();

    if (DBG_MAIN){
        std::cout<<"\n=================================================================="<<std::endl;
        std::cout<<"********************END OF PLACING APERIODIC FLOWS********************"<<std::endl;
        std::cout<<"=================================================================="<<std::endl; 
    } 

    std::string latency_result_file = "latency_results_" + std::to_string(num_nodes) + "_switches_" + std::to_string(num_links) + "_links_" + std::to_string(flow_specifications.size()) + "_periodic_" + std::to_string(aflow_specifications.size()) + "_aperiodic.csv";
    std::string acceptance_ratio_result_file = "acceptance_ratio_results_" + std::to_string(num_nodes) + "_switches_" + std::to_string(num_links) + "_links_" + std::to_string(flow_specifications.size()) + "_periodic_" + std::to_string(aflow_specifications.size()) + "_aperiodic.csv";
    acceptance_ratio = rt_network_design->post_placement_performance_analysis_csv(output_dirname_fullpath, latency_result_file, acceptance_ratio_result_file);
    return acceptance_ratio;
}

/*
* Design given the topology specifications and a single, random flow specification within DEADLINE_MIN_SEC and DEADLINE_MAX_SEC and return acceptance ratio
*/
std::pair<double, double> design_given_topo_specs(const char* network_file_path, int num_flows, std::string output_dirname_fullpath, latency_scheme ds){

    uint num_nodes, num_links;

    std::pair<double, double> acceptance_ratio;

    std::vector<Node*> node_specifications;
    std::vector<Link*> link_specifications;
    std::vector<Flow*> flow_specifications;
    std::vector<AperiodicFlow*> aflow_specifications;
    FlowGenerator* fg;

    // Parse the Network XML file
    parseNetworkSpecifications(network_file_path, node_specifications, link_specifications);
    num_nodes = node_specifications.size();
    num_links = link_specifications.size();

    // Generate Flow Specifications
    fg = new FlowGenerator(num_flows);
    fg->generate_periodic_flows_across_deadline_range_use_node_names(DEADLINE_MIN_SEC, DEADLINE_MAX_SEC, num_nodes);
    
    //Sort the flows
    std::sort(fg->generated_periodic_flows.begin(), fg->generated_periodic_flows.end(), compareFlow);
    
    
    
    if (DBG_MAIN)
        fg->printGeneratedFlows();

    //This fills in cross references between the node, link and flow structs
    capture_all_interdependencies(node_specifications, link_specifications, fg->generated_periodic_flows, fg->generated_aperiodic_flows);

    //Debug print to view the sorted order
    if (DBG_SORTED_FLOWS){
        std::cout<<"Sorted Flows: "<<std::endl;
        for (auto F: fg->generated_periodic_flows)
            std::cout<<F<<" "<<F->flow_name<<"(Priority="<<F->priority<<","<<"Deadline="<<F->relative_deadline<<")"<<std::endl;
        std::cout<<std::endl<<std::endl;  
    }

    //Create an instance of the RT Network Design Problem
    RTNetwork* rt_network_design = new RTNetwork(num_nodes, num_links, node_specifications, link_specifications, fg->generated_periodic_flows, fg->generated_aperiodic_flows, ds);

    if (DBG_MAIN)
        rt_network_design->printNetwork();
    
    if (DBG_MAIN){
        std::cout<<"\n\n=================================================================="<<std::endl;
        std::cout<<"********************PLACING PERIODIC FLOWS********************"<<std::endl;
        std::cout<<"=================================================================="<<std::endl;
    }

    rt_network_design->place_periodic_flows();

    if (DBG_MAIN){
        std::cout<<"\n=================================================================="<<std::endl;
        std::cout<<"********************END OF PLACING PERIODIC FLOWS********************"<<std::endl;
        std::cout<<"=================================================================="<<std::endl;
    }

    if (DBG_MAIN){
        std::cout<<"\n\n=================================================================="<<std::endl;
        std::cout<<"********************PLACING APERIODIC FLOWS********************"<<std::endl;
        std::cout<<"=================================================================="<<std::endl;
    }
    
    rt_network_design->place_aperiodic_flows();

    if (DBG_MAIN){
        std::cout<<"\n=================================================================="<<std::endl;
        std::cout<<"********************END OF PLACING APERIODIC FLOWS********************"<<std::endl;
        std::cout<<"=================================================================="<<std::endl; 
    } 

    std::string latency_result_file = "latency_results_" + std::to_string(num_nodes) + "_switches_" + std::to_string(num_links) + "_links_" + std::to_string(fg->generated_periodic_flows.size()) + "_periodic_" + std::to_string(fg->generated_aperiodic_flows.size()) + "_aperiodic.csv";
    std::string acceptance_ratio_result_file = "acceptance_ratio_results_" + std::to_string(num_nodes) + "_switches_" + std::to_string(num_links) + "_links_" + std::to_string(fg->generated_periodic_flows.size()) + "_periodic_" + std::to_string(fg->generated_aperiodic_flows.size()) + "_aperiodic.csv";
    acceptance_ratio = rt_network_design->post_placement_performance_analysis_csv(output_dirname_fullpath, latency_result_file, acceptance_ratio_result_file);
    return acceptance_ratio;
}

/*
* Design given the topology specifications and across multiple flow specifications across a deadline range and return a list of acceptance ratios
* Used to reproduce experiment#1, where deadline is varied got a given topology spec or a set of topology specifications
*/
std::map<std::pair<double, int>, std::vector<std::pair<double, double>>>& exp1_design_given_topo_specs_deadline_range(const char* network_file_path, double deadline_start, double deadline_inc, double deadline_finish, int num_flows, std::string output_dirname_fullpath, latency_scheme ds){

    uint num_nodes, num_links;
    double cur_deadline;

    std::pair<double, double> acceptance_ratio;
    static std::map<std::pair<double, int>, std::vector<std::pair<double, double>>> acceptance_ratios_map = {};
    std::pair<double, int> input_metric;

    std::vector<Node*> node_specifications;
    std::vector<Link*> link_specifications;
    std::vector<Flow*> flow_specifications;
    std::vector<AperiodicFlow*> aflow_specifications;
    FlowGenerator* fg;
    RTNetwork* rt_network_design;

    // Parse the Network XML file
    parseNetworkSpecifications(network_file_path, node_specifications, link_specifications);

    num_nodes = node_specifications.size();
    num_links = link_specifications.size();

    // Generate Flow Specifications
    for (cur_deadline = deadline_start; cur_deadline <= deadline_finish; cur_deadline += deadline_inc){

        fg = new FlowGenerator(num_flows);

        fg->generate_periodic_flows_specific_deadline_use_node_names(cur_deadline, num_nodes);
        //Sort the flows
        std::sort(fg->generated_periodic_flows.begin(), fg->generated_periodic_flows.end(), compareFlow);

        if (DBG_MAIN)
            fg->printGeneratedFlows();

        //This fills in cross references between the node, link and flow structs
        capture_all_interdependencies(node_specifications, link_specifications, fg->generated_periodic_flows, fg->generated_aperiodic_flows);

        //Debug print to view the sorted order
        if (DBG_SORTED_FLOWS){
            std::cout<<"Sorted Flows: "<<std::endl;
            for (auto F: fg->generated_periodic_flows)
                std::cout<<F<<" "<<F->flow_name<<"(Priority="<<F->priority<<","<<"Deadline="<<F->relative_deadline<<")"<<std::endl;
            std::cout<<std::endl<<std::endl;  
        }

        //Create an instance of the RT Network Design Problem
        rt_network_design = new RTNetwork(num_nodes, num_links, node_specifications, link_specifications, fg->generated_periodic_flows, fg->generated_aperiodic_flows, ds);

        if (DBG_MAIN)
            rt_network_design->printNetwork();
        
        std::string latency_result_file = "latency_results_" + std::to_string(num_nodes) + "_switches_" + std::to_string(num_links) + "_links_" + std::to_string(fg->generated_periodic_flows.size()) + "_periodic_" + std::to_string(fg->generated_aperiodic_flows.size()) + "_aperiodic.csv";
        std::string acceptance_ratio_result_file = "acceptance_ratio_results_" + std::to_string(num_nodes) + "_switches_" + std::to_string(num_links) + "_links_" + std::to_string(fg->generated_periodic_flows.size()) + "_periodic_" + std::to_string(fg->generated_aperiodic_flows.size()) + "_aperiodic.csv";
        input_metric = std::make_pair(cur_deadline, num_flows);

        if (DBG_MAIN){
            std::cout<<"\n\n=================================================================="<<std::endl;
            std::cout<<"********************PLACING PERIODIC FLOWS********************"<<std::endl;
            std::cout<<"=================================================================="<<std::endl;
        }

        rt_network_design->place_periodic_flows();

        if (DBG_MAIN){
            std::cout<<"\n=================================================================="<<std::endl;
            std::cout<<"********************END OF PLACING PERIODIC FLOWS********************"<<std::endl;
            std::cout<<"=================================================================="<<std::endl;
        }

        if (DBG_MAIN){
            std::cout<<"\n\n=================================================================="<<std::endl;
            std::cout<<"********************PLACING APERIODIC FLOWS********************"<<std::endl;
            std::cout<<"=================================================================="<<std::endl;
        }
        
        rt_network_design->place_aperiodic_flows();

        if (DBG_MAIN){
            std::cout<<"\n=================================================================="<<std::endl;
            std::cout<<"********************END OF PLACING APERIODIC FLOWS********************"<<std::endl;
            std::cout<<"=================================================================="<<std::endl; 
        } 

        //rt_network_design->post_placement_performance_analysis();
        //return 0;

        acceptance_ratio = rt_network_design->post_placement_performance_analysis_csv(output_dirname_fullpath, latency_result_file, acceptance_ratio_result_file);
        acceptance_ratios_map[input_metric].push_back(acceptance_ratio);

        for (auto x: node_specifications){
            x->enterFlows.clear();
            x->enterAFlows.clear();
            x->exitFlows.clear();
            x->exitAFlows.clear();
        }

        for (auto y: link_specifications){
            y->flowsDirect.clear();
            y->flowsReverse.clear();
        }

    }

    //     std::cout << "Key: (" << format_time((it->first).first) <<","<< (it->first).second <<")"<<" -> Values: ";
    //     for (auto ar : it->second){
    //         std::cout <<ar.first <<","<<ar.second<<" ";
    //     }
    //     std::cout << "\n\n\n";
    // }      
    
    return acceptance_ratios_map;
}

/*
Function to get average for a specific key
*/
double getAverageForKey(const std::map<double, std::vector<double>>& myMap, 
                        double key) {
    auto it = myMap.find(key);
    if (it != myMap.end() && !it->second.empty()) {
        double sum = std::accumulate(it->second.begin(), it->second.end(), 0.0);
        return sum / it->second.size();
    }
    return 0.0;  // or throw exception
}

/*
Function to get average for a specific key
*/
double getTotalForKey(const usefulEdgesCountMap& myMap, 
                        std::string key) {
    auto it = myMap.find(key);
    if (it != myMap.end() && !it->second.empty()) {
        uint sum = std::accumulate(it->second.begin(), it->second.end(), 0.0);
        return sum;
    }
    return 0.0;  // or throw exception
}

/*
Filter Flow Information
e.g., const std::string latency_results_filename = "latency_results_topologies_5switch_8link.csv";
const std::string per_flow_results_filename = "Flow10_lats_5switch_8link";
const std::string flow_name = "Flow 10,";
*/
void filter_flow_stats(std::string flow_name, std::string latency_results_filename, std::string per_flow_results_filename) {

    std::ifstream inputFile(latency_results_filename);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open input file: " << latency_results_filename << std::endl;
    }
    
    std::ofstream outputFile(per_flow_results_filename);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open output file: " << per_flow_results_filename << std::endl;
        inputFile.close();
    }
    
    std::string line;
    std::string searchPattern = flow_name + ",";
    
    bool headerCopied = false;

    while (std::getline(inputFile, line)) {

        if ((line.find("Relative-Deadline") != std::string::npos) && !headerCopied){
            headerCopied = true;
            outputFile << line << "\n";
        }
        else if (line.find(searchPattern) != std::string::npos) {
            outputFile << line << "\n";
        }
    }
    inputFile.close();
    outputFile.close();
}


