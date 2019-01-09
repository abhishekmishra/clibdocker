/*
 * clibdocker: docker_volumes.c
 * Created on: 06-Jan-2019
 *
 * MIT License
 *
 * Copyright (c) 2018 Abhishek Mishra
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "docker_volumes.h"
#include "docker_util.h"
#include "log.h"

error_t make_docker_volume(docker_volume** volume, time_t created_at,
		char* name, char* driver, char* mountpoint, char* scope) {
	(*volume) = (docker_volume*) malloc(sizeof(docker_volume));
	if ((*volume) == NULL) {
		return E_ALLOC_FAILED;
	}
	(*volume)->created_at = created_at;
	(*volume)->name = make_defensive_copy(name);
	(*volume)->driver = make_defensive_copy(driver);
	(*volume)->mountpoint = make_defensive_copy(mountpoint);
	(*volume)->scope = make_defensive_copy(scope);
	(*volume)->labels = array_list_new((void (*)(void *)) &free_pair);
	(*volume)->options = array_list_new((void (*)(void *)) &free_pair);
	(*volume)->status = array_list_new((void (*)(void *)) &free_pair);
	return E_SUCCESS;
}

void free_docker_volume(docker_volume* volume) {
	free(volume->name);
	free(volume->driver);
	free(volume->driver);
	free(volume->mountpoint);
	free(volume->scope);
	array_list_free(volume->options);
	array_list_free(volume->labels);
	array_list_free(volume->status);
	free(volume);
}

DOCKER_GETTER_IMPL(volume, time_t, created_at)
DOCKER_GETTER_IMPL(volume, char*, name)
DOCKER_GETTER_IMPL(volume, char*, driver)
DOCKER_GETTER_IMPL(volume, char*, mountpoint)
DOCKER_GETTER_IMPL(volume, char*, scope)

DOCKER_GETTER_ARR_ADD_IMPL(volume, pair*, labels)
DOCKER_GETTER_ARR_LEN_IMPL(volume, labels)
DOCKER_GETTER_ARR_GET_IDX_IMPL(volume, pair*, labels)

DOCKER_GETTER_ARR_ADD_IMPL(volume, pair*, options)
DOCKER_GETTER_ARR_LEN_IMPL(volume, options)
DOCKER_GETTER_ARR_GET_IDX_IMPL(volume, pair*, options)

DOCKER_GETTER_ARR_ADD_IMPL(volume, pair*, status)
DOCKER_GETTER_ARR_LEN_IMPL(volume, status)
DOCKER_GETTER_ARR_GET_IDX_IMPL(volume, pair*, status)

/**
 * Get the list of volumes, matching the filters provided.
 * (Any and all filters can be null/0.)
 *
 * \param ctx the docker context
 * \param result the result object to be returned.
 * \param volumes the list of docker_volume_item objects
 * \param warnings the list of warnings
 * \param filter_dangling 0 indicates false, anything else is true
 * \param filter_driver driver filter
 * \param filter_label
 * \param filter_name
 * \return error code.
 */
error_t docker_volumes_list(docker_context* ctx, docker_result** result,
		struct array_list** volumes, struct array_list** warnings,
		int filter_dangling, char* filter_driver, char* filter_label,
		char* filter_name) {
	char* url = create_service_url_id_method(VOLUME, NULL, NULL);

	struct array_list* params = array_list_new(
			(void (*)(void *)) &free_url_param);
	url_param* p;
	json_object* filters = make_filters();

	if (filter_dangling == 0) {
		add_filter_str(filters, "dangling", "false");
	}
	if (filter_driver != NULL) {
		add_filter_str(filters, "driver", filter_driver);
	}
	if (filter_label != NULL) {
		add_filter_str(filters, "label", filter_label);
	}
	if (filter_name != NULL) {
		add_filter_str(filters, "name", filter_name);
	}
	make_url_param(&p, "filters", (char*) filters_to_str(filters));
	array_list_add(params, p);

	json_object *response_obj = NULL;
	struct http_response_memory chunk;
	docker_api_get(ctx, result, url, params, &chunk, &response_obj);

	(*volumes) = array_list_new((void (*)(void *)) &free_docker_volume);
	(*warnings) = array_list_new(&free);

	json_object* volumes_obj;
	json_object_object_get_ex(response_obj, "Volumes", &volumes_obj);
	if (volumes_obj) {
		int num_vols = json_object_array_length(volumes_obj);
		for (int i = 0; i < num_vols; i++) {
			json_object* current_obj = json_object_array_get_idx(volumes_obj,
					i);
			docker_volume* vi;
			struct tm ctime;
			memset(&ctime, 0, sizeof(struct tm));
			parse_iso_datetime(get_attr_str(current_obj, "CreatedAt"), &ctime);
			time_t created_time = mktime(&ctime);

			make_docker_volume(&vi, created_time,
					get_attr_str(current_obj, "Name"),
					get_attr_str(current_obj, "Driver"),
					get_attr_str(current_obj, "Mountpoint"),
					get_attr_str(current_obj, "Scope"));

			json_object* labels_obj;
			json_object_object_get_ex(current_obj, "Labels", &labels_obj);
			if (labels_obj != NULL) {
				json_object_object_foreach(labels_obj, key, val)
				{
					pair* p;
					make_pair(&p, key, (char*) json_object_get_string(val));
					docker_volume_labels_add(vi, p);
				}
			}
			json_object* options_obj;
			json_object_object_get_ex(current_obj, "Options", &options_obj);
			if (options_obj != NULL) {
				json_object_object_foreach(options_obj, key1, val1)
				{
					pair* p;
					make_pair(&p, key1, (char*) json_object_get_string(val1));
					docker_volume_options_add(vi, p);
				}
			}
			array_list_add((*volumes), vi);
		}
	}

	json_object* warnings_obj;
	json_object_object_get_ex(response_obj, "Warnings", &warnings_obj);
	if (warnings_obj) {
		int num_warns = json_object_array_length(warnings_obj);
		for (int i = 0; i < num_warns; i++) {
			json_object* current_obj = json_object_array_get_idx(warnings_obj,
					i);
			char* warning = (char*) json_object_get_string(current_obj);
			array_list_add((*warnings), warning);
		}
	}

	free(params);
	return E_SUCCESS;
}

