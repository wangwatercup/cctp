#include "StdAfx.h"
#include "TraderApi.h"
#include "CTPMsgQueue.h"
#include "include\toolkit.h"
#include "include\Lock.h"
#include <fstream>

CTraderApi::CTraderApi(void)
{
	m_msgQueue = NULL;
	m_status = E_uninit;
	m_lRequestID = 0;

	m_hThread = NULL;
	m_bRunning = false;

	/*InitializeCriticalSection(&m_csList);
	InitializeCriticalSection(&m_csMap);*/
	InitializeCriticalSection(&m_csOrderRef);
}


CTraderApi::~CTraderApi(void)
{
	Disconnect();

	//DeleteCriticalSection(&m_csList);
	//DeleteCriticalSection(&m_csMap);
	DeleteCriticalSection(&m_csOrderRef);
}

void CTraderApi::StopThread()
{
	//ֹͣ�����߳�
	m_bRunning = false;
	WaitForSingleObject(m_hThread,INFINITE);
	CloseHandle(m_hThread);
	m_hThread = NULL;
}

void CTraderApi::RegisterMsgQueue(CCTPMsgQueue* pMsgQueue)
{
	m_msgQueue = pMsgQueue;
}

bool CTraderApi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)   
{
	bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));
	if(bRet)
	{
		if(m_msgQueue)
		{
			(*m_msgQueue->m_fnOnRspError)(this,pRspInfo,nRequestID,bIsLast);
		}
	}
	return bRet;
}

bool CTraderApi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)   
{
	bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));

	return bRet;
}

void CTraderApi::Connect(const string& szPath,
		const string& szAddresses,
		const string& szBrokerId,
		const string& szInvestorId,
		const string& szPassword,
		THOST_TE_RESUME_TYPE nResumeType,
		const string& szUserProductInfo,
		const string& szAuthCode)
{
	m_szBrokerId = szBrokerId;
	m_szInvestorId = szInvestorId;
	m_szPassword = szPassword;
	m_szUserProductInfo = szUserProductInfo;
	m_szAuthCode = szAuthCode;

	char *pszPath = new char[szPath.length()+20];
	sprintf(pszPath,"%s\\Td\\",szPath.c_str());
	makedirs(pszPath);

	m_pApi = CThostFtdcTraderApi::CreateFtdcTraderApi(pszPath);
	delete[] pszPath;

	m_status = E_inited;
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnConnect)(this,NULL,m_status);
	
	if (m_pApi)
	{
		m_pApi->RegisterSpi(this);
		
		//��ӵ�ַ
		size_t len = szAddresses.length()+1;
		char* buf = new char[len];
		strncpy(buf,szAddresses.c_str(),len);

		char* token = strtok(buf, _QUANTBOXC2CTP_SEPS_);
		while(token)
		{
			if (strlen(token)>0)
			{
				m_pApi->RegisterFront(token);
			}
			token = strtok( NULL, _QUANTBOXC2CTP_SEPS_);
		}
		delete[] buf;

		m_pApi->SubscribePublicTopic(nResumeType);
		m_pApi->SubscribePrivateTopic(nResumeType);
		
		//��ʼ������
		m_pApi->Init();
		m_status = E_connecting;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnConnect)(this,NULL,m_status);
	}
}

void CTraderApi::Disconnect()
{
	// �������������������ں����ֽ�����Release,�ֻع�ͷ�����ͣ����ܵ��µ���
	StopThread();

	m_status = E_unconnected;
	if(m_pApi)
	{
		m_pApi->RegisterSpi(NULL);
		m_pApi->Release();
		m_pApi = NULL;

		if(m_msgQueue)
			(*m_msgQueue->m_fnOnDisconnect)(this,NULL,m_status);
	}

	m_lRequestID = 0;//�����߳��Ѿ�ֹͣ��û�б�Ҫ��ԭ�Ӳ�����

	//ReleaseRequestListBuf();
	//ReleaseRequestMapBuf();
}

//CTraderApi::SRequest* CTraderApi::MakeRequestBuf(RequestType type)
//{
//	SRequest *pRequest = new SRequest;
//	if (NULL == pRequest)
//		return NULL;
//
//	memset(pRequest,0,sizeof(SRequest));
//	pRequest->type = type;
//
//	return pRequest;
//}

