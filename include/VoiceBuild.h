/*
@file
@brief 讯飞录音识别构建类

@author		Willis ZHU
@date		2017/1/17
*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <errno.h>
#include <process.h>
#include <time.h>


#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "winrec.h"
#include "VoiceRec.h"

#define FRAME_LEN	640 


enum{
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



typedef struct _UserData {
	int     build_fini;  //标识语法构建是否完成
	int     update_fini; //标识更新词典是否完成
	int     errcode; //记录语法构建或更新词典回调错误码
	char    grammar_id[MAX_GRAMMARID_LEN]; //保存语法构建返回的语法ID
}UserData;

class VoiceBuild{
private:
	UserData* udata;
	char asr_params[MAX_PARAMS_LEN];

	int vad_eos;
	char* GRM_FILE;
	char* accent;

	VoiceRec* vr;

public:
	VoiceBuild();
	~VoiceBuild(){ if(vr) delete vr; if(udata) delete udata; };
	void set(int accent_status, int place);
	VoiceRec* get_vr(){ return vr; };
	char* get_asr_params(){ return asr_params; };
	int build_grammar(); //构建离线识别语法网络
	int update_lexicon(); //更新离线识别语法词典
	int run_asr(); //进行离线语法识别
	int start_build();

};