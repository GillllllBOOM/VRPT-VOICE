/*
@file
@brief 讯飞录音识别构建类实现

@author		Willis ZHU
@date		2017/3/10
*/

#include <mutex>
#include "VoiceBuild.h"
using namespace std;

//UDP 通讯
extern WSADATA wsaData;
extern SOCKET RecvSocket;
extern sockaddr_in RecvAddr;
extern sockaddr_in SenderAddr;
extern int SenderAddrSize;

//退出锁
mutex mEnd;

const char * ASR_RES_PATH = "fo|res/asr/common.jet";  //离线语法识别资源路径
#ifdef _WIN64
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld_x64";  //构建离线语法识别网络生成数据保存路径
#else
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld";  //构建离线语法识别网络生成数据保存路径
#endif

VoiceBuild::VoiceBuild(){
	vr = new VoiceRec;
	asr_params[MAX_PARAMS_LEN] = { NULL };
	vad_eos = 500;
	accent = "mandarin";
	GRM_FILE = "UI.bnf"; 
}

void VoiceBuild::set_accent(int accent_status){
	switch (accent_status){
	case 1://粤语
		accent = "cantonese";
		break;
	case 2://四川话
		accent = "lmz";
		break;
	case 3://河南话
		accent = "henanese";
		break;
	case 4://东北话
		accent = "dongbeiese";
		break;
	default://普通话
		accent = "mandarin";
		break;
	}
}

void VoiceBuild::set_place(int place, int eos){
	vad_eos = eos;//最长静默时间
	switch (place){
	case 1://返回
		GRM_FILE = "back.bnf";
		break;
	case 2://日常巡检
		GRM_FILE = "daily0.bnf";
		break;
	case 3:
		GRM_FILE = "test.bnf";
		break;
	case 4:
		GRM_FILE = "emergency2.bnf";
		break;
	case 5://应急处置
		GRM_FILE = "commander0.bnf";
		break;
	default://主界面
		GRM_FILE = "UI.bnf";
		break;
	}
}

void VoiceBuild::set_place(char* place, int eos) {
	vad_eos = eos;//最长静默时间
	GRM_FILE = place;
}


//构建语法回调接口
int build_grm_cb(int ecode, const char *info, void *udata)
{
	UserData *grm_data = (UserData *)udata;

	if (NULL != grm_data) {
		grm_data->build_fini = 1;
		grm_data->errcode = ecode;
	}

	if (MSP_SUCCESS == ecode && NULL != info) {
		printf("构建语法成功！ 语法ID:%s\n", info);
		if (NULL != grm_data)
			_snprintf(grm_data->grammar_id, MAX_GRAMMARID_LEN - 1, info);
	}
#ifdef __DEBUG__
	else
		printf("构建语法失败！%d\n", ecode);
#endif 

	return 0;
}

