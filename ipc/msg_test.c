#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/types.h>

extern int errno;       // error NO.
#define MSGTXTLEN 1024  // msg text length
#define MSGKEY	0x1234	// can be also IPC_PRIVATE
#define TASKCNT 10000	// count of tasks
#define WORKINGTIME 30	// time in secs to work

#define MSGGETTYPE()	((rand() % 5) + 1)

struct msg {
  long mtype;
  char mtext[MSGTXTLEN];
};

// create or open a message queue
int open_msq(unsigned int key, int print)
{
  int msq;
  char str[2048];

  usleep((rand() % 10 + 1) * 100);
  msq = msgget(key, IPC_CREAT | 0666);
  if (msq < 0) {
    sprintf(str, "%06d: failed to create message queue", getpid());
    perror(str);
    return -1;
  }
  if (print) {
    sprintf(str, "message queue 0x%08x created\n", msq);
    fprintf(stderr, str);
  }
  return msq;
}

// remove a message queue
void close_msq(int msq, int print)
{
  char str[2048];

  usleep((rand() % 10 + 1) * 100);
  msgctl(msq, IPC_RMID, NULL);
  if (print) {
    sprintf(str, "%06d: message queue 0x%08X is gone\n", getpid(), msq);
    fprintf(stderr, str);
  }
}

// send messages to the queue
void snd_msg(int msq, int print)
{
  char str[2048];
  struct msg msg;

  msg.mtext[1] = 0;
  while (1) {
    usleep((rand() % 10 + 1) * 100);

    msg.mtype = MSGGETTYPE();
    msg.mtext[0] = '0' + (char) msg.mtype;
    // the last param can be: 0, IPC_NOWAIT, MSG_NOERROR, or IPC_NOWAIT|MSG_NOERROR.
    //msgsnd(msq, (void *) &msg, 2, IPC_NOWAIT);
    if (msgsnd(msq, (void *) &msg, 2, 0) < 0) {
      // terminate on all errors except EACCESS and EAGAIN
      if (errno != EACCES && errno != EAGAIN)
        break;
    }
    if (print) {
      sprintf(str, "%06d: send msg: %s\n", getpid(), msg.mtext);
      fprintf(stderr, str);
    }
  }
}

// read from the queue
void rcv_msg(int msq, int print)
{
  long msgtyp;
  int msgflg;
  struct msg msg;
  char str[2048];

  while (1) {
    usleep((rand() % 10 + 1) * 100);

    // set random parameters of `msgtyp` and `msgflg`
    //msgflg = MSG_NOERROR | IPC_NOWAIT;
    msgflg = MSG_NOERROR;
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
      if (msgtyp > 0 && rand() % 2)
	// used with msgtyp greater than 0 to read the first message in the queue
	// with message type that differs from `msgtyp`
	msgflg |= MSG_EXCEPT;
    }

    if (print) {
      sprintf(str, "msgtyp = %ld\n", msgtyp);
      fprintf(stderr, str);
    }
    if (msgrcv(msq, (void *) &msg, 2, msgtyp, msgflg) < 0) {
      // terminate on all errors except EACCESS and EAGAIN
      if (errno != EACCES && errno != EAGAIN)
        break;
    }
    if (print) {
      sprintf(str, "%06d: received msg: %s\n", getpid(), msg.mtext);
      fprintf(stderr, str);
    }
  }
}

int main()
{
  int msq, i, not_finished, choice;
  int op[2]; // op[0] -- operation snd, op[1] -- operation rcv
  pid_t pids[TASKCNT], pid;
  void (*operation)(int, int);

  // initialise rnd generator, variables and message queue
  srand(time(NULL));
  memset(pids, '\0', sizeof(pids));
  if ((msq = open_msq(MSGKEY, 0)) < 0)
    return -1;

  // create TASKCNT tasks
  fprintf(stderr, "wait a moment, generating message senders and receivers... ");
  op[0] = op[1] = 0;
  not_finished = TASKCNT;
  for (i = 0; i < TASKCNT; i++) {
    choice = rand() % 2;
    op[choice]++;

    pids[i] = fork();

    // child
    if (pids[i] == 0) {
      // random choice rcv_msg/snd_msg
      operation = choice ? rcv_msg : snd_msg;
      operation(msq, 0);
      return 0;
    }
   
    // if we cannot produce another child, continue with those created yet
    if (pids[i] == -1) {
      not_finished = i;
      pids[i] = 0;
      i--;
      op[choice]--;
      break;
    }
  }
  fprintf(stderr, "done\ncontinue testing (%d sec) with %d childs alive: %d senders, %d receivers\n", WORKINGTIME, i+1, op[0], op[1]);

  // let workers to do something interesting...
  sleep(WORKINGTIME);
  // destroy message queue to finish workers
  close_msq(msq, 0);
  sleep(1);

  // clean up
  while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
    for (i = 0; i < TASKCNT; i++) {
      if (pids[i] == pid) {
        pids[i] = 0;
	not_finished--;
	break;
      }
    }
    if (i == TASKCNT)
      fprintf(stderr, "error: pid %d is not mY child!\n", pid);
  }
  if (not_finished)
   fprintf(stderr, "not finished %d from %d childs yet, I'll kill them ;)\n", not_finished, TASKCNT);

  // kill not finished workers yet
  for (i = 0; i < TASKCNT; i++) {
    if (pids[i] > 0) {
      kill(pids[i], SIGKILL);
      if (waitpid(pids[i], NULL, 0) != pids[i])
        fprintf(stderr, "error: waiting to child %d kill failed\n", pids[i]);
    }
  }

  return 0;
}
