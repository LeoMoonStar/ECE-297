#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>

/* Must include this to have prototype for the thread functions */ 
#include <pthread.h>

/************************************************************
 * Utility Code 
 ************************************************************/ 

int sendall(const int sock, const char *buf, const size_t len)
{
  size_t tosend = len;
  while (tosend > 0) {
    size_t bytes = send(sock, buf, tosend, 0);
    if (bytes <= 0) 
      break; // send() was not successful, so stop.
    tosend -= bytes;
    buf += bytes;
  };

  return tosend == 0 ? 0 : -1;
}

int recvline(const int sock, char *buf, const size_t buflen)
{
  int status = 0; // Return status.
  size_t bufleft = buflen;

  while (bufleft > 0) {
    // Read one byte from scoket.
    size_t bytes = recv(sock, buf, 1, 0);
    if (bytes <= 0) {
      // recv() was not successful, so stop.
      status = -1;
      break;
    } else if (*buf == '\n') {
      // Found end of line, so stop.
      *buf = 0; // Replace end of line with a null terminator.
      status = 0;
      break;
    } else {
      // Keep going.
      bufleft -= 1;
      buf += 1;
    }
  }

  return status;
}

/************************************************************
 * Thread Pool Definitions, Data Structures, Variables
 ************************************************************/ 

#define MAX_THREADS 10 

struct _ThreadInfo { 
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen; 
  int clientsock; 
  pthread_t theThread; 
}; 
typedef struct _ThreadInfo *ThreadInfo; 

/*  Thread buffer, and circular buffer fields */ 
ThreadInfo runtimeThreads[MAX_THREADS]; 
unsigned int botRT = 0, topRT = 0;

/* Mutex to guard print statements */ 
pthread_mutex_t  printMutex    = PTHREAD_MUTEX_INITIALIZER; 

/* Mutex to guard condition -- used in getting/returning from thread pool*/ 
pthread_mutex_t conditionMutex = PTHREAD_MUTEX_INITIALIZER;
/* Condition variable -- used to wait for avaiable threads, and signal when available */ 
pthread_cond_t  conditionCond  = PTHREAD_COND_INITIALIZER;


ThreadInfo getThreadInfo(void) { 
  ThreadInfo currThreadInfo = NULL;

  /* Wait as long as there are no more threads in the buffer */ 
  pthread_mutex_lock( &conditionMutex ); 
  while (((botRT+1)%MAX_THREADS)==topRT)
    pthread_cond_wait(&conditionCond,&conditionMutex); 
  
  /* At this point, there is at least one thread available for us. We
     take it, and update the circular buffer  */ 
  currThreadInfo = runtimeThreads[botRT]; 
  botRT = (botRT + 1)%MAX_THREADS;

  /* Release the mutex, so other clients can add threads back */ 
  pthread_mutex_unlock( &conditionMutex ); 
  
  return currThreadInfo;
}

/* Function called when thread is about to finish -- unless it is
   called, the ThreadInfo assigned to it is lost */ 
void releaseThread(ThreadInfo me) {
  pthread_mutex_lock( &conditionMutex ); 
  assert( botRT!=topRT ); 
 
  runtimeThreads[topRT] = me; 
  topRT = (topRT + 1)%MAX_THREADS; 

  /* tell getThreadInfo a new thread is available */ 
  pthread_cond_signal( &conditionCond ); 

  /* Release the mutex, so other clients can get new threads */ 
  pthread_mutex_unlock( &conditionMutex ); 
}

/* Prototype for the thread callback function : 
void * funcion_name( void *argument ) */ 

/* This function receives a string from clients and echoes it back --
   the thread is released when the thread is finished */
void * threadCallFunction(void *arg) { 
  ThreadInfo tiInfo = (ThreadInfo)arg; 
  char buffer[50]; 
  int  length; 
  recvline( tiInfo->clientsock, &buffer[0], 48 ); 
  length = strlen(buffer);
  buffer[length] = '\n'; 
  buffer[length+1] = 0; 
  sendall( tiInfo->clientsock, &buffer[0], length+1 ); 
  
  if (close(tiInfo->clientsock)<0) { 
    pthread_mutex_lock( &printMutex ); 
    printf("ERROR in closing socket to %s:%d.\n", 
	   inet_ntoa(tiInfo->clientaddr.sin_addr), tiInfo->clientaddr.sin_port);
    pthread_mutex_unlock( &printMutex ); 
  }
  releaseThread( tiInfo ); 
  return NULL; 
}


  
int main( int argc, char *argv[] ) { 
  int port, listensocket, status; 
  unsigned int i; 
  struct sockaddr_in listenaddr;

  if (argc!=2) { 
    printf("Error, %s must be provided a port number"); 
    return -1; 
  }

  port = atoi( argv[1] ); 
  listensocket = socket (AF_INET, SOCK_STREAM, 0); 
  
  memset(&listenaddr, 0, sizeof listenaddr);
  listenaddr.sin_family = AF_INET;
  listenaddr.sin_port = htons(port);
  inet_pton(AF_INET, "localhost", &(listenaddr.sin_addr)); 
  status = bind(listensocket, (struct sockaddr*) &listenaddr, sizeof listenaddr);
  if (status != 0) {
    printf("Error binding socket.\n");
    return -1; 
  }

  status = listen(listensocket, 15); 
  if (status != 0) { 
    printf("Error listening.\n"); 
    return -1; 
  }

  /* First, we allocate the thread pool */ 
  for (i = 0; i!=MAX_THREADS; ++i)
    runtimeThreads[i] = malloc( sizeof( struct _ThreadInfo ) ); 

  while (1) { 
    ThreadInfo tiInfo = getThreadInfo(); 
    tiInfo->clientaddrlen = sizeof(struct sockaddr_in); 
    tiInfo->clientsock = accept(listensocket, (struct sockaddr*)&tiInfo->clientaddr, &tiInfo->clientaddrlen);
    if (tiInfo->clientsock <0) {
      pthread_mutex_lock( &printMutex ); 
      printf("ERROR in connecting to %s:%d.\n", 
	     inet_ntoa(tiInfo->clientaddr.sin_addr), tiInfo->clientaddr.sin_port);
      pthread_mutex_unlock( &printMutex ); 
    } else {
      pthread_create( &tiInfo->theThread, NULL, threadCallFunction, tiInfo ); 
    }
  }

  /* At the end, wait until all connections close */ 
  for (i=topRT; i!=botRT; i = (i+1)%MAX_THREADS)
    pthread_join(runtimeThreads[i]->theThread, 0 ); 

  /* Deallocate all the resources */ 
  for (i=0; i!=MAX_THREADS; i++)
    free( runtimeThreads[i] ); 

  return 0; 
}
