
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include <getopt.h>   // for getopt_long

#include "block.h"

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)data;
	
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL)
	{
		printf("not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}
	
	memcpy(&(mem->memory[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;
	
	return realsize;
}

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	//assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static char url[2000] = "";
	
	struct Params * params = NULL;
	params = add_string_param(params, "url", 'u', url, 1);
	eval_params(params, argc, argv);
	
	struct MemoryStruct chunk;
	chunk.memory = NULL;
	chunk.size = 0;
	
	CURL *curl = NULL;
	CURLcode res;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	res = curl_easy_perform(curl);
	
	curl_easy_cleanup(curl);
	curl = NULL;
	
	fprintf(stderr, "- received %d byte response.\n", (int)chunk.size);
	if (chunk.size > 0)
	{
		fwrite(chunk.memory, chunk.size, 1, stdout);
		free(chunk.memory);
	}
}
