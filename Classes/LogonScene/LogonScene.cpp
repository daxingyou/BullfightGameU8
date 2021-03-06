//
//  LogonScene.cpp
//  
//
//  Created by on 15/3/16.
//
//

#include "LogonScene.h"
#include "../Tools/Tools.h"
#include "../Tools/DataModel.h"
#include "../Tools/Statistics.h"
#include "../PopDialogBox/PopDialogBoxLoading.h"
#include "../PopDialogBox/PopDialogBoxOtherLoading.h"
#include "../PopDialogBox/PopDialogBoxLogonAccount.h"
#include "../PopDialogBox/PopDialogBoxTipInfo.h"
#include "../PopDialogBox/PopDialogBoxUpdateTipInfo.h"
#include "../Tools/StatisticsConfig.h"
#include "../Network/CMD_Server/PacketAide.h"
#include "../GameLobby/GameLobbyScene.h"
//#include "../Network/ListernerThread/LogonGameListerner.h"
//#include "../Network/ListernerThread/LobbyGameListerner.h"
#include "../Network/CMD_Server/Packet.h"
#include "../MTNotificationQueue/LobbyMsgHandler.h"
#include "../Network/MD5/MD5.h"
//#include "../Network/CMD_Server/cmd_ox.h"
#include "../Tools/BaseAttributes.h"
#include "../Platform/coPlatform.h"
#include "../extensions/spine/Json.h"
#include "../u8sdk/CU8sdkFunction.h"
LogonScene* LogonScene::pLScene=NULL;

#define kURLQQLogon(name)  CCString::createWithFormat("{\"act\":200 ,\"url\":\"http://www.qicainiu.net/QQLogin.aspx?%s\"}",(name))->getCString()
//#define  CCString::createWithFormat()
//#define kURLQQLogon(name)  CCString::createWithFormat("{\"act\":200 ,\"url\":\"http://121.40.31.203:9021/Test.aspx?%s\"}",(name))->getCString()