//void CTraderApi::ReleaseRequestListBuf()
//{
//	CLock cl(&m_csList);
//	while (!m_reqList.empty())
//	{
//		SRequest * pRequest = m_reqList.front();
//		delete pRequest;
//		m_reqList.pop_front();
//	}
//}

//void CTraderApi::ReleaseRequestMapBuf()
//{
//	CLock cl(&m_csMap);
//	for (map<int,SRequest*>::iterator it=m_reqMap.begin();it!=m_reqMap.end();++it)
//	{
//		delete (*it).second;
//	}
//	m_reqMap.clear();
//}

//void CTraderApi::ReleaseRequestMapBuf(int nRequestID)
//{
//	CLock cl(&m_csMap);
//	map<int,SRequest*>::iterator it = m_reqMap.find(nRequestID);
//	if (it!=m_reqMap.end())
//	{
//		delete it->second;
//		m_reqMap.erase(nRequestID);
//	}
//}

//void CTraderApi::AddRequestMapBuf(int nRequestID,SRequest* pRequest)
//{
//	if(NULL == pRequest)
//		return;
//
//	CLock cl(&m_csMap);
//	map<int,SRequest*>::iterator it = m_reqMap.find(nRequestID);
//	if (it!=m_reqMap.end())
//	{
//		SRequest* p = it->second;
//		if(pRequest != p)//���ʵ����ָ����ͬһ�ڴ棬���ٲ���
//		{
//			delete p;
//			m_reqMap[nRequestID] = pRequest;
//		}
//	}
//}

//DWORD WINAPI SendThread(LPVOID lpParam)
//{
//	CTraderApi* pTrade = reinterpret_cast<CTraderApi *>(lpParam);
//	if (pTrade)
//		pTrade->RunInThread();
//	return 0;
//}

//void CTraderApi::AddToSendQueue(SRequest * pRequest)
//{
//	if (NULL == pRequest)
//		return;
//
//	CLock cl(&m_csList);
//	bool bFind = false;
//	//Ŀǰ��ȥ����ͬ���͵����󣬼�û�жԴ���ͬ������������Ż�
//	//for (list<SRequest*>::iterator it = m_reqList.begin();it!= m_reqList.end();++it)
//	//{
//	//	if (pRequest->type == (*it)->type)
//	//	{
//	//		bFind = true;
//	//		break;
//	//	}
//	//}
//
//	if (!bFind)
//		m_reqList.push_back(pRequest);
//
//	if (NULL == m_hThread
//		&&!m_reqList.empty())
//	{
//		m_bRunning = true;
//		m_hThread = CreateThread(NULL,0,SendThread,this,0,NULL); 
//	}
//}


