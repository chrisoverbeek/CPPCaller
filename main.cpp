#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "sqlite3.h"
#include "rustlib.h"

// Function to get current timestamp as string
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Function that calls the Rust library and writes to SQLite DB
void callRustLibrary() {
    const char* input = "Hello from C++!";
    
    // Call the Rust library function
    char* result = format_message_ffi(input);
    
    if (result == nullptr) {
        std::cerr << "Error: Failed to format message" << std::endl;
        return;
    }
    
    std::cout << "Formatted message: " << result << std::endl;
    
    // Open/create SQLite database
    sqlite3* db;
    int rc = sqlite3_open("messages.db", &db);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        free_string(result);
        return;
    }
    
    // Create table if it doesn't exist
    const char* createTableSQL = 
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "message TEXT NOT NULL, "
        "timestamp TEXT NOT NULL)";
    
    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Error creating table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        free_string(result);
        return;
    }
    
    // Insert the formatted message with timestamp
    std::string timestamp = getCurrentTimestamp();
    sqlite3_stmt* stmt;
    const char* insertSQL = "INSERT INTO messages (message, timestamp) VALUES (?, ?)";
    
    rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        free_string(result);
        return;
    }
    
    sqlite3_bind_text(stmt, 1, result, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Error inserting data: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Successfully saved to database with timestamp: " << timestamp << std::endl;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    // Free the string allocated by Rust
    free_string(result);
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
