# check a file (the usual case)

./scripts/checkpatch.pl --no-tree --file --terse --show-types arch/x86_64/scheduler/process.c

# check what's about to be commited

git diff | ./scripts/checkpatch.pl --no-tree --no-signoff -

# check commits already made

./scripts/checkpatch.pl --no-tree --no-signoff -g HEAD
./scripts/checkpatch.pl --no-tree --no-signoff -g master~5..HEAD
