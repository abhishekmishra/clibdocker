/**
 * Copyright (c) 2020 Abhishek Mishra
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

/**
 * \file docker_result.h
 * \brief Docker Results
 */

#ifndef SRC_DOCKER_RESULT_H_
#define SRC_DOCKER_RESULT_H_

#define DOCKER_PARAM_TRUE 1
#define DOCKER_PARAM_FALSE 0

#ifdef __cplusplus  
extern "C" {
#endif

#include "docker_common.h"
#include <time.h>

// The error code usage below based on suggestions at
// https://stackoverflow.com/questions/6286874/c-naming-suggestion-for-error-code-enums

enum _config_error {
	E_UNKNOWN_ERROR = 1,
	E_SUCCESS = 0,
	E_INVALID_INPUT = -1,
	E_FILE_NOT_FOUND = -2,
	E_ALLOC_FAILED = -3,
	E_PING_FAILED = -4,
	E_CONNECTION_FAILED = -5
};

/* type to provide in your API */
typedef enum _config_error d_err_t;

#define HTTP_GET_STR "GET"
#define HTTP_POST_STR "POST"
#define HTTP_DELETE_STR "DELETE"

/* use this to provide a perror style method to help consumers out */
//struct _errordesc {
//	int code;
//	char *message;
//} errordesc[] = { { E_SUCCESS, "No error" },
//		{ E_INVALID_INPUT, "Invalid input" }, { E_FILE_NOT_FOUND,
//				"File not found" }, };
/**
 * The universal result object for docker API calls in this library.
 * This struct and associated methods provide ways to extract
 * success/failure state of the API Call, and messages associated with
 * any error.
 *
 * The struct must be inspected before the result values of the API
 * calls are used to ensure safe access.
 *
 * Also the result object must be freed using free_docker_result,
 * as soon as it is no longer needed.
 */
typedef struct docker_result_t {
	d_err_t error_code;
	time_t start_time;
	time_t end_time;
	char* url;
	char* method;
	char* request_json_str;
	char* response_json_str;
	long http_error_code;
	char* message;
} docker_result;

/**
 * Utility method to create docker result, should be used by all API
 * implementations to create the result object to return.
 */
MODULE_API d_err_t new_docker_result(docker_result** result);

/**
 * Utility method to create docker result, should be used by all API
 * implementations to create the result object to return.
 *
 * Makes a copy of all provided data so that they can be
 * freed after creation of the result.
 */
//MODULE_API d_err_t make_docker_result(docker_result** result, d_err_t error_code,
//		long http_error_code, const char* url, const char* msg);

/**
 * Frees all internal memory used in the docker_result, should be
 * called for all result objects as soon as they are no longer needed.
 */
MODULE_API void free_docker_result(docker_result* result);

MODULE_API docker_result* docker_result_clone(docker_result* result);

MODULE_API d_err_t docker_result_get_error_code(docker_result* result);
MODULE_API time_t docker_result_get_start_time(docker_result* result);
MODULE_API time_t docker_result_get_end_time(docker_result* result);
MODULE_API char* docker_result_get_url(docker_result* result);
MODULE_API char* docker_result_get_method(docker_result* result);
MODULE_API char* docker_result_get_request_json_str(docker_result* result);
MODULE_API char* docker_result_get_response_json_str(docker_result* result);
MODULE_API long docker_result_get_http_error_code(docker_result* result);
MODULE_API char* docker_result_get_message(docker_result* result);

/**
 * Check if the error_code is E_SUCCESS
 */
MODULE_API int is_ok(docker_result* result);


//TODO these two functions can be made a single generic function.
/**
 * A simple error handler suitable for programs
 * which just want to log the error (if any).
 */
MODULE_API void docker_simple_error_handler_print(docker_result* res);

/**
 * A simple error handler suitable for programs
 * which just want to log the error (if any).
 */
MODULE_API void docker_simple_error_handler_log(docker_result* res);

#ifdef __cplusplus 
}
#endif

#endif /* SRC_DOCKER_RESULT_H_ */
