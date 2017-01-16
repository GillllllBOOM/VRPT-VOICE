/*
* 语音听写(iFly Auto Transform)技术能够实时地将语音转换成对应的文字。
*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <string>
#include <errno.h>
#include <process.h>
#include <time.h>
#include <list>
#include <iostream>
#include <fstream>

#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "speech_recognizer.h"
using namespace std;

#define FRAME_LEN	640 
#define	BUFFER_SIZE	4096

enum{
	EVT_INIT = 0,
	EVT_START,
	EVT_STOP,
	EVT_QUIT,
	EVT_TOTAL
};
static HANDLE events[EVT_TOTAL] = { NULL, NULL, NULL };

static COORD begin_pos = { 0, 0 };
static COORD last_pos = { 0, 0 };
#define SAMPLE_RATE_16K     (16000)
#define SAMPLE_RATE_8K      (8000)
#define MAX_GRAMMARID_LEN   (32)
#define MAX_PARAMS_LEN      (1024)

const char * ASR_RES_PATH = "fo|res/asr/common.jet";  //离线语法识别资源路径
#ifdef _WIN64
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld_x64";  //构建离线语法识别网络生成数据保存路径
#else
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld";  //构建离线语法识别网络生成数据保存路径
#endif


//const char * LEX_NAME = "greeting"; //更新离线识别语法的槽

typedef struct _UserData {
	int     build_fini;  //标识语法构建是否完成
	int     update_fini; //标识更新词典是否完成
	int     errcode; //记录语法构建或更新词典回调错误码
	char    grammar_id[MAX_GRAMMARID_LEN]; //保存语法构建返回的语法ID
}UserData;

string LOG_FILE ;
extern string AUDIO_FILE;
int vad_eos = 500;

extern DWORD waitres;
extern int signal;

const char *get_audio_file(void); //选择进行离线语法识别的语音文件
int build_grammar(UserData *udata); //构建离线识别语法网络
int update_lexicon(UserData *udata); //更新离线识别语法词典
int run_asr(UserData *udata); //进行离线语法识别

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
	else
		printf("构建语法失败！%d\n", ecode);

	return 0;
}

//读取BNF构建语法，生成语法id
int build_grammar(UserData *udata, char* GRM_FILE)
{
	FILE *grm_file = NULL;
	char *grm_content = NULL;
	unsigned int grm_cnt_len = 0;
	char grm_build_params[MAX_PARAMS_LEN] = { NULL };
	int ret = 0;

	grm_file = fopen(GRM_FILE, "rb");
	if (NULL == grm_file) {
		printf("打开\"%s\"文件失败！[%s]\n", GRM_FILE, strerror(errno));
		return -1;
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
		return -1;
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
	ret = QISRBuildGrammar("bnf", grm_content, grm_cnt_len, grm_build_params, build_grm_cb, udata);

	free(grm_content);
	grm_content = NULL;

	return ret;
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
//int update_lexicon(UserData *udata)
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

static void show_result(char *string, char is_over)
{
	COORD orig, current;
	CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE w = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(w, &info);
	current = info.dwCursorPosition;

	if (current.X == last_pos.X && current.Y == last_pos.Y) {
		SetConsoleCursorPosition(w, begin_pos);
	}
	else {
		/* changed by other routines, use the new pos as start */
		begin_pos = current;
	}
	if (is_over)
		SetConsoleTextAttribute(w, FOREGROUND_GREEN);
	printf("Result: [ %s ]\n", string);
	if (is_over)
		SetConsoleTextAttribute(w, info.wAttributes);

	GetConsoleScreenBufferInfo(w, &info);
	last_pos = info.dwCursorPosition;
}

//
static void show_key_hints(void)
{
	printf("\n\
		   ----------------------------\n\
		   Press r to start speaking\n\
		   Press s to end your speaking\n\
		   Press q to quit\n\
		   ----------------------------\n");
}

/* helper thread: to listen to the keystroke */
static unsigned int  __stdcall helper_thread_proc(void * para)
{
	int key;
	int quit = 0;
	
	int errcode = 0;
	struct speech_rec* asr = (struct speech_rec*)para;

	do {
		key = _getch();
		switch (key) {
		case 'r':
		case 'R':{
			signal = EVT_START;
			if (errcode = sr_start_listening(asr)) {
				printf("start listen failed %d\n", errcode);
				quit = 1;
			}
			break;
		}
		case 's':
		case 'S':{
			signal = EVT_STOP;
			printf("stop\n");
			if (errcode = sr_stop_listening(asr)) {
				printf("stop listening failed %d\n", errcode);
				quit = 1;
			}
			break;
		}
		case 'q':
		case 'Q':{
			sr_stop_listening(asr);
			quit = 1;
			signal = EVT_QUIT;
			PostQuitMessage(0);
			break;
		}
		default:
			break;
		}

		if (quit)
			break;
	} while (1);

	return errcode;
}

