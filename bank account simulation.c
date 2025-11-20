#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_ACCOUNTS 200
#define ADMIN_PASS "admin123"

// Interest rates (annual)
#define SAVINGS_ANNUAL_RATE 5.0f     // 5% annually
#define FD_ANNUAL_RATE 7.0f          // 7% annually

// FD early break penalty (percentage of balance deducted once)
#define FD_BREAK_PENALTY_PERCENT 1.0f

struct Account {
    int accNo;
    char name[100];
    float balance;
    int pin;
    int type;               // 1 = Savings, 2 = Current, 3 = Fixed Deposit
    int fdMonthsRemaining;  // For FD: months remaining until maturity; 0 for non-FD
};

static struct Account accounts[MAX_ACCOUNTS];
static int totalAccounts = 0;
static int nextAccNo = 1000;

// ---------- Utility Input Helpers ----------
static void clear_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static void read_line(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) { buf[0] = '\0'; return; }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
}

static int read_int(const char *prompt) {
    char buf[64];
    int val;
    while (1) {
        if (prompt) printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (sscanf(buf, "%d", &val) == 1) return val;
        printf("Invalid input. Please enter a valid integer.\n");
    }
}

static float read_float(const char *prompt) {
    char buf[64];
    float val;
    while (1) {
        if (prompt) printf("%s", prompt);
        read_line(buf, sizeof(buf));
        if (sscanf(buf, "%f", &val) == 1) return val;
        printf("Invalid input. Please enter a valid number.\n");
    }
}

// ---------- Transaction Logging ----------
void logTransaction(int accNo, const char *action, float amount, float balance) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%d_history.txt", accNo);
    FILE *fp = fopen(filename, "a");
    if (!fp) {
        fprintf(stderr, "Error: could not open history file for account %d\n", accNo);
        return;
    }
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(fp, "%s | %s : %.2f | Balance: %.2f\n", timestr, action, amount, balance);
    fclose(fp);
}

// ---------- Account Find / Verify ----------
int findAccountIndexByAccNo(int accNo) {
    for (int i = 0; i < totalAccounts; i++) {
        if (accounts[i].accNo == accNo) return i;
    }
    return -1;
}

int verifyPIN(int accNo) {
    int idx = findAccountIndexByAccNo(accNo);
    if (idx == -1) {
        printf("Account not found!\n");
        return -1;
    }
    int pin = read_int("Enter PIN: ");
    if (pin != accounts[idx].pin) {
        printf("Incorrect PIN!\n");
        return -1;
    }
    return idx;
}

// ---------- File I/O ----------
void loadAccounts(void) {
    FILE *fp = fopen("accounts.txt", "r");
    if (!fp) return;
    totalAccounts = 0;
    int maxAcc = nextAccNo - 1;
    while (totalAccounts < MAX_ACCOUNTS) {
        struct Account a;
        int scanned = fscanf(fp, "%d %[^\t\n] %f %d %d %d",
                             &a.accNo, a.name, &a.balance, &a.pin, &a.type, &a.fdMonthsRemaining);
        if (scanned != 6) break;
        accounts[totalAccounts++] = a;
        if (a.accNo > maxAcc) maxAcc = a.accNo;
    }
    fclose(fp);
    nextAccNo = maxAcc + 1;
}

void saveAccounts(void) {
    FILE *fp = fopen("accounts.txt", "w");
    if (!fp) {
        fprintf(stderr, "Error: could not save accounts file.\n");
        return;
    }
    for (int i = 0; i < totalAccounts; i++) {
        // Write: accNo name balance pin type fdMonthsRemaining
        // Use a single-word name or a name with underscores; we stored name with spaces, so replace newlines/extra tabs
        char nameSafe[100];
        strncpy(nameSafe, accounts[i].name, sizeof(nameSafe));
        nameSafe[sizeof(nameSafe)-1] = '\0';
        // replace newline or tabs with spaces
        for (size_t j = 0; nameSafe[j]; j++) if (nameSafe[j] == '\t' || nameSafe[j] == '\n') nameSafe[j] = ' ';
        fprintf(fp, "%d\t%s\t%.2f\t%d\t%d\t%d\n",
                accounts[i].accNo,
                nameSafe,
                accounts[i].balance,
                accounts[i].pin,
                accounts[i].type,
                accounts[i].fdMonthsRemaining);
    }
    fclose(fp);
}

