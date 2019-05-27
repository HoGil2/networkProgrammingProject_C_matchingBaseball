#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define USER_ID_SIZE 21

void error_handling(char* msg);

// client handling thread
void* handle_clnt(void* arg);

// needed for login_view
int login_view(void* arg);
int login(void* arg);
int sign_up(void* arg);
int rw_login_db(char* mode, char* id, int id_len, char* pw, int pw_len);
// removing socket from socket array
int remove_socket(void* arg);

typedef struct User {
	char id[USER_ID_SIZE+1];
	//char pw[USER_ID_SIZE];
	int sock;
} User;

typedef struct UserController {
	User userList[MAX_CLNT];
	int cnt;
} UserController;


UserController userController;
pthread_mutex_t sock_mutx;
pthread_mutex_t login_db_mutx;

int main(int argc, char* argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;

	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr*) & serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	userController.cnt = 0; // userController init

	while (1) {
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*) & clnt_adr, &clnt_adr_sz);

		/*pthread_mutex_lock(&sock_mutx);
		userController.userList[userController.cnt++].sock = clnt_sock;
		pthread_mutex_unlock(&sock_mutx);

		pthread_create(&t_id, NULL, handle_clnt, (void*)& clnt_sock);
		pthread_detach(t_id);*/
		// 로그 처리 필요
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
		login_view(&clnt_sock);
	}
	close(serv_sock);
	return 0;
}

void error_handling(char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void* handle_clnt(void* arg) {
	int clnt_sock = *((int*)arg);

	if (login_view(&clnt_sock) == 1)
		//main_view(clnt_sock);

	return NULL;
}

int login_view(void* arg) {
	int clnt_sock = *((int*)arg);
	FILE* readfp = fdopen(clnt_sock, "r");
	FILE* writefp = fdopen(clnt_sock, "w");
	char msg[BUF_SIZE];
	int answer;
	int login_result = 0;

	while (1) {
		answer = fgetc(readfp);
		fflush(readfp);

		switch (answer) {
		case '1':
			login_result = login(&clnt_sock);

			if (login_result == 1)
				return 1;
			else
				break;
		case '2':
			sign_up(&clnt_sock);
			break;
		case '3':
			return 0;
		default:
			break;
		}
	}

	return 0;	
}

int login(void* arg) {
	int clnt_sock = *((int*)arg);
	FILE* readfp = fdopen(clnt_sock, "r");
	FILE* writefp = fdopen(clnt_sock, "w");
	char* mode = "r";
	char uid[USER_ID_SIZE];
	char upw[USER_ID_SIZE];
	int verify_result = 0;

	fgets(uid, sizeof(uid), readfp); // 사용자로부터 아이디 수신
	fputs(uid, stdout);
	fflush(readfp);

	fgets(upw, sizeof(upw), readfp); // 사용자로부터 비밀번호 수신
	fputs(upw, stdout);
	fflush(readfp);

	verify_result = rw_login_db(mode, uid
		, strlen(uid), upw, strlen(upw)); // 아이디/비밀번호 확인

	fputc(verify_result, writefp);
	fflush(writefp);

	return verify_result;
}


int sign_up(void* arg) {
	int clnt_sock = *((int*)arg);
	FILE* readfp = fdopen(clnt_sock, "r");
	FILE* writefp = fdopen(clnt_sock, "w");
	char *mode = "r+";	// needed for login_db fopen
	char uid[USER_ID_SIZE];
	char upw[USER_ID_SIZE];
	int sign_up_result = 0;

	fgets(uid, sizeof(uid), readfp); // 사용자로부터 아이디 수신
	fputs(uid, stdout);
	fflush(readfp);

	fgets(upw, sizeof(upw), readfp); // 사용자로부터 비밀번호 수신
	fputs(upw, stdout);
	fflush(readfp);

	// DB에서 아이디 중복 확인 및 추가, -1: fgets로 인한 개행문자 제거
	sign_up_result = rw_login_db(mode, uid
		, strlen(uid), upw, strlen(upw));	

	fputc(sign_up_result, writefp);
	fflush(writefp);

	return sign_up_result;
}

int rw_login_db(char *rw, char* id, int id_size, char* pw, int pw_size) {
	FILE* fp = NULL;
	char* mode = rw;
	int uid_size = id_size;
	int upw_size = pw_size;
	char uid[uid_size];
	char upw[upw_size];
	char get_id[uid_size];
	char get_pw[upw_size];
	int is_duplicated_id = 0;
	int result = 0;

	memcpy(uid, id, strlen(id) - 1);
	memcpy(upw, pw, strlen(pw) - 1);

	pthread_mutex_lock(&login_db_mutx); // login_db.txt mutx lock
	// login_db.txt open 로그 처리 필요
	if ((fp = fopen("login_db.txt", mode)) == NULL) {
		error_handling("fopen() error");
	}

	if (strcmp(mode, "r") == 0) {	// 로그인에 해당하므로 mode가 read 전용
		// login_db.txt 에서 한줄씩 가져옴 "id pw\n"
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {
			printf("%s %s\n", get_id, get_pw);
			if (strcmp(uid, get_id) == 0) {	// 아이디 일치
				if (strcmp(upw, get_pw) == 0) {	// 비밀번호 일치
					result = 1;	// true
				}
			}
		}
	}
	else if (strcmp(mode, "r+") == 0) {	// 회원가입에 해당하므로 mode가 rw
		// login_db.txt 에서 한줄씩 가져옴
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {	
			if (strcmp(uid, get_id) == 0) {	// 아이디 일치
				is_duplicated_id = 1;				// 아이디 중복 체크
			}
		}
	}

	// 아이디가 중복되지 않으면 login_db.txt에 새로운 계정 등록
	if ((strcmp(mode, "r+") == 0) && (is_duplicated_id != 1)) {	
		fprintf(fp, "%s %s\n", uid, upw);
		result = 1;	// true
	}
	fclose(fp);							// login_db.txt close
	pthread_mutex_unlock(&login_db_mutx); // login_db.txt mutx unlock
	
	return result;	// 1 or 0
}

// remove disconnected client
int remove_socket(void* arg) {
	int result = 0;
	int sock = *((int*)arg);

	pthread_mutex_lock(&sock_mutx);
	for (int i = 0; i < userController.cnt; i++){
		if (sock == userController.userList[i].sock){
			while (i++ < userController.cnt - 1)
				userController.userList[i] = userController.userList[i + 1];
			result = 1;
			break;
		}
	}
	userController.cnt--;
	pthread_mutex_unlock(&sock_mutx);

	return result;
}
