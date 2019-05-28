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
	User user_list[MAX_CLNT];
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
	Room room_list[MAX_ROOM];
	int cnt; // matching room count
} MatchingRoomController;

typedef struct CreatedRoomController {
	Room room_list[MAX_ROOM];
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
int rw_login_db(char* mode, char* id, int id_len, char* pw, int pw_len);
// needed for main_view
int main_view(User* user);
int enter_matching_room(User* user);
Room* entered_Room(User* user);

// removing socket from socket array
int remove_socket(void* arg);

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
	//created_matching_rooms(); 

	while (1) {
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr*) & clnt_adr, &clnt_adr_sz);

		//pthread_create(&t_id, NULL, handle_clnt, (void*)& clnt_sock);
		//pthread_detach(t_id);
		// �α� ó�� �ʿ�
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));

		handle_clnt(&clnt_sock);
	}
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
		room->name[strlen(name)] = (i+1) + '0';	// int to char / "matching_room1",...

		// add room at room_list in roomController
		mRoomController.room_list[mRoomController.cnt++] = *room;
	}

	for (int i = 0; i < 2; i++) {
		fprintf(stdout, "%s\n", mRoomController.room_list[i].id);
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

	// �α��� �� user�� userControl.user_list�� �ְԵȴ�.
	if ((user = login_view(&clnt_sock)) != NULL) {	
		pthread_mutex_lock(&sock_mutx);	// sock mutex lock
		userController.user_list[userController.cnt++] = *user;
		pthread_mutex_unlock(&sock_mutx);	// sock mutex unlock

		main_view(user);	// main view ���� user�� ���ϵ� ����
	}

	return NULL;
}

User* login_view(void* arg) {
	int clnt_sock = *((int*)arg);
	FILE* readfp = fdopen(clnt_sock, "r");
	FILE* writefp = fdopen(clnt_sock, "w");
	User* user = (User*)malloc(sizeof(User));
	char msg[BUF_SIZE];
	int answer;
	int login_result = 0;

	while (1) {
		answer = fgetc(readfp);
		fflush(readfp);

		switch (answer) {
		case '1':
			login_result = login(&clnt_sock, user);

			if (login_result == 1)
				return user;	// user ������ ��ȯ
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
	FILE* readfp = fdopen(clnt_sock, "r");
	FILE* writefp = fdopen(clnt_sock, "w");
	char* mode = "r";
	char uid[USER_ID_SIZE];
	char upw[USER_ID_SIZE];
	int verify_result = 0;

	fgets(uid, sizeof(uid), readfp); // ����ڷκ��� ���̵� ����
	fputs(uid, stdout);
	fflush(readfp);

	fgets(upw, sizeof(upw), readfp); // ����ڷκ��� ��й�ȣ ����
	fputs(upw, stdout);
	fflush(readfp);

	verify_result = rw_login_db(mode, uid
		, strlen(uid), upw, strlen(upw)); // ���̵�/��й�ȣ Ȯ��

	if (verify_result == 1) {
		strncpy(user->id, uid, strlen(uid) - 1);	// ���� ���̵� ���
		user->sock = clnt_sock;	// ���̵�� ������ �����ϱ� ����
	}

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

	fgets(uid, sizeof(uid), readfp); // ����ڷκ��� ���̵� ����
	fputs(uid, stdout);
	fflush(readfp);

	fgets(upw, sizeof(upw), readfp); // ����ڷκ��� ��й�ȣ ����
	fputs(upw, stdout);
	fflush(readfp);

	// DB���� ���̵� �ߺ� Ȯ�� �� �߰�, -1: fgets�� ���� ���๮�� ����
	sign_up_result = rw_login_db(mode, uid
		, strlen(uid), upw, strlen(upw));	

	fputc(sign_up_result, writefp);
	fflush(writefp);

	return sign_up_result;
}

int main_view(User* user) {
	int clnt_sock = user->sock;
	FILE* readfp = fdopen(clnt_sock, "r");
	FILE* writefp = fdopen(clnt_sock, "w");
	int answer;

	while (1) {
		answer = fgetc(readfp);
		fflush(readfp);

		switch (answer) {
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
	FILE* readfp = fdopen(clnt_sock, "r");
	FILE* writefp = fdopen(clnt_sock, "w");
	Room* room;
	char msg[BUF_SIZE];

	room = entered_Room(user);

	while (room->gameController.ready_state[0] && room->gameController.ready_state[1]) {
		// ä�� �����Ӱ�
		// ����ȭ �ʿ�?
		fgets(msg, sizeof(msg), readfp);


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
		in_room_cnt = mRoomController.room_list[i].cnt;
		if (in_room_cnt < 2) {	// empty space in room
			room = &mRoomController.room_list[i];
			mRoomController.room_list[i].clnt_sock[in_room_cnt] = clnt_sock;
			entered = 1;
			fputs("���� �濡 ����\n", stdout);
			break;
			// handling?
		}
	}
	// be required mutex unlock

	if (entered == 1) {
		return room;
	}

	return NULL;
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
	// login_db.txt open �α� ó�� �ʿ�
	if ((fp = fopen("login_db.txt", mode)) == NULL) {
		error_handling("fopen() error");
	}

	if (strcmp(mode, "r") == 0) {	// �α��ο� �ش��ϹǷ� mode�� read ����
		// login_db.txt ���� ���پ� ������ "id pw\n"
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {
			printf("%s %s\n", get_id, get_pw);
			if (strcmp(uid, get_id) == 0) {	// ���̵� ��ġ
				if (strcmp(upw, get_pw) == 0) {	// ��й�ȣ ��ġ
					result = 1;	// true
				}
			}
		}
	}
	else if (strcmp(mode, "r+") == 0) {	// ȸ�����Կ� �ش��ϹǷ� mode�� rw
		// login_db.txt ���� ���پ� ������
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {	
			if (strcmp(uid, get_id) == 0) {	// ���̵� ��ġ
				is_duplicated_id = 1;				// ���̵� �ߺ� üũ
			}
		}
	}

	// ���̵� �ߺ����� ������ login_db.txt�� ���ο� ���� ���
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
		if (sock == userController.user_list[i].sock) {
			while (i++ < userController.cnt - 1)
				userController.user_list[i] = userController.user_list[i + 1];
			result = 1;
			break;
		}
	}
	userController.cnt--;
	pthread_mutex_unlock(&sock_mutx);

	return result;
}