/**
 * Create a new volume by providing name, driver and an optional list of key/value pairs for labels
 *
 * \param ctx the docker context
 * \param result the result object to return
 * \param volume the volume object to return
 * \param name name of the volume to create (cannot be NULL)
 * \param driver name of the driver to use
 * \param num_labels the number of labels to be attached.
 * \param key/values char* key, char* value args pair for each label
 * \return error code
 */
error_t docker_volume_create(docker_context* ctx, docker_result** result,
		docker_volume** volume, char* name, char* driver, int num_labels,
		...) {
	va_list kvargs;
	va_start(kvargs, num_labels);
	json_object* request_obj = json_object_new_object();
	json_object_object_add(request_obj, "Name", json_object_new_string(name));
	json_object_object_add(request_obj, "Driver",
			json_object_new_string(driver));
	json_object* labels = json_object_new_object();
	for (int i = 0; i < num_labels; i++) {
		char* key = va_arg(kvargs, char*);
		char* val = va_arg(kvargs, char*);
		json_object_object_add(labels, key, json_object_new_string(val));
	}
	json_object_object_add(request_obj, "Labels", labels);

	const char* request_str = json_object_get_string(request_obj);
	docker_log_debug("Request body is %s", request_str);

	char* url = create_service_url_id_method(VOLUME, NULL, "create");
	json_object *response_obj = NULL;
	struct http_response_memory chunk;
	docker_api_post(ctx, result, url, NULL, request_str, &chunk, &response_obj);

	//If volume was created parse the response and return the details.
	if ((*result)->http_error_code == 201) {
		docker_volume* vi;
		struct tm ctime;
		memset(&ctime, 0, sizeof(struct tm));
		parse_iso_datetime(get_attr_str(response_obj, "CreatedAt"), &ctime);
		time_t created_time = mktime(&ctime);
		make_docker_volume(&vi, created_time,
				get_attr_str(response_obj, "Name"),
				get_attr_str(response_obj, "Driver"),
				get_attr_str(response_obj, "Mountpoint"),
				get_attr_str(response_obj, "Scope"));

		json_object* labels_obj;
		json_object_object_get_ex(response_obj, "Labels", &labels_obj);
		if (labels_obj != NULL) {
			json_object_object_foreach(labels_obj, key, val)
			{
				pair* p;
				make_pair(&p, key, (char*) json_object_get_string(val));
				docker_volume_labels_add(vi, p);
			}
		}

		json_object* status_obj;
		json_object_object_get_ex(response_obj, "Status", &status_obj);
		if (status_obj != NULL) {
			json_object_object_foreach(status_obj, key1, val1)
			{
				pair* p;
				make_pair(&p, key1, (char*) json_object_get_string(val1));
				docker_volume_status_add(vi, p);
			}
		}
		(*volume) = vi;
	}

	return E_SUCCESS;
}

/**
 * Inspect an existing volume.
 *
 * \param ctx the docker context
 * \param result the result object to return
 * \param volume the volume object to return
 * \param name name of the volume to inspect (cannot be NULL)
 * \return error code
 */
