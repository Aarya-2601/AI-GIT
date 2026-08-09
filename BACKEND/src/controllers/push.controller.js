const {upload_url}=require('../services/minioservice.js');

//request is the whole object which has the chunks as its body
try_push=async(req, res)=>{
    try{
        const {chunks}=req.body;
        if(!chunks || !Array.isArray(chunks)){
            return res.status(400).send({
                status: 'error',
                message: 'Chunks are required and should be an array'
            });
        }
        const upload_urls={};
        for(const hash of chunks){
            upload_urls[hash]=await upload_url(hash);  //minIo will generate presigned url
        }
        return res.status(200).send({
            status: 'ok',
            upload_urls: upload_urls
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
    try_push: try_push,
};