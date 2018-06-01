#include <stdio.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/vfs.h>
#include <dirent.h>

using namespace cv;
using namespace std;

#define BUFFSIZE 100
#define MINI_BUFFSIZE 100

/* [상황에 맞춰 변경해줘야할 변수들?]
 *
 * (경로)
 * - const char* record_list_path : "/home/pi/openCVtest/BlackBox/Record/Dir_List.txt"
 *   	>> Dir_List.txt의 경로를 저장한다.
 * - const char* listcount_path : "/home/pi/openCVtest/BlackBox/Record/Dir_Count.txt"
 *   	>> Dir_Count.txt의 경로를 저장한다.
 * - char path_buffer[BUFFSIZE] : "/home/pi/openCVtest/BlackBox/Record/%Y%m%d_%H/"
 *   	>> '년월일_시'로 만들 디렉토리의 경로
 * - char List_buffer[BUFFSIZE]
 *   	>> path_buffer에서 마지막의 "/"를 뺀 나머지 부분 (자동으로 값 들어감)
 * - statfs("/home/pi/openCVtest/BlackBox/Record/", &s)
 *   	>> statfs함수를 통해 정보를 가져오기 위해 마운트 할 곳의 경로
 * - char buffer[MINI_BUFFSZE] : "%4Y%2m%2d_%2H%2M%2S.avi"
 *   	>> 디렉토리 안에 만들 영상파일의 이름
 * - 가장 안쪽 while문 안에서의 buffer : "%4Y-%2m-%2d %2H:%2M:%2S"
 *   	currenttime[MINI_BUFFSIZE]     : buffer + millsec(2자리)
 *   	>> 화면에서 시간 출력을 위한 buffer
 *
 * (해상도, fps)
 * - Size size = Size( Width, Height)
 * - video.set(CAP_PROP_FPS,30.0) : 30.0 부분에 fps(초당 프레임수)를 저장한다.
 */


int rmdirs(const char *path, int is_error_stop)
{
    DIR *  dir_ptr      = NULL;
    struct dirent *file = NULL;
    struct stat   buf;
    char   filename[1024];
	
	/* struct dirent{
	 * 	long d_ino;			// I-노드 번호
	 * 	off_t d_off;			// offset
	 * 	unsigned short d_reclen;	// 파일 이름 길이
	 * 	char d_name[NAME_MAX+1];	// 파일 이름
	 * };
	 */

	/* struct stat{
	 * 	mode_t st_mode;			// 파일 타입과 퍼미션
	 * 	ino_t st_ino;			// i-node 번호
	 *	dev_t st_dev;			// 장치 번호
	 *	dev_t st_rdev;			// 특수 파일의 장치 번호
	 *	nlink_t st_nlink;		// 링크 수
	 *	uid_t st_uid;			// 소유자의 USER ID
	 *	gid_t st_gid;			// 소유자의 GROUP ID
	 *	off_t st_size;			// 정규 파일의 바이트 수
	 *	time_t st_atime;		// 마지막 접근 시각
	 *	time_t st_mtime;		// 마지막 수정 시각
	 *	time_t st_ctime;		// 마지막 상태 변경 시각
	 *	long st_blksize;		// 마지막 상태 변경 시각
	 *	long st_blocks;			// 할당한 블록의 개수
	 * };
	 */
	 
	
	 // 목록을 읽을 디렉토리명으로 DIR *를 return받는다.
	 // path가 디렉토리가 아니라면 삭제하고 종료한다.
    if((dir_ptr = opendir(path)) == NULL) {
		return unlink(path);
    }

	 /* int unlink(const char* pathname)
	  * 헤더 : unistd.h
	  * 설명 : pathname에서 지정한 파일을 삭제한다.
	  */


	 // 디렉토리의 처음부터 파일 또는 디렉토리명을 순서대로 한개씩 읽는다.
    while((file = readdir(dir_ptr)) != NULL) {

		 // readdir 읽혀진 파일명 중에 현재 디렉토리를 나타내는 .도 포함되어 있으므로
		 // 무한반복에 빠지지 않기 위해 파일명이 .이라면 skip해야 한다.
        if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {
             continue;
        }
        
        sprintf(filename, "%s/%s", path, file->d_name);
        	
		 // 파일의 속성(파일의 유형, 크기, 생성시간/변경시간 등을 가져온다.
        if(lstat(filename, &buf) == -1) {
            continue;
        }

		 // 검색된 이름의 속성이 디렉토리이면
        if(S_ISDIR(buf.st_mode)) { 
			 // 검색된 파일이 directory이면 재기호출로 하위 디렉토리를 다시 검색
            if(rmdirs(filename, is_error_stop) == -1 && is_error_stop) {
                return -1;
            }
			 // 일반파일 또는 symbolic link이면
        } else if(S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) { 
            if(unlink(filename) == -1 && is_error_stop) {
                return -1;
            }
        }
    }
    
	 // open된 directory정보를 close한다.
    closedir(dir_ptr);
    
    return rmdir(path);
}


