#pragma once
#include "duktape.h""
#include <string>

// Provides a wrapper for the context, as well as some helper functions
class ScriptManager
{
	duk_context* ctx;

	bool scriptLoaded;

public:

	// Dynamically allocates the context
	ScriptManager();

	// Since we are dynamically allocating memory in the constructor, we should deallocate it in the destructor.
	~ScriptManager();

	// Pursuant to Rule of Three, a copy constructor is explicitly defined
	ScriptManager(const ScriptManager& obj);

	// Pursuant to Rule of Three, a copy operator is explicitly defined
	ScriptManager& operator=(const ScriptManager& obj);

	// Returns a the context being wrapper around
	duk_context* getContext();

	// Reads in the file with the given name, and puts its contents on the stack
	void loadScript(const std::string fileName);

	// Defines which function from the loaded file is to be run, how many arguments it takes, and what those arguments are
	void runScript(std::string functionName, int type, int arg_count = 0, ...);
};
