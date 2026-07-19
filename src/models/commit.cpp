#include "models/commit.hpp"
#include <sstream>
#include <utility>  //low level library 

namespace Models {

    Commit::Commit(std::string treeHash, std::vector<std::string> parents, 
                   std::string author, std::string committer, 
                   std::string message, long long timestamp)
        : mtreeHash(std::move(treeHash)), mparentHashes(std::move(parents)),
          mauthor(std::move(author)), mcommitter(std::move(committer)),
          mmessage(std::move(message)), mtimestamp(timestamp) {}

    std::string Commit::serialize() const {
        std::ostringstream body;

        // 1. Point directly to the root snapshot tree directory structure
        body << "tree " << mtreeHash << "\n";

        // 2. Track parent commits (empty if initial commit, multiple if merge commit)
        for (const auto& parent : mparentHashes) {
            body << "parent " << parent << "\n";
        }

        // 3. Inject user timelines (+0000 tracks standard UTC clock states)
        body << "author " << mauthor << " " << mtimestamp << " +0000\n";
        body << "committer " << mcommitter << " " << mtimestamp << " +0000\n";

        // 4. Git isolating standard requires a clean blank newline separating the header and message body
        body << "\n" << mmessage << "\n";

        std::string commitContent = body.str();

        // 5. Encapsulate data payload with standard top-level object headers
        std::string header = "commit " + std::to_string(commitContent.size());
        header.push_back('\0');

        return header + commitContent;
    }
}