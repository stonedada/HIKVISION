#include <stdio.h>
#include <iostream>
#include "Windows.h"
#include "HCNetSDK.h"
using namespace std;

//时间解析宏定义
#define GET_YEAR(_time_)      (((_time_)>>26) + 2000) 
#define GET_MONTH(_time_)     (((_time_)>>22) & 15)
#define GET_DAY(_time_)       (((_time_)>>17) & 31)
#define GET_HOUR(_time_)      (((_time_)>>12) & 31) 
#define GET_MINUTE(_time_)    (((_time_)>>6)  & 63)
#define GET_SECOND(_time_)    (((_time_)>>0)  & 63)

void CALLBACK cbMessageCallback(LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser)
{
	switch (lCommand)
	{
	case COMM_UPLOAD_FACESNAP_RESULT: //人脸抓拍报警信息
	{
		NET_VCA_FACESNAP_RESULT struFaceSnap = { 0 };
		memcpy(&struFaceSnap, pAlarmInfo, sizeof(NET_VCA_FACESNAP_RESULT));

		NET_DVR_TIME struAbsTime = { 0 };
		struAbsTime.dwYear = GET_YEAR(struFaceSnap.dwAbsTime);
		struAbsTime.dwMonth = GET_MONTH(struFaceSnap.dwAbsTime);
		struAbsTime.dwDay = GET_DAY(struFaceSnap.dwAbsTime);
		struAbsTime.dwHour = GET_HOUR(struFaceSnap.dwAbsTime);
		struAbsTime.dwMinute = GET_MINUTE(struFaceSnap.dwAbsTime);
		struAbsTime.dwSecond = GET_SECOND(struFaceSnap.dwAbsTime);

		//保存抓拍场景图片
		if (struFaceSnap.dwBackgroundPicLen > 0 && struFaceSnap.pBuffer2 != NULL)
		{
			char cFilename[256] = { 0 };
			HANDLE hFile;
			DWORD dwReturn;

			char chTime[128];
			sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay, struAbsTime.dwHour, struAbsTime.dwMinute, struAbsTime.dwSecond);

			sprintf(cFilename, "FaceSnapBackPic.jpg", struFaceSnap.struDevInfo.struDevIP.sIpV4, chTime);

			hFile = CreateFile((LPCWSTR)cFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				break;
			}
			WriteFile(hFile, struFaceSnap.pBuffer2, struFaceSnap.dwBackgroundPicLen, &dwReturn, NULL);
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}

		//保存抓取的人脸图片
		if (struFaceSnap.dwBackgroundPicLen > 0 && struFaceSnap.pBuffer2 != NULL)
		{
			char cFilename[256] = { 0 };
			HANDLE hFile;
			DWORD dwReturn;

			char chTime[128];
			sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay, struAbsTime.dwHour, struAbsTime.dwMinute, struAbsTime.dwSecond);

			sprintf(cFilename, "2FaceSnapBackPic.jpg", struFaceSnap.struDevInfo.struDevIP.sIpV4, chTime);

			hFile = CreateFile((LPCWSTR)cFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				break;
			}
			WriteFile(hFile, struFaceSnap.pBuffer1, struFaceSnap.dwFacePicLen, &dwReturn, NULL);
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}
		//------------------------------------------------------
		// 
		//-----------------------------------------------------



		printf("人脸抓拍报警[0x%x]: Abs[%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d] Dev[ip:%s,port:%d,ivmsChan:%d] \n", \
			lCommand, struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay, struAbsTime.dwHour, \
			struAbsTime.dwMinute, struAbsTime.dwSecond, struFaceSnap.struDevInfo.struDevIP.sIpV4, \
			struFaceSnap.struDevInfo.wPort, struFaceSnap.struDevInfo.byIvmsChannel);
	}
	break;
	default:
		printf("其他报警，报警信息类型: 0x%x\n", lCommand);
		break;
	}

	return;
}

void main() {
	//---------------------------------------
	// 初始化
	NET_DVR_Init();
	//设置连接时间与重连时间
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);

	//---------------------------------------
	// 注册设备
	LONG lUserID;

	//登录参数，包括设备地址、登录用户、密码等
	NET_DVR_USER_LOGIN_INFO struLoginInfo = { 0 };
	struLoginInfo.bUseAsynLogin = 0; //同步登录方式
	strcpy(struLoginInfo.sDeviceAddress, "192.168.1.65"); //设备IP地址
	struLoginInfo.wPort = 8000; //设备服务端口
	strcpy(struLoginInfo.sUserName, "admin"); //设备登录用户名
	strcpy(struLoginInfo.sPassword, "abcd1234"); //设备登录密码

	//设备信息, 输出参数
	NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = { 0 };

	lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
	if (lUserID < 0)
	{
		printf("Login failed, error code: %d\n", NET_DVR_GetLastError());
		NET_DVR_Cleanup();
		return;
	}

	//设置报警回调函数
	/*注：多台设备对接时也只需要调用一次设置一个回调函数，不支持不同设备的事件在不同的回调函数里面返回*/
	NET_DVR_SetDVRMessageCallBack_V50(0, cbMessageCallback, NULL);

	//启用布防
	LONG lHandle;
	NET_DVR_SETUPALARM_PARAM  struAlarmParam = { 0 };
	struAlarmParam.dwSize = sizeof(struAlarmParam);
	struAlarmParam.byFaceAlarmDetection = 0; //人脸抓拍报警，上传COMM_UPLOAD_FACESNAP_RESULT类型报警信息
	//其他报警布防参数不需要设置，不支持

	lHandle = NET_DVR_SetupAlarmChan_V41(lUserID, &struAlarmParam);
	if (lHandle < 0)
	{
		printf("NET_DVR_SetupAlarmChan_V41 error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Logout(lUserID);
		NET_DVR_Cleanup();
		return;
	}

	Sleep(50000); //等待过程中，如果设备上传报警信息，在报警回调函数里面接收和处理报警信息

	//撤销布防上传通道
	if (!NET_DVR_CloseAlarmChan_V30(lHandle))
	{
		printf("NET_DVR_CloseAlarmChan_V30 error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Logout(lUserID);
		NET_DVR_Cleanup();
		return;
	}

	//注销用户
	NET_DVR_Logout(lUserID);
	//释放SDK资源
	NET_DVR_Cleanup();
	return;
}
