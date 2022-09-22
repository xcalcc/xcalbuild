// This file is distributed under MIT-LICENSE. See COPYING for details.

#pragma once

struct bear_message;

int  bear_open_json_output(char const * file);
void bear_close_json_output(int fd);

void bear_append_json_output(int fd, struct bear_message const * e, int debug);

void bear_set_known_compilers();
void bear_print_known_compilers();
