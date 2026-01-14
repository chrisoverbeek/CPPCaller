#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <memory>
#include "sqlite3.h"

#pragma comment(lib, "wintrust.lib")

// Function pointer types for the Rust library
typedef char* (*FormatMessageFFI)(const char*);
typedef void (*FreeString)(char*);

// Enum to track library loading failure reasons
enum class LoadFailureReason {
    SUCCESS,
    FILE_NOT_FOUND,
    SIGNATURE_VERIFICATION_FAILED,
    LOAD_LIBRARY_FAILED,
    FUNCTION_LOADING_FAILED
};

// Structure to hold dynamically loaded library functions
struct RustLibrary {
    HMODULE handle = nullptr;
    FormatMessageFFI format_message_ffi = nullptr;
    FreeString free_string = nullptr;
    
    ~RustLibrary() {
        if (handle) {
            FreeLibrary(handle);
        }
    }
};

// Structure to hold library loading result
struct LibraryLoadResult {
    std::unique_ptr<RustLibrary> library;
    LoadFailureReason failureReason = LoadFailureReason::SUCCESS;
};

// Function to verify digital signature of a DLL
bool verifySignature(const wchar_t* filePath) {
    WINTRUST_FILE_INFO fileInfo = {};
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = filePath;
    fileInfo.hFile = nullptr;
    fileInfo.pgKnownSubject = nullptr;

    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    
    WINTRUST_DATA wintrustData = {};
    wintrustData.cbStruct = sizeof(WINTRUST_DATA);
    wintrustData.pPolicyCallbackData = nullptr;
    wintrustData.pSIPClientData = nullptr;
    wintrustData.dwUIChoice = WTD_UI_NONE;
    wintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    wintrustData.dwUnionChoice = WTD_CHOICE_FILE;
    wintrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    wintrustData.hWVTStateData = nullptr;
    wintrustData.pwszURLReference = nullptr;
    wintrustData.dwProvFlags = WTD_SAFER_FLAG;
    wintrustData.dwUIContext = 0;
    wintrustData.pFile = &fileInfo;

    LONG status = WinVerifyTrust(nullptr, &policyGUID, &wintrustData);
    
    // Close the trust data
    wintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(nullptr, &policyGUID, &wintrustData);
    
    return status == ERROR_SUCCESS;
}

// Function to dynamically load the Rust library
LibraryLoadResult loadRustLibrary(const std::wstring& dllPath) {
    LibraryLoadResult result;
    auto lib = std::make_unique<RustLibrary>();
    
    // Check if file exists
    DWORD fileAttrib = GetFileAttributesW(dllPath.c_str());
    if (fileAttrib == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"Library not found: " << dllPath << std::endl;
        result.failureReason = LoadFailureReason::FILE_NOT_FOUND;
        return result;
    }
    
    // Verify digital signature
    std::wcout << L"Verifying signature of: " << dllPath << std::endl;
    if (!verifySignature(dllPath.c_str())) {
        std::wcerr << L"Error: Library signature verification failed!" << std::endl;
        std::wcerr << L"The library is either not signed or has an invalid signature." << std::endl;
        result.failureReason = LoadFailureReason::SIGNATURE_VERIFICATION_FAILED;
        return result;
    }
    
    std::wcout << L"Signature verified successfully." << std::endl;
    
    // Load the library
    lib->handle = LoadLibraryW(dllPath.c_str());
    if (!lib->handle) {
        std::cerr << "Error: Failed to load library. Error code: " << GetLastError() << std::endl;
        result.failureReason = LoadFailureReason::LOAD_LIBRARY_FAILED;
        return result;
    }
    
    // Get function pointers
    lib->format_message_ffi = reinterpret_cast<FormatMessageFFI>(
        GetProcAddress(lib->handle, "format_message_ffi")
    );
    lib->free_string = reinterpret_cast<FreeString>(
        GetProcAddress(lib->handle, "free_string")
    );
    
    if (!lib->format_message_ffi || !lib->free_string) {
        std::cerr << "Error: Failed to load required functions from library" << std::endl;
        result.failureReason = LoadFailureReason::FUNCTION_LOADING_FAILED;
        return result;
    }
    
    std::cout << "Library loaded successfully." << std::endl;
    result.library = std::move(lib);
    result.failureReason = LoadFailureReason::SUCCESS;
    return result;
}

// Function to get current timestamp as string
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Function that calls the Rust library and writes to SQLite DB
void callRustLibrary(RustLibrary* lib) {
    if (!lib) {
        std::cerr << "Error: Library not loaded" << std::endl;
        return;
    }
    
    const char* input = "Hello from C++!";
    
    // Call the Rust library function
    char* result = lib->format_message_ffi(input);
    
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
        lib->free_string(result);
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
        lib->free_string(result);
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
        lib->free_string(result);
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
    lib->free_string(result);
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
    
    // Attempt to load the Rust library dynamically
    // Try common library names
    std::vector<std::wstring> possiblePaths = {
        L"rustlib.dll",
        L"librustlib.dll",
        L".\\rustlib.dll",
        L".\\target\\release\\rustlib.dll",
        L".\\target\\debug\\rustlib.dll"
    };
    
    std::unique_ptr<RustLibrary> rustLib = nullptr;
    LoadFailureReason lastFailureReason = LoadFailureReason::FILE_NOT_FOUND;
    for (const auto& path : possiblePaths) {
        std::wcout << L"Attempting to load: " << path << std::endl;
        LibraryLoadResult result = loadRustLibrary(path);
        if (result.library) {
            rustLib = std::move(result.library);
            break;
        }
        // Track the last non-file-not-found error for better error reporting
        if (result.failureReason != LoadFailureReason::FILE_NOT_FOUND) {
            lastFailureReason = result.failureReason;
        }
    }
    
    if (rustLib) {
        // Call the library function
        callRustLibrary(rustLib.get());
    } else {
        // Provide specific error message based on the failure reason
        std::cout << "\nNote: Unable to load Rust library." << std::endl;
        switch (lastFailureReason) {
            case LoadFailureReason::FILE_NOT_FOUND:
                std::cout << "Reason: Library file not found in any of the expected locations." << std::endl;
                break;
            case LoadFailureReason::SIGNATURE_VERIFICATION_FAILED:
                std::cout << "Reason: Library signature verification failed." << std::endl;
                std::cout << "The library is either not signed or has an invalid signature." << std::endl;
                break;
            case LoadFailureReason::LOAD_LIBRARY_FAILED:
                std::cout << "Reason: Failed to load the library (LoadLibrary failed)." << std::endl;
                break;
            case LoadFailureReason::FUNCTION_LOADING_FAILED:
                std::cout << "Reason: Library loaded but required functions could not be found." << std::endl;
                break;
            default:
                std::cout << "Reason: Unknown error." << std::endl;
                break;
        }
        std::cout << "Continuing without Rust library functionality." << std::endl;
    }
    
    return 0;
}
