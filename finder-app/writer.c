#include <stdio.h>
#include <syslog.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
int main( int argc, char *argv[] )  {


     openlog("Logs",  LOG_PID,LOG_USER);
    
     syslog(LOG_INFO, "Start logging");
     if(argv[1] == NULL)
     {
   
     syslog(LOG_ERR, "The first argument is empty.");
      return 1;
     }
     if(argv[2] == NULL)
     {
     
      syslog(LOG_ERR, "The second argument is empty.");
      return 1;
     }
   
  
 char path[200];
  memset(path,'\0',200);
 
 
  for(int i = strlen(argv[1]) - 1; i>0; i--)  
       {
     
            if (argv[1][i] == '/')
            {
                
              
               strncpy(path, argv[1], i);
               syslog(LOG_INFO, "Path : %s",path);
	      struct stat sb;

	    if (stat(path, &sb) == 0) 
		   {
			
			break; 
		    } 
		    else 
		    {
			//printf("NO11\n");
		    
			 syslog(LOG_ERR, "Folder does not exist ");
			 syslog(LOG_INFO, "Create Folder ");
			 mkdir(path, 0777);
			//return 1; 
		    }
		    }
		   
    }
    


    syslog(LOG_DEBUG, "Writing %s to %s",argv[2],argv[1]);
     // open the file for writing
    FILE *fp = fopen(argv[1], "awb+");//add file if there is no create it
    
    if (fp == NULL)
    {
 
        syslog(LOG_ERR, "Error opening the file");
        return 1;
    }
    // write to the text file
   
     fprintf(fp, "%s\n",argv[2]);

    // close the file
    fclose(fp);
    syslog(LOG_DEBUG, "write is finished.");
    
     syslog(LOG_INFO, "No error");
     closelog();
     syslog(LOG_INFO, "End logging");
   return 0;
}