LogonScene::LogonScene()
	:eLogonType(LOGON_ACCOUNT)
	, isReadMessage(true)
	, eStatisticsType(STATISTICS_LOGON)
{


	readRMS();
	scheduleUpdate();

	/*char szJson[512] = { 0 };
	sprintf(szJson,
		"{\"act\":20,\"type\":%d,\"item\":%d,\"cost\":%d,\"amount\":%d,\"name\":\"%s\",\"uid\":\"%s\",\"order\":\"%s\"}",
		type, item, coPayGetCostAmount(item), coPayGetProductAmount(item), coPayGetProductName(item).c_str(),
		uid.c_str(), order.c_str());
	log("%s", szJson);*/
	/*const char* text = "{\"data\": [\"UIActivity.plist\",\"UIVip.plist\"]}";
	//const char* text = "{\"record\":{\"data\":20130101,\"rate\":23,\"buy\":1},\"record\":{\"data\":45,\"rate\":67,\"buy\":1},\"record\":{\"data\":45,\"rate\":76,\"buy\":10}}";
	Json *root = Json_create(text);

	Json* _date=Json_getItem(root, "data");
	if (_date->type == Json_Array)
	{
		for (int i = 0; i <Json_getSize(_date); i++)
		{
			Json *s=Json_getItemAt(_date,i);
			CCLOG("--------===================== %s<<%s>>",s->valuestring, __FUNCTION__);
		}
	}
	Json* _record = root->child;
	while (_record)
	{
		//Json* _date = Json_getItem(_record, "data");
		//if (_date->type == Json_String)
		{
		//	const char * date = _date->valuestring;
			//CCLOG("Date: %s", date);
		}
		
		Json* _rate = Json_getItem(_record, "rate");
		if (_rate->type == Json_Number)
		{
			int rate = _rate->valueint;
			CCLOG("Rate: %d", rate);
		}
		Json* _buy = Json_getItem(_record, "buy");
		if (_buy->type == Json_Number)
		{
			int buy = _buy->valueint;
			CCLOG("Buy: %d", buy);
		}
		_record = _record->next;
	}
	Json_dispose(root);*/



	//CCLOG("--------:%s <<%s>>", platformAction("{\"act\":100}").c_str(), __FUNCTION__);
	//初始化签到信息
	initSignInfo();
	//初始化更多帐号
	initMoreAccount();
#if(DEBUG_TEST==0||DEBUG_TEST==1)
	CCLabelTTF *label = CCLabelTTF::create(GAME_VERSION, "Marker Felt", 20);
	this->addChild(label, 2);
	label->setColor(ccc3(0, 0, 0));
	label->setPosition(ccp(0, DataModel::sharedDataModel()->deviceSize.height));
	label->setAnchorPoint(ccp(0, 1));
#endif

	setKeypadEnabled(true);//设置相应按键消息 
}
LogonScene::~LogonScene(){
	CCLOG("~ <<%s>>", __FUNCTION__);
	//TCPSocketControl::sharedTCPSocketControl()->removeTCPSocket(SOCKET_LOGON_GAME);
	//移除对象
	this->removeAllChildrenWithCleanup(true);
	//清理数据
	GUIReader::purge();
	CCArmatureDataManager::purge();
	//清理纹理
	CCSpriteFrameCache::sharedSpriteFrameCache()->removeUnusedSpriteFrames();
	CCTextureCache::sharedTextureCache()->removeUnusedTextures();
}
CCScene* LogonScene::scene()
{
    CCScene *scene = CCScene::create();
    LogonScene *layer = LogonScene::create();
    scene->addChild(layer);
    pLScene=layer;
    return scene;
}
CCScene* LogonScene::scene(updateInfo uInfo)
{
	CCScene *scene = CCScene::create();
	LogonScene *layer = LogonScene::create();
	scene->addChild(layer);
	layer->showUpdateContent(uInfo);
	pLScene = layer;
	return scene;
}
//读取配置文件
void LogonScene::readConfigData(){
	std::string path = CCFileUtils::sharedFileUtils()->getWritablePath();
	path += "updateInfo";
	{
		//读取本地文件
		unsigned long bufferSize = 0;
		unsigned char* sData = CCFileUtils::sharedFileUtils()->getFileData(path.c_str(), "r", &bufferSize);
		if (sData)
		{
			Json *root = Json_create(CCString::createWithFormat("%s", sData)->getCString());
			if (root)
			{
				DataModel::sharedDataModel()->urlLogon->removeAllObjects();
				Json* _date = Json_getItem(root, "updateConfig");
				Json* _strUrlLogonList = Json_getItem(_date, "url_logon_list");
				if (_strUrlLogonList&&_strUrlLogonList->type == Json_Array)
				{
					for (int i = 0; i < Json_getSize(_strUrlLogonList); i++)
					{
						Json *s = Json_getItemAt(_strUrlLogonList, i);
						Json *sAddr = Json_getItem(s, "logonAddr");
						//std::string strLogonAddr = sAddr->valuestring;
						CCString *strAddr = CCString::createWithFormat("%s", sAddr->valuestring);
						DataModel::sharedDataModel()->urlLogon->addObject(strAddr);
					}
				}
			}
			Json_dispose(root);
		}
	}
}
//显示更新
void LogonScene::showUpdateContent(updateInfo uInfo){
	if (uInfo.isShowUpdateTip)
	{
		PopDialogBoxUpdateTipInfo *pUTipInfo = PopDialogBoxUpdateTipInfo::create();
		this->addChild(pUTipInfo, 100);
		//pUTipInfo->setTipInfoContent(GBKToUTF8(uInfo.strUpdateContent.c_str()).c_str());
		//pUTipInfo->iUpdateType = uInfo.iUpdateType;
		pUTipInfo->setTipInfoData(uInfo);
		//CCLOG("%s <<%s>>", GBKToUTF8(uInfo.strUpdateContent.c_str()).c_str(), __FUNCTION__);
	}
}
void LogonScene::onEnter(){
	CCLayer::onEnter();
	readConfigData();//读取数据
	//添加背景
	CCSize deviceSize=DataModel::sharedDataModel()->deviceSize;
	CCSprite *spriteBg=CCSprite::create("res/logon.jpg");
	this->addChild(spriteBg);
	spriteBg->setPosition(ccp(deviceSize.width/2,deviceSize.height/2));
	float scale=deviceSize.height/spriteBg->getContentSize().height;
	spriteBg->setScaleY(scale);
	//牛动画
	CCArmatureDataManager::sharedArmatureDataManager()->addArmatureFileInfo(CCS_PATH_SCENE(AnimationLogonSprite.ExportJson));
	CCArmature *pAnimate = CCArmature::create("AnimationLogonSprite");
	pAnimate->setAnchorPoint(CCPointZero);
	pAnimate->setPosition(CCPointZero);
	pAnimate->getAnimation()->play("AnimationN");
	spriteBg->addChild(pAnimate);
	
	//标题动画
	CCArmatureDataManager::sharedArmatureDataManager()->addArmatureFileInfo(CCS_PATH_SCENE(AnimationMenuTitle.ExportJson));
	CCArmature *pAnimateTitle = CCArmature::create("AnimationMenuTitle");
	pAnimateTitle->setAnchorPoint(ccp(1,1));
	pAnimateTitle->setPosition(SCENE_SIZE);
	pAnimateTitle->getAnimation()->play("Animation1");
	pAnimateTitle->setScale(0.95);
	spriteBg->addChild(pAnimateTitle);

	//闪光
	CCArmatureDataManager::sharedArmatureDataManager()->addArmatureFileInfo(CCS_PATH_SCENE(AnimationLogonLight.ExportJson));
	CCArmature *pAnimateLight = CCArmature::create("AnimationLogonLight");
	pAnimateLight->setAnchorPoint(ccp(0.5, 0.5));
	pAnimateLight->setPosition(ccp(835,130));
	pAnimateLight->getAnimation()->play("AnimationLight");
	spriteBg->addChild(pAnimateLight);
	//////////////////////////////////////////////////////////////////////////
	//创建UI层
    m_pWidget = UILayer::create();
	this->addChild(m_pWidget);
	//加载UI层
	UILayout *pWidget = dynamic_cast<UILayout*>(GUIReader::shareReader()->widgetFromJsonFile(CCS_PATH_SCENE(UILogon.ExportJson)));
	m_pWidget->addWidget(pWidget);
	//绑定按键
	UIButton* button;
	for (int i = 0; i < 5; i++)
	{
		button  = static_cast<UIButton*>(m_pWidget->getWidgetByName(CCString::createWithFormat("ButtonLogon%d",i)->getCString()));
		button->addTouchEventListener(this, SEL_TouchEvent(&LogonScene::onMenuLogon));
		button->setVisible(false);
	}
	//分享
	UIButton* pBShare = static_cast<UIButton*>(m_pWidget->getWidgetByName("ButtonShare"));
	pBShare->addTouchEventListener(this, SEL_TouchEvent(&LogonScene::onMenuShare));
	pBShare->setEnabled(false);
	//logonGameByAccount();
	//默认登录
	//defaultLogon();
	if (isFirstLogon)
	{
		statisticsInstall();
	}

	std::string sPara = platformAction("{\"act\":603}").c_str();
	if (sPara.length()!=0)
	{
		DataModel::sharedDataModel()->sLogonPassword;
		//pwd  accou;
		//{"pwd":"2048c05c5606c25c0e2e7adc7b006c81", "account" : "nbaobama"}

	}
	CU8sdkFunction::GetInstance().OnU8sdkInit();
	//U8
	CU8sdkFunction::GetInstance().OnU8sdkLogin();
}
void LogonScene::onExit(){

	CCLayer::onExit();
}
//初始化签到信息
void LogonScene::initSignInfo(){
	std::string sSign = Tools::getStringByRMS(RMS_SIGN_RECORD);
	Json *root = Json_create(sSign.c_str());
	if (root)
	{
		Json* _date = Json_getItem(root, "signData");
		Json* _record = root->child;
		while (_record)
		{
			long userID=0;
			int day = 0;
			Json* _userID = Json_getItem(_record, "userId");
			if (_userID->type == Json_Number)
			{
				userID = _userID->valueint;
			}
			Json* _day = Json_getItem(_record, "signDay");
			if (_day->type == Json_Number)
			{
				day = _day->valueint;
			}
			DataModel::sharedDataModel()->mapSignRecord.insert(map<long, int>::value_type(userID, day));
			_record = _record->next;
		}
	}
	Json_dispose(root);
}
//初始化更多帐号
void LogonScene::initMoreAccount(){
	std::string sMore = Tools::getStringByRMS(RMS_MORE_ACCOUNT);
	Json *root = Json_create(sMore.c_str());
	if (root)
	{
		Json* _date = Json_getItem(root, "moreAccount");
		Json* _record = root->child;
		while (_record)
		{
			MoreAccount moreAccount;
			Json* _userID = Json_getItem(_record, "userId");
			if (_userID->type == Json_String)
			{
				moreAccount.userAccount = _userID->valuestring;
			}
			Json* _pwd = Json_getItem(_record, "userPwd");
			if (_pwd->type == Json_String)
			{
				moreAccount.userPwd = _pwd->valuestring;
			}
			DataModel::sharedDataModel()->vecMoreAccount.push_back(moreAccount);
			_record = _record->next;
		}
	}
	Json_dispose(root);
}
//默认登录
void LogonScene::defaultLogon(){
	switch (DataModel::sharedDataModel()->logonType)
	{
	case LOGON_ACCOUNT:
	{
		UIButton *pBLogon = static_cast<UIButton*>(m_pWidget->getWidgetByName(CCString::createWithFormat("ButtonLogon%d", 0)->getCString()));
		onMenuLogon(pBLogon,TOUCH_EVENT_ENDED);
	}
		break;
	case LOGON_QQ:
	{
		UIButton *pBLogon = static_cast<UIButton*>(m_pWidget->getWidgetByName(CCString::createWithFormat("ButtonLogon%d", 1)->getCString()));
		onMenuLogon(pBLogon, TOUCH_EVENT_ENDED);
	}
		break;
	default:
		break;
	}
}
void LogonScene::onMenuLogon(CCObject* pSender, TouchEventType type){
	switch (type)
	{
	case TOUCH_EVENT_ENDED:
		{
			UIButton *pBtn =(UIButton *)pSender;
			setLogonType((LogonType)pBtn->getTag());
			DataModel::isQuickUser = false;
			switch (pBtn->getTag())
			{
			case LOGON_ACCOUNT:
				{
					setStatisticsType(STATISTICS_LOGON);
					//logonGame();
					PopDialogBox *pLogon = PopDialogBoxLogonAccount::create();
					this->addChild(pLogon);
					DataModel::sharedDataModel()->logonType = LOGON_ACCOUNT;
				}
				break;
			case LOGON_REGISTER://注册
			{
				setStatisticsType(STATISTICS_REGISTER);
				PopDialogBoxRegistered *pRegistered = PopDialogBoxRegistered::create();
				this->addChild(pRegistered);
				pRegistered->setIPopAssistRegistered(this);
			}
				break;
			case LOGON_QQ:
			{
				setStatisticsType(STATISTICS_LOGON);
				DataModel::sharedDataModel()->logonType = LOGON_QQ;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
				PopDialogBoxTipInfo *tipInfo = PopDialogBoxTipInfo::create();
				this->addChild(tipInfo);
				tipInfo->setTipInfoContent(BaseAttributes::sharedAttributes()->sWaitCodeing.c_str());
#else
				m_pWidget->setTouchEnabled(false);
				std::string urlQQLogon="";
				urlQQLogon.append("sessionID=");
				urlQQLogon.append(DataModel::sharedDataModel()->sSessionID.c_str());
				urlQQLogon.append("&code=");
				std::string code = "server";
				code.append(DataModel::sharedDataModel()->sSessionID.c_str());
				code.append(DataModel::sharedDataModel()->sSessionVerion.c_str());
				code.append("lmyspread");

				MD5 md;
				code = md.GetMd5(code.c_str(), code.length());

				urlQQLogon.append(code.c_str());
				urlQQLogon.append("&machineCode=");
				urlQQLogon.append(Tools::getMachineID().c_str());
				CCLOG("%s <<%s>>", kURLQQLogon(urlQQLogon.c_str()), __FUNCTION__);
				platformAction(kURLQQLogon(urlQQLogon.c_str()));
#endif
                
			}
				break;
			case LOGON_QUICK://快速登录
				{
					setStatisticsType(STATISTICS_LOGON);
					DataModel::isQuickUser = true;
					DataModel::sharedDataModel()->logonType = LOGON_QUICK;
					quickLogon();
					/*PopDialogBoxTipInfo *tipInfo = PopDialogBoxTipInfo::create();
					this->addChild(tipInfo);
					tipInfo->setTipInfoContent(BaseAttributes::sharedAttributes()->sWaitCodeing.c_str());
					//TCPSocketControl::sharedTCPSocketControl()->removeTCPSocket(SOCKET_LOGON_GAME);
					*/
				}
				break;
			case LOGON_WEI_XIN:
			{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#else
				platformAction("{\"act\":910}").c_str();
#endif
			}
				break;
			default:
				break;
			}
			
		}
		break;
	default:
		break;
	}
}
//分享
void LogonScene::onMenuShare(CCObject* pSender, TouchEventType type){
	if (type==TOUCH_EVENT_ENDED)
	{
		platformAction("{\"act\":901}").c_str();
		
	}
}
//快速登录
void LogonScene::quickLogon(){
	connectServer();
	std::string uuid=platformAction("{\"act\":100}").c_str();
	CCLOG("uuid:%s <<%s>>",uuid.c_str(), __FUNCTION__);
	CMD_MB_Quick_Logon mbQuickLogon;
	mbQuickLogon.dwOpTerminal = 2;

	mbQuickLogon.dwSessionID = (int)atol(DataModel::sharedDataModel()->sSessionID.c_str());

	std::string code = "server";
	code.append(DataModel::sharedDataModel()->sSessionID.c_str());
	code.append(DataModel::sharedDataModel()->sSessionVerion.c_str());
	code.append("lmyspread");
	MD5 md;
	code = md.GetMd5(code.c_str(), code.length());

	strcpy(mbQuickLogon.szStatisCode,code.c_str());

	strcpy(mbQuickLogon.szMachineID, Tools::getMachineID().c_str());//机器码
	gameSocket.SendData(MDM_MB_LOGON, SUB_MB_QUICK_LOGIN, &mbQuickLogon, sizeof(mbQuickLogon));
	/*switch (DataModel::sharedDataModel()->logonType)
	{
	case LOGON_ACCOUNT:
	{
		if (strcmp(DataModel::sharedDataModel()->sLogonAccount.c_str(), "") == 0)
		{
			PopDialogBoxRegistered *pRegistered = PopDialogBoxRegistered::create();
			this->addChild(pRegistered);
			pRegistered->setIPopAssistRegistered(this);
		}
		else
		{
			logonGameByAccount(0);
		}
	}
		break;
	case LOGON_QQ:
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		PopDialogBoxTipInfo *tipInfo = PopDialogBoxTipInfo::create();
		this->addChild(tipInfo);
		tipInfo->setTipInfoContent(BaseAttributes::sharedAttributes()->sWaitCodeing.c_str());
#else
		m_pWidget->setTouchEnabled(false);
		platformAction(urlQQLogon).c_str();
#endif
	}
		break;
	default:
		if (strcmp(DataModel::sharedDataModel()->sLogonAccount.c_str(), "") == 0)
		{
			PopDialogBoxRegistered *pRegistered = PopDialogBoxRegistered::create();
			this->addChild(pRegistered);
			pRegistered->setIPopAssistRegistered(this);
		}
		break;
	}*/
}
void LogonScene::update(float delta){
	/*if (isReadMessage)
	{
		MessageQueue::update(delta);
	}*/
	gameSocket.updateSocketData(delta);
}
//连接服务器
void LogonScene::connectServer(){
	PopDialogBox *box = PopDialogBoxOtherLoading::create();
	this->addChild(box, 10, TAG_LOADING);
	//box->setSocketName(SOCKET_LOGON_GAME);

	if (gameSocket.getSocketState() != CGameSocket::SOCKET_STATE_CONNECT_SUCCESS)
	{
		if (DataModel::sharedDataModel()->ipaddr.length()==0)
		{
			std::string sLogonAddr = DataModel::sharedDataModel()->getLogonAddr();
			struct hostent* hostInfo = gethostbyname(sLogonAddr.c_str());
			if (hostInfo)
			{
				DataModel::sharedDataModel()->ipaddr = inet_ntoa(*(struct in_addr *)*hostInfo->h_addr_list);
			}
		}
		CCLOG("ip:%s <<%s>>", DataModel::sharedDataModel()->ipaddr.c_str(), __FUNCTION__);
		gameSocket.Create(DataModel::sharedDataModel()->ipaddr.c_str(), PORT_LOGON);
		gameSocket.setIGameSocket(this);
	}
	/*PopDialogBox *box = PopDialogBoxLoading::create();
	this->addChild(box, 10, TAG_LOADING);
	box->setSocketName(SOCKET_LOGON_GAME);

	TCPSocketControl::sharedTCPSocketControl()->removeTCPSocket(SOCKET_LOGON_GAME);
	TCPSocket *tcp = getSocket();
	if (tcp)
	{
		tcp->createSocket(GAME_IP, PORT_LOGON, new LogonGameListerner());
	}*/
}
//登录游戏////////////////////////////////////////////////////////////////////////
void LogonScene::logonGameByAccount(float dt){
	connectServer();
	
	//if (isCreate)
	{
		CMD_MB_LogonAccounts logonAccounts;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
		logonAccounts.cbDeviceType = 2;
#elif(CC_TARGET_PLATFORM==CC_PLATFORM_IOS)
		logonAccounts.cbDeviceType = 3;
#else
		logonAccounts.cbDeviceType = 2;
#endif

		logonAccounts.dwPlazaVersion = VERSION_PLAZA;
		strcpy(logonAccounts.szAccounts, DataModel::sharedDataModel()->sLogonAccount.c_str());
		//strcpy(logonAccounts.szAccounts,"zhangh189");
		strcpy(logonAccounts.szMachineID, Tools::getMachineID().c_str());
		strcpy(logonAccounts.szMobilePhone, "");
		strcpy(logonAccounts.szPassPortID, "");
		strcpy(logonAccounts.szPhoneVerifyID, "1");
		/*//游戏标识
		for (int i = 0; i < 10; i++)
		{
			logonAccounts.wModuleID[i] = 0;
		}
		//logonAccounts.wModuleID[0] = 210; //210为二人牛牛标示
		logonAccounts.wModuleID[0] = 30; //30为百人牛牛标示
		logonAccounts.wModuleID[1] = 130; //1002为通比牛牛标示
		logonAccounts.wModuleID[2] = 430; //六人换牌
		*/
		resetKindID(logonAccounts);

		MD5 m;
		//std::string passWord = GBKToUTF8(DataModel::sharedDataModel()->sLogonPassword.c_str());
		//m.ComputMd5(passWord.c_str(), passWord.length());
		m.ComputMd5(DataModel::sharedDataModel()->sLogonPassword.c_str(), DataModel::sharedDataModel()->sLogonPassword.length());
		std::string md5PassWord = m.GetMd5();
		strcpy(logonAccounts.szPassword, md5PassWord.c_str());
		CCLOG("pwd:%s <<%s>>",logonAccounts.szPassword, __FUNCTION__);
		//09-08 17:26:48.599: D/cocos2d-x debug info(13570): pwd:a796dd352fd25ae2d739719e950ded1d <<logonGameByAccount>>

		gameSocket.SendData(MDM_MB_LOGON, SUB_MB_LOGON_ACCOUNTS, &logonAccounts, sizeof(logonAccounts));
		
	}
}
//读取网络消息回调
void LogonScene::onEventReadMessage(WORD wMainCmdID,WORD wSubCmdID,void * pDataBuffer, unsigned short wDataSize){
	switch (wMainCmdID)
	{
		case MDM_MB_SOCKET://socket连接成功
			//onEventConnect(wSubCmdID,pDataBuffer,wDataSize);
			break;
		case MDM_MB_LOGON://登录 
			onEventLogon(wSubCmdID,pDataBuffer,wDataSize);
			break;
		case MDM_MB_SERVER_LIST://列表信息
			onEventServerList(wSubCmdID,pDataBuffer,wDataSize);
			break;
	default:
		CCLOG("other:%d   %d<<%s>>",wMainCmdID,wSubCmdID,__FUNCTION__);
		break;
	}
}
//重设要获取的房间ID
void LogonScene::resetKindID(CMD_MB_LogonAccounts &logonAccounts){
	//游戏标识
	for (int i = 0; i < 10; i++)
	{
		logonAccounts.wModuleID[i] = 0;
	}
	logonAccounts.wModuleID[0] = 210; //210为二人牛牛标示
	logonAccounts.wModuleID[1] = 30; //30为百人牛牛标示
	logonAccounts.wModuleID[2] = 130; //1002为通比牛牛标示
	logonAccounts.wModuleID[3] = 430; //六人换牌
}
//登录游戏
void LogonScene::otherLogonGamePara(){
	CMD_MB_LogonAccounts logonAccounts;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	logonAccounts.cbDeviceType = 2;
#elif(CC_TARGET_PLATFORM==CC_PLATFORM_IOS)
	logonAccounts.cbDeviceType = 3;
#else
	logonAccounts.cbDeviceType = 2;
#endif
	
	logonAccounts.dwPlazaVersion = VERSION_PLAZA;
	strcpy(logonAccounts.szAccounts, DataModel::sharedDataModel()->sLogonAccount.c_str());
	//strcpy(logonAccounts.szAccounts,"zhangh189");
	strcpy(logonAccounts.szMachineID, Tools::getMachineID().c_str());
	strcpy(logonAccounts.szMobilePhone, "");
	strcpy(logonAccounts.szPassPortID, "12");
	strcpy(logonAccounts.szPhoneVerifyID, "1");
	resetKindID(logonAccounts);
	
	strcpy(logonAccounts.szPassword, DataModel::sharedDataModel()->sLogonPassword.c_str());
	gameSocket.SendData(MDM_MB_LOGON, SUB_MB_LOGON_ACCOUNTS, &logonAccounts, sizeof(logonAccounts));
	
}
//注册游戏
void LogonScene::registeredGame(){
	CMD_MB_RegisterAccounts registeredAccount;
	
	//游戏标识
	for (int i = 0; i < 10; i++)
	{
		registeredAccount.wModuleID[i] = 0;
	}
	//registeredAccount.wModuleID[0] = 210; //210为二人牛牛标示
	registeredAccount.wModuleID[0] = 30; //30为百人牛牛标示
	registeredAccount.wModuleID[1] = 130; //1002为通比牛牛标示
	registeredAccount.wModuleID[2] = 430; //六人换牌

	registeredAccount.dwPlazaVersion = VERSION_PLAZA;//广场版本

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	registeredAccount.cbDeviceType = 2;
#elif(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	registeredAccount.cbDeviceType = 3;
#else
	registeredAccount.cbDeviceType = 2;
#endif


	
	MD5 m;
	m.ComputMd5(sRegisterPasswrod.c_str(), sRegisterPasswrod.length());
	std::string md5PassWord = m.GetMd5();
	strcpy(registeredAccount.szLogonPass, md5PassWord.c_str());//登录密码
	strcpy(registeredAccount.szInsurePass, md5PassWord.c_str());//银行密码
	CCLOG("registeredPassword:%s <<%s>>",registeredAccount.szLogonPass, __FUNCTION__);

	registeredAccount.wFaceID = 1;//头像标识
	registeredAccount.cbGender = 1;//用户性别
	strcpy(registeredAccount.szAccounts, sRegisterAccount.c_str());//登录帐号
	strcpy(registeredAccount.szNickName, UTF8ToGBK(sRegisterNickname.c_str()).c_str());//用户昵称
	//strcpy(registeredAccount.szSpreader, "");//推荐帐号
	//strcpy(registeredAccount.szPassPortID, "");//证件号码
	//strcpy(registeredAccount.szCompellation, "");//真实名字

	//registeredAccount.cbValidateFlags = 1;//校验标识		   
	strcpy(registeredAccount.szMachineID, Tools::getMachineID().c_str());//机器序列
	strcpy(registeredAccount.szMobilePhone, "");//电话号码


	gameSocket.SendData(MDM_MB_LOGON, SUB_MB_REGISTER_ACCOUNTS, &registeredAccount, sizeof(CMD_GP_RegisterAccounts));
}
/*//连接成功
void LogonScene::onEventConnect(WORD wSubCmdID,void * pDataBuffer, unsigned short wDataSize){
	switch (wSubCmdID)
	{
	case SUB_GP_SOCKET_OPEN:
		{
			switch (eLogonType)
			{
			case LogonScene::LOGON_ACCOUNT:
			{
				logonGame();
			}
				break;
			case LogonScene::LOGON_QQ:
            {
                logonGame();
            }
				break;
			case LogonScene::LOGON_REGISTER:
			{
				registeredGame();
			}
				break;
			case LogonScene::LOGON_QUICK:
				break;
			default:
				break;
			}
			
		}
		break;
	default:
		break;
	}
}*/
//发送数据统计
void LogonScene::sendLoginStatistics(){
	switch (eStatisticsType)
	{
	case LogonScene::STATISTICS_REGISTER:
	{
		//发送注册统计数据
		Statistics *pStatistice = Statistics::create();
		pStatistice->sendStatisticsData(Statistics::S_REGISTER);
	}
	break;
	case LogonScene::STATISTICS_LOGON:
	{
		//发送登录统计数据
		Statistics *pStatistice = Statistics::create();
		pStatistice->sendStatisticsData(Statistics::S_ACCOUNT_LOGON);
	}
	break;
	case LogonScene::STATISTICS_QQ:
		break;
	default:
		break;
	}
}
//解析附加数据
void LogonScene::parsingAdditionalData(CMD_MB_LogonSuccess *ls, void * pDataBuffer, unsigned short wDataSize){
	int wPacketSize = 0;
	BYTE cbDataBuffer[SOCKET_TCP_PACKET + sizeof(TCP_Head)];
	CopyMemory(cbDataBuffer, pDataBuffer, wDataSize);
	wPacketSize += sizeof(CMD_MB_LogonSuccess);
	while (true)
	{
		void * pDataBuffer1 = cbDataBuffer + wPacketSize;
		tagDataDescribe *DataDescribe = (tagDataDescribe*)pDataBuffer1;
		wPacketSize += sizeof(tagDataDescribe);
		switch (DataDescribe->wDataDescribe)
		{
		case DTP_GP_GET_LABA_COUNT://喇叭个数
		{
			DTP_GP_GetLabaCount  pHornNum;
			CopyMemory(&pHornNum, cbDataBuffer + wPacketSize, DataDescribe->wDataSize);
			DataModel::sharedDataModel()->userInfo->dwHornCount = pHornNum.dwLabaCount;
		}
		break;
		case DTP_GP_GET_DIAL_COUNT://转盘个数
		{
			DTP_GP_DialCount  pDialNum;
			CopyMemory(&pDialNum, cbDataBuffer + wPacketSize, DataDescribe->wDataSize);
			DataModel::sharedDataModel()->userInfo->dwDialCount = pDialNum.dwDialCount;
		}
		break;
		default:
			break;
		}
		wPacketSize += DataDescribe->wDataSize;
		if (wPacketSize >= wDataSize)
		{
			break;
		}
	}
}
//登录消息
void LogonScene::onEventLogon(WORD wSubCmdID,void * pDataBuffer, unsigned short wDataSize){
	switch (wSubCmdID)
	{
	case SUB_MB_LOGON_SUCCESS:
		{
			//效验参数
			//if (wDataSize != sizeof(CMD_MB_LogonSuccess)) return ;
			if (wDataSize < sizeof(CMD_MB_LogonSuccess)) return;
			CMD_MB_LogonSuccess *ls = (CMD_MB_LogonSuccess*)pDataBuffer;
			CCLOG("登录成功 %d %s", ls->dwUserID, GBKToUTF8(ls->szNickName).c_str());

			DataModel::sharedDataModel()->isShowNewTaskMsg = ls->dwTaskCount>0;
			//赋值
			strcpy(DataModel::sharedDataModel()->userInfo->szNickName,ls->szNickName);
			DataModel::sharedDataModel()->userInfo->lScore=ls->lUserScore;
			DataModel::sharedDataModel()->userInfo->dwGameID=ls->dwGameID;
			DataModel::sharedDataModel()->userInfo->dwUserID=ls->dwUserID;
			DataModel::sharedDataModel()->userInfo->cbGender=ls->cbGender;
			DataModel::sharedDataModel()->userInfo->wFaceID=ls->wFaceID;
			DataModel::sharedDataModel()->cbInsurePwd=ls->cbInsurePwd;
			DataModel::sharedDataModel()->userInfo->lIngot = ls->lIngot;
			DataModel::sharedDataModel()->userInfo->lIngotScore = ls->lIngotScore;
			DataModel::sharedDataModel()->sPhone = ls->szPhone;
			DataModel::sharedDataModel()->userInfo->dwVipLevel = ls->dwVipLevel;
			DataModel::sharedDataModel()->userInfo->dwFirstOnLineOrder = ls->dwFirstOnLineOrder;
			if (!DataModel::sharedDataModel()->sPhone.empty())
			{
				DataModel::sharedDataModel()->sPhone.erase(0, DataModel::sharedDataModel()->sPhone.find_first_not_of(" "));
				DataModel::sharedDataModel()->sPhone.erase(DataModel::sharedDataModel()->sPhone.find_last_not_of(" ") + 1);
			}

		

			
			if (strcmp(sRegisterAccount.c_str(),"")!=0)
			{
				DataModel::sharedDataModel()->sLogonAccount = sRegisterAccount;
			}
			if (strcmp(sRegisterPasswrod.c_str(), "") != 0)
			{
				DataModel::sharedDataModel()->sLogonPassword = sRegisterPasswrod;
			}
			//是帐号登录，保存登录信息
			if (DataModel::sharedDataModel()->logonType==LOGON_ACCOUNT)
			{
				//保存更多帐号
				if (isNewAccount(DataModel::sharedDataModel()->sLogonAccount.c_str()))
				{
					MoreAccount moreAccount;
					moreAccount.userAccount = DataModel::sharedDataModel()->sLogonAccount;
					moreAccount.userPwd = DataModel::sharedDataModel()->sLogonPassword;
					DataModel::sharedDataModel()->vecMoreAccount.push_back(moreAccount);
					saveAccount();
				}

				Tools::saveStringByRMS(RMS_LOGON_ACCOUNT, DataModel::sharedDataModel()->sLogonAccount);
				Tools::saveStringByRMS(RMS_LOGON_PASSWORD, DataModel::sharedDataModel()->sLogonPassword);
			}
			//解析附加数据
			parsingAdditionalData(ls,pDataBuffer,wDataSize);

			//数据统计
			sendLoginStatistics();

		}
		break;
	case SUB_MB_LOGON_FAILURE:
		{
			//效验参数
			//assert(wDataSize >= sizeof(CMD_MB_LogonFailure));
			//if (wDataSize < sizeof(CMD_MB_LogonFailure)) return;
			CMD_MB_LogonFailure *lf = (CMD_MB_LogonFailure*)pDataBuffer;
			int code = lf->lResultCode;
			char *describeStr = lf->szDescribeString;
			this->getChildByTag(TAG_LOADING)->removeFromParentAndCleanup(true);
			CCLOG("登录失败:%s", GBKToUTF8(describeStr).c_str());
			//TCPSocketControl::sharedTCPSocketControl()->stopSocket(SOCKET_LOGON_GAME);
		
			gameSocket.Destroy(true);
			
			PopDialogBoxOtherLoading *pLoading = (PopDialogBoxOtherLoading*)this->getChildByTag(TAG_LOADING);
			if (pLoading)
			{
				pLoading->removeFromParentAndCleanup(true);
			}

			PopDialogBoxTipInfo *tipInfo = PopDialogBoxTipInfo::create();
			this->addChild(tipInfo);
			tipInfo->setTipInfoContent(GBKToUTF8(describeStr).c_str());
			sRegisterAccount = "";
			sRegisterPasswrod = "";
			sRegisterNickname = "";
		}
		break;
	case SUB_MB_UPDATE_NOTIFY:
		{
			assert(wDataSize >= sizeof(CMD_MB_UpdateNotify));
			if (wDataSize < sizeof(CMD_MB_UpdateNotify)) return;
			CMD_MB_UpdateNotify *lf = (CMD_MB_UpdateNotify*)pDataBuffer;
			CCLOG("升级提示");
			PopDialogBoxOtherLoading *pLoading = (PopDialogBoxOtherLoading*)this->getChildByTag(TAG_LOADING);
			if (pLoading)
			{
				pLoading->removeFromParentAndCleanup(true);
			}
			
			PopDialogBoxTipInfo *tipInfo = PopDialogBoxTipInfo::create();
			this->addChild(tipInfo);
			tipInfo->setTipInfoContent(" 当前版本不是最新版！ ");


		}
		break;
	case SUB_GP_LOBBY_IP:
	{
		//效验参数
		if (wDataSize < sizeof(CMD_GP_LobbyIp)) return;
		CMD_GP_LobbyIp *pLobbyIp = (CMD_GP_LobbyIp*)pDataBuffer;
		DataModel::sharedDataModel()->sLobbyIp = pLobbyIp->szServerIP;
		DataModel::sharedDataModel()->lLobbyProt = pLobbyIp->dwServerPort;
		/*TCPSocket *tcp = TCPSocketControl::sharedTCPSocketControl()->getTCPSocket(SOCKET_LOBBY);
		if (tcp&&tcp->eSocketState!=TCPSocket::SOCKET_STATE_CONNECT_SUCCESS){
			tcp->createSocket(pLobbyIp->szServerIP, pLobbyIp->dwServerPort, new LobbyGameListerner());
			//tcp->createSocket("112.1.1.1", pLobbyIp->dwServerPort, new LobbyGameListerner());
		}*/
		
	}
		break;
	case SUB_MB_QUICK_LOGIN:
	{
		CMD_MB_Quick_Logon_Success *pQuickLogonSuccess = (CMD_MB_Quick_Logon_Success*)pDataBuffer;
		char *describeStr = pQuickLogonSuccess->szDescribeString;
		CCLog("LOGON_QUICK快速登录:%s", pQuickLogonSuccess->szDescribeString);
		if (pQuickLogonSuccess->lResultCode==0)
		{
			logonQQ(pQuickLogonSuccess->szAccounts, pQuickLogonSuccess->szLogonPass);
		}
		else
		{
			PopDialogBoxTipInfo *tipInfo = PopDialogBoxTipInfo::create();
			this->addChild(tipInfo);
			tipInfo->setTipInfoContent(GBKToUTF8(describeStr).c_str());
		}
	}
		break;
	default:
		CCLOG("--------%d <<%s>>", wSubCmdID, __FUNCTION__);
		break;
	}
}
//获取列表
void LogonScene::onEventServerList(WORD wSubCmdID,void * pDataBuffer, unsigned short wDataSize){
	switch (wSubCmdID)
	{
	case SUB_MB_LIST_KIND:
		{
			//效验参数
			//if (wDataSize != sizeof(tagGameKind)) return false;
			int size=sizeof(tagGameKind);
			int count=wDataSize/sizeof(tagGameKind);
			BYTE cbDataBuffer[SOCKET_TCP_PACKET + sizeof(TCP_Head)];
			CopyMemory(cbDataBuffer, pDataBuffer, wDataSize);

			for (int i = 0; i < count; i++)
			{
				tagGameKind *gs = (tagGameKind*)(cbDataBuffer + i*sizeof(tagGameKind));
				CCLOG("---<<%s>>",__FUNCTION__);
			}
			CCLOG("获取游戏种类");
		}
		break;
	case SUB_MB_LIST_SERVER://房间列表
		{
			int gameServerSize = sizeof(tagGameServer);
			int serverCount = wDataSize / gameServerSize;

			BYTE cbDataBuffer[SOCKET_TCP_PACKET + sizeof(TCP_Head)];
			CopyMemory(cbDataBuffer, pDataBuffer, wDataSize);

			DataModel::sharedDataModel()->removeTagGameServerList(DataModel::sharedDataModel()->tagGameServerListOxTwo);
			DataModel::sharedDataModel()->removeTagGameServerList(DataModel::sharedDataModel()->tagGameServerListOxHundred);
			DataModel::sharedDataModel()->removeTagGameServerList(DataModel::sharedDataModel()->tagGameServerListOxOneByOne);
			DataModel::sharedDataModel()->removeTagGameServerList(DataModel::sharedDataModel()->tagGameServerListSixSwap);
			for (int i = 0; i < serverCount; i++)
			{
				void * pDataBuffer = cbDataBuffer + i*sizeof(tagGameServer);
				tagGameServer *gameServer = (tagGameServer*)pDataBuffer;
				tagGameServer *tempTag=new tagGameServer();
				memcpy(tempTag,gameServer,sizeof(tagGameServer));
				//memcoyp(gameServer,0,sizeof(tagGameServer));
				//分割字符串
				std::string roomName = tempTag->szGameLevel;
				int pos = roomName.find("-");
				std::string s = roomName.substr(0, pos);
				strcpy(tempTag->szGameLevel,s.c_str());

				if (tempTag->wKindID==210)
				{
					DataModel::sharedDataModel()->tagGameServerListOxTwo.push_back(tempTag);
				}else if(tempTag->wKindID==30)
				{
					DataModel::sharedDataModel()->tagGameServerListOxHundred.push_back(tempTag);
				}
				else if (tempTag->wKindID == 130)
				{
					DataModel::sharedDataModel()->tagGameServerListOxOneByOne.push_back(tempTag);
					CCLOG("port %d  %d  %s<<%s>>", tempTag->wServerPort, tempTag->wSortID, GBKToUTF8(tempTag->szDescription).c_str(), __FUNCTION__);

				}
				else if (tempTag->wKindID == 430)
				{
					DataModel::sharedDataModel()->tagGameServerListSixSwap.push_back(tempTag);
					CCLOG("port %d  %d  six swap %s<<%s>>", tempTag->wServerPort, tempTag->wSortID, GBKToUTF8(tempTag->szDescription).c_str(), __FUNCTION__);

				}
				else
				{
					CCLOG("port %d  %d  %s<<%s>>", tempTag->wServerPort, tempTag->wSortID, GBKToUTF8(tempTag->szDescription).c_str(), __FUNCTION__);
				}
				
				//sort(DataModel::sharedDataModel()->tagGameServerList.begin(), DataModel::sharedDataModel()->tagGameServerList.end(), less_second);
				
              //  CCLOG("port-----:%d ",tempTag->wServerPort);
			}
			DataModel::sharedDataModel()->sortVector(DataModel::sharedDataModel()->tagGameServerListOxTwo);
			DataModel::sharedDataModel()->sortVector(DataModel::sharedDataModel()->tagGameServerListOxHundred);
			DataModel::sharedDataModel()->sortVector(DataModel::sharedDataModel()->tagGameServerListOxOneByOne);
			DataModel::sharedDataModel()->sortVector(DataModel::sharedDataModel()->tagGameServerListSixSwap);
		}
		break;
	case SUB_MB_LIST_FINISH:
		{
			unscheduleUpdate();
            gameSocket.Destroy(true);
			LobbyMsgHandler::sharedLobbyMsgHandler()->connectServer(DataModel::sharedDataModel()->sLobbyIp, DataModel::sharedDataModel()->lLobbyProt);
			//TCPSocketControl::sharedTCPSocketControl()->stopSocket(SOCKET_LOGON_GAME);

			Tools::saveIntByRMS(RMS_LOGON_TYPE, DataModel::sharedDataModel()->logonType);

			Tools::setTransitionAnimation(0,0,GameLobbyScene::scene(false,""));
		}
		break;
	case SUB_GP_UNREAD_MSG_COUNT://未读的信息
	{
		CMD_GP_UserIDNotify *pUserIDNotify = (CMD_GP_UserIDNotify*)pDataBuffer;
	}
		break;
		/*case DTP_GP_GET_LABA_COUNT: //喇叭数
	{
		DTP_GP_GetLabaCount * pHornNum = (DTP_GP_GetLabaCount *)pDataBuffer;
		DataModel::sharedDataModel()->userInfo->dwHornCount = pHornNum->dwLabaCount;
		//pIndividualUserData->dwHornNum = pHornNum->dwLabaCount;
		break;
	}
	case DTP_GP_GET_DIAL_COUNT: //转盘数
	{
		ASSERT(DataDescribe.wDataSize == sizeof(DTP_GP_DialCount));
		if (DataDescribe.wDataSize == sizeof(DTP_GP_DialCount))
		{
			DTP_GP_DialCount * pDialNum = (DTP_GP_DialCount *)pDataBuffer;
			pIndividualUserData->dwDialNum = pDialNum->dwDialCount;
		}
		break;
	}*/

	default:
		CCLOG("other:%d<<%s>>",wSubCmdID,__FUNCTION__);
		break;
	}
}
void LogonScene::onOpen(){
	CCLOG("open <<%s>>", __FUNCTION__);
}
void LogonScene::onError(const char* sError){
	
	PopDialogBoxTipInfo *tipInfo = PopDialogBoxTipInfo::create();
	this->addChild(tipInfo);
	tipInfo->setTipInfoContent(sError);

	PopDialogBoxOtherLoading *pLoading = (PopDialogBoxOtherLoading*)this->getChildByTag(TAG_LOADING);
	if (pLoading)
	{
		pLoading->removeFromParentAndCleanup(true);
	}
}
bool LogonScene::onMessage(WORD wMainCmdID, WORD wSubCmdID, void * pDataBuffer, unsigned short wDataSize){
	onEventReadMessage(wMainCmdID, wSubCmdID, pDataBuffer, wDataSize);
	return true;
}
//注册回调
void LogonScene::onRegistered(const char *sAccount, const char*sNickname, const char*sPassword){
	sRegisterAccount = sAccount;
	sRegisterNickname = sNickname;
	sRegisterPasswrod = sPassword;
	connectServer();
	registeredGame();
}
/************************************************************************/
/* 存档                                                                     */
/************************************************************************/
void LogonScene::readRMS(){
	if (isHaveSaveFile()) {
		DataModel::isMusic = Tools::getBoolByRMS(RMS_IS_MUSIC);
		DataModel::isSound = Tools::getBoolByRMS(RMS_IS_SOUND);
		DataModel::sharedDataModel()->logonType = Tools::getIntByRMS(RMS_LOGON_TYPE);
		isFirstLogon = Tools::getBoolByRMS(RMS_FIRST_LOGON);
	}
	else{
		this->initRSM();
        
	}
}
void LogonScene::initRSM(){
	Tools::saveBoolByRMS(RMS_IS_MUSIC, true);
	Tools::saveBoolByRMS(RMS_IS_SOUND, true);
	Tools::saveIntByRMS(RMS_LOGON_TYPE, 0);
	Tools::saveStringByRMS(RMS_LOGON_ACCOUNT,"");
	Tools::saveStringByRMS(RMS_LOGON_PASSWORD,"");
	Tools::saveStringByRMS(RMS_SIGN_RECORD, "");
	Tools::saveStringByRMS(RMS_MORE_ACCOUNT,"");
	Tools::saveStringByRMS(RMS_NEW_MSG_TIP, "");
	Tools::saveBoolByRMS(RMS_FIRST_LOGON, true);
}
bool LogonScene::isHaveSaveFile(){
	if (!Tools::getBoolByRMS("isHaveSaveFileXml"))
	{
		Tools::saveBoolByRMS("isHaveSaveFileXml", true);
		return false;
	}
	else{
		return true;
	}
}
//是否是新帐号
bool LogonScene::isNewAccount(const char *sAccount){
	for (int i = 0; i < DataModel::sharedDataModel()->vecMoreAccount.size(); i++)
	{
		MoreAccount moreAccount = DataModel::sharedDataModel()->vecMoreAccount[i];
		if (strcmp(moreAccount.userAccount.c_str(),sAccount)==0)
		{
			return false;
		}
	}
	return true;
}
//保存帐号
void LogonScene::saveAccount(){
	//保存
	std::string saveStr = "{";
	for (int i = 0; i < DataModel::sharedDataModel()->vecMoreAccount.size(); i++)
	{
		MoreAccount moreAccount = DataModel::sharedDataModel()->vecMoreAccount[i];
		saveStr += CCString::createWithFormat("\"AccountData\":{\"userId\":\"%s\",\"userPwd\":\"%s\"},", moreAccount.userAccount.c_str(), moreAccount.userPwd.c_str())->getCString();
	}
	saveStr.erase(saveStr.end() - 1);
	saveStr += "}";
	Tools::saveStringByRMS(RMS_MORE_ACCOUNT, saveStr.c_str());
}

