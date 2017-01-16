/*
* ������д(iFly Auto Transform)�����ܹ�ʵʱ�ؽ�����ת���ɶ�Ӧ�����֡�
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

const char * ASR_RES_PATH = "fo|res/asr/common.jet";  //�����﷨ʶ����Դ·��
#ifdef _WIN64
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld_x64";  //���������﷨ʶ�������������ݱ���·��
#else
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld";  //���������﷨ʶ�������������ݱ���·��
#endif


//const char * LEX_NAME = "greeting"; //��������ʶ���﷨�Ĳ�

typedef struct _UserData {
	int     build_fini;  //��ʶ�﷨�����Ƿ����
	int     update_fini; //��ʶ���´ʵ��Ƿ����
	int     errcode; //��¼�﷨��������´ʵ�ص�������
	char    grammar_id[MAX_GRAMMARID_LEN]; //�����﷨�������ص��﷨ID
}UserData;

string LOG_FILE ;
extern string AUDIO_FILE;
int vad_eos = 500;

extern DWORD waitres;
extern int signal;

const char *get_audio_file(void); //ѡ����������﷨ʶ��������ļ�
int build_grammar(UserData *udata); //��������ʶ���﷨����
int update_lexicon(UserData *udata); //��������ʶ���﷨�ʵ�
int run_asr(UserData *udata); //���������﷨ʶ��

//�����﷨�ص��ӿ�
int build_grm_cb(int ecode, const char *info, void *udata)
{
	UserData *grm_data = (UserData *)udata;

	if (NULL != grm_data) {
		grm_data->build_fini = 1;
		grm_data->errcode = ecode;
	}

	if (MSP_SUCCESS == ecode && NULL != info) {
		printf("�����﷨�ɹ��� �﷨ID:%s\n", info);
		if (NULL != grm_data)
			_snprintf(grm_data->grammar_id, MAX_GRAMMARID_LEN - 1, info);
	}
	else
		printf("�����﷨ʧ�ܣ�%d\n", ecode);

	return 0;
}

//��ȡBNF�����﷨�������﷨id
int build_grammar(UserData *udata, char* GRM_FILE)
{
	FILE *grm_file = NULL;
	char *grm_content = NULL;
	unsigned int grm_cnt_len = 0;
	char grm_build_params[MAX_PARAMS_LEN] = { NULL };
	int ret = 0;

	grm_file = fopen(GRM_FILE, "rb");
	if (NULL == grm_file) {
		printf("��\"%s\"�ļ�ʧ�ܣ�[%s]\n", GRM_FILE, strerror(errno));
		return -1;
	}

	fseek(grm_file, 0, SEEK_END);
	grm_cnt_len = ftell(grm_file);
	fseek(grm_file, 0, SEEK_SET);

	grm_content = (char *)malloc(grm_cnt_len + 1);
	if (NULL == grm_content)
	{
		printf("�ڴ����ʧ��!\n");
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

//���´ʵ�ص�����
int update_lex_cb(int ecode, const char *info, void *udata)
{
	UserData *lex_data = (UserData *)udata;

	if (NULL != lex_data) {
		lex_data->update_fini = 1;
		lex_data->errcode = ecode;
	}

	if (MSP_SUCCESS == ecode)
		printf("���´ʵ�ɹ���\n");
	else
		printf("���´ʵ�ʧ�ܣ�%d\n", ecode);

	return 0;
}

//���±����﷨�ʵ�
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

//�������̼����߳�
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
			printf("��\"%s\"�ļ�ʧ�ܣ�[%s]\n", LOG_FILE, strerror(errno));
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

//��ʼ��ȫ�ֽ��
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
		printf("\n��⵽��Ĭ\n");
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
	
	printf("��ѡ�����? \n0: ��ͨ��\n1: ����\n2: �Ĵ���\n3: ���ϻ�\n4: ������\n");
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


	//�����﷨ʶ���������
	/*engine_type:�������� cloud/local/mixed Ĭ��:cloud
	**language:���� zh_cn/zh_tw/en_us Ĭ��:zh_cn
	**accent����������
	**sample_rate����Ƶ������  16000,8000 Ĭ��:16000
	**asr_threshold��ʶ������ 0~100 Ĭ��:0   ���Ŷȴ�������ֵ�ŷ��ؽ��
	**asr_denoise���Ƿ�������  0����������1������  Ĭ�ϲ�����            ������
	**asr_res_path������ʶ����Դ·�� 
	**grm_build_path�������﷨����·��  ���ݱ���·�����ļ��У�
	**result_type�������ʽ  plain,json,xml
	**text_encoding���ı������ʽ
	**vad_enable��VAD���ܿ���  0Ϊ�ر� Ĭ�Ͽ���
	**vad_eos������β���������ʱ��  0-10000���롣 Ĭ��Ϊ2000 ���β������ʱ�������˴�ֵ������Ϊ�û���Ƶ�Ѿ��������˲������ڴ�VAD����ʱ��Ч
	**local_grammar�������﷨id
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
	const char *login_config = "appid = 584f9076"; //��¼����
	UserData asr_data;
	int ret = 0;
	int place = 0;
	char * GRM_FILE = "UI������.bnf"; //��������ʶ���﷨�������õ��﷨�ļ�

	ret = MSPLogin(NULL, NULL, login_config); //��һ������Ϊ�û������ڶ�������Ϊ���룬��NULL���ɣ������������ǵ�¼����
	if (MSP_SUCCESS != ret) {
		printf("��¼ʧ�ܣ�%d\n", ret);
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
		printf("��������ʶ���﷨����...\n");
		printf("��ѡ�񳡾�? \n0: UI������\n2: ѵ�������˽���\n3: �籾ѡ�����\n4: �ճ�Ѳ�����\n5: �������ý���\n6: �������\n");
		//\n1: Уǹ���궨�����������ʵս�������Ƶ��ѧ����
		scanf("%d", &place);

		switch (place){
		case 1:
			GRM_FILE = "Уǹ���궨�����������ʵս�������Ƶ��ѧ����.bnf";
			vad_eos = 0;
			break;
		case 2:
			GRM_FILE = "ѵ�������˽���.bnf";
			vad_eos = 0;
			break;
		case 3:
			GRM_FILE = "�籾ѡ�����.bnf";
			vad_eos = 0;
			break;
		case 4:
			GRM_FILE = "�ճ�Ѳ��.bnf";
			vad_eos = 0;
			break;
		case 5:
			GRM_FILE = "��������.bnf";
			vad_eos = 0;
			break;
		case 6:
			GRM_FILE = "command.bnf";
			vad_eos = 4000;
			break;
		default:
			GRM_FILE = "UI������.bnf";
			vad_eos = 0;
			break;
		}

		ret = build_grammar(&asr_data, GRM_FILE);  //��һ��ʹ��ĳ�﷨����ʶ����Ҫ�ȹ����﷨���磬��ȡ�﷨ID��֮��ʹ�ô��﷨����ʶ�������ٴι���T
		if (MSP_SUCCESS != ret) {
			printf("�����﷨����ʧ�ܣ�\n");
			goto exit;
		}
		while (1 != asr_data.build_fini)
			Sleep(300);
		if (MSP_SUCCESS != asr_data.errcode)
			goto exit;
		printf("����ʶ���﷨���繹����ɣ���ʼʶ��...\n");

		ret = run_asr(&asr_data);
		if (MSP_SUCCESS != ret) {
			printf("�����﷨ʶ�����: %d \n", ret);
			goto exit;
		}
	}
	

	/*printf("�밴���������\n");
	_getch();*/
	
	/*
	printf("���������﷨�ʵ�...\n");
	ret = update_lexicon(&asr_data);  //���﷨�ʵ���еĴ�����Ҫ����ʱ������QISRUpdateLexicon�ӿ���ɸ���
	if (MSP_SUCCESS != ret) {
		printf("���´ʵ����ʧ�ܣ�\n");
		goto exit;
	}

	while (1 != asr_data.update_fini)
		Sleep(300);
	if (MSP_SUCCESS != asr_data.errcode)
		goto exit;
	printf("���������﷨�ʵ���ɣ���ʼʶ��...\n");

	ret = run_asr(&asr_data);
	if (MSP_SUCCESS != ret) {
		printf("�����﷨ʶ�����: %d \n", ret);
		goto exit;
	}*/

exit:
	MSPLogout();
	printf("�밴������˳�...\n");
	_getch();
	return 0;
}
