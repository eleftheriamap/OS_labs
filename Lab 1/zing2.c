#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
int zing()
 {        
	 int k;         
	char *p;         
	p = getlogin();  
        	if (p==NULL)    //checks if error occured         
		{      			
			perror("Error getting username");                
			 return 1;         }  
        	
	k = write(1, "I am user ",10);         
	if (k==-1)         
		{       
			perror("Error in write attemp 1");                
			 return 1;         }  
        
	k = write(1, p, 10);        
 	if (k==-1)         
		{       
			perror("Error in write attemp 2");                 
			return 1;         }  
        
	k = write(1,"\n",1);        
 	if (k==-1)         
		{       
			perror("Error in write attemp 3");                
			 return 1;         }       
	  return 0; 
}  
