#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "command.h"
#include "network.h"
#include "common.h"
#include "debug.h"
#include "statemachine.h"
CControlChannelContext::CControlChannelContext(SOCKET sockfd,char* IPAddr,char* PortBase){
		int i;
		_controlState = CCcomWaitingState::getInstance();
		_ControlSock = sockfd;
		strncpy(peerAddr,IPAddr, );
		strncpy(peerPortBase,PortBase, );
		for(i=0;i<MAXNB_DEVICES;i++){
			_DevContext[i] = NULL;
		}
}
int	CControlChannelContext::contect2DataCom(int idx){
		_DataSock = tcp_connect(peerAddr,peerPortBase+idx);
		if(_DataSock == -1){
			err("tcp_connect data connection fail\n");
			return -1;
		}
		return 0;
}
void CControlChannelContext::recvConnectData(){
		_controlState->recvConnectData(this,_ControlSock);
	}
void CControlChannelContext::recvMngEvent(){
		_controlState->recvMngEvent();
	}
void CControlChannelContext::recvMsgDeviceEvent(CDeviceEvent* evt){
		_controlState->recvMsgDeviceEvent(evt);
}
int CControlChannelContext::sendData2ControlCom(void* buf,int bufLen){
	return send_msg(_ControlSock,(char*)buf,bufLen);
}
int CControlChannelContext::recvDataFromControlCom(void*buf,int rdLen){
	return recv_msg(_ControlSock,(char*)buf,rdLen);
}

int CControlChannelContext::sendCMD2ControlCom( uint16_t cmdtype,int status ,uint32 magicID)
{
/*	struct op_common reqCMD;
	memset(&reqCMD,0,sizeof(struct op_common));
	reqCMD.version = gClientVersion;
	reqCMD.code = cmdtype;
	reqCMD.status = status;
	op_common_correct_endian(&reqCMD,1);*/
	if(sendCMD(cmdtype,status,magicID)<0)
	{
		err("\n");
		return -1;
	}
	return 0;
}

int CControlChannelContext::recvCMDFromControlCom( struct op_common* ptPeerCMD )
{
	struct op_import_request import_req;
	char *buf = (char*)ptPeerCMD;

	int rdlen = this->recvDataFromControlCom((void*)buf,sizeof(struct op_common));
	if(rdlen<0){
		err("recv_msg fail\n");
		return -1;
	}
	op_common_correct_endian(ptPeerCMD,0);
	return 0;
}

//int CControlChannelContext::IMPORTHandler( struct op_import_request* peerRequest )
{
	return IMPORTHandler(_ControlSock,_DataSock,peerRequest);
}

/********************************************************************************
*
*			CCcomWaitingState   
*						--OP_REQ_CONNECT_ON->CCcomReadyState	
*
*********************************************************************************/
CControlBaseState* CCcomWaitingState::getInstance(){
		if(_instance == NULL){
			_instance = new CCcomWaitingState;
		}
		return dynamic_cast<CControlBaseState*>(_instance);
	}
void CCcomWaitingState::recvConnectData(CControlChannelContext* context,SOCKET fd){
		struct op_common peerCMD;
		
		int ret = context->recvCMDFromControlCom(&peerCMD);
		if(ret<0){
			err("recv_msg fail\n");
			context->closeControlCom();
			context->changeState(CCcomDead::getInstance());
			return;
			
		}
		if(peerCMD.code == OP_REQ_CONNECT_ON){
				
			notice("State  CCcomWaitingState recv OP_REQ_CONNECT_ON will be CCcomReadyState! \n");
			context->changeState(CCcomReadyState::getInstance());
					
				
		}else{
			err("State CCcomWaitingState,recv CMD %d\n",peerCMD.code);
		}

}
/********************************************************************************
*
*			CCcomReadyState   
*						--OP_REQ_CONNECT_OFF-->CCcomWaitingState
*						--OP_REQ_DEVICE_READY-->CDevReady
*						-- send fail---->CCcomDead
*********************************************************************************/
CControlBaseState* CCcomReadyState::getInstance(){
		if(_instance == NULL){
			_instance = new CCcomReadyState;
		}
		return dynamic_cast<CControlBaseState*>(_instance);
	}
