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
#define MSQKEY 0xbeef
#define MSQS 5		// count of message queues
#define MSQS_MAX 100
#define MSQS_MIN 1
#define WORKERS 1000	// count of msg senders and receivers together
#define WORKERS_MAX 10000
#define WORKERS_MIN 10	// to avoid livelock (all workers of the same type)
#define TIMEOUT 10	// runtime of the test in secs
#define TIMEOUT_MAX 300
#define TIMEOUT_MIN 2

#define MSGGETTYPE()	((rand() % 5) + 1)

extern int errno;
int silent;
int verbose;

struct msg {
  long mtype;
  char mtext[MSGTXTLEN];
};

void wait_sigusr1()
{
  sigset_t set;
  int sig;

  // SIGUSR1 is blocked (inherited) from the parent!
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  if (sigwait(&set, &sig)) {
    if (!silent)
      fprintf(stderr, "Error: cannot set a SIGUSR1 handler\n");
    exit(-1);
  }
}

// get message queue limits and parameters and system resources consumed by message queues
int ipc_info_msq(void)
{
  /* Information  about system-wide message queue limits and parameters. */
  struct msginfo msginfo;
  /*
   * The index of the highest used entry in the kernel's internal array recording
   * information about all message queues.
   */
  int lue;

  lue = msgctl(0, IPC_INFO, (struct msqid_ds *)&msginfo);
  if (lue < 0)
    perror("msgctl(IPC_INFO)");

  lue = msgctl(0, MSG_INFO, (struct msqid_ds *)&msginfo);
  if (lue < 0)
    perror("msgctl(MSG_INFO)");

  return lue;
}

// create or open a message queue
int open_msq(unsigned int key)
{
  int msq;
  char str[2048];

  usleep((rand() % 10 + 1) * 100);
  msq = msgget(key, IPC_CREAT | 0666);
  if (msq < 0) {
    sprintf(str, "%06d: failed to create message queue", getpid());
    if (!silent)
      perror(str);
    return -1;
  }
  if (verbose) {
    sprintf(str, "message queue 0x%08x created\n", msq);
    fprintf(stderr, str);
  }
  return msq;
}

// remove a message queue
void close_msq(int msq)
{
  char str[2048];

  usleep((rand() % 10 + 1) * 100);
  msgctl(msq, IPC_RMID, NULL);
  if (verbose) {
    sprintf(str, "%06d: message queue 0x%08X is gone\n", getpid(), msq);
    fprintf(stderr, str);
  }
}

// send messages to the queue
void snd_msg(int msq)
{
  char str[2048];
  struct msg msg;
  int msgflg;

  // wait for a signal to start
  wait_sigusr1();

  msg.mtext[1] = 0;
  while (1) {
    usleep((rand() % 10 + 1) * 100);

    msg.mtype = MSGGETTYPE();
    msg.mtext[0] = '0' + (char) msg.mtype;
    msgflg = rand() % 2 ? 0 : IPC_NOWAIT;
    // the last param can be: 0, IPC_NOWAIT, MSG_NOERROR, or IPC_NOWAIT|MSG_NOERROR.
    if (msgsnd(msq, (void *) &msg, 2, msgflg) < 0) {
      // terminate on all errors except EACCESS and EAGAIN
      if (errno != EACCES && errno != EAGAIN)
        break;
    }
    if (verbose) {
      sprintf(str, "%06d: send msg '%s'\n", getpid(), msg.mtext);
      fprintf(stderr, str);
    }
  }
}

// read from the queue
void rcv_msg(int msq)
{
  long msgtyp;
  int msgflg;
  struct msg msg;
  char str[2048];

  // wait for a signal to start
  wait_sigusr1();

  while (1) {
    usleep((rand() % 10 + 1) * 100);

    // set random parameters of `msgtyp` and `msgflg`
    msgflg = rand() % 2 ? 0 : IPC_NOWAIT;
    msgflg |= MSG_NOERROR;
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

    if (msgrcv(msq, (void *) &msg, 2, msgtyp, msgflg) < 0) {
      // terminate on all errors except EACCESS and EAGAIN
      if (errno != EACCES && errno != EAGAIN) {
        if (verbose) {
          sprintf(str, "%06d: received msg of msgtype %ld failed: %s\n", getpid(), msgtyp, strerror(errno));
          fprintf(stderr, str);
	}
        break;
      }
    }
    if (verbose) {
      sprintf(str, "%06d: received msg '%s', msgtype %ld\n", getpid(), msg.mtext, msgtyp);
      fprintf(stderr, str);
    }
  }
}

