#ifndef STATEMACHINE_H
#define STATEMACHINE_H	1
#define MAXNB_DEVICES	4
int CMDHandler(SOCKET sockfd);
typedef enum {
	DEVICEEVENT_DEVICE_ATTACH,
	DEVICEEVENT_DEVICE_DETTACH,
}DEVICEEVENT;

class CDeviceEvent{
public:
	DEVICEEVENT DeviceEvent;
	int	len;
	void* opague;
};


/**********************************************************
*
*		Control Com State and Control Com Context
*
***********************************************************/
/*
state pattern 
base:
CControlBaseState
drive:
CCcomWaitingState
CCcomReadyState
CCcomDead
*/

class CControlChannelContext;
class CControlBaseState{
public:
	virtual void recvConnectData(CControlChannelContext* ptContext,SOCKET fd) {};
	virtual void recvMngEvent() {};
	virtual void recvMsgDeviceEvent(CControlChannelContext* ptContext,CDeviceEvent* event){};
	//vitual void recvUsrCMD() = 0;
};

class CControlChannelContext{
public:
	CControlChannelContext(SOCKET sockfd,char* IPAddr,char* PortBase);
	
	void recvConnectData();
	void recvMngEvent();
	void recvMsgDeviceEvent(CDeviceEvent* evt);
	void changeState(CControlBaseState* newState){
		_controlState = newState;
	}
//	int	contect2DataCom(int idx);
	/*void closeDataCom(){
		closesocket(_DataSock);
	}*/
	void closeControlCom(){
		closesocket(_ControlSock);
	}
	SOCKET getControlSock(){
		return _ControlSock;
	}
	SOCKET getDataSock(){
		return _DataSock;
	}
	int sendCMD2ControlCom(uint16_t cmdtype,int status);
	int sendData2ControlCom(void* buf,int bufLen);
	int recvCMDFromControlCom(struct op_common* ptPeerCMD);
	int recvDataFromControlCom(void*buf,int rdLen);
	int	findEmptyDevContext(){
		int i ,found = 0;
		for(i=0;i<MAXNB_DEVICES;i++){
			if(_DevContext[i]){
				if(!_DevContext->isEmptyDevContext())
					return i;
			}
		}
		return -1;
	}
	int poseDevContext(uint16_t vid,uint16_t pid,uint32_t magic,int idx){
		_DevContext[idx]->poseContext(vid,pid,magic);
	}
	int resetDevContext(int idx){
		_DevContext[idx]->resetContext();
	}
	int findDevContextThrMagic(uint32_t magic){
		int i ,found = 0;
		for(i=0;i<MAXNB_DEVICES;i++){
			if(_DevContext[i]){
				if(!_DevContext->isSameContext(magic))
					return i;
			}
		}
		return -1;
	}
	int findDevContextThrDevID(uint16_t vid,uint16_t pid){
		int i ,found = 0;
		for(i=0;i<MAXNB_DEVICES;i++){
			if(_DevContext[i]){
				if(!_DevContext->isSameContext(vid,pid))
					return i;
			}
		}
		return -1;
	}
	int handleDevCMD(int idx,struct op_common* cmd,SOCKET fd){
		return _DevContext[idx]->handleDevCmd(cmd,fd);
	}
	int handleMsgDeviceEvent(int idx,CDeviceEvent* event){
		return _DevContext[idx]->handleMsgDeviceEvent(event);
	}
	//int IMPORTHandler(struct op_import_request* peerRequest);
private:
	friend class CControlBaseState;
	
private:
	CControlBaseState*	 _controlState;
	CDeviceStateContext* _DevContext[MAXNB_DEVICES];
	SOCKET _ControlSock;
	SOCKET _DataSock;
	char   peerAddr[];
	char   peerPortBase[];
};


class CCcomWaitingState:public CControlBaseState{
private:
	
	static CCcomWaitingState* _instance;
	CCcomWaitingState(){
		_instance = NULL;
	}
public:
	static CControlBaseState* getInstance();
	void recvConnectData(CControlChannelContext* context,SOCKET fd);
	

};

