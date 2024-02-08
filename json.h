#ifndef JSON_H
#define JSON_H

#include <stdbool.h>

typedef enum JSON_head
{
	JSON_STRING,
	JSON_INT,
	JSON_FLOAT,
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_BOOL,
	JSON_NULL
} JSON_head_t;

typedef struct JSON_POD
{
	JSON_head_t head;
	union
	{
		int int_value;
		float float_value;
		bool bool_value;
		unsigned int null_value;
	} value;
} JSON_POD_t;

typedef struct JSON_string
{
	JSON_head_t head;
	unsigned int length;
	char* value;
} JSON_string_t;

typedef struct JSON_object
{
	JSON_head_t head;
	unsigned int capacity;
	unsigned int length;
	JSON_head_t** keys;
	JSON_head_t** values;
} JSON_object_t;

typedef struct JSON_array
{
	JSON_head_t head;
	unsigned int capacity;
	unsigned int length;
	JSON_head_t** values;
} JSON_array_t;

JSON_head_t* JSON_parse_string(char** text);
JSON_head_t* JSON_parse_num(char** text);
JSON_head_t* JSON_parse_object(char** text);
JSON_head_t* JSON_parse_array(char** text);
JSON_head_t* JSON_parse_null(char** text);
JSON_head_t* JSON_parse_value(char** text);
void JSON_print_value(JSON_head_t* head, int indent);

#endif
