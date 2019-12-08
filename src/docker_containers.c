/*
 * docker_containers.c
 *
 *  Created on: 11-Dec-2018
 *      Author: abhishek
 */

#include "docker_util.h"
#include <arraylist.h>
#include <docker_log.h>
#include "docker_containers.h"

#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <json-c/linkhash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "docker_connection_util.h"

/**
 * List docker containers
 *
 * \param ctx the docker context
 * \param result the result object to return
 * \param container_list array_list of containers to be returned
 * \param all all or running only
 * \param limit max containers to return
 * \param size return the size of containers in response
 * \param varargs pairs of filters char* filter_name, char* filter_value (terminated by a NULL)
 * \return error code
 */
d_err_t docker_container_list(docker_context* ctx, docker_ctr_list** container_list,
	int all, int limit, int size, ...) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, NULL, "json") != 0) {
		return E_ALLOC_FAILED;
	}

	if (all > 0) {
		docker_call_params_add(call, "all", "true");
	}

	if (limit > 0) {
		char* lim_val = (char*)calloc(128, sizeof(char));
		if(lim_val == NULL) {
			return E_ALLOC_FAILED;
		}
		sprintf(lim_val, "%d", limit);
		docker_call_params_add(call, "limit", lim_val);
		free(lim_val);
	}

	if (size > 0) {
		docker_call_params_add(call, "size", "true");
	}

	va_list kvargs;
	va_start(kvargs, size);
	json_object* filters = make_filters();
	while (true) {
		char* filter_name = va_arg(kvargs, char*);
		if (filter_name == NULL) {
			break;
		}
		char* filter_value = va_arg(kvargs, char*);
		if (filter_value == NULL) {
			break;
		}
		add_filter_str(filters, filter_name, filter_value);
	}
	docker_call_params_add(call, "filters", (char*)filters_to_str(filters));

	d_err_t err = docker_call_exec(ctx, call, container_list);

	free_docker_call(call);
	return err;

}

d_err_t docker_create_container(docker_context* ctx,
	char** id, docker_ctr_create_params* params) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, NULL, "create") != 0) {
		return E_ALLOC_FAILED;
	}

	docker_call_request_data_set(call, (char*)json_object_to_json_string(params));
	docker_call_request_method_set(call, "POST");
	docker_call_content_type_header_set(call, HEADER_JSON);

	(*id) = NULL;
	json_object* response_obj = NULL;

	d_err_t err = docker_call_exec(ctx, call, &response_obj);

	json_object* idObj;
	if (json_object_object_get_ex(response_obj, "Id", &idObj)) {
		const char* container_id = json_object_get_string(idObj);
		(*id) = str_clone(container_id);
	}
	else {
		docker_log_debug("Id not found.");
	}

	json_object_put(response_obj);
	free_docker_call(call);
	return err;
}

docker_ctr* docker_inspect_container(docker_context* ctx, char* id, int size) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "json") != 0) {
		return NULL;
	}

	docker_ctr* ctr = NULL;
	d_err_t err = docker_call_exec(ctx, call, &ctr);

	free_docker_call(call);
	return ctr;
}

/**
 * List all processes in a container identified by id.
 *
 * \param ctx is a docker context
 * \param id is the container id
 * \param ps_args is the command line args to be passed to the ps command (can be NULL).
 * \return the process details as docker_container_ps list.
 */
