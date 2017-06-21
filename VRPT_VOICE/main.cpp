#include <thread>
#include "VoiceBuild.h"

WSADATA wsaData;//初始化
SOCKET RecvSocket;
sockaddr_in RecvAddr;//服务器地址

sockaddr_in SenderAddr;
int SenderAddrSize;

int helper_thread_proc(VoiceBuild* vb)
{
	int errcode = 0;
	if (!vb) return -1;
	errcode = vb->start_build();
	printf("thread exit\n");
	return errcode;
}



int main(int argc, char* argv[]){
	VoiceBuild vb;
	vb.set_accent(MANDARIN);

	if (argc > 1) {
		vb.set_place(argv[1], 0);
		std::thread t(helper_thread_proc, &vb);
		t.detach();

		while (1) {
			int key = _getch();
			switch (key) {
			case 'a':
			case 'A': {
				vb.start_listen();
				break;
			}
			case 's':
			case 'S': {
				vb.stop_listen();
				break;
			}
			case 'q':
			case 'Q': {
				vb.quit_listen();
				std::thread t(helper_thread_proc, &vb);
				t.detach();
				break;
			}
			}
		}
	}
	else {

		vb.set_place(2, 0);
		vb.set_accent(1);


		std::thread t(helper_thread_proc, &vb);
		t.detach();

		while (1) {
			int key = _getch();
			switch (key) {
			case 'a':
			case 'A': {
				vb.start_listen();
				break;
			}
			case 's':
			case 'S': {
				vb.stop_listen();
				break;
			}
			case 'q':
			case 'Q': {
				vb.quit_listen();
				std::thread t(helper_thread_proc, &vb);
				t.detach();
				break;
			}
			}
		}
	}

}