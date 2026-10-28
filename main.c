#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "contact.h"

static void trim_whitespace(char *s) {
    char *start = s;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

static bool read_menu_int(const char *prompt, int *value) {
    char line[64];
    printf("%s", prompt);
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("\nInput closed. Exiting.\n");
        exit(EXIT_SUCCESS);
    }
    trim_whitespace(line);

    char *end = NULL;
    long parsed = strtol(line, &end, 10);
    if (line[0] == '\0' || end == line || *end != '\0') {
        return false;
    }
    *value = (int)parsed;
    return true;
}

int main(void) {
    int choice = 0;
    AddressBook addressBook;
    initialize(&addressBook);

    do {
        printf("\nAddress Book Menu:\n");
        printf("1. Create contact\n");
        printf("2. Search contact\n");
        printf("3. Edit contact\n");
        printf("4. Delete contact\n");
        printf("5. List all contacts\n");
        printf("6. Save and Exit\n");
        printf("0. Exit without saving\n");

        if (!read_menu_int("Enter your choice: ", &choice)) {
            printf("Invalid choice. Please try again.\n");
            continue;
        }

        switch (choice) {
            case 1:
                createContact(&addressBook);
                break;
            case 2:
                searchContact(&addressBook);
                break;
            case 3:
                editContact(&addressBook);
                break;
            case 4:
                deleteContact(&addressBook);
                break;
            case 5:
                listContacts(&addressBook);
                break;
            case 6:
                saveAndExit(&addressBook);
                break;
            case 0:
                printf("Exiting without saving.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0 && choice != 6);

    return 0;
}