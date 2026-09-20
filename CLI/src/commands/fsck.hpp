#pragma once

namespace Commands
{
    // Rebuilds .aigit/metadata.db from a full disk walk of .aigit/objects
    // (verifying every object against its own ID) and reports the result.
    // Returns 0 if every object on disk verified cleanly, 1 if any were
    // corrupt (reported, not deleted -- see AIGIT_NOTES.md step 11 for
    // gc, which is the step that actually removes unreachable objects).
    int runFsck();
}
