#include <sqlite3ext.h> /* Use sqlite3ext.h for extensions */
SQLITE_EXTENSION_INIT1

#include <openssl/evp.h>
#include <string.h>
#include <stdlib.h>

// Base64URL encoding function
void base64url_encode(sqlite3_context *context, int argc, sqlite3_value **argv) {
    if (argc != 1 || sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    const unsigned char *input = sqlite3_value_blob(argv[0]);
    int input_len = sqlite3_value_bytes(argv[0]);

    // Calculate output size for Base64 encoding
    int encoded_len = 4 * ((input_len + 2) / 3);
    char *encoded = malloc(encoded_len + 1); // Allocate memory for encoded output

    if (!encoded) {
        sqlite3_result_error_nomem(context);
        return;
    }

    // Perform Base64 encoding
    int actual_len = EVP_EncodeBlock((unsigned char *)encoded, input, input_len);

    // Convert Base64 to Base64URL (replace + with -, / with _)
    for (int i = 0; i < actual_len; i++) {
        if (encoded[i] == '+') encoded[i] = '-';
        else if (encoded[i] == '/') encoded[i] = '_';
    }

    // Remove padding characters '='
    while (actual_len > 0 && encoded[actual_len - 1] == '=') {
        actual_len--;
    }
    encoded[actual_len] = '\0'; // Null-terminate the result

    sqlite3_result_text(context, encoded, -1, free); // Return the encoded string
}

// Base64URL decoding function
void base64url_decode(sqlite3_context *context, int argc, sqlite3_value **argv) {
    if (argc != 1 || sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }

    const char *input = (const char *)sqlite3_value_text(argv[0]);
    int input_len = strlen(input);

    // Convert Base64URL characters back to Base64 characters
    char *modified_input = malloc(input_len + 1);
    if (!modified_input) {
        sqlite3_result_error_nomem(context);
        return;
    }

    for (int i = 0; i < input_len; i++) {
        if (input[i] == '-') {
            modified_input[i] = '+';
        } else if (input[i] == '_') {
            modified_input[i] = '/';
        } else {
            modified_input[i] = input[i];
        }
    }
    modified_input[input_len] = '\0';

    // Add padding if necessary
    int pad_len = input_len % 4 == 0 ? 0 : 4 - (input_len % 4);
    modified_input = realloc(modified_input, input_len + pad_len + 1);
    for (int i = 0; i < pad_len; i++) {
        modified_input[input_len + i] = '=';
    }
    modified_input[input_len + pad_len] = '\0';

    // Decode Base64
    int max_decoded_len = 3 * (input_len / 4);
    unsigned char *decoded = malloc(max_decoded_len);

    if (!decoded) {
        free(modified_input);
        sqlite3_result_error_nomem(context);
        return;
    }

    int actual_len = EVP_DecodeBlock(decoded, (const unsigned char *)modified_input, input_len + pad_len);

    // Correct the actual length to exclude padding bytes
    while (actual_len > 0 && decoded[actual_len - 1] == '\0') {
        actual_len--;
    }

    sqlite3_result_blob(context, decoded, actual_len, free);

    free(modified_input);
}

// Function to register the extension
int sqlite3_base64url_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi) {
    SQLITE_EXTENSION_INIT2(pApi);

    // Register base64url_encode function
    int rc = sqlite3_create_function_v2(
        db,
        "base64url_encode",
        1,
        SQLITE_UTF8 | SQLITE_DETERMINISTIC,
        NULL,
        base64url_encode,
        NULL,
        NULL,
        NULL
    );
    if (rc != SQLITE_OK) return rc;

    // Register base64url_decode function
    rc = sqlite3_create_function_v2(
        db,
        "base64url_decode",
        1,
        SQLITE_UTF8 | SQLITE_DETERMINISTIC,
        NULL,
        base64url_decode,
        NULL,
        NULL,
        NULL
    );
    if (rc != SQLITE_OK) return rc;

    // Auto-register for subsequent database connections
    sqlite3_auto_extension((void (*)(void))sqlite3_base64url_init);

    return SQLITE_OK_LOAD_PERMANENTLY;
}