// ---------- Account Creation ----------
void createAccount(void) {
    if (totalAccounts >= MAX_ACCOUNTS) {
        printf("Bank limit reached!\n");
        return;
    }

    struct Account a;
    a.accNo = nextAccNo++;
    printf("Enter name: ");
    read_line(a.name, sizeof(a.name));
    if (strlen(a.name) == 0) strncpy(a.name, "Unknown", sizeof(a.name));

    // PIN: enforce 4-digit
    while (1) {
        a.pin = read_int("Set a 4-digit PIN: ");
        if (a.pin >= 1000 && a.pin <= 9999) break;
        printf("PIN must be a 4-digit number.\n");
    }

    printf("Select Account Type:\n1. Savings\n2. Current\n3. Fixed Deposit\nChoice: ");
    while (1) {
        a.type = read_int(NULL);
        if (a.type >= 1 && a.type <= 3) break;
        printf("Invalid choice. Enter 1, 2, or 3: ");
    }

    a.balance = 0.0f;
    a.fdMonthsRemaining = 0;

    if (a.type == 3) {
        int months = read_int("Enter FD duration in months (e.g., 12): ");
        if (months <= 0) months = 12;
        a.fdMonthsRemaining = months;
        float initial;
        initial = read_float("Enter initial deposit amount for FD: ");
        if (initial < 0.0f) initial = 0.0f;
        a.balance = initial;
        printf("FD created: Duration %d months, Initial Amount %.2f\n", a.fdMonthsRemaining, a.balance);
        logTransaction(a.accNo, "FD Created", a.balance, a.balance);
    } else {
        printf("Account created as %s. Initial balance: 0.00\n",
               (a.type == 1 ? "Savings" : "Current"));
    }

    accounts[totalAccounts++] = a;
    saveAccounts();
    printf("Account created successfully! Account No: %d\n", a.accNo);
}

// ---------- Customer Operations ----------
void deposit_op(int idx) {
    float amt = read_float("Enter amount to deposit: ");
    if (amt <= 0.0f) { printf("Amount must be positive.\n"); return; }

    if (accounts[idx].type == 3 && accounts[idx].fdMonthsRemaining > 0) {
        printf("Cannot deposit into active FD. Consider breaking FD or wait until maturity.\n");
        return;
    }

    accounts[idx].balance += amt;
    printf("Deposit successful! New Balance: %.2f\n", accounts[idx].balance);
    saveAccounts();
    logTransaction(accounts[idx].accNo, "Deposit", amt, accounts[idx].balance);
}

void withdraw_op(int idx) {
    float amt = read_float("Enter amount to withdraw: ");
    if (amt <= 0.0f) { printf("Amount must be positive.\n"); return; }

    if (accounts[idx].type == 3 && accounts[idx].fdMonthsRemaining > 0) {
        printf("Withdrawal from active FD is not allowed. Use 'Break FD' option.\n");
        return;
    }

    if (amt <= accounts[idx].balance) {
        accounts[idx].balance -= amt;
        printf("Withdrawal successful! New Balance: %.2f\n", accounts[idx].balance);
        saveAccounts();
        logTransaction(accounts[idx].accNo, "Withdraw", amt, accounts[idx].balance);
    } else {
        printf("Insufficient balance!\n");
    }
}

void checkBalance_op(int idx) {
    const char *typeStr = (accounts[idx].type == 1 ? "Savings" :
                           accounts[idx].type == 2 ? "Current" : "Fixed Deposit");
    printf("Account Holder: %s\n", accounts[idx].name);
    printf("Account Type: %s\n", typeStr);
    printf("Balance: %.2f\n", accounts[idx].balance);
    if (accounts[idx].type == 3) {
        printf("FD Months Remaining: %d\n", accounts[idx].fdMonthsRemaining);
    }
}

void transfer_op(int idx) {
    int toAcc = read_int("Enter receiver account number: ");
    int toIdx = findAccountIndexByAccNo(toAcc);
    if (toIdx == -1) { printf("Receiver account not found!\n"); return; }

    if (accounts[idx].type == 3 && accounts[idx].fdMonthsRemaining > 0) {
        printf("Transfer from active FD is not allowed. Break FD or wait until maturity.\n");
        return;
    }

    float amt = read_float("Enter amount to transfer: ");
    if (amt <= 0.0f) { printf("Amount must be positive.\n"); return; }

    if (accounts[idx].balance >= amt) {
        accounts[idx].balance -= amt;
        accounts[toIdx].balance += amt;
        printf("Transfer successful!\n");
        saveAccounts();
        logTransaction(accounts[idx].accNo, "Transfer Sent", amt, accounts[idx].balance);
        logTransaction(accounts[toIdx].accNo, "Transfer Received", amt, accounts[toIdx].balance);
    } else {
        printf("Insufficient balance!\n");
    }
}

