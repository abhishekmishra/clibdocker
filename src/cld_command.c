/*
 * clibdocker: cld_command.c
 * Created on: 06-Feb-2019
 *
 * clibdocker
 * Copyright (C) 2018 Abhishek Mishra <abhishekmishra3@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include "cld_command.h"

/**
 * Create a new value object of given type.
 *
 * \param val object to create
 * \param type
 * \return error code
 */
cld_cmd_err make_cld_val(cld_val** val, cld_type type) {
	(*val) = (cld_val*) calloc(1, sizeof(cld_val));
	if ((*val) == NULL) {
		return CLD_COMMAND_ERR_ALLOC_FAILED;
	}
	(*val)->type = type;
	clear_cld_val(*val);
	return CLD_COMMAND_SUCCESS;
}

/**
 * Free the created value
 */
void free_cld_val(cld_val* val) {
	if (val->str_value) {
		free(val->str_value);
	}
	free(val);
}

/**
 * Reset values to system defaults.
 */
void clear_cld_val(cld_val* val) {
	val->bool_value = 0;
	val->int_value = 0;
	val->dbl_value = 0.0;
	val->str_value = NULL;
}

/**
 * Parse the input and read the value of the type of the val object.
 * (Should not be called when the values is a flag.)
 * The value should be set as soon as the argument/option is seen
 *
 * \param val object whose value will be set
 * \param input string input
 * \return error code
 */
cld_cmd_err parse_cld_val(cld_val* val, char* input) {
	if (input == NULL) {
		return CLD_COMMAND_ERR_UNKNOWN;
	} else {
		int v;
		double d;
		switch (val->type) {
		case CLD_TYPE_BOOLEAN:
			sscanf(input, "%d", &v);
			if (v > 0) {
				val->bool_value = 1;
			} else {
				val->bool_value = 0;
			}
			break;
		case CLD_TYPE_INT:
			sscanf(input, "%d", &v);
			val->int_value = v;
			break;
		case CLD_TYPE_DOUBLE:
			sscanf(input, "%g", &d);
			val->dbl_value = d;
			break;
		case CLD_TYPE_STRING:
			val->str_value = input;
			break;
		default:
			return CLD_COMMAND_ERR_UNKNOWN;
		}
		return CLD_COMMAND_SUCCESS;
	}
}

/**
 * Create a new option given a name and type.
 *
 * \param option object to create
 * \param name
 * \param short_name
 * \param type
 * \param description
 * \return error code
 */
cld_cmd_err make_option(cld_option** option, char* name, char* short_name,
		cld_type type, char* description) {
	(*option) = (cld_option*) calloc(1, sizeof(cld_option));
	if ((*option) == NULL) {
		return CLD_COMMAND_ERR_ALLOC_FAILED;
	}
	(*option)->name = name;
	(*option)->short_name = short_name;
	(*option)->description = description;
	make_cld_val(&((*option)->val), type);
	return CLD_COMMAND_SUCCESS;
}

/**
 * Free resources used by option
 */
void free_option(cld_option* option) {
	if (option->short_name) {
		free(option->short_name);
	}
	if (option->description) {
		free(option->description);
	}

	free(option->val);
	free(option->name);
	free(option);
}

/**
 * Create a new argument given a name and type.
 *
 * \param argument object to create
 * \param name
 * \param type
 * \param description
 * \return error code
 */
cld_cmd_err make_argument(cld_argument** arg, char* name, cld_type type,
		char* description) {
	(*arg) = (cld_argument*) calloc(1, sizeof(cld_argument));
	if ((*arg) == NULL) {
		return CLD_COMMAND_ERR_ALLOC_FAILED;
	}
	(*arg)->name = name;
	(*arg)->description = description;
	(*arg)->optional = 0;
	make_cld_val(&((*arg)->val), type);
	return CLD_COMMAND_SUCCESS;
}

/**
 * Free resources used by argument
 */
void free_argument(cld_argument* arg) {
	if (arg->description) {
		free(arg->description);
	}
	free(arg->val);
	free(arg->name);
	free(arg);
}

/**
 * Create a new command with the given name and handler
 * Options and sub-commands need to be added after creation.
 * The sub-commands and options lists will be initialized,
 * so one just needs to add items using the arraylist add function.
 *
 * \param command obj to be created
 * \param name
 * \param short_name
 * \param description
 * \param handler function ptr to handler
 * \return error code
 */
