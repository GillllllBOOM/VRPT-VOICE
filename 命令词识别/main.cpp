/*
* ������д(iFly Auto Transform)�����ܹ�ʵʱ�ؽ�����ת���ɶ�Ӧ�����֡�
*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <errno.h>
#include <process.h>
#include <list>

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
	signal = EVT_INIT;
	int errcode = 0;
	struct speech_rec* asr = (struct speech_rec*)para;

	do {
		key = _getch();
		switch (key) {
		case 'r':
		case 'R':{
			signal = EVT_START;
			printf("start\n");
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

	printf("Start Listening...\n");
}

//vad_end detected
void on_speech_end(int reason)
{
	if (reason == END_REASON_VAD_DETECT)//detected speech done
		printf("\nSpeaking done! END_REASON_VAD_DETECT\n");
	else
		printf("\nRecognizer error %d\n", reason);
}

/*recognize audio data from computer memeory */
static void __stdcall rec_mem(const char* audio_file, unsigned long len , char* session_begin_params)
{
	unsigned int	total_len = 0;
	int				errcode = 0;
	FILE*			f_pcm = NULL;
	char*			p_pcm = NULL;
	unsigned long	pcm_count = 0;
	unsigned long	pcm_size = 0;
	unsigned long	read_size = 0;
	struct speech_rec asr;
	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end
	};

	if (NULL == audio_file)
		goto iat_exit;

	f_pcm = fopen(audio_file, "rb");
	if (NULL == f_pcm)
	{
		printf("\nopen [%s] failed! \n", audio_file);
		goto iat_exit;
	}

	fseek(f_pcm, 0, SEEK_END);
	pcm_size = ftell(f_pcm); //��ȡ��Ƶ�ļ���С 
	fseek(f_pcm, 0, SEEK_SET);

	p_pcm = (char *)malloc(pcm_size);
	if (NULL == p_pcm)
	{
		printf("\nout of memory! \n");
		goto iat_exit;
	}

	read_size = fread((void *)p_pcm, 1, pcm_size, f_pcm); //��ȡ��Ƶ�ļ�����
	if (read_size != pcm_size)
	{
		printf("\nread [%s] error!\n", audio_file);
		goto iat_exit;
	}

	errcode = sr_init(&asr, session_begin_params, SR_USER, 0, &recnotifier);
	if (errcode) {
		printf("speech recognizer init failed : %d\n", errcode);
		goto iat_exit;
	}

	errcode = sr_start_listening(&asr);
	if (errcode) {
		printf("\nsr_start_listening failed! error code:%d\n", errcode);
		goto iat_exit;
	}

	while (1)
	{
		unsigned int len = 10 * FRAME_LEN; // ÿ��д��200ms��Ƶ(16k��16bit)��1֡��Ƶ20ms��10֡=200ms��16k�����ʵ�16λ��Ƶ��һ֡�Ĵ�СΪ640Byte
		int ret = 0;

		if (pcm_size < 2 * len)
			len = pcm_size;
		if (len <= 0)
			break;

		printf(">");
		ret = sr_write_audio_data(&asr, &p_pcm[pcm_count], len);

		if (0 != ret)
		{
			printf("\nwrite audio data failed! error code:%d\n", ret);
			goto iat_exit;
		}

		pcm_count += (long)len;
		pcm_size -= (long)len;
	}

	errcode = sr_stop_listening(&asr);
	if (errcode) {
		printf("\nsr_stop_listening failed! error code:%d \n", errcode);
		goto iat_exit;
	}

iat_exit:
	if (NULL != f_pcm)
	{
		fclose(f_pcm);
		f_pcm = NULL;
	}
	if (NULL != p_pcm)
	{
		free(p_pcm);
		p_pcm = NULL;
	}

	sr_stop_listening(&asr);
	sr_uninit(&asr);
}


