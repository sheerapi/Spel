#!/bin/sh
set -eu

TAG=$(git describe --dirty --always --tags)
BRANCH=$(git rev-parse --abbrev-ref HEAD)
DIFF=$(git diff HEAD | git hash-object --stdin)
COMMIT=$(git log --pretty=format:%H -n 1)
DIRTY=$(git status --porcelain -uall | wc -l)

awk \
  -v tag="$TAG" \
  -v commit="$COMMIT" \
  -v branch="$BRANCH" \
  -v diff="$DIFF" \
  -v dirty="$DIRTY" '
{
    sub(/@VCS_TAG@/, tag)
    sub(/@VCS_COMMIT@/, commit)
    sub(/@VCS_BRANCH@/, branch)
    sub(/@VCS_DIFF_HASH@/, diff)
    sub(/@VCS_DIRTY_COUNT@/, dirty)
    print
}
' "$1" > "$2"