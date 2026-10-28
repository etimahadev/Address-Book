#include <stdio.h>
#include <string.h>
#include "file.h"

/* Save all contacts to contacts.txt in a simple tab-separated format. */
void saveContactsToFile(AddressBook *addressBook) {
    FILE *fp = fopen("contacts.txt", "w");
    if (fp == NULL) {
        perror("Unable to save contacts.txt");
        return;
    }

    fprintf(fp, "%d\n", addressBook->contactCount);
    for (int i = 0; i < addressBook->contactCount; i++) {
        fprintf(fp, "%s\t%s\t%s\n",
                addressBook->contacts[i].name,
                addressBook->contacts[i].phone,
                addressBook->contacts[i].email);
    }

    fclose(fp);
}

/* Load contacts from contacts.txt if the file exists. */
void loadContactsFromFile(AddressBook *addressBook) {
    addressBook->contactCount = 0;

    FILE *fp = fopen("contacts.txt", "r");
    if (fp == NULL) {
        return; /* First run: file will be created on save. */
    }

    int count = 0;
    if (fscanf(fp, "%d", &count) != 1 || count < 0 || count > MAX_CONTACTS) {
        fclose(fp);
        return;
    }

    char name[50];
    char phone[20];
    char email[50];
    for (int i = 0; i < count; i++) {
        if (fscanf(fp, "%49s %19s %49s", name, phone, email) != 3) {
            break;
        }
        strcpy(addressBook->contacts[i].name, name);
        strcpy(addressBook->contacts[i].phone, phone);
        strcpy(addressBook->contacts[i].email, email);
        addressBook->contactCount++;
    }

    fclose(fp);
}