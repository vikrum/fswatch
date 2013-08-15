#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <CoreServices/CoreServices.h> 

/* fswatch.c
 * 
 * $ clang -framework CoreServices -o fswatch fswatch.c
 *
 */

extern char **environ;
//the command to run
char *to_run;
char **exclude;
int excludeCount;

//fork a process when there's any change in watch file
void callback( 
    ConstFSEventStreamRef streamRef, 
    void *clientCallBackInfo, 
    size_t numEvents, 
    void *eventPaths, 
    const FSEventStreamEventFlags eventFlags[], 
    const FSEventStreamEventId eventIds[]) 
{ 
  pid_t pid;
  int   status;

  for(int i = 0; i < numEvents; i++) {
    const char* path = ((char **)eventPaths)[i];
    fprintf(stderr, "event %x -> %s\n", eventFlags[i], path);
    for(int i = 0; i < excludeCount; i++) {
      if(strstr(path, exclude[i])) {
        fprintf(stderr, "excluding %s %s\n", path, exclude[i]);
	return;
      }
    }
  }

  if((pid = fork()) < 0) {
    fprintf(stderr, "error: couldn't fork \n");
    exit(1);
  } else if (pid == 0) {
    char *args[4] = { "/bin/bash", "-c", to_run, 0 };
    if(execve(args[0], args, environ) < 0) {
      fprintf(stderr, "error: error executing\n");
      exit(1);
    }
  } else {
    while(wait(&status) != pid)
      ;
  }
} 
 
//set up fsevents and callback
int main(int argc, char **argv) {

  if(argc < 3) {
    fprintf(stderr, "usage: %s <path to watch> <command to fork> [n (sub)path to exclude ...] \n", argv[0]);
    exit(1);
  }

  to_run = argv[2];
  exclude = &argv[3];
  excludeCount = argc - 3;

  CFStringRef mypath = CFStringCreateWithCString(NULL, argv[1], kCFStringEncodingUTF8); 
  CFArrayRef pathsToWatch = CFStringCreateArrayBySeparatingStrings (NULL, mypath, CFSTR(":"));

  void *callbackInfo = NULL; 
  FSEventStreamRef stream; 
  CFAbsoluteTime latency = 1.0;

  stream = FSEventStreamCreate(NULL,
    &callback,
    callbackInfo,
    pathsToWatch,
    kFSEventStreamEventIdSinceNow,
    latency,
    kFSEventStreamCreateFlagNone
  ); 

  FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode); 
  FSEventStreamStart(stream);
  CFRunLoopRun();

}
