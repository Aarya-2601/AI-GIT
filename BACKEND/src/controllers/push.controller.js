const {
    upload_url,
    object_exists
} = require('../services/minioservice.js');

const {
    saveRepository
} = require('../services/repositoryservice.js');

const try_push = async (req, res) => {

    try {

        const { chunks } = req.body;

        if (!chunks || !Array.isArray(chunks))
        {
            return res.status(400).send({
                status: 'error',
                message: 'Chunks are required and should be an array'
            });
        }


        const upload_urls = {};
        const existing_chunks = [];


        for (const hash of chunks) {
            const exists = await object_exists(hash);
            if (exists)
            {
                existing_chunks.push(hash);
                continue;
            }
            upload_urls[hash] = await upload_url(hash);
        }


        return res.status(200).send({

            status: 'ok',
            existing_chunks: existing_chunks,
            upload_urls: upload_urls

        });

    }
    catch (error) {

        console.error('Error in try_push:', error);

        return res.status(500).send({

            status: 'error',
            message: 'Failed to negotiate upload',
            error: error.message

        });
    }
};


const finalize_push = async (req, res) => {

    try {

        const {
            repo,
            head,
            refs
        } = req.body;


        if (
            !repo ||
            typeof repo !== 'string'
        ) {

            return res.status(400).send({
                status: 'error',
                message: 'Repository name is required'
            });
        }


        if (
            !head ||
            typeof head !== 'string'
        ) {

            return res.status(400).send({
                status: 'error',
                message: 'HEAD reference is required'
            });
        }


        if (
            !refs ||
            typeof refs !== 'object' ||
            Array.isArray(refs)
        ) {

            return res.status(400).send({
                status: 'error',
                message: 'Repository refs are required'
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
                message: 'HEAD must point to a supplied ref'
            });
        }


        const commitHash = refs[head];

        if (
            !commitHash ||
            typeof commitHash !== 'string'
        ) {

            return res.status(400).send({
                status: 'error',
                message: 'HEAD ref must contain a commit hash'
            });
        }
        const commitExists =
            await object_exists(commitHash);


        if (!commitExists) {

            return res.status(409).send({
                status: 'error',
                message:
                    'Cannot finalize push because the HEAD commit is missing from remote storage',
                missing_object: commitHash
            });
        }


        const repository =
            await saveRepository(
                repo,
                head,
                refs
            );


        return res.status(200).send({

            status: 'ok',

            message: 'Push finalized successfully',

            repository: repository

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
                message: error.message
            });
        }


        return res.status(500).send({

            status: 'error',

            message: 'Failed to finalize push',

            error: error.message

        });
    }
};


module.exports = {
    try_push,
    finalize_push
};