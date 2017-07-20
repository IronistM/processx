
#include "processx-unix.h"

void processx__con_destroy(Rconnection con) {
  processx_conn_handle_t *handle = con->private;
  if (handle) {
    processx_handle_t *process = handle->process;
    if (process) {
      if (process->fd1 == con->status) {
	process->fd1 = -1;
	process->std_out = NULL;
      }
      if (process->fd2 == con->status) {
	process->fd2 = -1;
	process->std_err = NULL;
      }
    }
  }
  if (con->status >= 0) close(con->status);
  con->status = -1;
  con->isopen = 0;

  if (handle) free(handle);
  con->private = 0;
}

size_t processx__con_read(void *target, size_t sz, size_t ni,
			  Rconnection con) {
  int num;
  int fd = con->status;
  processx_conn_handle_t *handle = con->private;

  if (fd < 0) error("Connection was already closed");
  if (sz != 1) error("Can only read bytes from processx connections");

  /* Already got EOF? */
  if (con->EOF_signalled) return 0;

  num = read(fd, target, ni);

  con->incomplete = 1;

  if (num < 0 && errno == EAGAIN) {
    num = 0;			/* cannot return negative number */

  } else if (num < 0) {
    error("Cannot read from processx pipe");

  } else if (num == 0) {
    con->incomplete = 0;
    con->EOF_signalled = 1;
    /* If the last line does not have a trailing '\n', then
       we add one manually, because otherwise readLines() will
       never read this line. */
    if (handle->tail != '\n') {
      ((char*)target)[0] = '\n';
      num = 1;
    }

  } else {
    /* Make note of the last character, to know if the last line
       was incomplete or not. */
    handle->tail = ((char*)target)[num - 1];
  }

  return (size_t) num;
}

int processx__con_fgetc(Rconnection con) {
  int x = 0;
#ifdef WORDS_BIGENDIAN
  return processx__con_read(&x, 1, 1, con) ? BSWAP_32(x) : -1;
#else
  return processx__con_read(&x, 1, 1, con) ? x : -1;
#endif
}

processx_conn_handle_t* processx__create_connection(
  processx_handle_t *handle,
  int fd, const char *membername,
  SEXP private) {

  Rconnection con;
  SEXP res =
    PROTECT(R_new_custom_connection("processx", "r", "textConnection", &con));

  processx_conn_handle_t *conn_handle =
    (processx_conn_handle_t*) malloc(sizeof(processx_conn_handle_t));
  if (!conn_handle) error("out of memory");
  conn_handle->tail = '\n';
  conn_handle->process = handle;

  con->incomplete = 1;
  con->EOF_signalled = 0;
  con->private = conn_handle;
  con->status = fd;		/* slight abuse */
  con->canseek = 0;
  con->canwrite = 0;
  con->canread = 1;
  con->isopen = 1;
  con->blocking = 0;
  con->text = 1;
  con->UTF8out = 1;
  con->destroy = &processx__con_destroy;
  con->read = &processx__con_read;
  con->fgetc = &processx__con_fgetc;
  con->fgetc_internal = &processx__con_fgetc;

  defineVar(install(membername), res, private);
  UNPROTECT(1);

  return conn_handle;
}

void processx__create_connections(processx_handle_t *handle, SEXP private) {

  if (handle->fd1 >= 0) {
    handle->std_out = processx__create_connection(handle, handle->fd1,
						 "stdout_pipe", private);
  }

  if (handle->fd2 >= 0) {
    handle->std_err = processx__create_connection(handle, handle->fd2,
						 "stderr_pipe", private);
  }
}
