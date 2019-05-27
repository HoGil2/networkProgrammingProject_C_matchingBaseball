#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define USER_ID_SIZE 20

void* clnt_view(void* arg);
// needed for login_view
int login_view(void* arg);
int login(void* arg);
int sign_up(void* arg);

void* send_msg(void* arg);
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
	int login_view_result = 0;

	login_view_result = login_view(&sock);

	if (login_view_result == 1)	// �α��� ����
		main_view(&sock);

	return NULL;
}

// ���� �� ù ȭ������ �α��θ� ���� ��
int login_view(void* arg) {
	int sock = *((int*)arg);
	FILE* readfp = fdopen(sock, "r");
	FILE* writefp = fdopen(sock, "w");
	char msg[BUF_SIZE];
	int answer;
	int login_result = 0;

	/*while (1) {
		fgets(msg, BUF_SIZE, readfp);
		if (!strcmp(msg, ">>\n")) {
			fputs(">> ", stdout);
			break;
		}
		fputs(msg, stdout);
		fflush(stdout);
	}*/
	while (1) {
		fputs("=== �� �� ===\n", stdout);
		fputs("1. �α���\n", stdout);
		fputs("2. ���� ����\n", stdout);
		fputs("3. ����\n", stdout);
		fputs("�޴��� �������ּ���. (1-3). ? ", stdout);
		fflush(stdout);

		answer = fgetc(stdin);
		fgetc(stdin);	// ���๮�� ����
		fputc(answer, writefp);
		fflush(writefp);

		switch (answer) {
		case '1': // �α���
			login_result = login(&sock);	// �α��� ���

			if (login_result == 1)	// �α��� ����
				return 1;
			else	// �α��� ����
				break;
		case '2': // ���� ����
			sign_up(&sock);
			break;
		case '3':
			fputs("������ ����˴ϴ�.\n", stdout);
			return 0;	// ���� ��û
		default:
			fputs("���� �� �Է����ּ���. (1-3) ? \n", stdout);
			break;
		}
	}
	
	return 0;
}

// �α����� ���� ���̵�/��й�ȣ�� ����ڷκ��� �Է¹���
int login(void* arg) {
	int sock = *((int*)arg);
	FILE* readfp = fdopen(sock, "r");
	FILE* writefp = fdopen(sock, "w");
	int id_len = USER_ID_SIZE, pw_len = USER_ID_SIZE;
	char id[id_len+1], pw[pw_len+1];
	char msg[BUF_SIZE];
	char answer;
	int login_result = 0;

	fputs("���̵� �Է����ּ���.: ", stdout);
	fgets(id, sizeof(id), stdin); // ���̵� �Է�
	fputs(id, writefp);	// ������ ����
	fflush(writefp);

	fputs("��й�ȣ�� �Է����ּ���.: ", stdout);
	fgets(pw, sizeof(pw), stdin); // ��й�ȣ �Է�
	fputs(pw, writefp);	// ������ ����
	fflush(writefp);
	
	if ((fgetc(readfp)) == 1) {	// �α��� ����
		fputs("�α��� ����!\n", stdout);
		login_result = 1;
	}
	else {
		fputs("���̵�/��й�ȣ�� Ȯ�����ּ���.\n", stdout);
	}
	
	return login_result;	// 1 or 0
}

// ȸ�������� ���� ���̵�/��й�ȣ�� ����ڷκ��� �Է¹���
int sign_up(void* arg){
	int sock = *((int*)arg);
	FILE* readfp = fdopen(sock, "r");
	FILE* writefp = fdopen(sock, "w");
	int id_len = USER_ID_SIZE;
	int pw_len = USER_ID_SIZE;
	char id[id_len], pw[pw_len];
	int sign_up_result = 0;
	char answer;

	fputs("���̵� �Է����ּ���.: ", stdout);
	fgets(id, sizeof(id), stdin);
	fflush(stdin);	// �Է¹��� �ʱ�ȭ

	fputs("��й�ȣ�� �Է����ּ���.: ", stdout);
	fgets(pw, sizeof(pw), stdin);
	fflush(stdin);	// �Է¹��� �ʱ�ȭ

	fputs(id, writefp);
	fflush(writefp);

	fputs(pw, writefp);
	fflush(writefp);
	
	if ((fgetc(readfp)) == 1) {	// �������� ����
		fputs("�������� ����!\n", stdout);
		sign_up_result = 1;
	}
	else {
		fputs("�̹� �����ϴ� ���̵��Դϴ�.\n", stdout);
	}
	fflush(readfp);

	return sign_up_result;
}

void* send_msg(void* arg) 
{
	int sock = *((int*)arg);
	char msg[BUF_SIZE];
	while (1)
	{
		fgets(msg, BUF_SIZE, stdin);
		write(sock, msg, strlen(msg));
	}
	return NULL;
}

void* recv_msg(void* arg) 
{
	int sock = *((int*)arg);
	char msg[BUF_SIZE];
	int str_len;
	while (1)
	{
		str_len = read(sock, msg, BUF_SIZE - 1);
		if (str_len == -1)
			return (void*)-1;
		msg[str_len] = 0;
		printf("%s\n", msg);
		//fputs(msg, stdout);
	}
	return NULL;
}

void error_handling(char* msg) 
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}