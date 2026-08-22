#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace Remote
{

struct UploadNegotiation
{
    std::unordered_map<std::string, std::string> uploadUrls;
};

class RemoteCASClient
{
private:
    std::string baseUrl;

public:
    explicit RemoteCASClient(const std::string& baseUrl);

    bool healthCheck() const;

    UploadNegotiation negotiateUpload(
        const std::vector<std::string>& objectIds
    ) const;
};

}