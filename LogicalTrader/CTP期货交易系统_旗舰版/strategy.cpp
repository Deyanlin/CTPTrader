#include "strategy.h"
#include <fstream>
#include "kdb_function.h"


using namespace std;
using namespace kdb;

kdb::Connector kdbConnector;
string m_kdbScript = "ARB_IAD_V2T1_[sn_SN3M]_1_200_HIT.q";
int m_ShortLongInitNum = 0;
int m_ReadShortLongInitNum = 0;
int m_ReadShortLongSignal = 0;
TThostFtdcVolumeType m_VolumeTarget = 1;
int m_OrderVolumeMultiple = 2;

void Strategy::OnTickData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{	
	//�����˻���ӯ����Ϣ
	CalculateEarningsInfo(pDepthMarketData);

	if(strcmp(pDepthMarketData->InstrumentID, m_instId) == 0)
	{
		cerr<<"�����յ�����:"<<pDepthMarketData->InstrumentID<<","<<pDepthMarketData->TradingDay<<","<<pDepthMarketData->UpdateTime<<",���¼�:"<<pDepthMarketData->LastPrice<<",��ͣ��:"<<pDepthMarketData->UpperLimitPrice<<",��ͣ��:"<<pDepthMarketData->LowerLimitPrice<<endl;
		
		//�������ݵ�vector
		SaveDataVec(pDepthMarketData);

		//�������ݵ�txt��csv
		SaveDataTxtCsv(pDepthMarketData);

		//����׷��,�����ɹ�����׷��
		TDSpi_stgy->CancelOrder(pDepthMarketData->UpdateTime, pDepthMarketData->LastPrice);
	
		//����ƽ�֣��������߼��ļ��㣩
		StrategyCalculate(pDepthMarketData);


	}
}



//���ý��׵ĺ�Լ����
void Strategy::Init(string instId, int kdbPort, string kdbScrpit)
{
	strcpy_s(m_instId, instId.c_str());
	kdbConnector.connect("localhost", kdbPort);	
	std::string kdb_init_string = "h:hopen `::5000;PairFormula:{(x%y)};f:{x%y};bollingerBands: {[k;n;data]      movingAvg: mavg[n;data];    md: sqrt mavg[n;data*data]-movingAvg*movingAvg;      movingAvg+/:(k*-1 0 1)*\\:md};ReceiveTimeToDate:{(\"\"z\"\" $ 1970.01.01+ floor x %86400000000 )+ 08:00:00.000 +\"\"j\"\"$ 0.001* x mod  86400000000};isTable:{if[98h=type x;:1b];if[99h=type x;:98h=type key x];0b};isTable2: {@[{isTable value x}; x; 0b]}; ";
	kdbConnector.sync(kdb_init_string.c_str());
	m_kdbScript = kdbScrpit + m_kdbScript;
	////////////////////////////////////////////////////
	/////��һ��������Ҫ�ֶ����ر�                  //////
	///////////////////////////////////////////////////
	kdb::Result res = kdbConnector.sync("isTable2 `ShortLong");
	//�����񲻴������ó�ʼ�ź�Ϊ0
	if (res.res_->g)
	{
		res = kdbConnector.sync("exec count Signal from ShortLong");
		m_ShortLongInitNum = res.res_->j;
	}
	else
	{
		kdbConnector.sync("FinalSignal:([] Date:(); ReceiveDate:(); Symbol:(); LegOneBidPrice1:(); LegOneBidVol1:(); LegOneAskPrice1:(); LegOneAskVol1:(); LegTwoBidPrice1:(); LegTwoBidVol1:(); LegTwoAskPrice1:(); LegTwoAskVol1:(); LegThreeBidPrice1:(); LegThreeBidVol1:(); LegThreeAskPrice1:(); LegThreeAskVol1:(); BidPrice1:(); BidVol1:(); AskPrice1:(); AskVol:(); LowerBand:(); HigherBand:(); Signal:());");
		kdbConnector.sync(m_kdbScript.c_str());
		m_ShortLongInitNum = 0;
	}


}



