int main(int argc, char *argv[]){
	
	struct tree_node *root;
	pid_t pid;
	int status;
	
	if (argc!=2){		//elexos oti iparxei arxio
		printf("Wrong arguments!!!\n");
		exit(1);
	}
	
	root = get_tree_from_file(argv[1]); 	//an iparxe, dimiourgoume to structure k pernoume ti riza
	
	if (root== NULL){
		printf("Root not found!!!\n");
		exit(1);
	}

	pid = fork();				//dimiourgia prwtou komvou
	
	if (pid < 0) 
	{	perror("error with fork");
		exit(1);
	}

	if (pid == 0) 
	{	
		fork_procs(root);		//arxizoume ti dimiourgia diergasiwn
		exit(1);
	}
		
	wait_for_ready_children(1);		//perimenei na alla3ei katastasi to paidi tis (diladi na kanei stop i root)

	show_pstree(pid);			//etsi exoun dimiourgithei ola ta paidia kai tipwnw to dentro
	print_tree(root);			//tipwnw kai me to dentro arxeiou g elegxo apotelesmatwn
	
	kill(pid, SIGCONT);			//stelnw shma sto paidi gia na sinexistoun oi diergasies 

	pid = wait(&status);			//perimeno ton prwto komvo na pethanei	
		
	explain_wait_status(pid,status);	
	
	return 0;
}