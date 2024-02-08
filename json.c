#include "json.h"

#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

JSON_head_t* JSON_parse_string(char** text)
{
	if(**text != '"')
	{
		fprintf(stderr, "[JSON_parse_string] error: unexpected character '%c'. string must begin with '\"'\n", **text);
		exit(EXIT_FAILURE);
	}
	*text += 1;
	
	char* start = *text;
	unsigned int length = 0;
	while(**text != '\0')
	{
		if(**text == '\\')
		{
			*text += 1;
		}
		else if(**text == '"')
		{
			JSON_string_t* json_string = malloc(sizeof(JSON_string_t));
			json_string->head = JSON_STRING;
			json_string->length = length;
			json_string->value = malloc(length+1);
			memcpy(json_string->value, start, length);
			json_string->value[length] = '\0';

			return &json_string->head;
		}
		else
		{
			 length++;
		}

		*text += 1;
	}

	fprintf(stderr, "[JSON_parse_string] error: unexpected EOF\n");
	exit(EXIT_FAILURE);
}

JSON_head_t* JSON_parse_num(char** text)
{
	if(!isdigit(**text) && **text != '-' && **text != '.')
	{
		fprintf(stderr, "[JSON_parse_num] error: unexpected character '%c'. number must begin with a digit, '-', or '.'\n", **text);
		exit(EXIT_FAILURE);
	}

	int whole_body = 0;

	bool after_point = false;
	float frac_place = 0.1f;
	float frac_body = 0;

	bool after_exp = false;
	int exp_body = 0;
	
	int sign = 1;

	while(**text != '\0')
	{
		if(**text == '-')
		{
			sign = -1;
		}
		else if(**text == '.')
		{
			after_point = true;
		}
		else if(**text == 'e' || **text == 'E')
		{
			after_exp = true;
			sign = 1;
		}
		else if(isdigit(**text))
		{
			int digit = **text - '0';
			if(after_exp)
			{
				exp_body *= 10;
				exp_body += digit * sign;
			}
			else if(after_point)
			{
				frac_body += digit * sign * frac_place;
				frac_place /= 10.0f;
			}
			else
			{
				whole_body *= 10;
				whole_body += digit * sign;
			}
		}
		else
		{
			float total = (whole_body + frac_body) * pow(10, exp_body);
			JSON_POD_t* json_pod = malloc(sizeof(JSON_POD_t));
			if(after_point)
			{
				json_pod->head = JSON_FLOAT;
				json_pod->value.float_value = total;
			}
			else
			{
				json_pod->head = JSON_INT;
				json_pod->value.int_value = (int) total;
			}
			
			*text -= 1;
			return &json_pod->head;
		}
		
		*text += 1;
	}

	fprintf(stderr, "[JSON_parse_num] error: unexpected EOF\n");
	exit(EXIT_FAILURE);
}

void JSON_object_resize(JSON_object_t* json_object)
{
	JSON_head_t** old_keys = json_object->keys;
	JSON_head_t** old_values = json_object->values;
	json_object->capacity *= 2;
	json_object->keys = malloc(sizeof(JSON_string_t*) * json_object->capacity);
	json_object->values = malloc(sizeof(JSON_head_t*) * json_object->capacity);

	for(int i = 0; i < json_object->length; i++)
	{
		json_object->keys[i] = old_keys[i];
		json_object->values[i] = old_values[i];
	}

	free(old_keys);
	free(old_values);
}

void JSON_object_add(JSON_object_t* json_object, JSON_head_t* key, JSON_head_t* value)
{
	if(json_object->length >= json_object->capacity)
	{
		JSON_object_resize(json_object);
	}

	json_object->keys[json_object->length] = key;
	json_object->values[json_object->length] = value;
	json_object->length += 1;
}

