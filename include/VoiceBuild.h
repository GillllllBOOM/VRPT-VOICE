/*
@file
@brief Ѷ��¼��ʶ�𹹽���

@author		Willis ZHU
@date		2017/3/10
*/

#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "winrec.h"
#include "VoiceRec.h"

#define FRAME_LEN	640 

enum{//�źű�ʶ
	EVT_INIT = 0,
	EVT_START,
	EVT_STOP,
	EVT_QUIT,
	EVT_TOTAL
};


#define SAMPLE_RATE_16K     (16000)
#define SAMPLE_RATE_8K      (8000)
#define MAX_GRAMMARID_LEN   (32)
#define MAX_PARAMS_LEN      (1024)

#define MANDARIN 0
#define CANTONESE 1
#define SICHUANESE 2
#define HENANESE 3
#define DONGBEIESE 4

#define UIBNF 0
#define BACKBNF 1
#define DAILYBNF 2
#define EMTBNF 5


typedef struct _UserData {
	int     build_fini;  //��ʶ�﷨�����Ƿ����
	int     update_fini; //��ʶ���´ʵ��Ƿ����
	int     errcode; //��¼�﷨��������´ʵ�ص�������
	char    grammar_id[MAX_GRAMMARID_LEN]; //�����﷨�������ص��﷨ID
}UserData;

class VoiceBuild{
private:
	UserData* udata;
	char asr_params[MAX_PARAMS_LEN];//ʶ�����

	int vad_eos;    //��Ƶĩ�˾�Ĭʱ�䣨ms��
	char* GRM_FILE; //�﷨�ļ�
	char* accent;   //����

	VoiceRec* vr;

	void build_grammar(); //��������ʶ���﷨����
//	int update_lexicon(); //��������ʶ���﷨�ʵ�
	void build_asr_params();//����ʶ�����
	int run_asr(); //����ʶ��

public:
	VoiceBuild();
	~VoiceBuild(){ if (vr) delete vr; if (udata) delete udata;};
	void set_accent(int accent_status);

	void set_place(int place, int eos);
	void set_place(char* place, int eos);

	int start_build();//��ʼ����

	int start_listen();
	int stop_listen();
	void quit_listen();

};