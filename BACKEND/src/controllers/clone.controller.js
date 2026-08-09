const {download_url}=require('../services/minioservice.js');
const db=require('../services/minioservice.js')

const try_clone=async(req, res)=>{
    try{
        const {repoName}=req.params;
        if(!reponame){
            return res.status(400).send({
                status: 'error',
                message: 'Repository name is required'
            });
        }
        
        //postgre_sql ka block of code 
        //query postgreSQL for the latest commit
        //query it for all the file paths and chunk hashes

        //generate download urls for it
        const download_urls={};
        for(const file of postgre_sql.file){
            download_urls[files]= await download_url(file.chunk);
        }

        //return complete info
        return res.status(200).send({
        status: 'ok',
        //manifest:{
        //repo, commit, commit created at
        //}
        download_urls: download_urls
        });
    }
    catch(error){
        console.error('Error in try_clone:', error);
        return res.status(500).send({
            status: 'error',
            message: 'Failed to initiate clone',
            error: error.message
        });
    }
}

module.exports={
    try_clone,
};