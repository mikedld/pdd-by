#ifndef PDDBY_PRIVATE_DECODE_SIMPLE_DATA_PARSER_H
#define PDDBY_PRIVATE_DECODE_SIMPLE_DATA_PARSER_H

#include "pddby.h"

#include <stdint.h>

struct pddby_simple_data_parser;
typedef struct pddby_simple_data_parser pddby_simple_data_parser_t;

pddby_simple_data_parser_t* pddby_simple_data_parser_new(pddby_t* pddby);
char* pddby_simple_data_parser_parse(pddby_simple_data_parser_t* parser, char const* text, int32_t* number,
    char*** image_names);
void pddby_simple_data_parser_free(pddby_simple_data_parser_t* parser);

#endif // PDDBY_PRIVATE_DECODE_SIMPLE_DATA_PARSER_H
