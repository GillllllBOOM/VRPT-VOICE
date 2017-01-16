/*
@file
@brief 基于录音接口和MSC接口封装一个MIC录音识别的模块

@author		taozhang9
@date		2016/05/27
*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <errno.h>
#include <process.h>
#include <iostream>
#include <fstream>
#include <string>

#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "winrec.h"
#include "speech_recognizer.h"
using namespace std;


#ifdef _WIN64
#pragma comment(lib,"../lib/msc_x64.lib") //x64
#else
#pragma comment(lib,"../lib/msc.lib") //x86
#endif




#define SR_DBGON 1
#if SR_DBGON == 1
#	define sr_dbg printf
//#	define __FILE_SAVE_VERIFY__  /* save the recording data into file 'rec.pcm' too */
#else
#	define sr_dbg
#endif




#define DEFAULT_FORMAT		\
{\
	WAVE_FORMAT_PCM,	\
	1,					\
	16000,				\
	32000,				\
	2,					\
	16,					\
	sizeof(WAVEFORMATEX)	\
}

/* internal state */
enum {
	SR_STATE_INIT,
	SR_STATE_STARTED
};

int signal;
list<speech_audio_data>speech_audio_buffer; 
string AUDIO_FILE;
ofstream audio_file;

/* for debug. saving the recording to a file */
#ifdef __FILE_SAVE_VERIFY__
#define VERIFY_FILE_NAME	"rec.pcm"
static int open_stored_file(const char * name);
static int loopwrite_to_file(char *data, size_t length);
static void safe_close_file();
#endif

#define SR_MALLOC malloc
#define SR_MFREE  free
#define SR_MEMSET	memset

#ifdef __FILE_SAVE_VERIFY__

static FILE *fdwav = NULL;

static int open_stored_file(const char * name)
{
	fdwav = fopen(name, "wb+");
	if(fdwav == NULL) {
		printf("error open file failed\n");
		return -1;
	}
	return 0;
}

static int loopwrite_to_file(char *data, size_t length)
{
	size_t wrt = 0, already = 0;
	int ret = 0;
	if(fdwav == NULL || data == NULL)
		return -1;

	while(1) {
		wrt = fwrite(data + already, 1, length - already, fdwav);
		if(wrt == (length - already) )
			break;
		if(ferror(fdwav)) {
			ret = -1;
			break;
		}
		already += wrt;
	}

	return ret;
}

static void safe_close_file()
{
	if(fdwav) {
		fclose(fdwav);
		fdwav = NULL;
	}
}
#endif


//出现错误结束识别
static void end_sr_on_error(struct speech_rec *sr, int errcode)
{
	/*if(sr->aud_src == SR_MIC)
		stop_record(sr->recorder);*/
	
	if (sr->session_id) {
		
		if (sr->notif.on_speech_end)
			sr->notif.on_speech_end(errcode);

		QISRSessionEnd(sr->session_id, "err");
		sr->session_id = NULL;
	}
	//sr->state = SR_STATE_INIT;
#ifdef __FILE_SAVE_VERIFY__
	safe_close_file();
#endif
}


static void end_sr_on_vad(struct speech_rec *sr)
{
	int errcode;
	int ret;
	const char *rslt;

	//if (sr->aud_src == SR_MIC)
	//	stop_record(sr->recorder);	
	//sr->state = SR_STATE_INIT;
	ret = QISRAudioWrite(sr->session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &sr->ep_stat, &sr->rec_stat);
	if (ret != 0) {
		sr_dbg("write LAST_SAMPLE failed: %d\n", ret);
		QISRSessionEnd(sr->session_id, "write err");
		return ;
	}
	sr->rec_stat = MSP_AUDIO_SAMPLE_CONTINUE;
	while(sr->rec_stat != MSP_REC_STATUS_COMPLETE ){
		rslt = QISRGetResult(sr->session_id, &sr->rec_stat, 0, &errcode);
		if (rslt && sr->notif.on_result)
			sr->notif.on_result(rslt, sr->rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);

		Sleep(100); /* for cpu occupy, should sleep here */
	}

	if (sr->session_id) {
		if (sr->notif.on_speech_end)
			sr->notif.on_speech_end(END_REASON_VAD_DETECT);
		QISRSessionEnd(sr->session_id, "VAD Normal");
		sr->session_id = NULL;
	}
	//sr->state = SR_STATE_INIT;
#ifdef __FILE_SAVE_VERIFY__
	safe_close_file();
#endif
}

