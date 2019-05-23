#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void error_handling(char* msg);

// client handling thread
void* handle_clnt(void* arg);

// needed for login
bool login_view(void* arg);
bool login(void* arg);
int rw_login_db(char* rw, char* id, int id_size, char* pw, int pw_size);
// needed for sign up
bool sign_up(void* arg);

bool remove_socket(void* arg);

typedef struct UserController {
	int userList[MAX_CLNT];
	int cnt;
} UserController;

typedef struct User {
	char id[20];
	//char pw[20];
	int sock;
} User;

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

		pthread_mutex_lock(&sock_mutx);
		// �α� ó�� �ʿ�
		userController.userList[userController.cnt++] = clnt_sock;
		pthread_mutex_unlock(&sock_mutx);

		pthread_create(&t_id, NULL, handle_clnt, (void*)& clnt_sock);
		pthread_detach(t_id);
		printf("Connected clien IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
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

	if (login_view(&clnt_sock) == true)
		//main_view(clnt_sock);

	return NULL;
}

bool login_view(void* arg) {
	int clnt_sock = *((int*)arg);
	char msg[][BUF_SIZE] = { "\n=== ��   �� ===\n", "1. �α���\n2. ���� ����\n3. ��   ��\n�޴��� �������ּ��� (1-3). ? "
		, "���� �� �Է����ּ���. (1-3) ? ", "������ ����Ǿ����ϴ�.\n" };
	char answer;
	bool login_result = false;
	bool sign_up_result = false;
	int str_len;
	User user;

	while (true) {
		str_len = write(clnt_sock, msg[0], sizeof(msg[0]));
		str_len = write(clnt_sock, msg[1], sizeof(msg[1]));
		//if(str_len) // �α� ó�� ����?
		str_len = read(clnt_sock, &answer, sizeof(&answer));
		//if(str_len) // �α� ó��?

		switch (answer) {
		case '1': // �α� ó�� �ʿ�
			login_result = login(&clnt_sock);
			if (login_result == true)
				return true;
			else
				break;
		case '2': // �α� ó�� �ʿ�
			sign_up(&clnt_sock);
			break;
		case '3':
			write(clnt_sock, msg[3], sizeof(msg[3]));
			remove_socket(&clnt_sock);
			return false;
		default:
			write(clnt_sock, msg[2], sizeof(msg[2]));
			break;
		}
	}
	return false;
}

bool login(void* arg) {
	int clnt_sock = *((int*)arg);
	char* mode = "r";
	int str_len[2];
	char msg[][BUF_SIZE] = { "���̵� �Է����ּ���.", "��й�ȣ�� �Է����ּ���"
		, "���̵�/��й�ȣ�� Ȯ�����ּ���." };
	char clnt_info[][BUF_SIZE] = { {0,}, {0,} };

	while (1) {
		for (int i = 0; i < 2; i++) {
			write(clnt_sock, msg[i], sizeof(msg[i]));
			str_len[i] = read(clnt_sock, clnt_info[i], BUF_SIZE);
		}

		// verify login info
		if (rw_login_db(mode, clnt_info[0], str_len[0], clnt_info[1], str_len[1]) == true) {
			break;
		}
		else {
			write(clnt_sock, msg[2], sizeof(msg[2]));
			return false;
		}
	}

	return true;
}

bool sign_up(void* arg) {
	int clnt_sock = *((int*)arg);
	char *mode = "r+";	// needed for login_db fopen
	int str_len[3];
	char msg[][BUF_SIZE] = { "\n=== ���� ���� ===\n���̵�: ", "��й�ȣ: "
		, "���� ������ �����Ͻðڽ��ϱ� (Y/N) ?", "�̹� �����ϴ� ���̵�. �ٸ� ���̵� �Է����ּ���.: " };
	char clnt_info[][BUF_SIZE] = { {0,}, {0,} };
	char confirm;
	int sign_up_result = 0;

	while (1)
	{
		for (int i = 0; i < 3; i++) {
			write(clnt_sock, msg[i], sizeof(msg[i]));
			str_len[i] = read(clnt_sock, clnt_info[i], BUF_SIZE);
		}
		read(clnt_sock, &confirm, sizeof(char));
		if (confirm=='y' || confirm=='Y') {
			sign_up_result = rw_login_db(mode, clnt_info[0], str_len[0], clnt_info[1], str_len[1]);

			// write user id/pw
			if (sign_up_result == 1) {
				break;
			}
			else if (sign_up_result == 2) {
				write(clnt_sock, msg[3], sizeof(msg[3]));
			}
			else {
				error_handling("rw_login_db() error");
			}
		}
	}
	return true;
}

int rw_login_db(char *rw, char* id, int id_size, char* pw, int pw_size) {
	FILE* fp = NULL;
	char* mode = rw;
	int uid_size = id_size;
	int upw_size = pw_size;
	char* uid = id;
	char* upw = pw;
	char get_id[uid_size];
	char get_pw[upw_size];
	int is_duplicated_id = 0;
	int result = 0;

	pthread_mutex_lock(&login_db_mutx); // login_db.txt mutx lock
	if ((fp = fopen("login_db.txt", mode)) == NULL) {// login_db.txt open �α� ó�� �ʿ�
		error_handling("fopen() error");
	}

	if (strcmp(mode, "r") == 0) {	// �α��ο� �ش��ϹǷ� mode�� read ����
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {	// login_db.txt ���� ���پ� ������
			if (strncmp(get_id, uid, uid_size) == 0) {	// ���̵� ��ġ
				if (strncmp(get_pw, upw, upw_size) == 0) {	// ��й�ȣ ��ġ
					result = 1;	// true
				}
			}
		}
	}
	else if (strcmp(mode, "r+") == 0) {	// ȸ�����Կ� �ش��ϹǷ� mode�� rw
		while (fscanf(fp, "%s %s\n", get_id, get_pw) != EOF) {	// login_db.txt ���� ���پ� ������
			if (strncmp(get_id, uid, uid_size) == 0) {	// ���̵� ��ġ
				is_duplicated_id = 1;				// ���̵� �ߺ�üũ
				result = 2;
			}
		}
	}

	if ((strcmp(mode, "r+") == 0) && (is_duplicated_id != 1)) {	// ���̵� �ߺ����� ������ login_db.txt�� ���ο� ���� ���
		for (int i = 0; i < uid_size; i++)	// ���̵� �Է�
			fputc(uid[i], fp);
		fputs(" ", fp);						// ��й�ȣ�� ������ " " ����
		for (int i = 0; i < upw_size; i++)	// ��й�ȣ �Է�
			fputc(upw[i], fp);
		fputs("\n", fp);
		result = 1;	// true
	}
	fclose(fp);							// login_db.txt close
	pthread_mutex_unlock(&login_db_mutx); // login_db.txt mutx unlock
	
	return result;
}

// remove disconnected client
bool remove_socket(void* arg) {
	bool result = false;
	int sock = *((int*)arg);

	pthread_mutex_lock(&sock_mutx);
	for (int i = 0; i < userController.cnt; i++)
	{
		if (sock == userController.userList[i])
		{
			while (i++ < userController.cnt - 1)
				userController.userList[i] = userController.userList[i + 1];
			result = true;
			break;
		}
	}
	userController.cnt--;
	pthread_mutex_unlock(&sock_mutx);

	return result;
}
