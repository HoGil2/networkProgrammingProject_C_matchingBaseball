#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define USER_ID_SIZE 21
#define ANSWER_SIZE 3

typedef struct User {
	char id[USER_ID_SIZE];
} User;

typedef struct MultipleArg {
	int sock;
	char id[USER_ID_SIZE];
} MultipleArg;

void* clnt_view(void* arg);
// needed for login_view
User* login_view(void* arg);
int login(void* arg, User* user);
int sign_up(void* arg);
// needed for main_view
int main_view(void* arg, User* user);
int enter_matching_room(void* arg, User* user);

void* send_msg(void* multiple_arg);
void* recv_msg(void* arg);
void error_handling(char* msg);

int main(int argc, char* argv[]) 
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void* thread_return;

	if (argc != 3) {
		printf("Usage : %s <IP> <PORT> \n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	
	clnt_view(&sock);

	/*pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);*/
	close(sock);
	return 0;
}

void* clnt_view(void* arg) {
	int sock = *((int*)arg);
	User* user = (User*)malloc(sizeof(User));
	memset(user->id, 0, sizeof(user->id));	// user init

	if ((user = login_view(&sock)) != NULL)	// 로그인 성공
		main_view(&sock, user);

	return NULL;
}

// 접속 시 첫 화면으로 로그인를 위한 뷰
User* login_view(void* arg) {
	int sock = *((int*)arg);
	//FILE* readfp = fdopen(sock, "r");
	//FILE* writefp = fdopen(sock, "w");
	User* user = (User*)malloc(sizeof(User));
	char msg[BUF_SIZE] = { 0, };
	char answer[ANSWER_SIZE] = { 0, };
	int login_result = 0;
	int str_len = 0;

	while (1) {
		fputs("* * * * * * * * * * *\n", stdout);
		fputs("*                   *\n", stdout);
		fputs("*  다자간 숫자야구  *\n", stdout);
		fputs("*                   *\n", stdout);
		fputs("* * * * * * * * * * *\n", stdout);
		fputs("\n=== 메 뉴 ===\n", stdout);
		fputs("\n1. 로그인\n", stdout);
		fputs("\n2. 계정 생성\n", stdout);
		fputs("\n3. 종 료\n", stdout);
		fputs("\n메뉴를 선택해주세요. (1-3). ? ", stdout);

		fgets(answer, sizeof(answer), stdin);
		str_len = write(sock, answer, strlen(answer));
		if (str_len == -1) // write() error
			return 0;
		
		switch (answer[0]) {
		case '1': // 로그인
			login_result = login(&sock, user);	// 로그인 결과

			if (login_result == 1) {	// 로그인 성공
				return user;
			}
			else	// 로그인 실패
				break;
		case '2': // 계정 생성
			sign_up(&sock);
			break;
		case '3':
			fputs("\n연결이 종료됩니다.\n", stdout);
			return NULL;	// 종료 요청
		default:
			fputs("\n보기 중 입력해주세요. (1-3) ? \n", stdout);
			break;
		}
	}
	
	return NULL;
}

// 로그인을 위해 아이디/비밀번호를 사용자로부터 입력받음
int login(void* arg, User* user) {
	int sock = *((int*)arg);
	char id[USER_ID_SIZE] = { 0, };
	char pw[USER_ID_SIZE] = { 0, };
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE] = { 0, };
	int login_result = 0;
	int str_len = 0;

	fputs("\n--- 로그인 ---\n", stdout);

	fputs("\n아이디 : ", stdout);
	fgets(id, sizeof(id), stdin); // 아이디 입력
	//fputs(id, stdout);

	fputs("\n비밀번호 : ", stdout);
	fgets(pw, sizeof(pw), stdin); // 비밀번호 입력
	//fputs(pw, stdout);

	sprintf(msg, "%s%s", id, pw);	// msg == "id\npw\n"
	msg[strlen(msg)] = EOF;

	str_len = write(sock, msg, strlen(msg));
	if (str_len == -1) // write() error
		return 0;
	
	str_len = read(sock, answer, sizeof(answer));
	answer[str_len - 1] = '\0';
	fputs(answer, stdout);

	if ((answer[0]) == '1') {	// 로그인 성공
		strncpy(user->id, id, strlen(id)-1);
		fputs("\n로그인 성공!\n", stdout);
		login_result = 1;
	}
	else {
		fputs("\n아이디/비밀번호를 확인해주세요.\n", stdout);
	}
	
	return login_result;	// 1 or 0
}

