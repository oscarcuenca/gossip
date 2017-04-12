/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

void MP1Node::printMessage(string callr_fn, Address *sendr, Address *recvr, 
                            MessageHdr *msg, int size)
{
    MsgTypes msgType;
    //char *msg_chr;
    
    cout << "<bbi>[" << this->par->getcurrtime() << "]in " << callr_fn << " of MP1Node:" << this->memberNode->addr.getAddress();
    memcpy(&msgType, msg, sizeof(MsgTypes));
    //msg_chr = (char *)msg;
    
    switch(msgType) {
        case(JOINREQ): 
            cout << " JOINREQ"; 
            break;
  
        case(JOINREP): 
            cout << " JOINREP"; 
            break;

        case(MMBRTBL): 
            cout << " MMBRTBL"; 
            break;

        case(DUMMYLASTMSGTYPE): 
            cout << "DUMMYLASTMSGTYPE" << " "; 
            break;
        
        default: 
            cout << "UNKNOWN";

    }
    cout << " from=" << sendr->getAddress();
    cout << " to=" << recvr->getAddress();
    cout << endl;
}

void MP1Node::printNodeData(string caller_fn) {
    cout << "<bbi>[" << this->par->getcurrtime() << "]in " << caller_fn << " of MP1Node:" << this->memberNode->addr.getAddress();
    cout << " data:";        
    cout <<             "inGroup=" << this->memberNode->inGroup << "| ";
    cout <<             "heartbeat=" << this->memberNode->heartbeat << "| "; 
    cout <<             "nnb=" << this->memberNode->nnb << "| ";               
    cout <<             "memberList: size=" << this->memberNode->memberList.size() << "| ";        
    cout << endl;
    cout << "<bbi>[" << this->par->getcurrtime() << "]in " << caller_fn << " of MP1Node:" << this->memberNode->addr.getAddress();
    cout << " data: memberList: ";        
    cout << endl;
    
    // Cannot use this->memberNode->myPos iterator bc it messes up the caller_fn
    size_t pos = 0;
    for (pos = 0; pos < this->memberNode->memberList.size(); pos++) {
        //thisMemberListEntry = this->memberNode->myPos;     
        cout << "<bbi>[" << this->par->getcurrtime() << "]in " << caller_fn << " of MP1Node:" << this->memberNode->addr.getAddress();
        cout << " ";
        cout << "pos=" << pos << "| ";
        cout << "id="           << this->memberNode->memberList[pos].id << "| ";    
        cout << "port="         << this->memberNode->memberList[pos].port << "| ";    
        cout << "heartbeat="    << this->memberNode->memberList[pos].heartbeat << "| ";                
        cout << "timestamp="    << this->memberNode->memberList[pos].timestamp << "| ";                            
        cout << endl;
    }
}

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    //cout << "<bbi>" << "in " << this->memberNode->addr.getAddress() << " nodeStart()" << 
    //    " joinaddr=" << joinaddr.getAddress() << 
        //"| addr[0]=" << this->memberNode->addr.addr[0] << 
        //"| addr[1]=" << this->memberNode->addr.addr[1] << 
    //    endl;

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	 
	//int id = *(int*)(&memberNode->addr.addr);
	//int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    //cout << "<bbi>in initThisNode: id=" << id << "| port=" << port << "|" << endl;

    return 0;
}

void MP1Node::addMemberListEntry(int id, short port, long heartbeat) {
#ifdef DEBUGLOG
    Address newNode;
    memcpy(&newNode.addr[0], &id, sizeof(int));
    memcpy(&newNode.addr[4], &port, sizeof(short));        
    this->log->logNodeAdd(&this->memberNode->addr, &newNode);
#endif

    MemberListEntry *newMemberListEntry = new MemberListEntry(id, port, heartbeat,
                                            (long)this->par->getcurrtime());
    this->memberNode->memberList.emplace_back(*newMemberListEntry);    
    delete(newMemberListEntry);    
}

