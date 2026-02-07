// Learning Objective:
// This tutorial demonstrates how to implement a basic plugin architecture using dynamic
// libraries (DLLs on Windows, Shared Objects/SOs on Unix-like systems). You will learn
// how to dynamically load a library at runtime, retrieve a function pointer from it,
// and execute code provided by the plugin, extending your application's functionality
// without recompiling the main executable.

#include <iostream>  // For console output
#include <string>    // For string manipulation
#include <cstdio>    // For sprintf_s (Windows) or snprintf (Linux)

// Include platform-specific headers for dynamic library loading
#ifdef _WIN32
#include <windows.h> // Required for LoadLibrary, GetProcAddress, FreeLibrary
#else
#include <dlfcn.h>   // Required for dlopen, dlsym, dlclose on Unix-like systems
#endif

// --- Plugin Interface Definition ---
// This section defines the "contract" between the main application (host) and any plugin.
// Both the host and the plugin MUST agree on these function signatures.
// 'extern "C"' is crucial here. It tells the C++ compiler to use C-style name
// mangling for these functions, which makes it much easier to find them by name
// when loading dynamically, as C++ compilers often "mangle" names to include
// type information, making them unique but harder to find externally.

// This typedef defines the signature of a function that the plugin must expose.
// It's a function that takes no arguments and returns a const char* (a C-style string).
typedef const char* (*GetPluginNameFunc)();

// This typedef defines the signature of another function that the plugin might expose.
// It's a function that takes an integer and returns an integer.
typedef int (*PerformOperationFunc)(int a, int b);

// --- Main Application (Host) Code ---
int main() {
    std::cout << "--- Dynamic Plugin Loader ---" << std::endl;

    // 1. Define the plugin library filename.
    // This needs to be platform-specific.
    // On Windows, dynamic libraries are .dll files.
    // On Unix-like systems (Linux, macOS), they are .so (Shared Object) or .dylib (macOS).
    #ifdef _WIN32
    const std::string pluginFilename = "BasicPlugin.dll";
    #else
    const std::string pluginFilename = "./libBasicPlugin.so"; // Assuming it's in the current dir
    #endif

    std::cout << "Attempting to load plugin: " << pluginFilename << std::endl;

    // 2. Load the dynamic library.
    // This function attempts to load the specified library into the application's address space.
    // If successful, it returns a handle to the loaded library; otherwise, it returns NULL.
    #ifdef _WIN32
    HMODULE pluginHandle = LoadLibraryA(pluginFilename.c_str());
    if (!pluginHandle) {
        std::cerr << "Error: Could not load plugin library (code: " << GetLastError() << ")" << std::endl;
        return 1; // Indicate an error
    }
    #else
    void* pluginHandle = dlopen(pluginFilename.c_str(), RTLD_LAZY); // RTLD_LAZY means resolve symbols as needed
    if (!pluginHandle) {
        std::cerr << "Error: Could not load plugin library: " << dlerror() << std::endl;
        return 1; // Indicate an error
    }
    #endif

    std::cout << "Plugin library loaded successfully." << std::endl;

    // 3. Get a pointer to the desired plugin functions.
    // After loading the library, we need to find specific functions within it by their name.
    // GetProcAddress (Windows) or dlsym (Unix-like) takes the library handle and the
    // function name (as a C-string) and returns a raw, untyped function pointer.
    // It is crucial to cast this raw pointer to the correct function signature
    // (e.g., GetPluginNameFunc) defined in our interface.

    // Get "GetPluginName" function
    #ifdef _WIN32
    GetPluginNameFunc getPluginName = (GetPluginNameFunc)GetProcAddress(pluginHandle, "GetPluginName");
    #else
    GetPluginNameFunc getPluginName = (GetPluginNameFunc)dlsym(pluginHandle, "GetPluginName");
    #endif

    if (!getPluginName) {
        std::cerr << "Error: Could not find function 'GetPluginName' in plugin." << std::endl;
        #ifdef _WIN32
        FreeLibrary(pluginHandle); // Clean up
        #else
        dlclose(pluginHandle);     // Clean up
        #endif
        return 1;
    }

    // Get "PerformAddition" function
    #ifdef _WIN32
    PerformOperationFunc performAddition = (PerformOperationFunc)GetProcAddress(pluginHandle, "PerformAddition");
    #else
    PerformOperationFunc performAddition = (PerformOperationFunc)dlsym(pluginHandle, "PerformAddition");
    #endif

    if (!performAddition) {
        std::cerr << "Error: Could not find function 'PerformAddition' in plugin." << std::endl;
        #ifdef _WIN32
        FreeLibrary(pluginHandle); // Clean up
        #else
        dlclose(pluginHandle);     // Clean up
        #endif
        return 1;
    }

    std::cout << "Plugin functions found successfully." << std::endl;

    // 4. Call the plugin functions.
    // Now that we have correctly typed function pointers, we can call them just like
    // any other function in our application. The actual code will be executed from
    // within the dynamically loaded library.
    const char* pluginName = getPluginName();
    std::cout << "Plugin says: Hello from '" << pluginName << "'!" << std::endl;

    int result = performAddition(10, 25);
    std::cout << "Plugin performed 10 + 25 = " << result << std::endl;

    // 5. Unload the dynamic library.
    // It's good practice to unload the library when it's no longer needed to free up
    // system resources. This also allows for potential updates to the plugin file
    // without restarting the main application (though handling updates cleanly is
    // a more advanced topic).
    #ifdef _WIN32
    if (!FreeLibrary(pluginHandle)) {
        std::cerr << "Warning: Could not free plugin library (code: " << GetLastError() << ")" << std::endl;
    }
    #else
    if (dlclose(pluginHandle) != 0) {
        std::cerr << "Warning: Could not close plugin library: " << dlerror() << std::endl;
    }
    #endif

    std::cout << "Plugin library unloaded." << std::endl;
    std::cout << "--- End of Tutorial ---" << std::endl;

    return 0; // Success
}

