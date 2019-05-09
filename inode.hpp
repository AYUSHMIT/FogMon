#ifndef INODE_HPP_
#define INODE_HPP_

#include "server.hpp"
#include "istorage.hpp"
#include <string>

class INode {
public:
    //start listener for incoming ping and directions
    virtual void start() = 0;
    //stop everything
    virtual void stop() = 0;

    virtual bool setParam(std::string name, int value) = 0;

    virtual IConnections* getConnections() = 0;
    virtual IStorage* getStorage() = 0;

    virtual void setMyIp(std::string ip) = 0;
    virtual std::string getMyIp() = 0;
    
    virtual int getIperfPort() = 0;
    virtual int getEstimatePort() = 0;
    
    virtual Server* getServer() = 0;
protected:
    std::string ipS;

    virtual int startIperf() = 0;
    virtual int startEstimate() = 0;
};

#endif