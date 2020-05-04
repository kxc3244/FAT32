/*
****STUDENT 1****
Name: Kevin Chawla
ID: 1001543244

****STUDENT 2****
Name: Javier Gonzalez
ID:1001580485 
*/
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <ctype.h>

uint16_t BPB_BytesPerSec;
uint8_t  BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t  BPB_NumFATs;
uint16_t BPB_RootEntCnt;
uint32_t BPB_FATSz32;

struct __attribute__((__packed__)) DirectoryEntry
{
	char DIR_Name[11];
	uint8_t Dir_Attr;	
	uint8_t  Unused1[8];
	uint16_t DIR_FirstClusterHigh;
	uint8_t  Unused[4];
	uint16_t DIR_FirstClusterLow;
	uint32_t DIR_FileSize;
	
};

struct DirectoryEntry dir[16];

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  int already_open = 0;
  int root_cluster;
  int offset;
  int prev[2];
  
  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }
    //Compare the give filename with the command open and start a stream for the
    //said filename and then read the FAT32 filesystem for required information
    //to perform other tasks necessary in a file system
    FILE *fp;
    if(strcmp(token[0],"open")  ==  0)
    {
      if(already_open == 1)
      {
        printf("Error: File system image already open.\n");
      }
      else if(already_open == 0)
      {
  	    fp = fopen(token[1],"r");
        already_open = 1;
	//Moving the file pointer and reading all the values from the file system
        fseek(fp,11,SEEK_SET);
    	  fread(&BPB_BytesPerSec,2,1,fp);

    	  fseek(fp,13,SEEK_SET);
    	  fread(&BPB_SecPerClus,1,1,fp);

    	  fseek(fp,14,SEEK_SET);
    	  fread(&BPB_RsvdSecCnt,2,1,fp);

    	  fseek(fp,16,SEEK_SET);
    	  fread(&BPB_NumFATs,1,1,fp);

    	  fseek(fp,36,SEEK_SET);
      	fread(&BPB_FATSz32,4,1,fp);
        
        root_cluster = (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
        offset = root_cluster;
        fseek(fp, offset, SEEK_SET);
        fread(&dir[0], 16, sizeof(struct DirectoryEntry), fp);

        if(!fp)
    	  {
    	    printf("Error: File system image not found.\n");
          already_open = 0;
    	  }
      }
    }
    //Close the file stream after done manipulating the file system. 
    // The step is important for the prevention of memory waste as
    // if we don't close the stream it will not be closed until the 
    //system restarts and will slow down our application
    else if(strcmp(token[0],"close")  ==  0)
    {
      if(already_open == 0)
      {
        printf("Error: File system not open.\n");
      }
      else
      {
        fclose(fp);
        already_open = 0;
      }
    }
    //Implementation of the command info which prints the necessary 
    //information of the filesystem that the user might need for further calculations 
    //of the filesytem cluster or addresess or various directories
    else if(strcmp(token[0],"info")==0)
    {
      if(already_open == 1)
      {
        printf("BPB_BytesPerSec = Dec:%d\tHex:%x\n",BPB_BytesPerSec, BPB_BytesPerSec);
	      printf("BPB_SecPerClus =  Dec:%d\t\tHex:%x\n",BPB_SecPerClus, BPB_SecPerClus);
    	  printf("BPB_RsvdSecCnt =  Dec:%d\tHex:%x\n",BPB_RsvdSecCnt, BPB_RsvdSecCnt);
    	  printf("BPB_NumFATs =     Dec:%d\t\tHex:%x\n",BPB_NumFATs, BPB_NumFATs);
	      printf("BPB_FATSz32 =     Dec:%d\tHex:%x\n", BPB_FATSz32, BPB_FATSz32);
      }
      else if(already_open == 0)
      {
        printf("Error: File system image must be opened first.\n");
      }
    }
    //Implementation of Command to list all the files in the current working directory
    //This include displaying all the files and directories
    else if(strcmp(token[0],"ls")==0)
    {
      if(already_open == 1)
      { 
        fseek(fp, offset, SEEK_SET);
        fread(&dir[0], 16, sizeof(struct DirectoryEntry), fp);

        int i;
        for(i=0; i<16; i++)
        { //Excluding system volume names and deleted files
          int x = 0xe5;
          if(dir[i].Dir_Attr == 0x01 || dir[i].Dir_Attr == 0x10 || dir[i].Dir_Attr == 0x20 && dir[i].DIR_Name[0] != x)
          {
            printf("%.11s\n", dir[i].DIR_Name);
          }
        }
      }
      else if(already_open == 0)
      {
        printf("Error: File system image must be opened first.\n");
      }
    }
    //Implementation of the command cd which is used to move through the 
    //directories.
    else if(strcmp(token[0],"cd")==0)
    {
      if(already_open == 1)
      {
        int pos;
        int j;
        int prev_selected = 0;
        for(j=0; j<16; j++)
        {
          if(!strncmp( token[1], "..", 2 ) == 0 )
          {
            char IMG_Name[11];
            strncpy( IMG_Name, dir[j].DIR_Name, strlen( dir[j].DIR_Name ) );
            char expanded_name[12];
            memset( expanded_name, ' ', 12 );

            char *stoken = strtok( token[1], "." );

            strncpy( expanded_name, stoken, strlen( stoken ) );

            stoken = strtok( NULL, "." );

            if( stoken )
            {
              strncpy( (char*)(expanded_name+8), stoken, strlen(stoken ) );
            }

            expanded_name[11] = '\0';

            int i;
            for( i = 0; i < 11; i++ )
            {
              expanded_name[i] = toupper( expanded_name[i] );
            }

            if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
            {
              pos = dir[j].DIR_FirstClusterLow;
            }
          }
          else
          {
            prev_selected = 1;
          }

        }
        if(prev_selected == 0)
        { //Taking the file pointer to the position where the directory file is
          prev[0] = offset;
          offset = ((pos-2) * BPB_BytesPerSec) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
          prev[1] = offset;
        }
        else
        {
          offset = prev[0];
        }
        fseek(fp, offset, SEEK_SET);
        fread(&dir[0], 16, sizeof(struct DirectoryEntry), fp);
      }
      else if(already_open == 0)
      {
        printf("Error: File system image must be opened first.\n");
      }
    }
    //Implementation of the command read which takes the filename and positional parameters
    //from the user and then prints out the number of bytes specified
    else if(strcmp(token[0],"read")==0)
    {
      if(already_open == 1)
      {
        int start_pos = 0;
        int pos;
        int j;
        for(j=0; j<16; j++)
        {
          char IMG_Name[11];
          strncpy( IMG_Name, dir[j].DIR_Name, strlen( dir[j].DIR_Name ) );
          char expanded_name[12];
          memset( expanded_name, ' ', 12 );

          char *stoken = strtok( token[1], "." );

          strncpy( expanded_name, stoken, strlen( stoken ) );

          stoken = strtok( NULL, "." );

          if( stoken )
          {
            strncpy( (char*)(expanded_name+8), stoken, strlen(stoken ) );
          }

          expanded_name[11] = '\0';

          int i;
          for( i = 0; i < 11; i++ )
          {
            expanded_name[i] = toupper( expanded_name[i] );
          }

          if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
          {
            pos = dir[j].DIR_FirstClusterLow;
          }
        }
        int value = 0;
        start_pos = atoi(token[2]);
        //Calculating the offset using the positional parameters
        offset = (pos-2) * BPB_BytesPerSec + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec) + start_pos;
        fseek(fp, offset, SEEK_SET);
        int k; //Bytes in the given file
        for(k=0; k<atoi(token[3]); k++)
        {
          fseek(fp, offset+k, SEEK_SET);
          fread(&value, 1, 1, fp);
          printf("%x ", value);
        }
        printf("\n");
      }
      else if(already_open == 0)
      {
        printf("Error: File system image must be opened first.\n");
      }
    }
     //Implementation of the command stat which takes the filename as imput
    //and then prints out Attributes, size and starting cluster numbers for the given file
    else if(strcmp(token[0],"stat")==0)
    {
      if(already_open == 1)
      {
        int pos;
        int file_id;
        int j;
        for(j=0; j<16; j++)
        {
          char IMG_Name[11];
          strncpy( IMG_Name, dir[j].DIR_Name, strlen( dir[j].DIR_Name ) );
          char expanded_name[12];
          memset( expanded_name, ' ', 12 );

          char *stoken = strtok( token[1], "." );

          strncpy( expanded_name, stoken, strlen( stoken ) );

          stoken = strtok( NULL, "." );

          if( stoken )
          {
            strncpy( (char*)(expanded_name+8), stoken, strlen(stoken ) );
          }

          expanded_name[11] = '\0';

          int i;
          for( i = 0; i < 11; i++ )
          {
            expanded_name[i] = toupper( expanded_name[i] );
          }

          if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
          {
            pos = dir[j].DIR_FirstClusterLow;
            file_id = j;
          }
        }
        printf("Name: %.11s\n", dir[file_id].DIR_Name);
        printf("Attribute Byte: %d\n", dir[file_id].Dir_Attr);
        printf("Starting Cluster Number: %d\n", dir[file_id].DIR_FirstClusterLow);
        printf("Size: %d\n", dir[file_id].DIR_FileSize);
      }
      else if(already_open == 0)
      {
        printf("Error: File system image must be opened first.\n");
      }
    }
    //Implementation of the command get to fetch the file
    // and put it in the current working directory
    else if(strcmp(token[0],"get")==0)
    {
      if(already_open == 1)
      {
        int pos;
        int j;
        for(j=0; j<16; j++)
        {
          char IMG_Name[11];
          strncpy( IMG_Name, dir[j].DIR_Name, strlen( dir[j].DIR_Name ) );
          char expanded_name[12];
          memset( expanded_name, ' ', 12 );

          char *stoken = strtok( token[1], "." );

          strncpy( expanded_name, stoken, strlen( stoken ) );

          stoken = strtok( NULL, "." );

          if( stoken )
          {
            strncpy( (char*)(expanded_name+8), stoken, strlen(stoken ) );
          }

          expanded_name[11] = '\0';

          int i;
          for( i = 0; i < 11; i++ )
          {
            expanded_name[i] = toupper( expanded_name[i] );
          }

          if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
          {
            pos = dir[j].DIR_FirstClusterLow;
          }
        }
        offset = (pos-2) * BPB_BytesPerSec + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
        fseek(fp, offset, SEEK_SET);
        fread(&dir[0], 16, sizeof(struct DirectoryEntry), fp);
      }
      else if(already_open == 0)
      {
        printf("Error: File system image must be opened first.\n");
      }
    }
    //TO make sure only these commands are accepted as they are the only implemented
    else if(!strcmp(token[0],"get")==0 || !strcmp(token[0],"stat")==0
     || !strcmp(token[0],"read")==0 || !strcmp(token[0],"cd")==0
      || !strcmp(token[0],"ls")==0 || !strcmp(token[0],"open")==0
       || !strcmp(token[0],"close")==0 || strcmp(token[0],"info")==0)
    {
      printf("Error: Command Not Found.\n");
    }

    free( working_root );

  }
  return 0;
}