void viewHistory(int accNo) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%d_history.txt", accNo);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("No transaction history found!\n");
        return;
    }
    printf("\n--- Transaction History for %d ---\n", accNo);
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        fputs(line, stdout);
    }
    fclose(fp);
}

void breakFD_op(int idx) {
    if (accounts[idx].type != 3 || accounts[idx].fdMonthsRemaining <= 0) {
        printf("No active FD to break.\n");
        return;
    }
    printf("Breaking FD will apply a %.2f%% penalty on the current balance and convert to Savings.\n",
           FD_BREAK_PENALTY_PERCENT);
    int confirm = read_int("Confirm break FD? (1 = Yes, 0 = No): ");
    if (confirm != 1) { printf("FD break cancelled.\n"); return; }

    float penalty = accounts[idx].balance * (FD_BREAK_PENALTY_PERCENT / 100.0f);
    accounts[idx].balance -= penalty;
    accounts[idx].type = 1; // Convert to Savings
    accounts[idx].fdMonthsRemaining = 0;

    printf("FD broken. Penalty deducted: %.2f. New Balance: %.2f. Account converted to Savings.\n",
           penalty, accounts[idx].balance);
    saveAccounts();
    logTransaction(accounts[idx].accNo, "FD Broken Penalty", penalty, accounts[idx].balance);
}

// ---------- Interest & Time Progression ----------
void applyMonthlyInterestToAll(void) {
    float savingsMonthlyRate = SAVINGS_ANNUAL_RATE / 12.0f;
    float fdMonthlyRate = FD_ANNUAL_RATE / 12.0f;

    printf("\nApplying monthly interest: Savings %.4f%%, FD %.4f%%\n",
           savingsMonthlyRate, fdMonthlyRate);

    for (int i = 0; i < totalAccounts; i++) {
        if (accounts[i].type == 1) {
            float interest = accounts[i].balance * (savingsMonthlyRate / 100.0f);
            if (interest > 0.0f) {
                accounts[i].balance += interest;
                logTransaction(accounts[i].accNo, "Interest Added (Savings)", interest, accounts[i].balance);
            }
        } else if (accounts[i].type == 3) {
            float interest = accounts[i].balance * (fdMonthlyRate / 100.0f);
            if (interest > 0.0f) {
                accounts[i].balance += interest;
                logTransaction(accounts[i].accNo, "Interest Added (FD)", interest, accounts[i].balance);
            }
        }
    }
    saveAccounts();
    printf("Monthly interest applied.\n");
}

void advanceMonths(int months) {
    if (months <= 0) { printf("Months must be positive.\n"); return; }

    for (int m = 0; m < months; m++) {
        applyMonthlyInterestToAll();
        for (int i = 0; i < totalAccounts; i++) {
            if (accounts[i].type == 3 && accounts[i].fdMonthsRemaining > 0) {
                accounts[i].fdMonthsRemaining--;
                if (accounts[i].fdMonthsRemaining == 0) {
                    logTransaction(accounts[i].accNo, "FD Matured", 0.0f, accounts[i].balance);
                }
            }
        }
        saveAccounts();
    }
    printf("Advanced %d month(s). FD durations updated.\n", months);
}

// ---------- Admin Operations ----------
void viewAllAccounts(void) {
    printf("\n--- All Accounts ---\n");
    for (int i = 0; i < totalAccounts; i++) {
        const char *typeStr = (accounts[i].type == 1 ? "Savings" :
                               accounts[i].type == 2 ? "Current" : "FD");
        // Mask PIN for basic security
        printf("AccNo: %d | Name: %s | Type: %s | Balance: %.2f",
               accounts[i].accNo, accounts[i].name, typeStr, accounts[i].balance);
        if (accounts[i].type == 3) {
            printf(" | FD Months Remaining: %d", accounts[i].fdMonthsRemaining);
        }
        printf(" | PIN: ****\n");
    }
}

void deleteAccount(void) {
    int acc = read_int("Enter account number to delete: ");
    int idx = findAccountIndexByAccNo(acc);
    if (idx == -1) { printf("Account not found!\n"); return; }

    for (int j = idx; j < totalAccounts - 1; j++) accounts[j] = accounts[j+1];
    totalAccounts--;
    saveAccounts();
    printf("Account deleted successfully!\n");
}

