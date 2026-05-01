#include <iostream>
#include <vector>
#include <tuple>
#include <string>
#include <random>
#include<algorithm>
#include "flow_factory.h"
#include "topo_factory.h"
#include "netdesign_factory.h"
#include "util.h"

//Random Number Generation Utility: https://stackoverflow.com/questions/7114043/random-number-generation-in-c11-how-to-generate-how-does-it-work
typedef std::default_random_engine MyRNG;  // the default random number generator
uint32_t seed_val = 1727;           // assign some seed value

/* Class Definitions of Node Class*/
void Node::init(double bw_capacity, std::vector<Link*>& lnks, 
    std::vector<Flow*>& ent_Flows, std::vector<Flow*>& ex_Flows, 
    std::vector<AperiodicFlow*>& ent_AFlows, std::vector<AperiodicFlow*>& ex_AFlows,
    AperiodicServer* aperiodic_server_specs){
    node_bw_capacity = bw_capacity;
    links = lnks;
    enterFlows = ent_Flows;
    exitFlows = ex_Flows;
    enterAFlows = ent_AFlows;
    exitAFlows = ex_AFlows;
    aperiodic_server = aperiodic_server_specs;        
}

Node::Node(uint id, std::string name, double node_cap){
    node_id = id;
    node_name = name;
    node_bw_capacity = node_cap;
    //std::cout<<"Done initializing switch with "<<"ID = "<<id<<std::endl;
}

uint Node::getid(){
    return node_id;
}

std::string Node::get_name(){ 
    return node_name;
}

void Node::printLinks(){
    std::cout<<"Links: ";
    for (auto i : links)
        std::cout << i <<"("<<i->link_id <<")"<<" ";
    std::cout<<std::endl;
}

void Node::printEnterFlows(){
    std::cout<<"Enter Flows: ";
    for (auto i : enterFlows)
        std::cout << i<<"("<<i->flow_id <<")"<<" ";
    std::cout<<std::endl;
}

void Node::printExitFlows(){
    std::cout<<"Exit Flows: ";
    for (auto i : exitFlows)
        std::cout << i<<"("<<i->flow_id <<")"<<" ";
    std::cout<<std::endl;
}

/* Class Definitions of Link Class*/
void Link::init(Node& n1, Node& n2, double bw_capacity, 
    std::vector<Link*>& lnks, std::vector<Flow*>& dir_Flows,
    std::vector<Flow*>& rev_Flows){
    
    *node1 = n1;
    *node2 = n2;
    link_bw_capacity = bw_capacity;
    flowsDirect = dir_Flows;
    flowsReverse = rev_Flows;        
}

Link::Link(uint id, std::string name, double bw, std::string source, std::string destination){
    link_id = id;
    link_name = name;
    link_bw_capacity = bw;
    src = source;
    dst = destination;
    //std::cout<<"Done initializing link with "<<"ID = "<<id<<std::endl;
}

Link::Link(uint id, std::string name, double bw, std::string source, std::string destination, double c){
    link_id = id;
    link_name = name;
    link_bw_capacity = bw;
    src = source;
    dst = destination;
    cost = c;
    //std::cout<<"Done initializing link with "<<"ID = "<<id<<std::endl;
}

 // Implementation of copy constructor
 Link::Link(const Link & linkType)
     : link_id(linkType.link_id)
        , link_name(linkType.link_name)
        , link_bw_capacity(linkType.link_bw_capacity)
        , src(linkType.src)
        , dst(linkType.dst){
    
    printf("COPYING\n");
}


Link& Link::operator=(const Link &rhs)
{
    if ( this != &rhs )
    {
        Link temp(rhs);
        std::swap(temp.link_id, link_id);
        std::swap(temp.link_name, link_name);
        std::swap(temp.link_bw_capacity, link_bw_capacity);
        std::swap(temp.src, src);
        std::swap(temp.dst, dst);
    }
    return *this;
}


void Link::displayLink(){
    std::cout << "Link ID: "<<link_id<<std::endl;
    std::cout << "Link Name: "<<link_name<<std::endl;
    std::cout << "Node 1: "<<node1->node_id<<std::endl;
    std::cout << "Node 2: "<<node2->node_id<<std::endl;
    std::cout << "Link Bandwidth: "<<link_bw_capacity<<std::endl;
}


/* Class Definitions of Path Class*/
Path::Path(std::vector<Node*> nodes_N, std::vector<Link*> links_L){
    nodes = nodes_N;
    links = links_L;
}

bool Path::containsLink(Link* l){
    
    bool containsLink = false;
    for (auto L:links){
        if (L->link_id == l->link_id){
            containsLink = true;
            break;
        }
    }
    return containsLink;
}

void Path::get_folds(std::vector<std::vector<Node*>>& folds){
    
    //std::vector<std::vector<Node*>> folds;
    std::unordered_map<Node*, bool, CustomNodePHash> visited;
    std::vector<Node*>::const_iterator first;
    std::vector<Node*>::const_iterator last;

    auto start = 0;
    for (auto i=0; i != nodes.size(); i++){
        if(visited[nodes[i]]){
            first = nodes.begin() + start;
            last = nodes.begin() + i;
            
            std::vector<Node*> val1(first, last);
            folds.push_back(val1);
            for(auto x : val1)
                visited[x] = false;
            start = i;
        }
        else
            visited[nodes[i]] = true;

    }
    std::vector<Node*>::const_iterator beg = nodes.begin() + start;
    std::vector<Node*>::const_iterator fin = nodes.end();
    std::vector<Node*> val2(beg, fin);
    folds.push_back(val2);

}

Path* Path::get_subpath(Node* inter){

    std::vector<Node*>::iterator subpath_start_node;
    std::vector<Link*>::iterator subpath_start_link;

    for (std::vector<Node*>::iterator sw=nodes.begin(); sw!=nodes.end(); sw++){
        if(*sw == inter){
            subpath_start_node = sw;
            break;
        }
    }
    
    // std::cout<<"Starting node is "<<(*subpath_start_node)->node_name<<"\n";
    std::vector<Node*> subpath_nodes(subpath_start_node, nodes.end());

    // std::cout<<"Number of nodes = "<<subpath_nodes.size()<<std::endl;

    std::vector<Link*> subpath_links;
    if (subpath_nodes.size() > 1){

        for (auto val=links.begin(); val!=links.end(); val++){
            // std::cout<<"Checking "<<(*val)->link_name<<std::endl;
            if(((*val)->node1 == *subpath_start_node)||((*val)->node2 == *subpath_start_node)){
                subpath_start_link = val;
                break;
            }
        }
        if ((subpath_start_link!=links.begin()) && (subpath_start_link!=links.end()))
            subpath_start_link++;
        //std::cout<<"Starting link is"<<(*subpath_start_link)->link_name<<"\n";
        subpath_links.assign(subpath_start_link, links.end());

    }

    // for (auto ch:subpath_links)
    //     std::cout<<"Link is: "<<ch->link_name<<std::endl;

    Path* subpath = new Path(subpath_nodes, subpath_links);
    return subpath;
}

void Path::printPath(){

    uint i;

    for (i = 0; i < nodes.size()-1; i++)
        std::cout<<nodes[i]->node_name<<"--";
    
    std::cout<<nodes[i]->node_name<<std::endl;
}

void Path::reversePath(){

    std::reverse(nodes.begin(), nodes.end());
    std::reverse(links.begin(), links.end());
    
}