void CCcomReadyState::recvConnectData(CControlChannelContext* context,SOCKET fd){
		struct op_common peerCMD;
		int devContextIdx = -1;
		//char *buf = (char *)&peerCMD;
		int ret = context->recvCMDFromControlCom(&peerCMD);
		if(ret<0){
			context->closeControlCom();
			err("recv_msg fail\n");
			context->changeState(CCcomDead::getInstance());
			return;
		}

		switch(peerCMD.code){
			case OP_REQ_DEVLIST:
					/*TODO*/
					break;
			case OP_REQ_CONNECT_OFF:
				notice("State  CCcomReadyState recv OP_REQ_CONNECT_OFF will be CCcomWaitingState! \n");
				context->changeState(CCcomWaitingState::getInstance());
			default:
				if(peerCMD.magic){
					devContextIdx = context->findDevContextThrMagic();
					if(devContextIdx == -1){
						err("peerCMD.code %d  peerCMD.magic %d fail to find devContext\n",peerCMD.code,peerCMD.magic);
						break;
					}
					
					ret = context->handleDevCmd(devContextIdx,&peerCMD,fd);
					if(ret<0){
						err("handleDevCmd fail close control socket");
						context->closeControlCom();
						context->changeState(CCcomDead::getInstance());
					}


				}else
					err("peerCMD.code %d  peerCMD.magic 0 will not match device context\n",peerCMD.code);
				break;
		}
		
}
static int gDevIDSouce = 1;
static uint32_t genMagicId(){
	gDevIDSouce++;
	return (uint32_t) (gDevIDSouce);
}
void CCcomReadyState::recvMsgDeviceEvent(CControlChannelContext* context,CDeviceEvent* evt){
		//struct op_common cmd;
		int ret = 0;
		int fail = 0;
		uint32_t magic = 0;
		int	devCntxIdx = -1;
		
		if(evt->DeviceEvent == DEVICEEVENT_DEVICE_ATTACH && evt->len == sizeof(struct op_deviceattach_request) && evt->opague != NULL){
		
			struct op_deviceattach_request*	ptDevID; 
			ptDevID = (struct op_deviceattach_request*)evt->len;
			magic = genMagicId();
			/**/
			devCntxIdx = context->findEmptyDevContext();
			if(devCntxIdx == -1){
				err("too many usb device redirected!");
				return;
			}
			//ptDevID = (struct op_deviceattach_request*)evt->len;
			do{
				ret = context->sendCMD2ControlCom(OP_REQ_DEVICE_ATTACH,ST_OK,magic);
				if(ret<0){
					err("send_msg struct op_common fail\n");
					fail = 1;
					break;
				}
				
				ret = context->sendData2ControlCom(evt->opague,evt->len);
				if(ret<0){
					err("send_msg  SYSFS_BUS_ID fail\n");
					fail = 1;
					//context->resetDevContext(devCntxIdx);
					break;
				}

			}while(0);

			if(fail){
				notice("State  CCcomReadyState send fail Will be CCcomDead!\n");
				context->closeControlCom();
				context->changeState(CCcomDead::getInstance());
			}else{
			/*send something*/
				notice("State  CCcomReadyState recv Device ATTACH pose devcontext %d magic %d! \n",devCntxIdx,magic);
				context->poseDevContext(ptDevID->vid,ptDevID->pid,magic,devCntxIdx);
				//context->changeState(CDevReady::getInstance());
			}
	}else if(evt->DeviceEvent == DEVICEEVENT_DEVICE_DETTACH && evt->len == sizeof(struct op_devicedetach_request) && evt->opague != NULL){
		struct op_devicedetach_request *ptDevID;
		ptDevID = (struct op_devicedetach_request*)evt->len;
		devCntxIdx = context->findDevContextThrDevID(ptDevID->vid,ptDevID->pid);
		if(devCntxIdx<0){
			err("DEVICEEVENT_DEVICE_DETTACH vid %x pid %x not find \n",ptDevID->vid,ptDevID->pid);
		}else{
			notice("DEVICEEVENT_DEVICE_DETTACH vid %x pid %x \n",ptDevID->vid,ptDevID->pid);
			ret = context->handleMsgDeviceEvent(devCntxIdx,evt);
			context->resetDevContext(devCntxIdx);
			if(ret<0){
				context->closeControlCom();
				context->changeState(CCcomDead::getInstance());
			}
		}
	}
}
/********************************************************************************
*
*			CDeviceStateContext   
*						
*						
*						
*						
*********************************************************************************/
int CDeviceStateContext::createDataCom( char *hostname, char *port )
{
	_DataSock = tcp_connect(hostname,port);
	if(_DataSock == -1){
		err("tcp_connect data connection fail\n");
		return -1;
	}
	return 0;
}

