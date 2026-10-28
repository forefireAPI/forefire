#!/bin/bash
# run.bash writes nothing to disk any more — it reads the RMSE off ANN_test's
# stdout — so there is nothing to clean. Kept because tests/run.bash calls a
# clean.bash in each suite directory.
#
# The old result.txt this used to remove was named results.txt here, so it
# never matched anything either.
exit 0