JSON_head_t* JSON_parse_object(char** text)
{
	if(**text != '{')
	{
		fprintf(stderr, "[JSON_parse_object] error: unexpected character '%c'. object must begin with '{'\n", **text);
		exit(EXIT_FAILURE);
	}
	*text += 1;

	JSON_object_t* json_object = malloc(sizeof(JSON_object_t));
	json_object->head = JSON_OBJECT;
	json_object->capacity = 4;
	json_object->length = 0;
	json_object->keys = malloc(sizeof(JSON_string_t*) * json_object->capacity);
	json_object->values = malloc(sizeof(JSON_head_t*) * json_object->capacity);

	enum
	{
		SEEK_KEY,
		SEEK_COLON,
		SEEK_VALUE,
		SEEK_COMMA
	} state = SEEK_KEY;
	JSON_head_t* key = NULL;

	while(**text != '\0')
	{
		if(**text == '}')
		{
			return &json_object->head;
		}
		else if(**text == ',')
		{
			if(state != SEEK_COMMA)
			{
				fprintf(stderr, "[JSON_parse_object] error: unexpected ','\n");
				exit(EXIT_FAILURE);
			}

			state = SEEK_KEY;
		}
		else if(**text == ':')
		{
			if(state != SEEK_COLON)
			{
				fprintf(stderr, "[JSON_parse_object] error: unexpected ':'\n");
				exit(EXIT_FAILURE);
			}

			state = SEEK_VALUE;
		}
		else if(!isspace(**text))
		{
			if(state == SEEK_KEY)
			{
				key = JSON_parse_value(text);
				if(*key != JSON_STRING)
				{
					fprintf(stderr, "[JSON_parse_object] error: non-string key\n");
					exit(EXIT_FAILURE);
				}
				state = SEEK_COLON;
			}
			else if(state == SEEK_VALUE)
			{
				JSON_head_t* value = JSON_parse_value(text);
				JSON_object_add(json_object, key, value);
				key = NULL;
				state = SEEK_COMMA;
			}
			else
			{
				fprintf(stderr, "[JSON_parse_object] error: unexpected value\n");
				exit(EXIT_FAILURE);
			}
		}

		*text += 1;
	}

	fprintf(stderr, "[JSON_parse_object] error: unexpected EOF\n");
	exit(EXIT_FAILURE);
}

void JSON_array_resize(JSON_array_t* json_array)
{
	JSON_head_t** old_values = json_array->values;
	json_array->capacity *= 2;
	json_array->values = malloc(sizeof(JSON_head_t*) * json_array->capacity);

	for(int i = 0; i < json_array->length; i++)
	{
		json_array->values[i] = old_values[i];
	}

	free(old_values);
}

void JSON_array_add(JSON_array_t* json_array, JSON_head_t* value)
{
	if(json_array->length >= json_array->capacity)
	{
		JSON_array_resize(json_array);
	}

	json_array->values[json_array->length] = value;
	json_array->length += 1;
}

JSON_head_t* JSON_parse_array(char** text)
{
	if(**text != '[')
	{
		fprintf(stderr, "[JSON_parse_array] error: unexpected character '%c'. array must begin with '['\n", **text);
		exit(EXIT_FAILURE);
	}
	*text += 1;

	JSON_array_t* json_array = malloc(sizeof(JSON_array_t));
	json_array->head = JSON_ARRAY;
	json_array->capacity = 4;
	json_array->length = 0;
	json_array->values = malloc(sizeof(JSON_head_t*) * json_array->capacity);

	enum
	{
		SEEK_VALUE,
		SEEK_COMMA
	} state = SEEK_VALUE;

	while(**text != '\0')
	{
		if(**text == ']')
		{
			return &json_array->head;
		}
		else if(**text == ',')
		{
			if(state != SEEK_COMMA)
			{
				fprintf(stderr, "[JSON_parse_array] error: unexpected ','\n");
				exit(EXIT_FAILURE);
			}

			state = SEEK_VALUE;
		}
		else if(!isspace(**text))
		{
			if(state != SEEK_VALUE)
			{
				fprintf(stderr, "[JSON_parse_array] error: unexpected value\n");
				exit(EXIT_FAILURE);
			}
			
			JSON_head_t* value = JSON_parse_value(text);
			JSON_array_add(json_array, value);
			state = SEEK_COMMA;
		}
		
		*text += 1;
	}

	fprintf(stderr, "[JSON_parse_array] error: unexpected EOF\n");
	exit(EXIT_FAILURE);
}

