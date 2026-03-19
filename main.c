#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#define GROWTH_RATE 0.07

typedef struct {
    int age;
    double income;
    double expenses;
    double retirement_age;
} Client;

typedef struct {
    double balance;
    char type[20];
} Account;

typedef struct {
    int age;
    double balance;
    double income;
    double expenses;
} ProjectionState;

double calculate_tax(double income) {
    if (income <= 11000)
        return income * 0.10;
    else if (income <= 44725)
        return 1100 + (income - 11000) * 0.12;
    else if (income <= 95375)
        return 5147 + (income - 44725) * 0.22;
    else if (income <= 182050)
        return 16290 + (income - 95375) * 0.24;
    else
        return income * 0.37;
}

double normal_random(double mean, double stddev) {
    double u1 = ((double)rand() / RAND_MAX);
    if (u1 == 0) u1 = 0.0001; // log can become -inf if u1 is zero;
    double u2 = ((double)rand() / RAND_MAX);
    double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return mean + z * stddev;
}

double random_return() {
    return normal_random(0.10, 0.15);
}

ProjectionState advance_one_year(ProjectionState current) {
    ProjectionState next = current;
    next.age += 1;
    next.balance += (current.income - current.expenses);
    next.balance -= calculate_tax(current.income);
    double rand_growth_rate = random_return();
    next.balance *= (1 + rand_growth_rate);
    return next;
}

int main(int argc, char *argv[]) {
    double balance = atof(argv[1]);
    double income = atof(argv[2]);
    double expenses = atof(argv[3]);
    int retirement_age = atoi(argv[4]);

    srand(time(NULL));
    int count = 0;
    int runs = 1000;
    for (int j = 0; j < runs; j++) {
        ProjectionState state = {
            .age = 30,
            .balance = balance,
            .income = income,
            .expenses = expenses
        };

        int final_age = 80; 

        for (int i = state.age; i <= final_age; i++) {
            if (i >= retirement_age) {
                state.income = 0; // income stops after retirement age
            }
            state = advance_one_year(state);

            // printf("Balance: $%.2f\n", state.balance);
            if (state.balance <= 0) {
                break;
            } 

         }

       
        if (state.balance > 0) count += 1;
    }

    printf("Probablity of success: %f", ((double)count / runs));
    return 0;
}