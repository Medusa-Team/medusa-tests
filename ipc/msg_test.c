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

int main()
{
  int msgqid;
  char errbuf[2048];
  long msgtyp;
  int msgflg;

  // initialise rnd generator
  srand(time(NULL));

  // create a message queue
  usleep((rand() % 10 + 1) * 100);
  msgqid = msgget(MSGKEY, IPC_CREAT | 0666);
  if (msgqid < 0) {
    sprintf(errbuf, "%06d: failed to create message queue", getpid());
    perror(errbuf);
  }
  sprintf(errbuf, "message queue 0x%08x created\n", msgqid);
  //printf(errbuf);

  // send messages to the queue
  msg.mtext[1] = 0;
  for (int i = 0; i < MSGSNDCNT; i++) {
    usleep((rand() % 10 + 1) * 100);
    msg.mtype = MSGGETTYPE();
    msg.mtext[0] = '0' + (char) msg.mtype;
    // the last param can be: 0, IPC_NOWAIT, MSG_NOERROR, or IPC_NOWAIT|MSG_NOERROR.
    msgsnd(msgqid, (void *) &msg, 2, IPC_NOWAIT);
    sprintf(errbuf, "%06d: send msg: %s\n", getpid(), msg.mtext);
    //printf(errbuf);
  }

  // read from the queue
  for (int i = 0; i < MSGRCVCNT; i++) {
    usleep((rand() % 10 + 1) * 100);

    // set random parameters of `msgtyp` and `msgflg`
    msgflg = MSG_NOERROR | IPC_NOWAIT;
    // if `msgtyp` is 0, then the first message in the queue is read
    msgtyp = 0;
    if (rand() % 2) {
      // if `msgtyp` is greater than 0, then the first message in the queue of type
      // `msgtyp` will be read
      msgtyp = MSGGETTYPE();
      // if `msgtyp` is less than 0, then the first message in the queue with the
      // lowest type less than or equal to the absolute value of `msgtyp` will be
      // read
      msgtyp *= (rand() % 2) * 2 - 1;
      printf("msgtyp = %ld\n", msgtyp);
      if (msgtyp > 0 && rand() % 2)
	// used with msgtyp greater than 0 to read the first message in the queue
	// with message type that differs from `msgtyp`
	msgflg |= MSG_EXCEPT;
    }

    msgrcv(msgqid, (void *) &msg, 2, msgtyp, msgflg);
    sprintf(errbuf, "%06d: received msg: %s\n", getpid(), msg.mtext);
    //printf(errbuf);
  }

  usleep((rand() % 10 + 1) * 100);
  msgctl(msgqid, IPC_RMID, NULL);
  sprintf(errbuf, "%06d: message queue 0x%08X is gone\n", getpid(), msgqid);
  //printf(errbuf);

  return 0;
}