CDeviceStateContext::CDeviceStateContext(CControlChannelContext* ptCntrlCntx,uint32_t magic) :_vid(0),
	_pid(0),
	_used(0),
	_controlContext(ptCntrlCntx),
	_magicID(magic)
{
	_devState = CDcomReady::getInstance();
	memset(_busid,0,SYSFS_BUS_ID_SIZE);
}

void CDeviceStateContext::resetContext()
{
	_vid = 0;
	_pid = 0;
	_used = 0;
	_magicID = 0;
	memset(_busid,0,SYSFS_BUS_ID_SIZE);
	_devState = CDcomReady::getInstance();
}

int CDeviceStateContext::isEmptyDevContext()
{
	if(_used)
		return 0;
	else
		return 1;
}

void CDeviceStateContext::setDeviceId( uint16_t newVid,uint16_t newPid )
{
	_vid = newVid;
	_pid = newPid;
	_used = 1;
}
/********************************************************************************
*
*			CDcomReady   
*						
*						--OP_REQ_IMPORT-->CDcomRunning
*						--DETTACH---->CDcomInit
*						
*********************************************************************************/
CDevBaseState* CDcomReady::getInstance(){
		if(_instance == NULL){
			_instance = new CDcomReady;
		}
		return dynamic_cast<CDevBaseState*>(_instance);
}
int CDcomReady::handleDevCmd(CDeviceStateContext* ptContext,struct op_common* cmd,SOCKET fd){
		
		struct op_import_request import_req;
		char *buf = NULL;
		int ret;
		int	devIndx;
	
		int rdlen = 0; 
		
		switch(cmd->code){
			case OP_REQ_IMPORT:
				notice("State CDevReady,recv CMD OP_REQ_IMPORT\n");
				/*recv cmd data*/
				buf = (char*)&import_req;
				rdlen = _controlContext->recvDataFromControlCom((void*)buf,sizeof(struct op_import_request));
				devIndx = htonl(import_req.portIndx);
				if(devIndx>MAXNB_REDIRECTUSBDEVICE){
					err("OP_REQ_IMPORT devIndx  %d exceed max\n",devIndx);
					break ;
				}
				/*create data connection*/
				ret = context->createDataCom(devIndx);
				if(ret<0){
					if(_controlContext->sendCMD2ControlCom(OP_REP_IMPORT,ST_NA)<0){
						err("Create Data connection fail send OP_REP_IMPORT ST_NA\n");
						//closesocket(sockfd);
						//context->closeControlCom();
						//context->changeState(CCcomDead::getInstance());
						
						return -1;
					}
				}
				/*do import cmd*/
				ret = IMPORTHandler(_controlContext->getControlSock(),_controlContext->getDataSock(),&import_req);
				if(ret<0){
					err("IMPORTHandler fail close all connection\n");
					context->closeDataCom();
					//context->closeControlCom();
					//context->changeState(CCcomDead::getInstance());
					return -1;
				}
				
				
				context->changeState(CDcomRunning::getInstance());
				break;
			default :
				notice("State CDevReady,recv CMD %d\n",cmd->code);
				/*TODO*/
				break;
		}
		return 0;
}
void CDcomReady::handleMsgDeviceEvent(CControlChannelContext* context,CDeviceEvent* evt){
		struct op_common cmd;
		int ret = 0;
		int fail = 0;
		switch(evt->DeviceEvent){
			case DEVICEEVENT_DEVICE_ATTACH:

				break;
			case DEVICEEVENT_DEVICE_DETTACH:
				//if(evt->len == SYSFS_BUS_ID_SIZE && evt->opague != NULL){
					{
						/*send something*/
						//_controlContext->sendCMD2ControlCom(OP_REQ_IMPORT,ST_NA);
						notice("State  CCcomReadyState recv Driver Ready will be CDevReady! \n");
						//context->closeDataCom();
						//_controlContext->changeState(CDcomInit::getInstance());
					}
				}
				break;
			default:

				break;
		}
		return 0;

}
/********************************************************************************
*
*			CDcomRunning   
*
*						-- DEVICEEVENT_DEVICE_DETTACH---->CDcomReady
*********************************************************************************/
CDevBaseState* CDcomRunning::getInstance(){
		if(_instance == NULL){
			_instance = new CDcomRunning;
		}
		return dynamic_cast<CDevBaseState*>(_instance);
	}
