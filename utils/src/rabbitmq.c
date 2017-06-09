#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <R.h>
#include <Rinternals.h>

amqp_socket_t *socket = NULL;
amqp_connection_state_t conn;

// ----------------------------------------------------------------------
// Helper function to check status of RabbitMQ call
// ----------------------------------------------------------------------
int amqp_check_status(amqp_rpc_reply_t x, char const *context) {
  switch (x.reply_type) {
  case AMQP_RESPONSE_NORMAL:
    return TRUE;

  case AMQP_RESPONSE_NONE:
    REprintf("%s: missing RPC reply type!\n", context);
    break;

  case AMQP_RESPONSE_LIBRARY_EXCEPTION:
    REprintf("%s: %s\n", context, amqp_error_string2(x.library_error));
    break;

  case AMQP_RESPONSE_SERVER_EXCEPTION:
    switch (x.reply.id) {
    case AMQP_CONNECTION_CLOSE_METHOD: {
      amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
      REprintf("%s: server connection error %uh, message: %.*s\n",
              context,
              m->reply_code,
              (int) m->reply_text.len, (char *) m->reply_text.bytes);
      break;
    }
    case AMQP_CHANNEL_CLOSE_METHOD: {
      amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
      REprintf("%s: server channel error %uh, message: %.*s\n",
              context,
              m->reply_code,
              (int) m->reply_text.len, (char *) m->reply_text.bytes);
      break;
    }
    default:
      REprintf("%s: unknown server error, method id 0x%08X\n", context, x.reply.id);
      break;
    }
    break;
  }

  return FALSE;
}

// ----------------------------------------------------------------------
// Helper function to check return code of RabbitMQ call
// ----------------------------------------------------------------------
int amqp_check_error(int x, char const *context) {
  if (x < 0) {
    fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x));
    return FALSE;
  }
  return TRUE;
}

// ----------------------------------------------------------------------
// Open connection to RabbitMQ
// ----------------------------------------------------------------------
SEXP rabbitmq_connect(SEXP host) {
  Rprintf("connect\n");
  const char* host_s = CHAR(asChar(host));
  const char* vhost = "/";
  const char* username = "guest";
  const char* password = "guest";
  int port = 5672;
  int status;
  amqp_rpc_reply_t reply;

  if (socket != NULL) {
    REprintf("Already connected to rabbitmq, close connection first\n");
    return ScalarLogical(FALSE);
  }

  conn = amqp_new_connection();

  socket = amqp_tcp_socket_new(conn);
  if (!socket) {
    REprintf("Error creating TCP socket\n");
    socket = NULL;
    return ScalarLogical(FALSE);
  }

  status = amqp_socket_open(socket, host_s, port);
  if (status) {
    REprintf("Error opening TCP socket\n");
    socket = NULL;
    return ScalarLogical(FALSE);
  }

  reply = amqp_login(conn, vhost, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, username, password);
  if (!amqp_check_status(reply, "Login")) {
    socket = NULL;
    return ScalarLogical(FALSE);
  }

  amqp_channel_open(conn, 1);
  if (!amqp_check_status(amqp_get_rpc_reply(conn), "Open channel")) {
    socket = NULL;
    return ScalarLogical(FALSE);
  }

  return ScalarLogical(TRUE);
}

// ----------------------------------------------------------------------
// Close connection to RabbitMQ
// ----------------------------------------------------------------------
SEXP rabbitmq_close() {
  Rprintf("close\n");
  amqp_rpc_reply_t reply;

  if (socket == NULL) {
    return ScalarLogical(TRUE);
  }
  socket = NULL;

  reply = amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
  if (!amqp_check_status(reply, "Closing channel")) {
    return ScalarLogical(FALSE);
  }

  reply = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
  if (!amqp_check_status(reply, "Closing connection")) {
    return ScalarLogical(FALSE);
  }


  if (!amqp_check_error(amqp_destroy_connection(conn), "Ending connection")) {
    return ScalarLogical(FALSE);
  }

  return ScalarLogical(TRUE);
}

// ----------------------------------------------------------------------
// Publish a message to RabbitMQ
// ----------------------------------------------------------------------
SEXP rabbitmq_publish(SEXP msg, SEXP queue) {
  Rprintf("publish\n");
  const char* msg_s = CHAR(asChar(msg));
  const char* queue_s = CHAR(asChar(queue));
  int reply;
  amqp_basic_properties_t props;
  amqp_queue_declare_ok_t *res;

  // declare queue
  res = amqp_queue_declare(conn, 1,
                           amqp_cstring_bytes(queue_s),
                           0, 1, 0, 0,
                           amqp_empty_table);
  if (res == NULL) {
    amqp_check_status(amqp_get_rpc_reply(conn), "queue.declare");
    return ScalarLogical(FALSE);
  }

  // publish message
  props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props.content_type = amqp_cstring_bytes("text/plain");
  props.delivery_mode = 2; /* persistent delivery mode */
  reply = amqp_basic_publish(conn, 1,
                             amqp_cstring_bytes(""),
                             amqp_cstring_bytes(queue_s),
                             0, 0, &props,
                             amqp_cstring_bytes(msg_s));

  return ScalarLogical(amqp_check_error(reply, "Publish"));
}

// ----------------------------------------------------------------------
// Listen for message from RabbitMQ
// ----------------------------------------------------------------------
SEXP rabbitmq_listen(SEXP queue, SEXP messages, SEXP fn) {
  Rprintf("listen\n");
  const char* queue_s = CHAR(asChar(queue));

  // simple check
  if(!isFunction(fn)) error("'fn' must be a function");

  // declare queue
  res = amqp_queue_declare(conn, 1,
                           amqp_cstring_bytes(queue_s),
                           0, 1, 0, 0,
                           amqp_empty_table);
  if (res == NULL) {
    amqp_check_status(amqp_get_rpc_reply(conn), "queue.declare");
    return ScalarLogical(FALSE);
  }

  // listen for messages
  amqp_basic_consume(conn, 1,
                     amqp_cstring_bytes(queuename),
                     amqp_empty_bytes, 0, 1, 0,
                     amqp_empty_table);
  if (amqp_check_error(amqp_get_rpc_reply(conn), "Consuming")) {
    return ScalarLogical(FALSE);
  }

  // process messages
  //for (;;) {
    amqp_rpc_reply_t res;
    amqp_envelope_t envelope;

    amqp_maybe_release_buffers(conn);
    res = amqp_consume_message(conn, &envelope, NULL, 0);
    if (AMQP_RESPONSE_NORMAL != res.reply_type) {
      break;
    }
    deliver = (amqp_basic_deliver_t *)envelope.payload.method.decoded;
    delivery_tag = deliver->delivery_tag;

    printf("Delivery %u, exchange %.*s routingkey %.*s\n",
           (unsigned) envelope.delivery_tag,
           (int) envelope.exchange.len, (char *) envelope.exchange.bytes,
           (int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);

    if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
      printf("Content-type: %.*s\n",
             (int) envelope.message.properties.content_type.len,
             (char *) envelope.message.properties.content_type.bytes);
    }
    printf("----\n");

    amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
    amqp_destroy_envelope(&envelope);

    res = amqp_basic_ack(conn, 1, delivery_tag, 0);
    if (!amqp_check_error(res, "Basic ACK")) {
      return ScalarLogical(FALSE);
    }
  //}

//  R_GlobalEnv
//  R_fcall = PROTECT(lang2(fn, R_NilValue));
//  UNPROTECT(2);

  return ScalarLogical(TRUE);
}
