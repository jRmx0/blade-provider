#ifndef DISPATCHER_H
#define DISPATCHER_H

char *dispatch_metadata_json(void);
char *dispatch_compute_json(const char *request_json);
void dispatch_string_free(char *value);

#endif // DISPATCHER_H