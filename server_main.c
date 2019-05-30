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
#define MAX_ROOM 2
#define ROOM_NAME_SIZE 21
#define ANSWER_SIZE 3

typedef struct User {
	char id[USER_ID_SIZE];
	//char pw[USER_ID_SIZE];
	int sock;
} User;

typedef struct UserController {
	User* user_list[MAX_CLNT];
	int cnt;
} UserController;

typedef struct GameController {
	int answer_list[2][3];
	int ready_state[2];
	//int turn;
} GameController;

//typedef struct MessageController {
//	char ready[BUF_SIZE];
//	char quit[BUF_SIZE];
//	char invite[BUF_SIZE];
//};

typedef struct Room {
	int id;
	char name[ROOM_NAME_SIZE];
	int clnt_sock[2];
	int cnt;
	GameController gameController;
} Room;

typedef struct MatchingRoomController {
	Room* room_list[MAX_ROOM];
	int cnt; // matching room count
} MatchingRoomController;

typedef struct CreatedRoomController {
	Room* room_list[MAX_ROOM];
	int cnt;	// created room count
} CreatedRoomController;

void error_handling(char* msg);

// client handling thread
void* handle_clnt(void* arg);
// client enter this room and play game
void create_matching_rooms();

// needed for login_view
User* login_view(void* arg);
int login(void* arg, User* user);
int sign_up(void* arg);
int rw_login_db(char* mode, char* id, char* pw);
// needed for main_view
int main_view(User* user);
int enter_matching_room(User* user);
Room* entered_Room(User* user);

// removing socket from socket array
int remove_user(User* user);

UserController userController;
MatchingRoomController mRoomController;
CreatedRoomController cRoomController;
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
	create_matching_rooms();	// matching room init

	while (1) {
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*) & clnt_adr, &clnt_adr_sz);

		pthread_create(&t_id, NULL, handle_clnt, (void*)& clnt_sock);
		pthread_detach(t_id);
		// 로그 처리 필요
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));

		//handle_clnt(&clnt_sock);
	}

	// remove_rooms();	// room 동적할당 해제
	close(serv_sock);
	return 0;
}

void create_matching_rooms() {
	char name[USER_ID_SIZE] = "matching_room";
	mRoomController.cnt = 0;	// mRoom cnt init;
	
	for (int i = 0; i < 2; i++) {
		Room* room = (Room*)malloc(sizeof(Room));
		room->id = i + 1;
		strcpy(room->name, name);
		room->name[strlen(name)] = (i + 1) + '0';	// int to char / "matching_room1",...

		// room->gameControl init
		memset(&room->gameController, 0, sizeof(room->gameController)); 

		// add room at room_list in roomController
		mRoomController.room_list[mRoomController.cnt++] = room;
	}

	for (int i = 0; i < 2; i++) {
		//fprintf(stdout, "%s\n", mRoomController.room_list[i].name);
	}
}