//开启键盘监听线程
static HANDLE start_helper_thread(struct speech_rec* para)
{
	HANDLE hdl;

	hdl = (HANDLE)_beginthreadex(NULL, 0, helper_thread_proc, para, 0, NULL);

	return hdl;
}

static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;

string replaceAll(string src, char oldChar, char newChar){
	string head = src;
	for (int i = 0; i < src.size(); i++){
		if (src[i] == oldChar)
			head[i] = newChar;
	}
	return head;
}


void on_result(const char *result, char is_last)
{
	if (result) {
		size_t left = g_buffersize - 1 - strlen(g_result);
		size_t size = strlen(result);
		if (left < size) {
			g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
			if (g_result)
				g_buffersize += BUFFER_SIZE;
			else {
				printf("mem alloc failed\n");
				return;
			}
		}
		strncat(g_result, result, size);

		ofstream out_file;
		out_file.open(LOG_FILE, ios::app);
		if (!out_file) {
			printf("打开\"%s\"文件失败！[%s]\n", LOG_FILE, strerror(errno));
			return;
		}

		time_t temp;
		time(&temp);
		string s = ctime(&temp);
		s = replaceAll(s, '\n', ' ');
		out_file << s << ": " << g_result <<endl<<endl;

		out_file.close();

		show_result(g_result, is_last);
	}
}

//初始化全局结果
void on_speech_begin()
{
	if (g_result)
	{
		free(g_result);
	}
	g_result = (char*)malloc(BUFFER_SIZE);
	g_buffersize = BUFFER_SIZE;
	memset(g_result, 0, g_buffersize);
}

//vad_end detected
void on_speech_end(int reason)
{
	if (reason == END_REASON_VAD_DETECT)//detected speech done
		printf("\n监测到静默\n");
	else
		printf("\nRecognizer error %d\n", reason);
}


static unsigned int  __stdcall rec_mic_thread_proc(void* session_begin_params){
	int errcode;
	int i = 0;
	HANDLE helper_thread = NULL;

	struct speech_rec asr;
	char isquit = 0;
	signal = EVT_INIT;

	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end
	};


	errcode = sr_init(&asr, (char*)session_begin_params, SR_MIC, DEFAULT_INPUT_DEVID, &recnotifier);
	if (errcode) {
		printf("speech recognizer init failed\n");
		return errcode;
	}


	helper_thread = start_helper_thread(&asr);
	if (helper_thread == NULL) {
		printf("create thread failed\n");
		goto exit;
	}

	show_key_hints();
	printf("vad_eos:%d\n",vad_eos);

	while (1){
		switch (signal) {
		case EVT_START:
			sr_start_recognize(&asr);
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
	}
	


exit:
	if (helper_thread != NULL) {
		WaitForSingleObject(helper_thread, INFINITE);
		CloseHandle(helper_thread);
	}

	sr_uninit(&asr);
	return 0;
}

static HANDLE start_mic_thread(char* session_begin_params)
{
	HANDLE hdl;

	hdl = (HANDLE)_beginthreadex(NULL, 0, rec_mic_thread_proc, session_begin_params, 0, NULL);

	return hdl;
}


int run_asr(UserData *udata)
{
	char asr_params[MAX_PARAMS_LEN] = { NULL };
	const char *rec_rslt = NULL;
	const char *session_id = NULL;
	const char *asr_audiof = NULL;
	FILE *f_pcm = NULL;
	char *pcm_data = NULL;
	long pcm_count = 0;
	long pcm_size = 0;
	int last_audio = 0;
	int aud_stat = MSP_AUDIO_SAMPLE_CONTINUE;
	int ep_status = MSP_EP_LOOKING_FOR_SPEECH;
	int rec_status = MSP_REC_STATUS_INCOMPLETE;
	int rss_status = MSP_REC_STATUS_INCOMPLETE;
	int errcode = -1;
	int aud_src = 0;
	int accent_status = 0;
	char *accent = "mandarin";

	HANDLE rec_thread = NULL;
	
	printf("请选择口音? \n0: 普通话\n1: 粤语\n2: 四川话\n3: 河南话\n4: 东北话\n");
	scanf("%d", &accent_status);
	if (aud_src != 0) {
		
	}
	switch (accent_status){
	case 1:
		accent = "cantonese";
		break;
	case 2:
		accent = "lmz";
		break;
	case 3:
		accent = "henanese";
		break;
	case 4:
		accent = "dongbeiese";
		break;
	default:
		break;
	}


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
		"engine_type = local, accent = %s,  vad_eos = %d, \
				asr_res_path = %s, sample_rate = %d, asr_denoise = 0,\
						grm_build_path = %s, local_grammar = %s, \
								result_type = plain, result_encoding = GB2312, ",
								accent,
								vad_eos,
								ASR_RES_PATH,
								SAMPLE_RATE_16K,
								GRM_BUILD_PATH,
								udata->grammar_id
								);
	rec_thread = start_mic_thread(asr_params);
	if (rec_thread == NULL) {
		printf("create thread failed\n");
		goto exit;
	}
	//rec_mic(asr_params);

	//asr_audiof = get_audio_file();
	//demo_file(asr_audiof, asr_params);

