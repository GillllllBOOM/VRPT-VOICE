/*
@file
@brief Ѷ��¼��ʶ�𹹽���ʵ��

@author		Willis ZHU
@date		2017/3/10
*/

#include <mutex>
#include "VoiceBuild.h"
using namespace std;

//UDP ͨѶ
extern WSADATA wsaData;
extern SOCKET RecvSocket;
extern sockaddr_in RecvAddr;
extern sockaddr_in SenderAddr;
extern int SenderAddrSize;

//�˳���
mutex mEnd;

const char * ASR_RES_PATH = "fo|res/asr/common.jet";  //�����﷨ʶ����Դ·��
#ifdef _WIN64
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld_x64";  //���������﷨ʶ�������������ݱ���·��
#else
const char * GRM_BUILD_PATH = "res/asr/GrmBuilld";  //���������﷨ʶ�������������ݱ���·��
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
	case 1://����
		accent = "cantonese";
		break;
	case 2://�Ĵ���
		accent = "lmz";
		break;
	case 3://���ϻ�
		accent = "henanese";
		break;
	case 4://������
		accent = "dongbeiese";
		break;
	default://��ͨ��
		accent = "mandarin";
		break;
	}
}

void VoiceBuild::set_place(int place, int eos){
	vad_eos = eos;//���Ĭʱ��
	switch (place){
	case 1://����
		GRM_FILE = "back.bnf";
		break;
	case 2://�ճ�Ѳ��
		GRM_FILE = "daily0.bnf";
		break;
	case 3:
		GRM_FILE = "test.bnf";
		break;
	case 4:
		GRM_FILE = "emergency2.bnf";
		break;
	case 5://Ӧ������
		GRM_FILE = "commander0.bnf";
		break;
	default://������
		GRM_FILE = "UI.bnf";
		break;
	}
}

void VoiceBuild::set_place(char* place, int eos) {
	vad_eos = eos;//���Ĭʱ��
	GRM_FILE = place;
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
#ifdef __DEBUG__
	else
		printf("�����﷨ʧ�ܣ�%d\n", ecode);
#endif 

	return 0;
}

//��ȡBNF�����﷨�������﷨id
void VoiceBuild::build_grammar()
{
	try {
		FILE *grm_file = NULL;
		char *grm_content = NULL;
		unsigned int grm_cnt_len = 0;
		char grm_build_params[MAX_PARAMS_LEN] = { NULL };
		int ret = 0;
		//��ʼ���µ�udata
		if (udata) delete udata;
		udata = new UserData;

		printf("�ļ�����%s\n", GRM_FILE);				//�﷨�ļ�
		grm_file = fopen(GRM_FILE, "rb");
		if (NULL == grm_file) {
			char error[1024];
			sprintf(error, "��\"%s\"�ļ�ʧ�ܣ�[%s]\n", GRM_FILE, strerror(errno));
			throw error;
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

		//�����﷨
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
		MessageBox(NULL, TEXT("����������ʧ�ܣ��Ҳ�����Ӧ�﷨�ļ�!���������������"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (std::exception& e) {
		char errorcode[1024];
		sprintf(errorcode, e.what());
		size_t size = sizeof(errorcode);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;
		MessageBox(NULL, TEXT("�����˹���ʧ��!������������"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (...) {
		MessageBox(NULL, TEXT("�����˹���ʧ��!������������"), TEXT("�����˴���"), MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	}
			
	return;
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
		
	/*��Ϊ����һ���߳̽���ʶ�𣬴��ڻ��ڻ�ȡʶ����ʱ�˳���ʶ��session�����²�ѯ�˿�session���Ӷ�����
	**�������˳��ͻ�ȡ���ǰ�������
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

	errcode = vr->sr_init(asr_params, SR_MIC, DEFAULT_INPUT_DEVID);//��ʼ��
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
		case EVT_START://����ڴ�recorder������ʼʶ��
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
		build_grammar();  //��һ��ʹ��ĳ�﷨����ʶ����Ҫ�ȹ����﷨���磬��ȡ�﷨ID��֮��ʹ�ô��﷨����ʶ�������ٴι���
		while (1 != udata->build_fini)
			Sleep(300);
		if (MSP_SUCCESS != udata->errcode)
			throw udata->errcode;
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
					printf("sendto()failed��%d\n", WSAGetLastError());
				}
			}*/
			//MessageBox(NULL, TEXT("����������ʧ�ܣ�û�����������豸!���������������"), buffer, MB_ICONEXCLAMATION);
			break;
		}
		case -E_SR_NOMEM:
			MessageBox(NULL, TEXT("����������ʧ�ܣ��ռ����!���������������"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
			break;
		case  -E_SR_RECORDFAIL:
			MessageBox(NULL, TEXT("����������ʧ�ܣ����������豸���ù���!���������������"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
			break;
		default:
			MessageBox(NULL, TEXT("����������ʧ�ܣ��﷨�ļ��������������������"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
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
		MessageBox(NULL, TEXT("����������ʧ��!������������"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (...) {
		MessageBox(NULL, TEXT("����������ʧ��!������������"), TEXT("�����˴���"), MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	}

	return;
}


int VoiceBuild::start_build()
{
	try {
		const char *login_config = "appid = 5932a3b6"; //��¼����
		int ret = 0;

		ret = MSPLogin(NULL, NULL, login_config); //��һ������Ϊ�û������ڶ�������Ϊ���룬��NULL���ɣ������������ǵ�¼����
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
		MessageBox(NULL, TEXT("������Ѷ�ɵ�¼ʧ�ܣ�appidʧЧ!����ϵ�����ߡ�"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (std::exception& e) {
		char errorcode[1024];
		sprintf(errorcode, e.what());
		size_t size = sizeof(errorcode);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;
		MessageBox(NULL, TEXT("�����˵�½ʧ��!������������"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
	}
	catch (...) {
		MessageBox(NULL, TEXT("�����˵�¼ʧ��!������������"), TEXT("�����˴���"), MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		
	}

	MSPLogout();
	return 0;
}
