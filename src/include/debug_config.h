#ifndef debug_config_h
#define debug_config_h

#define US_PER_SEC 1000000
#define MS_PER_SEC 1000
#define US_PER_MS 1000
#define DBG_DEL_COMP 0
#define DBG_ADAP_ROU 1
#define DBG_ADAP_ROU_PRE 0
#define DBG_RESP_TIME 0
#define DBG_FRC 0
#define DBG_EXPLORE 0
#define DBG_DSE 0
#define DBG_RMA_DELAY 0
#define DBG_NODE_DELAY 0
#define DBG_LINK_DELAY 0
#define DBG_SORTED_FLOWS 0
#define DBG_NW_CAPTURE_SPECS 0
#define DBG_NW_DESIGN 0
#define DBG_VIEW 0
#define DBG_DBG 0
#define DBG_LINE 1
#define DBG_UTIL 0
#define DBG_IGNORE_NODE_DELAY 0
#define DBG_SYSTEM_FACTORY 0
#define DBG_FLOW_FACTORY 0
#define DBG_MAIN 0
#define DBG_NV_EXPLORE 0

#if DBG_LINE
    #define DBG_PRINT std::cout<<"\n---- DEBUG In "<<__FILE__<<":"<<__func__<<":"<<__LINE__<<std::endl;
#else
    #define DBG_PRINT
#endif

#endif