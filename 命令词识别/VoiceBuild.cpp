/*
@file
@brief Ѷ��¼��ʶ�𹹽���ʵ��

@author		Willis ZHU
@date		2017/1/17
*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <string>
#include <errno.h>
#include <process.h>

#include "VoiceBuild.h"
using namespace std;

const char * ASR_RES_PATH = "fo|res/asr/common.jet";  //�����﷨ʶ����Դ·��
#ifdef _WIN64
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld_x64";  //���������﷨ʶ�������������ݱ���·��
#else
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld";  //���������﷨ʶ�������������ݱ���·��
#endif

VoiceBuild::VoiceBuild(){
	vr = new VoiceRec;
	udata = new UserData;
	vad_eos = 500;
	accent = "mandarin";
	GRM_FILE = "UI������.bnf"; 
	asr_params[MAX_PARAMS_LEN] = { NULL };

}

void VoiceBuild::set(int accent_status, int place){
	vad_eos = 500;
	switch (place){
	case 1:
		GRM_FILE = "Уǹ���궨�����������ʵս�������Ƶ��ѧ����.bnf";
		break;
	case 2:
		GRM_FILE = "ѵ�������˽���.bnf";
		break;
	case 3:
		GRM_FILE = "�籾ѡ�����.bnf";
		break;
	case 4:
		GRM_FILE = "�ճ�Ѳ��.bnf";
		break;
	case 5:
		GRM_FILE = "��������.bnf";
		break;
	case 6:
		GRM_FILE = "command.bnf";
		vad_eos = 4000;
		break;
	default:
		GRM_FILE = "UI������.bnf";
		break;
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
		accent = "mandarin";
		break;
	}
}


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
int VoiceBuild::build_grammar()
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
	VoiceRec* vr = (VoiceRec*)para;

	do {
		key = _getch();
		switch (key) {
		case 'r':
		case 'R':{
			vr->set_signal(EVT_START);
			if (errcode = vr->sr_start_listening()) {
				printf("start listen failed %d\n", errcode);
				quit = 1;
			}
			break;
		}
		case 's':
		case 'S':{
			vr->set_signal(EVT_STOP);
			printf("stop\n");
			if (errcode = vr->sr_stop_listening()) {
				printf("stop listening failed %d\n", errcode);
				quit = 1;
			}
			break;
		}
		case 'q':
		case 'Q':{
			vr->sr_stop_listening();
			quit = 1;
			vr->set_signal(EVT_QUIT);
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
static HANDLE start_helper_thread(VoiceRec* para)
{
	HANDLE hdl;

	hdl = (HANDLE)_beginthreadex(NULL, 0, helper_thread_proc, para, 0, NULL);

	return hdl;
}







static unsigned int  __stdcall rec_mic_thread_proc(void* vb){
	int errcode;
	int i = 0;
	HANDLE helper_thread = NULL;


	VoiceBuild* voice_build = (VoiceBuild*)vb;
	
	char isquit = 0;

#ifdef  __DEBUG__
	printf("start_rec_mic_thread\n");
	
#endif
	errcode = voice_build->get_vr()->sr_init(voice_build->get_asr_params(), SR_MIC, DEFAULT_INPUT_DEVID);
	if (errcode) {
		cout << "errcode: " <<errcode<< endl;
		printf("speech recognizer init failed\n");
		return errcode;
	}


	helper_thread = start_helper_thread(voice_build->get_vr());
	if (helper_thread == NULL) {
		printf("create thread failed\n");
		goto exit;
	}

	show_key_hints();

#ifdef  __DEBUG__
	
#endif

	while (1){
		switch (voice_build->get_vr()->get_signal()) {
		case EVT_START:
			voice_build->get_vr()->sr_start_recognize();
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

	voice_build->get_vr()->sr_uninit();
	return 0;
}

static HANDLE start_mic_thread(VoiceBuild* vb)
{
	HANDLE hdl;

	hdl = (HANDLE)_beginthreadex(NULL, 0, rec_mic_thread_proc, vb, 0, NULL);

	return hdl;
}


int VoiceBuild::run_asr() 
{
	int ret = 0;
	HANDLE rec_thread = NULL;

	ret = build_grammar();  //��һ��ʹ��ĳ�﷨����ʶ����Ҫ�ȹ����﷨���磬��ȡ�﷨ID��֮��ʹ�ô��﷨����ʶ�������ٴι���T
	if (MSP_SUCCESS != ret) {
		printf("�����﷨����ʧ�ܣ�\n");
		goto exit;
	}
	while (1 != udata->build_fini)
		Sleep(300);
	if (MSP_SUCCESS != udata->errcode)
		goto exit;
	printf("����ʶ���﷨���繹����ɣ���ʼʶ��...\n");


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
	
	rec_thread = start_mic_thread(this);
	if (rec_thread == NULL) {
		printf("create thread failed\n");
		goto exit;
	}

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
int VoiceBuild::start_build()
{
	const char *login_config = "appid = 584f9076"; //��¼����
	int ret = 0;

	ret = MSPLogin(NULL, NULL, login_config); //��һ������Ϊ�û������ڶ�������Ϊ���룬��NULL���ɣ������������ǵ�¼����
	if (MSP_SUCCESS != ret) {
		printf("��¼ʧ�ܣ�%d\n", ret);
		goto exit;
	}
	
	ret = run_asr();
	if (MSP_SUCCESS != ret) {
		printf("�����﷨ʶ�����: %d \n", ret);
		goto exit;
	}

exit:
	MSPLogout();
	return 0;
}
