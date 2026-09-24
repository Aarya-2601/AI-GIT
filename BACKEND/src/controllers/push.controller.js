const {
    upload_url,
    object_exists
} = require('../services/minioservice.js');

const {
    saveRepository
} = require('../services/repositoryservice.js');


// ------------------------------------------------------------
// PHASE 1: NEGOTIATE
//
// Client sends all local object hashes.
// Backend returns upload URLs only for objects missing in MinIO.
// ------------------------------------------------------------

const try_push = async (req, res) => {

    try {

        const { chunks } = req.body;


        if (!chunks || !Array.isArray(chunks)) {

            return res.status(400).send({
                status: 'error',
                message: 'Chunks are required and should be an array'
            });
        }


        const upload_urls = {};
        const existing_chunks = [];


        for (const hash of chunks) {

            const exists =
                await object_exists(hash);


            if (exists) {

                existing_chunks.push(hash);

                continue;
            }


            upload_urls[hash] =
                await upload_url(hash);
        }


        return res.status(200).send({

            status: 'ok',

            existing_chunks:
                existing_chunks,

            upload_urls:
                upload_urls

        });

    }
    catch (error) {

        console.error(
            'Error in try_push:',
            error
        );


        return res.status(500).send({

            status: 'error',

            message:
                'Failed to negotiate upload',

            error:
                error.message

        });
    }
};


// ------------------------------------------------------------
// PHASE 2: FINALIZE
//
// Called only after all missing objects have been uploaded.
//
// Stores:
//
// repo
//   ├── HEAD
//   ├── refs
//   └── complete object catalogue
//
// Clone/pull can later use this catalogue to determine exactly
// which content-addressed objects belong to this repository.
// ------------------------------------------------------------

const finalize_push = async (req, res) => {

    try {

        const {
            repo,
            head,
            refs,
            objects
        } = req.body;


        if (
            !repo ||
            typeof repo !== 'string'
        ) {

            return res.status(400).send({
                status: 'error',
                message:
                    'Repository name is required'
            });
        }


        if (
            !head ||
            typeof head !== 'string'
        ) {

            return res.status(400).send({
                status: 'error',
                message:
                    'HEAD reference is required'
            });
        }


        if (
            !refs ||
            typeof refs !== 'object' ||
            Array.isArray(refs)
        ) {

            return res.status(400).send({
                status: 'error',
                message:
                    'Repository refs are required'
            });
        }


        if (
            !objects ||
            !Array.isArray(objects)
        ) {

            return res.status(400).send({
                status: 'error',
                message:
                    'Repository object catalogue is required'
            });
        }


        if (
            !Object.prototype.hasOwnProperty.call(
                refs,
                head
            )
        ) {

            return res.status(400).send({
                status: 'error',
                message:
                    'HEAD must point to a supplied ref'
            });
        }


        const commitHash =
            refs[head];


        if (
            !commitHash ||
            typeof commitHash !== 'string'
        ) {

            return res.status(400).send({
                status: 'error',
                message:
                    'HEAD ref must contain a commit hash'
            });
        }


        /*
         * HEAD's commit should itself be part of the catalogue.
         */
        if (!objects.includes(commitHash)) {

            return res.status(400).send({
                status: 'error',
                message:
                    'HEAD commit is missing from repository object catalogue'
            });
        }


        /*
         * Most important consistency check:
         *
         * Before publishing repository state, verify that EVERY object
         * advertised by the repository actually exists remotely.
         *
         * Otherwise clone could receive metadata for an incomplete
         * repository.
         */

        for (const objectId of objects) {

            const exists =
                await object_exists(objectId);


            if (!exists) {

                return res.status(409).send({

                    status: 'error',

                    message:
                        'Cannot finalize an incomplete repository',

                    missing_object:
                        objectId

                });
            }
        }


        const repository =
            await saveRepository(
                repo,
                head,
                refs,
                objects
            );


        return res.status(200).send({

            status: 'ok',

            message:
                'Push finalized successfully',

            repository:
                repository

        });

    }
    catch (error) {

        console.error(
            'Error in finalize_push:',
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
                'Failed to finalize push',

            error:
                error.message

        });
    }
};


module.exports = {
    try_push,
    finalize_push
};