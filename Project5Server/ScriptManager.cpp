#include "ScriptManager.h"
#include <iostream>
#include "dukglue/dukglue.h"
#include <fstream>
#include <cstdarg>

// Constructor that dynamically allocates
ScriptManager::ScriptManager()
{
	this->ctx = duk_create_heap_default();
	this->scriptLoaded = false;
}

ScriptManager::~ScriptManager()
{
	duk_destroy_heap(this->ctx);
}

ScriptManager::ScriptManager(const ScriptManager& obj)
{
	std::cout << "ERROR: Cannot copy construct ScriptManager objects!" << std::endl;
	this->ctx = duk_create_heap_default();
	this->scriptLoaded = obj.scriptLoaded;
}

ScriptManager& ScriptManager::operator=(const ScriptManager& obj)
{
	std::cout << "ERROR: Cannot copy assign ScriptManager objects!" << std::endl;
	this->scriptLoaded = obj.scriptLoaded;
	return *this;
}

duk_context* ScriptManager::getContext()
{
	return this->ctx;
}

void ScriptManager::loadScript(const std::string fileName)
{
	// This piece of code simply reads the entire contents of the file and puts it in a string
	std::ifstream script_file = std::ifstream(fileName);
	std::stringstream script_contents_stream;
	script_contents_stream << script_file.rdbuf();
	std::string script_contents = script_contents_stream.str();
	// This is to get rid of the previous script contents if one was already loaded
	if (this->scriptLoaded)
	{
		duk_pop(ctx);
	}
	// This pushes the contents of the file onto the stack
	duk_push_lstring(ctx, script_contents.c_str(), (duk_size_t)script_contents.length());
	// This ensures the contents was pushed correctly
	if (duk_peval(ctx) != 0) {
		printf("Error: %s\n", duk_safe_to_string(ctx, -1));
		exit(1);
	}
	// Clears the stack after the previous call
	duk_pop(ctx);
	duk_push_global_object(ctx);
	this->scriptLoaded = true;

}

/**
* Type 0 = String Arguments 
* Type 1 = Int Arguments
*/
void ScriptManager::runScript(std::string functionName, int type, int arg_count, ...)
{
	if (this->scriptLoaded)
	{
		// This indicates the name of the function within the script file that you wish to run
		duk_get_prop_string(ctx, -1, functionName.c_str());
		// This pushes all the passed in arguments onto the stack
		va_list args;
		va_start(args, arg_count);

		if (type == 0) {
			
		}
		for (int i = 0; i < arg_count; i++)
		{
			if (type == 0) {
				dukglue_push(this->ctx, va_arg(args, const char*));
			}
			if (type == 1) {
				dukglue_push(this->ctx, va_arg(args, int));
			}

		}
		
		va_end(args);

		//If you want your script function to return non-string values, or operate on/do something with return values, you will have to modify this
		if (duk_pcall(ctx, arg_count) != 0)
		{
			std::cout << "Error: ";
		}
		// std::string = duk_safe_to_string(ctx, -1);
		// std::cout << duk_safe_to_string(ctx, -1) << std::endl;
		duk_pop(this->ctx);
	}
	else
	{
		std::cout << "ERROR: Cannot run script unless script is loaded, function is chosen, and arguments are given" << std::endl;
		exit(1);
	}

}