#pragma once

namespace Commands
{
    // One-time migration pass: rewrites every legacy (pre-header) object
    // in .aigit/objects into the current header'd/compressed format in
    // place, preserving IDs exactly. Safe to run on an already-migrated
    // or mixed repo (already-current objects are left untouched), and
    // safe to interrupt/re-run.
    int runMigrate();
}
