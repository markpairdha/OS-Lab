#include "MfsLibrary.h"
#include <unistd.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define fAttrSize 11 //in bytes 8+3
#define fNameSize 8
#define fOffsSize 3

//scans the disk for existing file or opens new file.
filehandler myfopen(char *file_name)
{
	int disk=open("Mfs.dat", O_RDWR, S_IRUSR | S_IWUSR);

	if(disk<0) {printf("Disk not present\n");return -1;}

	char *fAttr = (char*)malloc(sizeof(char)*fAttrSize);
	char *fName = (char*)malloc(sizeof(char)*fNameSize);

	char *filename = (char*)malloc(sizeof(char)*fNameSize);
	memcpy(filename, file_name, fNameSize);

	int Offs;

	while(1)
	{
		if(read(disk, (void*)fAttr, fAttrSize)!=fAttrSize)
		{
			printf("Error in read\n");
			return -1;
		}

		lseek(disk,-fAttrSize,SEEK_CUR);

		memcpy((void*)fName, (void*)fAttr, fNameSize);
		memcpy((void*)(&Offs), (void*)(fAttr+fNameSize), fOffsSize);

			
		printf("Filename: %s length: %lu\n",fName,strlen(fName));

		if(strlen(fName)==0)
			break;

		if(strcmp(filename,fName)==0)
			return disk;

		if(lseek(disk,Offs,SEEK_CUR)<0)
		{
			printf("No File and not space.\n");
			return -1;
		}
	}

	printf("File not found. Creating new file\n");

	if(write(disk,(void*)filename,fNameSize)!=fNameSize)
	{
		printf("Error in writing filename\n");
		return -1;
	}

	lseek(disk,-fNameSize,SEEK_CUR);
	return disk;
}

int myfread(void *out, int howmanybytes, filehandler fh)
{
	lseek(fh, fNameSize, SEEK_CUR);

	int bytes;

	char* ex = (char*)malloc(sizeof(char)*fOffsSize);

	if(read(fh, (void*)ex, fOffsSize)!=fOffsSize)
	{
		printf("Error in reading\n");
		return -1;
	}

	memcpy((void*)(&bytes), (void*)ex, fOffsSize);

	if(bytes<howmanybytes)
	{
		printf("File has only %d bytes, reading %d bytes\n",bytes,bytes);
		howmanybytes=bytes;
	}

	if(read(fh, out, howmanybytes)!=howmanybytes)
	{
		printf("Error in reading\n");
		return -1;
	}

	printf("Read data: %s\n", (char*)out);

	lseek(fh, 0, SEEK_SET);

	return howmanybytes;
}

