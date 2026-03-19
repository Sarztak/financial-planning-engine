# Financial Planning Engine

A Monte Carlo retirement simulation engine written in C.

## What it does

Given a set of financial inputs it runs 1000 simulations of a retirement plan and returns the probability that money lasts until age 90. Each simulation uses a different sequence of market returns drawn from a normal distribution based on historical S&P 500 data — mean return of 10%, standard deviation of 15%.

The core insight this models is **sequence of returns risk** — two people with identical savings can have wildly different outcomes depending on whether the market crashes at the start of their retirement or the end. A single deterministic projection misses this entirely. Monte Carlo surfaces it.

## Usage

```bash
gcc -o output main.c -lm
./output <starting_balance> <annual_income> <annual_expenses> <retirement_age>
```

Example:

```bash
./output 200000 60000 40000 65
```

Output:

```
Probability of success: 0.970000
```

## How it works

The engine maintains a `ProjectionState` struct that represents a snapshot of finances at any given year — balance, income, expenses, age. A single function advances this state forward one year:

- Net income after federal tax and expenses is added to balance
- Balance grows by a randomly sampled market return
- At retirement age income drops to zero and balance is drawn down to cover expenses
- If balance hits zero the simulation ends as a failure

This loop runs from age 30 to 90. The probability of success is the fraction of 1000 simulations where the balance remains above zero at age 90.

## Tax calculation

Federal income tax is calculated on taxable income after the standard deduction ($14,600 for 2024) using current marginal brackets. This affects net savings during the accumulation phase and will affect withdrawal taxation in retirement in a future update.

## Market return distribution

Returns are sampled from a normal distribution using the Box-Muller transform — a method for generating normally distributed random numbers from uniform random inputs. The parameters reflect long run historical US equity market behavior. This means some years the simulation sees -20% returns, others +35%, which is what actually happens and what stress tests a retirement plan.

## Why C

The engine is intentionally written in C to keep the simulation fast, explicit, and close to the metal. With 1000 simulations each running a 60 year projection the performance headroom matters. More importantly the constraints of C force every decision about memory and state to be explicit — there is no hidden allocation, no garbage collection, no abstraction obscuring what the projection loop is actually doing.

## What is next

- Withdrawal taxation — 401k distributions in retirement are taxable income
- Social Security income with combined income taxation thresholds
- Multiple account types — 401k, Roth, taxable brokerage with different tax treatment
- Optimal withdrawal sequencing to minimize lifetime tax burden
- Dynamic arrays for storing full simulation result trajectories