static unsigned int  __stdcall rec_mic_thread_proc(void* session_begin_params){
	int errcode;
	int i = 0;
	HANDLE helper_thread = NULL;

	struct speech_rec asr;
	char isquit = 0;

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
		if (isquit)
			break;
	}
	

	/*while (1) {
		switch (signal) {
		case EVT_START:{
			if (errcode = sr_start_listening(&asr)) {
				printf("start listen failed %d\n", errcode);
				isquit = 1;
			}
			
			break;
		}
		case EVT_STOP:
			if (errcode = sr_stop_listening(&asr)) {
				printf("stop listening failed %d\n", errcode);
				isquit = 1;
			}
			break;
		case EVT_QUIT:
			sr_stop_listening(&asr);
			isquit = 1;
			break;
		default:
			break;
		}
		
		if (isquit)
			break;
	}*/

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
/* recognize the audio from microphone */
static void rec_mic(char* session_begin_params)
{
	int errcode;
	int i = 0;
	HANDLE helper_thread = NULL;

	struct speech_rec asr;
	DWORD waitres;
	char isquit = 0;

	struct speech_rec_notifier recnotifier = {
		on_result,
		on_speech_begin,
		on_speech_end
	};

	errcode = sr_init(&asr, session_begin_params, SR_MIC, DEFAULT_INPUT_DEVID, &recnotifier);
	if (errcode) {
		printf("speech recognizer init failed\n");
		return;
	}

	for (i = 0; i < EVT_TOTAL; ++i) {
		events[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	helper_thread = start_helper_thread(&asr);
	if (helper_thread == NULL) {
		printf("create thread failed\n");
		goto exit;
	}

	show_key_hints();

	while (1) {
		waitres = WaitForMultipleObjects(EVT_TOTAL, events, FALSE, INFINITE);
		switch (waitres) {
		case WAIT_FAILED:
		case WAIT_TIMEOUT:
			printf("Why it happened !?\n");
			break;
		case WAIT_OBJECT_0 + EVT_START:
			if (errcode == sr_start_listening(&asr)) {
				printf("start listen failed %d\n", errcode);
				isquit = 1;
			}
			break;
		case WAIT_OBJECT_0 + EVT_STOP:
			if (errcode == sr_stop_listening(&asr)) {
				printf("stop listening failed %d\n", errcode);
				isquit = 1;
			}
			break;
		case WAIT_OBJECT_0 + EVT_QUIT:
			sr_stop_listening(&asr);
			isquit = 1;
			break;
		default:
			break;
		}
		if (isquit)
			break;
	}

exit:
	if (helper_thread != NULL) {
		WaitForSingleObject(helper_thread, INFINITE);
		CloseHandle(helper_thread);
	}

	for (i = 0; i < EVT_TOTAL; ++i) {
		if (events[i])
			CloseHandle(events[i]);
	}

	sr_uninit(&asr);
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
		"engine_type = local, accent = %s,  vad_eos = 8000, \
				asr_res_path = %s, sample_rate = %d, \
						grm_build_path = %s, local_grammar = %s, \
								result_type = plain, result_encoding = GB2312, ",
								accent,
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
	char * GRM_FILE = "test.bnf"; //��������ʶ���﷨�������õ��﷨�ļ�

	ret = MSPLogin(NULL, NULL, login_config); //��һ������Ϊ�û������ڶ�������Ϊ���룬��NULL���ɣ������������ǵ�¼����
	if (MSP_SUCCESS != ret) {
		printf("��¼ʧ�ܣ�%d\n", ret);
		goto exit;
	}
	while (true){
		memset(&asr_data, 0, sizeof(UserData));
		printf("��������ʶ���﷨����...\n");
		printf("��ѡ�񳡾�? \n0: ����0\n1: ����1\n2: ����2\n3: ����3\n4: ����4\n");

		scanf("%d", &place);

		switch (place){
		case 1:
			GRM_FILE = "test.bnf";
			break;
		case 2:
			GRM_FILE = "test.bnf";
			break;
		case 3:
			GRM_FILE = "test.bnf";
			break;
		case 4:
			GRM_FILE = "test.bnf";
			break;
		default:
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
