# Stacked Branches

Use a stacked branch when one issue depends on work that is already on another branch but has not
merged to `main` yet.

Example: issue `#23` depends on `22-clear-primary-window-through-opengl-renderer`.

## Start From The Parent Branch

Fetch the latest remote branches:

```bash
git fetch origin
```

Create a bookmark for the parent branch tip. This is optional, but useful if the parent branch gets
deleted after merge:

```bash
git branch base-22-for-23 origin/22-clear-primary-window-through-opengl-renderer
```

Create your issue branch from the parent branch:

```bash
git switch 22-clear-primary-window-through-opengl-renderer
git pull --ff-only origin 22-clear-primary-window-through-opengl-renderer
git switch -c 23-your-issue-branch
```

## Open The Pull Request

Push your branch:

```bash
git push -u origin 23-your-issue-branch
```

Open the pull request with this base and compare:

```text
base: 22-clear-primary-window-through-opengl-renderer
compare: 23-your-issue-branch
```

This keeps the review focused on the `#23` commits instead of showing both `#22` and `#23`.

## If The Parent Branch Merges Before You Finish

Fetch the updated remote state:

```bash
git fetch origin
```

If the parent branch was merged normally, a plain rebase is usually enough:

```bash
git switch 23-your-issue-branch
git rebase origin/main
```

If the parent branch was squash-merged, rebased before merge, or deleted, use the bookmark:

```bash
git switch 23-your-issue-branch
git rebase --onto origin/main base-22-for-23
```

Then change the pull request base to:

```text
base: main
compare: 23-your-issue-branch
```

## Mental Model

If your branch starts like this:

```text
main -- A -- B        # parent issue work
             \
              C -- D  # your issue work
```

`base-22-for-23` points at `B`. After the parent issue merges, this command moves only `C -- D` onto
`main`:

```bash
git rebase --onto origin/main base-22-for-23
```
