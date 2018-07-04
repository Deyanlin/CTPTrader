//���������룺SHFE/DCE/CZCE/CFFEX(������/����/֣��/�н���)



/**********************************************************************
��Ŀ����: CTP�ڻ�����ϵͳ(�콢��)
����޸ģ�20150508
�������ߣ���ӯ���������Ŷӣ��ٷ�QQȺ��202367118


**********************************************************************/

#include <afx.h>
#include "ctp.h"
#include "mdspi.h"
#include "traderspi.h"
#include "strategy.h"
#include <iostream>
#include <string>
#include <cstdio>



int kdbPort = 9000;

int requestId=0;//������

HANDLE g_hEvent;//���߳��õ��ľ��

Strategy* g_strategy;//������ָ��

CtpTraderSpi* g_pUserSpi_tradeAll;//ȫ�ֵ�TD�ص�����������˻������������õ�

//�˻���������
DWORD WINAPI ThreadProc(LPVOID lpParameter);





int main(int argc, const char* argv[])
{

	g_hEvent=CreateEvent(NULL, true, false, NULL); 

	SetConsoleTitle("CTP�ڻ�����ϵͳ_�콢��");

	//--------------��ȡ�����ļ�����ȡ�˻���Ϣ����������ַ�����׵ĺ�Լ����--------------
	ReadMessage readMessage;
	memset(&readMessage, 0, sizeof(readMessage));
	SetMessage(readMessage, kdbPort);
	kdbGetData();
	

	//--------------��ʼ������UserApi����������APIʵ��----------------------------------
	CThostFtdcMdApi* pUserApi_md = CThostFtdcMdApi::CreateFtdcMdApi(".\\MDflow\\");
	CtpMdSpi* pUserSpi_md = new CtpMdSpi(pUserApi_md);//�����ص����������MdSpi
	pUserApi_md->RegisterSpi(pUserSpi_md);// �ص�����ע��ӿ���
	pUserApi_md->RegisterFront(readMessage.m_mdFront);// ע������ǰ�õ�ַ	
	pUserSpi_md->setAccount(readMessage.m_appId, readMessage.m_userId, readMessage.m_passwd);//���͹�˾��ţ��û���������
	pUserSpi_md->setInstId(readMessage.m_read_contract);//MD���趩������ĺ�Լ�������Խ��׵ĺ�Լ



	//--------------��ʼ������UserApi����������APIʵ��----------------------------------
	CThostFtdcTraderApi* pUserApi_trade = CThostFtdcTraderApi::CreateFtdcTraderApi(".\\TDflow\\");
	CtpTraderSpi* pUserSpi_trade = new CtpTraderSpi(pUserApi_trade, pUserApi_md, pUserSpi_md);//���캯����ʼ����������
	pUserApi_trade->RegisterSpi((CThostFtdcTraderSpi*)pUserSpi_trade);// ע���¼���
	pUserApi_trade->SubscribePublicTopic(THOST_TERT_RESTART);// ע�ṫ����
	pUserApi_trade->SubscribePrivateTopic(THOST_TERT_QUICK);// ע��˽����THOST_TERT_QUICK
	pUserApi_trade->RegisterFront(readMessage.m_tradeFront);// ע�ύ��ǰ�õ�ַ
	pUserSpi_trade->setAccount(readMessage.m_appId, readMessage.m_userId, readMessage.m_passwd);//���͹�˾��ţ��û���������	
	pUserSpi_trade->setInstId(readMessage.m_read_contract);//���Խ��׵ĺ�Լ

	g_pUserSpi_tradeAll = pUserSpi_trade;//ȫ�ֵ�TD�ص�����������˻������������õ�




	//--------------��������ʵ��--------------------------------------------------------
	g_strategy = new Strategy(pUserSpi_trade);
	g_strategy->Init(readMessage.m_read_contract, kdbPort, kdbScriptExePath);
	



	//--------------TD�߳�������--------------------------------------------------------	
	pUserApi_trade->Init();//TD��ʼ����ɺ��ٽ���MD�ĳ�ʼ��
	



	//--------------�˻�����ģ��--------------------------------------------------------
	HANDLE hThread1=CreateThread(NULL,0,ThreadProc,NULL,0,NULL);
	CloseHandle(hThread1);
	WaitForSingleObject(hThread1, INFINITE);
	

	timer_start(kdbSetData, 60000);
	pUserApi_md->Join();//�ȴ��ӿ��߳��˳�
	pUserApi_trade->Join();
	
	while (true);
}



//�˻���������
DWORD WINAPI ThreadProc(
	LPVOID lpParameter
	)
{
	string str;

	int a;
	cerr<<"--------------------------------------------------------�˻���������������"<<endl;
	cerr<<endl<<"������ָ��(�鿴�ֲ�:show,ǿƽ�ֲ�:close,������:yxkc, ��ֹ����:jzkc)��";

	while(1)
	{
		cin>>str;
		if(str == "show")
			a = 0;	
		else if(str == "close")
			a = 1;
		else if(str == "yxkc")
			a = 2;
		else if(str == "jzkc")
			a = 3;
		else
			a = 4;

		switch(a){
		case 0:
			{		
				cerr<<"�鿴�˻��ֲ�:"<<endl;
				g_pUserSpi_tradeAll->printTrade_message_map();
				cerr<<"������ָ��(�鿴�ֲ�:show,ǿƽ�ֲ�:close,������:yxkc, ��ֹ����:jzkc)��"<<endl;
				break;
			}
		case 1:
			{
				cerr<<"ǿƽ�˻��ֲ�:"<<endl;
				g_pUserSpi_tradeAll->ForceClose();
				cerr<<"������ָ��(�鿴�ֲ�:show,ǿƽ�ֲ�:close,������:yxkc, ��ֹ����:jzkc)��"<<endl;
				break;
			}
		case 2:
			{
				cerr<<"������:"<<endl;
				g_strategy->set_allow_open(true);
				cerr<<"������ָ��(�鿴�ֲ�:show,ǿƽ�ֲ�:close,������:yxkc, ��ֹ����:jzkc)��"<<endl;
				break;

			}
		case 3:
			{
				cerr<<"��ֹ����:"<<endl;
				g_strategy->set_allow_open(false);
				cerr<<"������ָ��(�鿴�ֲ�:show,ǿƽ�ֲ�:close,������:yxkc, ��ֹ����:jzkc)��"<<endl;
				break;

			}
		case 4:
			{
				cerr<<endl<<"�����������������ָ��(�鿴�ֲ�:show,ǿƽ�ֲ�:close,������:yxkc, ��ֹ����:jzkc)��"<<endl;
				break;
			}


		}

	}

	return 0;

}