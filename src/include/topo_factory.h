#ifndef topology_factory_h
#define topology_factory_h

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <utility>  
#include <util.h>

using namespace std;

/*
* Node in our network is a 'switch' which has flows associated with it,
* links connected to it and delays expended at it 
*/

class Node{

    public:
        uint node_id;
        std::string node_name;
        double node_bw_capacity;
        
        std::vector<Link*> links;

        std::vector<Flow*> enterFlows;
        std::vector<Flow*> exitFlows;

        std::vector<AperiodicFlow*> enterAFlows;
        std::vector<AperiodicFlow*> exitAFlows;
        
        AperiodicServer* aperiodic_server;
    
    //Methods
        void init(double bw_capacity, 
            std::vector<Link*>& links,
            std::vector<Flow*>& enterFlows,
            std::vector<Flow*>& exitFlows,
            std::vector<AperiodicFlow*>& enterAFlows,
            std::vector<AperiodicFlow*>& exitAFlows,
            AperiodicServer* aperiodic_server_specs);

        Node(uint id=0, std::string name="test", double node_bw_capacity=100e6);
        uint getid();
        std::string get_name();
        void printLinks();
        void printEnterFlows();
        void printExitFlows();
        bool operator==(const Node& other) const{ 
            return (node_id == other.node_id && node_name == other.node_name
            && node_bw_capacity == other.node_bw_capacity);
        }
        bool operator==(const Node* other) const{ 
            return (node_id == other->node_id && node_name == other->node_name
            && node_bw_capacity == other->node_bw_capacity);
        }  
};

class CustomNodePHash{
    public:
        size_t operator()(Node* p) const{
            return (p->node_id + p->node_name.length() + p->node_bw_capacity);
        }
};

class Link{

    public:
        uint link_id;
        std::string link_name;
        double link_bw_capacity;
        std::string src;
        std::string dst;

        Node* node1;
        Node* node2;   
        std::vector<Flow*> flowsDirect; // flows from node1->node2
        std::vector<Flow*> flowsReverse; //flows from node2->node1

        float cost;

    //Methods
        void init(Node& n1, Node& n2,
            double bw_capacity, std::vector<Link*>& links,
            std::vector<Flow*>& directFlows, std::vector<Flow*>& reverseFlows);
        Link(uint id, std::string name, double bw_cap, std::string src, std::string dst);
        Link(uint id, std::string name, double bw_cap, std::string src, std::string dst, double c);
        Link(const Link &);   
        Link& operator=(const Link &);
        void displayLink();
};

class CustomLinkPHash{
    public:
        size_t operator()(Link* l) const{
            return (l->link_id + l->link_name.length() + l->link_bw_capacity);
        }
};

class Path{
    public:
        std::vector<Node*> nodes;
        std::vector<Link*> links;
        std::tuple<Node*,Link*,Node*> segment;

    // Methods
        Path(std::vector<Node*> nodes_N = {}, std::vector<Link*> links_L={});
        bool containsLink(Link* l);
        void get_folds(std::vector<std::vector<Node*>>& x);
        Path* get_subpath(Node* intermediate);
        bool operator==(const Path &other) const{
            return (nodes == other.nodes && links == other.links);
        }
        void printPath();
        void reversePath(); 
};

class CustomPathHash{
    public:
        size_t operator()(Path& p) const{
            return (p.nodes.size() + p.links.size());
        }
};

class CustomFlowPathHash{
    public:
        size_t operator()(const FlowPath& fp) const{
            return ((fp.second)->nodes.size() + (fp.second)->links.size());
        }
};

#endif