JSON_head_t* JSON_parse_bool(char** text)
{
	bool value_true = memcmp(*text, "true", 4) == 0;
	bool value_false = memcmp(*text, "false", 5) == 0;
	if(!value_true && !value_false)
	{
		fprintf(stderr, "[JSON_parse_bool] error: expected \"true\" or \"false\"\n");
		exit(EXIT_FAILURE);
	}

	JSON_POD_t* json_bool = malloc(sizeof(JSON_POD_t));
	json_bool->head = JSON_BOOL;
	json_bool->value.bool_value = value_true;
	*text += value_true ? 3 : 4;

	return &json_bool->head;
}

JSON_head_t* JSON_parse_null(char** text)
{
	if(memcmp(*text, "null", 4) != 0)
	{
		fprintf(stderr, "[JSON_parse_null] error: expected \"null\"\n");
		exit(EXIT_FAILURE);
	}

	JSON_POD_t* json_null = malloc(sizeof(JSON_POD_t));
	json_null->head = JSON_NULL;
	json_null->value.null_value = 0;
	*text += 3;

	return &json_null->head;
}

JSON_head_t* JSON_parse_value(char** text)
{
	while(**text != '\0')
	{
		if(**text == '"')
		{
			JSON_head_t* json_string = JSON_parse_string(text);
			return json_string;
		}
		else if(isdigit(**text) || **text == '-' || **text == '.')
		{
			JSON_head_t* json_num = JSON_parse_num(text);
			return json_num;
		}
		else if(**text == '{')
		{
			JSON_head_t* json_object = JSON_parse_object(text);
			return json_object;
		}
		else if(**text == '[')
		{
			JSON_head_t* json_array = JSON_parse_array(text);
			return json_array;
		}
		else if(**text == 't' || **text == 'f')
		{
			JSON_head_t* json_bool = JSON_parse_bool(text);
			return json_bool;
		}
		else if(**text == 'n')
		{
			JSON_head_t* json_null = JSON_parse_null(text);
			return json_null;
		}

		*text += 1;
	}

	fprintf(stderr, "[JSON_parse_value] error: unexpected EOF\n");
	exit(EXIT_FAILURE);
}

void JSON_print_value(JSON_head_t* head, int indent)
{
	switch(*head)
	{
		case JSON_STRING:
		{
			JSON_string_t* json_string = (JSON_string_t*) head;
			printf("[STRING] %s\n", json_string->value);
			break;
		}
		case JSON_INT:
		{
			JSON_POD_t* json_pod = (JSON_POD_t*) head;
			printf("[INT] %d\n", json_pod->value.int_value);
			break;
		}
		case JSON_FLOAT:
		{
			JSON_POD_t* json_pod = (JSON_POD_t*) head;
			printf("[FLOAT] %f\n", json_pod->value.float_value);
			break;
		}
		case JSON_OBJECT:
		{
			JSON_object_t* json_object = (JSON_object_t*) head;
			printf("[OBJECT]\n");
			for(int i = 0; i < json_object->length; i++)
			{
				for(int i = 0; i <= indent; i++)
				{ printf("\t"); }
				printf("%s : ", ((JSON_string_t*) json_object->keys[i])->value);
				JSON_print_value(json_object->values[i], indent+1);
			}
			break;
		}
		case JSON_ARRAY:
		{
			JSON_array_t* json_array = (JSON_array_t*) head;
			printf("[ARRAY]\n");
			for(int i = 0; i < json_array->length; i++)
			{
				for(int i = 0; i <= indent; i++)
				{ printf("\t"); }
				printf("%d. ", i);
				JSON_print_value(json_array->values[i], indent+1);
			}
			break;
		}
		case JSON_BOOL:
		{
			JSON_POD_t* json_pod = (JSON_POD_t*) head;
			printf("[BOOL] %s\n", json_pod->value.bool_value ? "true" : "false");
			break;
		}
		case JSON_NULL:
		{
			JSON_POD_t* json_pod = (JSON_POD_t*) head;
			printf("[NULL]\n");
			break;
		}
	}
}

int main(int argc, char** argv)
{
	FILE* file = fopen("sample.json", "r");
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* text = malloc(size+1);
	fread(text, 1, size, file);
	fclose(file);
	text[size] = '\0';

	printf("%s\n", text);

	JSON_head_t* json = JSON_parse_value(&text);
	JSON_print_value(json, 0);

	return 0;
}
