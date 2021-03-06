#include "strategy.h"
#include <fstream>
#include "kdb_function.h"
#include <stdlib.h>


using namespace std;
using namespace kdb;

kdb::Connector kdbConnector;
string m_kdbScript = "TRE_IAD_V2T1_[IC_IH_A50]_K60V50B50N0d5S1d5_HIT_Long.q";
int m_ShortLongInitNum = 0;
int m_ReadShortLongInitNum = 0;
int m_ReadShortLongSignal = 0;
TThostFtdcVolumeType m_VolumeTarget = 1;
int m_OrderVolumeMultiple = 1;
int m_IndexFuturePositionLimit = 20;
int m_ForwardPositionLimit = 20;

void Strategy::OnTickData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{	
	//计算账户的盈亏信息
	CalculateEarningsInfo(pDepthMarketData);

	if(strcmp(pDepthMarketData->InstrumentID, m_instId) == 0)
	{
		cerr<<"策略收到行情:"<<pDepthMarketData->InstrumentID<<","<<pDepthMarketData->TradingDay<<","<<pDepthMarketData->UpdateTime<<",最新价:"<<pDepthMarketData->LastPrice<<",涨停价:"<<pDepthMarketData->UpperLimitPrice<<",跌停价:"<<pDepthMarketData->LowerLimitPrice<<endl;
		
		//保存数据到vector
		SaveDataVec(pDepthMarketData);

		//保存数据到txt和csv
		SaveDataTxtCsv(pDepthMarketData);

		//撤单追单,撤单成功后再追单
		TDSpi_stgy->CancelOrder(pDepthMarketData->UpdateTime, pDepthMarketData->LastPrice);
	
		//开仓平仓（策略主逻辑的计算）
		StrategyCalculate(pDepthMarketData);


	}
}



//设置交易的合约代码
void Strategy::Init(string instId, int kdbPort, string kdbScrpit)
{
	strcpy_s(m_instId, instId.c_str());
	kdbConnector.connect("localhost", kdbPort);	
	std::string kdb_init_string = "h:hopen `::5000;PairFormula:{(x%y)};f:{x%y};bollingerBands: {[k;n;data]      movingAvg: mavg[n;data];    md: sqrt mavg[n;data*data]-movingAvg*movingAvg;      movingAvg+/:(k*-1 0 1)*\\:md};;isTable:{if[98h=type x;:1b];if[99h=type x;:98h=type key x];0b};isTable2: {@[{isTable value x}; x; 0b]};isTable:{if[98h=type x;:1b];if[99h=type x;:98h=type key x];0b};isTable2: {@[{isTable value x}; x; 0b]};PairFormulaDValue:{[x;mx;y;my] (x*mx) - (y*my)};";
	std::string kdb_init_par_string = "PositionAddedTimes:0;LocalHigh:-99999999;LocalLow:99999999;DrawBack: 0.0;";
	kdbConnector.sync(kdb_init_string.c_str());
	m_kdbScript = kdbScrpit + m_kdbScript;
	////////////////////////////////////////////////////
	/////第一次启动需要手动加载表                  //////
	///////////////////////////////////////////////////
	kdb::Result res = kdbConnector.sync("isTable2 `QuoteData");
	//如果表格不存在设置初始信号为0
	if (res.res_->g)
	{
		res = kdbConnector.sync("exec count Signal from ShortLong");
		m_ShortLongInitNum = res.res_->j;
	}
	else
	{
		kdbConnector.sync(kdb_init_par_string.c_str());
		kdbConnector.sync("QuoteData:([] Date:();`float$LegOneBidPrice1:();`float$LegOneAskPrice1:();`float$LegTwoBidPrice1:();`float$LegTwoAskPrice1:();`float$BidPrice1:();`float$AskPrice1:());");
		kdbConnector.sync("QuoteDataKindle: select Date:last Date,BidPrice1Open:first BidPrice1,BidPrice1High:max BidPrice1,BidPrice1Low:min BidPrice1, BidPrice1Close:last BidPrice1,BidVol1:100,AskPrice1Open:first AskPrice1,AskPrice1High:max AskPrice1,AskPrice1Low:min AskPrice1, AskPrice1Close:last AskPrice1,AskVol1:last 100,LegOneBidPrice1:last LegOneBidPrice1, LegOneAskPrice1:last LegOneAskPrice1, LegTwoBidPrice1:last LegTwoBidPrice1, LegTwoAskPrice1: last LegTwoAskPrice1 by 60 xbar Date.second from QuoteData;delete second from `QuoteDataKindle;");
		kdbConnector.sync("ShortLong:select from QuoteDataKindle;update Signal: 0N from `ShortLong;");
		m_ShortLongInitNum = 0;
	}


}