//void CTraderApi::RunInThread()
//{
//	int iRet = 0;
//
//	while (!m_reqList.empty()&&m_bRunning)
//	{
//		SRequest * pRequest = m_reqList.front();
//		long lRequest = InterlockedIncrement(&m_lRequestID);
//		switch(pRequest->type)
//		{
//		case E_ReqAuthenticateField:
//			iRet = m_pApi->ReqAuthenticate(&pRequest->ReqAuthenticateField,lRequest);
//			break;
//		case E_ReqUserLoginField:
//			iRet = m_pApi->ReqUserLogin(&pRequest->ReqUserLoginField,lRequest);
//			break;
//		case E_SettlementInfoConfirmField:
//			iRet = m_pApi->ReqSettlementInfoConfirm(&pRequest->SettlementInfoConfirmField,lRequest);
//			break;
//		case E_QryInstrumentField:
//			iRet = m_pApi->ReqQryInstrument(&pRequest->QryInstrumentField,lRequest);
//			break;
//		case E_QryTradingAccountField:
//			iRet = m_pApi->ReqQryTradingAccount(&pRequest->QryTradingAccountField,lRequest);
//			break;
//		case E_QryInvestorPositionField:
//			iRet = m_pApi->ReqQryInvestorPosition(&pRequest->QryInvestorPositionField,lRequest);
//			break;
//		case E_QryInvestorPositionDetailField:
//			iRet=m_pApi->ReqQryInvestorPositionDetail(&pRequest->QryInvestorPositionDetailField,lRequest);
//			break;
//		case E_QryInstrumentCommissionRateField:
//			iRet = m_pApi->ReqQryInstrumentCommissionRate(&pRequest->QryInstrumentCommissionRateField,lRequest);
//			break;
//		case E_QryInstrumentMarginRateField:
//			iRet = m_pApi->ReqQryInstrumentMarginRate(&pRequest->QryInstrumentMarginRateField,lRequest);
//			break;
//		case E_QryDepthMarketDataField:
//			iRet = m_pApi->ReqQryDepthMarketData(&pRequest->QryDepthMarketDataField,lRequest);
//			break;
//		default:
//			_ASSERT(FALSE);
//			break;
//		}
//
//		if (0 == iRet)
//		{
//			//���سɹ�����ӵ��ѷ��ͳ�
//			m_nSleep = 1;
//			AddRequestMapBuf(lRequest,pRequest);
//
//			CLock cl(&m_csList);
//			m_reqList.pop_front();
//		}
//		else
//		{
//			//ʧ�ܣ���4���ݽ�����ʱ����������1s
//			m_nSleep *= 4;
//			m_nSleep %= 1023;
//		}
//		Sleep(m_nSleep);
//	}
//
//	//�����߳�
//	CloseHandle(m_hThread);
//	m_hThread = NULL;
//	m_bRunning = false;
//}

void CTraderApi::OnFrontConnected()
{
	m_status =  E_connected;
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnConnect)(this,NULL,m_status);

	//���ӳɹ����Զ�������֤���¼
	if(m_szAuthCode.length()>0
		&&m_szUserProductInfo.length()>0)
	{
		//������֤�������֤
		ReqAuthenticate();
	}
	else
	{
		ReqUserLogin();
	}
}

void CTraderApi::OnFrontDisconnected(int nReason)
{
	m_status = E_unconnected;
	CThostFtdcRspInfoField RspInfo;
	//��������Ĵ�����Ϣ��Ϊ��ͳһ������Ϣ
	RspInfo.ErrorID = nReason;
	GetOnFrontDisconnectedMsg(&RspInfo);

	if(m_msgQueue)
		(*m_msgQueue->m_fnOnDisconnect)(this,&RspInfo,m_status);
}

int CTraderApi::ReqAuthenticate()
{
	if (NULL == m_pApi)
		return NULL_ERROR;

	//SRequest* pRequest = MakeRequestBuf(E_ReqAuthenticateField);
	//if (pRequest)
	{
		m_status = E_authing;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnConnect)(this,NULL,m_status);

		CThostFtdcReqAuthenticateField body;
		memset(&body,0,sizeof(body));
		strncpy(body.BrokerID, m_szBrokerId.c_str(),sizeof(TThostFtdcBrokerIDType));
		strncpy(body.UserID, m_szInvestorId.c_str(),sizeof(TThostFtdcInvestorIDType));
		strncpy(body.UserProductInfo,m_szUserProductInfo.c_str(),sizeof(TThostFtdcProductInfoType));
		strncpy(body.AuthCode,m_szAuthCode.c_str(),sizeof(TThostFtdcAuthCodeType));
		long lRequest = InterlockedIncrement(&m_lRequestID);
		return m_pApi->ReqAuthenticate(&body,lRequest);
		//AddToSendQueue(pRequest);
	}
}

void CTraderApi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo)
		&&pRspAuthenticateField)
	{
		m_status = E_authed;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnConnect)(this,NULL,m_status);

		ReqUserLogin();
	}
	else
	{
		m_status = E_connected;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnDisconnect)(this,pRspInfo,E_authing);
	}

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

