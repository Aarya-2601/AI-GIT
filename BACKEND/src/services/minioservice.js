//auto-generation of presigned URLs
const {client}=require('../config/storage.js')

const BUCKET_NAME=process.env.MINIO_BUCKET_NAME || 'aigit-chunks';
const EXPIRY_SECONDS=3600 //urls will remain valid for 1 hr

const bucket_exists=async()=>{
    try{
        const exists=await client.bucketExists(BUCKET_NAME);
        if(!exists){
            await client.makeBucket(BUCKET_NAME);
            console.log(`Created MinIO bucket: ${BUCKET_NAME}`);
        }
    }
    catch(error){
        console.error('Error verifying MinIO bucket status:', error);
        throw error;
    }
};

const upload_url=async(chunkHash)=>{
    try{
        return await client.presignedPutObject(BUCKET_NAME, chunkHash, EXPIRY_SECONDS);
    }
    catch(error){
        console.error(`Failed to generate upload URL for chunk ${chunkHash}:`, error);
        throw error;
    }
};

//parameter is a string chunkHash
//returns a promise which reolves into a string which is presigned url for the chunk

const download_url=async(chunkHash)=>{
    try{
        return await client.presignedGetObject(BUCKET_NAME, chunkHash, EXPIRY_SECONDS);
    }
    catch(error){
        console.error(`Failed to generate download URL for chunk ${chunkHash}:`, error);
        throw error;
    }
};

//stores as a js literal in key value pairs, where key and value are same
module.exports={
    bucket_exists: bucket_exists,
    upload_url: upload_url,
    download_url: download_url,
};