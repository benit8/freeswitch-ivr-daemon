#include "esl/esl.h"

ESL_DECLARE(const char*) cJSON_GetObjectCstr(const cJSON* object, const char* string)
{
	cJSON* cj = cJSON_GetObjectItem(object, string);

	if (!cj || cj->type != cJSON_String || !cj->valuestring)
		return NULL;

	return cj->valuestring;
}