int CTraderApi::ReqUserLogin()
{
	if (NULL == m_pApi)
		return NULL_ERROR;

	//SRequest* pRequest = MakeRequestBuf(E_ReqUserLoginField);
	//if (pRequest)
	{
		m_status = E_logining;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnConnect)(this,NULL,m_status);

		CThostFtdcReqUserLoginField body;
		memset(&body,0,sizeof(body));
		strncpy(body.BrokerID, m_szBrokerId.c_str(),sizeof(TThostFtdcBrokerIDType));
		strncpy(body.UserID, m_szInvestorId.c_str(),sizeof(TThostFtdcInvestorIDType));
		strncpy(body.Password, m_szPassword.c_str(),sizeof(TThostFtdcPasswordType));
		strncpy(body.UserProductInfo,m_szUserProductInfo.c_str(),sizeof(TThostFtdcProductInfoType));
		long lRequest = InterlockedIncrement(&m_lRequestID);
		return m_pApi->ReqUserLogin(&body,lRequest);

		//AddToSendQueue(pRequest);
	}
}

void CTraderApi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo)
		&&pRspUserLogin)
	{
		m_status = E_logined;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnConnect)(this,pRspUserLogin,m_status);
		
		memcpy(&m_RspUserLogin,pRspUserLogin,sizeof(CThostFtdcRspUserLoginField));
		m_nMaxOrderRef = atol(pRspUserLogin->MaxOrderRef);
		ReqSettlementInfoConfirm();
	}
	else
	{
		m_status = E_authed;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnDisconnect)(this,pRspInfo,E_logining);
	}

	//if (bIsLast)
	//	ReleaseRequestMapBuf(nRequestID);
}

int CTraderApi::ReqSettlementInfoConfirm()
{
	if (NULL == m_pApi)
		return NULL_ERROR;

	/*SRequest* pRequest = MakeRequestBuf(E_SettlementInfoConfirmField);
	if (pRequest)*/
	{
		m_status = E_confirming;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnConnect)(this,NULL,m_status);

		CThostFtdcSettlementInfoConfirmField body;
		memset(&body,0,sizeof(body));
		strncpy(body.BrokerID, m_szBrokerId.c_str(),sizeof(TThostFtdcBrokerIDType));
		strncpy(body.InvestorID, m_szInvestorId.c_str(),sizeof(TThostFtdcInvestorIDType));
		long lRequest = InterlockedIncrement(&m_lRequestID);
		return m_pApi->ReqSettlementInfoConfirm(&body,lRequest);
		//AddToSendQueue(pRequest);
	}
}

void CTraderApi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if (!IsErrorRspInfo(pRspInfo)
		&&pSettlementInfoConfirm)
	{
		m_status = E_confirmed;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnConnect)(this,NULL,m_status);
	}
	else
	{
		m_status = E_logined;
		if(m_msgQueue)
			(*m_msgQueue->m_fnOnDisconnect)(this,pRspInfo,E_confirming);
	}

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