//�������߼��ļ��㣬70������������0.6Ԫ�������࣬�µ�������
void Strategy::StrategyCalculate(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	TThostFtdcInstrumentIDType    instId;//��Լ,��Լ�����ڽṹ�����Ѿ�����
	strcpy_s(instId, m_instId);
	string instIDString = instId;
	string order_type = TDSpi_stgy->Send_TThostFtdcCombOffsetFlagType(instIDString.substr(0,2));
	

	TThostFtdcDirectionType       dir;//����,'0'��'1'��
	TThostFtdcCombOffsetFlagType  kpp = "8";//��ƽ��"0"����"1"ƽ,"3"ƽ��
	TThostFtdcPriceType           price;//�۸�0���м�,��������֧��
	TThostFtdcVolumeType          vol;//����

	kdbConnector.sync(m_kdbScript.c_str());
	kdb::Result res = kdbConnector.sync("exec count Signal from ShortLong");
	m_ReadShortLongInitNum = res.res_->j;

	if (m_ReadShortLongInitNum > m_ShortLongInitNum)
	{
		m_ShortLongInitNum = m_ReadShortLongInitNum;
		kdb::Result res = kdbConnector.sync("exec last Signal from ShortLong");
		m_ReadShortLongSignal = res.res_->j;

		if (m_ReadShortLongInitNum == 1)
		{
			m_OrderVolumeMultiple = 1;
		}
		else
		{
			m_OrderVolumeMultiple = 2;
		}

		if (m_ReadShortLongSignal > 0)
		{
			dir = '0';
			price = pDepthMarketData->AskPrice1;
			vol = m_VolumeTarget * m_OrderVolumeMultiple;
			SendCorrectOrderOnSymbol(instId, order_type, dir, &kpp, price, vol);
		}
		else if (m_ReadShortLongSignal < 0)
		{
			dir = '1';
			price = pDepthMarketData->BidPrice1;
			vol = m_VolumeTarget * m_OrderVolumeMultiple;
			SendCorrectOrderOnSymbol(instId, order_type, dir, &kpp, price, vol);
		}
	}

	#pragma region strategyTemplate
	//if(m_futureData_vec.size() >= 70)
	//{
	//	//�ֲֲ�ѯ�����в�λ����
	//	if(strcmp(pDepthMarketData->InstrumentID, m_instId) == 0)
	//	{
	//		if(m_futureData_vec.size() % 20 == 0)
	//		{
	//			if(TDSpi_stgy->SendHolding_long(m_instId) + TDSpi_stgy->SendHolding_short(m_instId) < 3)//��չ�����3��
	//			{
	//				//����
	//				if(m_futureData_vec[m_futureData_vec.size()-1].new1 - m_futureData_vec[m_futureData_vec.size()-70].new1 >= 0.6)
	//				{
	//					strcpy_s(instId, m_instId);
	//					dir = '0';
	//					strcpy_s(kpp, "0");
	//					price = pDepthMarketData->LastPrice + 3;
	//					vol = 1;
	//					
	//					if(m_allow_open == true)
	//					{
	//						TDSpi_stgy->ReqOrderInsert(instId, dir, kpp, price, vol);

	//					}
	//				}
	//				//����
	//				else if(m_futureData_vec[m_futureData_vec.size()-70].new1 - m_futureData_vec[m_futureData_vec.size()-1].new1 >= 0.6)
	//				{
	//					strcpy_s(instId, m_instId);
	//					dir = '1';
	//					strcpy_s(kpp, "0");
	//					price = pDepthMarketData->LastPrice - 3;
	//					vol = 1;
	//					if(m_allow_open == true)
	//					{
	//						TDSpi_stgy->ReqOrderInsert(instId, dir, kpp, price, vol);

	//					}
	//				}

	//			}
	//		}

	//	}


	//	//ǿƽ���гֲ�
	//	if(m_futureData_vec.size() % 140 == 0)
	//		TDSpi_stgy->ForceClose();
	//}
	#pragma endregion strategyTemplate

}





