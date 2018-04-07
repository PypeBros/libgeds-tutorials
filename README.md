

== Expressions

Expressions are mostly used to define predicates to state transitions (that is, conditions under which the transit
ion can be taken) or actions that should take place along with the state transition. They manipulate the "control
data" of the object, made of 16 slots (16-bit wide signed values each) that serves
- as communication between the engine and the entity-specific logic
- that can save additional information for that entity.
Things like speed, hitpoints, internal counters, etc. are typically handled through a `cdata` variable (also refer
red to as a 'gobvar' in some part of the code).

=== Basic operations on gobvars

The expressions are evaluated on demand, using a stack-based system that pops some former values and then push bac
k results. For instance,

| [0-9]+ | push the corresponding constant on the stack (decimal number)
| $[0-9a-f]+ | push the corresponding constant on the stack (hexadecimal number)
| v[0-9a-vf] | reads the contents of one gobvar (which one is define by the hex digit immediately after `v` comman
d) and push it on the stack
| :[0-9a-f] | pops the top of the stack and writes it into one gobvar
|+ - * / | pops two values from the stack, perform the corresponding arithmetic operation (with 16-bit signed arit
hmetic) and pushes back the result on the stack. 

So if we want to compute 42 / 7, we'll use the expression `42 7 /`, which pushes 42, then push 7 (which is thus on
 the top of the stack) and finally apply the division to get 6 on the top of the stack.
If we want to increase some value in entity variable 3, we will do `v3 2 + :3` which 
- pushes the current contents of the gobvar (let's say 40),
- then pushes the constant '2',
- then performs the addition and place the result (42) instead of the two operands,
- finally consume that value to write it back in the gobvar.

=== Additional operation available:
| ~ | replaces the top-of-stack (TOS) with its arithmetic opposite (0 - TOS)
| % | replaces the 2 top stack elements with the remainder of before-top-of-stack (BTOS) divided TOS (modulo opera
tor)
| m | replaces BTOS and TOS by the minimum of both values.
| M | replaces BTOS and TOS by the maximum of both values.
|`|`| replaces BTOS and TOS by the bitwise-OR of both values
|`&`| replaces BTOS and TOS by the bitwise AND of both values
|`^`| replaces BTOS and TOS by the bitwise XOR of both values
| ! | replaces TOS with the bitwise-NOT of its value.

=== Comparison:

The evaluator will consider any non-0 value as a boolean 'true', while 0 is considered 'false'. Comparison operator produce pure 0/1 results so that we can then use bitwise OR/AND/XOR' to combine their results. All the operators below pop BTOS and TOS and push a boolean result
| = | true if BTOS equals TOS.
| != | true unless BTOS equals TOS.
| < | true if BTOS is stricly less than TOS. `1 2 <` is true.
| > | true if BTOS is stricly greater than TOS. `2 1 >` is true.
| <= | true if BTOS is less than or equal to TOS.
| >= | true if BTOS is greater than or equal to TOS.
| ? | bit-wise test: apply bitwise AND between BTOS and TOS and produce true if and only if the result is equal to TOS. 
| !? | negative bitwise test. Apply the bitwise test, but produce false if and only if the result is equal to TOS.