int CTraderApi::ReqOrderInsert(
	const string& szInstrumentId,
	TThostFtdcDirectionType Direction,
	const TThostFtdcCombOffsetFlagType CombOffsetFlag,
	const TThostFtdcCombHedgeFlagType CombHedgeFlag,
	TThostFtdcVolumeType VolumeTotalOriginal,
	TThostFtdcPriceType LimitPrice,
	TThostFtdcOrderPriceTypeType OrderPriceType,
	TThostFtdcTimeConditionType TimeCondition,
	TThostFtdcContingentConditionType ContingentCondition,
	TThostFtdcPriceType StopPrice,
	TThostFtdcVolumeConditionType VolumeCondition,
	TThostFtdcOrderRefType	OrderRef)
{
	if (NULL == m_pApi)
		return 0;

	/*SRequest* pRequest = MakeRequestBuf(E_InputOrderField);
	if (NULL == pRequest)
		return 0;*/

	CThostFtdcInputOrderField body;
	memset(&body,0,sizeof(body));
	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	
	body.MinVolume = 1;
	body.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	body.IsAutoSuspend = 0;
	body.UserForceClose = 0;
	body.IsSwapOrder = 0;

	//��Լ
	strncpy(body.InstrumentID, szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	//����
	body.Direction = Direction;
	//��ƽ
	memcpy(body.CombOffsetFlag,CombOffsetFlag,sizeof(TThostFtdcCombOffsetFlagType));
	
	//�۸�
	body.OrderPriceType = OrderPriceType;
	body.LimitPrice = LimitPrice;		
	
	//����
	body.VolumeTotalOriginal = VolumeTotalOriginal;
	//Ͷ��
	memcpy(body.CombHedgeFlag,CombHedgeFlag,sizeof(TThostFtdcCombHedgeFlagType));
	
	//������
	body.VolumeCondition = VolumeCondition;
	body.TimeCondition = TimeCondition;
	body.ContingentCondition = ContingentCondition;
	body.StopPrice = StopPrice;

	//int nRet = 0;
	{
		//���ܱ���̫�죬m_nMaxOrderRef��û�иı���ύ��
		/*CLock cl(&m_csOrderRef);

		nRet = m_nMaxOrderRef;
		sprintf(body.OrderRef,"%d",nRet);
		++m_nMaxOrderRef;*/

		//�����浽���У�����ֱ�ӷ���
		strncpy(body.OrderRef, OrderRef, sizeof(TThostFtdcOrderRefType));

		long lRequest = InterlockedIncrement(&m_lRequestID);
		return m_pApi->ReqOrderInsert(&body,lRequest);
	}
	//delete pRequest;//�����ֱ��ɾ��

	//��ζ�λ�������ñ�������ʵ���ϲ�������
	//�����¿��������������ͬʱ�������µ�����������Ҫ����
	//return nRet;
}

void CTraderApi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspOrderInsert)(this,pInputOrder,pRspInfo,nRequestID,bIsLast);
}

void CTraderApi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnErrRtnOrderInsert)(this,pInputOrder,pRspInfo);
}

void CTraderApi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
	puts("on return trade");
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRtnTrade)(this,pTrade);
}

int CTraderApi::ReqOrderAction(TThostFtdcBrokerIDType brokerID, TThostFtdcInvestorIDType investorID, TThostFtdcOrderRefType orderRef, int frontID, int sessionID, TThostFtdcExchangeIDType exchangeID, TThostFtdcOrderSysIDType orderSysID, TThostFtdcInstrumentIDType instrumentID)
{
    if (NULL == m_pApi)
            return NULL_ERROR;
    
	CThostFtdcInputOrderActionField body;

	memset(&body,0,sizeof(body));
	///���͹�˾����
	strncpy(body.BrokerID, brokerID,sizeof(TThostFtdcBrokerIDType));
	///Ͷ���ߴ���
	strncpy(body.InvestorID, investorID,sizeof(TThostFtdcInvestorIDType));
	///��������
	strncpy(body.OrderRef, orderRef,sizeof(TThostFtdcOrderRefType));
	///ǰ�ñ��
	body.FrontID = frontID;
	///�Ự���
	body.SessionID = sessionID;
	///����������
	strncpy(body.ExchangeID,exchangeID,sizeof(TThostFtdcExchangeIDType));
	///�������
	strncpy(body.OrderSysID,orderSysID,sizeof(TThostFtdcOrderSysIDType));
	///������־
	body.ActionFlag = THOST_FTDC_AF_Delete;
	///��Լ����
	strncpy(body.InstrumentID, instrumentID,sizeof(TThostFtdcInstrumentIDType));    

    long lRequest = InterlockedIncrement(&m_lRequestID);

   return m_pApi->ReqOrderAction(&body,lRequest);
   
	//	ofstream myfile;
	//myfile.open (".\\logs\\log_cancel_order.txt", std::ofstream::out | std::ofstream::app);
	//myfile <<body->InstrumentID<<" "<<body->FrontID<<" "<<body->SessionID<<" "<<body->OrderRef<<endl;
	//myfile.close();     
}

