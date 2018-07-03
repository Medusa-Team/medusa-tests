#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/errno.h>
     
extern int errno;       // error NO.
#define MSGTXTLEN 128   // msg text length
#define MSGKEY	0x1234	// can be also IPC_PRIVATE
#define MSGTYPE	1	// type of message

struct msg_buf {
  long mtype;
  char mtext[MSGTXTLEN];
} msg;

int main(int argc,char **argv)
{
  int ret = 0;
  int msgqid, rc;

  // create a message queue
  msgqid = msgget(MSGKEY, IPC_CREAT | 0666);
  if (msgqid < 0) {
    perror("failed to create message queue");
    return 1;
  }
  printf("message queue 0x%08x created\n",msgqid);
  
  // message to send
  msg.mtype = MSGTYPE;
  sprintf (msg.mtext, "a"); 

  // send the message to queue
  // the last param can be: 0, IPC_NOWAIT, MSG_NOERROR, or IPC_NOWAIT|MSG_NOERROR.
  rc = msgsnd(msgqid, (void *) &msg, 2, IPC_NOWAIT); 
  if (rc < 0) {
    perror("msgsnd failed");
    ret = 1;
    goto err;
  }
  printf("send msg: %s\n", msg.mtext);

  // read the message from queue
  rc = msgrcv(msgqid, (void *) &msg, 2, MSGTYPE, MSG_NOERROR | IPC_NOWAIT); 
  if (rc < 0) {
    perror("msgrcv failed");
    ret = 1;
    goto err;
  } 
  printf("received msg: %s\n", msg.mtext);

err:
  // remove the queue
  rc = msgctl(msgqid, IPC_RMID, NULL);
  if (rc < 0) {
    perror("msgctl(IPC_RMID) failed");
    ret = 1;
  }
  printf("message queue 0x%08X is gone\n", msgqid);

  return ret;
}