int main(int argc, char *argv[])
{
	int err_Check;
	
	// 디렉토리 목록.txt 를 관리하기 위한 변수들
	const char* record_list_path= "/home/pi/openCVtest/BlackBox/Record/Dir_List.txt";
	FILE *fwp, *frp;
	int Dir_length;
	int Dir_Count=0;
	// Dir_Count의 값을 백업하고 불러오기 위한 파일포인터및 경로
	const char* listcount_path= "/home/pi/openCVtest/BlackBox/Record/Dir_Count.txt";
	FILE *fwp2, *frp2;
	
// 가장 먼저 Camer가 제대로 연결되었는지 확인한다.
//
	VideoCapture video(0);

	if(!video.isOpened())
	{
		puts("(ERROR)Camera Device가 제대로 연결되지 않았습니다");
		return -1;
	}
	

	Mat frame;
	
	// 웹캠에서 캡쳐되는 이미지 크기를 가져옴
	Size size = Size((int)video.get(CAP_PROP_FRAME_WIDTH),
			(int)video.get(CAP_PROP_FRAME_HEIGHT));

	video.set(CAP_PROP_FPS, 30.0);



// 영상의 크기를 확인하는 방법?
// (결과 파일엔 불필요하므로 주석처리)
// 
//	Size out_size = Size((int)video.get(CAP_PROP_FRAME_WIDTH), (int)video.get(CAP_PROP_FRAME_HEIGHT));
//	cout << "Video Size : " << out_size << endl;
	


	bool CONTINUED = 1;

	while(CONTINUED)
	{
		
	// 년월일_시분초 에 해당하는 디렉토리를 만들자
	//
		// 디렉토리 생성을 위한 변수
		int mkdir_status;
		char path_buffer[BUFFSIZE];	

		// 디렉토리 이름에 시간을 반영하기 위한 변수	
		time_t cur = time(NULL);
		struct tm* ptm = localtime(&cur);	

		// 버퍼 충돌 방지를 위한 memset
		memset(path_buffer,'\0',BUFFSIZE);	

	
		// path_buffer에 현재시각을 반영한 경로를 문자열의 형태로 넣는다.
		err_Check = strftime(path_buffer,sizeof(path_buffer), "/home/pi/openCVtest/BlackBox/Record/%Y%m%d_%H/",ptm);
		if(err_Check ==0 )	
		{
			puts("(ERROR) 디렉토리를 만들기 위한 경로지정을 할 수 없습니다.");
			return -2;
		}

//		시간이 잘 버퍼에 잘 저장되었는지 확인하는 방법?
//		결과 파일엔 불필요하므로 주석처리
//		puts(time_buffer);
	
	// "년월일_시" 의 이름을 갖는 디렉토리를 생성한다.
	//
		err_Check = mkdir(path_buffer, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);


		// 디렉토리 생성도중 에러가 발생했다면 종료한다
		// 단, 이미 디렉토리가 존재하는 경우엔, 에러는 발생하지만 종료할 필요는 없으므로 예외처리를 해준다.
		if(err_Check == -1 && errno != EEXIST)
		{
			puts("(ERROR) 디렉토리를 정상적으로 만들 수 없습니다.");
			
			return -2;
		}

		
		// 디렉토리가 기존에 존재하지 않아서 새로 만들었다면, 
		// 그 디렉토리의 이름을 txt파일의 맨 끝에 새로 적어준다. 
		if(err_Check != -1)
		{
			
			printf("디렉토리생성) ");
			puts(path_buffer);
			fwp = fopen(record_list_path, "a+");
			if(fwp == NULL) 
			{
				puts("File Open ERROR");
				return -5;
			}

			char List_buffer[BUFFSIZE];
			memset(List_buffer,'\0',BUFFSIZE);

			// strlen(path_buffer) -1 을 해준이유는 path_buffer의 마지막에 있는 /를 제거하기 위함이다.	
			strncpy(List_buffer,path_buffer,strlen(path_buffer)-1);

			// 파일의 맨 끝으로 커서를 이동한다.
			//int fseek_err_Check = fseek(fwp,0,SEEK_END);

			int fwp_err_check = fwrite(List_buffer,1,strlen(List_buffer),fwp);
			
			// fread에서 사용하기 위해서 경로의 길이를 저장해둔다.
			Dir_length = strlen(List_buffer);
			
			
			fclose(fwp);
		}

	// 파일 시스템의 전체 용량중 남은 공간이 어느정도 인지 그 퍼센트를 계산한다.
	//

		int ret;
		struct statfs s;
		long used_blocks;
		long used_percent;
		long free_percent;



		// 인자로 주어진 파일/디렉토리의 마운트된 곳의 정보를 가져온다.
	    	if ( statfs("/home/pi/openCVtest/BlackBox/Record/", &s) !=0) 
		{
		 	perror("statfs");
		  	return -6;
	      	}

 	     	if (s.f_blocks >0) 
		{
		      	used_blocks = s.f_blocks - s.f_bfree;
    
	    		if (used_blocks ==0)
				used_percent = 0;
			else 
			{
				used_percent = (long)(used_blocks * 100.0 / (used_blocks + s.f_bavail) + 0.5);
			}
  

			if (s.f_bfree ==0)
				free_percent = 0;
			else 
			{
				free_percent = (long)(s.f_bavail * 100.0 / (s.f_blocks) + 0.5);
			}
   
	     
		}

	// 파일 시스템의 남은 용량이 20% 이하이면 가장 오래된 Directory를 삭제한다.
	//
		if(free_percent<=20)

		{
			// Dir_count의 값을 불러온다.
			frp2 = fopen(listcount_path, "r");
			fscanf(frp2,"%d",&Dir_Count);
			fclose(frp2);


			frp = fopen(record_list_path, "r");
			if(frp == NULL) 
			{
				puts("File Open ERROR");
				return -5;
			}
			
			char List_buffer[BUFFSIZE];
			memset(List_buffer,'\0',BUFFSIZE);
			
			// 가장 오래된 Directory를 찾기 위해서
			// Dir_length만큼의 텍스트를 읽어온다.
			// 다음 Directory를 지정해주기 위해 fseek 함수로 커서를 해당하는 위치로 옮겨준다.
			int fseek_err_Check = fseek(frp,Dir_length*Dir_Count,SEEK_SET);
			int frp_err_check = fread(List_buffer,1,Dir_length,frp);
			Dir_Count++;
			
			fclose(frp);


			// Dir_count의 값을 백업한다.
			fwp2 = fopen(listcount_path, "w+");
			fprintf(fwp2,"%d",Dir_Count++);
			fclose(fwp2);

			// 읽어온 Dirctory를 삭제시킨다.
			int tmppp = rmdirs(List_buffer,1);

			puts("");
			puts("! Memory가 20% 이하입니다.");
			printf("디렉토리삭제) ");
			puts(List_buffer);
			puts("");
		}

	// 시간에 해당하는 디렉토리에 녹화파일이 저장되도록 만들어주는 과정
	// 지정한 디렉토리를 현재 작업위치로 만들어준다.
	//
		err_Check = chdir(path_buffer);
		if(err_Check!=0)
		{
			puts("(ERROR) 디렉토리를 찾을 수 없습니다.");
			return -2;
		}

		
	// 파일로 동영상을 저장하기 위한 준비
	//
		cur = time(NULL);
		ptm = localtime(&cur);
		char buffer[MINI_BUFFSIZE];
		memset(buffer,'\0',MINI_BUFFSIZE);
		err_Check=strftime(buffer, MINI_BUFFSIZE, "%4Y%2m%2d_%2H%2M%2S.avi",ptm);
		if(err_Check == 0)
		{
			puts("(ERROR) 디렉토리에 접근할 수 없습니다.");
			return -2;
		}

		VideoWriter outputVideo;
		outputVideo.open(buffer, VideoWriter::fourcc('X','V','I','D'),
			30,size,true);
		if(!outputVideo.isOpened())
		{
			puts("(ERROR) 영상 저장을 위한 초기화 작업중 에러 발생");
			return -3;
		}
		
		
	// 지정된 시간이 지나면 자동으로 저장을 하고 재녹화를 하기 위하여 만들어 준 시간변수들
	// 
		time_t t_start;
		time_t t_end;
		t_start = time(NULL);
		struct tm* start_ptm = localtime(&t_start);
		struct tm* end_ptm;

		int start_hour = start_ptm->tm_hour;

	// 웹캠으로부터 영상을 가져와 창에 출력한다.
	//
		
		while(1)
		{
			// 웹캠으로부터 한 프레임을 읽어옴
			video >> frame;
	
			// 웹캠에서 캡처되는 속도를 가져옴
			int fps = video.get(CAP_PROP_FPS);
			int wait = int(1.0 / fps * 1000);
			
	
		// 화면에 출력할 시간에 대한 포맷생성
		//
			timeval curTime;
			gettimeofday(&curTime,NULL);
			int milli = curTime.tv_usec / 10000;	//2자리까지만 필요함
	
			time_t rawtime;
			
			time(&rawtime);
			ptm = localtime(&rawtime);
			
			// 버퍼의 충돌을 방지하기위해 memset을한다.
			memset(buffer,'\0',MINI_BUFFSIZE);

			err_Check = strftime(buffer, 30, "%4Y-%2m-%2d %2H:%2M:%2S",ptm);
			if(err_Check == 0)
			{
				puts("(ERROR) Display 시간표시에 에러발생");
				return -4;
			}

			// millisecond까지 표시하기 위하여!
			char currenttime[MINI_BUFFSIZE];

			// 버퍼의 충돌방지
			memset(currenttime, '\0', MINI_BUFFSIZE);

			sprintf(currenttime, "%s:%02d", buffer, milli);
	

		// 화면에 현재 시간 출력하기
		//
			int myFont = FONT_HERSHEY_SIMPLEX;	// 숫자넣어도된다.
			double myFontSize = 0.6;
			Scalar Red(0,0,255);
			int myFontThickness = 2;
			putText(frame, currenttime, Point(10,470), myFont,myFontSize, Red, myFontThickness); 
			
	
	
		// 화면에 영상을 보여줌
		//
			imshow("Blackbox", frame);
			
		// 동영상 파일에 한프레임을 저장함.
		//
			outputVideo << frame;
	
		// 60초가 지나면 저장메시지를 출력하고, 재녹화를 시작한다.
		//
			t_end = time(NULL);
			end_ptm = localtime(&t_end);

			if( (end_ptm->tm_hour) != (start_hour) ) 
			{
				printf("%s recorded successfully\n",buffer);
				break;
			}
			
			if(t_end - t_start == 60) 
			{
				printf("%s recorded successfully\n",buffer);
				break;
			}
			
		// ESC키를 입력하면 프로그램을 종료한다.
		//
			if(waitKey(wait) == 27) {CONTINUED = 0; break;}
			
		} 


	}
	
	


	return 0;
}