void LogonScene::keyBackClicked(){
	platformAction("{\"act\":400}");
}
//统计安装
void LogonScene::statisticsInstall(){
	Statistics *pStatistice = Statistics::create();
	pStatistice->sendStatisticsData(Statistics::S_INSTALL);
}
void LogonScene::closeWebView(){
    m_pWidget->setTouchEnabled(true);
}
void LogonScene::logonQQ(const char*id,const char*pwd){
    closeWebView();
    DataModel::sharedDataModel()->sLogonAccount=id;
    DataModel::sharedDataModel()->sLogonPassword=pwd;
    scheduleOnce(SEL_SCHEDULE(&LogonScene::logonGameByAccount),0.5);
}
//微信登录
void LogonScene::updateLogonWX(float dt){
	connectServer();
	{
		CMD_MB_AccessToken logonToken;
		logonToken.dwSessionID = 111111;
		strcpy(logonToken.szUMId, "");
		logonToken.dwSex = 0;
		strcpy(logonToken.szNickName, "");
		strcpy(logonToken.szMachineID, Tools::getMachineID().c_str());
		strcpy(logonToken.szAccessToken, sTokenCode.c_str());
		gameSocket.SendData(MDM_MB_LOGON, SUB_MB_ACCESSTOKEN, &logonToken, sizeof(logonToken));
	}
}

