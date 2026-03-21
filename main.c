#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#define GROWTH_RATE 0.07

// this is a client; it has income expenses and retirement_age
typedef struct {
    int age;
    double income;
    double expenses;
    double retirement_age;
} Client;

// this is account which has balance and a type IRA, Roth, 401K etc
typedef struct {
    double balance;
    char type[20];
} Account;

// This is a state of cash at a particular year
typedef struct {
    int age;
    double balance;
    double income;
    double expenses;
} ProjectionState;

// this is tax calcuation by brackets after standard deductions
double calculate_tax(double income) {
    double std_deduction = 14600.0; // income - standard deduction;

    double taxable = income - std_deduction; 
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

// function that computes a random value from normal distribution using Box-Muller transform given a mean and a stddev
double normal_random(double mean, double stddev) {
    double u1 = ((double)rand() / RAND_MAX);
    if (u1 == 0) u1 = 0.0001; // log can become -inf if u1 is zero;
    double u2 = ((double)rand() / RAND_MAX);
    double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
    return mean + z * stddev;
}

double gross_withdrawal_401k(double expenses) {
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

double get_pct_ss_taxable(double income) {
    if (income < 25000)
        return 0;
    else if (income < 34000)
        return 0.5;
    else 
        return 0.85;
}

double compute_agi(double net_income, double ss_benefit) {
    double low = net_income;  // the lower limit is when tax on ss is zero 
    double high = net_income + ss_benefit * 0.85; // the upper limit is when tax on ss is 85%
    // the AGI cannot be out of this limit; these are upper and lower bounds

    double err = 0.01; // the err is $0.01
    while (high - low > err) {
        double mid = (low + high) / 2.0; // mid is our initial guess

        // using mid calculate combined income and use that to determine % of ss taxable
        double combined_income = mid + (ss_benefit * 0.5);
        double pct_ss_taxable = get_pct_ss_taxable(combined_income);

        // not the right hand side of equation AGI = x + taxable_ss(AGI) can be solved by using mid as our guess and taxable_ss = ss_benefit * pct_ss_taxable
        double rhs = net_income + ss_benefit * pct_ss_taxable;


        // now my left hand side is mid and if my right hand side is less that lhs
        // then I need to increase my search space
        if (mid > rhs)
            low = mid;
        else
            high = mid;
    }

    return (low + high) / 2.0;
}

double get_taxable_ss(double income, double ss_benefit) {
    double agi = compute_agi(income, ss_benefit);
    // agi = income + taxable_ss
    return agi - income;

}

// this function computes the income for the next year after all the deductions
ProjectionState advance_one_year(ProjectionState current) {
    ProjectionState next = current;
    next.age += 1;
    // double net_income = current.income - calculate_tax(current.income) - current.expenses;
    // next.balance += net_income;
    double rand_growth_rate = normal_random(0.10, 0.15);
    if (next.balance > 0) // you cannot invest money you don't have principal unless you are private equity firm;
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
        double ss_benefit = 42000;

        for (int i = state.age; i <= final_age; i++) {

            // there are various source of income that gets added
            // 1. social security after 67
            // 2. 401K can be added at any time but let us assume now after retirement
            if (retirement_age < 67) {
                if (i >= retirement_age) {
                    if (i < 67) {
                        // income from salary stops and now comes from 401K withdrawals
                        // social security has not started yet
                        state.balance -= (expenses + calculate_tax(expenses));
                        state.income = 0;
                    }
                    else {
                        // now this is tricky because taxable ss needs the income
                        // which is coming from 401K but how much to withdraw 401K
                        // is a decision you might make based on how much social security
                        // you are getting; here we assume that it is fixed for now
                        // but it really depends on your age and income history
                        double withdrawal = expenses - ss_benefit;
                        if (withdrawal > 0) {
                            // if withdrawal are greater than zero then withdraw from 401K
                            double taxable_ss = get_taxable_ss(withdrawal, ss_benefit);
                            double taxable_income = withdrawal + taxable_ss;
                            double tax = calculate_tax(taxable_income);

                            // now from the balance the withdrawal amount will be deducted; some portion of that will be submitted as taxes at the year end
                            // social security amount will be added to the balance
                            state.balance -= (withdrawal + tax);
                            state.balance += ss_benefit;
                        }
                    }
                        
                }
                else { // i is less than the retirement age
                    double tax = calculate_tax(state.income);
                    state.balance += (state.income - tax);
                }
            }
            else {
                // now after 67 the ss_benefits will get added as income and that will be taxes as usual
                if (i <= retirement_age) {
                    if (i > 67) {
                        // now this is again tricky; I can directly add ss_benefits to the income because tax on ss depends on the income 
                        // to model this I am just going to add the ss to the benefits and take out the taxes 
                        double taxable_ss = get_taxable_ss(state.income, ss_benefit);
                        double tax = calculate_tax(state.income + taxable_ss);
                        state.balance += (ss_benefit + state.income);
                        state.balance -= tax;
                    }
                    else {
                        double tax = calculate_tax(state.income);
                        state.balance += (state.income - tax);
                    }
                }
                else {
                    // post retirement income stops 401K withdrawal starts and social security continues after 67
                    double withdrawal = expenses - ss_benefit;
                    if (withdrawal > 0) {
                        // if net_expense are greater than zero then withdraw from 401K
                        double taxable_ss = get_taxable_ss(withdrawal, ss_benefit);
                        double taxable_income = withdrawal + taxable_ss;
                        double tax = calculate_tax(taxable_income);

                        // now from the balance the withdrawal amount will be deducted; some portion of that will be submitted as taxes at the year end
                        // social security amount will be added to the balance
                        state.balance -= (withdrawal + tax);
                        state.balance += ss_benefit;
                    }
                }
            }

            // the advance_one_year should not compute the taxes
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