#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <wait.h>
#include <getopt.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/types.h>

#define MSGTXTLEN 1024  // msg text length
#define MSGKEY	0x1234	// can be also IPC_PRIVATE
#define WORKERS 1000	// count of msg senders and receivers together 
#define WORKERS_MAX 10000
#define WORKERS_MIN 1
#define TIMEOUT 10	// runtime of the test in secs
#define TIMEOUT_MAX 300
#define TIMEOUT_MIN 2

#define MSGGETTYPE()	((rand() % 5) + 1)

extern int errno;

struct msg {
  long mtype;
  char mtext[MSGTXTLEN];
};

void wait_signal(int sig)
{
  sigset_t set;

  sigemptyset(&set);
  sigaddset(&set, sig);
  if (!sigwait(&set, &sig)) {
    fprintf(stderr, "Error: cannot set a SIGUSR1 handler\n");
    exit(-1);
  }
}

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

  // wait for a signal to start
  wait_signal(SIGUSR1);

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

  // wait for a signal to start
  wait_signal(SIGUSR1);

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

void help(char *progname)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s [--help | -h] [--workers=N] [--timeout=T]\n\n", progname);
  fprintf(stderr, "Test message queue IPC subsystem.\n\n");
  fprintf(stderr, "Program tries to create N workers (through fork() system call);\n");
  fprintf(stderr, "each worker (randomly choosen) is one of msg sender or receiver.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options\n");
  fprintf(stderr, "  --help\tThis message ;)\n");
  fprintf(stderr, "  --workers=N\tThe count of workers. The default number of wor-\n");
  fprintf(stderr, "\t\tkers is %d.\n", WORKERS);
  fprintf(stderr, "\t\tN should be an integer in <%d; %d>.\n", WORKERS_MIN, WORKERS_MAX);
  fprintf(stderr, "  --timeout=T\tThe duration of the test. The default timeout is\n");
  fprintf(stderr, "\t\tset to %d secs. The timeout starts *after* crea-\n", TIMEOUT);
  fprintf(stderr, "\t\ttion of all workers. So the duration of the test\n");
  fprintf(stderr, "\t\tconsists from two parts: the time of workers cre-\n");
  fprintf(stderr, "\t\tation and the run of them.\n");
  fprintf(stderr, "\t\tT should be an integer in <%d; %d>.\n", TIMEOUT_MIN, TIMEOUT_MAX);
  fprintf(stderr, "\n");
  fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
  int workers = WORKERS, timeout = TIMEOUT;
  int msq, i, c, worker_type;
  int op[2]; // op[0] -- operation snd, op[1] -- operation rcv
  pid_t pids[WORKERS_MAX], pid;
  char buf[1024];
  void (*operation)(int, int);
  static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"workers", required_argument, 0, 'w'},
    {"timeout", required_argument, 0, 't'},
    {0, 0, 0, 0},
  };

  while ((c = getopt_long(argc, argv, ":h", long_options, 0)) != -1) {
    switch (c) {
    case 'h':
      help(argv[0]);
      return 0;
    case ':':
      fprintf(stderr, "Error: missing an argument of an option!\n");
      help(argv[0]);
      return -1;
    case '?':
      fprintf(stderr, "Error: invalid option!\n");
      help(argv[0]);
      return -1;
    default:
      help(argv[0]);
      return -1;

    case 'w':
      memset(buf, '\0', sizeof(buf));
      if (sscanf(optarg, "%d%s", &workers, buf) != 1 || strlen(buf) || workers < WORKERS_MIN || workers > WORKERS_MAX) {
        fprintf(stderr, "Error: invalid argument of the option '--workers'.\n");
        help(argv[0]);
        return -1;
      }
      break;
    case 't':
      memset(buf, '\0', sizeof(buf));
      if (sscanf(optarg, "%d%s", &timeout, buf) != 1 || strlen(buf) || timeout < TIMEOUT_MIN || timeout > TIMEOUT_MAX) {
        fprintf(stderr, "Error: invalid argument of the option '--timeout'.\n");
        help(argv[0]);
        return -1;
      }
      break;
    }
  }

  // initialise rnd generator, variables and message queue
  srand(time(NULL));
  memset(pids, '\0', sizeof(pids));
  if ((msq = open_msq(MSGKEY, 0)) < 0)
    return -1;

  // create WORKERS tasks
  fprintf(stderr, "wait a moment... I'm trying to generate %d message senders and receivers... ", workers);
  op[0] = op[1] = 0;
  for (i = 0; i < workers; i++) {
    worker_type = rand() % 2;
    op[worker_type]++;

    pids[i] = fork();

    // child
    if (pids[i] == 0) {
      // random choice rcv_msg/snd_msg
      operation = worker_type ? rcv_msg : snd_msg;
      operation(msq, 0);
      return 0;
    }
   
    // if we cannot produce another child, continue with those created yet
    if (pids[i] == -1) {
      workers = i;
      pids[i] = 0;
      op[worker_type]--;
      break;
    }

    // put all children in process group of eldest child
    setpgid(pids[i], pids[0]);
  }
  fprintf(stderr, "done\ntest continues for another %d secs with %d children alive: %d senders, %d receivers\n",
          timeout, workers, op[0], op[1]);

  // let workers to do something interesting... start all workers by sending a SIGUSR1 signal
  kill(-pids[0], SIGUSR1);
  sleep(timeout);
  // destroy message queue to finish workers
  close_msq(msq, 0);
  sleep(1);

  // clean up
  while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
    for (i = 0; i < WORKERS_MAX; i++) {
      if (pids[i] == pid) {
        pids[i] = 0;
	workers--;
	break;
      }
    }
    if (i == WORKERS_MAX)
      fprintf(stderr, "error: pid %d is not mY child!\n", pid);
  }
  if (workers)
   fprintf(stderr, "not finished %d from %d children yet, I'll kill them ;)\n", workers, WORKERS);

  // kill not finished workers yet
  for (i = 0; i < WORKERS_MAX; i++) {
    if (pids[i] > 0) {
      kill(pids[i], SIGKILL);
      if (waitpid(pids[i], NULL, 0) != pids[i])
        fprintf(stderr, "error: waiting to child %d kill failed\n", pids[i]);
    }
  }

  return 0;
}