void error_handling(char* msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void* handle_clnt(void* arg) {
	int clnt_sock = *((int*)arg);
	User* user;

	// 로그인 후 user를 userControl.user_list에 넣게된다.
	if ((user = login_view(&clnt_sock)) != NULL) {	
		// userController userlist에 user 등록
		pthread_mutex_lock(&sock_mutx);	// sock mutex lock
		userController.user_list[userController.cnt++] = user;
		pthread_mutex_unlock(&sock_mutx);	// sock mutex unlock

		main_view(user);	// main view 진입 user에 소켓도 포함
		
		// 메인 종료 시 유저 등록 해제
		remove_user(user);
	}
	else {
		close(clnt_sock);
	}

	return NULL;
}

User* login_view(void* arg) {
	int clnt_sock = *((int*)arg);
	User* user = (User*)malloc(sizeof(User));
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int login_result = 0;
	int str_len = 0;

	memset(user, 0, sizeof(user));	// user init

	while (1) {
		str_len = read(clnt_sock, answer, sizeof(answer));
		if (str_len == -1) // read() error
			return 0;
		answer[str_len - 1] = '\0';

		switch (answer[0]) {
		case '1':
			login_result = login(&clnt_sock, user);

			if (login_result == 1)
				return user;	// user 정보를 반환
			else
				break;
		case '2':
			sign_up(&clnt_sock);
			break;
		case '3':
			return NULL;
		default:
			break;
		}
	}

	return NULL;	
}

int login(void* arg, User* user) {
	int clnt_sock = *((int*)arg);
	char mode[3] = "r";
	char uid[USER_ID_SIZE] = { 0, };
	char upw[USER_ID_SIZE] = { 0, };
	char answer[ANSWER_SIZE] = "0\n";
	char msg[BUF_SIZE] = { 0, };
	char* str;
	int verify_result = 0;
	int str_len = 0;

	// 사용자로부터 아이디 비밀번호 수신
	str_len = read(clnt_sock, msg, sizeof(msg));
	if (str_len == -1) // read() error
		return 0;
	printf("str_:%d, str:%ld\n", str_len, strlen(msg));

	str = strtok(msg, "\n");	// "\n" 기준 앞이 아이디
	strncpy(uid, str, strlen(str));
	//fputs(uid, stdout);
	fprintf(stdout, "uid: %s\n", uid);

	str = strtok(NULL, "\n");	// 나머지 비밀번호
	strncpy(upw, str, strlen(str));
	fputs(upw, stdout);

	verify_result = rw_login_db(mode, uid, upw); // 아이디/비밀번호 확인

	if (verify_result == 1) {
		answer[0] = '1';
		strncpy(user->id, uid, strlen(uid));	// 유저 아이디 등록
		//printf("%s\n", user->id);
		user->sock = clnt_sock;	// 아이디와 소켓을 맵핑하기 위해
	}

	str_len = write(clnt_sock, answer, strlen(answer));
	if (str_len == -1) // write() error
		return 0;

	return verify_result;
}

int sign_up(void* arg) {
	int clnt_sock = *((int*)arg);
	char mode[3] = "r+";	// needed for login_db fopen
	char uid[USER_ID_SIZE] = { 0, };
	char upw[USER_ID_SIZE] = { 0, };
	int sign_up_result = 0;
	char answer[ANSWER_SIZE] = { 0, };
	char msg[BUF_SIZE] = { 0, };
	char* str;
	int str_len = 0;

	str_len = read(clnt_sock, answer, sizeof(answer));
	if (str_len == -1) // read() error
		return 0;
	//printf("str_:%d, str:%ld, %s\n", str_len, strlen(answer), answer);

	if (!(strcmp(answer, "y\n")) || !(strcmp(answer, "Y\n"))) {
		str_len = read(clnt_sock, msg, sizeof(msg));
		if (str_len == -1) // read() error
			return 0;
		//printf("str_:%d, str:%ld\n", str_len, strlen(upw));

		str = strtok(msg, "\n");	// " " 기준 앞이 아이디
		strncpy(uid, str, strlen(str));
		fputs(uid, stdout);

		str = strtok(NULL, "\n");	// 나머지 비밀번호
		strncpy(upw, str, strlen(str));
		fputs(upw, stdout);

		// DB에서 아이디 중복 확인 및 추가
		sign_up_result = rw_login_db(mode, uid, upw);

		if (sign_up_result == 1) {
			answer[0] = '1';
		}

		str_len = write(clnt_sock, answer, strlen(answer));
		if (str_len == -1) // write() error
			return 0;
	}
	return sign_up_result;
}

int main_view(User* user) {
	int clnt_sock = user->sock;
	char answer[ANSWER_SIZE] = { 0, };
	int str_len = 0;

	while (1) {
		str_len = read(clnt_sock, answer, sizeof(answer));
		if (str_len == -1) // read() error
			return 0;
		answer[str_len - 1] = '\0';

		switch (answer[0]) {
		case '1':
			enter_matching_room(user);
			break;
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
			break;
		case '7':
			return 0;
		default:
			break;
		}
	}

	return 0;
}

int enter_matching_room(User* user) {
	int clnt_sock = user->sock;
	Room* room;
	char msg[BUF_SIZE];
	int str_len = 0;

	room = entered_Room(user);
	
	// 함수화 필요
	while (1) {
		while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0) {
			//if
			if (str_len == -1) // read() error
				return 0;
			msg[str_len] = '\0';

			if (room->gameController.ready_state[0] && room->gameController.ready_state[1]) {
				break;
			}

			if (room->cnt == 2) {	// 다른 사용자가 있을 시 메세지 전송
				fprintf(stdout, "%d %d\n", room->clnt_sock[0], room->clnt_sock[1]);
				if (clnt_sock == room->clnt_sock[0]) {
					str_len = write(room->clnt_sock[1], msg, str_len);
					if (str_len == -1) // write() error
						return 0;
				}
				else {
					str_len = write(room->clnt_sock[0], msg, str_len);
					if (str_len == -1) // write() error
						return 0;
				}
			}
			else {
				fputs(msg, stdout);
				if (str_len == -1) // write() error
					return 0;
			}
		}
		break;
	}

	//start_game();

	return 0;
}

