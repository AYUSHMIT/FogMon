
#include "master_connections.hpp"
#include "inode.hpp"

#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/un.h>
#include <string>
#include <iostream>

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;
using namespace std;

MasterConnections::MasterConnections(iNode *parent, int nThread) {
    this->parent = parent;
    num = nThread;
    this->running = false;
    
    this->workers = new thread[num];
}

MasterConnections::~MasterConnections() {
    delete [] this->workers;
}

//start the workers and the queue
void MasterConnections::start() {
    
    //check if workers are already running
    for(int i=0; i<num; i++) {
        if(this->workers[i].joinable())
        {
            return;
        }
    }

    //start
    this->running = true;
    this->queue.startqueue();
    for(int i=0; i<num; i++) {
        this->workers[i] = thread(&MasterConnections::worker, this);
    }
}

//stop the workers and the queue
void MasterConnections::stop() {
    this->running = false;
    this->queue.stopqueue();

    for(int i=0; i<num; i++) {
        if(this->workers[i].joinable())
        {
            this->workers[i].join();
        }
    }

}

void MasterConnections::request(int fd) {
    this->queue.push(fd);
}

void MasterConnections::worker() {
    while(this->running.load()) {
        int fd;
        if(this->queue.pop(&fd)!=0)
            break;

        this->handler(fd);    
        close(fd);    
    }
    this->running = false;
}

//return 1 if all ok
//return 0 if end of stream
//return -1 if error
int readS(long fd, void *data, int len) {
	int n;
	int pos =0;
	while(pos<len && (n = recv(fd, &(((char*)data)[pos]), len-pos,0)) > 0)
	{
        if(n == 0)
        {
            //end of stream
            return -1;
        }
		pos+=n;
	}
	if(pos<len)
		return -1;
	return 1;
}

//return 1 if all ok
//return -1 if error
int writeS(long fd, const char *data, int len) {
	int n;
	int pos =0;
	while(pos < len && (n = write(fd, &(data[pos]), len-pos)) < len)
	{
		if(n < 0 && !(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
		{
			perror("send");
			return -1;
		}else
		{
			pos+=n;
		}
	}
	return 1;
}

bool MasterConnections::getMessage(int fd, Message * m) {
    if(fd < 0 || m == NULL)
        return false;

    int error;
    int32_t len;

    error = readS(fd, &len, sizeof(len));
    if (error < 0)
    {
        perror("   recv() failed at len");
    }else if(len > 0) {
        char * data = new char[len];
        error = readS(fd, data, len);
        if(error < 0) {
            delete data;
            perror("   recv() failed at data");
        }else {
            m->Clear();
            if(m->ParseJson(data))
                return true;
        }
    }
    return false;
}

void MasterConnections::handler(int fd) {
    int error;
    Message m;
    if(!this->getMessage(fd,&m))
        return;


    if(m.getType() == Message::Type::REQUEST) {
        if(m.getArgument() == Message::Argument::NODES) {
            if(m.getCommand() == Message::Command::GET) {
                //build array of nodes
                vector<string> nodes = this->parent->getStorage()->getNodes();
                //send nodes

            }else if(m.getCommand() == Message::Command::SET) {
                //refresh all the nodes with the array of nodes
                vector<string> ips;
                if(!m.getData(ips))
                    return;

                //ips now contains the ip of the nodes
                this->parent->getStorage()->refreshNodes(ips);
            }
        }else if(m.getArgument() == Message::Argument::REPORT) {
            if(m.getCommand() == Message::Command::GET) {
                //build report
                this->parent->getStorage()->generateReport();
                //send report
                
            }
        }
    }else if(m.getType() == Message::Type::NOTIFY) {
        if(m.getCommand() == Message::Command::UPDATE) {
            if(m.getArgument() == Message::Argument::NODES) {
                //data contains 2 array: new and deleted nodes
                vector<string> ipsNew;
                vector<string> ipsRem;
                if(!m.getData(ipsNew,ipsRem))
                    return;
                //update the nodes
                this->parent->getStorage()->updateNodes(ipsNew,ipsRem);
            }
        }
    }
    
}