void CTraderApi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	/*if(pInputOrderAction)
	{
		ofstream myfile;
		myfile.open (".\\logs\\log_order_action.txt", std::ofstream::out | std::ofstream::app);
		myfile <<pInputOrderAction->FrontID<<" "<<" "<<pInputOrderAction->SessionID<<" "<<pInputOrderAction->OrderRef<<" "<<endl;
		if(pRspInfo)
			myfile<<pRspInfo->ErrorID<<" "<<pRspInfo->ErrorMsg<<endl;
		myfile.close();
	}
	if(!pInputOrderAction && pRspInfo)
	{
		ofstream myfile;
		myfile.open (".\\logs\\log_order_action.txt", std::ofstream::out | std::ofstream::app);
		myfile<<pRspInfo->ErrorID<<" "<<pRspInfo->ErrorMsg<<endl;
		myfile.close();
	}*/
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspOrderAction)(this,pInputOrderAction,pRspInfo,nRequestID,bIsLast);
}

void CTraderApi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnErrRtnOrderAction)(this,pOrderAction,pRspInfo);
}

void CTraderApi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
	/*ofstream myfile;
	myfile.open (".\\logs\\log_order.txt", std::ofstream::out | std::ofstream::app);
	myfile <<pOrder->InstrumentID<<" "<<pOrder->InsertTime<<"cpp api on return orderr\n";
	myfile.close();*/
	if(m_msgQueue)
	{
		(* m_msgQueue->m_fnOnRtnOrder)(this,pOrder);
	}
}

int CTraderApi::ReqQryTradingAccount()
{
	if (NULL == m_pApi)
		return NULL_ERROR;
	
	/*SRequest* pRequest = MakeRequestBuf(E_QryTradingAccountField);
	if (NULL == pRequest)
		return;*/
	
	CThostFtdcQryTradingAccountField body;
	memset(&body,0,sizeof(body));
	
	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	long lRequest = InterlockedIncrement(&m_lRequestID);
	return m_pApi->ReqQryTradingAccount(&body,lRequest);
	//AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryTradingAccount)(this,pTradingAccount,pRspInfo,nRequestID,bIsLast);

	//if (bIsLast)
	//	ReleaseRequestMapBuf(nRequestID);
}

int CTraderApi::ReqQryInvestorPosition(const string& szInstrumentId)
{
	if (NULL == m_pApi)
		return NULL_ERROR;
	
	/*SRequest* pRequest = MakeRequestBuf(E_QryInvestorPositionField);
	if (NULL == pRequest)
		return;*/
	
	CThostFtdcQryInvestorPositionField body;
	memset(&body,0,sizeof(body));
	
	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	long lRequest = InterlockedIncrement(&m_lRequestID);
	m_pApi->ReqQryInvestorPosition(&body,lRequest);
}

void CTraderApi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryInvestorPosition)(this,pInvestorPosition,pRspInfo,nRequestID,bIsLast);

	//if (bIsLast)
	//	ReleaseRequestMapBuf(nRequestID);
}

int CTraderApi::ReqQryInvestorPositionDetail(const string& szInstrumentId)
{
	if (NULL == m_pApi)
		return NULL_ERROR;

	/*SRequest* pRequest = MakeRequestBuf(E_QryInvestorPositionDetailField);
	if (NULL == pRequest)
		return;*/

	CThostFtdcQryInvestorPositionDetailField body;
	memset(&body,0,sizeof(body));

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	long lRequest = InterlockedIncrement(&m_lRequestID);
	return m_pApi->ReqQryInvestorPositionDetail(&body,lRequest);
}

void CTraderApi::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryInvestorPositionDetail)(this,pInvestorPositionDetail,pRspInfo,nRequestID,bIsLast);

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

int CTraderApi::ReqQryInstrument(const string& szInstrumentId)
{
	if (NULL == m_pApi)
		return NULL_ERROR;
	
	/*SRequest* pRequest = MakeRequestBuf(E_QryInstrumentField);
	if (NULL == pRequest)
		return;*/
	
	CThostFtdcQryInstrumentField body;
	memset(&body,0,sizeof(body));
	
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	long lRequest = InterlockedIncrement(&m_lRequestID);
	return m_pApi->ReqQryInstrument(&body,lRequest);
}

void CTraderApi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryInstrument)(this,pInstrument,pRspInfo,nRequestID,bIsLast);

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

