const express= require('express');  //api framework likhne ke liye
const cors= require('cors');  //cross origin handler
require('dotenv').config();  //required to read the .env file

const app=express();
const{bucket_exists}=require('./services/minioservice.js'); //importing the bucket exists function from minio

const PORT=process.env.port||3000;

//apply json parser middleware
app.use(cors());
app.use(express.json());

//get's health check, see if the website is running fine
app.get('/health', (req,res)=>{
    res.status(200).send({
        status: 'ok',
        service: 'ai-git backend'
    });
});

//put 
app.post('/api/v1/push/negotiate', (req, res)=>{
    const {chunks}=req.body;

    if(!chunks || !Array.isArray(chunks)){
        return res.status(400).send({
            status: 'error',
            message: 'chunks is required and should be an array'
        });
    }

    res.status(200).send({
        missing_chunks: chunks.map(chunk=>chunk.sha256),
        upload_urls: {}
    });
});

app.listen(PORT, async()=> {
    console.log(`AI-Git backend live on http://localhost:${PORT}`);
    try{
        await bucket_exists();
    }
    catch(err){
        console.error(`Failed to initialized MinIo bucket on startup`);
    }
}
);