/*
 *
 * Copyright (c) 2018-2022 Abhishek Mishra
 *
 * This file is part of clibdocker.
 *
 * clibdocker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation, 
 * either version 3 of the License, or (at your option) 
 * any later version.
 *
 * clibdocker is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public 
 * License along with clibdocker. 
 * If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "docker_util.h"
#include "docker_system.h"
#include <docker_log.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json_tokener.h>

#include "docker_connection_util.h"

d_err_t docker_ping(docker_context* ctx) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, SYSTEM, NULL, "_ping") != 0) {
		return E_ALLOC_FAILED;
	}

	json_object *response_obj = NULL;
	
	d_err_t err = docker_call_exec(ctx, call, &response_obj);
	
	json_object_put(response_obj);

	free_docker_call(call);
	return E_SUCCESS;
}

d_err_t docker_system_version(docker_context* ctx,
		docker_version** version) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, SYSTEM, NULL, "version") != 0) {
		return E_ALLOC_FAILED;
	}

	d_err_t err = docker_call_exec(ctx, call, (json_object**) version);

	free_docker_call(call);
	return err;
}

d_err_t docker_system_info(docker_context* ctx,
		docker_info** info) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, SYSTEM, NULL, "info") != 0) {
		return E_ALLOC_FAILED;
	}

	d_err_t err = docker_call_exec(ctx, call, (json_object**)info);

	free_docker_call(call);
	return err;
}

void parse_events_cb(char* msg, void* cb, void* cbargs) {
	void (*events_cb)(docker_event*, void*) = (void (*)(docker_event*, void*))cb;
	if (msg) {
		if(events_cb) {
			json_object* evt_obj = json_tokener_parse(msg);
			events_cb(evt_obj, cbargs);
			json_object_put(evt_obj);
		} else {
			docker_log_debug("Message = Empty");
		}
	}
}

d_err_t docker_system_events(docker_context* ctx,
		arraylist** events, time_t start_time, time_t end_time) {
	if (end_time <= 0) {
		docker_log_warn(
				"This call with end_time %d will never end, and will have no response, use the method with callbacks instead.",
				end_time);
		return E_INVALID_INPUT;
	} else {
		return docker_system_events_cb(ctx, NULL, NULL, events,
				start_time, end_time);
	}
}

d_err_t docker_system_events_cb(docker_context* ctx,
		void (*docker_events_cb)(docker_event* evt, void* cbargs), void* cbargs,
		arraylist** events, time_t start_time, time_t end_time) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, SYSTEM, NULL, "events") != 0) {
		return E_ALLOC_FAILED;
	}

	char* start_time_str = (char*) calloc(128, sizeof(char));
	if (start_time_str == NULL) 
	{ 
		return E_ALLOC_FAILED; 
	}
	sprintf(start_time_str, "%lu", start_time);
	docker_call_params_add(call, "since", start_time_str);
	free(start_time_str);

	if (end_time != 0) {
		char* end_time_str = (char*) calloc(128, sizeof(char));
		if (end_time_str == NULL)
		{
			return E_ALLOC_FAILED;
		}
		sprintf(end_time_str, "%lu", end_time);
		docker_call_params_add(call, "until", end_time_str);
		free(end_time_str);
	}

	docker_call_status_cb_set(call, &parse_events_cb);
	docker_call_cb_args_set(call, docker_events_cb);
	docker_call_client_cb_args_set(call, cbargs);
	json_object *response_obj = NULL;

	d_err_t err = docker_call_exec(ctx, call, &response_obj);

	//cannot use the default response object, as that parses only one object from the response
	arraylist_new(events, (void (*)(void *)) &json_object_put);
	char* response_data = docker_call_response_data_get(call);
	if (err == E_SUCCESS) {
		if (response_data && strlen(response_data) > 0) {
			size_t len = strlen(response_data);
			size_t start = 0;
			size_t end = 0;
			for (size_t i = 0; i < len; i++) {
				if (response_data[i] == '\n') {
					response_data[i] = '\0';
					json_object* item = json_tokener_parse(
						response_data + start);
					arraylist_add(*events, item);
					response_data[i] = '\n';
					start = i;
				}
			}
		}
	}

	json_object_put(response_obj);
	free_docker_call(call);
	return err;
}

d_err_t docker_system_df(docker_context* ctx, docker_df** df) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, SYSTEM, NULL, "system/df") != 0) {
		return E_ALLOC_FAILED;
	}

	d_err_t err = docker_call_exec(ctx, call, (json_object**)df);

	free_docker_call(call);
	return err;
}
