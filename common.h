
struct license_object {
  /* number of available licenses */
  int nlicenses;
};

/* perror prefix */
extern char perror_buf[100];
/* signal handler flag for interruption */
extern int is_signalled;

/* License object pointer to shared memory */
extern struct license_object * license_obj;

/* create license object*/
int create_license(const int n);
/* destroy and cler the shared memory object */
int destroy_license();

const char * make_msg(const char * msg);

struct msgbuf {
 long mtype;       /* message type, must be > 0 */
 char mtext[1];
};

enum msg_types {
  //can't start at 0, because its invalid message type
  
  TYPE_LICENSE=100, //to access license in shared memroy
  TYPE_LOGFILE      //to access log file
};

int msg_send(const struct msgbuf * buf);
int msg_recv(      struct msgbuf * buf);
