#pragma once

namespace Commands
{
    // Mark-and-sweep garbage collection: marks every object reachable
    // from a branch ref (walking commit history -> trees -> blobs ->
    // manifest chunks) or from the staging index, then deletes every
    // object on disk that wasn't marked. Never deletes anything
    // reachable from a ref or the index.
    int runGc();
}