cld_cmd_err make_command(cld_command** command, char* name, char* short_name,
		char* description, cld_command_handler handler) {
	(*command) = (cld_command*) calloc(1, sizeof(cld_command));
	if ((*command) == NULL) {
		return CLD_COMMAND_ERR_ALLOC_FAILED;
	}
	(*command)->name = name;
	(*command)->short_name = short_name;
	(*command)->description = description;
	(*command)->handler = handler;
	(*command)->options = array_list_new(&free_option);
	(*command)->sub_commands = array_list_new(&free_command);
	(*command)->args = array_list_new(&free_argument);
	return CLD_COMMAND_SUCCESS;
}

/**
 * Free a command object
 */
void free_command(cld_command* command) {
	if (command->short_name) {
		free(command->short_name);
	}
	if (command->description) {
		free(command->description);
	}
	free(command->name);
	array_list_free(command->options);
	array_list_free(command->sub_commands);
	array_list_free(command->args);
	free(command);
}

/**
 * Run the help command for all commands or single command
 */
cld_cmd_err help_cmd_handler(void* handler_args, struct array_list* options,
		struct array_list* args, cld_command_output_handler success_handler,
		cld_command_output_handler error_handler) {
	struct array_list* commands = (struct array_list*) handler_args;
	char* help_str;
	cld_cmd_err e = get_help_for(&help_str, commands, args);
	if (e == CLD_COMMAND_SUCCESS) {
		(*success_handler)(help_str, e);
	} else {
		(*error_handler)("Error getting help", e);
	}
	return e;
}

/**
 * Get the help string for the arg_commands from the registered commands list.
 * \param help_str the help string to return
 * \param commands is the configured list of commands
 * \param arg_commands is a list of string
 * \return error code
 */
cld_cmd_err get_help_for(char** help_str, struct array_list* commands,
		struct array_list* arg_commands) {
	struct array_list* cmd_list = commands;
	for (int i = 0; i < array_list_length(arg_commands); i++) {
		if (cmd_list != NULL) {
			char* cmd_name = array_list_get_idx(arg_commands, i);
			int found_cmd = 0;
			for (int j = 0; j < array_list_length(cmd_list); j++) {
				cld_command* cmd = array_list_get_idx(cmd_list, j);
				if (strcmp(cmd_name, cmd->name) == 0) {
					found_cmd = 1;
					cmd_list = cmd->sub_commands;
					(*help_str) = cmd->description;
					break;
				}
			}
			if (found_cmd == 0) {
				return CLD_COMMAND_ERR_COMMAND_NOT_FOUND;
			}
		} else {
			return CLD_COMMAND_ERR_COMMAND_NOT_FOUND;
		}
	}
	return CLD_COMMAND_ERR_UNKNOWN;
}

int gobble(int argc, char** argv, int at_pos) {
	if (at_pos > (argc - 1)) {
		return argc;
	} else {
		for (int i = at_pos; i < argc; i++) {
			if (i == argc) {
				argv[i] = NULL;
			} else {
				argv[i] = argv[i + 1];
			}
		}
		return argc - 1;
	}
}

cld_cmd_err parse_args(struct array_list* args, int* argc, char*** argv) {
	int ac = (*argc);
	char** av = (*argv);
	return CLD_COMMAND_SUCCESS;
}

/**
 * Execute a single line containing one top-level command.
 * All output is written to stdout, all errors to stderr
 *
 * \param commands the list of commands registered (this is a list of cld_command*)
 * \param argc the number of tokens in the line
 * \param argv args as an array of strings
 */
cld_cmd_err exec_command(struct array_list* commands, int argc, char** argv) {
	//First read all commands
	struct array_list* cmd_names = array_list_new(&free);
	cld_command* cmd_to_exec = NULL;
	struct array_list* cmd_list = commands;
	for (int i = 0; i < argc; i++) {
		char* cmd_name = argv[i];
		int found = 0;
		if (cmd_list != NULL) {
			for (int j = 0; j < array_list_length(cmd_list); j++) {
				cld_command* cmd = (cld_command*) array_list_get_idx(cmd_list,
						j);
				if (strcmp(cmd_name, cmd->name) == 0) {
					found = 1;
					cmd_list = cmd->sub_commands;
					array_list_add(cmd_names, cmd_name);
					cmd_to_exec = cmd;
					break;
				}
			}
		}
		//if current name is not a command break
		if (found == 0) {
			break;
		}
	}

	if (cmd_to_exec == NULL) {
		printf("No valid command found. Type help to get more help\n");
		return CLD_COMMAND_ERR_COMMAND_NOT_FOUND;
	}

	for (int i = 0; i < array_list_length(cmd_names); i++) {
		argc = gobble(argc, argv, 0);
	}

	//Then read all options

	//Now read all arguments

	return CLD_COMMAND_ERR_UNKNOWN;
}
