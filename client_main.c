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
int room_view(void* arg, User* user);
int enter_matching_room(void* arg, User* user);
int create_room(void* arg, User* user);		//방 만들기
int enter_room(void* arg, User* user);		//방 들어가기
int search_room(void* arg);		//방 목록 보기
int search_user(void* arg);		//접속한 사용자 보기
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

	if (connect(sock, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) == -1)
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
	int view_mode = 1;

	while (view_mode > 0) {		//로그인뷰 루프
		if ((user = login_view(&sock)) == NULL) {	// 로그인 성공
			view_mode--;
			continue;
		}
		view_mode++;

		while (view_mode > 1) {		//로비뷰 루프
			if (!main_view(&sock, user)) {
				view_mode--;
				continue;
			}
			view_mode++;

			while (view_mode > 2) {		//룸뷰 루프
				if (room_view(&sock, user) == 3) {
					view_mode--;
					continue;
				}
				view_mode--;
			}
		}
	}
	return NULL;
}

// 접속 시 첫 화면으로 로그인를 위한 뷰
User* login_view(void* arg) {
	int sock = *((int*)arg);
	//FILE* readfp = fdopen(sock, "r");
	//FILE* writefp = fdopen(sock, "w");
	User* user = (User*)malloc(sizeof(User));
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int login_result = 0;

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
		fflush(stdout);

		fgets(answer, sizeof(answer), stdin);
		write(sock, answer, sizeof(answer));
		fflush(stdin);

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
int login(void* arg, User * user) {
	int sock = *((int*)arg);
	int id_len = USER_ID_SIZE, pw_len = USER_ID_SIZE;
	char id[id_len + 1], pw[pw_len + 1];
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int login_result = 0;

	fputs("\n--- 로그인 ---\n", stdout);

	fputs("\n아이디 : ", stdout);
	fgets(id, sizeof(id), stdin); // 아이디 입력
	//fflush(stdin);

	fputs("\n비밀번호 : ", stdout);
	fgets(pw, sizeof(pw), stdin); // 비밀번호 입력
	//fflush(stdin);
	sprintf(msg, "%s%s", id, pw);
	write(sock, msg, sizeof(msg));
	printf("로그인중...\n");
	sleep(0.5);
	read(sock, answer, sizeof(answer));
	if ((answer[0]) == '1') {	// 로그인 성공
		strncpy(user->id, id, strlen(id) - 1);
		fputs("\n로그인 성공!\n", stdout);
		login_result = 1;
	}
	else {
		fputs("\n아이디/비밀번호를 확인해주세요.\n", stdout);
	}

	return login_result;	// 1 or 0
}

// 회원가입을 위해 아이디/비밀번호를 사용자로부터 입력받음
int sign_up(void* arg) {
	int sock = *((int*)arg);
	int id_len = USER_ID_SIZE;
	int pw_len = USER_ID_SIZE;
	char id[id_len], pw[pw_len], id_pw[id_len + pw_len];
	int sign_up_result = 0;
	char answer[ANSWER_SIZE];

	fputs("\n--- 계정생성 ---\n", stdout);

	fputs("\n아이디 : ", stdout);
	fgets(id, sizeof(id), stdin);
	//fflush(stdin);	// 입력버퍼 초기화

	fputs("\n비밀번호 : ", stdout);
	fgets(pw, sizeof(pw), stdin);
	//fflush(stdin);	// 입력버퍼 초기화
	fputs("\n위의 내용대로 생성하시겠습니까 (Y/N) ? ", stdout);
	fgets(answer, sizeof(answer), stdin);

	if (strcmp(answer, "Y\n") || strcmp(answer, "y\n")) {
		sprintf(id_pw, "%s%s", id, pw);
		write(sock, id_pw, sizeof(id_pw));
		printf("계정 생성중...\n");
		sleep(0.5);
		read(sock, answer, sizeof(answer));
		if ((answer[0]) == 1) {	// 계정생성 성공
			fputs("\n계정생성 성공!\n", stdout);
			sign_up_result = 1;
		}
		else {
			fputs("\n이미 존재하는 아이디입니다.\n", stdout);
		}
	}

	return sign_up_result;
}

int main_view(void* arg, User * user) {
	int sock = *((int*)arg);
	char answer[ANSWER_SIZE];

	fprintf(stdout, "\n%s 님 환영합니다!\n", user->id);

	while (1) {
		fputs("\n=== 메 뉴 ===\n", stdout);
		fputs("\n1. 매칭으로 게임 시작하기\n", stdout);
		fputs("\n2. 방 생성하기\n", stdout);
		fputs("\n3. 방 참가하기\n", stdout);
		fputs("\n4. 입장 가능한 방 조회\n", stdout);
		fputs("\n5. 현재 사용자 조회\n", stdout);
		fputs("\n6. 랭킹 조회\n", stdout);
		fputs("\n7. 로그아웃\n", stdout);
		fputs("\n메뉴를 선택해주세요 (1-6) ? ", stdout);
		fflush(stdout);

		fgets(answer, sizeof(answer), stdin);
		write(sock, answer, sizeof(answer));
		fflush(stdin);

		switch (answer[0]) {
		case '1':
			enter_matching_room(&sock, user);
			break;
		case '2':
			create_room(&sock, user);
			break;
		case '3':
			enter_room(&sock, user);
			break;
		case '4':
			search_room(&sock);
			break;
		case '5':
			search_user(&sock);
			break;
		case '6':
			break;
		case '7':
			printf("접속을 종료합니다.\n\n");
			return 0;
		default:
			fputs("\n보기 중 입력해주세요. (1-6) ? \n", stdout);
			break;
		}
	}

	return 0;
}

int enter_user_room(void* arg, User * user) {
	int sock = *((int*)arg);
	int str_len = 0;
	char msg[BUF_SIZE] = { 0, };



}

int enter_matching_room(void* arg, User * user) {
	int sock = *((int*)arg);
	pthread_t snd_thread, rcv_thread;
	void* thread_return;
	MultipleArg* multiple_arg;

	multiple_arg = (MultipleArg*)malloc(sizeof(MultipleArg)); // init
	multiple_arg->sock = sock;
	memcpy(multiple_arg->id, user->id, strlen(user->id));

	pthread_create(&snd_thread, NULL, send_msg, (void*)multiple_arg);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)& sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);

	return 0;
}

