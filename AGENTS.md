- Everything should be cross-compatible between Linux and Windows.
- When running `ezbuild.sh Debug` for verification, prefer the repo-local ignored build directory
  `build/Debug` so build artifacts stay in the working tree but remain untracked by Git.

## Agent skills

### Issue tracker

Issues and PRDs are tracked in GitHub Issues for `nick-cuttle/GameEngine`; use the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Use the default three-label triage vocabulary: `needs-triage`, `needs-info`, `wontfix`. Issues are passed to a human by default. See `docs/agents/triage-labels.md`.

### Domain docs

This repo uses a single-context layout: root `CONTEXT.md` and root `docs/adr/` for ADRs. See `docs/agents/domain.md`.