static void end_sr_on_normal(struct speech_rec *sr){
	const char * rslt = NULL;
	int ret;

	if (sr->state < SR_STATE_STARTED) {
		sr_dbg("Not started or already stopped.\n");
		return;
	}

#ifdef __FILE_SAVE_VERIFY__
	safe_close_file();
#endif

	sr->state = SR_STATE_INIT;
	ret = QISRAudioWrite(sr->session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &sr->ep_stat, &sr->rec_stat);
	if (ret != 0) {
		sr_dbg("write LAST_SAMPLE failed: %d\n", ret);
		QISRSessionEnd(sr->session_id, "write err");
		return;
	}
	while (sr->rec_stat != MSP_REC_STATUS_COMPLETE) {
		rslt = QISRGetResult(sr->session_id, &sr->rec_stat, 0, &ret);
		if (MSP_SUCCESS != ret)	{
			sr_dbg("\nQISRGetResult failed! error code: %d\n", ret);
			end_sr_on_error(sr, ret);
			return;
		}
	
		if (NULL != rslt && sr->notif.on_result){
			sr->notif.on_result(rslt, sr->rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);
		}
		Sleep(100);
	}

	QISRSessionEnd(sr->session_id, "normal");
	sr->session_id = NULL;
	return;
}

/* after stop_record, there are still some data callbacks */
static void wait_for_rec_stop(struct recorder *rec, unsigned int timeout_ms)
{
	while (!is_record_stopped(rec)) {
		Sleep(1);
		if (timeout_ms != (unsigned int)-1)
			if (0 == timeout_ms--)
				break;
	}
}


double start = 0;
double endend =0;

/* the record call back 
**驱动提供音频数据时，回调该函数。
*/
static void asr_cb(char *data, unsigned long len, void *user_para)
{
	endend = GetTickCount();
	start = endend;

	if(len == 0 || data == NULL)
		return;

	struct speech_audio_data temp = {data, len};
	speech_audio_buffer.push_back(temp);
	audio_file << data;

	 
#ifdef __FILE_SAVE_VERIFY__
	loopwrite_to_file(data, len);
#endif

}

static char * skip_space(char *s)
{
	while (s && *s != ' ' && *s != '\0')
		s++;
	return s;
}

static int update_format_from_sessionparam(char * session_para, WAVEFORMATEX *wavefmt)
{
	char *s;
	if ((s = strstr(session_para, "sample_rate"))) {
		if (s = strstr(s, "=")) {
			s = skip_space(s);
			if (s && *s) {
				wavefmt->nSamplesPerSec = atoi(s);
				wavefmt->nAvgBytesPerSec = wavefmt->nBlockAlign * wavefmt->nSamplesPerSec;
			}
		}
		else
			return -1;
	}
	else {
		return -1;
	}

	return 0;
}

//往识别写入音频
int sr_write_audio_data(struct speech_rec *sr, volatile char *data, volatile unsigned int len)
{
	const char *rslt = NULL;
	int ret = 0;


	if (!sr)
		return -E_SR_INVAL;
	if (!data || !len)
		return 0;

	ret = QISRAudioWrite(sr->session_id, (char *)data, len, sr->audio_status, &sr->ep_stat, &sr->rec_stat);
	if (ret) {
		end_sr_on_error(sr, ret);
		return ret;
	}

	sr->audio_status = MSP_AUDIO_SAMPLE_CONTINUE;

	if (MSP_EP_AFTER_SPEECH == sr->ep_stat){
		end_sr_on_vad(sr);
	}

	return 0;
}