int create_room(void* arg, User * user) {		//방 생성
	int sock = *(int*)arg;
	char name[21];
	fputs("\n생성할 방 이름 : ", stdout);
	fgets(name, sizeof(name), stdin);
	write(sock, name, sizeof(name));

	room_view(&sock, user);

	return 0;
}

int enter_room(void* arg, User * user) {		//
	int sock = *(int*)arg;
	char name[21];
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int str_len = 0;

	fputs("\n입장할 방 번호를 입력해주세요. ", stdout);
	fgets(name, sizeof(name), stdin);
	str_len = write(sock, name, strlen(name));
	if (str_len == -1)
		return 0;

	printf("%s(으)로 입장중... ", name);

	read(sock, answer, sizeof(answer));	// 입장 결과 수신

	if (!strcmp(msg, "1")) {
		printf("\n입장 실패\n");
	}
	else {
		printf("\n입장 성공\n");
		room_view(&sock, user);	// 입장
	}
	return 0;
}

int search_room(void* arg) {		//룸 id name 출력
	int sock = *(int*)arg;
	char msg[1000];

	fputs("\n방 조회\n", stdout);
	read(sock, msg, sizeof(msg) - 1);
	fputs(msg, stdout);
	return 0;
}

int search_user(void* arg) {	// 접속중인 사용자 조회
	int sock = *(int*)arg;
	char msg[1000];

	fputs("\n사용자 조회\n", stdout);
	read(sock, msg, sizeof(msg) - 1);
	fputs(msg, stdout);
	return 0;
}

int room_view(void* arg, User * user) { //미완
	MultipleArg player;
	player.sock = *(int*)arg;
	strcpy(player.id, user->id);
	char msg[BUF_SIZE];
	pthread_t rd, wr;
	void* thread_return;

	printf("방 입장\n%s :", user->id);
	pthread_create(&rd, NULL, recv_msg, (void*)& player);
	pthread_create(&wr, NULL, send_msg, (void*)& player);
	pthread_join(wr, &thread_return);
	pthread_join(rd, &thread_return);
	/*
	while (1) {
		fgets(msg, sizeof(msg), stdin);
		if (strcmp(msg, "q\n") || strcmp(msg, "Q\n")) {
			write(sock, "0", sizeof("0"));
			break;
		}
		else if (strcmp(msg, "r\n") || strcmp(msg, "R\n")) {
			write(sock, "1", sizeof("1"));
		}
		else {
			fputs(msg, stdout);
		}
	}
	*/
	printf("방 퇴장\n");
	return 0;
}


void* send_msg(void* multiple_arg)
{
	MultipleArg mArg = *((MultipleArg*)multiple_arg);
	char msg[BUF_SIZE];
	char name_msg[USER_ID_SIZE + BUF_SIZE];
	char uid[USER_ID_SIZE];

	strncpy(uid, mArg.id, sizeof(mArg.id)); // id 복사

	while (1)
	{
		fgets(msg, sizeof(msg), stdin);
		sprintf(name_msg, "%s: %s", uid, msg);
		write(mArg.sock, name_msg, strlen(name_msg));
		if (!strcmp(msg, "/q\n") || !strcmp(msg, "Q\n")) {

			break;
		}
	}
	return NULL;
}

void* recv_msg(void* arg)
{
	MultipleArg mArg = *((MultipleArg*)arg);
	char name_msg[USER_ID_SIZE + BUF_SIZE];
	int str_len;
	while (1)
	{
		str_len = read(mArg.sock, name_msg, USER_ID_SIZE + BUF_SIZE);
		if (str_len == -1)
			return (void*)-1;
		name_msg[str_len] = 0;
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