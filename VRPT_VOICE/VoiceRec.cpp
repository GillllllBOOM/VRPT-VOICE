/*
@file
@brief 基于录音接口和讯飞MSC接口封装MIC录音识别类实现

@author		Willis ZHU
@date		2017/3/10
*/

#include <mutex>
#include "qisr.h"
#include "msp_cmn.h"
#include "msp_errors.h"
#include "winrec.h"
#include "VoiceRec.h"
#include "json/json.h"
#include "json/reader.h"
using namespace std;


#ifdef _WIN64
#pragma comment(lib,"../lib/msc_x64.lib") //x64
#else
#pragma comment(lib,"../lib/msc.lib") //x86
#endif


#define SR_DBGON 1
#if SR_DBGON == 1
#	define sr_dbg printf
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


#define SR_MALLOC malloc
#define SR_MFREE  free
#define SR_MEMSET memset

#define CONFLAG 18

extern WSADATA wsaData;
extern SOCKET RecvSocket;
extern sockaddr_in RecvAddr;
extern sockaddr_in SenderAddr;
extern int SenderAddrSize;

extern mutex mEnd;

double beginTime = 0;
double endTime;

VoiceRec::VoiceRec(){
	sr = new speech_rec; 
	signal = 0; 
	g_result = NULL;
	g_buffersize = BUFFER_SIZE;
}

#ifdef __FILE_SAVE_VERIFY__
int VoiceRec::open_audio_file()
{
#ifdef __DEBUG__
	cout << AUDIO_FILE << endl;
#endif
	audio_stream = fopen(AUDIO_FILE, "ab+");
	if (audio_stream == NULL) {
		printf("error open file failed\n");
		return -1;
	}
	return 0;
}

int VoiceRec::loopwrite_to_file(char *data, size_t length)
{
	size_t wrt = 0, already = 0;
	
	if(audio_stream == NULL || data == NULL)
		return -1;

	while(1) {
		wrt = fwrite(data + already, 1, length - already, audio_stream);
		if(wrt == (length - already) )
			break;
		if (ferror(audio_stream)) {
			return -1;
		}
		already += wrt;
	}

	return 0;
}

void VoiceRec::safe_close_file()
{
	if (audio_stream) {
		fclose(audio_stream);
		audio_stream = NULL;
	}
}
#endif

static string replaceAll(string src, char oldChar, char newChar){
	string head = src;
	for (int i = 0; i < src.size(); i++){
		if (src[i] == oldChar)
			head[i] = newChar;
	}
	return head;
}

