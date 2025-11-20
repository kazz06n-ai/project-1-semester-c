#include <stdio.h>
#include <string.h>

#define MAX_ACCOUNTS 200
#define ADMIN_PASS "admin123"

// Interest rates (annual)
#define SAVINGS_ANNUAL_RATE 5.0f     // 5% annually
#define FD_ANNUAL_RATE 7.0f          // 7% annually

// FD early break penalty (percentage of balance deducted once)
#define FD_BREAK_PENALTY_PERCENT 1.0f

// Account types
// 1 = Savings, 2 = Current, 3 = Fixed Deposit
struct Account {
    int accNo;
    char name[50];
    float balance;
    int pin;
    int type;
    int fdMonthsRemaining;  // For FD: months remaining until maturity; 0 for non-FD
};

struct Account accounts[MAX_ACCOUNTS];
int totalAccounts = 0;

// ---------- Utilities ----------

void logTransaction(int accNo, const char *action, float amount, float balance) {
    char filename[64];
    sprintf(filename, "%d_history.txt", accNo);
    FILE *fp = fopen(filename, "a");
    if (!fp) {
        printf("Error: could not open history file for account %d\n", accNo);
        return;
    }
    fprintf(fp, "%s: %.2f | Balance: %.2f\n", action, amount, balance);
    fclose(fp);
}

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
    int pin;
    printf("Enter PIN: ");
    scanf("%d", &pin);
    if (pin != accounts[idx].pin) {
        printf("Incorrect PIN!\n");
        return -1;
    }
    return idx;
}

// ---------- File I/O ----------

void loadAccounts() {
    FILE *fp = fopen("accounts.txt", "r");
    if (fp == NULL) return;
    // Format: accNo name balance pin type fdMonthsRemaining
    while (fscanf(fp, "%d %49s %f %d %d %d",
                  &accounts[totalAccounts].accNo,
                  accounts[totalAccounts].name,
                  &accounts[totalAccounts].balance,
                  &accounts[totalAccounts].pin,
                  &accounts[totalAccounts].type,
                  &accounts[totalAccounts].fdMonthsRemaining) == 6) {
        totalAccounts++;
        if (totalAccounts >= MAX_ACCOUNTS) break;
    }
    fclose(fp);
}

void saveAccounts() {
    FILE *fp = fopen("accounts.txt", "w");
    if (!fp) {
        printf("Error: could not save accounts file.\n");
        return;
    }
    for (int i = 0; i < totalAccounts; i++) {
        fprintf(fp, "%d %s %.2f %d %d %d\n",
                accounts[i].accNo,
                accounts[i].name,
                accounts[i].balance,
                accounts[i].pin,
                accounts[i].type,
                accounts[i].fdMonthsRemaining);
    }
    fclose(fp);
}

// ---------- Account Creation ----------