void resetPIN(void) {
    int acc = read_int("Enter account number to reset PIN: ");
    int idx = findAccountIndexByAccNo(acc);
    if (idx == -1) { printf("Account not found!\n"); return; }

    int newPin;
    while (1) {
        newPin = read_int("Enter new 4-digit PIN: ");
        if (newPin >= 1000 && newPin <= 9999) break;
        printf("PIN must be a 4-digit number.\n");
    }
    accounts[idx].pin = newPin;
    saveAccounts();
    printf("PIN reset successfully!\n");
}

void adminMenu(void) {
    int choice;
    do {
        printf("\n--- Admin Menu ---\n");
        printf("1. View All Accounts\n");
        printf("2. Delete Account\n");
        printf("3. Reset PIN\n");
        printf("4. Apply Monthly Interest\n");
        printf("5. Advance Months (apply interest + reduce FD months)\n");
        printf("6. Exit Admin\n");
        choice = read_int("Enter choice: ");
        switch (choice) {
            case 1: viewAllAccounts(); break;
            case 2: deleteAccount(); break;
            case 3: resetPIN(); break;
            case 4: applyMonthlyInterestToAll(); break;
            case 5: {
                int months = read_int("Enter number of months to advance: ");
                advanceMonths(months);
                break;
            }
            case 6: printf("Exiting Admin...\n"); break;
            default: printf("Invalid choice!\n");
        }
    } while (choice != 6);
}

// ---------- Customer Menu ----------
void customerMenu(int idx) {
    int choice;
    do {
        printf("\n--- Customer Menu ---\n");
        printf("1. Deposit\n");
        printf("2. Withdraw\n");
        printf("3. Check Balance\n");
        printf("4. Transfer\n");
        printf("5. View History\n");
        if (accounts[idx].type == 3 && accounts[idx].fdMonthsRemaining > 0) {
            printf("6. Break FD (Penalty %.2f%%)\n", FD_BREAK_PENALTY_PERCENT);
            printf("7. Exit Customer\n");
        } else {
            printf("6. Exit Customer\n");
        }

        choice = read_int("Enter choice: ");

        if (accounts[idx].type == 3 && accounts[idx].fdMonthsRemaining > 0) {
            switch (choice) {
                case 1: deposit_op(idx); break;
                case 2: withdraw_op(idx); break;
                case 3: checkBalance_op(idx); break;
                case 4: transfer_op(idx); break;
                case 5: viewHistory(accounts[idx].accNo); break;
                case 6: breakFD_op(idx); break;
                case 7: printf("Exiting Customer...\n"); break;
                default: printf("Invalid choice!\n");
            }
        } else {
            switch (choice) {
                case 1: deposit_op(idx); break;
                case 2: withdraw_op(idx); break;
                case 3: checkBalance_op(idx); break;
                case 4: transfer_op(idx); break;
                case 5: viewHistory(accounts[idx].accNo); break;
                case 6: printf("Exiting Customer...\n"); break;
                default: printf("Invalid choice!\n");
            }
        }

    } while (!((accounts[idx].type == 3 && accounts[idx].fdMonthsRemaining > 0 && choice == 7) ||
               (!(accounts[idx].type == 3 && accounts[idx].fdMonthsRemaining > 0) && choice == 6)));
}

// ---------- Main ----------
int main(void) {
    loadAccounts();

    int choice;
    do {
        printf("\n--- Bank System ---\n");
        printf("1. Create Account\n");
        printf("2. Customer Login\n");
        printf("3. Admin Login\n");
        printf("4. Exit\n");
        choice = read_int("Enter choice: ");

        switch (choice) {
            case 1: createAccount(); break;
            case 2: {
                int acc = read_int("Enter account number: ");
                int idx = verifyPIN(acc);
                if (idx != -1) customerMenu(idx);
                break;
            }
            case 3: {
                char pass[128];
                printf("Enter Admin Password: ");
                read_line(pass, sizeof(pass));
                if (strcmp(pass, ADMIN_PASS) == 0) adminMenu();
                else printf("Incorrect Admin Password!\n");
                break;
            }
            case 4: printf("Exiting...\n"); break;
            default: printf("Invalid choice!\n");
        }
    } while (choice != 4);

    return 0;
}
