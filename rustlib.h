#ifndef RUSTLIB_H
#define RUSTLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Format a message by encapsulating the input string.
 * 
 * @param input - A null-terminated C string to be formatted
 * @return A pointer to a newly allocated null-terminated C string containing
 *         the formatted message, or NULL if an error occurred.
 *         The caller MUST free this string using free_string() when done.
 */
char* format_message_ffi(const char* input);

/**
 * Free a string that was allocated by the Rust library.
 * 
 * @param ptr - A pointer to a string returned by format_message_ffi()
 *              If ptr is NULL, this function does nothing.
 */
void free_string(char* ptr);

#ifdef __cplusplus
}
#endif

#endif // RUSTLIB_H