/* devid will be ignored if aud_src is not SR_MIC ; if devid == -1, then
 * the default input device will be used.  
 */
//初始化speech_recognizer、创建录音机对象  注意session_begin_params参数传参
int sr_init(struct speech_rec * sr, char * session_begin_params, enum sr_audsrc aud_src, int devid, struct speech_rec_notifier * notify)
{
	speech_audio_buffer.clear();
	int errcode;
	size_t param_size;
	WAVEFORMATEX wavfmt = DEFAULT_FORMAT;

	

	if (aud_src == SR_MIC && get_input_dev_num() == 0) {
		return -E_SR_NOACTIVEDEVICE;
	}

	if (!sr)
		return -E_SR_INVAL;

	SR_MEMSET(sr, 0, sizeof(struct speech_rec));
	sr->state = SR_STATE_INIT;
	sr->aud_src = aud_src;
	sr->ep_stat = MSP_EP_LOOKING_FOR_SPEECH;
	sr->rec_stat = MSP_REC_STATUS_SUCCESS;
	sr->audio_status = MSP_AUDIO_SAMPLE_FIRST;
	sr->rec_pos = 0;

	param_size = strlen(session_begin_params) + 1;
	sr->session_begin_params = (char*)SR_MALLOC(param_size);
	if (sr->session_begin_params == NULL) {
		sr_dbg("mem alloc failed\n");
		return -E_SR_NOMEM;
	}
	strncpy(sr->session_begin_params, session_begin_params, param_size - 1);

	sr->notif = *notify;

	audio_file.open(AUDIO_FILE, ios::app);
	if (!audio_file) {
		printf("打开\"%s\"文件失败！[%s]\n", AUDIO_FILE, strerror(errno));
		return -1;
	}
	
	if (aud_src == SR_MIC) {
		errcode = create_recorder(&sr->recorder, asr_cb,NULL);
		if (sr->recorder == NULL || errcode != 0) {
			sr_dbg("create recorder failed: %d\n", errcode);
			errcode = -E_SR_RECORDFAIL;
			goto fail;
		}
		update_format_from_sessionparam(session_begin_params, &wavfmt);
	
		errcode = open_recorder(sr->recorder, devid, &wavfmt);
		if (errcode != 0) {
			sr_dbg("recorder open failed: %d\n", errcode);
			errcode = -E_SR_RECORDFAIL;
			goto fail;
		}
	}

	return 0;

fail:
	if (sr->recorder) {
		destroy_recorder(sr->recorder);
		sr->recorder = NULL;
	}

	if (sr->session_begin_params) {
		SR_MFREE(sr->session_begin_params);
		sr->session_begin_params = NULL;
	}
	SR_MEMSET(&sr->notif, 0, sizeof(sr->notif));

	return errcode;
}