//策略主逻辑的计算，70条行情里涨了0.6元，则做多，下跌则做空
void Strategy::StrategyCalculate(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	TThostFtdcInstrumentIDType    instId;//合约,合约代码在结构体里已经有了
	strcpy_s(instId, m_instId);
	string instIDString = instId;
	string order_type = TDSpi_stgy->Send_TThostFtdcCombOffsetFlagType(instIDString.substr(0,2));
	m_ForwardPositionLimit = abs(TDSpi_stgy->SendTodayHolding_short(m_instId) - TDSpi_stgy->SendTodayHolding_long(m_instId)) + TDSpi_stgy->SendTodayHolding_short(m_instId) + TDSpi_stgy->SendTodayHolding_long(m_instId);
	

	TThostFtdcDirectionType       dir;//方向,'0'买，'1'卖
	TThostFtdcCombOffsetFlagType  kpp = "8";//开平，"0"开，"1"平,"3"平今
	TThostFtdcPriceType           price;//价格，0是市价,上期所不支持
	TThostFtdcVolumeType          vol;//数量
	if (m_ForwardPositionLimit <= m_IndexFuturePositionLimit)
	{
		kdbConnector.sync(m_kdbScript.c_str());
	}	
	kdb::Result res = kdbConnector.sync("exec count Signal from ShortLong");
	m_ReadShortLongInitNum = res.res_->j;

	if (m_ReadShortLongInitNum > m_ShortLongInitNum)
	{
		m_ShortLongInitNum = m_ReadShortLongInitNum;
		kdb::Result res = kdbConnector.sync("exec last Signal from ShortLong");
		m_ReadShortLongSignal = res.res_->j;

		if (m_ReadShortLongSignal > 0)
		{
			dir = '0';
			price = pDepthMarketData->AskPrice1;
			vol = m_VolumeTarget * m_OrderVolumeMultiple;
			SendCorrectOrderOnSymbol(instId, order_type, dir, &kpp, price, vol);
		}
		else if (m_ReadShortLongSignal == 0)
		{
			res = kdbConnector.sync("CoverShortLong:-2#select  from ShortLong;exec first Signal from CoverShortLong");
			dir = '1';
			price = pDepthMarketData->BidPrice1;
			vol = res.res_->j *m_OrderVolumeMultiple;
			SendCorrectOrderOnSymbol(instId, order_type, dir, &kpp, price, vol);
		}
	}

	#pragma region strategyTemplate
	//if(m_futureData_vec.size() >= 70)
	//{
	//	//持仓查询，进行仓位控制
	//	if(strcmp(pDepthMarketData->InstrumentID, m_instId) == 0)
	//	{
	//		if(m_futureData_vec.size() % 20 == 0)
	//		{
	//			if(TDSpi_stgy->SendHolding_long(m_instId) + TDSpi_stgy->SendHolding_short(m_instId) < 3)//多空共满仓3手
	//			{
	//				//做多
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
	//				//做空
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


	//	//强平所有持仓
	//	if(m_futureData_vec.size() % 140 == 0)
	//		TDSpi_stgy->ForceClose();
	//}
	#pragma endregion strategyTemplate

}





//设置是否允许开仓
void Strategy::set_allow_open(bool x)
{
	m_allow_open = x;

	if(m_allow_open == true)
	{
		cerr<<"当前设置结果：允许开仓"<<endl;


	}
	else if(m_allow_open == false)
	{
		cerr<<"当前设置结果：禁止开仓"<<endl;

	}

}





//保存数据到txt和csv
void Strategy::SaveDataTxtCsv(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	//保存行情到txt
	string date = pDepthMarketData->TradingDay;
	string time = pDepthMarketData->UpdateTime;
	double buy1price = pDepthMarketData->BidPrice1;
	int buy1vol = pDepthMarketData->BidVolume1;
	double new1 = pDepthMarketData->LastPrice;
	double sell1price = pDepthMarketData->AskPrice1;
	int sell1vol = pDepthMarketData->AskVolume1;
	double vol = pDepthMarketData->Volume;
	double openinterest = pDepthMarketData->OpenInterest;//持仓量



	string instId = pDepthMarketData->InstrumentID;

	//更改date的格式
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



//保存数据到vector
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
	double openinterest = pDepthMarketData->OpenInterest;//持仓量


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
	cerr<<"收到合约个数:"<<m_instMessage_map_stgy.size()<<endl;

}





