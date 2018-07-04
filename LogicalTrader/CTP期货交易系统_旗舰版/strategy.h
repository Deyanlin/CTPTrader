#ifndef _STRATEGY_H
#define _STRATEGY_H

#include "ThostFtdcMdApi.h"
#include <vector>
#include "StructFunction.h"
#include <string>
#include <iostream>
#include <map>
#include "traderspi.h"
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time




class Strategy
{
public:
	Strategy(CtpTraderSpi* TDSpi):TDSpi_stgy(TDSpi)
	{		
		m_allow_open = false;
		GetHistoryData();
	}

	//����ص�������ÿ�յ�һ������ʹ���һ��
	void OnTickData(CThostFtdcDepthMarketDataField *pDepthMarketData);

	//���ý��׵ĺ�Լ����
	void Init(string instId, int kdbPort, string kdbScript);
	
	//�������߼��ļ��㣬70������������0.6Ԫ�������࣬�µ������գ�ϵͳĬ�Ͻ�ֹ���֣����ڽ�������"yxkc"�������֣�
	void StrategyCalculate(CThostFtdcDepthMarketDataField *pDepthMarketData);

	//�����Ƿ�������
	void set_allow_open(bool x);
	
	//�����������ݵ�vector
	void SaveDataVec(CThostFtdcDepthMarketDataField *pDepthMarketData);
	
	//�����������ݵ�txt��csv
	void SaveDataTxtCsv(CThostFtdcDepthMarketDataField *pDepthMarketData);

	//���ú�Լ-��Լ��Ϣ�ṹ���map
	void set_instMessage_map_stgy(map<string, CThostFtdcInstrumentField*>& instMessage_map_stgy);

	//�����˻���ӯ����Ϣ
	void CalculateEarningsInfo(CThostFtdcDepthMarketDataField *pDepthMarketData);
	
	//��ȡ��ʷ����
	void GetHistoryData();

	//�����ݵ�kdb
	void DataInsertToKDB(CThostFtdcDepthMarketDataField *pDepthMarketData);

	std::string return_current_time_and_date();


private:

	CtpTraderSpi* TDSpi_stgy;//TDָ��

	TThostFtdcInstrumentIDType m_instId;//��Լ����

	FutureData futureData;//�Զ�����������ݽṹ��

	vector<FutureData> m_futureData_vec;//�����������ݵ�vector,�����ָ����Ҫnew���ٱ���

	bool m_allow_open;//TRUE�����֣�FALSE��ֹ����
		
	map<string, CThostFtdcInstrumentField*> m_instMessage_map_stgy;//�����Լ��Ϣ��map,ͨ��set_instMessage_map_stgy()������TD����
	
	double m_openProfit_account;//�����˻��ĸ���ӯ��,�����ּ���
	
	double m_closeProfit_account;//�����˻���ƽ��ӯ��

	vector<History_data> history_data_vec;//������ʷ���ݵ�vector

};

#endif