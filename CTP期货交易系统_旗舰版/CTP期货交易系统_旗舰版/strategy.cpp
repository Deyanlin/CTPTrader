#include "strategy.h"
#include <fstream>
#include "kdb_function.h"


using namespace std;
using namespace kdb;

kdb::Connector kdbConnector;

void Strategy::OnTickData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{	
	//�����˻���ӯ����Ϣ
	CalculateEarningsInfo(pDepthMarketData);

	/*if(strcmp(pDepthMarketData->InstrumentID, m_instId) == 0)
	{*/
		cerr<<"�����յ�����:"<<pDepthMarketData->InstrumentID<<","<<pDepthMarketData->TradingDay<<","<<pDepthMarketData->UpdateTime<<",���¼�:"<<pDepthMarketData->LastPrice<<",��ͣ��:"<<pDepthMarketData->UpperLimitPrice<<",��ͣ��:"<<pDepthMarketData->LowerLimitPrice<<endl;
		
		//�������ݵ�vector
		SaveDataVec(pDepthMarketData);

		//�������ݵ�txt��csv
		SaveDataTxtCsv(pDepthMarketData);

		//����׷��,�����ɹ�����׷��
		TDSpi_stgy->CancelOrder(pDepthMarketData->UpdateTime, pDepthMarketData->LastPrice);
	
		//����ƽ�֣��������߼��ļ��㣩
		StrategyCalculate(pDepthMarketData);

		DataInsertToKDB(pDepthMarketData);
	/*}*/
}



//���ý��׵ĺ�Լ����
void Strategy::Init(string instId, int kdbPort)
{
	strcpy_s(m_instId, instId.c_str());
	kdbConnector.connect("localhost", kdbPort);	
	//kdb::Result res = kdbConnector.sync("CTPQuote:([] Date:();`symbol$Symbol:(); `float$Leg1Bid1:(); `float$Leg1Ask1:(); `float$Leg2Bid1:();`float$Leg2Ask1:())");
}



//�������߼��ļ��㣬70������������0.6Ԫ�������࣬�µ�������
void Strategy::StrategyCalculate(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	TThostFtdcInstrumentIDType    instId;//��Լ,��Լ�����ڽṹ�����Ѿ�����
	TThostFtdcDirectionType       dir;//����,'0'��'1'��
	TThostFtdcCombOffsetFlagType  kpp;//��ƽ��"0"����"1"ƽ,"3"ƽ��
	TThostFtdcPriceType           price;//�۸�0���м�,��������֧��
	TThostFtdcVolumeType          vol;//����


	if(m_futureData_vec.size() >= 70)
	{
		//�ֲֲ�ѯ�����в�λ����
		if(strcmp(pDepthMarketData->InstrumentID, m_instId) == 0)
		{
			if(m_futureData_vec.size() % 20 == 0)
			{
				if(TDSpi_stgy->SendHolding_long(m_instId) + TDSpi_stgy->SendHolding_short(m_instId) < 3)//��չ�����3��
				{
					//����
					if(m_futureData_vec[m_futureData_vec.size()-1].new1 - m_futureData_vec[m_futureData_vec.size()-70].new1 >= 0.6)
					{
						strcpy_s(instId, m_instId);
						dir = '0';
						strcpy_s(kpp, "0");
						price = pDepthMarketData->LastPrice + 3;
						vol = 1;
						
						if(m_allow_open == true)
						{
							TDSpi_stgy->ReqOrderInsert(instId, dir, kpp, price, vol);

						}
					}
					//����
					else if(m_futureData_vec[m_futureData_vec.size()-70].new1 - m_futureData_vec[m_futureData_vec.size()-1].new1 >= 0.6)
					{
						strcpy_s(instId, m_instId);
						dir = '1';
						strcpy_s(kpp, "0");
						price = pDepthMarketData->LastPrice - 3;
						vol = 1;
						if(m_allow_open == true)
						{
							TDSpi_stgy->ReqOrderInsert(instId, dir, kpp, price, vol);

						}
					}

				}
			}

		}


		//ǿƽ���гֲ�
		if(m_futureData_vec.size() % 140 == 0)
			TDSpi_stgy->ForceClose();
	}

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
