#ifndef _EXECINFO_H_
#define _EXECINFO_H_

/* Include required system headers */
#include <stddef.h>  /* for size_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Library version information */
#define EXECINFO_VERSION_MAJOR    1
#define EXECINFO_VERSION_MINOR    1  
#define EXECINFO_VERSION_PATCH    0
#define EXECINFO_VERSION_STRING   "1.1.0"

/* Maximum practical stack depth */
#define EXECINFO_MAX_FRAMES       128

/* Compiler feature detection */
#ifndef __GNUC_PREREQ
# if defined __GNUC__ && defined __GNUC_MINOR__
#  define __GNUC_PREREQ(maj, min) \
    ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
# else
#  define __GNUC_PREREQ(maj, min) 0
# endif
#endif

#ifndef __has_attribute
# define __has_attribute(x) 0
#endif

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

/* Compatibility macros */
#ifndef __THROW
# ifdef __cplusplus
#  define __THROW throw()
# else
#  define __THROW
# endif
#endif

#ifndef __nonnull
# if __GNUC_PREREQ(3,3) || __has_attribute(__nonnull__)
#  define __nonnull(params) __attribute__((__nonnull__ params))
# else
#  define __nonnull(params)
# endif
#endif

#ifndef __wur
# if __GNUC_PREREQ(3,4) || __has_attribute(__warn_unused_result__)
#  define __wur __attribute__((__warn_unused_result__))
# else
#  define __wur
# endif
#endif

#ifndef __pure
# if __GNUC_PREREQ(2,96) || __has_attribute(__pure__)
#  define __pure __attribute__((__pure__))
# else
#  define __pure
# endif
#endif

/**
 * Store up to SIZE return addresses of the current program state in ARRAY
 * and return the exact number of values stored.
 * 
 * This function walks the call stack and collects return addresses from
 * each stack frame. The addresses can be used with addr2line or similar
 * tools to get symbolic information.
 * 
 * @param buffer Array to store the return addresses
 * @param size Maximum number of addresses to store
 * @return Number of addresses actually stored, or 0 on error
 * 
 * @note The first address is typically the caller of backtrace(),
 *       the second is the caller's caller, etc.
 * 
 * Example:
 * @code
 * void *buffer[10];
 * int count = backtrace(buffer, 10);
 * printf("Got %d stack frames\n", count);
 * @endcode
 */
int backtrace(void **buffer, int size) __THROW __nonnull((1)) __wur;

/**
 * Return an array of strings describing the addresses in ARRAY.
 * 
 * This function takes the return addresses from backtrace() and converts
 * them into an array of strings containing symbolic information such as
 * function names, offsets, and filenames when available.
 * 
 * @param buffer Array of return addresses from backtrace()
 * @param size Number of addresses in the array
 * @return Array of string pointers, or NULL on error
 * 
 * @note The returned array must be freed with free(), but the individual
 *       strings should NOT be freed separately as they are part of the
 *       same memory allocation.
 * 
 * @note If symbol information is not available, addresses will be shown
 *       in hexadecimal format.
 * 
 * Example:
 * @code
 * void *buffer[10];
 * int count = backtrace(buffer, 10);
 * char **strings = backtrace_symbols(buffer, count);
 * if (strings) {
 *     for (int i = 0; i < count; i++)
 *         printf("%s\n", strings[i]);
 *     free(strings);
 * }
 * @endcode
 */
char **backtrace_symbols(void *const *buffer, int size) __THROW __nonnull((1)) __wur;

/**
 * Write symbolic backtrace information directly to a file descriptor.
 * 
 * This function is similar to backtrace_symbols() but writes the output
 * directly to a file descriptor instead of returning an array of strings.
 * This is useful when memory allocation might fail or when you want to
 * write to stderr or a log file.
 * 
 * @param buffer Array of return addresses from backtrace()
 * @param size Number of addresses in the array  
 * @param fd File descriptor to write to (e.g., STDERR_FILENO)
 * 
 * @note This function does not allocate memory and is safe to use in
 *       signal handlers or low-memory situations.
 * 
 * @note Each line of output is terminated with a newline character.
 * 
 * Example:
 * @code
 * void *buffer[10];
 * int count = backtrace(buffer, 10);
 * backtrace_symbols_fd(buffer, count, STDERR_FILENO);
 * @endcode
 */
void backtrace_symbols_fd(void *const *buffer, int size, int fd) __THROW __nonnull((1));

/* Convenience macros for common usage patterns */

/**
 * Print a backtrace to stderr (convenience macro)
 */
#define PRINT_BACKTRACE() do { \
    void *_bt_buffer[EXECINFO_MAX_FRAMES]; \
    int _bt_size = backtrace(_bt_buffer, EXECINFO_MAX_FRAMES); \
    backtrace_symbols_fd(_bt_buffer, _bt_size, 2); \
} while(0)

/**
 * Get backtrace as string array (convenience macro)
 * @note Remember to free() the returned array
 */
#define GET_BACKTRACE_SYMBOLS(_buffer, _size) \
    backtrace_symbols((_buffer), (_size))

#ifdef __cplusplus
}
#endif

#endif /* _EXECINFO_H_ */