void LogonScene::logonWX(const char*code){
	sTokenCode = code;
	scheduleOnce(SEL_SCHEDULE(&LogonScene::updateLogonWX), 1);
}

//SDK登录
void LogonScene::updateLogonSDK(float dt){
	connectServer();
	{
		CMD_MB_AccessToken logonToken;
		logonToken.dwSessionID = atoi((CU8sdkFunction::GetInstance().OnGetChannelid()).c_str());
		strcpy(logonToken.szUMId, sUmid.c_str());
		logonToken.dwSex = 0;
		strcpy(logonToken.szNickName, sSdkNickname.c_str());
		strcpy(logonToken.szMachineID, Tools::getMachineID().c_str());
		strcpy(logonToken.szAccessToken, sTokenCode.c_str());
		logonToken.dwSubSessionID = atoi(sSubSessionID.c_str());
		CCLog("SDKlgoin -- %d", logonToken.dwSubSessionID);
		gameSocket.SendData(MDM_MB_LOGON, SUB_MB_ACCESSTOKEN, &logonToken, sizeof(logonToken));
		CCLog("SDKlgoin");
	}
}

void LogonScene::SDKlgoin(const char* umid, const char* token, const char* userid, const char* username)
{
	sUmid = umid;
	sTokenCode = token;
	sSubSessionID = userid;
	sSdkNickname = username;
	scheduleOnce(SEL_SCHEDULE(&LogonScene::updateLogonSDK), 1);
}