//读取BNF构建语法，生成语法id
void VoiceBuild::build_grammar()
{
	try {
		FILE *grm_file = NULL;
		char *grm_content = NULL;
		unsigned int grm_cnt_len = 0;
		char grm_build_params[MAX_PARAMS_LEN] = { NULL };
		int ret = 0;
		//初始化新的udata
		if (udata) delete udata;
		udata = new UserData;

		printf("文件名：%s\n", GRM_FILE);				//语法文件
		grm_file = fopen(GRM_FILE, "rb");
		if (NULL == grm_file) {
			char error[1024];
			sprintf(error, "打开\"%s\"文件失败！[%s]\n", GRM_FILE, strerror(errno));
			throw error;
		}

		fseek(grm_file, 0, SEEK_END);
		grm_cnt_len = ftell(grm_file);
		fseek(grm_file, 0, SEEK_SET);

		grm_content = (char *)malloc(grm_cnt_len + 1);
		if (NULL == grm_content)
		{
			printf("内存分配失败!\n");
			fclose(grm_file);
			grm_file = NULL;
		}
		fread((void*)grm_content, 1, grm_cnt_len, grm_file);
		grm_content[grm_cnt_len] = '\0';
		fclose(grm_file);
		grm_file = NULL;

		_snprintf(grm_build_params, MAX_PARAMS_LEN - 1,
			"engine_type = local, \
				asr_res_path = %s, sample_rate = %d, \
						grm_build_path = %s, ",
			ASR_RES_PATH,
			SAMPLE_RATE_16K,
			GRM_BUILD_PATH
		);

		//构建语法
		QISRBuildGrammar("bnf", grm_content, grm_cnt_len, grm_build_params, build_grm_cb, udata);

#ifdef __DEBUG__
		printf("build_fini = %d, grammar_id = %s\n", udata->build_fini, udata->grammar_id);
#endif
		free(grm_content);
		grm_content = NULL;
	}
	catch (char* error) {
		printf("%s\n", error);
		size_t size = strlen(error);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, error, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;
		MessageBox(NULL, TEXT("语音端启动失败，找不到对应语法文件!请检查后重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (std::exception& e) {
		char errorcode[1024];
		sprintf(errorcode, e.what());
		size_t size = sizeof(errorcode);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;
		MessageBox(NULL, TEXT("语音端构建失败!请重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (...) {
		MessageBox(NULL, TEXT("语音端构建失败!请重新启动。"), TEXT("语音端错误"), MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	}
			
	return;
}

//更新词典回调函数
int update_lex_cb(int ecode, const char *info, void *udata)
{
	UserData *lex_data = (UserData *)udata;

	if (NULL != lex_data) {
		lex_data->update_fini = 1;
		lex_data->errcode = ecode;
	}

	if (MSP_SUCCESS == ecode)
		printf("更新词典成功！\n");
	else
		printf("更新词典失败！%d\n", ecode);

	return 0;
}

//更新本地语法词典
//int VoiceBuild::update_lexicon()
//{
//	const char *lex_content = "";
//	unsigned int lex_cnt_len = strlen(lex_content);
//	char update_lex_params[MAX_PARAMS_LEN] = { NULL };
//
//	_snprintf(update_lex_params, MAX_PARAMS_LEN - 1,
//		"engine_type = local, text_encoding = GB2312, \
//				asr_res_path = %s, sample_rate = %d, \
//						grm_build_path = %s, grammar_list = %s, ",
//						ASR_RES_PATH,
//						SAMPLE_RATE_16K,
//						GRM_BUILD_PATH,
//						udata->grammar_id);
//	return QISRUpdateLexicon(LEX_NAME, lex_content, lex_cnt_len, update_lex_params, update_lex_cb, udata);
//}

int VoiceBuild::start_listen(){
	int errcode = 0;
	if (!vr) return -10;

	vr->set_signal(EVT_START);
	if (errcode = vr->sr_start_listening()) {
		printf("start listen failed %d\n", errcode);
	}
	return errcode;
}

int VoiceBuild::stop_listen(){
	int errcode = 0;
	if (!vr) return -10;

	vr->set_signal(EVT_STOP);
	if (errcode = vr->sr_stop_listening()) {
		printf("stop listen failed %d\n", errcode);
	}
	return errcode;
}

void VoiceBuild::quit_listen(){
	if (vr){
		
	/*因为在另一条线程进行识别，存在还在获取识别结果时退出了识别session，导致查询了空session，从而报错。
	**所以在退出和获取结果前后加锁。
	*/
		
		vr->set_signal(EVT_QUIT);
		vr->sr_stop_listening();
		if (vr->get_sr() && vr->get_sr()->session_id != NULL) {
			mEnd.lock();
			QISRSessionEnd(vr->get_sr()->session_id, "quit");
			vr->get_sr()->session_id = NULL;	
			mEnd.unlock();
		}
		
	}
	return;
}



int VoiceBuild::run_asr(){
	int errcode;
	int i = 0;

	errcode = vr->sr_init(asr_params, SR_MIC, DEFAULT_INPUT_DEVID);//初始化
	if (errcode) {
		printf("speech recognizer init failed\n");
		return errcode;
	}

	errcode = start_listen();
	if (errcode){
		printf("VoiceBuild start listen failed\n");
		goto exit;
	}

	while (1){
		char isquit = 0;
		switch (vr->get_signal()) {
		case EVT_START://如果在从recorder监听开始识别
			vr->sr_start_recognize();
			break;
		case EVT_QUIT:
			isquit = 1;
			break;
		default:
			break;
		}
		if (isquit){
			break;
		}
		Sleep(100);
	}
exit:
	vr->sr_uninit();
	return 0;
}

////////////////////////////////////////////////////


void VoiceBuild::build_asr_params() 
{
	try {
		int ret = 0;
		build_grammar();  //第一次使用某语法进行识别，需要先构建语法网络，获取语法ID，之后使用此语法进行识别，无需再次构建
		while (1 != udata->build_fini)
			Sleep(300);
		if (MSP_SUCCESS != udata->errcode)
			throw udata->errcode;
		printf("离线识别语法网络构建完成，开始识别...\n");
		//离线语法识别参数设置
		/*engine_type:引擎类型 cloud/local/mixed 默认:cloud
		**language:语言 zh_cn/zh_tw/en_us 默认:zh_cn
		**accent：语言区域
		**sample_rate：音频采样率  16000,8000 默认:16000
		**asr_threshold：识别门限 0~100 默认:0   置信度大于门限值才返回结果
		**asr_denoise：是否开启降噪  0：不开启，1：开启  默认不开启            待测试
		**asr_res_path：离线识别资源路径
		**grm_build_path：离线语法生成路径  数据保存路径（文件夹）
		**result_type：结果格式  plain,json,xml
		**text_encoding：文本编码格式
		**vad_enable：VAD功能开关  0为关闭 默认开启
		**vad_eos：允许尾部静音的最长时间  0-10000毫秒。 默认为2000 如果尾部静音时长超过了此值，则认为用户音频已经结束，此参数仅在打开VAD功能时有效
		**local_grammar：离线语法id
		**.......
		*/
		_snprintf(asr_params, MAX_PARAMS_LEN - 1,
			"engine_type = local, accent = %s,  vad_eos = %d, asr_threshold = 1, \
				asr_res_path = %s, sample_rate = %d, asr_denoise = 1,\
						grm_build_path = %s, local_grammar = %s, \
								result_type = json, result_encoding = GB2312, ",
			accent,
			vad_eos,
			ASR_RES_PATH,
			SAMPLE_RATE_16K,
			GRM_BUILD_PATH,
			udata->grammar_id
		);

		ret = run_asr();
		if (ret) {
			throw ret;
		}
	}
	catch (int d) {
		char errorcode[1024];
		sprintf(errorcode, "Error Code:%d", d);
		size_t size = sizeof(errorcode);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;

		char SendBuf[1024];
		int BufLen = 1024;
		switch (d) {
		case -E_SR_NOACTIVEDEVICE: {
			strcpy(SendBuf, "speechError#microphoneOffline#");
			/*if (strlen(SendBuf) != 0) {
				int isend = sendto(RecvSocket, SendBuf, BufLen, 0, (sockaddr *)&SenderAddr, SenderAddrSize);
				if (isend == SOCKET_ERROR) {
					printf("sendto()failed：%d\n", WSAGetLastError());
				}
			}*/
			//MessageBox(NULL, TEXT("语音端启动失败，没有语音输入设备!请检查后重新启动。"), buffer, MB_ICONEXCLAMATION);
			break;
		}
		case -E_SR_NOMEM:
			MessageBox(NULL, TEXT("语音端启动失败，空间溢出!请检查后重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
			break;
		case  -E_SR_RECORDFAIL:
			MessageBox(NULL, TEXT("语音端启动失败，语音输入设备调用故障!请检查后重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
			break;
		default:
			MessageBox(NULL, TEXT("语音端启动失败！语法文件错误，请检查后重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
			break;
		}	
		delete buffer;
	}
	catch (std::exception& e) {
		char errorcode[1024];
		sprintf(errorcode, e.what());
		size_t size = sizeof(errorcode);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;
		MessageBox(NULL, TEXT("语音端启动失败!请重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (...) {
		MessageBox(NULL, TEXT("语音端启动失败!请重新启动。"), TEXT("语音端错误"), MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	}

	return;
}


int VoiceBuild::start_build()
{
	try {
		const char *login_config = "appid = 5932a3b6"; //登录参数
		int ret = 0;

		ret = MSPLogin(NULL, NULL, login_config); //第一个参数为用户名，第二个参数为密码，传NULL即可，第三个参数是登录参数
		if (MSP_SUCCESS != ret) {
			throw ret;
		}

		build_asr_params();
	}
	catch (int d) {
		char errorcode[1024];
		sprintf(errorcode, "Error Code:%d", d);
		size_t size = sizeof(errorcode);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;
		MessageBox(NULL, TEXT("语音端讯飞登录失败，appid失效!请联系开发者。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (std::exception& e) {
		char errorcode[1024];
		sprintf(errorcode, e.what());
		size_t size = sizeof(errorcode);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;
		MessageBox(NULL, TEXT("语音端登陆失败!请重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (...) {
		MessageBox(NULL, TEXT("语音端登录失败!请重新启动。"), TEXT("语音端错误"), MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		
	}

	MSPLogout();
	return 0;
}
