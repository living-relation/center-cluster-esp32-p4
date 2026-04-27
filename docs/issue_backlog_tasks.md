# Codebase issue triage: proposed tasks

## 1) Typo fix task
**Title:** Correct CAN hardware spelling typo in README

**Problem found:** README currently says "small can transciever".

**Proposed work:**
- Replace `transciever` with `transceiver` in the CANBUS setup section.
- Quick scan for nearby grammar issues in that paragraph to improve clarity.

**Acceptance criteria:**
- README uses the correct term `transceiver` in the CANBUS setup section.

## 2) Bug fix task
**Title:** Resolve GPIO pin conflict between tach input and CAN TX

**Problem found:**
- Tach input is configured on `GPIO_NUM_5`.
- CAN TX is also configured on `GPIO_NUM_5`.

These features cannot safely operate together with the same pin assignment.

**Proposed work:**
- Reassign either `TACH_GPIO` or `CAN_TX` to a non-conflicting pin.
- Add a compile-time guard (or config check) to fail fast if both are set to the same GPIO.
- Verify both tach and CAN traffic function after reassignment.

**Acceptance criteria:**
- No shared GPIO assignment between tach input and CAN TX.
- Build-time or startup-time check prevents accidental future conflicts.
- Manual validation confirms both tach updates and CAN reception work.

## 3) Code comment/documentation discrepancy task
**Title:** Fix mismatched RPM filter comment for `MAX_PERIOD_CHANGE`

**Problem found:** `MAX_PERIOD_CHANGE` is set to `0.12f` while the comment says `15% allowed change`.

**Proposed work:**
- Align the comment and value so they describe the same threshold (either 12% or 15%).
- If changing behavior, mention rationale in commit message/changelog.

**Acceptance criteria:**
- The constant value and its inline comment are consistent.

## 4) Test improvement task
**Title:** Add protocol loader reset/detection regression tests

**Problem found:** `protocol_loader_init()` resets `protocol_count` and `active_protocol`, but does not explicitly reset `protocol_hits[]` and `detection_done`. This can cause stale detection state if init is ever called more than once in a runtime/session.

**Proposed work:**
- Add tests covering repeated calls to `protocol_loader_init()`.
- Verify `detection_done` and per-protocol hit counters start clean after re-init.
- Add a negative test where detection should not remain locked from previous runs.

**Acceptance criteria:**
- New tests fail before the reset fix and pass after.
- Re-initialization always produces a clean protocol detection state.