/////////////////////////////////////////////////////////////////////////////
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#include <jni.h>

//call c
#ifdef __cplusplus
extern "C"
{
#endif


	//java 调用(签名验证)
	JNIEXPORT void JNICALL Java_com_xw_BullfightGame_BullfightGame_JniQQLogin(JNIEnv* env,jobject job,jint type, jstring account, jstring password)
	{
		const char * sAccount= env->GetStringUTFChars(account, NULL);
		const char * sPwd= env->GetStringUTFChars(password, NULL);
		//const char * sAccount= "zh1236";
		//const char * sPwd= "z12345678";
		switch(type)
		{
		case 1:
		{
			LogonScene::pLScene->logonQQ(sAccount,sPwd);
		}
			break;
		default:
		{
			LogonScene::pLScene->closeWebView();
		}
			break;
		}
	} 
	//java 调用(签名验证)
	JNIEXPORT void JNICALL Java_com_xw_BullfightGame_BullfightGame_JniWXLogin(JNIEnv* env,jobject job, jstring code)
	{
		const char * sCode= env->GetStringUTFChars(code, NULL);
		
		LogonScene::pLScene->logonWX(sCode);
		
	}
#ifdef __cplusplus
}
#endif
//////////////////////////////////////////////////////////////////////////
#endif // #if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
