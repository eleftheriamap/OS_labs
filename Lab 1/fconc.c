#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void doWrite(int fd, const char *buff, int len){
	size_t idx = 0;
	ssize_t wcnt;
		do {
			wcnt = write(fd,buff + idx, len - idx);
			if (wcnt == -1){ /* error */
				perror("write");
				exit(1);
			}
			idx += wcnt;
		} while (idx < len);
		return;
}

void write_file(int fdw, int ftr){
	char buff[1024];
	ssize_t rcnt = read(ftr,buff,sizeof(buff));
	for (;;){
		if (rcnt == 0) /* end-of-file */
			return;
		if (rcnt == -1){ /* error */
			perror("read");
			exit(1);
		}
		doWrite(fdw,buff,rcnt);
		rcnt = read(ftr,buff,sizeof(buff));
	}
}



int main(int argc, char **argv)
{
	//Check inputs
    if ( ( argc < 3 ) || (argc > 4) ) {
		write(1, "Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n",61) ;
        return(1) ;
    }

	//open read A
	int fda,fdb,fdc;
	fda = open(argv[1], O_RDONLY); // flag = read only
	if (fda == -1){
		perror("open");
		return(1);
	}
	
	//open read B
	fdb = open(argv[2], O_RDONLY); // flag = read only
	if (fdb == -1){
		perror("open");
		return(1);
	}	
	
	char *fout;
    if ( argc == 4 )
		fout = argv[3] ;
    else
		fout = "fconc.out" ;

	//open write C
	int oflags, mode;
	oflags = O_CREAT|O_WRONLY|O_TRUNC;
	// flag O_CREAT => create the file if it does not already exist
	// flag O_WRONLY => open for writing only
	// flag O_TRUNC => truncate existing file to zero length (if file already exists)
	mode = S_IRUSR|S_IWUSR;
	// mode : the permissions to be set on the file , only if open() creates it
	// mode S_IRWXU => permissions read , write , execute/search by owner
	// mode S_IRWXG => permissions read , write , execute/search by group
	// mode S_IRWXO => permissions read , write , execute/search by others
	fdc = open(fout, oflags, mode);
	if (fdc == -1){
	perror("error open");
	return(1);
	}
	
	write_file(fdc,fda);
	write_file(fdc,fdb);
	
	close(fda);
	close(fdb);
	close(fdc);
	
	return (0);
}