//�����Ƿ�������
void Strategy::set_allow_open(bool x)
{
	m_allow_open = x;

	if(m_allow_open == true)
	{
		cerr<<"��ǰ���ý����������"<<endl;


	}
	else if(m_allow_open == false)
	{
		cerr<<"��ǰ���ý������ֹ����"<<endl;

	}

}





//�������ݵ�txt��csv
void Strategy::SaveDataTxtCsv(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	//�������鵽txt
	string date = pDepthMarketData->TradingDay;
	string time = pDepthMarketData->UpdateTime;
	double buy1price = pDepthMarketData->BidPrice1;
	int buy1vol = pDepthMarketData->BidVolume1;
	double new1 = pDepthMarketData->LastPrice;
	double sell1price = pDepthMarketData->AskPrice1;
	int sell1vol = pDepthMarketData->AskVolume1;
	double vol = pDepthMarketData->Volume;
	double openinterest = pDepthMarketData->OpenInterest;//�ֲ���



	string instId = pDepthMarketData->InstrumentID;

	//����date�ĸ�ʽ
	string a = date.substr(0,4);
	string b = date.substr(4,2);
	string c = date.substr(6,2);

	string date_new = a + "-" + b + "-" + c;


	ofstream fout_data("output/" + instId + "_" + date + ".txt",ios::app);

	fout_data<<date_new<<","<<time<<","<<buy1price<<","<<buy1vol<<","<<new1<<","<<sell1price<<","<<sell1vol<<","<<vol<<","<<openinterest<<endl;

	fout_data.close();




	ofstream fout_data_csv("output/" + instId + "_" + date + ".csv",ios::app);

	fout_data_csv<<date_new<<","<<time<<","<<buy1price<<","<<buy1vol<<","<<new1<<","<<sell1price<<","<<sell1vol<<","<<vol<<","<<openinterest<<endl;

	fout_data_csv.close();

}



//�������ݵ�vector
void Strategy::SaveDataVec(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	string date = pDepthMarketData->TradingDay;
	string time = pDepthMarketData->UpdateTime;
	double buy1price = pDepthMarketData->BidPrice1;
	int buy1vol = pDepthMarketData->BidVolume1;
	double new1 = pDepthMarketData->LastPrice;
	double sell1price = pDepthMarketData->AskPrice1;
	int sell1vol = pDepthMarketData->AskVolume1;
	double vol = pDepthMarketData->Volume;
	double openinterest = pDepthMarketData->OpenInterest;//�ֲ���


	futureData.date = date;
	futureData.time = time;
	futureData.buy1price = buy1price;
	futureData.buy1vol = buy1vol;
	futureData.new1 = new1;
	futureData.sell1price = sell1price;
	futureData.sell1vol = sell1vol;
	futureData.vol = vol;
	futureData.openinterest = openinterest;
	m_futureData_vec.push_back(futureData);	

}


void Strategy::set_instMessage_map_stgy(map<string, CThostFtdcInstrumentField*>& instMessage_map_stgy)
{
	m_instMessage_map_stgy = instMessage_map_stgy;
	cerr<<"�յ���Լ����:"<<m_instMessage_map_stgy.size()<<endl;

}





//�����˻���ӯ����Ϣ
void Strategy::CalculateEarningsInfo(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	//���º�Լ�����¼ۣ�û�гֲ־Ͳ���Ҫ���£��гֲֵģ����ǽ��׵ĺ�ԼҲҪ���¡�Ҫ�ȼ���ӯ����Ϣ�ټ�������߼�

	//�жϸú�Լ�Ƿ��гֲ�
	if(TDSpi_stgy->send_trade_message_map_KeyNum(pDepthMarketData->InstrumentID) > 0)
		TDSpi_stgy->setLastPrice(pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice);


	//�����˻��ĸ���ӯ��,�����ּ���
	m_openProfit_account = TDSpi_stgy->sendOpenProfit_account(pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice); 

	//�����˻���ƽ��ӯ��
	m_closeProfit_account = TDSpi_stgy->sendCloseProfit();

	cerr<<" ƽ��ӯ��:"<<m_closeProfit_account<<",����ӯ��:"<<m_openProfit_account<<"��ǰ��Լ:"<<pDepthMarketData->InstrumentID<<" ���¼�:"<<pDepthMarketData->LastPrice<<" ʱ��:"<<pDepthMarketData->UpdateTime<<endl;//double����Ϊ0��ʱ�����-1.63709e-010����С��0�ģ���+1���ֵ��1

	TDSpi_stgy->printTrade_message_map();



}



