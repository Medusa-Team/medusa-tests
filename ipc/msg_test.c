#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/errno.h>

extern int errno;       // error NO.
#define MSGTXTLEN 1024  // msg text length
#define MSGKEY	0x1234	// can be also IPC_PRIVATE
#define MSGSNDCNT 10	// count of message snd attempts
#define MSGRCVCNT 10	// count of message rcv attempts

#define MSGGETTYPE()	((rand() % 3) + 1)

struct msg_buf {
  long mtype;
  char mtext[MSGTXTLEN];
} msg;

int main(int argc,char **argv)
{
  int ret = 0;
  int msgqid, rc;
  char errbuf[1024];
  long msgtyp;

  // initialise rnd generator
  srand(time(NULL));

  // create a message queue
  usleep((rand() % 10 + 1) * 100);
  msgqid = msgget(MSGKEY, IPC_CREAT | 0666);
  if (msgqid < 0) {
    sprintf(errbuf, "%06d: failed to create message queue", getpid());
    perror(errbuf);
    //return 1;
  }
  printf("message queue 0x%08x created\n",msgqid);

  // send messages to the queue
  msg.mtext[1] = 0;
  for (int i = 0; i < MSGSNDCNT; i++) {
    usleep((rand() % 10 + 1) * 100);
    msg.mtype = MSGGETTYPE();
    msg.mtext[0] = '0' + (char) msg.mtype;
    // the last param can be: 0, IPC_NOWAIT, MSG_NOERROR, or IPC_NOWAIT|MSG_NOERROR.
    rc = msgsnd(msgqid, (void *) &msg, 2, IPC_NOWAIT);
    if (rc < 0) {
      //perror("msgsnd failed");
      //ret = 1;
      //goto err;
    }
    printf("%06d: send msg: %s\n", getpid(), msg.mtext);
  }

  // read from the queue
  for (int i = 0; i < MSGRCVCNT; i++) {
    usleep((rand() % 10 + 1) * 100);
    msgtyp = MSGGETTYPE();
    rc = msgrcv(msgqid, (void *) &msg, 2, msgtyp, MSG_NOERROR | IPC_NOWAIT);
    if (rc < 0) {
      //perror("msgrcv failed");
      //ret = 1;
      //goto err;
    }
    printf("%06d: received msg: %s\n", getpid(), msg.mtext);
  }

//err:
  // remove the queue
  usleep((rand() % 10 + 1) * 100);
  rc = msgctl(msgqid, IPC_RMID, NULL);
  if (rc < 0) {
    //perror("msgctl(IPC_RMID) failed");
    ret = 1;
  }
  printf("%06d: message queue 0x%08X is gone\n", getpid(), msgqid);

  return ret;
}
