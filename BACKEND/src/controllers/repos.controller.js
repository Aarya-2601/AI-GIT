const {
    getRepository
} = require('../services/repositoryservice.js');


// ------------------------------------------------------------
// GET /api/v1/repos/:repoName
//
// Lightweight read-only metadata endpoint.
// Reads repository state from the JSON file written by finalize_push.
//
// Actual on-disk schema (confirmed from aigit-e2e-test.json):
//   name       : string  — repository name
//   head       : string  — full ref path, e.g. "refs/heads/main"
//   refs       : object  — { "refs/heads/main": "<sha256>" }
//   objects    : string[] — array of SHA-256 content hashes (NOT returned)
//   updated_at : string  — ISO-8601 timestamp (snake_case)
//
// Returns safe metadata only. The objects array is stripped and replaced
// with objectCount. No MinIO calls are made.
// ------------------------------------------------------------

const get_repo_meta = async (req, res) => {

    try {

        const { repoName } = req.params;

        const repository =
            await getRepository(repoName);


        if (!repository) {

            return res.status(404).send({
                status: 'error',
                message:
                    `Repository '${repoName}' was not found`
            });
        }


        return res.status(200).send({

            status: 'ok',

            repository: {
                name:        repository.name,
                head:        repository.head,
                refs:        repository.refs,
                objectCount: Array.isArray(repository.objects)
                                 ? repository.objects.length
                                 : 0,
                updated_at:  repository.updated_at
            }

        });

    }
    catch (error) {

        if (error.message === 'Invalid repository name') {

            return res.status(400).send({
                status: 'error',
                message: error.message
            });
        }


        console.error(
            'Error in get_repo_meta:',
            error
        );


        return res.status(500).send({

            status: 'error',

            message:
                'Failed to fetch repository metadata',

            error:
                error.message

        });
    }
};


module.exports = { get_repo_meta };
