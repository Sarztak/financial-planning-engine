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
    double taxable = income - 14600; // income - standard deduction;
    if (taxable <= 0) 
        return 0;
    else if (taxable <= 11000)
        return taxable * 0.10;
    else if (taxable <= 44725)
        return 1100 + (taxable - 11000) * 0.12;
    else if (taxable <= 95375)
        return 5147 + (taxable - 44725) * 0.22;
    else if (taxable <= 182050)
        return 16290 + (taxable - 95375) * 0.24;
    else
        return taxable * 0.37;
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

double gross_withdrawal(double expenses) {
    // here is what I can understand so my initial guess is the middle value 1. 5 expenses then I check how much I get after taxes. now if this amount is even less that what I want as my expenses then my guess is wrong, I can't really have the amount that I am withdrawing being less than what I want as expense; I want higher than that so my lower limit increases to mid and vice versa if my net is higher than expense then I am withdrawing more so I lower my search range. then at some point low and high converse close enough so that expenses = net and I return the mid point of low and high so the error in estimation can be at max 0.005 dollars

    double low = expenses;
    double high = expenses * 2;
    double error = 0.01; // the error is $0.01
    while (high - low > error) {
        double mid = (low + high) / 2.0;
        double net = mid - calculate_tax(mid); // this is amt after taxes
        if (net < expenses) low = mid; 
        else high = mid;
    }
    return (low + high) / 2.0;
}

ProjectionState advance_one_year(ProjectionState current) {
    ProjectionState next = current;
    next.age += 1;
    double net_income = current.income - calculate_tax(current.income) - current.expenses;
    next.balance += net_income;
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

        int final_age = 90; 

        for (int i = state.age; i <= final_age; i++) {
            if (i >= retirement_age) {
                state.income = 0; // income stops after retirement age
                state.balance -= gross_withdrawal(state.expenses);
                // in retirement we withdraw money which is taxable 
            }
            state = advance_one_year(state);

            // printf("Balance: $%.2f\n", state.balance);
            if (state.balance <= 0) {
                printf("Broke at age %d: $%.2f\n", state.age, state.balance);
                break;
            } 

         }

        printf("Final balance: $%.2f\n", state.balance);
        if (state.balance > 0) count += 1;
    }
    printf("Probablity of success: %f", ((double)count / runs));
    return 0;
}