exit:
	if (rec_thread != NULL) {
		WaitForSingleObject(rec_thread, INFINITE);
		CloseHandle(rec_thread);
		return 0;
	}
	return -1;
}

/* main thread: start/stop record ; query the result of recgonization.
* record thread: record callback(data write)
* helper thread: ui(keystroke detection) no need in VRPT
* 
*/
int main(int argc, char* argv[])
{
	const char *login_config = "appid = 584f9076"; //登录参数
	UserData asr_data;
	int ret = 0;
	int place = 0;
	char * GRM_FILE = "UI主界面.bnf"; //构建离线识别语法网络所用的语法文件

	ret = MSPLogin(NULL, NULL, login_config); //第一个参数为用户名，第二个参数为密码，传NULL即可，第三个参数是登录参数
	if (MSP_SUCCESS != ret) {
		printf("登录失败：%d\n", ret);
		goto exit;
	}
	while (true){
		time_t temp;
		string s;
		string p;
		time(&temp);
		s = ctime(&temp);

		p = replaceAll(s,':','_');	
		p = replaceAll(p, '\n', '.');
		p = replaceAll(p, ' ', '_');

		LOG_FILE = "log/result_" + p + "log";
		AUDIO_FILE = "log/result_" + p + "pcm";


		memset(&asr_data, 0, sizeof(UserData));
		printf("构建离线识别语法网络...\n");
		printf("请选择场景? \n0: UI主界面\n2: 训练、考核界面\n3: 剧本选择界面\n4: 日常巡检界面\n5: 紧急处置界面\n6: 长句测试\n");
		//\n1: 校枪、标定、基础射击、实战射击、视频教学界面
		scanf("%d", &place);

		switch (place){
		case 1:
			GRM_FILE = "校枪、标定、基础射击、实战射击、视频教学界面.bnf";
			vad_eos = 0;
			break;
		case 2:
			GRM_FILE = "训练、考核界面.bnf";
			vad_eos = 0;
			break;
		case 3:
			GRM_FILE = "剧本选择界面.bnf";
			vad_eos = 0;
			break;
		case 4:
			GRM_FILE = "日常巡检.bnf";
			vad_eos = 0;
			break;
		case 5:
			GRM_FILE = "紧急处置.bnf";
			vad_eos = 0;
			break;
		case 6:
			GRM_FILE = "command.bnf";
			vad_eos = 4000;
			break;
		default:
			GRM_FILE = "UI主界面.bnf";
			vad_eos = 0;
			break;
		}

		ret = build_grammar(&asr_data, GRM_FILE);  //第一次使用某语法进行识别，需要先构建语法网络，获取语法ID，之后使用此语法进行识别，无需再次构建T
		if (MSP_SUCCESS != ret) {
			printf("构建语法调用失败！\n");
			goto exit;
		}
		while (1 != asr_data.build_fini)
			Sleep(300);
		if (MSP_SUCCESS != asr_data.errcode)
			goto exit;
		printf("离线识别语法网络构建完成，开始识别...\n");

		ret = run_asr(&asr_data);
		if (MSP_SUCCESS != ret) {
			printf("离线语法识别出错: %d \n", ret);
			goto exit;
		}
	}
	

	/*printf("请按任意键继续\n");
	_getch();*/
	
	/*
	printf("更新离线语法词典...\n");
	ret = update_lexicon(&asr_data);  //当语法词典槽中的词条需要更新时，调用QISRUpdateLexicon接口完成更新
	if (MSP_SUCCESS != ret) {
		printf("更新词典调用失败！\n");
		goto exit;
	}

	while (1 != asr_data.update_fini)
		Sleep(300);
	if (MSP_SUCCESS != asr_data.errcode)
		goto exit;
	printf("更新离线语法词典完成，开始识别...\n");

	ret = run_asr(&asr_data);
	if (MSP_SUCCESS != ret) {
		printf("离线语法识别出错: %d \n", ret);
		goto exit;
	}*/

exit:
	MSPLogout();
	printf("请按任意键退出...\n");
	_getch();
	return 0;
}