// 회원가입을 위해 아이디/비밀번호를 사용자로부터 입력받음
int sign_up(void* arg){
	int sock = *((int*)arg);
	char id[USER_ID_SIZE] = { 0, };
	char pw[USER_ID_SIZE] = { 0, };
	int sign_up_result = 0;
	char answer[ANSWER_SIZE] = { 0, };
	char msg[BUF_SIZE];
	int str_len = 0;

	fputs("\n--- 계정생성 ---\n", stdout);

	fputs("\n아이디 : ", stdout);
	fgets(id, sizeof(id), stdin);

	fputs("\n비밀번호 : ", stdout);
	fgets(pw, sizeof(pw), stdin);

	fputs("\n위의 내용대로 생성하시겠습니까 (Y/N) ? ", stdout);
	fgets(answer, sizeof(answer), stdin);
	str_len = write(sock, answer, strlen(answer));
	if (str_len == -1) // write() error
		return 0;

	sprintf(msg, "%s%s", id, pw);	// msg == "id\npw\n"
	msg[strlen(msg)] = EOF;

	if(!(strcmp(answer, "y\n")) || !(strcmp(answer, "Y\n"))){
		str_len = write(sock, msg, strlen(msg));	// write "id\npw\n"
		if (str_len == -1) // write() error
			return 0;
		printf("str_: %d, str: %ld", str_len, strlen(msg));

		str_len = read(sock, answer, sizeof(answer));
		if (str_len == -1) // read() error
			return 0;
		answer[str_len - 1] = '\0';
		fputs(answer, stdout);

		if ((answer[0]) == '1') {	// 계정생성 성공
			fputs("\n계정생성 성공!\n", stdout);
			sign_up_result = 1;
		}
		else {
			fputs("\n이미 존재하는 아이디입니다.\n", stdout);
		}
	}

	return sign_up_result;
}

int main_view(void* arg, User* user) {
	int sock = *((int*)arg);
	char answer[ANSWER_SIZE] = { 0, };
	int str_len = 0;
	
	while (1) {
		fprintf(stdout, "\n%s 님 환영합니다!\n", user->id);
		fputs("\n=== 메 뉴 ===\n", stdout);
		fputs("\n1. 매칭으로 게임 시작하기\n", stdout);
		fputs("\n2. 방 생성하기\n", stdout);
		fputs("\n3. 방 참가하기\n", stdout);
		fputs("\n4. 입장 가능한 방 조회\n", stdout);
		fputs("\n5. 현재 사용자 조회\n", stdout);
		fputs("\n6. 랭킹 조회\n", stdout);
		fputs("\n메뉴를 선택해주세요 (1-6) ? ", stdout);

		fgets(answer, sizeof(answer), stdin);
		str_len = write(sock, answer, strlen(answer));
		if (str_len == -1) // write() error
			return 0;

		switch (answer[0]) {
		case '1':
			enter_matching_room(&sock, user);
			break;
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':

		default:
			fputs("\n보기 중 입력해주세요. (1-6) ? \n", stdout);
			break;
		}
	}

	return 0;
}

int enter_matching_room(void* arg, User* user) {
	int sock = *((int*)arg);
	pthread_t snd_thread, rcv_thread;
	void* thread_return;
	MultipleArg* multiple_arg;
	
	multiple_arg = (MultipleArg*)malloc(sizeof(MultipleArg)); // init
	multiple_arg->sock = sock;
	memcpy(multiple_arg->id, user->id, strlen(user->id));
	
	fputs("방에 입장했습니다.\n", stdout);

	pthread_create(&snd_thread, NULL, send_msg, (void*)multiple_arg);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);

	return 0;
}

void* send_msg(void* multiple_arg) 
{
	MultipleArg* mArg = (MultipleArg*)multiple_arg;
	char msg[BUF_SIZE];
	char name_msg[USER_ID_SIZE+BUF_SIZE];
	char uid[USER_ID_SIZE] = { 0, };
	int str_len = 0;

	strncpy(uid, mArg->id, sizeof(mArg->id)); // id 복사

	while (1)
	{
		fgets(msg, sizeof(msg), stdin);
		sprintf(name_msg, "%s : %s", uid, msg);
		str_len = write(mArg->sock, name_msg, strlen(name_msg));
		if (str_len == -1) // write() error
			return 0;
		if (!strcmp(msg, "/q\n") || !strcmp(msg, "Q\n")) {
			write(mArg->sock, msg, strlen(msg));
			break;
		}
	}
	return NULL;
}

void* recv_msg(void* arg) 
{
	int sock = *((int*)arg);
	char name_msg[USER_ID_SIZE+BUF_SIZE];
	int str_len;
	while (1)
	{
		str_len = read(sock, name_msg, BUF_SIZE - 1);
		if (str_len == -1)
			return (void*)-1;
		name_msg[str_len - 1] = '\0';
		if (!strcmp(name_msg, "/q") || !strcmp(name_msg, "Q"))
			break;
		fputs(name_msg, stdout);
	}
	return NULL;
}

void error_handling(char* msg) 
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}