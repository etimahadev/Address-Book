#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "contact.h"
#include "file.h"

#define CANCEL_INPUT "0"

/* ---------- Small input helpers ---------- */

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

static void str_to_lower(char *s) {
    for (; *s; s++) {
        *s = (char)tolower((unsigned char)*s);
    }
}

static bool read_line(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) {
        printf("\nInput closed. Exiting.\n");
        exit(EXIT_SUCCESS);
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return true;
}

static bool read_int(const char *prompt, int *value) {
    char line[64];
    char *end = NULL;

    read_line(prompt, line, sizeof(line));
    trim_whitespace(line);

    if (line[0] == '\0') {
        return false;
    }

    long parsed = strtol(line, &end, 10);
    if (end == line || *end != '\0') {
        return false;
    }

    *value = (int)parsed;
    return true;
}

/* ---------- Validation rules from the PDF ---------- */

static int fail(char *err, size_t err_size, const char *msg) {
    snprintf(err, err_size, "%s", msg);
    return 0;
}

static int validate_name(const char *name, char *err, size_t err_size) {
    size_t len = strlen(name);
    if (len < 4) {
        return fail(err, err_size, "Name must contain at least 4 characters");
    }

    for (size_t i = 0; i < len; i++) {
        if (!isalpha((unsigned char)name[i])) {
            return fail(err, err_size, "Name should contain only alphabets");
        }
    }
    return 1;
}

static int validate_phone(const char *phone, char *err, size_t err_size) {
    size_t len = strlen(phone);
    if (len != 10) {
        return fail(err, err_size, "Phone number must contain exactly 10 digits");
    }

    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)phone[i])) {
            return fail(err, err_size, "Phone number must contain only digits");
        }
    }

    if (phone[0] < '6' || phone[0] > '9') {
        return fail(err, err_size, "First digit must be between 6 and 9");
    }
    return 1;
}

/*
 * Strict educational validation:
 * local@domain.extension
 * - exactly one @
 * - no dot before @
 * - only letters/digits before @
 * - one dot after @
 * - at least one char between @ and .
 * - domain before dot: only letters/digits
 * - extension after dot: only letters (examples reject .com123)
 */
static int validate_email(const char *email, char *err, size_t err_size) {
    const char *at = strchr(email, '@');
    if (at == NULL) {
        return fail(err, err_size, "Email ID must contain exactly one @");
    }
    if (strchr(at + 1, '@') != NULL) {
        return fail(err, err_size, "Multiple or double @ symbols are not allowed");
    }
    if (at == email) {
        return fail(err, err_size, "Email ID must contain at least one character before @");
    }

    for (const char *p = email; p < at; p++) {
        if (*p == '.') {
            return fail(err, err_size, "The . must appear after @, not before @");
        }
        if (!isalnum((unsigned char)*p)) {
            return fail(err, err_size, "Invalid symbol in email ID. Only @ and . are allowed");
        }
    }

    const char *domain = at + 1;
    const char *dot = strchr(domain, '.');
    if (dot == NULL) {
        return fail(err, err_size, "Email domain must contain a . after @");
    }
    if (dot == domain) {
        return fail(err, err_size, "There must be at least one character between @ and .");
    }
    if (strchr(dot + 1, '.') != NULL) {
        return fail(err, err_size, "Only one . is allowed after @");
    }
    if (*(dot + 1) == '\0') {
        return fail(err, err_size, "There must be at least one character after .");
    }

    for (const char *p = domain; p < dot; p++) {
        if (!isalnum((unsigned char)*p)) {
            return fail(err, err_size, "Only alphabets or digits are allowed after @");
        }
    }

    for (const char *p = dot + 1; *p != '\0'; p++) {
        if (!isalpha((unsigned char)*p)) {
            return fail(err, err_size, "Only alphabets are allowed after the final domain dot");
        }
    }
    return 1;
}

/* ---------- Duplicate and search helpers ---------- */