class CCcomReadyState:public CControlBaseState{
public:
	static CControlBaseState* getInstance();
	void recvConnectData(CControlChannelContext* context,SOCKET fd);
	void recvMsgDeviceEvent(CControlChannelContext* context,CDeviceEvent* evt);
private:
	CCcomReadyState(){
		_instance = NULL;
	}
	static CCcomReadyState* _instance;
};


class CCcomDead:public CControlBaseState{
public:
	static CControlBaseState* getInstance();
	void recvConnectData(CControlChannelContext* context,SOCKET fd){
		;
	}
	void recvMsgDeviceEvent(){};
private:
	CCcomDead(){
		_instance = NULL;
	}
	static CCcomDead* _instance;
};
/**********************************************************
*
*		Data Com State and Data Com Context
*
***********************************************************/


/*

CDcomReady
CDcomRunning
*/
class CDeviceStateContext;
class CDevBaseState{
public:
	virtual int handleDevCmd(CDeviceStateContext* ptContext,struct op_common* cmd,SOCKET fd) {return 0};
	virtual int handleMngEvent() {return 0};
	virtual int handleMsgDeviceEvent(CDeviceStateContext* ptContext,CDeviceEvent* event){return 0};
	//vitual void recvUsrCMD() = 0;
};

class CDeviceStateContext{
public:
	CDeviceStateContext(CControlChannelContext* ptCntrlCntx,uint32_t magic);
	int	createDataCom(char *hostname, char *port);
	void closeDataCom(){
		closesocket(_DataSock);
	}
	void poseContext(uint16_t newVid,uint16_t newPid,uint32_t magic){
		setDeviceId(newVid,newPid);
		_magicID =  magic;
		//_controlContext = context;
	}
	void setDeviceId(uint16_t newVid,uint16_t newPid);
	void setBusId(char* newBusid){
		strncpy(_busid,newBusid,SYSFS_BUS_ID_SIZE);
	}
	void resetContext();
	int isSameContext(int magic){
		return ((_used)&&(magic)&&(magic == _magicID));
	}
	int isSameContext(uint16_t vid,uint16_t pid){
		return ((_used)&&(magic)&&(vid == _vid)&&(pid == _pid));
	}
	int isEmptyDevContext();
	int handleDevCmd(struct op_common* cmd,SOCKET fd) {
			return _devState->handleDevCmd(this,cmd,fd);

	}
	int handleMngEvent() {};
	int handleMsgDeviceEvent(CDeviceEvent* event){
		return 	_devState->handleMsgDeviceEvent(this,event);
	};
private:
	int		 _used;
	uint16_t _vid;
	uint16_t _pid;
	char _busid[SYSFS_BUS_ID_SIZE];
	uint32_t _magicID;
	CDevBaseState* _devState;
	CControlChannelContext* _controlContext;
	SOCKET _DataSock;
}
/*class CDcomInit:public CDevBaseState{
public:
	static CDevBaseState* getInstance();
	int handleDevCmd(CDeviceStateContext* ptContext,struct op_common* cmd,SOCKET fd);
	int handleMsgDeviceEvent(CControlChannelContext* context,CDeviceEvent* evt);
private:
	CDcomInit(){
		_instance = NULL;
	}
	static CDcomInit* _instance;
};
*/
class CDcomReady:public CDevBaseState{
public:
	static CDevBaseState* getInstance();
	int handleDevCmd(CDeviceStateContext* ptContext,struct op_common* cmd,SOCKET fd);
	int handleMsgDeviceEvent(CControlChannelContext* context,CDeviceEvent* evt);
private:
	CDcomReady(){
		_instance = NULL;
	}
	static CDcomReady* _instance;
};



class CDcomRunning:public CDevBaseState{
public:
	static CDevBaseState* getInstance();
	int handleDevCmd(CDeviceStateContext* ptContext,struct op_common* cmd,SOCKET fd){
		;
	}
	int handleMsgDeviceEvent(CControlChannelContext* context,CDeviceEvent* evt);
private:
	CDcomRunning(){
		_instance = NULL;
	}
	static CDcomRunning* _instance;
};


#endif