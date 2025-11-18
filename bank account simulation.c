#include <stdio.h>
#include <string.h>

#define MAX_ACCOUNTS 100

// Structure to hold account details
struct Account {
    int accountNumber;
    char name[50];
    float balance;
};

// Function prototypes
void createAccount(struct Account accounts[], int *count);
void deposit(struct Account accounts[], int count);
void withdraw(struct Account accounts[], int count);
void display(struct Account accounts[], int count);
int findAccount(struct Account accounts[], int count, int accNo);

int main() {
    struct Account accounts[MAX_ACCOUNTS];
    int count = 0;
    int choice;

    do {
        printf("\n--- Bank Menu ---\n");
        printf("1. Create Account\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. Display Account\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch(choice) {
            case 1:
                createAccount(accounts, &count);
                break;
            case 2:
                deposit(accounts, count);
                break;
            case 3:
                withdraw(accounts, count);
                break;
            case 4:
                display(accounts, count);
                break;
            case 5:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice!\n");
        }
    } while(choice != 5);

    return 0;
}

// Create a new account
void createAccount(struct Account accounts[], int *count) {
    if(*count >= MAX_ACCOUNTS) {
        printf("Cannot create more accounts!\n");
        return;
    }
    struct Account newAcc;
    printf("Enter account number: ");
    scanf("%d", &newAcc.accountNumber);
    printf("Enter name: ");
    scanf("%s", newAcc.name);
    printf("Enter initial balance: ");
    scanf("%f", &newAcc.balance);

    accounts[*count] = newAcc;
    (*count)++;
    printf("Account created successfully!\n");
}

// Find account by account number
int findAccount(struct Account accounts[], int count, int accNo) {
    for(int i = 0; i < count; i++) {
        if(accounts[i].accountNumber == accNo) {
            return i;
        }
    }
    return -1;
}

// Deposit money
void deposit(struct Account accounts[], int count) {
    int accNo;
    float amount;
    printf("Enter account number: ");
    scanf("%d", &accNo);
    int index = findAccount(accounts, count, accNo);
    if(index == -1) {
        printf("Account not found!\n");
        return;
    }
    printf("Enter amount to deposit: ");
    scanf("%f", &amount);
    accounts[index].balance += amount;
    printf("Deposited %.2f successfully!\n", amount);
}

// Withdraw money
void withdraw(struct Account accounts[], int count) {
    int accNo;
    float amount;
    printf("Enter account number: ");
    scanf("%d", &accNo);
    int index = findAccount(accounts, count, accNo);
    if(index == -1) {
        printf("Account not found!\n");
        return;
    }
    printf("Enter amount to withdraw: ");
    scanf("%f", &amount);
    if(amount > accounts[index].balance) {
        printf("Insufficient balance!\n");
    } else {
        accounts[index].balance -= amount;
        printf("Withdrew %.2f successfully!\n", amount);
    }
}

// Display account details
void display(struct Account accounts[], int count) {
    int accNo;
    printf("Enter account number: ");
    scanf("%d", &accNo);
    int index = findAccount(accounts, count, accNo);
    if(index == -1) {
        printf("Account not found!\n");
        return;
    }
    printf("\n--- Account Details ---\n");
    printf("Account Number: %d\n", accounts[index].accountNumber);
    printf("Name: %s\n", accounts[index].name);
    printf("Balance: %.2f\n", accounts[index].balance);
}