// --- Example Plugin Implementation (Conceptual - This would be in 'BasicPlugin.cpp') ---
/*
// To compile this plugin:
// On Windows (MSVC):
//   cl /LD BasicPlugin.cpp /FeBasicPlugin.dll
// On Linux (g++):
//   g++ -shared -fPIC BasicPlugin.cpp -o libBasicPlugin.so

#include <iostream>

// Use dllexport for Windows to mark functions for export.
// For Linux/macOS, functions are typically exported by default if not static,
// but specific visibility attributes might be used for fine-grained control.
#ifdef _WIN32
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API
#endif

// Define the plugin's exported functions using 'extern "C"'
// to ensure C-linkage, matching the host's expectation.

extern "C" PLUGIN_API const char* GetPluginName() {
    // This function provides a name for the plugin.
    // It's a simple way for the host to identify the loaded plugin.
    return "Basic Cpp Plugin v1.0";
}

extern "C" PLUGIN_API int PerformAddition(int a, int b) {
    // This function performs a simple operation.
    // It demonstrates passing arguments and returning a value.
    std::cout << "[Plugin Debug]: Performing addition: " << a << " + " << b << std::endl;
    return a + b;
}
*/
// --- Compilation Instructions ---
/*
To run this example, you need two separate compilation steps:

1. Compile the Plugin (BasicPlugin.cpp):
   Create a file named `BasicPlugin.cpp` with the content provided in the "Example Plugin Implementation" section above.

   On Windows (using MSVC compiler from Developer Command Prompt):
     cl /LD BasicPlugin.cpp /FeBasicPlugin.dll

   On Linux/macOS (using g++):
     g++ -shared -fPIC BasicPlugin.cpp -o libBasicPlugin.so

   Make sure the compiled plugin file (BasicPlugin.dll or libBasicPlugin.so) is in the same directory
   as your main executable, or in a location where the system can find dynamic libraries.

2. Compile the Host Application (this file, e.g., main.cpp):

   On Windows (using MSVC compiler):
     cl main.cpp

   On Linux/macOS (using g++):
     g++ main.cpp -o main_app -ldl

   Note: On Linux/macOS, you need to link with `-ldl` because `dlopen`, `dlsym`, `dlclose` are part of the `libdl` library.

After compiling both, run the `main_app` executable. You should see output from both the main application
and the dynamically loaded plugin!
*/