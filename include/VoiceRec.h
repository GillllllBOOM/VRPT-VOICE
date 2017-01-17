/*
@file
@brief 基于录音接口和讯飞MSC接口封装MIC录音识别类

@author		Willis ZHU
@date		2017/1/16
*/

#include <list>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#define DEFAULT_INPUT_DEVID     (-1)

#define E_SR_NOACTIVEDEVICE		1
#define E_SR_NOMEM				2
#define E_SR_INVAL				3
#define E_SR_RECORDFAIL			4
#define E_SR_ALREADY			5

#define	BUFFER_SIZE	4096

#define __FILE_SAVE_VERIFY__
#define __DEBUG__

static COORD begin_pos = { 0, 0 };
static COORD last_pos = { 0, 0 };


#define END_REASON_VAD_DETECT	0	/* detected speech done  */


#define SR_MIC  0
#define SR_USER 1

struct speech_rec {
	volatile int aud_src;  /* from mic or manual  stream write */
	const char * session_id;
	int ep_stat;
	int rec_stat;
	int audio_status;
	struct recorder *recorder;
	volatile int state;
	volatile unsigned int rec_pos;
	char * session_begin_params;
};

struct speech_audio_data{
	char *data;
	unsigned int len;
};


class VoiceRec{
private:
	speech_rec* sr;
	int signal;

	char* g_result;
	unsigned int g_buffersize;

#ifdef __FILE_SAVE_VERIFY__
	char* AUDIO_FILE;
	FILE* audio_stream;
	string LOG_FILE;
#endif

public:
	VoiceRec();
	~VoiceRec(){ if(sr) delete sr; if(AUDIO_FILE) free(AUDIO_FILE); };
	list<speech_audio_data>speech_audio_buffer;
	/* must init before start . devid = -1, then the default device will be used.
	devid will be ignored if the aud_src is not SR_MIC */
	int sr_init(char * session_begin_params, int aud_src, int devid);
	int sr_start_listening();
	void sr_start_recognize();
	int sr_stop_listening();
	/* only used for the manual write way. */
	int sr_write_audio_data(char *data, unsigned int len);
	/* must call uninit after you don't use it */
	void sr_uninit();

	speech_rec* get_sr(){ return sr; };
	int get_signal(){ return signal; };
	void set_signal(int t){ signal = t; };

#ifdef __FILE_SAVE_VERIFY__
	int open_audio_file();
	int loopwrite_to_file(char *data, size_t length);
	void safe_close_file();
#endif

	void end_sr_on_error(int errcode);
	void end_sr_on_vad();
	void end_sr_on_normal();

	void on_result(const char *result, char is_last);
	void on_speech_begin();
	void on_speech_end(int reason);
};



