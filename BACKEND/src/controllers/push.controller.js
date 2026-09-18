const {
    upload_url,
    object_exists
} = require('../services/minioservice.js');


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
            const exists = await object_exists(hash);


            if (exists) {

              
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


module.exports = {
    try_push
};