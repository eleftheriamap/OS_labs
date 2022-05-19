#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

void fork_procs(struct tree_node *root,int* pfd)
{	
	printf("PROCESS %s CREATED WITH PID = %ld\n",root->name,(long)getpid());
	change_pname(root->name); //������ �������� ����������
	int i;
	int pfd_child[2];
	int num1,num2,answer;

	if ((root->nr_children)==0){//��� � ��������� ����� �����,������ ����� �������
			int num = atoi (root->name);	//��������� ��� string �� integer
			if ((num==0)&& strcmp(root->name,"0")!=0){	//E�� ������� ������ ��� ������
				exit(69);
			}
			//������� ��� ����� ��� ���������� ��� pipe ��� ����������� � �������
			if ( write(pfd[1],&num, sizeof(int)) != sizeof(int) ) {	
				perror("error: write to pipe");
				exit(1);
			}
			sleep(SLEEP_PROC_SEC);
	}
	else{
		printf("PID = %ld: Creating pipe...\n",(long)getpid());
		//���������� pipe ��� ����������� ��� ������ �� �� ������
		if (pipe(pfd_child) < 0) {
			perror("error:creating pipe");
			exit(1);
		}
		
		for (i=0; i < 2; i++){
			pid_t child = fork();	//���������� �������
			if (child < 0) {
				perror("main: fork");
				exit(1);
			}
			if (child == 0) {
				fork_procs(root->children+i,pfd_child); //�������� ��� ���� �����. �� ������� ���������� ������� �� pipe ��� ������ ��� ����� �� ������ �� �����
				exit(1);
			}
			printf("Parent, PID = %ld: Created child with PID = %ld, waiting for it to terminate...\n",(long)getpid(), (long)child);
		}

	
	wait_for_dead_children(2);	//������� ��� ���������� ��� �������

	//�������� ��� �� pipe ��� ����� ��� ������� �� ������
	if (  read(pfd_child[0], &num1, sizeof(int) ) != sizeof(int) ) {
		perror("child: read from pipe");
		exit(1);
		}

	if ( read(pfd_child[0], &num2, sizeof(int) ) != sizeof(int) ) {
		perror("child: read from pipe");
		exit(1);
		}

	if (strcmp(root->name, "*")==0){//�������� ���������������
		answer=num1*num2;
			printf("%d %d %d\n\n",answer,num1,num2);
	}
	else
	if (strcmp(root->name, "+")==0)
		answer=num1+num2;	//�������� ���������
	else{	
		exit(69);
	}
	
	//������� ��� ������������� ��� pipe ��� ����������� � ������� ��� ���������� 
	if (write(pfd[1],&(answer), sizeof(int)) != sizeof(int)) {
			perror("error: write to pipe");
			exit(1);
			}

	
	}
	
	exit(0);
}



int main(int argc, char *argv[])
{

	struct tree_node *root;
	int pfd[2];
	int status;
	int finalAnswer;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);
	print_tree(root);
	
	if (root==NULL)
		exit(1);
	//���������� pipe ��� ����������� ��� main �� ��� ������ ��� �������
	printf("Parent: Creating pipe...\n");
	if (pipe(pfd) < 0) {
		perror("pipe");
		exit(1);
	}
	
	printf("MAIN PROCESS: Creating child...\n");
	pid_t pid = fork();//���������� ��� ������� ��� �������
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		printf("ROOT_PROCESS CREATED WITH PID = %ld\n",(long)getpid());
		fork_procs(root,pfd);
		exit(1);
	}


	
	show_pstree(pid);

	wait(&status);	//������� ��� ���������� ��� ����������
	explain_wait_status(pid, status);
	
	//�������� ��� ������ ��������� ��� ��� ������ ��� �������
	if (read(pfd[0], &finalAnswer, sizeof(finalAnswer)) != sizeof(finalAnswer)) {
		perror("child: read from pipe");
		exit(1);
		}
	printf("The final answer is: %d\n\n",finalAnswer);

	return 0;
}
