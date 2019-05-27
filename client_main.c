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

void* clnt_view(void* arg);
// needed for login_view
User* login_view(void* arg);
int login(void* arg, User* user);
int sign_up(void* arg);
// needed for main_view
int main_view(void* arg, User* user);
int auto_matching(void* arg, User* user);
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
	User* user = (User*)malloc(sizeof(User));	

	if ((user = login_view(&sock)) != NULL)	// �α��� ����
		main_view(&sock, user);

	return NULL;
}

// ���� �� ù ȭ������ �α��θ� ���� ��
User* login_view(void* arg) {
	int sock = *((int*)arg);
	FILE* readfp = fdopen(sock, "r");
	FILE* writefp = fdopen(sock, "w");
	User* user = (User*)malloc(sizeof(User));
	char msg[BUF_SIZE];
	int answer;
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

		answer = fgetc(stdin);
		fgetc(stdin);	// ���๮�� ����
		fputc(answer, writefp);
		fflush(writefp);

		switch (answer) {
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
int login(void* arg, User* user) {
	int sock = *((int*)arg);
	FILE* readfp = fdopen(sock, "r");
	FILE* writefp = fdopen(sock, "w");
	int id_len = USER_ID_SIZE, pw_len = USER_ID_SIZE;
	char id[id_len+1], pw[pw_len+1];
	char msg[BUF_SIZE];
	char answer;
	int login_result = 0;

	fputs("\n--- �α��� ---\n", stdout);

	fputs("\n���̵� : ", stdout);
	fgets(id, sizeof(id), stdin); // ���̵� �Է�
	fputs(id, writefp);	// ������ ����
	fflush(writefp);

	fputs("\n��й�ȣ : ", stdout);
	fgets(pw, sizeof(pw), stdin); // ��й�ȣ �Է�
	fputs(pw, writefp);	// ������ ����
	fflush(writefp);
	
	if ((login_result = fgetc(readfp)) == 1) {	// �α��� ����
		strncpy(user->id, id, strlen(id)-1);
		fputs("\n�α��� ����!\n", stdout);
	}
	else {
		fputs("\n���̵�/��й�ȣ�� Ȯ�����ּ���.\n", stdout);
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

	fputs("\n--- �������� ---\n", stdout);

	fputs("\n���̵� : ", stdout);
	fgets(id, sizeof(id), stdin);
	fflush(stdin);	// �Է¹��� �ʱ�ȭ

	fputs("\n��й�ȣ : ", stdout);
	fgets(pw, sizeof(pw), stdin);
	fflush(stdin);	// �Է¹��� �ʱ�ȭ

	fputs("\n���� ������ �����Ͻðڽ��ϱ� (Y/N) ? ", stdout);
	if ((fgetc(stdin) == 'Y') || fgetc(stdin) == 'y') {
		fputs(id, writefp);
		fflush(writefp);

		fputs(pw, writefp);
		fflush(writefp);
		fflush(stdin);

		if ((fgetc(readfp)) == 1) {	// �������� ����
			fputs("\n�������� ����!\n", stdout);
			sign_up_result = 1;
		}
		else {
			fputs("\n�̹� �����ϴ� ���̵��Դϴ�.\n", stdout);
		}
		fflush(readfp);
	}

	return sign_up_result;
}

int main_view(void* arg, User* user) {
	int sock = *((int*)arg);
	FILE* readfp = fdopen(sock, "r");
	FILE* writefp = fdopen(sock, "w");
	int answer;
	
	while (1) {
		fprintf(stdout, "\n%s �� ȯ���մϴ�!\n", user->id);
		fputs("\n=== �� �� ===\n", stdout);
		fputs("\n1. ��Ī���� ���� �����ϱ�\n", stdout);
		fputs("\n2. �� �����ϱ�\n", stdout);
		fputs("\n3. �� �����ϱ�\n", stdout);
		fputs("\n4. ���� ������ �� ��ȸ\n", stdout);
		fputs("\n5. ���� ����� ��ȸ\n", stdout);
		fputs("\n6. ��ŷ ��ȸ\n", stdout);
		fputs("\n�޴��� �������ּ��� (1-6) ? ", stdout);
		fflush(stdout);

		answer = fgetc(stdin);
		fgetc(stdin);	// ���๮�� ����
		fputc(answer, writefp);
		fflush(writefp);

		switch (answer) {
		case '1':
			auto_matching(&sock, user);
			break;
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':

		default:
			fputs("\n���� �� �Է����ּ���. (1-6) ? \n", stdout);
			break;
		}
	}

	return 0;
}

int auto_matching(void* arg, User* user) {
	int sock = *((int*)arg);
	
	printf("id: %s\n", user->id);

	/*while (1) {
	fgets(msg, BUF_SIZE, readfp);
	if (!strcmp(msg, ">>\n")) {
		fputs(">> ", stdout);
		break;
	}
	fputs(msg, stdout);
	fflush(stdout);
	}*/

	return 0;
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