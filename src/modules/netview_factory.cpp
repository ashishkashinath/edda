#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <flow_factory.h>
#include <util.h>
#include <debug_config.h>
#include <topo_factory.h>
#include <netview_factory.h>
#include <netview_factory.h>
#include <adaptive_routing.h>
#include <aperiodic_server.h>
#include <iomanip>

/* Class definitions for the RTNetView class*/
RTNetView::RTNetView(Flow* current_periodic_flow, std::vector<Flow*>& existing_periodic_flows,
            std::vector<AperiodicFlow*>& existing_aperiodic_flows, RTNetwork* rtn){
    cur_periodic_flow = current_periodic_flow;
    cur_aperiodic_flow = nullptr;
    source = current_periodic_flow->source;
    destination = current_periodic_flow->destination;
    planned_periodic_flows = existing_periodic_flows;
    planned_aperiodic_flows = existing_aperiodic_flows;
    rt_network = rtn;
    compute_view();

}

RTNetView::RTNetView(AperiodicFlow* current_aperiodic_flow, std::vector<Flow*>& existing_periodic_flows,
            std::vector<AperiodicFlow*>& existing_aperiodic_flows, RTNetwork* rtn){
    cur_periodic_flow = nullptr;
    cur_aperiodic_flow = current_aperiodic_flow;
    source = current_aperiodic_flow->source;
    destination = current_aperiodic_flow->destination;    
    planned_periodic_flows = existing_periodic_flows;
    planned_aperiodic_flows = existing_aperiodic_flows;
    rt_network = rtn;
    compute_view();
}

void RTNetView::get_all_possible_paths(Node* src, Node* dst){
    
    if(DBG_NV_EXPLORE)
        std::cout<<"Paths between "<<src->node_name<<" and "<<dst->node_name<<":"<<std::endl;
    std::unordered_map<Node*, bool, CustomNodePHash> visited;
    Path* discovered_path = new Path({},{});
   
    explore(src, dst, discovered_path, visited);
    // discovered_path->nodes.push_back(src);
    // exploreDFS(src, dst, discovered_path, 0, 100);
}

/*
* exploreDFS() is a function to store all possible paths from start to finish in
* the all_possible_paths array, provided the path length is in the interval [min_hops, max_hops]
*/
//TODO: need to record links in exploreDFS
void RTNetView::exploreDFS(Node* start, Node* finish, Path* path_visited, const int& min_hops, const int& max_hops){
    
    Node* back = path_visited->nodes.back();

    // Examine adjacent nodes
    for (auto link: back->links){
        Node* discovered = (link->node1 == start)?link->node2:link->node1;
        //continue if the node is already part of the path_visited
        if (std::find(path_visited->nodes.begin(), path_visited->nodes.end(), discovered)!=path_visited->nodes.end())
            continue;
        
        if (discovered == finish){
            path_visited->nodes.push_back(discovered);
            //Get hop count for this path

            const int sz = (int) path_visited->nodes.size();
            const int hops = sz - 1;

            if ((hops >= min_hops)&& (hops <= max_hops)){
                std::vector<Node*> x(path_visited->nodes.begin(), path_visited->nodes.begin()+sz);
                Path *temp = new Path(x,{});
                if (DBG_NV_EXPLORE){
                    std::cout<<"PATH FOUND"<<std::endl;
                    for (auto x:path_visited->nodes)
                            std::cout<<x->node_name<<" ";
                    std::cout<<"\n\n";
                }
                all_possible_paths.push_back(temp);
            }

            path_visited->nodes.erase(path_visited->nodes.begin()+sz);
            break;
        }
    }

    // In DFS, recursion comes after visiting adjacent nodes
    for (auto link:back->links){
        Node *disc = (link->node1 == start)?link->node2:link->node1;

        if ((std::find(path_visited->nodes.begin(), path_visited->nodes.end(), disc)!= path_visited->nodes.end())||(disc == finish))
            continue;
        
        path_visited->nodes.push_back(disc);
        exploreDFS(disc, finish, path_visited, min_hops, max_hops);

        int n = (int)path_visited->nodes.size() - 1;
        path_visited->nodes.erase(path_visited->nodes.begin() + n);
    }

}