//计算账户的盈亏信息
void Strategy::CalculateEarningsInfo(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	//更新合约的最新价，没有持仓就不需要更新，有持仓的，不是交易的合约也要更新。要先计算盈亏信息再计算策略逻辑

	//判断该合约是否有持仓
	if(TDSpi_stgy->send_trade_message_map_KeyNum(pDepthMarketData->InstrumentID) > 0)
		TDSpi_stgy->setLastPrice(pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice);


	//整个账户的浮动盈亏,按开仓价算
	m_openProfit_account = TDSpi_stgy->sendOpenProfit_account(pDepthMarketData->InstrumentID, pDepthMarketData->LastPrice); 

	//整个账户的平仓盈亏
	m_closeProfit_account = TDSpi_stgy->sendCloseProfit();

	cerr<<" 平仓盈亏:"<<m_closeProfit_account<<",浮动盈亏:"<<m_openProfit_account<<"当前合约:"<<pDepthMarketData->InstrumentID<<" 最新价:"<<pDepthMarketData->LastPrice<<" 时间:"<<pDepthMarketData->UpdateTime<<endl;//double类型为0有时候会是-1.63709e-010，是小于0的，但+1后的值是1

	TDSpi_stgy->printTrade_message_map();



}



//读取历史数据
void Strategy::GetHistoryData()
{
	vector<string> data_fileName_vec;
	Store_fileName("input/历史K线数据", data_fileName_vec);

	cout<<"历史K线文本数:"<<data_fileName_vec.size()<<endl;

	for(vector<string>::iterator iter = data_fileName_vec.begin(); iter != data_fileName_vec.end(); iter++)
	{
		cout<<*iter<<endl;
		ReadDatas(*iter, history_data_vec);

	}

	cout<<"K线条数:"<<history_data_vec.size()<<endl;

}

//存数据到kdb
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
	#pragma region CLOSE_TODAY_YD_OPEN
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
	#pragma endregion CLOSE_TODAY_YD_OPEN

	#pragma region CLOSE_TODAY_YD_OPEN
	if (order_type == "CLOSE_IF_TODAYPOS_OPEN")
	{
		if (dir == '0')
		{
			if      (TDSpi_stgy->SendTodayHolding_short(instId) > 0 && m_IndexFuturePositionLimit - TDSpi_stgy->SendTodayHolding_short(instId) - TDSpi_stgy->SendTodayHolding_long(instId) >= vol)
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "0", price, vol);
			}
			else if (TDSpi_stgy->SendTodayHolding_short(instId) == 0 && m_IndexFuturePositionLimit - TDSpi_stgy->SendTodayHolding_short(instId) - TDSpi_stgy->SendTodayHolding_long(instId) >= vol)
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "0", price, vol);
			}
			else if (TDSpi_stgy->SendTodayHolding_short(instId) == 0 && TDSpi_stgy->SendYdHolding_short(instId) >= vol)
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "1", price, vol);
			}
		}
		else if (dir == '1')
		{
			if		(TDSpi_stgy->SendTodayHolding_long(instId) > 0 && m_IndexFuturePositionLimit - TDSpi_stgy->SendTodayHolding_short(instId) - TDSpi_stgy->SendTodayHolding_long(instId) >= vol)
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "0", price, vol);
			}
			else if (TDSpi_stgy->SendTodayHolding_long(instId) == 0 && m_IndexFuturePositionLimit - TDSpi_stgy->SendTodayHolding_short(instId) - TDSpi_stgy->SendTodayHolding_long(instId) >= vol)
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "0", price, vol);
			}
			else if (TDSpi_stgy->SendTodayHolding_long(instId) == 0 && TDSpi_stgy->SendYdHolding_long(instId) >= vol)
			{
				TDSpi_stgy->ReqOrderInsert(instId, dir, "1", price, vol);
			}
		}
	}
	#pragma endregion CLOSE_TODAY_YD_OPEN
}