int CTraderApi::ReqQryInstrumentCommissionRate(const string& szInstrumentId)
{
	if (NULL == m_pApi)
		return NULL_ERROR;

	/*SRequest* pRequest = MakeRequestBuf(E_QryInstrumentCommissionRateField);
	if (NULL == pRequest)
		return;*/

	CThostFtdcQryInstrumentCommissionRateField body;
	memset(&body,0,sizeof(body));

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	long lRequest = InterlockedIncrement(&m_lRequestID);
	return m_pApi->ReqQryInstrumentCommissionRate(&body,lRequest);
}

void CTraderApi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryInstrumentCommissionRate)(this,pInstrumentCommissionRate,pRspInfo,nRequestID,bIsLast);

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

int CTraderApi::ReqQryInstrumentMarginRate(const string& szInstrumentId,TThostFtdcHedgeFlagType HedgeFlag)
{
	if (NULL == m_pApi)
		return NULL_ERROR;

	/*SRequest* pRequest = MakeRequestBuf(E_QryInstrumentMarginRateField);
	if (NULL == pRequest)
		return;*/

	CThostFtdcQryInstrumentMarginRateField body;
	memset(&body,0,sizeof(body));

	strncpy(body.BrokerID, m_RspUserLogin.BrokerID,sizeof(TThostFtdcBrokerIDType));
	strncpy(body.InvestorID, m_RspUserLogin.UserID,sizeof(TThostFtdcInvestorIDType));
	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	body.HedgeFlag = HedgeFlag;
	long lRequest = InterlockedIncrement(&m_lRequestID);
	return m_pApi->ReqQryInstrumentMarginRate(&body,lRequest);
}


int CTraderApi::ReqQryOrder(TThostFtdcBrokerIDType brokerID, TThostFtdcInvestorIDType investorID, TThostFtdcInstrumentIDType instrumentID, TThostFtdcExchangeIDType exchangeID, TThostFtdcOrderSysIDType orderSysID)
{
	if (NULL == m_pApi)
		return NULL_ERROR;
	
	CThostFtdcQryOrderField body;
	memset(&body,0,sizeof(body));
	
	///���͹�˾����
	strncpy(body.BrokerID, brokerID,sizeof(TThostFtdcBrokerIDType));
	///Ͷ���ߴ���
	strncpy(body.InvestorID, investorID,sizeof(TThostFtdcInvestorIDType));
	///����������
	strncpy(body.ExchangeID,exchangeID,sizeof(TThostFtdcExchangeIDType));
	///�������
	strncpy(body.OrderSysID, orderSysID,sizeof(TThostFtdcOrderSysIDType));

	long lRequest = InterlockedIncrement(&m_lRequestID);
	return m_pApi->ReqQryOrder(&body,lRequest);
}

void CTraderApi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryInstrumentMarginRate)(this,pInstrumentMarginRate,pRspInfo,nRequestID,bIsLast);

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

int CTraderApi::ReqQryDepthMarketData(const string& szInstrumentId)
{
	if (NULL == m_pApi)
		return NULL_ERROR;

	/*SRequest* pRequest = MakeRequestBuf(E_QryDepthMarketDataField);
	if (NULL == pRequest)
		return;*/

	CThostFtdcQryDepthMarketDataField body ;
	memset(&body,0,sizeof(body));

	strncpy(body.InstrumentID,szInstrumentId.c_str(),sizeof(TThostFtdcInstrumentIDType));
	long lRequest = InterlockedIncrement(&m_lRequestID);
	return m_pApi->ReqQryDepthMarketData(&body,lRequest);
	//AddToSendQueue(pRequest);
}

void CTraderApi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryDepthMarketData)(this,pDepthMarketData,pRspInfo,nRequestID,bIsLast);

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

void CTraderApi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspError)(this,pRspInfo,nRequestID,bIsLast);

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

void CTraderApi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryOrder)(this,pOrder,pRspInfo,nRequestID,bIsLast);

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}


void CTraderApi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRspQryTrade)(this,pTrade,pRspInfo,nRequestID,bIsLast);

	/*if (bIsLast)
		ReleaseRequestMapBuf(nRequestID);*/
}

void CTraderApi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
{
	if(m_msgQueue)
		(*m_msgQueue->m_fnOnRtnInstrumentStatus)(this,pInstrumentStatus);
}