/*
* explore() is a function to store all possible paths from start to finish in
* the all_possible_paths array
*/
void RTNetView::explore(Node* start, Node* finish, Path* path_discovered, 
    std::unordered_map<Node*, bool, CustomNodePHash>& visited){

    Node* discovered;

    if (!visited[start]){
        if(DBG_NV_EXPLORE)
            std::cout<<"Marking "<<start->node_name<<" as visited"<<std::endl;
        visited[start] = true;
        if(DBG_NV_EXPLORE)
            std::cout<<"Adding "<<start->node_name<<" to discovered nodes"<<std::endl;
        path_discovered->nodes.push_back(start);
    }
    
    if (start == finish){
        std::vector<Node*> nodes_visited(path_discovered->nodes.begin(), path_discovered->nodes.end());
        std::vector<Link*> links_visited(path_discovered->links.begin(), path_discovered->links.end());

        Path *X = new Path(nodes_visited,links_visited);
        all_possible_paths.push_back(X);

        if(DBG_NV_EXPLORE){
            std::cout<<"REACHED DESTINATION"<<start->node_name<<std::endl;
            
            for (auto x:X->nodes)
                std::cout<<x->node_name<<" ";
            std::cout<<"\n";

            for (auto x:X->links)
                std::cout<<x->link_name<<" ";
            std::cout<<"\n";
        }
        visited[start] = false;
    }
    else{
        //Discover the edges of Node 'start'
        if(DBG_NV_EXPLORE)
            std::cout<<"Exploring links at node "<<start->node_name<<std::endl;
        for (auto link: start->links){
            if(DBG_NV_EXPLORE)
                std::cout<<"At "<<start->node_name<<" Link Name = "<<link->link_name<<" , "<<"Link ID = "<<link->link_id<<std::endl;

            discovered = (link->node1 == start)?link->node2:link->node1;

            if (!visited[discovered]){
                if (DBG_NV_EXPLORE)
                    std::cout<<"Found an unvisited node "<<discovered->node_name<<" address "<<discovered<<std::endl;

                path_discovered->links.push_back(link);

                explore(discovered, finish, path_discovered, visited);

                if (DBG_NV_EXPLORE)
                    std::cout<<"Removing "<<path_discovered->nodes.back()->node_name<<" from path_discovered"<<std::endl;

                path_discovered->nodes.pop_back();
                path_discovered->links.pop_back();
            }
        }
        if (DBG_NV_EXPLORE)
            std::cout<<"Marking "<<start->node_name<<" as not visited"<<std::endl;
        visited[start] = false;
    }
}

/*
* RTNetView computes the "view" of the network seen by a flow in terms of anticipated delays
* The view, comprises only nodes and links where the flow _can potentially_ be placed
* node_view[x] captures the execution time of cur_(a)periodic_flow on node x
* 
*/
void RTNetView::compute_view(){
    
    std::vector<Path*> possible_paths;
    std::vector<Node*> intersecting_nodes;
    std::vector<Link*> intersecting_links;

    std::vector<Node*>::iterator rtnetview_nodes_it;
    std::vector<Link*>::iterator rtnetview_links_it;

    get_all_possible_paths(source, destination);

    for (auto P : all_possible_paths){
        /*get intersecting nodes and links*/
        intersecting_nodes = (P)->nodes;
        intersecting_links = (P)->links;

        for (auto x:intersecting_nodes){
            if (cur_periodic_flow)
                node_view[x] = cur_periodic_flow->getJobSelfProcessingTime(x); //cur_periodic_flow->perNodeDelay(x)
            else
                node_view[x] = cur_aperiodic_flow->getJobSelfProcessingTime(x); //cur_aperiodic_flow->perNodeDelay(x);
            
            if (std::find(rtnetview_nodes.begin(), rtnetview_nodes.end(), x) == rtnetview_nodes.end()){
                if (DBG_VIEW)
                    std::cout<<"Adding node "<<x->node_name<<" to the view"<<std::endl;
                rtnetview_nodes.push_back(x);
            }
        }
        
        for (auto y:intersecting_links){
            if(cur_periodic_flow)
                link_view[y] = cur_periodic_flow->perLinkDelay(y);
            else
                link_view[y] = cur_aperiodic_flow->perLinkDelay(y);

            if (std::find(rtnetview_links.begin(), rtnetview_links.end(), y) == rtnetview_links.end()){
                if (DBG_VIEW)
                    std::cout<<"Adding link "<<y->link_name<<"to the view"<<std::endl;
                rtnetview_links.push_back(y);
            }
        }
    }

    if (DBG_VIEW){
        if (cur_periodic_flow)
            std::cout<<"Computing View Done for "<<cur_periodic_flow->flow_name<<std::endl;
        else
            std::cout<<"Computing View Done for Aperiodic "<<cur_aperiodic_flow->flow_name<<std::endl;
    }

}

