## About
The ```os.c``` file in this repo is a copy of the original file provided with the first homework assignment edited to include two test suites. The tests themselves were almost entirely sourced from the shared TAU Computer Science Drive - credit goes out to the people who originally wrote them. I modified them to also include the basic set of tests provided in the original file and to make it possible to run them without using ```<math.h>```.  
## How to use these tests
1. Replace the original ```os.c``` file in your project directory with the file in this repo.
2. Compile your code along with the test file using ```gcc -O3 -Wall -std=c11 os.c pt.c```
3. Run the resulting executable. The test results will show up as output.
4. After running the tests and before submitting your solution, it is recommended to ensure that your code runs with the original ```os.c``` on the university's Nova server.

Please note that the first test suite is randomized and may therefore produce inconsistent results when run with **incorrect** code. Correct code should always pass both suites.
## Contributing
If you come across what you belive to be unexpected test behavior or edge cases that are not covered by the tests in their current form, please make sure there are no problems with your code. If not, you are welcome to submit a PR with a fix.