d_err_t docker_process_list_container(docker_context* ctx,
	docker_container_ps** ps, char* id,	char* process_args) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "top") != 0) {
		return E_ALLOC_FAILED;
	}

	json_object* response_obj = NULL;
	d_err_t err = docker_call_exec(ctx, call, &response_obj);

	if (err == E_SUCCESS) {
		docker_container_ps* p;
		p = (docker_container_ps*)malloc(sizeof(docker_container_ps));
		if (!p) {
			return E_ALLOC_FAILED;
		}
		json_object* titles_obj;
		json_object_object_get_ex(response_obj, "Titles", &titles_obj);
		size_t num_titles = json_object_array_length(titles_obj);
		arraylist_new(&p->titles, &free);
		for (int i = 0; i < num_titles; i++) {
			arraylist_add(p->titles,
				(char*)json_object_get_string(
					json_object_array_get_idx(titles_obj, i)));
		}

		json_object* processes_obj;
		json_object_object_get_ex(response_obj, "Processes", &processes_obj);
		size_t num_processes = json_object_array_length(processes_obj);
		arraylist_new(&p->processes, (void (*)(void*)) & arraylist_free);
		for (int i = 0; i < num_processes; i++) {
			json_object* process_obj = json_object_array_get_idx(processes_obj,
				i);
			arraylist* process_arr;
			arraylist_new(&process_arr, &free);
			size_t num_vals = json_object_array_length(process_obj);
			for (int j = 0; j < num_vals; j++) {
				arraylist_add(process_arr,
					(char*)json_object_get_string(
						json_object_array_get_idx(process_obj, j)));
			}
			arraylist_add(p->processes, process_arr);
		}
		(*ps) = p;
	}

	free_docker_call(call);
	return err;
}

/**
 * Get the logs for the docker container.
 *
 * \param ctx docker context
 * \param log pointer to string to be returned.
 * \param follow - this param has no effect for now, as socket support is not implemented.
 * \param stdout whether to get stdout (>0 means yes)
 * \param stderr whether to get stdin (>0 means yes)
 * \param since time since which the logs are to be fetched (unix timestamp)
 * \param until time till which the logs are to be fetched (unix timestamp)
 * \param timestamps add timestamps to log lines (>0 means yes)
 * \param tail 0 means all, any positive number indicates the number of lines to fetch.
 * \return error code
 */
d_err_t docker_container_logs(docker_context* ctx, char** log, char* id, int follow, 
	int std_out, int std_err, long since, long until, int timestamps, int tail) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "logs") != 0) {
		return E_ALLOC_FAILED;
	}

	if (std_out > 0) {
		docker_call_params_add(call, "stdout", "true");
	}

	if (std_err > 0) {
		docker_call_params_add(call, "stderr", "true");
	}

	if (since >= 0) {
		char* since_val = (char*)malloc(128 * sizeof(char));
		if (since_val == NULL) {
			return E_ALLOC_FAILED;
		}
		sprintf(since_val, "%ld", since);
		docker_call_params_add(call, "since", since_val);
		free(since_val);
	}

	if (until > 0) {
		char* until_val = (char*)malloc(128 * sizeof(char));
		if (until_val == NULL) {
			return E_ALLOC_FAILED;
		}
		sprintf(until_val, "%ld", until);
		docker_call_params_add(call, "until", until_val);
		free(until_val);
	}

	if (timestamps > 0) {
		docker_call_params_add(call, "timestamps", "true");
	}

	if (tail > 0) {
		char* tail_val = (char*)malloc(128 * sizeof(char));
		if (tail_val == NULL) {
			return E_ALLOC_FAILED;
		}
		sprintf(tail_val, "%d", tail);
		docker_call_params_add(call, "tail", tail_val);
		free(tail_val);
	}

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	if(docker_call_response_data_get(call) != NULL) {
		(*log) = str_clone(docker_call_response_data_get(call) + 8);
	}

	free_docker_call(call);
	return ret;
}

///////////// Get Container FS Changes

/**
 * Create a new container change item.
 */
d_err_t make_docker_container_change(docker_container_change** item,
	const char* path, const char* kind) {
	(*item) = (docker_container_change*)calloc(1,
		sizeof(docker_container_change));
	if (!(*item)) {
		return E_ALLOC_FAILED;
	}
	(*item)->path = str_clone(path);
	if (kind != NULL) {
		if (strcmp(kind, "0") == 0) {
			(*item)->kind = DOCKER_FS_MODIFIED;
		}
		if (strcmp(kind, "1") == 0) {
			(*item)->kind = DOCKER_FS_ADDED;
		}
		if (strcmp(kind, "2") == 0) {
			(*item)->kind = DOCKER_FS_DELETED;
		}
	}
	return E_SUCCESS;
}