static int find_by_phone(const AddressBook *addressBook, const char *phone, int exclude_index) {
    for (int i = 0; i < addressBook->contactCount; i++) {
        if (i != exclude_index && strcmp(addressBook->contacts[i].phone, phone) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_by_email(const AddressBook *addressBook, const char *email, int exclude_index) {
    for (int i = 0; i < addressBook->contactCount; i++) {
        if (i != exclude_index && strcmp(addressBook->contacts[i].email, email) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_exact_duplicate(const AddressBook *addressBook, const char *name,
                                const char *phone, const char *email, int exclude_index) {
    for (int i = 0; i < addressBook->contactCount; i++) {
        if (i == exclude_index) {
            continue;
        }
        const Contact *c = &addressBook->contacts[i];
        if (strcmp(c->name, name) == 0 && strcmp(c->phone, phone) == 0 &&
            strcmp(c->email, email) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_matches(const AddressBook *addressBook, int field, const char *key, int *matches) {
    int count = 0;
    for (int i = 0; i < addressBook->contactCount; i++) {
        const Contact *c = &addressBook->contacts[i];
        const char *value = (field == 1) ? c->name : (field == 2) ? c->phone : c->email;
        if (strcmp(value, key) == 0) {
            matches[count++] = i;
        }
    }
    return count;
}

static void print_contact(const Contact *c) {
    printf("Name: %-20s Phone: %-12s Email: %s\n", c->name, c->phone, c->email);
}

/* If one match exists, return it. If many exist, ask the user to select one. */
static int choose_match(const AddressBook *addressBook, const int *matches, int match_count) {
    if (match_count == 1) {
        return matches[0];
    }

    printf("Multiple matching records found:\n");
    for (int i = 0; i < match_count; i++) {
        printf("%d. ", i + 1);
        print_contact(&addressBook->contacts[matches[i]]);
    }
    printf("0. Cancel\n");

    for (;;) {
        int selected = 0;
        if (!read_int("Select one record: ", &selected)) {
            printf("Invalid choice. Please try again.\n");
            continue;
        }
        if (selected == 0) {
            return -1;
        }
        if (selected >= 1 && selected <= match_count) {
            return matches[selected - 1];
        }
        printf("Invalid choice. Please try again.\n");
    }
}

/* Prompt until a valid value is entered, or the user enters 0 to cancel. */
static bool prompt_valid_value(const char *prompt, int field, char *out, size_t out_size,
                               AddressBook *addressBook, int exclude_index) {
    char line[128];
    char err[160];

    for (;;) {
        read_line(prompt, line, sizeof(line));
        trim_whitespace(line);

        if (field == 3) {
            str_to_lower(line);
        }

        if (strcmp(line, CANCEL_INPUT) == 0) {
            return false;
        }

        size_t len = strlen(line);
        if (len >= out_size) {
            printf("Error: Value must be at most %zu characters long.\n", out_size - 1);
            printf("Enter 0 to cancel, or try again.\n");
            continue;
        }

        int ok = 0;
        if (field == 1) {
            ok = validate_name(line, err, sizeof(err));
        } else if (field == 2) {
            ok = validate_phone(line, err, sizeof(err));
        } else {
            ok = validate_email(line, err, sizeof(err));
        }

        if (!ok) {
            printf("Error: %s\n", err);
            printf("Enter 0 to cancel, or try again.\n");
            continue;
        }

        if (field == 2 && find_by_phone(addressBook, line, exclude_index) != -1) {
            printf("Error: Phone number already exists.\n");
            printf("Enter 0 to cancel, or try again.\n");
            continue;
        }
        if (field == 3 && find_by_email(addressBook, line, exclude_index) != -1) {
            printf("Error: Email ID already exists.\n");
            printf("Enter 0 to cancel, or try again.\n");
            continue;
        }

        memcpy(out, line, len + 1);
        return true;
    }
}

/* ---------- Required address book operations ---------- */

void initialize(AddressBook *addressBook) {
    addressBook->contactCount = 0;
    loadContactsFromFile(addressBook);
}

void saveAndExit(AddressBook *addressBook) {
    saveContactsToFile(addressBook);
    printf("Contacts saved. Goodbye!\n");
    exit(EXIT_SUCCESS);
}

void createContact(AddressBook *addressBook) {
    if (addressBook->contactCount >= MAX_CONTACTS) {
        printf("Address book is full.\n");
        return;
    }

    Contact new_contact;
    memset(&new_contact, 0, sizeof(new_contact));

    printf("Enter 0 at any prompt to cancel creation.\n");
    if (!prompt_valid_value("Enter name: ", 1, new_contact.name, sizeof(new_contact.name),
                            addressBook, -1)) {
        printf("Create contact cancelled.\n");
        return;
    }
    if (!prompt_valid_value("Enter phone number: ", 2, new_contact.phone, sizeof(new_contact.phone),
                            addressBook, -1)) {
        printf("Create contact cancelled.\n");
        return;
    }
    if (!prompt_valid_value("Enter email ID: ", 3, new_contact.email, sizeof(new_contact.email),
                            addressBook, -1)) {
        printf("Create contact cancelled.\n");
        return;
    }

    if (find_exact_duplicate(addressBook, new_contact.name, new_contact.phone,
                             new_contact.email, -1) != -1) {
        printf("Error: Duplicate contact already exists.\n");
        return;
    }

    addressBook->contacts[addressBook->contactCount++] = new_contact;
    printf("Contact added successfully.\n");
}

void searchContact(AddressBook *addressBook) {
    int choice = 0;
    printf("\nSearch by:\n1. Name\n2. Phone number\n3. Email ID\n0. Exit\n");
    if (!read_int("Enter your choice: ", &choice)) {
        printf("Invalid choice. Please try again.\n");
        return;
    }
    if (choice == 0) {
        return;
    }
    if (choice < 1 || choice > 3) {
        printf("Invalid choice. Please try again.\n");
        return;
    }

    const char *prompt = (choice == 1) ? "Enter name to search: " :
                         (choice == 2) ? "Enter phone number to search: " :
                                         "Enter email ID to search: ";
    char key[128];
    read_line(prompt, key, sizeof(key));
    trim_whitespace(key);

    if (choice == 3) {
        str_to_lower(key);
    }

    int matches[MAX_CONTACTS];
    int match_count = find_matches(addressBook, choice, key, matches);
    if (match_count == 0) {
        printf("No matching record found.\n");
        return;
    }

    int index = choose_match(addressBook, matches, match_count);
    if (index == -1) {
        printf("Search cancelled.\n");
        return;
    }

    printf("Matching contact:\n");
    print_contact(&addressBook->contacts[index]);
}

void editContact(AddressBook *addressBook) {
    int choice = 0;
    printf("\nEdit search by:\n1. Name\n2. Phone number\n3. Email ID\n0. Exit\n");
    if (!read_int("Enter your choice: ", &choice)) {
        printf("Invalid choice. Please try again.\n");
        return;
    }
    if (choice == 0) {
        return;
    }
    if (choice < 1 || choice > 3) {
        printf("Invalid choice. Please try again.\n");
        return;
    }

    const char *prompt = (choice == 1) ? "Enter name to find: " :
                         (choice == 2) ? "Enter phone number to find: " :
                                         "Enter email ID to find: ";
    char key[128];
    read_line(prompt, key, sizeof(key));
    trim_whitespace(key);

    if (choice == 3) {
        str_to_lower(key);
    }

    int matches[MAX_CONTACTS];
    int match_count = find_matches(addressBook, choice, key, matches);
    if (match_count == 0) {
        printf("No matching record found.\n");
        return;
    }

    int index = choose_match(addressBook, matches, match_count);
    if (index == -1) {
        printf("Edit cancelled.\n");
        return;
    }

    int edit_field = 0;
    printf("\nEdit:\n1. Name\n2. Phone number\n3. Email ID\n0. Back\n");
    if (!read_int("Enter your choice: ", &edit_field)) {
        printf("Invalid choice. Please try again.\n");
        return;
    }
    if (edit_field == 0) {
        return;
    }
    if (edit_field < 1 || edit_field > 3) {
        printf("Invalid choice. Please try again.\n");
        return;
    }

    Contact candidate = addressBook->contacts[index];
    char *new_value = (edit_field == 1) ? candidate.name :
                      (edit_field == 2) ? candidate.phone : candidate.email;
    size_t new_value_size = (edit_field == 1) ? sizeof(candidate.name) :
                            (edit_field == 2) ? sizeof(candidate.phone) :
                                                sizeof(candidate.email);
    const char *value_prompt = (edit_field == 1) ? "Enter new name: " :
                               (edit_field == 2) ? "Enter new phone number: " :
                                                   "Enter new email ID: ";
    if (!prompt_valid_value(value_prompt, edit_field, new_value, new_value_size,
                            addressBook, index)) {
        printf("Edit cancelled.\n");
        return;
    }

    if (find_exact_duplicate(addressBook, candidate.name, candidate.phone,
                             candidate.email, index) != -1) {
        printf("Error: Update rejected because it creates a duplicate contact.\n");
        return;
    }

    addressBook->contacts[index] = candidate;
    printf("Contact updated successfully.\n");
}

void deleteContact(AddressBook *addressBook) {
    int choice = 0;
    printf("\nDelete search by:\n1. Name\n2. Phone number\n3. Email ID\n0. Exit\n");
    if (!read_int("Enter your choice: ", &choice)) {
        printf("Invalid choice. Please try again.\n");
        return;
    }
    if (choice == 0) {
        return;
    }
    if (choice < 1 || choice > 3) {
        printf("Invalid choice. Please try again.\n");
        return;
    }

    const char *prompt = (choice == 1) ? "Enter name to find: " :
                         (choice == 2) ? "Enter phone number to find: " :
                                         "Enter email ID to find: ";
    char key[128];
    read_line(prompt, key, sizeof(key));
    trim_whitespace(key);

    if (choice == 3) {
        str_to_lower(key);
    }

    int matches[MAX_CONTACTS];
    int match_count = find_matches(addressBook, choice, key, matches);
    if (match_count == 0) {
        printf("No matching record found.\n");
        return;
    }

    int index = choose_match(addressBook, matches, match_count);
    if (index == -1) {
        printf("Delete cancelled.\n");
        return;
    }

    printf("Selected contact:\n");
    print_contact(&addressBook->contacts[index]);

    char confirm[16];
    read_line("Are you sure you want to delete? (y/n): ", confirm, sizeof(confirm));
    trim_whitespace(confirm);
    if (!(confirm[0] == 'y' || confirm[0] == 'Y')) {
        printf("Delete cancelled.\n");
        return;
    }

    for (int i = index; i < addressBook->contactCount - 1; i++) {
        addressBook->contacts[i] = addressBook->contacts[i + 1];
    }
    addressBook->contactCount--;
    printf("Contact deleted successfully.\n");
}

void listContacts(AddressBook *addressBook) {
    if (addressBook->contactCount == 0) {
        printf("No contacts available.\n");
        return;
    }

    printf("\n+------+----------------------+--------------+--------------------------+\n");
printf("| No.  | Name                 | Phone        | Email                    |\n");
printf("+------+----------------------+--------------+--------------------------+\n");

for (int i = 0; i < addressBook->contactCount; i++)
{
    printf("| %-4d | %-20s | %-12s | %-24s |\n",
           i + 1,
           addressBook->contacts[i].name,
           addressBook->contacts[i].phone,
           addressBook->contacts[i].email);
}

printf("+------+----------------------+--------------+--------------------------+\n");
}