static unsigned int  __stdcall QISRSession_thread_proc(void* session_begin_params){
	struct speech_rec *sr = (speech_rec*)session_begin_params;
	const char* session_id = NULL;
	int	errcode = MSP_SUCCESS;

	session_id = QISRSessionBegin(NULL, sr->session_begin_params, &errcode); //听写不需要语法，第一个参数为NULL
	if (MSP_SUCCESS != errcode)
	{
		sr_dbg("\nQISRSessionBegin failed! error code:%d\n", errcode);

		return errcode;
	}
	sr->session_id = session_id;
	sr->ep_stat = MSP_EP_LOOKING_FOR_SPEECH;
	sr->rec_stat = MSP_REC_STATUS_SUCCESS;
	sr->audio_status = MSP_AUDIO_SAMPLE_FIRST;

	sr->state = SR_STATE_STARTED;

	if (sr->notif.on_speech_begin)
		sr->notif.on_speech_begin();
	
	list<speech_audio_data>::iterator it;

	if (sr->rec_pos >  speech_audio_buffer.size())
		return -1;

	if (speech_audio_buffer.size() != 0){
		it = speech_audio_buffer.begin();
		for (unsigned int i = 1; i < sr->rec_pos; i++){
			it++;
		}
	}

	int time_cnt = 0;


	while (sr->ep_stat< MSP_EP_AFTER_SPEECH){
		if (is_record_stopped(sr->recorder)){
			wait_for_rec_stop(sr->recorder, (unsigned int)-1);
			if (sr->rec_pos == speech_audio_buffer.size()){
				end_sr_on_normal(sr);
				break;
			}
		}
	
		if (sr->rec_pos < speech_audio_buffer.size()){
			if (sr->rec_pos == 0)
				it = speech_audio_buffer.begin();
			else
				it++;

			errcode = sr_write_audio_data(sr, it->data, it->len);

			if (errcode) {
				end_sr_on_error(sr, errcode);
				return errcode;
			}

			if ( (time_cnt >90) & (sr->ep_stat< MSP_EP_AFTER_SPEECH)){
				sr->rec_pos = sr->rec_pos - 25;
				end_sr_on_normal(sr);
				break;
			}
			//printf("time_cnt:%d",time_cnt);
			//printf("rec_pos:%d\n",sr->rec_pos);
			time_cnt++;
			sr->rec_pos++;
		}
		
		Sleep(100);
	}
	return 0;
}

static HANDLE start_QISRSession_thread(void* session_begin_params)
{
	HANDLE hdl;

	hdl = (HANDLE)_beginthreadex(NULL, 0, QISRSession_thread_proc, session_begin_params, 0, NULL);

	return hdl;
}

void sr_start_recognize(struct speech_rec *sr){
	HANDLE QISRSession_thread = NULL;
	while (!is_record_stopped(sr->recorder) | (sr->rec_pos < speech_audio_buffer.size())){
		QISRSession_thread = start_QISRSession_thread(sr);
		if (QISRSession_thread == NULL) {
			printf("create QISRSession_thread failed\n");
			return;
		}
		WaitForSingleObject(QISRSession_thread, INFINITE);
		CloseHandle(QISRSession_thread);
	}
}

//开始听写，开始语音识别QISRSessionBegin
int sr_start_listening(struct speech_rec *sr)
{
	int ret;
	int	errcode = MSP_SUCCESS;

	printf("start listening\n");

	if (sr->state >= SR_STATE_STARTED) {
		sr_dbg("already STARTED.\n");
		return 0;
	}

	if (sr->aud_src == SR_MIC) {
		ret = start_record(sr->recorder);
		if (ret != 0) {
			sr_dbg("start record failed: %d\n", ret);
			return -E_SR_RECORDFAIL;
		}
#ifdef __FILE_SAVE_VERIFY__
		open_stored_file(VERIFY_FILE_NAME);
#endif
	}
	return 0;
}



//停止录音。识别结果，结束此次识别
int sr_stop_listening(struct speech_rec *sr)
{
	printf("Stop listening!\n");
	int ret = 0;
	if (sr->state < SR_STATE_STARTED) {
		sr_dbg("Not started or already stopped.\n");
		return 0;
	}

	if (sr->aud_src == SR_MIC) {
		ret = stop_record(sr->recorder);
#ifdef __FILE_SAVE_VERIFY__
		safe_close_file();
#endif
		if (ret != 0) {
			sr_dbg("Stop failed! \n");
			return -E_SR_RECORDFAIL;
		}
		wait_for_rec_stop(sr->recorder, (unsigned int)-1);
	}

	return 0;
}



//销毁退出
void sr_uninit(struct speech_rec * sr)
{
	if (audio_file)
		audio_file.close();

	if (sr->recorder) {
		if(!is_record_stopped(sr->recorder))
			stop_record(sr->recorder);
		close_recorder(sr->recorder);
		destroy_recorder(sr->recorder);
		sr->recorder = NULL;
	}

	if (sr->session_begin_params) {
		SR_MFREE(sr->session_begin_params);
		sr->session_begin_params = NULL;
	}
}