void VoiceRec::show_result(char *result, char is_over)
{	
	char SendBuf[1024];
	int BufLen = 1024;

	Json::Reader reader;
	Json::Value root;
	
	int i,j = 0;
	if (reader.parse(result, root) && is_over){  // reader将Json字符串解析到root，root将包含Json里所有子元素
		for (i = 0; i < root["ws"].size(); i++){
			string word = root["ws"][i]["cw"][j]["w"].asString();  // 访问节点
			int id = root["ws"][i]["cw"][j]["id"].asInt();
			int confidence = root["sc"].asInt();
			cout << "id:" << id << "  confidence:" << confidence << endl;
			/*switch (id){
			case 101:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#caliGun#");
				break; }
			case 102:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#caliScreen#");
				break; }
			case 103:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#train#");
				break; }
			case 104:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#check#");
				break; }
			case 105:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#game#");
				break; }
			case 106:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#back#");
				break; }
			case 107:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#basicShooting#");
				break; }
			case 108:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#dailyPatrol#");
				break; }
			case 109:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#actualShooting#");
				break; }
			case 110:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#emergencyHandling#");
				break; }
			case 111:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#videoTeaching#");
				break; }
			case 112:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#numOne#");
				break; }
			case 113:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#numTwo#");
				break; }
			case 114:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#numThree#");
				break; }
			case 115:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#numFour#");
				break; }
			case 116: {
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#lastPage#");
				break; }
			case 117: {
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#nextPage#");
				break; }
			case 118:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#quit#");
				break; }
			case 119:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#single#");
				break; }
			case 120:{
				if (confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#couple#");
				break; }	
			case 201:{
				if(confidence > CONFLAG)
					strcpy(SendBuf, "speechCommand#back#");
				break;
				}
			case 202:{
				if(confidence > 25)
				strcpy(SendBuf, "speechCommand#back#");
				break;
				}
			case 301:
				strcpy(SendBuf, "speechCommand#score#");
				break;
			case 302:
				strcpy(SendBuf, "speechCommand#persuade#dropWeapon#");
				break;
			case 303:
				strcpy(SendBuf, "speechCommand#persuade#headDown#");
				break;
			case 304:
				strcpy(SendBuf, "speechCommand#persuade#handWall#");
				break;
			case 401:
				strcpy(SendBuf, "speechCommand#question#name#");
				break;
			case 402:
				strcpy(SendBuf, "speechCommand#question#birthday#");
				break;
			case 403:
				strcpy(SendBuf, "speechCommand#question#id#");
				break;
			case 404:
				strcpy(SendBuf, "speechCommand#question#live#");
				break;
			case 405:
				strcpy(SendBuf, "speechCommand#question#job#");
				break;
			case 406:
				strcpy(SendBuf, "speechCommand#question#wheretogo#");
				break;
			case 407:
				strcpy(SendBuf, "speechCommand#question#belonging#");
				break;
			case 408:
				strcpy(SendBuf, "speechCommand#question#inside#");
				break;
			case 409:
				strcpy(SendBuf, "speechCommand#question#open#");
				break;
			case 410:
				strcpy(SendBuf, "speechCommand#question#showIdCard#");
				break;
			case 411:
				strcpy(SendBuf, "speechCommand#question#other#");
				break;
			case 412:
				strcpy(SendBuf, "speechCommand#over#release#");
				break;
			case 413:
				strcpy(SendBuf, "speechCommand#over#arrest#");
				break;
			case 414:
				strcpy(SendBuf, "speechCommand#explanation#");
				break;
			case 415:
				strcpy(SendBuf, "speechCommand#choose#black#");
				break;
			case 416:
				strcpy(SendBuf, "speechCommand#choose#gray#");
				break;
			case 417:
				strcpy(SendBuf, "speechCommand#choose#green#");
				break;
			case 418:
				strcpy(SendBuf, "speechCommand#choose#fat#");
				break;
			case 419:
				strcpy(SendBuf, "speechCommand#choose#withBag#");
				break;
			case 420:
				strcpy(SendBuf, "speechCommand#choose#usingPhone#");
				break;
			case 421:
				strcpy(SendBuf, "speechCommand#stop#red#");
				break;
			default:
				break;
			}
			if (strlen(SendBuf) != 0 ){
				int isend = sendto(RecvSocket, SendBuf, BufLen, 0, (sockaddr *)&SenderAddr, SenderAddrSize);
				if (isend == SOCKET_ERROR){
					printf("sendto()failed：%d\n", WSAGetLastError());
				}
			}*/
		}
	}
}

void VoiceRec::on_result(const char *result, char is_last)
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

#ifdef __FILE_SAVE_VERIFY__
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
		out_file << s << ": " << g_result << endl << endl;

		out_file.close();
#endif

		show_result(g_result, is_last);
	}
}

//初始化全局结果
void VoiceRec::on_speech_begin()
{
	if (g_result)
		free(g_result);

	g_result = (char*)malloc(BUFFER_SIZE);
	g_buffersize = BUFFER_SIZE;
	memset(g_result, 0, g_buffersize);
}

//vad_end detected
void VoiceRec::on_speech_end(int reason)
{
	if (reason == END_REASON_VAD_DETECT)//detected speech done
		printf("on vad\n");
//		printf("\n监测到静默\n");
	else
		printf("\nRecognizer error %d\n", reason);
}