int myfwrite(void *in, int howmanybytes, filehandler fh)
{
	char *fAttr = (char*)malloc(sizeof(char)*fAttrSize);
	char *fName = (char*)malloc(sizeof(char)*fNameSize);
	char *filename = malloc(sizeof(char)*fNameSize);
	char* ex = (char*)malloc(sizeof(char)*fOffsSize);
	memcpy((void*)ex, (void*)(&howmanybytes), fOffsSize);
	char *freename = (char*)malloc(sizeof(char)*fNameSize);
	freename = "freearea";

	int bytes;

	if(read(fh, (void*)fAttr, fAttrSize)!=fAttrSize)
	{
		printf("Error in read\n");
		return -1;
	}

	lseek(fh,-fAttrSize,SEEK_CUR);

	memcpy((void*)fName, (void*)fAttr, fNameSize);
	memcpy((void*)(&bytes), (void*)(fAttr+fNameSize), fOffsSize);

	memcpy(filename, fName, fNameSize);


	if(bytes==0)
	{
		lseek(fh, -fOffsSize, SEEK_CUR);
		if(write(fh,(void*)ex,fOffsSize)!=fOffsSize)
		{
			printf("Error in writing\n");
			return -1;
		}
		if(write(fh, in, howmanybytes)!=howmanybytes)
		{
			printf("Error in writing\n");
			return -1;
		}		
	}

	else if(howmanybytes<=bytes && bytes-howmanybytes>=fAttrSize)
	{	
		lseek(fh, -fOffsSize, SEEK_CUR);
		if(write(fh,(void*)ex,fOffsSize)!=fOffsSize)
		{
			printf("Error in writing\n");
			return -1;
		}		
		if(write(fh, in, howmanybytes)!=howmanybytes)
		{
			printf("Error in writing\n");
			return -1;
		}
		if(write(fh,(void*)freename,fNameSize)!=fNameSize)
		{
			printf("Error in writing\n");
			return -1;
		}

		bytes=bytes-howmanybytes-fAttrSize;
		memcpy((void*)ex, (void*)(&bytes), fOffsSize);

		if(write(fh,(void*)ex,fOffsSize)!=fOffsSize)
		{
			printf("Error in writing\n");
			return -1;
		}				
	}

	else if(howmanybytes<=bytes)
	{
		if(write(fh, in, howmanybytes)!=howmanybytes)
		{
			printf("Error in writing\n");
			return -1;
		}		
	}

	else
	{
		lseek(fh,-fAttrSize,SEEK_CUR);
		if(write(fh,(void*)freename,fNameSize)!=fNameSize)
		{
			printf("Error in writing\n");
			return -1;
		}

		lseek(fh,0,SEEK_SET);
		while(1)
		{
			if(read(fh, (void*)fAttr, fAttrSize)!=fAttrSize)
			{
				printf("Error in read\n");
				return -1;
			}

			lseek(fh,-fAttrSize,SEEK_CUR);

			memcpy((void*)fName, (void*)fAttr, fNameSize);
			memcpy((void*)(&bytes), (void*)(fAttr+fNameSize), fOffsSize);

			if(strcmp(freename,fName)==0 && bytes>=howmanybytes)
			{
				if(bytes-howmanybytes>=fAttrSize)
				{
					if(write(fh,(void*)filename,fNameSize)!=fNameSize)
					{
						printf("Error in writing\n");
						return -1;
					}
					if(write(fh,(void*)ex,fOffsSize)!=fOffsSize)
					{
						printf("Error in writing\n");
						return -1;
					}		
					if(write(fh, in, howmanybytes)!=howmanybytes)
					{
						printf("Error in writing\n");
						return -1;
					}
					if(write(fh,(void*)freename,fNameSize)!=fNameSize)
					{
						printf("Error in writing\n");
						return -1;
					}

					bytes=bytes-howmanybytes-fAttrSize;
					memcpy((void*)ex, (void*)(&bytes), fOffsSize);

					if(write(fh,(void*)ex,fOffsSize)!=fOffsSize)
					{
						printf("Error in writing\n");
						return -1;
					}
				}
				else
				{
					if(write(fh,(void*)filename,fNameSize)!=fNameSize)
					{
						printf("Error in writing\n");
						return -1;
					}
					lseek(fh, fOffsSize, SEEK_CUR);
					if(write(fh, in, howmanybytes)!=howmanybytes)
					{
						printf("Error in writing\n");
						return -1;
					}
				}
			}

			else if(strlen(fName)==0)
			{
				if(write(fh,(void*)filename,fNameSize)!=fNameSize)
				{
					printf("Error in writing\n");
					return -1;
				}				
				if(write(fh,(void*)ex,fOffsSize)!=fOffsSize)
				{
					printf("Error in writing\n");
					return -1;
				}
				if(write(fh, in, howmanybytes)!=howmanybytes)
				{
					printf("Error in writing\n");
					return -1;
				}					
				break;
			}

			if(lseek(fh,bytes,SEEK_CUR)<0)
			{
				printf("No File and not space.\n");
				return -1;
			}
		}
	}

	lseek(fh,0,SEEK_SET);

	return howmanybytes;

}