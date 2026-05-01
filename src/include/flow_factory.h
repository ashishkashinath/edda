#ifndef flow_factory_h
#define flow_factory_h

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <math.h>
#include <util.h>
#define PROPAGATION_DELAY 0

class Flow {
    public:
        uint flow_id;
        std::string flow_name;
        std::string src;
        std::string dst;
        double packet_size;
        double bandwidth;
        double period;
        double relative_deadline;
        uint priority;
        flow_type ft;
        flow_safety_mode fm;

        Node* source;
        Node* destination;      
        Path* assigned_path; //Clear this for every new design
        std::vector<Path*> all_possible_paths; //Clear this for every new design
        double actual_delay; //Clear this for every new design
        double expected_min_delay_rta; //Clear this for every new design
        double expected_min_delay_frc; //Clear this for every new design
        bool placed; //Clear this for every new design

        //Methods
        Flow(uint id, std::string name, Node& src, Node& dst, double packet_size, double rate, double period, double relative_deadline, uint priority);
        Flow(uint id, std::string name, std::string src_name, std::string dst_name, double pkt_size, double bw, 
        double time_period, double rel_ddl, uint prio, flow_type fl_type, flow_safety_mode fl_mode);
        
        //Methods based on Response-time Analysis
        double getJobSelfProcessingTime(Node *n);
        double perLinkDelay(Link* l);
        double perNodeDelay(Node* n);
        double perNodeInterferenceDelay(Node *n);
        double perNodeQueuingDelay(Node *n);
        double getPathDelay(Path* P);
        void displayFlow();
        void displayFlowMinimal();
};

class AperiodicFlow:public Flow {
    public:
        double period_min;
        double period_max;
        double utilization;
        double interference;
        
        AperiodicFlow(uint id, std::string name, Node& src, Node& dst, double packet_size, double rate, double period, double relative_deadline, uint priority, double period_low, double period_high):
        Flow{id, name, src, dst, packet_size, rate, period, relative_deadline, priority},period_min{period_low}, period_max{period_high}{};

        AperiodicFlow(uint id, std::string name, std::string src_name, std::string dst_name, double pkt_size, double bw, double time_period, double rel_ddl, uint prio, 
        flow_type fl_type, flow_safety_mode fl_mode, double period_low, double period_high):
        Flow{id, name, src_name, dst_name, pkt_size, bw, time_period, rel_ddl, prio, fl_type, fl_mode},period_min{period_low}, period_max{period_high}{};

        double getJobSelfProcessingTime(Node *n);

};

bool compareFlow(Flow *lhs, Flow *rhs);

#endif