void CDcomRunning::handleMsgDeviceEvent(CControlChannelContext* context,CDeviceEvent* evt){
		//struct op_common cmd;
		int ret = 0;
		int fail = 0;
		switch(evt->DeviceEvent){
			case DEVICEEVENT_DEVICE_ATTACH:
				
				break;
			case DEVICEEVENT_DEVICE_DETTACH:
				if(evt->len == sizeof(struct op_devicedetach_request) && evt->opague != NULL){
					do{
							
						ret = context->sendCMD2ControlCom(OP_DEVICE_DETACH,ST_OK);
						if(ret<0){
							err("send_msg struct op_common fail\n");
							fail = 1;
							break;
						}
						ret = context->sendData2ControlCom(evt->opague,evt->len);
						if(ret<0){
							err("send_msg  SYSFS_BUS_ID fail\n");
							fail = 1;
							break;
						}
					}while(0);

					if(fail){
						notice("State  CCcomReadyState send fail Will be CCcomDead!\n");
						//context->closeControlCom();
						//context->changeState(CCcomDead::getInstance());
						return -1;
					}else{
						/*send something*/
						notice("State  CCcomReadyState recv Driver Ready will be CDevReady! \n");
						//context->closeDataCom();
						context->changeState(CDcomReady::getInstance());
					}
					/*close data connection*/
					context->closeDataCom();
					return 0;
				}
				return -1;
			
			default:

				break;
		}
		return 0;

}

CControlBaseState* CCcomDead::getInstance(){
	if(_instance == NULL){
		_instance = new CCcomDead;
	}
	return dynamic_cast<CControlBaseState*>(_instance);
}


static int sendCMD(SOCKET sockfd,uint16_t cmdtype,int status,uint32_t magic){
	struct op_common reqCMD;
	memset(&reqCMD,0,sizeof(struct op_common));
	reqCMD.version = gClientVersion;
	reqCMD.code = cmdtype;
	reqCMD.status = status;
	reqCMD.magic = magic;
	//CMD_h2n(&ackCMD);
	op_common_correct_endian(&reqCMD,1);
	if(send_msg(sockfd,(char *)&reqCMD,sizeof(struct op_common))<0)
	{
			err("\n");
			return -1;
	}
	return 0;
}
static struct usbdevpriv* findSameBusIdDevTab(char* busid){
	int i;
	int found = 0;
	struct usbdevpriv	*pdev = NULL;
	struct usbdevInfo	*pdevInfo  = NULL;
	if(busid == NULL){
		return NULL;
	}
	
	/*find empty usbdevice table entry*/
	for(i=0,pdev=&gDevTab[0];i<DEVICES_MAXNB;i++,pdev++){
		if(!pdev->used)
			continue;
		pdevInfo = &(pdev->devInfo);
		/*check if devInfo entry is valid, 
		only the correct devEntry will be send*/
		if( pdevInfo->interfacenb&&!pdevInfo->uinfs){
				err("\n");
				continue;
		}
		if(!strncmp(pdevInfo->udev.busid,peerRequest->busid,SYSFS_BUS_ID_SIZE)){
			monitor("import device found !\n");	
			found = 1;
			break;

		}
	}
	if(found != 0)
		return pdev;
	else
		return NULL;

}
static int sendImportReply(SOCKET sockfd,struct op_import_request* peerRequest){

	struct usbdevpriv	*pdev = NULL;
	struct usbdevInfo	*pdevInfo  = NULL;
	struct usb_device_info		pdu_devs;
	pdev = findSameBusIdDevTab(peerRequest->busid);
   /*no-found, or found but relinked :will cause a ST_NA reply*/
	if(pdev == NULL|| pdev->linked){
		monitor("import device no found !\n");	
		monitor("send OP_REP_IMPORT with ST_NA !\n");
		if(sendCMD(OP_REP_IMPORT,ST_NA)<0){
			err("\n");
			//return -1;
		}
		//closesocket(sockfd);
		/*return -1 will close this cmd connection*/
		return -1;
	}
	
		/*found*/
	monitor("send OP_REP_IMPORT with ST_OK !\n");
	pdevInfo = &(pdev->devInfo);
	if(sendCMD(OP_REP_IMPORT,ST_OK)<0){
		err("\n");
		//closesocket(sockfd);
		return -1;
	}
	/*send CMD Data*/
	monitor("send usb_device_info !\n");
	memcpy(&pdu_devs,&(pdevInfo->udev),sizeof(struct usb_device_info));
	usb_device_info_correct_endian(&pdu_devs,0);
	if(send_msg(sockfd,(char *)&pdu_devs,sizeof(struct usb_device_info))<0){
		err("\n");
		//closesocket(sockfd);
		return -1;
	}
	return 0;
}