void free_docker_container_change(docker_container_change* item) {
	if (item) {
		if (item->path) {
			free(item->path);
		}
		free(item);
	}
}

d_err_t make_docker_changes_list(docker_changes_list** changes_list) {
	arraylist_new(changes_list,
		(void (*)(void*)) & free_docker_container_change);
	return E_SUCCESS;
}

int docker_changes_list_add(docker_changes_list* list,
	docker_container_change* item) {
	return arraylist_add(list, item);
}

docker_container_change* docker_changes_list_get_idx(docker_changes_list* list,
	int i) {
	return (docker_container_change*)arraylist_get(list, i);
}
size_t docker_changes_list_length(docker_changes_list* list) {
	return arraylist_length(list);
}

/**
 * Get the file system changes for the docker container.
 *
 * \param ctx docker context
 * \param changes pointer to struct to be returned.
 * \param id container id
 * \return error code
 */
d_err_t docker_container_changes(docker_context* ctx, docker_changes_list** changes, char* id) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "changes") != 0) {
		return E_ALLOC_FAILED;
	}

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);

	if ((json_object_get_type(response_obj) != json_type_null)) {
		docker_log_debug("Response = %s",
			json_object_to_json_string(response_obj));

		make_docker_changes_list(changes);
		for (int i = 0; i < json_object_array_length(response_obj); i++) {
			json_object* change_obj = json_object_array_get_idx(response_obj,
				i);
			docker_container_change* change;
			make_docker_container_change(&change,
				get_attr_str(change_obj, "Path"),
				get_attr_str(change_obj, "Kind"));
			docker_changes_list_add((*changes), change);
		}
	}
	else {
		docker_log_warn("Response = %s",
			json_object_to_json_string(response_obj));

		(*changes) = NULL;
	}

	free_docker_call(call);
	return ret;
}

/////// Docker container stats

/**
 * Get stats from a running container. (the non-streaming version)
 *
 * \param ctx docker context
 * \param stats the stats object to return
 * \param id container id
 * \return error code
 */
d_err_t docker_container_get_stats(docker_context* ctx,	docker_container_stats** stats, 
	char* id) {
	if (id == NULL || strlen(id) == 0) {
		return E_INVALID_INPUT;
	}

	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "stats") != 0) {
		return E_ALLOC_FAILED;
	}
	docker_call_params_add(call, "stream", str_clone("false"));

	d_err_t ret = docker_call_exec(ctx, call, stats);

	free_docker_call(call);
	return ret;
}

void parse_container_stats_cb(char* msg, void* cb, void* cbargs) {
	void (*docker_container_stats_cb)(docker_container_stats*,
		void*) = (void (*)(docker_container_stats*, void*))cb;
	if (msg) {
		if (docker_container_stats_cb) {
			json_object* response_obj = json_tokener_parse(msg);
			if (response_obj) {
				docker_container_stats_cb((docker_container_stats*)response_obj, cbargs);
			}
		}
	}
}

/**
 * Get stats from a running container. (the streaming version)
 *
 * \param ctx docker context
 * \param docker_container_stats_cb the callback which receives the stats object, and any client args
 * \param cbargs client args to be passed on to the callback (closure)
 * \param id container id
 * \return error code
 */