void RTNetView::printNetView(){
    std::cout<<"\n\n\t=============================================="<<std::endl;
    std::cout<<"\t****************PRINTING THE VIEW*************"<<std::endl;
    std::cout<<"\t=============================================="<<std::endl;

    if (cur_periodic_flow)
        std::cout<<"\tCurrent Flow : "<<cur_periodic_flow->flow_name<<std::endl;
    else
        std::cout<<"\tCurrent Aperiodic Flow : "<<cur_aperiodic_flow->flow_name<<std::endl;

    std::cout<<"\tPlanned Periodic Flows :";
    for (auto F: planned_periodic_flows)
        std::cout<<F<<"("<<F->flow_name<<")"<<" ";
    
    std::cout<<"\tPlanned Aperiodic Flows :";
    for (auto G: planned_aperiodic_flows)
        std::cout<<G<<"("<<G->flow_name<<")"<<" ";
    
    std::cout<<std::endl;

    std::cout<<"\tNode Weights :\n";
    std::unordered_map<Node*, double, CustomNodePHash > :: iterator it_node;
    for (it_node = node_view.begin(); it_node != node_view.end(); it_node++)
      std::cout <<"\t\t"<< it_node->first->node_name<<"("<<it_node->first <<")"<<": "<< it_node->second <<std::endl;

    std::cout<<"\tLink Weights :\n";
    std::unordered_map<Link*, double, CustomLinkPHash > :: iterator it_link;
    for (it_link = link_view.begin(); it_link != link_view.end(); it_link++)
      std::cout <<"\t\t"<< it_link->first->link_name<<"("<<it_link->first <<")"<<": "<< it_link->second << std::endl;
    
    std::cout<<"\tSource = "<<source->get_name()<<"-->"<<"Destination = "<<destination->get_name()<<std::endl;  
    std::cout<<"\tNumber of all_possible_paths in RTNetView = "<<all_possible_paths.size()<<std::endl;
    std::cout<<"\t----RTNetView has the following Paths----"<<std::endl;
    uint x = 0;

    for (auto P : all_possible_paths){
        std::cout<<"\t\tPath #"<<++x<<" ";
        printPath(P);
    }

    std::cout<<"\n\t=============================================="<<std::endl;
    std::cout<<"\t*******************END OF VIEW****************"<<std::endl;
    std::cout<<"\t=============================================="<<std::endl;

}

void RTNetView::printPath(Path* P){

    std::vector<Node*>::iterator node_it1, node_it2;
    std::vector<Link*>::iterator link_it;
    Link *L ;
    uint i;
    for (i = 0; i < (P->nodes).size()-1; i++){
            rt_network->getLink(P->nodes[i], P->nodes[i+1], &L);
            std::cout<<P->nodes[i]->node_name<<"--"<<(*L).link_name<<"--";
    }
    std::cout<<P->nodes[i]->node_name;

    // std::cout<<std::endl;
}

void RTNetView::printPathNodes(Path* P){
    for (uint i = 0; i < (P->nodes).size(); i++)
        std::cout<<P->nodes[i]<<"("<<P->nodes[i]->node_name<<")"<<" ";

    std::cout<<std::endl<<std::endl;
}

Path* RTNetView::get_shortest_hoplength_path(){
    uint min_path_length = UINT8_MAX;
    double path_length;

    for (auto P: all_possible_paths){
        path_length = P->links.size();
        if (path_length <= min_path_length){
            min_path_length = path_length;
            shortest_path = P;
        }
    }
    return shortest_path;
}