void help(char *progname)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s [--help | -h] [--silent | -s] [--verbose | -v]\n", progname);
  fprintf(stderr, "\t\t[--workers=N] [--timeout=T] [--queues=Q]\n\n");
  fprintf(stderr, "Test message queue IPC subsystem.\n\n");
  fprintf(stderr, "Program tries to create N workers (through fork() system call);\n");
  fprintf(stderr, "each worker (randomly choosen) is either msg sender or receiver.\n");
  fprintf(stderr, "For purpose of testing there are Q message queues used and the\n");
  fprintf(stderr, "test takes T seconds. Each worker is connected with one message\n");
  fprintf(stderr, "queue randomly choosen at the moment of creation of the worker.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Options\n");
  fprintf(stderr, "  --help\tThis message ;)\n");
  fprintf(stderr, "  --silent\tSuppress all messages (including error messages).\n");
  fprintf(stderr, "  --verbose\tPrint all messages about operations.\n");
  fprintf(stderr, "  --workers=N\tThe count of workers. The default number of wor-\n");
  fprintf(stderr, "\t\tkers is %d.\n", WORKERS);
  fprintf(stderr, "\t\tN should be an integer in <%d; %d>.\n", WORKERS_MIN, WORKERS_MAX);
  fprintf(stderr, "  --timeout=T\tThe duration of the test. The default timeout is\n");
  fprintf(stderr, "\t\tset to %d secs. The timeout starts *after* crea-\n", TIMEOUT);
  fprintf(stderr, "\t\ttion of all workers. So the duration of the test\n");
  fprintf(stderr, "\t\tconsists of two parts: the time of workers cre-\n");
  fprintf(stderr, "\t\tation and the execution of them.\n");
  fprintf(stderr, "\t\tT should be an integer in <%d; %d>.\n", TIMEOUT_MIN, TIMEOUT_MAX);
  fprintf(stderr, "  --queues=Q\tThe count of message queues. The default value is\n");
  fprintf(stderr, "\t\tset to %d.\n", MSQS);
  fprintf(stderr, "\t\tQ should be an integer in <%d; %d> and 10 * Q <= N.\n", MSQS_MIN, MSQS_MAX);
  fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
  int workers = WORKERS, timeout = TIMEOUT, msqs = MSQS;
  int i, c, worker_type;
  int op[2]; // op[0] -- operation snd, op[1] -- operation rcv
  pid_t pids[WORKERS_MAX], pid;
  int queues[MSQS_MAX];
  char buf[1024];
  sigset_t set;
  void (*operation)(int);
  static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"silent", no_argument, 0, 's'},
    {"verbose", no_argument, 0, 'v'},
    {"workers", required_argument, 0, 'w'},
    {"timeout", required_argument, 0, 't'},
    {"queues", required_argument, 0, 'q'},
    {0, 0, 0, 0},
  };

  silent = 0;
  while ((c = getopt_long(argc, argv, ":hsv", long_options, 0)) != -1) {
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
    case 'q':
      memset(buf, '\0', sizeof(buf));
      if (sscanf(optarg, "%d%s", &msqs, buf) != 1 || strlen(buf) || msqs < MSQS_MIN || msqs > MSQS_MAX) {
        fprintf(stderr, "Error: invalid argument of the option '--queues'.\n");
        help(argv[0]);
        return -1;
      }
      break;
    case 's':
      silent = 1;
      break;
    case 'v':
      verbose = 1;
      fprintf(stderr, "Verbose mode turned on.\n");
      break;
    }
  }

  // sanity checks
  if (silent && verbose) {
    fprintf(stderr, "Error: cannot set both options '--silent' and '--verbose' together!\n");
    help(argv[0]);
    return -1;
  }
  if (10 * msqs > workers) {
    fprintf(stderr, "Error: invalid number of message queues or workers, 10 * Q is greather than N!\n");
    help(argv[0]);
    return -1;
  }

  // probe msgctl(IPC_INFO) syscall
  if (ipc_info_msq() < 0)
    return -1;

  // prepare process signal mask to handle SIGUSR1
  // all children will inherite blocked SIGUSR1
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  if (sigprocmask(SIG_BLOCK, &set, 0) < 0) {
    if (!silent)
      perror("sigprocmask block SIGUSR1 failed");
    return -1;
  }

  // initialise rnd generator, variables and message queue(s)
  srand(time(NULL));
  memset(pids, '\0', sizeof(pids));
  memset(queues, (char) -1, sizeof(queues));
  for (i = 0; i < msqs; i++) {
    if ((queues[i] = open_msq(MSQKEY + i)) < 0) {
      while (i > 0) {
        close_msq(queues[i-1]);
	i--;
      }
      return -1;
    }
  }

  // create WORKERS tasks
  if (!silent)
    fprintf(stderr, "wait a moment... I'm trying to randomly generate %d message senders and receivers for %d different message queues... ", workers, msqs);
  op[0] = op[1] = 0;
  for (i = 0; i < workers; i++) {
    worker_type = rand() % 2;
    op[worker_type]++;

    pids[i] = fork();

    // child
    if (pids[i] == 0) {
      // random choice rcv_msg/snd_msg
      operation = worker_type ? rcv_msg : snd_msg;
      operation(queues[rand() % msqs]);
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
  // sanity check again for the number of message queues against successfully created workers
  if (10 * msqs > workers) {
    fprintf(stderr, "\nError: invalid number of queues (%d) or created workers (%d), 10 * Q is greather than N!\n",
            msqs, workers);
    help(argv[0]);
    kill(-pids[0], SIGKILL);
    return -1;
  }
  if (!silent)
    fprintf(stderr, "done\ntest continues for another %d secs with %d children alive: %d senders, %d receivers\n",
            timeout, workers, op[0], op[1]);

  // let workers to do something interesting... start all workers by sending a SIGUSR1 signal
  if (kill(-pids[0], SIGUSR1) < 0) {
    if (!silent)
      perror("deliver SIGUSR1 to workers failed");
    return -1;
  }
  sleep(timeout);
  // destroy message queue(s) to finish workers
  for (i = 0; i < msqs; i++)
    close_msq(queues[i]);
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
    if (!silent && i == WORKERS_MAX)
      fprintf(stderr, "error: pid %d is not mY child!\n", pid);
  }
  if (!silent && workers)
   fprintf(stderr, "not finished %d children yet, I'll kill them ;)\n", workers);

  // kill not finished workers yet
  for (i = 0; i < WORKERS_MAX; i++) {
    if (pids[i] > 0) {
      kill(pids[i], SIGKILL);
      if (!silent && waitpid(pids[i], NULL, 0) != pids[i])
        fprintf(stderr, "error: waiting to child %d kill failed\n", pids[i]);
    }
  }

  return 0;
}