error_t docker_volume_inspect(docker_context* ctx, docker_result** result,
		docker_volume** volume, char* name) {
	if (name == NULL) {
		return E_INVALID_INPUT;
	}
	char* url = create_service_url_id_method(VOLUME, NULL, name);
	json_object *response_obj = NULL;
	struct http_response_memory chunk;
	docker_api_get(ctx, result, url, NULL, &chunk, &response_obj);

	//If volume was returned parse the response and return the details.
	if ((*result)->http_error_code == 200) {
		docker_volume* vi;
		struct tm ctime;
		memset(&ctime, 0, sizeof(struct tm));
		parse_iso_datetime(get_attr_str(response_obj, "CreatedAt"), &ctime);
		time_t created_time = mktime(&ctime);
		make_docker_volume(&vi, created_time,
				get_attr_str(response_obj, "Name"),
				get_attr_str(response_obj, "Driver"),
				get_attr_str(response_obj, "Mountpoint"),
				get_attr_str(response_obj, "Scope"));

		json_object* labels_obj;
		json_object_object_get_ex(response_obj, "Labels", &labels_obj);
		if (labels_obj != NULL) {
			json_object_object_foreach(labels_obj, key, val)
			{
				pair* p;
				make_pair(&p, key, (char*) json_object_get_string(val));
				docker_volume_labels_add(vi, p);
			}
		}

		json_object* status_obj;
		json_object_object_get_ex(response_obj, "Status", &status_obj);
		if (status_obj != NULL) {
			json_object_object_foreach(status_obj, key1, val1)
			{
				pair* p;
				make_pair(&p, key1, (char*) json_object_get_string(val1));
				docker_volume_status_add(vi, p);
			}
		}
		(*volume) = vi;
	}

	return E_SUCCESS;
}

/**
 * Delete the given volume (identified by name).
 *
 * \param ctx the docker context
 * \param result the result object to return
 * \param name name of the volume to delete
 * \param force force delete if this value is 1
 * \return error code
 */
error_t docker_volume_delete(docker_context* ctx, docker_result** result,
		const char* name, int force) {
	char* url = create_service_url_id_method(VOLUME, NULL, name);

	struct array_list* params = array_list_new(
			(void (*)(void *)) &free_url_param);
	url_param* p;
	if (force == 1) {
		make_url_param(&p, "force", make_defensive_copy("true"));
		array_list_add(params, p);
	}

	json_object *response_obj = NULL;
	struct http_response_memory chunk;
	docker_api_delete(ctx, result, url, params, &chunk, &response_obj);

	return E_SUCCESS;
}

/**
 * Delete unused volumes.
 *
 * \param ctx the docker context
 * \param result the result object to return
 * \param volumes_deleted array_list with names of volumes deleted
 * \param space_reclaimed num bytes freed.
 * \param num_label_filters how many label filters are there
 * \param varargs triples (int filter_not, char* label_name, char* label_value)
 * \return error code
 */
error_t docker_volumes_delete_unused(docker_context* ctx,
		docker_result** result, struct array_list** volumes_deleted,
		unsigned long* space_reclaimed, int num_label_filters, ...) {
	char* url = create_service_url_id_method(VOLUME, NULL, "prune");

	struct array_list* params = array_list_new(
			(void (*)(void *)) &free_url_param);

	url_param* p;

	va_list kvargs;
	va_start(kvargs, num_label_filters);
	json_object* filters = make_filters();
	for (int i = 0; i < num_label_filters; i++) {
		int filter_not = va_arg(kvargs, int);
		char* label_name = va_arg(kvargs, char*);
		char* label_value = va_arg(kvargs, char*);

		char* filter_value;
		if (label_value != NULL) {
			filter_value = (char*) calloc(
					strlen(label_name) + strlen(label_value) + 2, sizeof(char));
			strcpy(filter_value, label_name);
			strcat(filter_value, "=");
			strcat(filter_value, label_value);
		} else {
			filter_value = (char*) calloc(strlen(label_name) + 1, sizeof(char));
			strcpy(filter_value, label_name);
		}
		if (filter_not == 1) {
			add_filter_str(filters, "label!", filter_value);
		} else {
			add_filter_str(filters, "label", filter_value);
		}
	}
	make_url_param(&p, "filters", (char*) filters_to_str(filters));
	array_list_add(params, p);

	json_object *response_obj = NULL;
	struct http_response_memory chunk;
	docker_api_post(ctx, result, url, params, "", &chunk, &response_obj);

	if ((*result)->http_error_code == 200) {
		if (response_obj) {
			struct array_list* vols_del = array_list_new(&free);
			json_object* vols_obj;
			json_object_object_get_ex(response_obj, "VolumesDeleted",
					&vols_obj);
			if (vols_obj) {
				int vols_len = json_object_array_length(vols_obj);
				for (int i = 0; i < vols_len; i++) {
					array_list_add(vols_del,
							make_defensive_copy(
									json_object_get_string(
											json_object_array_get_idx(vols_obj,
													i))));
				}
			}
			(*volumes_deleted) = vols_del;
			(*space_reclaimed) = get_attr_unsigned_long(response_obj,
					"SpaceReclaimed");
		}
	}
	return E_SUCCESS;
}