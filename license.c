#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "license.h"
#include "config.h"
#include "common.h"

static void crit_lock(const enum msg_types type){
  struct msgbuf buf;

  bzero(&buf, sizeof(buf));

  //wait for a message of our type
  buf.mtype = type;
  if(msg_recv(&buf) == -1){
    return;
  }
}

static void crit_unlock(const enum msg_types type){
  struct msgbuf buf;

  bzero(&buf, sizeof(buf));

  //return message of the type
  buf.mtype = type;
  if(msg_send(&buf) == -1){
    return;
  }
}

int getlicense(void){
  crit_lock(TYPE_LICENSE);

  /* while there are no licenses */
  while(license_obj->nlicenses <= 0){
    /* release critical section */
    crit_unlock(TYPE_LICENSE);

    /* if we are signalled, stop */
    if(is_signalled){
      return -1;
    }

    //sleep(1);
    /* take the crit_lock back */
    crit_lock(TYPE_LICENSE);
  }

  const int l = --license_obj->nlicenses;
  crit_unlock(TYPE_LICENSE);

  fprintf(stderr, "P%d: Using license %d\n", getpid(), l);

  return l;
}

int returnlicense(void){
  crit_lock(TYPE_LICENSE);
  license_obj->nlicenses++;
  crit_unlock(TYPE_LICENSE);

  fprintf(stderr, "P%d: license returned\n", getpid());

  return 0;
}


int initlicense(void){
  struct msgbuf buf;

  bzero(&buf, sizeof(buf));

  //insert one license
  buf.mtype = TYPE_LICENSE;
  if(msg_send(&buf) == -1){
    return -1;
  }

  //insert one file license
  buf.mtype = TYPE_LOGFILE;
  if(msg_send(&buf) == -1){
    return -1;
  }
  return 0;
}

int addtolicense(int n){
  crit_lock(TYPE_LICENSE);
  license_obj->nlicenses += n;
  crit_unlock(TYPE_LICENSE);
  fprintf(stderr, "P%d: %d license added\n", getpid(), n);
  return 0;
}

int removelicenses(int n){
  crit_lock(TYPE_LICENSE);
  license_obj->nlicenses -= n;
  crit_unlock(TYPE_LICENSE);
  fprintf(stderr, "P%d: %d license removed\n", getpid(), n);
  return 0;
}


void logmsg(const char* msg){

  const size_t len = strlen(msg);


  fprintf(stderr, "P%d: logging -> %s", getpid(), msg);

  /*log file is critical, so we have to lock it */
  crit_lock(TYPE_LOGFILE);

  const int fd = open(LOGNAME, O_CREAT | O_APPEND | O_WRONLY, 0700);
  if(fd == -1){
    perror(perror_buf);
  }else{
    write(fd, (void*) msg, len);
    close(fd);
  }

  crit_unlock(TYPE_LOGFILE);
}
