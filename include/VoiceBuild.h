/*
@file
@brief 讯飞录音识别构建类

@author		Willis ZHU
@date		2017/3/10
*/

#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "winrec.h"
#include "VoiceRec.h"

#define FRAME_LEN	640 

enum{//信号标识
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
	int     build_fini;  //标识语法构建是否完成
	int     update_fini; //标识更新词典是否完成
	int     errcode; //记录语法构建或更新词典回调错误码
	char    grammar_id[MAX_GRAMMARID_LEN]; //保存语法构建返回的语法ID
}UserData;

class VoiceBuild{
private:
	UserData* udata;
	char asr_params[MAX_PARAMS_LEN];//识别参数

	int vad_eos;    //音频末端静默时间（ms）
	char* GRM_FILE; //语法文件
	char* accent;   //口音

	VoiceRec* vr;

	void build_grammar(); //构建离线识别语法网络
//	int update_lexicon(); //更新离线识别语法词典
	void build_asr_params();//构建识别参数
	int run_asr(); //进行识别

public:
	VoiceBuild();
	~VoiceBuild(){ if (vr) delete vr; if (udata) delete udata;};
	void set_accent(int accent_status);

	void set_place(int place, int eos);
	void set_place(char* place, int eos);

	int start_build();//开始构建

	int start_listen();
	int stop_listen();
	void quit_listen();

};