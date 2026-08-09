const {download_url}=require('../services/minioservice.js')

try_pull= async (req, res)=>{
    try{
        const {chunks}=req.body;
        if(!chunks || !Array.isArray(chunks)){
            return res.status(400).send({
                status: 'error',
                message: 'Chunks are required and should be an array'
            });
        }
        const download_urls={};
        for(const hash of chunks){
            download_urls[hash]=await download_url(hash);
        }
        return res.status(200).send({
            status: 'ok',
            download_urls: download_urls
        });
    }
    catch(error){
        console.error('Error in try_push:', error);
        return res.status(500).send({
        status: 'error',
        message: 'Failed to generate upload URLs',
        error: error.message
        });
    }
};

module.exports={

};