static int IMPORTHandler(SOCKET controlSock,SOCKET dataSock,struct op_import_request* peerRequest){
	int result,i;
	int found = 0;
	struct usbdevpriv	*pdev;
	struct usbdevInfo	*pdevInfo;
	struct usbip_device*	udev;
	result = sendImportReply(controlSock£¬peerRequest);
	if(result<0)
		return -1;
	pdev = findSameBusIdDevTab(peerRequest->busid);
	if(pdev ==  NULL){
		err("%s %d fail to find %s \n",__FUNCTION__,__LINE__,peerRequest->busid);
		return -1;
	}

	pdevInfo = &(pdev->devInfo);
	/***********************************************************
	* find a empty usbip_dev entry.
	*****************************************************/
	for(found=0,i=0,udev=&gUdev[0];i<DEVICES_MAXNB;i++,udev++){
		if(udev->status!=SDEV_ST_EMPTY)
			continue;
		found = 1;
		break;
	}
	
	if(found){
		
	//	udev->connection.sockfd = sockfd;
		udev->priv= pdev;
		pdev->ptOwner =(void*) udev;
		udev->devid = getDevId(&pdevInfo->udev);
		if(udev->devid<=0){
				err("\n");
				//closesocket(sockfd);
				return -1;
		}
		monitor("Imported device get devid %x !\n",udev->devid );	
		//udev->relpyListHead =	NULL;
		//udev->requestListHead	= NULL;
		monitor("we call device libusb_open !\n");

		(pdev->LibObj)->ptOwner =(void*) pdev;
		if(usb_Instance_init((pdev->LibObj))<0){
			err("\n");
			//closesocket(sockfd);
			return -1;
		}
		monitor("will create tx thread and rx thread!\n");	
		
		udev->unplug_event = CreateEvent(NULL,FALSE,FALSE,NULL);
		if(udev->unplug_event==NULL){
			err("CreateEvent unplug_event fails\n");
			usb_Instance_deinit((pdev->LibObj));
			//closesocket(sockfd);
			return -1;	
		}
		monitor("handle udev %p unplug_event %x\n",udev,(unsigned int)udev->unplug_event);
		
		/*create data dispatcher and data connection service*/
		if(InitDispatcher(udev)<0){
			err("\n");
	
			CloseHandle(udev->unplug_event);
	
			usb_Instance_deinit((pdev->LibObj));
			//closesocket(sockfd);
			return -1;
		}
		RunDispatcher(udev);
		if(InitDataConnAdapter(udev,sockfd)<0){
			err("\n");
			deInitDispatcher(udev);
			CloseHandle(udev->unplug_event);
			usb_Instance_deinit((pdev->LibObj));
			//closesocket(sockfd);
			return -1;	
		}
		
		monitor("will create urbhandle thread!\n");	
		udev->status = SDEV_ST_USED;
		udev->monitor.exitModule = MODULE_NONE;
		udev->monitor.exitEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		if(udev->monitor.exitEvent==NULL){
			err("CreateEvent fails\n");
			deInitDataConnAdapter(udev);
			deInitDispatcher(udev);
			
			CloseHandle(udev->unplug_event);
			
			
			usb_Instance_deinit((pdev->LibObj));
			udev->status = SDEV_ST_EMPTY;
			return -1;
		}
		if(createMonitorService(udev)<0){
			err("createMonitorService fails\n");
			CloseHandle(udev->monitor.exitEvent);
			deInitDataConnAdapter(udev);
			deInitDispatcher(udev);
			
			CloseHandle(udev->unplug_event);
			
			
			usb_Instance_deinit((pdev->LibObj));
			udev->status = SDEV_ST_EMPTY;
			return -1;

		}
		pdev->linked = 1;
	}
	else{
		err("\n");
		//closesocket(sockfd);
		return -1;
	}
	
	return 0;
}


