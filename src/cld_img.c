#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cld_img.h"
#include "docker_all.h"
#include "cld_progress.h"

typedef struct {
	cld_command_output_handler success_handler;
	cld_multi_progress* multi_progress;
} docker_pull_update_args;

void log_pull_message(docker_image_create_status* status, void* client_cbargs) {
	docker_pull_update_args* upd_args = (docker_pull_update_args*) client_cbargs;
	if (status) {
		if (status->id) {
			int len = array_list_length(upd_args->multi_progress->progress_ls);
			int new_len = len;
			int found = 0;
			int loc = -1;
			for (int i = 0; i < len; i++) {
				cld_progress* p = (cld_progress*) array_list_get_idx(
						upd_args->multi_progress->progress_ls, i);
				if (strcmp(status->id, p->name) == 0) {
					found = 1;
					loc = i;
				}
			}
			if (found == 0) {
				cld_progress* p;
				if (create_cld_progress(&p, status->id, 0, 0) == 0) {
					array_list_add(upd_args->multi_progress->progress_ls, p);
					upd_args->multi_progress->old_count = array_list_length(
							upd_args->multi_progress->progress_ls) - 1;
					new_len += 1;
					p->message = status->status;
					if (status->progress != NULL) {
						p->extra = status->progress;
						p->current = status->progress_detail->current;
						p->total = status->progress_detail->total;
					} else {
						p->extra = NULL;
					}
				}
			} else {
				cld_progress* p = (cld_progress*) array_list_get_idx(
						upd_args->multi_progress->progress_ls, loc);
				upd_args->multi_progress->old_count = array_list_length(
						upd_args->multi_progress->progress_ls);
				p->message = status->status;
				if (status->progress != NULL) {
					p->extra = status->progress;
					p->current = status->progress_detail->current;
					p->total = status->progress_detail->total;
				} else {
					p->extra = NULL;
				}
			}
			upd_args->success_handler(CLD_COMMAND_IS_RUNNING,
					CLD_RESULT_PROGRESS, upd_args->multi_progress);
		} else {
			upd_args->success_handler(CLD_COMMAND_IS_RUNNING, CLD_RESULT_STRING,
					status->status);
		}
	}
}

cld_cmd_err img_pl_cmd_handler(void *handler_args, struct array_list *options,
		struct array_list *args, cld_command_output_handler success_handler,
		cld_command_output_handler error_handler) {
	int quiet = 0;
	docker_result *res;
	docker_context *ctx = get_docker_context(handler_args);

	docker_pull_update_args* upd_args = (docker_pull_update_args*) calloc(1,
			sizeof(docker_pull_update_args));
	upd_args->success_handler = success_handler;
	create_cld_multi_progress(&(upd_args->multi_progress));

	int len = array_list_length(args);
	if (len != 1) {
		error_handler(CLD_COMMAND_ERR_UNKNOWN, CLD_RESULT_STRING,
				"Image name not provided.");
		return CLD_COMMAND_ERR_UNKNOWN;
	} else {
		cld_argument* image_name_arg = (cld_argument*) array_list_get_idx(args,
				0);
		char* image_name = image_name_arg->val->str_value;
		d_err_t docker_error = docker_image_create_from_image_cb(ctx, &res,
				&log_pull_message, upd_args, image_name, NULL, NULL);
		handle_docker_error(res);
		if (docker_error == E_SUCCESS) {
			char* res_str = (char*) calloc(1024, sizeof(char));
			sprintf(res_str, "Image pull successful -> %s", image_name);
			success_handler(CLD_COMMAND_SUCCESS, CLD_RESULT_STRING, res_str);
			free(res_str);
			return CLD_COMMAND_SUCCESS;
		} else {
			return CLD_COMMAND_ERR_UNKNOWN;
		}
	}
	free_cld_multi_progress(upd_args->multi_progress);
}

cld_command *img_commands() {
	cld_command *image_command;
	if (make_command(&image_command, "image", "img", "Docker Image Commands",
	NULL) == CLD_COMMAND_SUCCESS) {
		cld_command *imgpl_command;
		if (make_command(&imgpl_command, "pull", "pl", "Docker Image Pull",
				&img_pl_cmd_handler) == CLD_COMMAND_SUCCESS) {
			cld_argument* image_name_arg;
			make_argument(&image_name_arg, "Image Name", CLD_TYPE_STRING,
					"Name of Docker Image to be pulled.");
			array_list_add(imgpl_command->args, image_name_arg);

			array_list_add(image_command->sub_commands, imgpl_command);
		}
	}
	return image_command;
}