Room* entered_Room(User* user) {
	int clnt_sock = user->sock;
	Room* room;
	int entered = 0;
	int in_room_cnt = 0;

	// be required mutex lock...
	for (int i = 0; i < mRoomController.cnt; i++) {
		in_room_cnt = mRoomController.room_list[i]->cnt;
		if (in_room_cnt < 2) {	// empty space in room
			room = mRoomController.room_list[i];
			mRoomController.room_list[i]->clnt_sock[in_room_cnt] = clnt_sock;
			entered = 1;
			room->cnt++;
			fprintf(stdout, "유저 방에 입장 %d %d\n", in_room_cnt, room->cnt);
			break;
			// handling?
		}
	}
	// be required mutex unlock

	if (entered == 1) {
		fputs("check\n", stdout);
		return room;
	}

	return NULL;
}

int rw_login_db(char *rw, char* id, char* pw) {
	FILE* fp = NULL;
	char mode[3];
	char uid[USER_ID_SIZE] = { 0, };
	char upw[USER_ID_SIZE] = { 0, };
	char get_id[USER_ID_SIZE] = { 0, };
	char get_pw[USER_ID_SIZE] = { 0, };
	int is_duplicated_id = 0;
	int result = 0;

	memcpy(mode, rw, strlen(rw));
	memcpy(uid, id, strlen(id));
	memcpy(upw, pw, strlen(pw));
	//fprintf(stdout, "%s %s %s\n", mode, uid, upw);

	pthread_mutex_lock(&login_db_mutx); // login_db.txt mutx lock
	// login_db.txt open 로그 처리 필요
	if ((fp = fopen("login_db.txt", mode)) == NULL) {
		error_handling("fopen() error");
	}
	if (strncmp(mode, "r", strlen(mode)) == 0) {	// 로그인에 해당하므로 mode가 read 전용
		// login_db.txt 에서 한줄씩 가져옴 "id pw\n"
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {
			if (strncmp(uid, get_id, strlen(uid)) == 0) {	// 아이디 일치
				if (strncmp(upw, get_pw, strlen(upw)) == 0) {	// 비밀번호 일치
					result = 1;	// true
				}
			}
		}
	}
	else if (strncmp(mode, "r+", strlen(mode)) == 0) {	// 회원가입에 해당하므로 mode가 rw
		// login_db.txt 에서 한줄씩 가져옴
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {	
			if (strncmp(uid, get_id, strlen(uid)) == 0) {	// 아이디 일치
				is_duplicated_id = 1;				// 아이디 중복 체크
			}
		}
	}

	// 아이디가 중복되지 않으면 login_db.txt에 새로운 계정 등록
	if (!(is_duplicated_id)) {	
		fprintf(fp, "%s %s\n", uid, upw);	// login_db.txt.에 계정 등록
		result = 1;	// true
	}
	fclose(fp);							// login_db.txt close
	pthread_mutex_unlock(&login_db_mutx); // login_db.txt mutx unlock
	
	return result;	// 1 or 0
}

// remove disconnected user
int remove_user(User* user) {
	int result = 0;

	pthread_mutex_lock(&sock_mutx);
	for (int i = 0; i < userController.cnt; i++){
		if (user == userController.user_list[i]) {
			while (i++ < userController.cnt - 1)
				userController.user_list[i] = userController.user_list[i + 1];
			result = 1;
			break;
		}
	}
	userController.cnt--;
	pthread_mutex_unlock(&sock_mutx);

	close(user->sock);	// 소켓 닫기
	free(user);	// user 동적 할당 해제

	return result;
}
