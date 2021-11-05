#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <strings.h>
#include <time.h>
#include "config.h"
#include "common.h"
#include "license.h"

static int shmid = -1, msgid = -1;
static char license_file[PATH_MAX];

int is_signalled = 0;
struct license_object * license_obj = NULL;
char perror_buf[100];

int create_license(const int num_licenses){

  /* create the license filename, using user ID*/
  snprintf(license_file, PATH_MAX, "/tmp/license.%u", getuid());

  if(num_licenses){
    fprintf(stderr, "P%u| License file is %s\n", getpid(), license_file);

    /* create the license file */
    int fd = creat(license_file, 0700);
    if(fd == -1){
      perror(perror_buf);
      return -1;
    }
    close(fd);
  }

  //create key for shared memory
  key_t license_key = ftok(license_file, 4444);
  if(license_key == -1){
    perror(perror_buf);
    return -1;
  }

  shmid = shmget(license_key, sizeof(struct license_object), (num_licenses) ? IPC_CREAT | IPC_EXCL | S_IRWXU : 0);
  if(shmid == -1){
    perror(perror_buf);
    return -1;
  }

  //create key for message queue
  license_key = ftok(license_file, 5555);
  if(license_key == -1){
    perror(perror_buf);
    return -1;
  }

  //get the message queue
  msgid = msgget(license_key, (num_licenses) ? IPC_CREAT | IPC_EXCL | S_IRWXU : 0);
  if(msgid == -1){
    perror(perror_buf);
    return -1;
  }

  license_obj = (struct license_object*) shmat(shmid, NULL, 0);
  if(license_obj == (void*)-1){
    perror(perror_buf);
    return -1;
  }

  if(num_licenses){

    /* clear the license object */
    bzero(license_obj, sizeof(struct license_object));
    license_obj->nlicenses = num_licenses;

    /* initialize it */
    initlicense();
  }

  return 0;
}

int destroy_license(const int num_licenses){
  int rv = 0;

  if(shmdt(license_obj) == -1){
    perror(perror_buf);
    return -1;
  }

  if(num_licenses > 0){
    fprintf(stderr, "P%u| Destroying license file %s\n", getpid(), license_file);

	  if(shmctl(shmid, IPC_RMID, NULL) == -1){
      perror(perror_buf);
      rv = -1;
    }

    if(msgctl(msgid, IPC_RMID, NULL) == -1){
      perror(perror_buf);
      rv = -1;
    }

    if(unlink(license_file) == -1){
      perror(perror_buf);
      rv = -1;
    }
  }
  return rv;
}

const char * make_msg(const char * msg){
  static char buf[100];
  char stamp[30];

  //create a time stamp
  time_t now = time(NULL);
  struct tm * tm = localtime(&now);
  strftime(stamp, sizeof(stamp), "%D %T", tm);

  //prepare the message
  snprintf(buf, sizeof(buf), "%s\t%u\t%s\n", stamp, getpid(), msg);

  return buf;
}

int msg_send(const struct msgbuf * buf){
  if(msgsnd(msgid, buf, sizeof(buf->mtext), 0) == -1){
    perror(perror_buf);
    return -1;
  }
  return 0;
}

int msg_recv(struct msgbuf * buf){
  if (msgrcv(msgid, buf, sizeof(buf->mtext), buf->mtype, 0) == -1){
    perror(perror_buf);
    return -1;
  }
  return 0;
}
