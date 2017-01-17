#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <string>
#include <errno.h>
#include <process.h>
#include <time.h>
#include <list>

#include "VoiceBuild.h"



int main(int argc, char* argv[]){
	VoiceBuild vb;
	vb.set(0,0);
	while (true){
		vb.start_build();
	}

}