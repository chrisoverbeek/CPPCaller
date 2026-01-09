#include <iostream>
#include <string>
#include "rustlib.h"

// Function that calls the Rust library to format a message
void callRustLibrary() {
    const char* input = "Hello from C++!";
    
    // Call the Rust library function
    char* result = format_message_ffi(input);
    
    if (result != nullptr) {
        std::cout << "Formatted message: " << result << std::endl;
        
        // Free the string allocated by Rust
        free_string(result);
    } else {
        std::cerr << "Error: Failed to format message" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "CPPCaller - C++ Command Line Tool" << std::endl;
    std::cout << "===================================" << std::endl;
    
    if (argc > 1) {
        std::cout << "Arguments received: ";
        for (int i = 1; i < argc; ++i) {
            std::cout << argv[i];
            if (i < argc - 1) std::cout << " ";
        }
        std::cout << std::endl;
    }
    
    // Call the placeholder function
    callRustLibrary();
    
    return 0;
}