d_err_t docker_container_get_stats_cb(docker_context* ctx,
	void (*docker_container_stats_cb)(docker_container_stats* stats,
		void* cbargs), void* cbargs, char* id) {
	if (id == NULL || strlen(id) == 0) {
		return E_INVALID_INPUT;
	}

	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "stats") != 0) {
		return E_ALLOC_FAILED;
	}
	docker_call_params_add(call, "stream", str_clone("true"));
	docker_call_status_cb_set(call, &parse_container_stats_cb);
	docker_call_cb_args_set(call, docker_container_stats_cb);
	docker_call_client_cb_args_set(call, cbargs);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);

	free_docker_call(call);
	return ret;
}

float docker_container_stats_get_cpu_usage_percent(
	docker_container_stats* stats) {
	float cpu_percent = 0.0;
	docker_container_cpu_stats* cpu_stats = docker_container_stats_cpu_stats_get(stats);
	docker_container_cpu_stats* precpu_stats = docker_container_stats_precpu_stats_get(stats);
	int cpu_count = docker_container_cpu_stats_online_cpus_get(cpu_stats);

	long long cpu_delta = docker_cpu_usage_total_get(docker_container_cpu_stats_cpu_usage_get(cpu_stats))
		- docker_cpu_usage_total_get(docker_container_cpu_stats_cpu_usage_get(precpu_stats));
	long long sys_delta = docker_container_cpu_stats_system_cpu_usage_get(cpu_stats)
		- docker_container_cpu_stats_system_cpu_usage_get(precpu_stats);
	if (sys_delta > 0) {
		cpu_percent = (float)((100.0 * cpu_delta * cpu_count) / sys_delta);
	}
	return cpu_percent;
}

/**
 * Start a container
 *
 * \param ctx docker context
 * \param id container id
 * \param detachKeys (optional, pass NULL if not needed) key combination for detaching a container.
 * \return error code
 */
d_err_t docker_start_container(docker_context* ctx, char* id, char* detachKeys) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "start") != 0) {
		return E_ALLOC_FAILED;
	}

	if (detachKeys != NULL) {
		docker_call_params_add(call, "detachKeys", detachKeys);
	}
	docker_call_request_data_set(call, "");
	docker_call_request_method_set(call, HTTP_POST_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	free_docker_call(call);
	return ret;
}

/**
 * Stop a container
 *
 * \param ctx docker context
 * \param id container id
 * \param t number of seconds to wait before killing the container
 * \return error code
 */
d_err_t docker_stop_container(docker_context* ctx, char* id, int t) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "stop") != 0) {
		return E_ALLOC_FAILED;
	}

	if (t > 0) {
		char* tstr = (char*)calloc(128, sizeof(char));
		if (tstr == NULL) {
			return E_ALLOC_FAILED;
		}
		sprintf(tstr, "%d", t);
		docker_call_params_add(call, "t", tstr);
		free(tstr);
	}
	docker_call_request_data_set(call, "");
	docker_call_request_method_set(call, HTTP_POST_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	//if (ret == 304) {
	//	(*result)->message = str_clone(
	//		"container is already stopped.");
	//}

	//if (ret == 404) {
	//	(*result)->message = str_clone("container not found.");
	//}

	free_docker_call(call);
	return ret;
}

/**
 * Restart a container
 *
 * \param ctx docker context
 * \param id container id
 * \param t number of seconds to wait before killing the container
 * \return error code
 */
d_err_t docker_restart_container(docker_context* ctx, char* id, int t) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "restart") != 0) {
		return E_ALLOC_FAILED;
	}

	if (t > 0) {
		char* tstr = (char*)calloc(128, sizeof(char));
		if (tstr == NULL) {
			return E_ALLOC_FAILED;
		}
		sprintf(tstr, "%d", t);
		docker_call_params_add(call, "t", tstr);
		free(tstr);
	}
	docker_call_request_data_set(call, "");
	docker_call_request_method_set(call, HTTP_POST_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	//if (ret == 404) {
	//	(*result)->message = str_clone("container not found.");
	//}

	free_docker_call(call);
	return ret;
}

/**
 * Kill a container
 *
 * \param ctx docker context
 * \param id container id
 * \param signal (optional - NULL for default i.e. SIGKILL) signal name to send
 * \return error code
 */
