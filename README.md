cat << 'EOF' > README.md
# Address Book Management System

A modular, console-driven contact manager implemented in C. The application provides an interactive terminal interface for managing contacts with persistent storage handled via text file operations.

## Features

- **Create Contacts**: Add new entries with name, phone number, and email (up to 100 contacts).
- **Search Contacts**: Find existing contacts by name, phone number, or email.
- **Edit Contacts**: Modify details of existing records.
- **Delete Contacts**: Remove unwanted entries.
- **List All Contacts**: View the entire directory at a glance.
- **Save & Exit**: Persists changes to disk, or exit without saving.

## Project Structure

| File | Purpose |
|------|---------|
| `main.c` | Driver program — menu loop and user navigation |
| `contact.c` / `contact.h` | Core data structures (Contact, AddressBook) and record operations |
| `file.c` / `file.h` | File I/O — loads and saves contacts in tab-separated format |
| `contacts.txt` | Persistent storage file (auto-created on first save) |

## Build and Run

**Prerequisites:** GCC compiler; Linux, macOS, or Windows WSL

```bash
gcc main.c contact.c file.c -o addressbook
./addressbook