//出现错误结束识别
void VoiceRec::end_sr_on_error(int errcode)
{
	if (sr->session_id) {
		on_speech_end(errcode);

		QISRSessionEnd(sr->session_id, "err");
		sr->session_id = NULL;
	}

	char errorcode[1024];
	sprintf(errorcode, "Error Code:%d", errcode);
	size_t size = sizeof(errorcode);
	wchar_t *buffer = new wchar_t[size + 1];
	MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
	buffer[size] = 0;
	MessageBox(NULL, TEXT("语音端识别出现错误!请重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	delete buffer;
#ifdef __FILE_SAVE_VERIFY__
	safe_close_file();
#endif
}


void VoiceRec::end_sr_on_vad()
{
	int errcode;
	int ret;
	const char *rslt;

	mEnd.lock();
#ifdef __DEBUG__
	printf("end_sr_on_vad write lock\n");
#endif // __DEBUG__
	if (sr->session_id == NULL) {
		mEnd.unlock();
		return;
	}
	ret = QISRAudioWrite(sr->session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &sr->ep_stat, &sr->rec_stat);
	mEnd.unlock();
	if (ret != 0) {
		sr_dbg("write LAST_SAMPLE failed: %d\n", ret);
		end_sr_on_error(ret);
		return ;
	}
	sr->rec_stat = MSP_AUDIO_SAMPLE_CONTINUE;
	while(sr->rec_stat != MSP_REC_STATUS_COMPLETE ){
		mEnd.lock();
#ifdef __DEBUG__
		printf("end_sr_on_vad result lock\n");
#endif // __DEBUG__
		if (sr->session_id == NULL){
			mEnd.unlock();
			return;
		}
		rslt = QISRGetResult(sr->session_id, &sr->rec_stat, 0, &errcode);
		mEnd.unlock();
		if (rslt)
			on_result(rslt, sr->rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);

		Sleep(100); /* for cpu occupy, should sleep here */
	}

	if (sr->session_id) {
		on_speech_end(END_REASON_VAD_DETECT);
		QISRSessionEnd(sr->session_id, "VAD Normal");
		sr->session_id = NULL;
	}

//#ifdef __FILE_SAVE_VERIFY__
//	safe_close_file();
//#endif
}

void VoiceRec::end_sr_on_normal(){
	const char * rslt = NULL;
	int ret;

	if (sr->state < SR_STATE_STARTED) {
		sr_dbg("Not started or already stopped.\n");
		return;
	}

//#ifdef __FILE_SAVE_VERIFY__
//	safe_close_file();
//#endif

	sr->state = SR_STATE_INIT;

	mEnd.lock();
#ifdef __DEBUG__
	printf("end_sr_on_normal write lock\n");
#endif // __DEBUG__

	if (sr->session_id == NULL) {
		mEnd.unlock();
		return;
	}

	ret = QISRAudioWrite(sr->session_id, NULL, 0, MSP_AUDIO_SAMPLE_LAST, &sr->ep_stat, &sr->rec_stat);
	mEnd.unlock();

	if (ret != 0) {
		sr_dbg("write LAST_SAMPLE failed: %d\n", ret);
		end_sr_on_error(ret);
		return;
	}

	while (sr->rec_stat != MSP_REC_STATUS_COMPLETE) {
		mEnd.lock();
#ifdef __DEBUG__
		printf("end_sr_on_normal result lock\n");
#endif // __DEBUG__
		if (sr->session_id == NULL) {
			mEnd.unlock();
			return;
		}
		rslt = QISRGetResult(sr->session_id, &sr->rec_stat, 0, &ret);
		mEnd.unlock();
		if (MSP_SUCCESS != ret)	{
			sr_dbg("\nQISRGetResult failed! error code: %d\n", ret);
			end_sr_on_error(ret);
			return;
		}
	
		if (NULL != rslt){
			on_result(rslt, sr->rec_stat == MSP_REC_STATUS_COMPLETE ? 1 : 0);
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



/* the record call back 
**驱动提供音频数据时，回调该函数。
*/
static void asr_cb(char *data, unsigned long len, void *user_para)
{
	if(len == 0 || data == NULL)
		return;
	endTime = GetTickCount();
//	cout << (endTime - beginTime) << endl;
	beginTime = endTime;
	VoiceRec* voice_rec = (VoiceRec*)user_para;
	struct speech_audio_data temp = {data, len};
	voice_rec->speech_audio_buffer.push_back(temp);

#ifdef __FILE_SAVE_VERIFY__
	voice_rec->loopwrite_to_file(data, len);
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
int VoiceRec::sr_write_audio_data(char *data, volatile unsigned int len)
{
	const char *rslt = NULL;
	int ret = 0;

	if (!sr)
		return -E_SR_INVAL;

	mEnd.lock();
#ifdef __DEBUG__
	printf("write audio data lock\n");
	printf("len: %d\n", len);
#endif // __DEBUG__

	
	if (!data || !len || !sr->session_id) {
		mEnd.unlock();
		return 0;
	}
	ret = QISRAudioWrite(sr->session_id, (char *)data, len, sr->audio_status, &sr->ep_stat, &sr->rec_stat);
	
#ifdef __DEBUG__
	printf("ret:%d\n", ret);
#endif // __DEBUG__

	mEnd.unlock();
	
	if (ret) 
		return ret;

#ifdef __DEBUG__
	printf("audio_status:%d, ep_stat:%d\n", sr->audio_status, sr->ep_stat);
#endif // __DEBUG__
	
	sr->audio_status = MSP_AUDIO_SAMPLE_CONTINUE;

	if (MSP_EP_AFTER_SPEECH == sr->ep_stat){
		end_sr_on_vad();
	}

	return 0;
}

/* devid will be ignored if aud_src is not SR_MIC ; if devid == -1, then
 * the default input device will be used.  
 */
//初始化speech_recognizer、创建录音机对象  注意session_begin_params参数传参
int VoiceRec::sr_init(char * session_begin_params, int aud_src, int devid)
{
	speech_audio_buffer.clear();
	int errcode;
	size_t param_size;
	WAVEFORMATEX wavfmt = DEFAULT_FORMAT;


	if (aud_src == SR_MIC && get_input_dev_num() == 0) {
		return -E_SR_NOACTIVEDEVICE;
	}

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


#ifdef __FILE_SAVE_VERIFY__
	time_t temp;
	string s;
	string p;
	time(&temp);
	s = ctime(&temp);

	p = replaceAll(s, ':', '_');
	p = replaceAll(p, '\n', '.');
	p = replaceAll(p, ' ', '_');

	LOG_FILE = "log/Result_" + p + "log";
	string AUDIO_TEMP = "log/Voice_" + p + "pcm";
	AUDIO_FILE = new char[strlen(AUDIO_TEMP.c_str())+1];
	strcpy(AUDIO_FILE, AUDIO_TEMP.c_str());

#ifdef __DEBUG__
	cout << AUDIO_TEMP << endl;
	cout << AUDIO_FILE << endl;
#endif

#endif
	
	if (aud_src == SR_MIC) {
		errcode = create_recorder(&sr->recorder, asr_cb, this);
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
	
	return errcode;
}


int QISRSession_thread_proc(VoiceRec *voice_rec){
	try {
		const char* session_id = NULL;
		int	errcode = MSP_SUCCESS;
		speech_rec* sr = voice_rec->get_sr();
		session_id = QISRSessionBegin(NULL, sr->session_begin_params, &errcode); 
		if (MSP_SUCCESS != errcode)
		{
			sr_dbg("\nQISRSessionBegin failed! error code:%d\n", errcode);
			throw errcode;
		}
		sr->session_id = session_id;
		sr->ep_stat = MSP_EP_LOOKING_FOR_SPEECH;
		sr->rec_stat = MSP_REC_STATUS_SUCCESS;
		sr->audio_status = MSP_AUDIO_SAMPLE_FIRST;

		sr->state = SR_STATE_STARTED;


		voice_rec->on_speech_begin();

		list<speech_audio_data>::iterator it;

		if (sr->rec_pos > voice_rec->speech_audio_buffer.size())
			return -1;

		if (voice_rec->speech_audio_buffer.size() != 0) {
			it = voice_rec->speech_audio_buffer.begin();
			for (unsigned int i = 1; i < sr->rec_pos; i++) {
				it++;
			}
		}

		int time_cnt = 0;
		int microsize = -1;

		while (sr->ep_stat < MSP_EP_AFTER_SPEECH) {
			if (is_record_stopped(sr->recorder)) {
				wait_for_rec_stop(sr->recorder, (unsigned int)-1);
				if (sr->rec_pos == voice_rec->speech_audio_buffer.size()) {
					voice_rec->end_sr_on_normal();
					break;
				}
			}

			if (sr->rec_pos < (voice_rec->speech_audio_buffer.size())) {
				if (sr->rec_pos == 0)
					it = voice_rec->speech_audio_buffer.begin();
				else
					it++;

				errcode = voice_rec->sr_write_audio_data(it->data, it->len);

				if (errcode) {
					voice_rec->end_sr_on_error(errcode);
					return errcode;
				}

				if ((time_cnt > 85) & (sr->ep_stat < MSP_EP_AFTER_SPEECH)) {
					sr->rec_pos = sr->rec_pos - 25;
					voice_rec->end_sr_on_normal();
					break;
				}

				time_cnt++;
				sr->rec_pos++;
			}
			else if (sr->rec_pos = (voice_rec->speech_audio_buffer.size())) {
				if (microsize != voice_rec->speech_audio_buffer.size())
					microsize = voice_rec->speech_audio_buffer.size();
				else
					throw -5;
			}
			if (!sr->session_id)
				break;
			
			Sleep(100);
		}
	}
	catch (int d) {
		if (d == -5) {
			char SendBuf[1024];
			int BufLen = 1024;
			strcpy(SendBuf, "speechError#microphoneOffline#");
			/*if (strlen(SendBuf) != 0) {
				int isend = sendto(RecvSocket, SendBuf, BufLen, 0, (sockaddr *)&SenderAddr, SenderAddrSize);
				if (isend == SOCKET_ERROR) {
					printf("sendto()failed：%d\n", WSAGetLastError());
				}
			}*/
			MessageBox(NULL, TEXT("没有语音输入设备!请检查后重新启动。"), TEXT("语音端错误。"), MB_ICONEXCLAMATION|MB_SYSTEMMODAL|MB_SYSTEMMODAL);
		}
		else {
			char errorcode[1024];
			sprintf(errorcode, "Error Code:%d", d);
			size_t size = sizeof(errorcode);
			wchar_t *buffer = new wchar_t[size + 1];
			MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
			buffer[size] = 0;
			MessageBox(NULL, TEXT("语音端识别失败，QISRSessionBegin failed!请重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL | MB_SYSTEMMODAL);
			delete buffer;
		}
		voice_rec->sr_stop_listening();
		voice_rec->set_signal(3);
	}
	catch (std::exception& e) {
		char errorcode[1024];
		sprintf(errorcode, e.what());
		size_t size = sizeof(errorcode);
		wchar_t *buffer = new wchar_t[size + 1];
		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
		buffer[size] = 0;
		MessageBox(NULL, TEXT("语音端识别失败!请重新启动。"), buffer, MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		delete buffer;
		voice_rec->sr_stop_listening();
		voice_rec->set_signal(3);
	}
	catch (...) {
		MessageBox(NULL, TEXT("语音端识别失败!请重新启动。"), TEXT("语音端错误"), MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		voice_rec->sr_stop_listening();
		voice_rec->set_signal(3);
	}
	return 0;
}


void VoiceRec::sr_start_recognize(){
	while (get_signal()==1){
		thread t(QISRSession_thread_proc,this);
		t.join();
	}
}


//开始听写，开始语音识别QISRSessionBegin
int VoiceRec::sr_start_listening()
{
	int ret = 0;

	printf("start listening\n");

	if (sr->state >= SR_STATE_STARTED) {
		sr_dbg("already STARTED.\n");
		return 0;
	}
#ifdef __FILE_SAVE_VERIFY__
	open_audio_file();
#endif

	if (sr->aud_src == SR_MIC) {
		ret = start_record(sr->recorder);
		if (ret != 0) {
			sr_dbg("start record failed: %d\n", ret);
			return -E_SR_RECORDFAIL;
		}
	}
	
	return 0;
}


//停止录音。识别结果，结束此次识别
int VoiceRec::sr_stop_listening()
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
void VoiceRec::sr_uninit()
{
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