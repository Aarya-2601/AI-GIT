const {
    download_url,
    object_exists
} = require('../services/minioservice.js');

const {
    getRepository
} = require('../services/repositoryservice.js');


// ------------------------------------------------------------
// Clone repository
//
// Returns:
//
// repository:
//   name
//   HEAD
//   refs
//
// download_urls:
//   objectHash -> presigned MinIO GET URL
//
// The backend does NOT reconstruct files.
// The CLI downloads the CAS/VCS objects and then uses the existing
// checkout + StorageManager reconstruction machinery.
// ------------------------------------------------------------

const try_clone = async (req, res) => {

    try {

        const { repoName } =
            req.params;


        if (
            !repoName ||
            typeof repoName !== 'string'
        ) {
            return res.status(400).send({
                status: 'error',
                message:
                    'Repository name is required'
            });
        }


        const repository =
            await getRepository(repoName);


        if (!repository) {

            return res.status(404).send({
                status: 'error',
                message:
                    `Repository '${repoName}' was not found`
            });
        }


        // ----------------------------------------------------
        // Validate repository metadata
        // ----------------------------------------------------

        if (
            !repository.head ||
            !repository.refs ||
            !Array.isArray(repository.objects)
        ) {
            return res.status(500).send({
                status: 'error',
                message:
                    'Remote repository metadata is incomplete'
            });
        }


        const headCommit =
            repository.refs[
                repository.head
            ];


        if (!headCommit) {

            return res.status(500).send({
                status: 'error',
                message:
                    'Remote HEAD does not point to a valid ref'
            });
        }


        if (
            !repository.objects.includes(
                headCommit
            )
        ) {
            return res.status(500).send({
                status: 'error',
                message:
                    'Remote HEAD commit is missing from the repository catalogue'
            });
        }


        // ----------------------------------------------------
        // Generate GET URLs for every object belonging to
        // this repository.
        // ----------------------------------------------------

        const download_urls = {};


        for (
            const objectId
            of repository.objects
        ) {

            const exists =
                await object_exists(
                    objectId
                );


            if (!exists) {

                return res.status(500).send({

                    status: 'error',

                    message:
                        'Remote repository is incomplete',

                    missing_object:
                        objectId

                });
            }


            download_urls[objectId] =
                await download_url(
                    objectId
                );
        }


        // ----------------------------------------------------
        // Complete clone manifest
        // ----------------------------------------------------

        return res.status(200).send({

            status: 'ok',

            repository: {

                name:
                    repository.name,

                head:
                    repository.head,

                refs:
                    repository.refs,

                updated_at:
                    repository.updated_at

            },

            download_urls:
                download_urls

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
                message:
                    error.message
            });
        }


        return res.status(500).send({

            status: 'error',

            message:
                'Failed to initiate clone',

            error:
                error.message

        });
    }
};


module.exports = {
    try_clone
};