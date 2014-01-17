#pragma once

#include "QuantBox.C2CTP.h"
#include "include\CTP\ThostFtdcMdApi.h"

#include <set>
#include <string>

using namespace std;

class CCTPMsgQueue;

class CMdUserApi :
	public CThostFtdcMdSpi
{
public:
	CMdUserApi(void);
	virtual ~CMdUserApi(void);

	void RegisterMsgQueue(CCTPMsgQueue* pMsgQueue);

	void Connect(const string& szPath,
		const string& szAddresses,
		const string& szBrokerId,
		const string& szInvestorId,
		const string& szPassword);
	void Disconnect();

	int Subscribe(const string& szInstrumentIDs);
	int Unsubscribe(const string& szInstrumentIDs);

private:
	//��������
	int Subscribe(const set<string>& instrumentIDs);
	//��¼����
	int ReqUserLogin();

	virtual void OnFrontConnected();
	virtual void OnFrontDisconnected(int nReason);
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

	//����Ƿ����
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);//��������Ϣ�͵���Ϣ����
	bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);//���ͳ�����Ϣ

private:
	CRITICAL_SECTION			m_csMapInstrumentIDs;

	ConnectionStatus			m_status;				//����״̬
	int							m_nRequestID;			//����ID��ÿ������ǰ����
	
	set<string>					m_setInstrumentIDs;		//���ڶ��ĵĺ�Լ
	CThostFtdcMdApi*			m_pApi;					//����API
	CCTPMsgQueue*				m_msgQueue;				//��Ϣ����ָ��

	string						m_szPath;				//���������ļ���·��
	set<string>					m_arrAddresses;			//��������ַ
	string						m_szBrokerId;			//����ID
	string						m_szInvestorId;			//Ͷ����ID
	string						m_szPassword;			//����
};

