//#include <WINSOCK2.H>
//#include <mutex>
//#include "VoiceBuild.h"
//
//
//#pragma comment(lib,"WS2_32.lib")
//
//#define __DEBUG__
//#define KEY "speechOnline"
//#define LOADLIB "loadLib"
//#define UI "UI"
//#define ACTUALSHOOTING "actualShooting"
//#define ABC "abc"
//#define BASICSHOOTING "basicShooting"
//#define VIDEOTEACHING "videoTeaching"
//#define SPEECHCONTROL "speechControl"
//#define QUIT "quit"
//#define EMT "emergencyHandling"
//#define DAILY "dailyPatrol"
//#define ACCENT "accent"
//
//
//WSADATA wsaData;//初始化
//SOCKET RecvSocket;
//sockaddr_in RecvAddr;//服务器地址
//
//sockaddr_in SenderAddr;
//int SenderAddrSize;
//
//std::mutex m;
//void speech_thread(VoiceBuild* vb)
//{
//	if (!vb) return;
//	m.lock();
//	vb->start_build();
//
//#ifdef __DEBUG__
//	printf("thread exit\n");
//#endif // __DEBUG__
//	m.unlock();
//	return;
//}
//
//
//int main(){
//	
//	//ShowWindow(::FindWindow(L"ConsoleWindowClass", NULL), SW_HIDE);
//	int Port = 8889;//服务器监听地址
//	char RecvBuf[1024];//发送数据的缓冲区
//	char SendBuf[1024];
//	int BufLen = 1024;//缓冲区大小
//	SenderAddrSize = sizeof(SenderAddr);
//	const char* split = "#";
//	char *temp = NULL;
//	bool connected = false;
//
//	VoiceBuild vb;
//
//	try {
//		memset(RecvBuf, 0, 1024 * sizeof(char));
//		memset(SendBuf, 0, 1024 * sizeof(char));
//
//
//		//初始化Socket
//		WSAStartup(MAKEWORD(2, 2), &wsaData);
//		//创建接收数据报的socket
//		RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//		//
//		//将socket与制定端口和0.0.0.0绑定
//		RecvAddr.sin_family = AF_INET;
//		RecvAddr.sin_port = htons(Port);
//		RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
//		bind(RecvSocket, (SOCKADDR *)&RecvAddr, sizeof(RecvAddr));
//
//		//调用Recvfrom函数在绑定的socket上接收数据
//		printf("receiving datagrams...\n");
//		
//	}
//	catch (std::exception& e) {
//		char errorcode[1024];
//		sprintf(errorcode, e.what());
//		size_t size = sizeof(errorcode);
//		wchar_t *buffer = new wchar_t[size + 1];
//		MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
//		buffer[size] = 0;
//		MessageBox(NULL, TEXT("语音绑定通信失败!请检查端口占用后，重新启动。"), buffer, MB_ICONEXCLAMATION);
//		delete buffer;
//		goto exit;
//	}
//	catch (...) {
//		MessageBox(NULL, TEXT("语音绑定通信失败!请检查端口占用后，重新启动。"), TEXT("语音端错误"), MB_ICONEXCLAMATION);
//		goto exit;
//	}
//	while (true){
//		try {
//			int irecv = recvfrom(RecvSocket, RecvBuf, BufLen, 0, (SOCKADDR *)&SenderAddr, &SenderAddrSize);
//			if (irecv == SOCKET_ERROR) {
//				printf("recv error:%d\n", WSAGetLastError());
//				throw WSAGetLastError();
//			}
//			temp = strtok(RecvBuf, split);
//			if (strcmp(temp, KEY) == 0) {
//#ifdef __DEBUG__
//				printf("BUF:%s\n", temp);
//#endif
//				strcpy(SendBuf, "speechOnline#");
//				int iSend = sendto(RecvSocket, SendBuf, BufLen, 0, (SOCKADDR *)&SenderAddr, SenderAddrSize);
//				if (iSend == SOCKET_ERROR) {
//					printf("sendto()Failed：%d\n", WSAGetLastError());
//					throw WSAGetLastError();
//				}
//				connected = true;
//				vb.set_accent(MANDARIN);
//			}
//			else if (strcmp(temp, LOADLIB) == 0) {
//				temp = strtok(NULL, split);
//				if (strcmp(temp, UI) == 0) {
//					vb.quit_listen();
//					vb.set_place(UIBNF, 0);
//					std::thread t(speech_thread, &vb);
//					t.detach();
//				}
//				else if (strcmp(temp, ACTUALSHOOTING) == 0) {
//					temp = strtok(NULL, split);
//					vb.quit_listen();
//					vb.set_place(BACKBNF, 0);
//					if (strcmp(temp, ABC) == 0) {
//						vb.set_place(BACKBNF, 0);
//					}
//					std::thread t(speech_thread, &vb);
//					t.detach();
//
//				}
//				else if (strcmp(temp, BASICSHOOTING) == 0) {
//					vb.quit_listen();
//					vb.set_place(BACKBNF, 0);
//					std::thread t(speech_thread, &vb);
//					t.detach();
//				}
//				else if (strcmp(temp, VIDEOTEACHING) == 0) {
//					vb.quit_listen();
//					vb.set_place(BACKBNF, 0);
//					std::thread t(speech_thread, &vb);
//					t.detach();
//				}
//				else if (strcmp(temp, EMT) == 0) {
//					temp = strtok(NULL, split);
//					vb.quit_listen();
//					vb.set_place(EMTBNF, 0);
//					if (strcmp(temp, ABC) == 0) {
//						vb.set_place(EMTBNF, 0);
//					}
//					std::thread t(speech_thread, &vb);
//					t.detach();
//				}
//				else if (strcmp(temp, DAILY) == 0) {
//					temp = strtok(NULL, split);
//					vb.quit_listen();
//					vb.set_place(DAILYBNF, 0);
//					if (strcmp(temp, ABC) == 0) {
//						vb.set_place(DAILYBNF, 0);
//					}
//					std::thread t(speech_thread, &vb);
//					t.detach();
//				}
//			}
//			else if (strcmp(temp, SPEECHCONTROL) == 0) {
//				temp = strtok(NULL, split);
//				if (strcmp(temp, QUIT) == 0) {
//					vb.quit_listen();
//					break;
//				}
//				else if (strcmp(temp, ACCENT) == 0) {
//					temp = strtok(NULL, split);
//					if (strcmp(temp, "mandarin") == 0) {
//						vb.quit_listen();
//						vb.set_accent(MANDARIN);
//						vb.set_place(UIBNF, 0);
//						std::thread t(speech_thread, &vb);
//						t.detach();
//					}
//					else if (strcmp(temp, "cantonese") == 0) {
//						vb.quit_listen();
//						vb.set_accent(CANTONESE);
//						vb.set_place(UIBNF, 0);
//						std::thread t(speech_thread, &vb);
//						t.detach();
//					}
//					else if (strcmp(temp, "sichuanese") == 0) {
//						vb.quit_listen();
//						vb.set_accent(SICHUANESE);
//						vb.set_place(UIBNF, 0);
//						std::thread t(speech_thread, &vb);
//						t.detach();
//					}
//					else if (strcmp(temp, "henanese") == 0) {
//						vb.quit_listen();
//						vb.set_accent(HENANESE);
//						vb.set_place(UIBNF, 0);
//						std::thread t(speech_thread, &vb);
//						t.detach();
//					}
//					else if (strcmp(temp, "dongbeiese") == 0) {
//						vb.quit_listen();
//						vb.set_accent(DONGBEIESE);
//						vb.set_place(UIBNF, 0);
//						std::thread t(speech_thread, &vb);
//						t.detach();
//					}
//
//				}
//			}
//		}
//		catch (int d) {
//			char errorcode[1024];
//			sprintf(errorcode, "Error Code:%d", d);
//			size_t size = sizeof(errorcode);
//			wchar_t *buffer = new wchar_t[size + 1];
//			MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
//			buffer[size] = 0;
//			MessageBox(NULL, TEXT("语音通信失败!请检查socket，重新启动。"), buffer, MB_ICONEXCLAMATION);
//			delete buffer;
//		}
//		catch (std::exception& e) {
//			char errorcode[1024];
//			sprintf(errorcode, e.what());
//			size_t size = sizeof(errorcode);
//			wchar_t *buffer = new wchar_t[size + 1];
//			MultiByteToWideChar(CP_ACP, 0, errorcode, size, buffer, size * sizeof(wchar_t));
//			buffer[size] = 0;
//			MessageBox(NULL, TEXT("语音通信失败!请重新启动。"), buffer, MB_ICONEXCLAMATION);
//			delete buffer;
//		}
//		catch (...) {
//			MessageBox(NULL, TEXT("语音通信失败!请重新启动。"), TEXT("语音端错误"), MB_ICONEXCLAMATION);
//		}
//	}
//
//exit:
//	//关闭socket，结束接收数据 
//	printf("finished receiving,closing socket..\n");
//	closesocket(RecvSocket);
//	//释放资源，退出
//#ifdef __DEBUG__
//	printf("Exiting.\n");
//#endif 
//	WSACleanup();
//	return 0;
//}