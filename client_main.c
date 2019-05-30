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
int create_room(void* arg, User* user);		//�� �����
int enter_room(void* arg, User* user);		//�� ����
int search_room(void* arg);		//�� ��� ����
int search_user(void* arg);		//������ ����� ����
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

	while (view_mode > 0) {		//�α��κ� ����
		if ((user = login_view(&sock)) == NULL) {	// �α��� ����
			view_mode--;
			continue;
		}
		view_mode++;

		while (view_mode > 1) {		//�κ�� ����
			if (!main_view(&sock, user)) {
				view_mode--;
				continue;
			}
			view_mode++;

			while (view_mode > 2) {		//��� ����
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

// ���� �� ù ȭ������ �α��θ� ���� ��
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
		fputs("*  ���ڰ� ���ھ߱�  *\n", stdout);
		fputs("*                   *\n", stdout);
		fputs("* * * * * * * * * * *\n", stdout);
		fputs("\n=== �� �� ===\n", stdout);
		fputs("\n1. �α���\n", stdout);
		fputs("\n2. ���� ����\n", stdout);
		fputs("\n3. �� ��\n", stdout);
		fputs("\n�޴��� �������ּ���. (1-3). ? ", stdout);
		fflush(stdout);

		fgets(answer, sizeof(answer), stdin);
		write(sock, answer, sizeof(answer));
		fflush(stdin);

		switch (answer[0]) {
		case '1': // �α���
			login_result = login(&sock, user);	// �α��� ���

			if (login_result == 1) {	// �α��� ����
				return user;
			}
			else	// �α��� ����
				break;
		case '2': // ���� ����
			sign_up(&sock);
			break;
		case '3':
			fputs("\n������ ����˴ϴ�.\n", stdout);
			return NULL;	// ���� ��û
		default:
			fputs("\n���� �� �Է����ּ���. (1-3) ? \n", stdout);
			break;
		}
	}

	return NULL;
}

// �α����� ���� ���̵�/��й�ȣ�� ����ڷκ��� �Է¹���
int login(void* arg, User * user) {
	int sock = *((int*)arg);
	int id_len = USER_ID_SIZE, pw_len = USER_ID_SIZE;
	char id[id_len + 1], pw[pw_len + 1];
	char msg[BUF_SIZE];
	char answer[ANSWER_SIZE];
	int login_result = 0;

	fputs("\n--- �α��� ---\n", stdout);

	fputs("\n���̵� : ", stdout);
	fgets(id, sizeof(id), stdin); // ���̵� �Է�
	//fflush(stdin);

	fputs("\n��й�ȣ : ", stdout);
	fgets(pw, sizeof(pw), stdin); // ��й�ȣ �Է�
	//fflush(stdin);
	sprintf(msg, "%s%s", id, pw);
	write(sock, msg, sizeof(msg));
	printf("�α�����...\n");
	sleep(0.5);
	read(sock, answer, sizeof(answer));
	if ((answer[0]) == '1') {	// �α��� ����
		strncpy(user->id, id, strlen(id) - 1);
		fputs("\n�α��� ����!\n", stdout);
		login_result = 1;
	}
	else {
		fputs("\n���̵�/��й�ȣ�� Ȯ�����ּ���.\n", stdout);
	}

	return login_result;	// 1 or 0
}

// ȸ�������� ���� ���̵�/��й�ȣ�� ����ڷκ��� �Է¹���
int sign_up(void* arg) {
	int sock = *((int*)arg);
	int id_len = USER_ID_SIZE;
	int pw_len = USER_ID_SIZE;
	char id[id_len], pw[pw_len], id_pw[id_len + pw_len];
	int sign_up_result = 0;
	char answer[ANSWER_SIZE];

	fputs("\n--- �������� ---\n", stdout);

	fputs("\n���̵� : ", stdout);
	fgets(id, sizeof(id), stdin);
	//fflush(stdin);	// �Է¹��� �ʱ�ȭ

	fputs("\n��й�ȣ : ", stdout);
	fgets(pw, sizeof(pw), stdin);
	//fflush(stdin);	// �Է¹��� �ʱ�ȭ
	fputs("\n���� ������ �����Ͻðڽ��ϱ� (Y/N) ? ", stdout);
	fgets(answer, sizeof(answer), stdin);

	if (strcmp(answer, "Y\n") || strcmp(answer, "y\n")) {
		sprintf(id_pw, "%s%s", id, pw);
		write(sock, id_pw, sizeof(id_pw));
		printf("���� ������...\n");
		sleep(0.5);
		read(sock, answer, sizeof(answer));
		if ((answer[0]) == 1) {	// �������� ����
			fputs("\n�������� ����!\n", stdout);
			sign_up_result = 1;
		}
		else {
			fputs("\n�̹� �����ϴ� ���̵��Դϴ�.\n", stdout);
		}
	}

	return sign_up_result;
}

int main_view(void* arg, User * user) {
	int sock = *((int*)arg);
	char answer[ANSWER_SIZE];

	fprintf(stdout, "\n%s �� ȯ���մϴ�!\n", user->id);

	while (1) {
		fputs("\n=== �� �� ===\n", stdout);
		fputs("\n1. ��Ī���� ���� �����ϱ�\n", stdout);
		fputs("\n2. �� �����ϱ�\n", stdout);
		fputs("\n3. �� �����ϱ�\n", stdout);
		fputs("\n4. ���� ������ �� ��ȸ\n", stdout);
		fputs("\n5. ���� ����� ��ȸ\n", stdout);
		fputs("\n6. ��ŷ ��ȸ\n", stdout);
		fputs("\n7. �α׾ƿ�\n", stdout);
		fputs("\n�޴��� �������ּ��� (1-6) ? ", stdout);
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
			printf("������ �����մϴ�.\n\n");
			return 0;
		default:
			fputs("\n���� �� �Է����ּ���. (1-6) ? \n", stdout);
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

int create_room(void* arg, User * user) {		//�� ����
	int sock = *(int*)arg;
	char name[21];
	fputs("\n������ �� �̸� : ", stdout);
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

	fputs("\n������ �� ��ȣ�� �Է����ּ���. ", stdout);
	fgets(name, sizeof(name), stdin);
	str_len = write(sock, name, strlen(name));
	if (str_len == -1)
		return 0;

	printf("%s(��)�� ������... ", name);

	read(sock, answer, sizeof(answer));	// ���� ��� ����

	if (!strcmp(msg, "1")) {
		printf("\n���� ����\n");
	}
	else {
		printf("\n���� ����\n");
		room_view(&sock, user);	// ����
	}
	return 0;
}

int search_room(void* arg) {		//�� id name ���
	int sock = *(int*)arg;
	char msg[1000];

	fputs("\n�� ��ȸ\n", stdout);
	read(sock, msg, sizeof(msg) - 1);
	fputs(msg, stdout);
	return 0;
}

int search_user(void* arg) {	// �������� ����� ��ȸ
	int sock = *(int*)arg;
	char msg[1000];

	fputs("\n����� ��ȸ\n", stdout);
	read(sock, msg, sizeof(msg) - 1);
	fputs(msg, stdout);
	return 0;
}

int room_view(void* arg, User * user) { //�̿�
	MultipleArg player;
	player.sock = *(int*)arg;
	strcpy(player.id, user->id);
	char msg[BUF_SIZE];
	pthread_t rd, wr;
	void* thread_return;

	printf("�� ����\n%s :", user->id);
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
	printf("�� ����\n");
	return 0;
}


void* send_msg(void* multiple_arg)
{
	MultipleArg mArg = *((MultipleArg*)multiple_arg);
	char msg[BUF_SIZE];
	char name_msg[USER_ID_SIZE + BUF_SIZE];
	char uid[USER_ID_SIZE];

	strncpy(uid, mArg.id, sizeof(mArg.id)); // id ����

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