//��ȡ��ʷ����
void Strategy::GetHistoryData()
{
	vector<string> data_fileName_vec;
	Store_fileName("input/��ʷK������", data_fileName_vec);

	cout<<"��ʷK���ı���:"<<data_fileName_vec.size()<<endl;

	for(vector<string>::iterator iter = data_fileName_vec.begin(); iter != data_fileName_vec.end(); iter++)
	{
		cout<<*iter<<endl;
		ReadDatas(*iter, history_data_vec);

	}

	cout<<"K������:"<<history_data_vec.size()<<endl;

}

//�����ݵ�kdb
void Strategy::DataInsertToKDB(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	string insertstring;
	string date = pDepthMarketData->TradingDay;
	string datesplit = "D";
	string time = pDepthMarketData->UpdateTime;
	string receivedate = return_current_time_and_date();
	string symbol = pDepthMarketData->InstrumentID;
	double bidPrice1 = pDepthMarketData->BidPrice1;
	int    bidvol1 = 1;
	double askPrice1 = pDepthMarketData->AskPrice1;
	int    askvol1 = 1;

	insertstring.append("`Quote insert (");
	insertstring.append(date.substr(0, 4) + ".");
	insertstring.append(date.substr(4, 2) + ".");
	insertstring.append(date.substr(6, 2));
	insertstring.append(datesplit);
	insertstring.append(time);
	insertstring.append(";");
	insertstring.append(receivedate);
	insertstring.append(";");
	insertstring.append("`");
	insertstring.append(symbol);
	insertstring.append(";");
	insertstring.append(to_string(bidPrice1));
	insertstring.append(";");
	insertstring.append(to_string(bidvol1));
	insertstring.append(";");
	insertstring.append(to_string(askPrice1));
	insertstring.append(";");
	insertstring.append(to_string(askvol1));
	insertstring.append(")");
	
	kdbConnector.sync(insertstring.c_str());
}

string  Strategy::return_current_time_and_date()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y.%m.%dD%X");
	auto duration = now.time_since_epoch();

	typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<8>
	>::type> Days; /* UTC: +8:00 */
	Days days = std::chrono::duration_cast<Days>(duration);
	duration -= days;
	auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;
	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	duration -= seconds;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
	duration -= milliseconds;
	auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
	duration -= microseconds;
	auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
	cout << (ss.str()).append(".").append(to_string(milliseconds.count())) << endl;
	return (ss.str()).append(".").append(to_string(milliseconds.count()));
}

void Strategy::SendCorrectOrderOnSymbol(TThostFtdcInstrumentIDType    instId, string order_type, TThostFtdcDirectionType dir, TThostFtdcCombOffsetFlagType * kpp, TThostFtdcPriceType price, TThostFtdcVolumeType vol)
{
	if (order_type == "CLOSE_TODAY_YD_OPEN")
	{
		if (dir == '0')
		{
			if (vol <= TDSpi_stgy->SendTodayHolding_short(instId))
			{
				TDSpi_stgy->ReqOrderInsert(instId,  dir, "3", price, vol);
			}
			else if (vol <= TDSpi_stgy->SendYdHolding_short(instId))
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "1", price, vol);
			}
			else if (vol > TDSpi_stgy->SendYdHolding_short(instId) && vol > TDSpi_stgy->SendTodayHolding_short(instId))
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "0", price, vol);
			}
		}
		else if (dir == '1')
		{
			if (vol <= TDSpi_stgy->SendTodayHolding_long(instId))
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "3", price, vol);
			}
			else if (vol <= TDSpi_stgy->SendYdHolding_long(instId))
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "1", price, vol);
			}
			else if (vol > TDSpi_stgy->SendYdHolding_long(instId) && vol > TDSpi_stgy->SendTodayHolding_long(instId))
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "0", price, vol);
			}
		}
	}
}