## Summary

<!-- One paragraph describing what this PR does and why. -->

## Type of Change

- [ ] Bug fix
- [ ] New feature
- [ ] Refactor (no behaviour change)
- [ ] Documentation
- [ ] Chore / tooling

## Changes

<!-- Bullet list of the key changes made. -->

-
-

## Testing

<!-- How was this tested? -->

- [ ] Backend compiles: `cmake --build build`
- [ ] No new compiler warnings
- [ ] End-to-end test passes: `wsl -e bash test_e2e.sh`
- [ ] Manual test: describe what you tested

## Architecture Checklist

- [ ] Controllers contain HTTP logic only (no business logic)
- [ ] Services contain no HTTP types
- [ ] Any new SQL uses prepared statements (no string concatenation)
- [ ] New C++ files have the standard Doxygen file header
- [ ] `noexcept` methods wrap body in `try/catch` and log errors

## Related Issues

<!-- Closes #xxx -->