void createAccount() {
    if (totalAccounts >= MAX_ACCOUNTS) {
        printf("Bank limit reached!\n");
        return;
    }

    struct Account a;
    a.accNo = 1000 + totalAccounts;

    printf("Enter name: ");
    scanf("%49s", a.name);

    printf("Set a 4-digit PIN: ");
    scanf("%d", &a.pin);

    printf("Select Account Type:\n");
    printf("1. Savings\n2. Current\n3. Fixed Deposit\nChoice: ");
    scanf("%d", &a.type);

    a.balance = 0.0f;
    a.fdMonthsRemaining = 0;

    if (a.type == 3) {
        int months;
        float initial;
        printf("Enter FD duration in months (e.g., 12): ");
        scanf("%d", &months);
        if (months <= 0) months = 12;
        a.fdMonthsRemaining = months;
        printf("Enter initial deposit amount for FD: ");
        scanf("%f", &initial);
        if (initial < 0) initial = 0;
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
    float amt;
    printf("Enter amount to deposit: ");
    scanf("%f", &amt);
    if (amt <= 0) { printf("Amount must be positive.\n"); return; }

    // FD allows deposit? Typically FDs don't allow top-ups; we disallow if FD active.
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
    float amt;
    printf("Enter amount to withdraw: ");
    scanf("%f", &amt);
    if (amt <= 0) { printf("Amount must be positive.\n"); return; }

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
    int toAcc;
    float amt;
    printf("Enter receiver account number: ");
    scanf("%d", &toAcc);
    int toIdx = findAccountIndexByAccNo(toAcc);
    if (toIdx == -1) { printf("Receiver account not found!\n"); return; }

    if (accounts[idx].type == 3 && accounts[idx].fdMonthsRemaining > 0) {
        printf("Transfer from active FD is not allowed. Break FD or wait until maturity.\n");
        return;
    }

    printf("Enter amount to transfer: ");
    scanf("%f", &amt);
    if (amt <= 0) { printf("Amount must be positive.\n"); return; }

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
    sprintf(filename, "%d_history.txt", accNo);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("No transaction history found!\n");
        return;
    }
    printf("\n--- Transaction History ---\n");
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
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
    printf("Confirm break FD? (1 = Yes, 0 = No): ");
    int confirm; scanf("%d", &confirm);
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

void applyMonthlyInterestToAll() {
    float savingsMonthlyRate = SAVINGS_ANNUAL_RATE / 12.0f;
    float fdMonthlyRate = FD_ANNUAL_RATE / 12.0f;

    printf("\nApplying monthly interest: Savings %.4f%%, FD %.4f%%\n",
           savingsMonthlyRate, fdMonthlyRate);

    for (int i = 0; i < totalAccounts; i++) {
        if (accounts[i].type == 1) {
            float interest = accounts[i].balance * (savingsMonthlyRate / 100.0f);
            if (interest > 0) {
                accounts[i].balance += interest;
                logTransaction(accounts[i].accNo, "Interest Added (Savings)", interest, accounts[i].balance);
            }
        } else if (accounts[i].type == 3) {
            float interest = accounts[i].balance * (fdMonthlyRate / 100.0f);
            if (interest > 0) {
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

void viewAllAccounts() {
    printf("\n--- All Accounts ---\n");
    for (int i = 0; i < totalAccounts; i++) {
        const char *typeStr = (accounts[i].type == 1 ? "Savings" :
                               accounts[i].type == 2 ? "Current" : "FD");
        printf("AccNo: %d | Name: %s | Type: %s | Balance: %.2f",
               accounts[i].accNo, accounts[i].name, typeStr, accounts[i].balance);
        if (accounts[i].type == 3) {
            printf(" | FD Months Remaining: %d", accounts[i].fdMonthsRemaining);
        }
        printf(" | PIN: %d\n", accounts[i].pin);
    }
}

void deleteAccount() {
    int acc;
    printf("Enter account number to delete: ");
    scanf("%d", &acc);
    int idx = findAccountIndexByAccNo(acc);
    if (idx == -1) { printf("Account not found!\n"); return; }

    for (int j = idx; j < totalAccounts - 1; j++) accounts[j] = accounts[j+1];
    totalAccounts--;
    saveAccounts();
    printf("Account deleted successfully!\n");
}

void resetPIN() {
    int acc, newPin;
    printf("Enter account number to reset PIN: ");
    scanf("%d", &acc);
    int idx = findAccountIndexByAccNo(acc);
    if (idx == -1) { printf("Account not found!\n"); return; }
    printf("Enter new PIN: ");
    scanf("%d", &newPin);
    accounts[idx].pin = newPin;
    saveAccounts();
    printf("PIN reset successfully!\n");
}

void adminMenu() {
    int choice;
    do {
        printf("\n--- Admin Menu ---\n");
        printf("1. View All Accounts\n");
        printf("2. Delete Account\n");
        printf("3. Reset PIN\n");
        printf("4. Apply Monthly Interest\n");
        printf("5. Advance Months (apply interest + reduce FD months)\n");
        printf("6. Exit Admin\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: viewAllAccounts(); break;
            case 2: deleteAccount(); break;
            case 3: resetPIN(); break;
            case 4: applyMonthlyInterestToAll(); break;
            case 5: {
                int months;
                printf("Enter number of months to advance: ");
                scanf("%d", &months);
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

        printf("Enter choice: ");
        scanf("%d", &choice);

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

int main() {
    loadAccounts();

    int choice;
    do {
        printf("\n--- Bank System ---\n");
        printf("1. Create Account\n");
        printf("2. Customer Login\n");
        printf("3. Admin Login\n");
        printf("4. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: createAccount(); break;
            case 2: {
                int acc;
                printf("Enter account number: ");
                scanf("%d", &acc);
                int idx = verifyPIN(acc);
                if (idx != -1) customerMenu(idx);
                break;
            }
            case 3: {
                char pass[64];
                printf("Enter Admin Password: ");
                scanf("%63s", pass);
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