d_err_t docker_kill_container(docker_context* ctx, char* id, char* signal) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "kill") != 0) {
		return E_ALLOC_FAILED;
	}

	if (signal != NULL) {
		docker_call_params_add(call, "signal", str_clone(signal));
	}
	docker_call_request_data_set(call, "");
	docker_call_request_method_set(call, HTTP_POST_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	//if ((*result)->http_error_code == 404) {
	//	(*result)->message = str_clone("container not found.");
	//}

	//if ((*result)->http_error_code == 409) {
	//	(*result)->message = str_clone("container is not running.");
	//}

	free_docker_call(call);
	return ret;
}

/**
 * Rename a container
 *
 * \param ctx docker context
 * \param id container id
 * \param name new name for the container
 * \return error code
 */
d_err_t docker_rename_container(docker_context* ctx, char* id, char* name) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "rename") != 0) {
		return E_ALLOC_FAILED;
	}

	if (name != NULL) {
		docker_call_params_add(call, "name", str_clone(name));
	}
	docker_call_request_data_set(call, "");
	docker_call_request_method_set(call, HTTP_POST_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	//if ((*result)->http_error_code == 404) {
	//	(*result)->message = str_clone("container not found.");
	//}

	//if ((*result)->http_error_code == 409) {
	//	(*result)->message = str_clone("name is already in use");
	//}

	free_docker_call(call);
	return ret;
}

/**
 * Pause a container
 *
 * \param ctx docker context
 * \param id container id
 * \return error code
 */
d_err_t docker_pause_container(docker_context* ctx, char* id) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "pause") != 0) {
		return E_ALLOC_FAILED;
	}

	docker_call_request_data_set(call, "");
	docker_call_request_method_set(call, HTTP_POST_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	//if ((*result)->http_error_code == 404) {
	//	(*result)->message = str_clone("container not found.");
	//}

	free_docker_call(call);
	return ret;
}

/**
 * Unpause a container
 *
 * \param ctx docker context
 * \param id container id
 * \return error code
 */
d_err_t docker_unpause_container(docker_context* ctx, char* id) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "unpause") != 0) {
		return E_ALLOC_FAILED;
	}

	docker_call_request_data_set(call, "");
	docker_call_request_method_set(call, HTTP_POST_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	//if ((*result)->http_error_code == 404) {
	//	(*result)->message = str_clone("container not found.");
	//}

	free_docker_call(call);
	return ret;
}

/**
 * Wait for a container
 *
 * \param ctx docker context
 * \param id container id
 * \param condition (optional - NULL for default "not-running") condition to wait for
 * \return error code
 */
d_err_t docker_wait_container(docker_context* ctx, char* id, char* condition) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, id, "wait") != 0) {
		return E_ALLOC_FAILED;
	}

	if (condition != NULL) {
		docker_call_params_add(call, "condition", str_clone(condition));
	}
	docker_call_request_data_set(call, "");
	docker_call_request_method_set(call, HTTP_POST_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	free_docker_call(call);
	return ret;
}

/**
 * Remove a container
 *
 * \param ctx docker context
 * \param result pointer to docker_result
 * \param id container id
 * \param v remove volumes associated with the container
 * \param force if the container is running, kill it before removing it.
 * \param link remove specified link
 * \return error code
 */
d_err_t docker_remove_container(docker_context* ctx, char* id, int v, int force, int link) {
	docker_call* call;
	if (make_docker_call(&call, ctx->url, CONTAINER, NULL, id) != 0) {
		return E_ALLOC_FAILED;
	}

	docker_call_request_method_set(call, HTTP_DELETE_STR);

	json_object* response_obj = NULL;
	d_err_t ret = docker_call_exec(ctx, call, &response_obj);
	json_object_put(response_obj);

	free_docker_call(call);
	return ret;
}