void MP1Node::delMemberListEntry(int del_id, short del_port) {
#ifdef DEBUGLOG
    Address fldNode;
    memcpy(&fldNode.addr[0], &del_id, sizeof(int));
    memcpy(&fldNode.addr[4], &del_port, sizeof(short));
    this->log->logNodeRemove(&this->memberNode->addr, &fldNode);
#endif

    this->memberNode->heartbeat += 1;
    
    bool member_fnd = false;
    vector<MemberListEntry>::iterator mbrPos;
    
    for (mbrPos = this->memberNode->memberList.begin();
        mbrPos != this->memberNode->memberList.end();
        mbrPos++) {

        if ((del_id == mbrPos->id) && (del_port == mbrPos->port)) {
            member_fnd = true;
            this->memberNode->memberList.erase(mbrPos);
            break;
        }                                                                                              
    }
    
    if (!member_fnd) {
        cout << "<bbi>[" << this->par->getcurrtime() << "]in delMemberListEntry of MP1Node:" 
            << this->memberNode->addr.getAddress() 
            << " id=" << del_id
            << " port=" << del_port
            << " not found !!!"
            << endl;
        exit(EXIT_FAILURE);    
    }

}

/**
 * FUNCTION NAME: addSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
void MP1Node::addSelfToGroup(Address *grp_coord_addr, long grp_coord_heartbeat) {
    this->memberNode->inGroup = true;        
    this->memberNode->heartbeat += 1;
    
    this->addMemberListEntry(*(int*)(&this->memberNode->addr.addr), 
                            *(short*)(&this->memberNode->addr.addr[4]), 
                            this->memberNode->heartbeat);
    
    if (!(*grp_coord_addr == this->memberNode->addr)) {
        this->memberNode->nnb += 1;
        this->addMemberListEntry(*(int*)(grp_coord_addr->addr),
                                *(short*)(&this->memberNode->addr.addr[4]),
                                grp_coord_heartbeat);
    }
    //this->printNodeData("addSelfToGroup");        
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif
    //MemberListEntry *newMemberListEntry; // , *thisMemberListEntry;
    //int id;
    //short port;
    //size_t pos;

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        //cout << "<bbi>[" << this->par->getcurrtime() << "]in introduceSelfToGroup of MP1Node:" << this->memberNode->addr.getAddress();        
        //cout << " starting group" << endl;
        this->memberNode->heartbeat += 1;
        this->addSelfToGroup(&this->memberNode->addr, this->memberNode->heartbeat);
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long);
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message
        msg->msgType = JOINREQ;
        memcpy((char *)(msg) + sizeof(MessageHdr), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        this->memberNode->heartbeat += 1;
        memcpy((char *)(msg) + sizeof(MessageHdr) + sizeof(memberNode->addr.addr), 
                &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);
        //cout << "<bbi>[" << this->par->getcurrtime() << "]in introduceSelfToGroup of MP1Node:" << this->memberNode->addr.getAddress();        
        //cout << " JOINREQ sent from:" << memberNode->addr.getAddress() << " to:" << joinaddr->getAddress() << endl;
        //this->printMessage(msg, msgsize);
        
        this->memberNode->heartbeat += 1;

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
    
    return(0); // <bbi> dummy return
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    //cout << "<bbi>[" << this->par->getcurrtime() << "]calling nodeLoopOps of MP1Node:" 
    //    << this->memberNode->addr.getAddress() << endl;

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    //cout << endl;
    //cout << "<bbi>[" << this->par->getcurrtime() << "]returned from nodeLoopOps of MP1Node:" 
    //    << this->memberNode->addr.getAddress() << endl;

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    //cout << "<bbi>" << "in " << this->memberNode->addr.getAddress() << " checkMessages()" << endl;
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 *  Increment heartbeat when msg is received or sent
	 *  Send JOINREP to a JOINREQ message
	 */
	MessageHdr *recd_msg, *send_msg;
	//static char recd_msgtype[20];
	Address recd_msg_sendr;
	//static char recd_msg_sendr_str[20];
	//static char recd_msgcontent[20];
	int send_msgsize;
	int this_id = *(int*)(&this->memberNode->addr.addr);
    short this_port = *(short*)(&this->memberNode->addr.addr[4]);
    
    //cout << "<bbi>[" << this->par->getcurrtime() << "]enter recvCallBack of MP1Node:" 
    //    << this->memberNode->addr.getAddress();
            
    this->memberNode->heartbeat += 1;

    recd_msg = (MessageHdr *) malloc(size * sizeof(char));
    memcpy((char *)(recd_msg), data, size);
    size_t recd_msg_pos = 0 + sizeof(MsgTypes);
    memcpy(&recd_msg_sendr, (char *)(recd_msg) + recd_msg_pos, sizeof(Address));
    recd_msg_pos += sizeof(Address);

    /*
    switch(recd_msg->msgType) {
        case(JOINREQ): cout << " JOINREQ: size=" << size; break;
        case(JOINREP): cout << " JOINREP: size=" << size; break;
        case(MMBRTBL): cout << " MMBRTBL: size=" << size; break;
        case(DUMMYLASTMSGTYPE): cout << " DUMMYLASTMSGTYPE: size=" << size; break;
        default: cout << " WTF!!!: size=" << size; return(false);
    }
    cout << " sender=" << recd_msg_sendr.getAddress() << endl;
    */
    
    switch(recd_msg->msgType) {
        case(JOINREQ): 
            if (this->memberNode->inited && 
                !this->memberNode->bFailed) {
                
                // send JOINREP message to JOINREQ sender: 
                //  format of data is {struct Address this->addr; long this->heartbeat}

                //cout << " JOINREQ from=" << recd_msg_sendr.getAddress();

                this->memberNode->heartbeat += 1;
                
                send_msgsize = sizeof(MessageHdr) + sizeof(Address) + sizeof(long);
                send_msg = (MessageHdr *) malloc(sizeof(send_msgsize));
                send_msg->msgType = JOINREP;
                memcpy((char *)(send_msg)+sizeof(MsgTypes), &this->memberNode->addr.addr, sizeof(Address));
                memcpy((char *)(send_msg)+sizeof(MsgTypes)+sizeof(Address), 
                                &this->memberNode->heartbeat, 
                                sizeof(long));
                this->emulNet->ENsend(&this->memberNode->addr, &recd_msg_sendr, (char *)send_msg, send_msgsize);    

                // write join notification
                this->addMemberListEntry(*(int*)(&recd_msg_sendr.addr),
                                        *(short*)(&recd_msg_sendr.addr[4]),
                                        *(long *)((char *)recd_msg + sizeof(MessageHdr) + 
                                            sizeof(recd_msg_sendr.addr)));

                free(send_msg);
            }    
            break;
            
        case(JOINREP):
            long recd_msg_sendr_heartbeat;
            memcpy(&recd_msg_sendr_heartbeat, 
                   (char *)recd_msg + sizeof(MessageHdr) + sizeof(recd_msg_sendr.addr),
                   sizeof(long)); 
            this->addSelfToGroup(&recd_msg_sendr, recd_msg_sendr_heartbeat);
            break;
            
        case(MMBRTBL):
            //cout << " MMBRTBL: size=" << size;
        
            // first member is always self
        	this->memberNode->memberList[0].heartbeat = this->memberNode->heartbeat;

            size_t recd_msg_members_n;
            memcpy(&recd_msg_members_n,
                    (char *)(recd_msg)+recd_msg_pos, 
                            sizeof(size_t));
            recd_msg_pos += sizeof(size_t);                

            for (size_t recd_msg_members_pos = 0; recd_msg_members_pos < recd_msg_members_n;
                recd_msg_members_pos++) {
                
                int recd_msg_member_id;
                short recd_msg_member_port;
                long recd_msg_member_heartbeat;
                
                memcpy(&recd_msg_member_id, (char *)(recd_msg)+recd_msg_pos, 
                        sizeof(recd_msg_member_id));
                recd_msg_pos += sizeof(recd_msg_member_id);        

                memcpy(&recd_msg_member_port, (char *)(recd_msg)+recd_msg_pos, 
                        sizeof(recd_msg_member_port));
                recd_msg_pos += sizeof(recd_msg_member_port);        

                memcpy(&recd_msg_member_heartbeat, (char *)(recd_msg)+recd_msg_pos, 
                        sizeof(recd_msg_member_heartbeat));
                recd_msg_pos += sizeof(recd_msg_member_heartbeat); 
                
                if ((recd_msg_member_id == this_id) && 
                    (recd_msg_member_port == this_port))
                    continue;   // do nothing      
                                                       
                bool recd_msg_member_fnd = false;                                                                                
                for (this->memberNode->myPos = this->memberNode->memberList.begin();
                    this->memberNode->myPos != this->memberNode->memberList.end(); 
                    this->memberNode->myPos++) {

                    if ((recd_msg_member_id == this->memberNode->myPos->id) && 
                        (recd_msg_member_port == this->memberNode->myPos->port)) { 
                        
                        recd_msg_member_fnd = true;
                        
                        if (recd_msg_member_heartbeat <= this->memberNode->myPos->heartbeat)
                            break;   // move to next member in msg
                        else {
                            this->memberNode->myPos->heartbeat = recd_msg_member_heartbeat;
                            this->memberNode->myPos->timestamp = this->par->getcurrtime();                    
                        }        
                    }                                                                                              
                }
                
                if (!recd_msg_member_fnd) {
                    this->addMemberListEntry(recd_msg_member_id, 
                                            recd_msg_member_port, 
                                            recd_msg_member_heartbeat);   
                }                    
            }
            
            //cout << " recd_msg_pos=" << recd_msg_pos;
            break;
                
        case(DUMMYLASTMSGTYPE): 
            break;
            
        default: 
            return(false);
    }

    //delete(recd_msg_sendr);
    free(recd_msg);
    
    //cout << endl;
    //cout << "<bbi>[" << this->par->getcurrtime() << "]exit recvCallBack of MP1Node:" 
    //    << this->memberNode->addr.getAddress() << endl;

    return(true);
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
	 
	if ((this->par->getcurrtime() % TOPS) != 0) 
	    return;

    //cout << "<bbi>[" << this->par->getcurrtime() << "]enter nodeLoopOps of MP1Node:" 
    //    << this->memberNode->addr.getAddress();
    //this->printNodeData("nodeLoopOps");        
    
    // check for failures
    bool nofails;
    vector<MemberListEntry>::iterator mbrPos;
    do {
        nofails = true;
        for (mbrPos = this->memberNode->memberList.begin();
            mbrPos != this->memberNode->memberList.end();
            mbrPos++) {
            if (mbrPos->timestamp + TREMOVE < this->par->getcurrtime()) {
                cout << "detecting potential failure at "
                    << this->memberNode->addr.getAddress()
                    << " for " << mbrPos->id
                    << ":" << mbrPos->port
                    << endl;
                //this->printNodeData("nodeLoopOps");
                // this member entry has failed
                nofails = false;
                this->delMemberListEntry(mbrPos->id, mbrPos->port);
                break;                                                    
            }        
        }                
    } while (!nofails);

	    
	// Send MMBRTBL msg
	//	MMBRTBL  // format: MMBRTBL|sender_addr|# MemberListEntry|<id|port|heartbeat|>    
	this->memberNode->heartbeat += 1;
	// first member is always self
	this->memberNode->memberList[0].heartbeat = this->memberNode->heartbeat;
	this->memberNode->memberList[0].timestamp = this->par->getcurrtime();
	
	//cout << " sizeof(MessageHdr)=" << sizeof(MessageHdr);
	//cout << " sizeof(MsgTypes)=" << sizeof(MsgTypes);
	//cout << " sizeof(Address)=" << sizeof(Address);
	//cout << " sizeof(size_t)=" << sizeof(size_t);
	//cout << " this->memberNode->memberList.size()=" << this->memberNode->memberList.size();
	//cout << " sizeof(MemberListEntry content)=" << sizeof(int) + sizeof(short) + sizeof(long);
	
	// find number of alive members in MemberListEntry
	size_t alive_n = 0, mmbr_pos = 0;
	for (alive_n = 0, mmbr_pos = 0; 
	        mmbr_pos < this->memberNode->memberList.size(); mmbr_pos++) {
        if (this->memberNode->memberList[mmbr_pos].timestamp + TFAIL >= 
            this->par->getcurrtime())
            alive_n++;
	}        
	
	size_t msgsize = sizeof(MessageHdr) + sizeof(Address) + sizeof(size_t) // # of entries
	                        + (alive_n * 
	                            (0 
	                            + sizeof(int)    // id
	                            + sizeof(short)  // port 
	                            + sizeof(long)   // heartbeat
	                            ));
    //cout << " msgsize=" << msgsize;
    
	MessageHdr *msg;
    msg = (MessageHdr *) malloc(msgsize);
    
    size_t msg_pos = 0;
    msg->msgType = MMBRTBL; msg_pos += sizeof(MsgTypes);
    memcpy((char *)(msg)+msg_pos, &this->memberNode->addr.addr, sizeof(Address));
    msg_pos += sizeof(Address);
    
    memcpy((char *)(msg)+msg_pos, 
                    &alive_n, 
                    sizeof(size_t));
    msg_pos += sizeof(size_t);                

    for (this->memberNode->myPos = this->memberNode->memberList.begin();
        this->memberNode->myPos != this->memberNode->memberList.end(); 
        this->memberNode->myPos++) {
        
        if (this->memberNode->myPos->timestamp + TFAIL < 
            this->par->getcurrtime())
            continue;

        memcpy((char *)(msg)+msg_pos, 
                    &this->memberNode->myPos->id, 
                    sizeof(this->memberNode->myPos->id));
        msg_pos += sizeof(this->memberNode->myPos->id);                

        memcpy((char *)(msg)+msg_pos, 
                    &this->memberNode->myPos->port, 
                    sizeof(this->memberNode->myPos->port));
        msg_pos += sizeof(this->memberNode->myPos->port);                

        memcpy((char *)(msg)+msg_pos, 
                    &this->memberNode->myPos->heartbeat, 
                    sizeof(this->memberNode->myPos->heartbeat));
        msg_pos += sizeof(this->memberNode->myPos->heartbeat);                
    }                
    
    //cout << " msg_pos=" << msg_pos;

    for (size_t target_num = 0; target_num < BGOSSIPFANOUT; target_num++) {
        // first element is self
        size_t target_elem = rand() % (this->memberNode->memberList.size() - 1) + 1;
        //cout << endl << " target_elem=" << target_elem;
        
        Address target_addr;
        //cout << " this->memberNode->memberList[target_elem].id=" 
        //    << this->memberNode->memberList[target_elem].id;
        //cout << " this->memberNode->memberList[target_elem].port=" 
        //    << this->memberNode->memberList[target_elem].port;
        memcpy(&target_addr.addr[0], &this->memberNode->memberList[target_elem].id, sizeof(int));
		memcpy(&target_addr.addr[4], &this->memberNode->memberList[target_elem].port, sizeof(short));
        //cout << " target_addr=" << target_addr.getAddress() << endl;

        this->emulNet->ENsend(&this->memberNode->addr, &target_addr, (char *)msg, msgsize);
        //cout << endl;
        //cout << " msgType=" << msg->msgType << " sent to=" << target_addr.getAddress();
        //cout << " msgsize=" << msgsize << " msg_pos=" << msg_pos;
        //this->printMessage("nodeLoopOps", &this->memberNode->addr, &target_addr, msg, msgsize);    
    }

    //cout << " attempting to free msg..." << endl;
	free(msg);    
	//cout << " freed msg" << endl;
	
    //cout << endl;        
    //cout << "<bbi>[" << this->par->getcurrtime() << "]exit nodeLoopOps of MP1Node:" 
    //    << this->memberNode->addr.getAddress() << endl; 

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
