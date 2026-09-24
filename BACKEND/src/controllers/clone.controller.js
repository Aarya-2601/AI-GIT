const {
    download_url,
    object_exists
} = require('../services/minioservice.js');

const {
    getRepository
} = require('../services/repositoryservice.js');


// Return the remote repository state together with download URLs
// for every object currently stored in the repository.
//
// For the presentation prototype, the client will use these objects
// to rebuild the local CAS/VCS state.
const try_clone = async (req, res) => {

    try {

        const { repoName } = req.params;


        if (
            !repoName ||
            typeof repoName !== 'string'
        ) {
            return res.status(400).send({
                status: 'error',
                message: 'Repository name is required'
            });
        }


        const repository =
            await getRepository(repoName);


        if (!repository) {
            return res.status(404).send({
                status: 'error',
                message: `Repository '${repoName}' was not found`
            });
        }


        /*
         * repositoryservice stores the root VCS state:
         *
         * HEAD -> branch -> commit
         *
         * At minimum verify that the published HEAD commit still
         * exists in MinIO before advertising the repository.
         */
        const headRef =
            repository.head;

        const headCommit =
            repository.refs?.[headRef];


        if (!headCommit) {
            return res.status(500).send({
                status: 'error',
                message: 'Remote repository metadata is invalid'
            });
        }


        const headExists =
            await object_exists(headCommit);


        if (!headExists) {
            return res.status(500).send({
                status: 'error',
                message:
                    'Remote repository is incomplete: HEAD commit is missing'
            });
        }


        /*
         * We cannot yet derive the complete reachable object graph
         * server-side because MinIO stores opaque content-addressed
         * objects.
         *
         * The next CLI step will therefore request/download objects
         * using the remote object catalogue supplied by the backend.
         *
         * For now this endpoint establishes the repository identity
         * and VCS root state needed by clone/pull.
         */
        return res.status(200).send({

            status: 'ok',

            repository: {
                name: repository.name,
                head: repository.head,
                refs: repository.refs,
                updated_at: repository.updated_at
            }

        });

    }
    catch (error) {

        console.error(
            'Error in try_clone:',
            error
        );


        if (
            error.message ===
            'Invalid repository name'
        ) {
            return res.status(400).send({
                status: 'error',
                message: error.message
            });
        }


        return res.status(500).send({
            status: 'error',
            message: 'Failed to initiate clone',
            error: error.message
        });
    }
};


module.exports = {
    try_clone
};