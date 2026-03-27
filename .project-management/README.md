# Project Management

This directory contains the project management documentation for the C Compiler refactoring effort.

## Documents

### EPICS.md
Detailed epics and sprint breakdown for implementing the Distinguished Engineer review recommendations.

### SPRINTS.md
Detailed sprint tasks with acceptance criteria and deliverables.

## Quick Reference

| Epic | Sprints | Duration |
|------|---------|----------|
| 1. Stabilize Compiler | 4 | 4 weeks |
| 2. Architecture Refactoring | 3 | 3 weeks |
| 3. Feature Completion | 4 | 4 weeks |
| 4. Self-Hosting | 6 | 6 weeks |
| **Total** | **17** | **~4 months** |

## Current Status

| Epic | Status | Notes |
|------|--------|-------|
| Epic 1 | Not Started | Recommended first |
| Epic 2 | Not Started | Depends on Epic 1 |
| Epic 3 | Not Started | Can run parallel to Epic 2 |
| Epic 4 | Not Started | Final goal |

## Key Milestones

1. **Milestone 1**: All tests passing (end of Epic 1)
2. **Milestone 2**: Clean architecture (end of Epic 2)
3. **Milestone 3**: Feature complete (end of Epic 3)
4. **Milestone 4**: Self-hosting (end of Epic 4)

## Recommendations

1. Start with Epic 1 (Stabilize) to fix current issues
2. Work on Epic 2 (Architecture) in parallel for long-term health
3. Epic 3 (Features) can be done in parallel with Epic 2
4. Epic 4 (Self-Hosting) is the final validation milestone

## Tracking Progress

Update the following files to track progress:
- `EPICS.md` - High-level status
- `SPRINTS.md` - Detailed task completion

## Definition of Done

Each sprint is complete when:
- All acceptance criteria met
- All tests pass
- Code reviewed
- Documentation updated
- No regressions

---

*Last Updated: 2026-03-27*
