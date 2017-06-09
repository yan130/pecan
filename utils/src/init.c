#include <R.h>
#include <Rinternals.h>
#include <stdlib.h>
#include <R_ext/Rdynload.h>

extern SEXP rabbitmq_connect(SEXP host);
extern SEXP rabbitmq_close();
extern SEXP rabbitmq_publish(SEXP msg, SEXP queue);
extern SEXP rabbitmq_listen(SEXP queue, SEXP messages, SEXP fn);

static const R_CallMethodDef CallEntries[] = {
  {"rabbitmq_connect",     (DL_FUNC) &rabbitmq_connect,     1},
  {"rabbitmq_close",       (DL_FUNC) &rabbitmq_close,       0},
  {"rabbitmq_publish",     (DL_FUNC) &rabbitmq_publish,     2},
  {"rabbitmq_listen",      (DL_FUNC) &rabbitmq_listen,      3}
};

void R_init_PEcAn_utils(DllInfo *dll) {
  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
}
